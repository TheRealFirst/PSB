#include "GraphicsContext.h"

#include "Core\Application.h"

#include <iostream>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AssetManagement\AssetManager.h"
#include "RendererUtils.h"


constexpr float PI = 3.14159265358979323846f;

PSB::GraphicsContext::GraphicsContext()
{
    m_WindowHandle = nullptr;
}

PSB::GraphicsContext::GraphicsContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
{
    LOG_ASSERT(m_WindowHandle);
}

void PSB::GraphicsContext::Init(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
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
    deviceDesc.defaultQueue.label = "the default queue";{}

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
    m_Config = {};

    m_SurfaceFormat = wgpuSurfaceGetPreferredFormat(m_Surface, m_Adapter);


    m_Config.nextInChain = nullptr;
    m_Config.width = width;
    m_Config.height = height;
    m_Config.format = m_SurfaceFormat;
    m_Config.viewFormatCount = 0;
    m_Config.viewFormats = nullptr;
    m_Config.usage = WGPUTextureUsage_RenderAttachment;
    m_Config.device = m_Device;
    m_Config.presentMode = WGPUPresentMode_Fifo;
    m_Config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(m_Surface, &m_Config);


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

    InitializePipeline();
    InitializeBuffers();
    InitializeBindGroups();

	
	m_DepthStencilAttachment.depthClearValue = 1.0f;
	m_DepthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
	m_DepthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;

	m_DepthStencilAttachment.depthReadOnly = false;

	m_DepthStencilAttachment.stencilClearValue = 0;
	m_DepthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
	m_DepthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
	m_DepthStencilAttachment.stencilReadOnly = true;
    m_RenderPassDesc.depthStencilAttachment = &m_DepthStencilAttachment;
}

void PSB::GraphicsContext::Delete()
{
    if (m_Queue) { wgpuQueueRelease(m_Queue);   m_Queue = nullptr; }
    if (m_Device) { wgpuDeviceRelease(m_Device); m_Device = nullptr; }
    if (m_Adapter) { wgpuAdapterRelease(m_Adapter); m_Adapter = nullptr; }
    if (m_Instance) { wgpuInstanceRelease(m_Instance); m_Instance = nullptr; }
    wgpuSurfaceUnconfigure(m_Surface);
    wgpuSurfaceRelease(m_Surface);
    wgpuRenderPipelineRelease(m_Pipeline);
	wgpuPipelineLayoutRelease(m_Layout);
	wgpuBindGroupLayoutRelease(m_BindGroupLayout);
    wgpuBindGroupRelease(m_BindGroup);
    wgpuTextureViewRelease(m_DepthTextureView);
    wgpuTextureDestroy(m_DepthTexture);
    wgpuTextureRelease(m_DepthTexture);
}

void PSB::GraphicsContext::SwapBuffers()
{
	float angle1 = (float)glfwGetTime();
	float c1 = cos(angle1);
	float s1 = sin(angle1);
	glm::mat4x4 M(1.0f);
	M = glm::rotate(M, angle1, glm::vec3(0.0f, 0.0f, 1.0f));
	M = glm::translate(M, glm::vec3(0.5f, 0.0f, 0.0f));
	M = glm::scale(M, glm::vec3(0.3f));
	m_Uniforms.modelMatrix = M;
    wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, offsetof(MyUniforms, modelMatrix), &m_Uniforms.modelMatrix, sizeof(glm::mat4));

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
    
    m_VertexBuffer.Bind(renderPass, 0);
    m_IndexBuffer.Bind(renderPass);
   

    uint32_t dynamicOffset = 0;

    wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_BindGroup, 1, &dynamicOffset);
    wgpuRenderPassEncoderDrawIndexed(renderPass, m_IndexCount, 1, 0, 0, 0);

    // dynamicOffset = 1 * m_UniformStride;
	// wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_BindGroup, 1, &dynamicOffset);
	// wgpuRenderPassEncoderDrawIndexed(renderPass, m_IndexCount, 1, 0, 0, 0);

    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

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

void PSB::GraphicsContext::InitializePipeline()
{
    WGPUShaderModule shaderModule = AssetManager::LoadShaderModule(RESOURCE_DIR "/shader.wgsl", m_Device);

    if (shaderModule == nullptr) {
        LOG_ERROR("Could not load shader!");
        exit(1);
    }

    // Create the render pipeline
    WGPURenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;

    WGPUVertexBufferLayout vertexBufferLayout{};
    
    std::vector<WGPUVertexAttribute> vertexAttribs(3);

    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[0].offset = offsetof(VertexAttributes, position);

    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = WGPUVertexFormat_Float32x3;
	vertexAttribs[2].offset = offsetof(VertexAttributes, color);


    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();

    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

    // We do not use any vertex buffer for this first simplistic example
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;

    // NB: We define the 'shaderModule' in the second part of this chapter.
    // Here we tell that the programmable vertex shader stage is described
    // by the function called 'vs_main' in that module.
    pipelineDesc.vertex.module = shaderModule;
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
    fragmentState.module = shaderModule;
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

	WGPUTextureFormat depthTextureFormat = WGPUTextureFormat_Depth24Plus;
	depthStencilState.format = depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

    pipelineDesc.depthStencil = &depthStencilState;

    WGPUTextureDescriptor depthTextureDesc{};
	depthTextureDesc.dimension = WGPUTextureDimension_2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = { m_Width, m_Height, 1 };
	depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = &depthTextureFormat;
	m_DepthTexture = wgpuDeviceCreateTexture(m_Device, &depthTextureDesc);

    WGPUTextureViewDescriptor depthTextureViewDesc{};
	depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
	depthTextureViewDesc.format = depthTextureFormat;
	m_DepthTextureView = wgpuTextureCreateView(m_DepthTexture, &depthTextureViewDesc);

    // Samples per pixel
    pipelineDesc.multisample.count = 1;

    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;

    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    WGPUBindGroupLayoutEntry bindingLayout{};
    setDefault(bindingLayout);

    bindingLayout.binding = 0;

    bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
    bindingLayout.buffer.hasDynamicOffset = true;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.nextInChain = nullptr;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    m_BindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_Device, &bindGroupLayoutDesc); // Crash here

    WGPUPipelineLayoutDescriptor layoutDesc{};
    layoutDesc.nextInChain = nullptr;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &m_BindGroupLayout;
    m_Layout = wgpuDeviceCreatePipelineLayout(m_Device, &layoutDesc);


    pipelineDesc.layout = m_Layout;

    m_Pipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);

    // We no longer need to access the shader module
    wgpuShaderModuleRelease(shaderModule);
}


WGPURequiredLimits PSB::GraphicsContext::GetRequiredLimits(WGPUAdapter adapter) const
{
    WGPUSupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supportedLimits);

    WGPURequiredLimits requiredLimits{};
    setDefault(requiredLimits.limits);

    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 3;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    // Maximum size of a buffer is 6 vertices of 2 float each
    requiredLimits.limits.maxBufferSize = supportedLimits.limits.maxBufferSize;
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);

    requiredLimits.limits.maxInterStageShaderComponents = 6;

    requiredLimits.limits.maxBindGroups = 1;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
    requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;

    requiredLimits.limits.maxTextureDimension1D = m_Height;
    requiredLimits.limits.maxTextureDimension2D = m_Width;
    requiredLimits.limits.maxTextureArrayLayers = 1;

    // These two limits are different because they are "minimum" limits,
    // they are the only ones we are may forward from the adapter's supported
    // limits.
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;


    return requiredLimits;
    
}

void PSB::GraphicsContext::InitializeBuffers()
{
    std::vector<float> vertexData;

    std::vector<uint32_t> indexData;

    

    // bool success = AssetManager::LoadGeometry(RESOURCE_DIR "/webgpu.txt", pointData, indexData, 2);
    bool success = AssetManager::LoadGeometry(RESOURCE_DIR "/pyramid.txt", vertexData, indexData, 6);

    if (!success) {
        LOG_ERROR("Could not load geometry!");
        exit(1);
    }
   
    m_VertexBuffer = VertexBuffer(m_Device, m_Queue, vertexData.data(), vertexData.size() * sizeof(float));

    m_IndexBuffer = IndexBuffer(m_Device, m_Queue, indexData.data(), indexData.size() * sizeof(uint32_t));
    m_IndexCount = static_cast<uint32_t>(indexData.size());

    WGPUSupportedLimits supportedLimits{};
    supportedLimits.nextInChain = nullptr;
	wgpuDeviceGetLimits(m_Device, &supportedLimits);
	WGPULimits deviceLimits = supportedLimits.limits;

	m_UniformStride = ceilToNextMultiple((uint32_t)sizeof(MyUniforms),
		(uint32_t)deviceLimits.minUniformBufferOffsetAlignment);

	WGPUBufferDescriptor bufferDesc{};
	bufferDesc.nextInChain = nullptr;
	bufferDesc.size = m_UniformStride + sizeof(MyUniforms);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform; // Vertex usage here!
	bufferDesc.mappedAtCreation = false;
    bufferDesc.label = "Uniform Buffer";
    m_UniformBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

    m_Uniforms.time = 1.0f;
    m_Uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	
    float angle1 = (float)glfwGetTime();
	float c1 = cos(angle1);
	float s1 = sin(angle1);
	glm::mat4x4 M(1.0);
	M = glm::rotate(M, angle1, glm::vec3(0.0f, 0.0f, 1.0f));
	M = glm::translate(M, glm::vec3(0.5f, 0.0f, 0.0f));
	M = glm::scale(M, glm::vec3(0.3f));
	m_Uniforms.modelMatrix = M;

	glm::vec3 focalPoint(0.0f, 0.0f, -2.0f);
	float angle2 = 3.0f * PI / 4.0f;
	float c2 = cosf(angle2);
	float s2 = sinf(angle2);

	glm::mat4 R2 = glm::rotate(glm::mat4x4(1.0f), -angle2, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 T2 = glm::translate(glm::mat4x4(1.0f), -focalPoint);
	m_Uniforms.viewMatrix = T2 * R2;

	float ratio = float(m_Width) / float(m_Height);
	float focalLength = 2.0;
	float nearPlane = 0.01f;
	float farPlane = 100.0f;
	float fov = 2 * glm::atan(1 / focalLength);
	m_Uniforms.projectionMatrix = glm::perspective(fov, ratio, nearPlane, farPlane);

	wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, 0, &m_Uniforms, sizeof(MyUniforms));

	m_Uniforms.time = 1.0f;
	m_Uniforms.color = { 0.0f, 1.0f, 0.4f, 0.7f };
	wgpuQueueWriteBuffer(m_Queue, m_UniformBuffer, m_UniformStride, &m_Uniforms, sizeof(MyUniforms));
}

void PSB::GraphicsContext::InitializeBindGroups()
{
	WGPUBindGroupEntry binding{};
	binding.nextInChain = nullptr;

	binding.binding = 0;
	binding.buffer = m_UniformBuffer;

	binding.offset = 0;
	binding.size = sizeof(MyUniforms);


	WGPUBindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.nextInChain = nullptr;
	bindGroupDesc.layout = m_BindGroupLayout;

	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	m_BindGroup = wgpuDeviceCreateBindGroup(m_Device, &bindGroupDesc);
}