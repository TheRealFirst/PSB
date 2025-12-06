#pragma once
#include "Core/Core.h"
#include "Buffers/VertexBuffer.h"
#include "Buffers/IndexBuffer.h"

#include <webgpu/webgpu.h>

#include <glm\glm.hpp>

struct GLFWwindow;

namespace PSB
{
	class GraphicsContext
	{
	public:
		GraphicsContext();
		GraphicsContext(GLFWwindow* windowHandle);
		~GraphicsContext() = default;

		void Init(uint32_t width, uint32_t height);
		void Delete();
		void SwapBuffers();

		void SetClearColor(glm::vec4 clearColor);
	private:
		// Internal structures
		struct MyUniforms {
			glm::vec4 color{ 0.0f };
			float time = 1.0f;
			float _pad[3];
		};

		static_assert(sizeof(MyUniforms) % 16 == 0);
	private:
		//Internal functions
		WGPUAdapter RequestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options);
		WGPUDevice RequestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor);
		std::pair<WGPUSurfaceTexture, WGPUTextureView> GetNextSurfaceViewData();
		void InitializePipeline();
		void InitializeBuffers();
		WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter) const;
		void InitializeBindGroups();
	private:
		// Internal variables
		WGPUInstance m_Instance = nullptr;
		WGPUAdapter m_Adapter = nullptr;
		WGPUDevice m_Device = nullptr;
		WGPUQueue m_Queue = nullptr;
		WGPURenderPipeline m_Pipeline = nullptr;
		GLFWwindow* m_WindowHandle = nullptr;
		WGPUSurface m_Surface = nullptr;
		WGPUTextureFormat m_SurfaceFormat = WGPUTextureFormat_Undefined;
		WGPUSurfaceConfiguration m_Config{};
		WGPURenderPassColorAttachment m_ColorAttachment{};
		WGPURenderPassDescriptor m_RenderPassDesc{};


		glm::vec4 m_ClearColor{0.01f, 0.01f, 0.01f, 1.0f};

		VertexBuffer m_VertexBuffer;
		uint32_t m_VertexCount;

		IndexBuffer m_IndexBuffer;
		uint32_t m_IndexCount;

		WGPUBuffer m_UniformBuffer;
		WGPUPipelineLayout m_Layout;
		WGPUBindGroupLayout m_BindGroupLayout;
		WGPUBindGroup m_BindGroup;
	};
}