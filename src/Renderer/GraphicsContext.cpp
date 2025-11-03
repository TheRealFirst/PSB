#include "GraphicsContext.h"

// Must be defined BEFORE any GLFW include to get native Win32 helpers
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// Ensure the Windows-specific structs are enabled in webgpu.h
#ifndef WGPU_TARGET_WINDOWS
#define WGPU_TARGET_WINDOWS 1
#endif
#include <Windows.h>
#include <webgpu.h>

PSB::GraphicsContext::GraphicsContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
{
    // tinyLog macros are single-arg; pass one boolean
    LOG_ASSERT(m_WindowHandle && "Window Handle is null");
}

void PSB::GraphicsContext::Init()
{
    WGPUInstanceDescriptor desc{};
    m_Instance = wgpuCreateInstance(&desc);
    LOG_ASSERT(m_Instance);
    LOG_DEBUG("WGPU Instance created");

    LOG_DEBUG("Requesting WGPUAdapter...");
    WGPURequestAdapterOptions adapterOpts{};       // no surface
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;

    m_Adapter = RequestAdapterSync(m_Instance, &adapterOpts);
    LOG_ASSERT(m_Adapter);
    LOG_DEBUG("Got adapter.");

    LOG_DEBUG("Requesting WGPUDevice...");
    WGPUDeviceDescriptor deviceDesc{};
    m_Device = RequestDeviceSync(m_Adapter, &deviceDesc);
    LOG_ASSERT(m_Device);
    LOG_DEBUG("Got device.");

    m_Queue = wgpuDeviceGetQueue(m_Device);
  
}

void PSB::GraphicsContext::Delete()
{
    if (m_Queue) { wgpuQueueRelease(m_Queue);   m_Queue = nullptr; }
    if (m_Device) { wgpuDeviceRelease(m_Device); m_Device = nullptr; }
    if (m_Adapter) { wgpuAdapterRelease(m_Adapter); m_Adapter = nullptr; }
    if (m_Instance) { wgpuInstanceRelease(m_Instance); m_Instance = nullptr; }
}

void PSB::GraphicsContext::SwapBuffers()
{
    // Implement later: acquire surface texture, render, present.
    // Keeping empty is fine for now.
}

// --- Helpers updated to AllowSpontaneous + event pumping ---

WGPUAdapter PSB::GraphicsContext::RequestAdapterSync(WGPUInstance /*instance*/, WGPURequestAdapterOptions const* options)
{
    WGPUAdapter outAdapter = nullptr;

    WGPURequestAdapterCallbackInfo acb{};
    acb.mode = WGPUCallbackMode_AllowSpontaneous;
    acb.callback = (WGPURequestAdapterCallback)
        +[](WGPURequestAdapterStatus s, WGPUAdapter a, const char*, void* u) {
        if (s == WGPURequestAdapterStatus_Success)
            *reinterpret_cast<WGPUAdapter*>(u) = a;
        };
    acb.userdata1 = &outAdapter;

    wgpuInstanceRequestAdapter(m_Instance, options, acb);

    // Pump both Dawn events and GLFW to avoid a frozen window while waiting.
    while (!outAdapter) {
        wgpuInstanceProcessEvents(m_Instance);
        glfwPollEvents();
    }
    return outAdapter;
}

WGPUDevice PSB::GraphicsContext::RequestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor)
{
    WGPUDevice outDevice = nullptr;

    WGPURequestDeviceCallbackInfo dcb{};
    dcb.mode = WGPUCallbackMode_AllowSpontaneous;
    dcb.callback = (WGPURequestDeviceCallback)
        +[](WGPURequestDeviceStatus s, WGPUDevice d, const char*, void* u) {
        if (s == WGPURequestDeviceStatus_Success)
            *reinterpret_cast<WGPUDevice*>(u) = d;
        };
    dcb.userdata1 = &outDevice;

    wgpuAdapterRequestDevice(adapter, descriptor, dcb);

    while (!outDevice) {
        wgpuInstanceProcessEvents(m_Instance);
        glfwPollEvents();
    }
    return outDevice;
}
