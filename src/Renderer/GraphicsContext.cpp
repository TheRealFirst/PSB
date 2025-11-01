#include "GraphicsContext.h"


PSB::GraphicsContext::GraphicsContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
{
	LOG_ASSERT(windowHandle, "Window Handle is null");
}

void PSB::GraphicsContext::Init()
{
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	WGPUInstance instance = wgpuCreateInstance(&desc);

	
	LOG_ASSERT(instance, "Could not initialize WGPU!");
	

	LOG_DEBUG("WGPU Instance: " + instance);

	glfwMakeContextCurrent(m_WindowHandle);

	LOG_DEBUG("Requesting WGPUAdapter...");

	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	m_Adapter = RequestAdapterSync(instance, &adapterOpts);

	LOG_DEBUG("Got adapter: " + m_Adapter);

	wgpuInstanceRelease(instance);

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
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData)
		{
			UserData& userData = *reinterpret_cast<UserData*>(pUserData);
			if (status == WGPURequestAdapterStatus_Success)
			{
				userData.adapter = adapter;
			}
			else {

				LOG_FATAL("Could not get WebGPU adapter.");
				LOG_FATAL(message);
			}
			userData.requestEnded = true;
		};

	wgpuInstanceRequestAdapter(
		instance /* equivalent of navigator.gpu */,
		options,
		onAdapterRequestEnded,
		(void*)&userData
	);

	LOG_ASSERT(userData.requestEnded);


	return userData.adapter;
}

WGPUDevice PSB::GraphicsContext::RequestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor)
{
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestDeviceStatus_Success) {
			userData.device = device;
		}
		else {
			LOG_FATAL("Could not get WebGPU device: ");
			LOG_FATAL(message);
		}
		userData.requestEnded = true;
		};

	wgpuAdapterRequestDevice(
		adapter,
		descriptor,
		onDeviceRequestEnded,
		(void*)&userData
	);


	LOG_ASSERT(userData.requestEnded);

	return userData.device;
}
