#pragma once
#include "Core/Core.h"

#include <webgpu/webgpu.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
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

		bool Init(uint32_t width, uint32_t height);
		void Delete();
		void OnFrame();

		void SetClearColor(glm::vec4 clearColor);

		void OnWindowResize(uint32_t width, uint32_t height);
	private:
		// Internal structures
		struct MyUniforms {
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrix;
			glm::mat4 modelMatrix;
			glm::vec4 color;
			float time;
			float _pad[3];
		};


		static_assert(sizeof(MyUniforms) % 16 == 0);
	private:
		//Internal functions
		WGPUAdapter RequestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options);
		WGPUDevice RequestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor);
		std::pair<WGPUSurfaceTexture, WGPUTextureView> GetNextSurfaceViewData();
		
		WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter) const;
		

		bool InitializeWindowAndDevice();
		void TerminateWindowAndDevice();

		bool InitializeSwapChain();
		void TerminateSwapChain();

		bool InitializeAttachments();

		bool InitializeDepthBuffer();
		void TerminateDepthBuffer();

		bool InitializePipeline();
		void TerminatePipeline();

		bool InitializeTexture();
		void TerminateTextures();

		bool InitializeGeometry();
		void TerminateGeometry();

		bool InitializeUniforms();
		void TerminateUniforms();

		bool InitializeBindGroups();
		void TerminateBindGroups();

		void UpdateProjectionMatrix();
	private:
		uint32_t m_Width;
		uint32_t m_Height;

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

		WGPUShaderModule m_ShaderModule = nullptr;
		


		glm::vec4 m_ClearColor{0.01f, 0.01f, 0.01f, 1.0f};

		WGPUBuffer m_VertexBuffer = nullptr;
	

		uint32_t m_VertexCount;

		WGPUBuffer m_UniformBuffer = nullptr;
		WGPUPipelineLayout m_Layout = nullptr;
		WGPUBindGroupLayout m_BindGroupLayout = nullptr;
		WGPUBindGroup m_BindGroup = nullptr;

		WGPUTexture m_DepthTexture = nullptr;
		WGPUTextureView m_DepthTextureView = nullptr;
		WGPURenderPassDepthStencilAttachment m_DepthStencilAttachment = nullptr;
		WGPUTextureFormat m_DepthTextureFormat = WGPUTextureFormat_Depth24Plus;

		MyUniforms m_Uniforms;

		WGPUTexture m_Texture = nullptr;
		WGPUTextureView m_TextureView = nullptr;
		WGPUSampler m_Sampler = nullptr;
	};
}