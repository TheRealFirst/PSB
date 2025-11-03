#include "GraphicsContext.h"


PSB::GraphicsContext::GraphicsContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
{
	LOG_ASSERT(windowHandle, "Window Handle is null");
}

void PSB::GraphicsContext::Init()
{
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	m_Instance = wgpuCreateInstance(&desc);

	
	LOG_ASSERT(m_Instance && "Could not initialize WGPU!");
	

	LOG_DEBUG("WGPU Instance: " + m_Instance);

	glfwMakeContextCurrent(m_WindowHandle);

	LOG_DEBUG("Requesting WGPUAdapter...");

	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	m_Adapter = RequestAdapterSync(m_Instance, &adapterOpts);

	LOG_DEBUG("Got adapter: " + m_Adapter);

	wgpuInstanceRelease(m_Instance);

	LOG_DEBUG("Requesting WGPUDevice...");

	WGPUDeviceDescriptor deviceDesc = {};

	deviceDesc.nextInChain = nullptr;
	// deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	// deviceDesc.defaultQueue.label = "The default queue";

	m_Device = RequestDeviceSync(m_Adapter, &deviceDesc);

	LOG_DEBUG("Got device: " + m_Device);

	m_Queue = wgpuDeviceGetQueue(m_Device);
}

void PSB::GraphicsContext::Delete()
{
	wgpuAdapterRelease(m_Adapter);
	wgpuDeviceRelease(m_Device);
	wgpuQueueRelease(m_Queue);
}

void PSB::GraphicsContext::SwapBuffers()
{
}

WGPUAdapter PSB::GraphicsContext::RequestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options)
{
	WGPUAdapter outAdapter = nullptr;

	WGPURequestAdapterCallbackInfo acb{};
	acb.mode = WGPUCallbackMode_WaitAnyOnly;
	acb.callback = (WGPURequestAdapterCallback)
		+[](WGPURequestAdapterStatus s, WGPUAdapter a, const char*, void* u) {
		if (s == WGPURequestAdapterStatus_Success)
			*reinterpret_cast<WGPUAdapter*>(u) = a;
		};
	acb.userdata1 = &outAdapter;

	WGPUFuture af = wgpuInstanceRequestAdapter(instance, options, acb);
	WGPUFutureWaitInfo awaitA{ .future = af };
	wgpuInstanceWaitAny(instance, 1, &awaitA, 0);

	return outAdapter;
}

WGPUDevice PSB::GraphicsContext::RequestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor)
{
	WGPUDevice outDevice = nullptr;

	WGPURequestDeviceCallbackInfo dcb{};
	dcb.mode = WGPUCallbackMode_WaitAnyOnly;
	dcb.callback = (WGPURequestDeviceCallback)
		+[](WGPURequestDeviceStatus s, WGPUDevice d, const char*, void* u) {
		if (s == WGPURequestDeviceStatus_Success)
			*reinterpret_cast<WGPUDevice*>(u) = d;
		};
	dcb.userdata1 = &outDevice;

	WGPUFuture df = wgpuAdapterRequestDevice(m_Adapter, descriptor, dcb);
	WGPUFutureWaitInfo awaitD{ .future = df };
	wgpuInstanceWaitAny(m_Instance, 1, &awaitD, 0);

	return outDevice;
}
