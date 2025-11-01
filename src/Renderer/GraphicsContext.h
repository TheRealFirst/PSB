#pragma once
#include "Core\Core.h"
#include "GLFW\glfw3.h"
#include "webgpu.h"

namespace PSB
{
	class GraphicsContext
	{
	public:
		GraphicsContext(GLFWwindow* windowHandle);
		~GraphicsContext() = default;

		void Init();
		void Delete();
		void SwapBuffers();
	private:
		WGPUAdapter RequestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options);
		WGPUDevice RequestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor);
	private:
		WGPUAdapter m_Adapter;
		WGPUDevice m_Device;
		WGPUQueue m_Queue;
		GLFWwindow* m_WindowHandle;
	};
}