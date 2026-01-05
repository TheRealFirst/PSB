#include "GUI.h"
#include "Core/Application.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/RendererUtils.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"


namespace PSB::GUI
{
	
	struct CircleGenerator
	{
		static float constexpr pi{ 3.1415927f };
		float const radius = 0.0f;
		uint32_t const quality = 0;
		float const da = 0.0f;

		CircleGenerator(float radius_, uint32_t quality_)
			: radius{radius_}, quality{quality_}, da{(2.0f * pi) / static_cast<float>(quality)}
		{}

		glm::vec2 getPoint(uint32_t i) const {
			float const angle{ da * static_cast<float>(i) };
			return { radius * glm::vec2(glm::cos(angle), glm::sin(angle)) };
		}
	};

	void generateCircleVertices(std::vector<VertexAttributesUI>& vertexData, glm::vec2 position, float radius, uint32_t quality)
	{
		CircleGenerator const generator{ radius, quality };
		vertexData.emplace_back(glm::vec2(0.0f));

		for (uint32_t i{ 0 }; i < quality; ++i)
		{
			vertexData.emplace_back(position + generator.getPoint(i));
		}
	}

	void generateCircleIndices(std::vector<uint32_t>& indexData, uint32_t quality)
	{
		for (uint32_t i = 1; i <= quality; ++i) {
			uint32_t i0 = 0;
			uint32_t i1 = i;
			uint32_t i2 = (i == quality) ? 1 : (i + 1);
			indexData.push_back(i0);
			indexData.push_back(i1);
			indexData.push_back(i2);
		}
	}

	struct RoundedRectangleGenerator
	{
		glm::vec2 const size{};
		glm::vec2 const centers[4]{};
		uint32_t const arc_quality;
		CircleGenerator const generator;

		RoundedRectangleGenerator(glm::vec2 size_, float radius, uint32_t quality) : size(size_), centers{ {size.x - radius, size.y - radius}, {radius, size.y - radius}, {radius, radius}, {size.x - radius, radius} }, arc_quality(quality / 4), generator(radius, quality - 4) {}

		glm::vec2 getPoint(uint32_t i) const
		{
			uint32_t const corner_idx{ i / arc_quality };
			return centers[corner_idx] + generator.getPoint(i - corner_idx);
		}
	};

	void generateRoundedRectangleVertices(std::vector<VertexAttributesUI>& vertexData, glm::vec2 position, glm::vec2 size, float radius, uint32_t quality)
	{
		RoundedRectangleGenerator const generator{ size, radius, quality };
		vertexData.emplace_back(position + size * 0.5f);
		for (uint32_t i{ 0 }; i < quality; ++i)
		{
			vertexData.emplace_back(position + generator.getPoint(i));
		}
	}

	void generateRoundedRectangleIndices(std::vector<uint32_t>& indexData, uint32_t quality)
	{
		for (uint32_t i = 1; i <= quality; ++i) {
			uint32_t i0 = 0;
			uint32_t i1 = i;
			uint32_t i2 = (i == quality) ? 1 : (i + 1);
			indexData.push_back(i0);
			indexData.push_back(i1);
			indexData.push_back(i2);
		}
	}


	void GUILayer::OnEvent(Event& e)
	{

	}

	void GUILayer::Init(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;

		m_Device = Application::Get().GetContext()->GetDevice();
		m_SurfaceFormat = Application::Get().GetContext()->GetTextureFormat();
		m_Queue = Application::Get().GetContext()->GetQueue();

		if (!InitializePipeline()) LOG_ASSERT(false);
		if (!InitializeBuffers()) LOG_ASSERT(false);
		if (!InitializeUniforms()) LOG_ASSERT(false);
		if (!InitializeBindGroups()) LOG_ASSERT(false);
	}

	void GUILayer::OnResize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		RecalculateProjection();
		RecalculateTransform();
	}

	void GUILayer::Render(WGPURenderPassEncoder renderPass)
	{
		wgpuRenderPassEncoderSetPipeline(renderPass, m_UIPipeline);
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_VertexBuffer, 0, wgpuBufferGetSize(m_VertexBuffer));
		wgpuRenderPassEncoderSetIndexBuffer(renderPass, m_IndexBuffer, WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(m_IndexBuffer));
		wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_UIBindGroup, 0, nullptr);
		wgpuRenderPassEncoderDrawIndexed(renderPass, m_IndexCount, 1, 0, 0, 0);
	}

	bool GUILayer::InitializePipeline()
	{
		m_ShaderModule = AssetManager::LoadShaderModule(RESOURCE_DIR "/uishader.wgsl", m_Device);

		if (m_ShaderModule == nullptr) {
			LOG_ERROR("Could not load shader!");
			exit(1);
		}

		// Create the render pipeline
		WGPURenderPipelineDescriptor pipelineDesc{};
		pipelineDesc.nextInChain = nullptr;

		std::vector<WGPUVertexAttribute> vertexAttribs(3);

		vertexAttribs[0].shaderLocation = 0;
		vertexAttribs[0].format = WGPUVertexFormat_Float32x2;
		vertexAttribs[0].offset = offsetof(VertexAttributesUI, position);

		vertexAttribs[1].shaderLocation = 1;
		vertexAttribs[1].format = WGPUVertexFormat_Float32x2;
		vertexAttribs[1].offset = offsetof(VertexAttributesUI, uv);

		vertexAttribs[2].shaderLocation = 2;
		vertexAttribs[2].format = WGPUVertexFormat_Float32x4;
		vertexAttribs[2].offset = offsetof(VertexAttributesUI, color);


		WGPUVertexBufferLayout vertexBufferLayout{};

		vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
		vertexBufferLayout.attributes = vertexAttribs.data();

		vertexBufferLayout.arrayStride = sizeof(VertexAttributesUI);
		vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

		pipelineDesc.vertex.bufferCount = 1;
		pipelineDesc.vertex.buffers = &vertexBufferLayout;

		pipelineDesc.vertex.module = m_ShaderModule;
		pipelineDesc.vertex.entryPoint = "vs_main";
		pipelineDesc.vertex.constantCount = 0;
		pipelineDesc.vertex.constants = nullptr;

		pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;

		pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

		pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;

		// But the face orientation does not matter much because we do not
		// cull (i.e. "hide") the faces pointing away from us (which is often
		// used for optimization).
		pipelineDesc.primitive.cullMode = WGPUCullMode_None;

		// We tell that the programmable fragment shader stage is described
		// by the function called 'fs_main' in the shader module.
		WGPUFragmentState fragmentState{};
		fragmentState.module = m_ShaderModule;
		fragmentState.entryPoint = "fs_main";
		fragmentState.constantCount = 0;
		fragmentState.constants = nullptr;

		WGPUBlendState blendState{};
		blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
		blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
		blendState.color.operation = WGPUBlendOperation_Add;
		blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
		blendState.alpha.dstFactor = WGPUBlendFactor_One;
		blendState.alpha.operation = WGPUBlendOperation_Add;

		WGPUColorTargetState colorTarget{};
		colorTarget.format = m_SurfaceFormat;
		colorTarget.blend = &blendState;
		colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

		// We have only one target because our render pass has only one output color
		// attachment.
		fragmentState.targetCount = 1;
		fragmentState.targets = &colorTarget;
		pipelineDesc.fragment = &fragmentState;

		pipelineDesc.depthStencil = nullptr;

		// Samples per pixel
		pipelineDesc.multisample.count = 1;

		// Default value for the mask, meaning "all bits on"
		pipelineDesc.multisample.mask = ~0u;

		// Default value as well (irrelevant for count = 1 anyways)
		pipelineDesc.multisample.alphaToCoverageEnabled = false;


		std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(1);

		WGPUBindGroupLayoutEntry& bindingLayout = bindingLayoutEntries[0];
		setDefault(bindingLayout);

		bindingLayout.binding = 0;
		bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
		bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
		bindingLayout.buffer.minBindingSize = sizeof(UniformsUI);
		bindingLayout.buffer.hasDynamicOffset = false;

		WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
		bindGroupLayoutDesc.nextInChain = nullptr;
		bindGroupLayoutDesc.entryCount = (uint32_t)bindingLayoutEntries.size();
		bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
		m_BindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_Device, &bindGroupLayoutDesc);

		WGPUPipelineLayoutDescriptor layoutDesc{};
		layoutDesc.nextInChain = nullptr;
		layoutDesc.bindGroupLayoutCount = 1;
		layoutDesc.bindGroupLayouts = &m_BindGroupLayout;
		pipelineDesc.layout = wgpuDeviceCreatePipelineLayout(m_Device, &layoutDesc);;

		m_UIPipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);

		return m_UIPipeline != nullptr;
	}

	bool GUILayer::InitializeBuffers()
	{
		uint32_t quality = 40;

		LOG_ASSERT(quality % 4 == 0);

		std::vector<VertexAttributesUI> vertexData;

		generateRoundedRectangleVertices(vertexData, { -0.5f, -0.5f }, glm::vec2(1.0f), 0.2f, quality);

		std::vector<uint32_t> indexData;

		generateRoundedRectangleIndices(indexData, quality);

		m_IndexCount = (uint32_t)indexData.size();


		WGPUBufferDescriptor bufferDesc{};
		bufferDesc.nextInChain = nullptr;
		bufferDesc.size = vertexData.size() * sizeof(VertexAttributesUI);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
		bufferDesc.mappedAtCreation = false;
		m_VertexBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

		wgpuQueueWriteBuffer(m_Queue, m_VertexBuffer, 0, vertexData.data(), bufferDesc.size);

		bufferDesc.size = indexData.size() * sizeof(uint32_t);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
		m_IndexBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

		wgpuQueueWriteBuffer(m_Queue, m_IndexBuffer, 0, indexData.data(), bufferDesc.size);

		return m_VertexBuffer != nullptr;
	}

	bool GUILayer::InitializeUniforms()
	{
		WGPUBufferDescriptor bufferDesc{};

		bufferDesc.nextInChain = nullptr;
		bufferDesc.size = sizeof(UniformsUI);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
		bufferDesc.mappedAtCreation = false;
		bufferDesc.label = "Uniform Buffer";
		m_UniformBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

		m_Uniforms.time = 1.0f;
		m_Uniforms.color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

		glm::vec3 Translation = { float(m_Width)/2.0f, float(m_Height) / 2.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1000.0f, 1000.0f, 1.0f };
		
		glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
		m_Uniforms.transformMatrix = glm::translate(glm::mat4(1.0f), Translation) * rotation *glm::scale(glm::mat4(1.0f), Scale);
		m_Uniforms.projectionMatrix = glm::ortho(0.0f, float(m_Width), 0.0f, float(m_Height), -1.0f, 1.0f);

		wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, 0, &m_Uniforms, sizeof(UniformsUI));
		return m_UniformBuffer != nullptr;
	}

	GUILayer::~GUILayer()
	{
		if (m_UIBindGroup) { wgpuBindGroupRelease(m_UIBindGroup); m_UIBindGroup = nullptr; }

		if (m_UniformBuffer) {
			wgpuBufferDestroy(m_UniformBuffer);
			wgpuBufferRelease(m_UniformBuffer);
			m_UniformBuffer = nullptr;
		}

		if (m_IndexBuffer) {
			wgpuBufferDestroy(m_IndexBuffer);
			wgpuBufferRelease(m_IndexBuffer);
			m_IndexBuffer = nullptr;
		}

		if (m_VertexBuffer) {
			wgpuBufferDestroy(m_VertexBuffer);
			wgpuBufferRelease(m_VertexBuffer);
			m_VertexBuffer = nullptr;
		}

		if (m_UIPipeline) { wgpuRenderPipelineRelease(m_UIPipeline); m_UIPipeline = nullptr; }

		if (m_BindGroupLayout) { wgpuBindGroupLayoutRelease(m_BindGroupLayout); m_BindGroupLayout = nullptr; }

		if (m_ShaderModule) { wgpuShaderModuleRelease(m_ShaderModule); m_ShaderModule = nullptr; }
	}

	void GUILayer::RecalculateProjection()
	{
		m_Uniforms.projectionMatrix = glm::ortho(0.0f, float(m_Width), 0.0f, float(m_Height), -1.0f, 1.0f);
		wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, offsetof(UniformsUI, projectionMatrix), &m_Uniforms.projectionMatrix, sizeof(UniformsUI::projectionMatrix));
	}

	bool GUILayer::InitializeBindGroups()
	{
		std::vector<WGPUBindGroupEntry> bindings(1);

		bindings[0].nextInChain = nullptr;
		bindings[0].binding = 0;
		bindings[0].buffer = m_UniformBuffer;
		bindings[0].offset = 0;
		bindings[0].size = sizeof(UniformsUI);


		WGPUBindGroupDescriptor bindGroupDesc{};
		bindGroupDesc.nextInChain = nullptr;
		bindGroupDesc.layout = m_BindGroupLayout;

		bindGroupDesc.entryCount = (uint32_t)bindings.size();
		bindGroupDesc.entries = bindings.data();
		m_UIBindGroup = wgpuDeviceCreateBindGroup(m_Device, &bindGroupDesc);

		return m_UIBindGroup != nullptr;
	}

	void GUILayer::RecalculateTransform()
	{
		glm::vec3 Translation = { float(m_Width) / 2.0f, float(m_Height) / 2.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1000.0f, 1000.0f, 1.0f };

		glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
		m_Uniforms.transformMatrix = glm::translate(glm::mat4(1.0f), Translation) * rotation * glm::scale(glm::mat4(1.0f), Scale);

		wgpuQueueWriteBuffer(
			m_Queue,
			m_UniformBuffer,
			offsetof(UniformsUI, transformMatrix),
			&m_Uniforms.transformMatrix,
			sizeof(m_Uniforms.transformMatrix)
		);
	}

}

