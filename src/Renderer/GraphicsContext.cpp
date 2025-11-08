#include "GraphicsContext.h"

#include "Core\Application.h"

#include <GLFW/glfw3.h>
#include <webgpu.h>

#include <glfw3webgpu.h>

// Ensure the Windows-specific structs are enabled in webgpu.h
#ifndef WGPU_TARGET_WINDOWS
#define WGPU_TARGET_WINDOWS 1
#endif
#include <Windows.h>

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

    m_Surface = glfwGetWGPUSurface(m_Instance, m_WindowHandle);

    m_Config.nextInChain = nullptr;
    m_Config.width = Application::Get().GetWindow().GetWidth();
    m_Config.height = Application::Get().GetWindow().GetHeight();

    WGPUSurfaceCapabilities caps{};
    wgpuSurfaceGetCapabilities(m_Surface, m_Adapter, &caps);

    WGPUTextureFormat surfaceFormat = caps.formats[0];

    m_Config.format = surfaceFormat;
    m_Config.viewFormatCount = 0;
    m_Config.viewFormats = nullptr;
    
    wgpuSurfaceConfigure(m_Surface, &m_Config);

    WGPURequestAdapterOptions adapterOpts{};      
    adapterOpts.nextInChain = nullptr;
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapterOpts.compatibleSurface = m_Surface;

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
    wgpuSurfaceUnconfigure(m_Surface);
    wgpuSurfaceRelease(m_Surface);
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
