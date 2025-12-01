#include "GraphicsContext.h"

#include "Core\Application.h"

#include <iostream>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

const char* shaderSource = R"(
@vertex
fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
	return vec4f(in_vertex_position, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

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
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "the default queue";

    deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
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
    m_RenderPassDesc.depthStencilAttachment = nullptr;
    m_RenderPassDesc.timestampWrites = nullptr;

    InitializePipeline();
    InitializeBuffers();
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
}

void PSB::GraphicsContext::SwapBuffers()
{
    auto [surfaceTexture, targetView] = GetNextSurfaceViewData();
    if (!targetView) return;

    // Create a command encoder for the draw call
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_Device, &encoderDesc);

    // Create the render pass that clears the screen with our color
    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;

    
    m_ColorAttachment.view = targetView;
    
    // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &m_RenderPassDesc);

    wgpuRenderPassEncoderSetPipeline(renderPass, m_Pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_VertexBuffer, 0, wgpuBufferGetSize(m_VertexBuffer));
    wgpuRenderPassEncoderDraw(renderPass, m_VertexCount, 1, 0, 0);

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
    // Load the shader module
    WGPUShaderModuleDescriptor shaderDesc{};
    #ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
    #endif

    // We use the extension mechanism to specify the WGSL part of the shader module descriptor
    WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    shaderCodeDesc.code = shaderSource;
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(m_Device, &shaderDesc);

    // Create the render pipeline
    WGPURenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;

    WGPUVertexBufferLayout vertexBufferLayout{};
    WGPUVertexAttribute positionAttrib;

    positionAttrib.shaderLocation = 0;
    positionAttrib.format = WGPUVertexFormat_Float32x2;
    positionAttrib.offset = 0;


    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.attributes = &positionAttrib;

    vertexBufferLayout.arrayStride = 2 * sizeof(float);
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

    // We do not use stencil/depth testing for now
    pipelineDesc.depthStencil = nullptr;

    // Samples per pixel
    pipelineDesc.multisample.count = 1;

    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;

    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    pipelineDesc.layout = nullptr;

    m_Pipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);

    // We no longer need to access the shader module
    wgpuShaderModuleRelease(shaderModule);
}

void setDefault(WGPULimits& limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

WGPURequiredLimits PSB::GraphicsContext::GetRequiredLimits(WGPUAdapter adapter) const
{
    WGPUSupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supportedLimits);

    WGPURequiredLimits requiredLimits{};
    setDefault(requiredLimits.limits);

    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 1;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    // Maximum size of a buffer is 6 vertices of 2 float each
    requiredLimits.limits.maxBufferSize = 6 * 2 * sizeof(float);
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    requiredLimits.limits.maxVertexBufferArrayStride = 2 * sizeof(float);

    // These two limits are different because they are "minimum" limits,
    // they are the only ones we are may forward from the adapter's supported
    // limits.
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;

    return requiredLimits;
    
}

void PSB::GraphicsContext::InitializeBuffers()
{
    std::vector<float> vertexData = {
        // Define a first triangle:
        -0.5, -0.5,
        +0.5, -0.5,
        +0.0, +0.5,

        // Add a second triangle:
        -0.55f, -0.5,
        -0.05f, +0.5,
        -0.55f, +0.5
    };
    m_VertexCount = static_cast<uint32_t>(vertexData.size() / 2);

    WGPUBufferDescriptor bufferDesc{};
    bufferDesc.nextInChain = nullptr;
    bufferDesc.size = vertexData.size() * sizeof(float);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDesc.mappedAtCreation = false;
    m_VertexBuffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

    wgpuQueueWriteBuffer(m_Queue, m_VertexBuffer, 0, vertexData.data(), bufferDesc.size);
}