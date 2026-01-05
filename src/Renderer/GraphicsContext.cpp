#include "GraphicsContext.h"

#include "Core\Application.h"

#include <iostream>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/MouseCodes.h"


#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AssetManagement\AssetManager.h"
#include "RendererUtils.h"


namespace PSB
{
    constexpr float PI = 3.14159265358979323846f;

    PSB::GraphicsContext::GraphicsContext()
    {
        m_WindowHandle = nullptr;
    }

    PSB::GraphicsContext::GraphicsContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
    {
        LOG_ASSERT(m_WindowHandle);
    }

    bool PSB::GraphicsContext::Init(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;

        if (!InitializeWindowAndDevice()) return false;
        if (!InitializeSwapChain()) return false;
        if (!InitializeAttachments()) return false;
        if (!InitializeDepthBuffer()) return false;
        if (!InitializePipeline()) return false;
        if (!InitializeTexture()) return false;
        if (!InitializeGeometry()) return false;
        if (!InitializeUniforms()) return false;
        if (!InitializeBindGroups()) return false;
		return true;
    }

	void GraphicsContext::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(PSB_BIND_EVENT_FN(GraphicsContext::OnMouseScroll));
		dispatcher.Dispatch<MouseButtonPressedEvent>(PSB_BIND_EVENT_FN(GraphicsContext::OnMouseButtonPressed));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(PSB_BIND_EVENT_FN(GraphicsContext::OnMouseButtonReleased));
		dispatcher.Dispatch<MouseMovedEvent>(PSB_BIND_EVENT_FN(GraphicsContext::OnMouseMoved));
	}

	bool GraphicsContext::OnMouseScroll(MouseScrolledEvent& e)
	{
		m_CameraState.zoom += m_DragState.scrollSensitivity * e.GetYOffset();
		m_CameraState.zoom = glm::clamp(m_CameraState.zoom, -2.0f, 2.0f);
		UpdateViewMatrix();
		return true;
	}

	bool GraphicsContext::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::Button0)
		{
			m_DragState.active = true;
			float xpos, ypos;
			m_DragState.startMouse = glm::vec2(Input::GetMouseX(), Input::GetMouseY());
			m_DragState.startCameraState = m_CameraState;
			return true;
		}
		return false;
	}

	bool GraphicsContext::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::Button0 && m_DragState.active)
		{
			m_DragState.active = false;
			return true;
		}
		return false;
	}

	bool GraphicsContext::OnMouseMoved(MouseMovedEvent& e)
	{
		if (m_DragState.active)
		{
			glm::vec2 currentMouse = e.GetPos();
			glm::vec2 delta = (currentMouse - m_DragState.startMouse) * m_DragState.sensitivity;
			m_CameraState.angles = m_DragState.startCameraState.angles + delta;

			m_CameraState.angles.y = glm::clamp(m_CameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
			UpdateViewMatrix();

			m_DragState.velocity = delta - m_DragState.previosDelta;
			m_DragState.previosDelta = delta;

			return true;
		}
		return false;
	}

	bool GraphicsContext::InitializeWindowAndDevice()
    {
		WGPUInstanceDescriptor desc{};
		m_Instance = wgpuCreateInstance(&desc);
		LOG_ASSERT(m_Instance);
		LOG_DEBUG("WGPU Instance created");

		LOG_DEBUG("Requesting WGPUAdapter...");

		m_Surface = glfwGetWGPUSurface(m_Instance, m_WindowHandle);

		WGPURequestAdapterOptions adapterOpts = {};
		adapterOpts.nextInChain = nullptr;
		adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
		adapterOpts.compatibleSurface = m_Surface;

		m_Adapter = RequestAdapterSync(m_Instance, &adapterOpts);
		LOG_ASSERT(m_Adapter);
		LOG_DEBUG("Got adapter.");

		LOG_DEBUG("Requesting WGPUDevice...");
		WGPUDeviceDescriptor deviceDesc{};

		deviceDesc.nextInChain = nullptr;
		deviceDesc.label = "My Device";
		deviceDesc.requiredFeatureCount = 0;
		WGPURequiredLimits requiredLimits = GetRequiredLimits(m_Adapter);
		deviceDesc.requiredLimits = &requiredLimits;
		deviceDesc.defaultQueue.nextInChain = nullptr;
		deviceDesc.defaultQueue.label = "the default queue";

		deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason,
			char const* message,
			void* /* pUserData */) {
				std::cout << "Device lost: reason " << reason;
				if (message) std::cout << " (" << message << ")";
				std::cout << std::endl;
			};

		m_Device = RequestDeviceSync(m_Adapter, &deviceDesc);
		LOG_ASSERT(m_Device);
		LOG_DEBUG("Got device.");

		auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
			std::cout << "Uncaptured device error: type " << type;
			if (message) std::cout << " (" << message << ")";
			std::cout << std::endl;
			};
		wgpuDeviceSetUncapturedErrorCallback(m_Device, onDeviceError, nullptr /* pUserData */);

		m_Queue = wgpuDeviceGetQueue(m_Device);


		LOG_ASSERT(m_Surface);
		
        #ifdef WEBGPU_BACKEND_WGPU
		m_SurfaceFormat = wgpuSurfaceGetPreferredFormat(m_Surface, m_Adapter);
        #else
        m_SurfaceFormat = WGPUTextureFormat_BGRA8Unorm;
        #endif

        if (m_Adapter) { wgpuAdapterRelease(m_Adapter); m_Adapter = nullptr; }
        return m_Device != nullptr;
    }

    void GraphicsContext::TerminateWindowAndDevice()
    {
		if (m_Queue) { wgpuQueueRelease(m_Queue);   m_Queue = nullptr; }
		if (m_Device) { wgpuDeviceRelease(m_Device); m_Device = nullptr; }
		if (m_Instance) { wgpuInstanceRelease(m_Instance); m_Instance = nullptr; }
    }

    bool GraphicsContext::InitializeSwapChain()
    {
		m_Config = {};

		m_Config.nextInChain = nullptr;
		m_Config.width = m_Width;
		m_Config.height = m_Height;
		m_Config.format = m_SurfaceFormat;
		m_Config.viewFormatCount = 0;
		m_Config.viewFormats = nullptr;
		m_Config.usage = WGPUTextureUsage_RenderAttachment;
		m_Config.device = m_Device;
		m_Config.presentMode = WGPUPresentMode_Fifo;
		m_Config.alphaMode = WGPUCompositeAlphaMode_Auto;

		wgpuSurfaceConfigure(m_Surface, &m_Config);
        return m_Surface != nullptr;
    }

    void GraphicsContext::TerminateSwapChain()
    {
		wgpuSurfaceUnconfigure(m_Surface);
		wgpuSurfaceRelease(m_Surface);
    }

    bool GraphicsContext::InitializeAttachments()
    {
		m_ColorAttachment = {};
		m_ColorAttachment.view = nullptr; // set per frame
		m_ColorAttachment.resolveTarget = nullptr;
		m_ColorAttachment.loadOp = WGPULoadOp_Clear;
		m_ColorAttachment.storeOp = WGPUStoreOp_Store;
		m_ColorAttachment.clearValue = WGPUColor{ m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a };

		m_RenderPassDesc = {};
		m_RenderPassDesc.colorAttachmentCount = 1;
		m_RenderPassDesc.colorAttachments = &m_ColorAttachment;
		m_RenderPassDesc.timestampWrites = nullptr;

		m_DepthStencilAttachment.depthClearValue = 1.0f;
		m_DepthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
		m_DepthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;

		m_DepthStencilAttachment.depthReadOnly = false;

		m_DepthStencilAttachment.stencilClearValue = 0;
		m_DepthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
		m_DepthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
		m_DepthStencilAttachment.stencilReadOnly = true;
		m_RenderPassDesc.depthStencilAttachment = &m_DepthStencilAttachment;


		m_UIPassColorAttachment = {};
		m_UIPassColorAttachment.view = nullptr; // set per frame
		m_UIPassColorAttachment.resolveTarget = nullptr;
		m_UIPassColorAttachment.loadOp = WGPULoadOp_Load;
		m_UIPassColorAttachment.storeOp = WGPUStoreOp_Store;
		
		m_UIRenderPassDesc = {};
		m_UIRenderPassDesc.colorAttachmentCount = 1;
		m_UIRenderPassDesc.colorAttachments = &m_UIPassColorAttachment;
		m_UIRenderPassDesc.depthStencilAttachment = nullptr;

        return true;
    }

    bool GraphicsContext::InitializeDepthBuffer()
    {
		WGPUTextureDescriptor depthTextureDesc{};
		depthTextureDesc.dimension = WGPUTextureDimension_2D;
		depthTextureDesc.format = m_DepthTextureFormat;
		depthTextureDesc.mipLevelCount = 1;
		depthTextureDesc.sampleCount = 1;
		depthTextureDesc.size = { m_Width, m_Height, 1 };
		depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
		depthTextureDesc.viewFormatCount = 1;
		depthTextureDesc.viewFormats = &m_DepthTextureFormat;
		m_DepthTexture = wgpuDeviceCreateTexture(m_Device, &depthTextureDesc);

		WGPUTextureViewDescriptor depthTextureViewDesc{};
		depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
		depthTextureViewDesc.baseArrayLayer = 0;
		depthTextureViewDesc.arrayLayerCount = 1;
		depthTextureViewDesc.baseMipLevel = 0;
		depthTextureViewDesc.mipLevelCount = 1;
		depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
		depthTextureViewDesc.format = m_DepthTextureFormat;
		m_DepthTextureView = wgpuTextureCreateView(m_DepthTexture, &depthTextureViewDesc);

        return m_DepthTextureView != nullptr;
    }

    void GraphicsContext::TerminateDepthBuffer()
    {
		wgpuTextureViewRelease(m_DepthTextureView);
		wgpuTextureDestroy(m_DepthTexture);
		wgpuTextureRelease(m_DepthTexture);
    }

	bool PSB::GraphicsContext::InitializePipeline()
	{
		m_ShaderModule = AssetManager::LoadShaderModule(RESOURCE_DIR "/shader.wgsl", m_Device);

		if (m_ShaderModule == nullptr) {
			LOG_ERROR("Could not load shader!");
			exit(1);
		}

		// Create the render pipeline
		WGPURenderPipelineDescriptor pipelineDesc{};
		pipelineDesc.nextInChain = nullptr;

		std::vector<WGPUVertexAttribute> vertexAttribs(4);

		vertexAttribs[0].shaderLocation = 0;
		vertexAttribs[0].format = WGPUVertexFormat_Float32x3;
		vertexAttribs[0].offset = offsetof(VertexAttributes3D, position);

		vertexAttribs[1].shaderLocation = 1;
		vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
		vertexAttribs[1].offset = offsetof(VertexAttributes3D, normal);

		vertexAttribs[2].shaderLocation = 2;
		vertexAttribs[2].format = WGPUVertexFormat_Float32x3;
		vertexAttribs[2].offset = offsetof(VertexAttributes3D, color);

		vertexAttribs[3].shaderLocation = 3;
		vertexAttribs[3].format = WGPUVertexFormat_Float32x2;
		vertexAttribs[3].offset = offsetof(VertexAttributes3D, uv);


        WGPUVertexBufferLayout vertexBufferLayout{};

		vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
		vertexBufferLayout.attributes = vertexAttribs.data();

		vertexBufferLayout.arrayStride = sizeof(VertexAttributes3D);
		vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

		// We do not use any vertex buffer for this first simplistic example
		pipelineDesc.vertex.bufferCount = 1;
		pipelineDesc.vertex.buffers = &vertexBufferLayout;

		// NB: We define the 'shaderModule' in the second part of this chapter.
		// Here we tell that the programmable vertex shader stage is described
		// by the function called 'vs_main' in that module.
		pipelineDesc.vertex.module = m_ShaderModule;
		pipelineDesc.vertex.entryPoint = "vs_main";
		pipelineDesc.vertex.constantCount = 0;
		pipelineDesc.vertex.constants = nullptr;

		// Each sequence of 3 vertices is considered as a triangle
		pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;

		// We'll see later how to specify the order in which vertices should be
		// connected. When not specified, vertices are considered sequentially.
		pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

		// The face orientation is defined by assuming that when looking
		// from the front of the face, its corner vertices are enumerated
		// in the counter-clockwise (CCW) order.
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


		WGPUDepthStencilState depthStencilState;
		setDefault(depthStencilState);

		depthStencilState.depthCompare = WGPUCompareFunction_Less;
		depthStencilState.depthWriteEnabled = true;

		depthStencilState.format = m_DepthTextureFormat;
		depthStencilState.stencilReadMask = 0;
		depthStencilState.stencilWriteMask = 0;

		pipelineDesc.depthStencil = &depthStencilState;

		// Samples per pixel
		pipelineDesc.multisample.count = 1;

		// Default value for the mask, meaning "all bits on"
		pipelineDesc.multisample.mask = ~0u;

		// Default value as well (irrelevant for count = 1 anyways)
		pipelineDesc.multisample.alphaToCoverageEnabled = false;


		std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

		WGPUBindGroupLayoutEntry& bindingLayout = bindingLayoutEntries[0];
		setDefault(bindingLayout);

		bindingLayout.binding = 0;

		bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
		bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
		bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
		bindingLayout.buffer.hasDynamicOffset = true;

		WGPUBindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
		setDefault(textureBindingLayout);
		textureBindingLayout.binding = 1;
		textureBindingLayout.visibility = WGPUShaderStage_Fragment;
		textureBindingLayout.texture.sampleType = WGPUTextureSampleType_Float;
		textureBindingLayout.texture.viewDimension = WGPUTextureViewDimension_2D;

		WGPUBindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
		setDefault(samplerBindingLayout);
		samplerBindingLayout.binding = 2;
		samplerBindingLayout.visibility = WGPUShaderStage_Fragment;
		samplerBindingLayout.sampler.type = WGPUSamplerBindingType_Filtering;

		WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
		bindGroupLayoutDesc.nextInChain = nullptr;
		bindGroupLayoutDesc.entryCount = (uint32_t)bindingLayoutEntries.size();
		bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
		m_BindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_Device, &bindGroupLayoutDesc);

		WGPUPipelineLayoutDescriptor layoutDesc{};
		layoutDesc.nextInChain = nullptr;
		layoutDesc.bindGroupLayoutCount = 1;
		layoutDesc.bindGroupLayouts = &m_BindGroupLayout;
		m_Layout = wgpuDeviceCreatePipelineLayout(m_Device, &layoutDesc);


		pipelineDesc.layout = m_Layout;

		m_Pipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);
		
        return m_Pipeline != nullptr;
	}

    void GraphicsContext::TerminatePipeline()
    {
        wgpuRenderPipelineRelease(m_Pipeline);
        wgpuShaderModuleRelease(m_ShaderModule);
		wgpuPipelineLayoutRelease(m_Layout);
		wgpuBindGroupLayoutRelease(m_BindGroupLayout);
    }

    bool GraphicsContext::InitializeTexture()
    {
		WGPUSamplerDescriptor samplerDesc{};
		samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
		samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
		samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
		samplerDesc.magFilter = WGPUFilterMode_Linear;
		samplerDesc.minFilter = WGPUFilterMode_Linear;
		samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
		samplerDesc.lodMinClamp = 0.0f;
		samplerDesc.lodMaxClamp = 8.0f;
		samplerDesc.compare = WGPUCompareFunction_Undefined;
		samplerDesc.maxAnisotropy = 1;
		m_Sampler = wgpuDeviceCreateSampler(m_Device, &samplerDesc);

		m_Texture = AssetManager::LoadTexture(RESOURCE_DIR "/fourareen/fourareen2K_albedo.jpg", m_Device, &m_TextureView);
		if (!m_Texture)
		{
			LOG_ERROR("Could not load texture!");
		}

        return m_TextureView != nullptr;
    }

	void GraphicsContext::TerminateTextures()
	{
		wgpuTextureViewRelease(m_TextureView);
		wgpuTextureDestroy(m_Texture);
		wgpuTextureRelease(m_Texture);
		wgpuSamplerRelease(m_Sampler);
	}

    bool GraphicsContext::InitializeGeometry() 
    {
		std::vector<VertexAttributes3D> vertexData;

		bool success = AssetManager::LoadGeometryFromObj(RESOURCE_DIR "/fourareen/fourareen.obj", vertexData);

		if (!success) {
			LOG_ERROR("Could not load geometry!");
			exit(1);
		}


		WGPUBufferDescriptor bufferDesc{};
		bufferDesc.nextInChain = nullptr;
		bufferDesc.size = vertexData.size() * sizeof(VertexAttributes3D);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
		bufferDesc.mappedAtCreation = false;
		m_VertexBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

		wgpuQueueWriteBuffer(m_Queue, m_VertexBuffer, 0, vertexData.data(), bufferDesc.size);

		m_VertexCount = static_cast<uint32_t>(vertexData.size());

        return m_VertexBuffer != nullptr;
    }

    void GraphicsContext::TerminateGeometry()
    {
        wgpuBufferDestroy(m_VertexBuffer);
        wgpuBufferRelease(m_VertexBuffer);
        m_VertexCount = 0;
    }

    bool GraphicsContext::InitializeUniforms()
    {
		WGPUBufferDescriptor bufferDesc{};

		bufferDesc.nextInChain = nullptr;
		bufferDesc.size = sizeof(MyUniforms);
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform; // Vertex usage here!
		bufferDesc.mappedAtCreation = false;
		bufferDesc.label = "Uniform Buffer";
		m_UniformBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

		m_Uniforms.time = 1.0f;
		m_Uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };


		m_Uniforms.modelMatrix = glm::mat4x4(1.0f);
		m_Uniforms.viewMatrix = glm::lookAt(glm::vec3(-2.0f, -3.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		m_Uniforms.projectionMatrix = glm::perspective(45 * PI / 180, m_Width / float(m_Height), 0.01f, 100.0f);

		wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, 0, &m_Uniforms, sizeof(MyUniforms));

		UpdateViewMatrix();
		return m_UniformBuffer != nullptr;
    }

	void GraphicsContext::TerminateUniforms()
	{
		wgpuBufferDestroy(m_UniformBuffer);
		wgpuBufferRelease(m_UniformBuffer);
	}

	bool GraphicsContext::InitializeBindGroups()
	{
		std::vector<WGPUBindGroupEntry> bindings(3);

		bindings[0].nextInChain = nullptr;
		bindings[0].binding = 0;
		bindings[0].buffer = m_UniformBuffer;
		bindings[0].offset = 0;
		bindings[0].size = sizeof(MyUniforms);

		bindings[1].nextInChain = nullptr;
		bindings[1].binding = 1;
		bindings[1].textureView = m_TextureView;

		bindings[2].nextInChain = nullptr;
		bindings[2].binding = 2;
		bindings[2].sampler = m_Sampler;


		WGPUBindGroupDescriptor bindGroupDesc{};
		bindGroupDesc.nextInChain = nullptr;
		bindGroupDesc.layout = m_BindGroupLayout;

		bindGroupDesc.entryCount = (uint32_t)bindings.size();
		bindGroupDesc.entries = bindings.data();
		m_BindGroup = wgpuDeviceCreateBindGroup(m_Device, &bindGroupDesc);

		return m_BindGroup != nullptr;
	}

	void GraphicsContext::TerminateBindGroups()
	{
		wgpuBindGroupRelease(m_BindGroup);
	}

	void GraphicsContext::UpdateProjectionMatrix()
	{
		float ratio = m_Width / float(m_Height);

		m_Uniforms.projectionMatrix = glm::perspective(45 * PI / 180, ratio, 0.01f, 100.0f);

		wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, offsetof(MyUniforms, projectionMatrix), &m_Uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));
	}

	void GraphicsContext::UpdateViewMatrix()
	{
		float cx = cos(m_CameraState.angles.x);
		float sx = sin(m_CameraState.angles.x);
		float cy = cos(m_CameraState.angles.y);
		float sy = sin(m_CameraState.angles.y);
		glm::vec3 position = glm::vec3(cx * cy, sx * cy, sy) * std::exp(-m_CameraState.zoom);
		m_Uniforms.viewMatrix = glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0, 0, 1));
		wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, offsetof(MyUniforms, viewMatrix), &m_Uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));
	}

	void GraphicsContext::UpdateDragInertia()
	{
		constexpr float eps = 1e-4f;

		if (!m_DragState.active)
		{
			if (std::abs(m_DragState.velocity.x) < eps && std::abs(m_DragState.velocity.y) < eps)
			{
				return;
			}
			m_CameraState.angles += m_DragState.velocity;
			m_CameraState.angles.y = glm::clamp(m_CameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);

			m_DragState.velocity *= m_DragState.inertia;
			UpdateViewMatrix();
		}
	}

	void PSB::GraphicsContext::Delete()
    {
		TerminateBindGroups();
		TerminateUniforms();
		TerminateGeometry();
		TerminateTextures();
		TerminatePipeline();
		TerminateDepthBuffer();
		TerminateSwapChain();
		TerminateWindowAndDevice();
    }

    void PSB::GraphicsContext::OnFrame()
    {
		UpdateDragInertia();
        // wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, offsetof(MyUniforms, modelMatrix), &m_Uniforms.modelMatrix, sizeof(glm::mat4));

        auto [surfaceTexture, targetView] = GetNextSurfaceViewData();
        if (!targetView) return;

        // Create a command encoder for the draw call
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "My command encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_Device, &encoderDesc);


        m_ColorAttachment.view = targetView;
        m_DepthStencilAttachment.view = m_DepthTextureView;


        // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &m_RenderPassDesc);

        wgpuRenderPassEncoderSetPipeline(renderPass, m_Pipeline);

        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_VertexBuffer, 0, wgpuBufferGetSize(m_VertexBuffer));


        uint32_t dynamicOffset = 0;

        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_BindGroup, 1, &dynamicOffset);
        // wgpuRenderPassEncoderDrawIndexed(renderPass, m_IndexCount, 1, 0, 0, 0);
        wgpuRenderPassEncoderDraw(renderPass, m_VertexCount, 1, 0, 0);

        // dynamicOffset = 1 * m_UniformStride;
        // wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_BindGroup, 1, &dynamicOffset);
        // wgpuRenderPassEncoderDrawIndexed(renderPass, m_IndexCount, 1, 0, 0, 0);

        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

		m_UIPassColorAttachment.view = targetView;


		WGPURenderPassEncoder uiPass = wgpuCommandEncoderBeginRenderPass(encoder, &m_UIRenderPassDesc);

		Application::Get().GetGUILayer()->Render(uiPass);

		wgpuRenderPassEncoderEnd(uiPass);
		wgpuRenderPassEncoderRelease(uiPass);

        // Finally encode and submit the render pass
        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(m_Queue, 1, &command);
        wgpuCommandBufferRelease(command);

        // At the end of the frame
        wgpuTextureViewRelease(targetView);
        #ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(m_Surface);
        #endif

        #if defined(WEBGPU_BACKEND_DAWN)
        wgpuDeviceTick(m_Device);
        #elif defined(WEBGPU_BACKEND_WGPU)
        wgpuDevicePoll(m_Device, false, nullptr);
        #endif

        wgpuTextureRelease(surfaceTexture.texture);
    }

    void PSB::GraphicsContext::SetClearColor(glm::vec4 clearColor)
    {
        m_ClearColor = clearColor;
        m_ColorAttachment.clearValue = WGPUColor{ m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a };
    }

    void PSB::GraphicsContext::OnWindowResize(uint32_t width, uint32_t height)
    {
		TerminateDepthBuffer();
		TerminateSwapChain();

        m_Width = width;
        m_Height = height;

		m_Surface = glfwGetWGPUSurface(m_Instance, m_WindowHandle);
		InitializeSwapChain();
		InitializeDepthBuffer();
		UpdateProjectionMatrix();
    }

    // --- Helpers updated to AllowSpontaneous + event pumping ---

    WGPUAdapter PSB::GraphicsContext::RequestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options)
    {
        struct UserData {
            WGPUAdapter adapter = nullptr;
            bool requestEnded = false;
        };
        UserData userData;

        auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData) {
            UserData& userData = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestAdapterStatus_Success) {
                userData.adapter = adapter;
            }
            else
            {
                LOG_ERROR("Could not get WebGPU adapter: ");
                LOG_ERROR(message);
            }
            userData.requestEnded = true;
            };

        wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, (void*)&userData);

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
                LOG_ERROR("Could not get WebGPU Device");
                LOG_ERROR(message);
            }
            userData.requestEnded = true;
            };

        wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void*)&userData);

        LOG_ASSERT(userData.requestEnded);


        return userData.device;
    }

    std::pair<WGPUSurfaceTexture, WGPUTextureView> PSB::GraphicsContext::GetNextSurfaceViewData()
    {
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(m_Surface, &surfaceTexture);


        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
            return { surfaceTexture, nullptr };
        }

        WGPUTextureViewDescriptor viewDescriptor;
        viewDescriptor.nextInChain = nullptr;
        viewDescriptor.label = "Surface texture view";
        viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
        viewDescriptor.dimension = WGPUTextureViewDimension_2D;
        viewDescriptor.baseMipLevel = 0;
        viewDescriptor.mipLevelCount = 1;
        viewDescriptor.baseArrayLayer = 0;
        viewDescriptor.arrayLayerCount = 1;
        viewDescriptor.aspect = WGPUTextureAspect_All;
        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

        return { surfaceTexture, targetView };
    }

    


    WGPURequiredLimits PSB::GraphicsContext::GetRequiredLimits(WGPUAdapter adapter) const
    {
        WGPUSupportedLimits supportedLimits;
        supportedLimits.nextInChain = nullptr;
        wgpuAdapterGetLimits(adapter, &supportedLimits);

        WGPURequiredLimits requiredLimits{};
        setDefault(requiredLimits.limits);

        // We use at most 1 vertex attribute for now
        requiredLimits.limits.maxVertexAttributes = 4;
        // We should also tell that we use 1 vertex buffers
        requiredLimits.limits.maxVertexBuffers = 1;
        // Maximum size of a buffer is 6 vertices of 2 float each
        requiredLimits.limits.maxBufferSize = supportedLimits.limits.maxBufferSize;
        // Maximum stride between 2 consecutive vertices in the vertex buffer
        requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes3D);

        requiredLimits.limits.maxInterStageShaderComponents = 8;

        requiredLimits.limits.maxBindGroups = 1;
        requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
        requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
        requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		int width;
		int height;

		glfwGetMonitorWorkarea(monitor, nullptr, nullptr, &width, &height);

        requiredLimits.limits.maxTextureDimension1D = (height > 2048 ? height : 2048);
        requiredLimits.limits.maxTextureDimension2D = (width > 2048 ? width : 2048);
        requiredLimits.limits.maxTextureArrayLayers = 1;

        requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
        requiredLimits.limits.maxSamplersPerShaderStage = 1;

        // These two limits are different because they are "minimum" limits,
        // they are the only ones we are may forward from the adapter's supported
        // limits.
        requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
        requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;


        return requiredLimits;

    }
}