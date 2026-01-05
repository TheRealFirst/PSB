#pragma once

#include "core/Layer.h"
#include <vector>

#include <webgpu/webgpu.h>

#include "GUIItem.h"


#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PSB::GUI
{
	struct VertexAttributesUI
	{
		glm::vec2 position{ 0.0f };
		glm::vec2 uv{ 0.0f };
		glm::vec4 color{ 1.0f };

		VertexAttributesUI(glm::vec2 position_ = glm::vec2(0.0f), glm::vec2 uv_ = glm::vec2(0.0f), glm::vec4 color_ = glm::vec4(1.0f)) : position{position_}, uv{uv_}, color{color_} {}
	};

	class GUILayer : public Layer
	{
	public:
		GUILayer() = default;
		~GUILayer();

		void Init(uint32_t width, uint32_t height);
		void OnResize(uint32_t width, uint32_t height);

		void OnEvent(Event& e);
		void Render(WGPURenderPassEncoder renderPass);
		
	private:
		struct UniformsUI
		{
			glm::mat4 projectionMatrix;
			glm::mat4 transformMatrix;
			glm::vec4 color;
			float time;
			float _pad[3];
		};

		static_assert(sizeof(UniformsUI) % 16 == 0);
	private:
		bool InitializePipeline();
		bool InitializeBuffers();
		bool InitializeUniforms();
		bool InitializeBindGroups();

		void RecalculateProjection();
		void RecalculateTransform();
	private:
		uint32_t m_Width;
		uint32_t m_Height;

		std::vector<GUIItem> m_Items;
		
		WGPUDevice m_Device = nullptr;
		WGPUTextureFormat m_SurfaceFormat = WGPUTextureFormat_Undefined;
		WGPUQueue m_Queue = nullptr;

		WGPURenderPipeline m_UIPipeline = nullptr;
		WGPUBindGroupLayout m_BindGroupLayout = nullptr;
		WGPUBindGroup m_UIBindGroup = nullptr;
		WGPUBuffer m_UniformBuffer = nullptr;
		WGPUBuffer m_VertexBuffer = nullptr;
		WGPUBuffer m_IndexBuffer = nullptr;
		WGPUShaderModule m_ShaderModule = nullptr;

		UniformsUI m_Uniforms;

		uint32_t m_IndexCount;
	};
}