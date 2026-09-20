//
//  RenderDevice.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/1.
//

#include "RenderDevice.h"
#include "ShaderFunction.h"
#include "RenderEncoder.h"
#include "CommandBuffer.h"
#include "RenderPass.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include "metal/MTLRenderDeviceWrapper.h"
#include "vulkan/VKRenderDevice.h"

#if GNX_OS_WINDOWS
#include "dx12/DX12RenderDevice.h"
#endif

#include "Runtime/BaseLib/include/LogService.h"

NAMESPACE_RENDERCORE_BEGIN

// DepthConfig 静态成员定义
// 注意：UseReverseZ 的值由上层 RenderSystem 的 BuildSetting 在初始化时同步
bool DepthConfig::UseReverseZ = true;

RenderDevice::RenderDevice() {}

RenderDevice::~RenderDevice() {}

RCBufferPtr RenderDevice::CreateVertexBuffer(const void* buffer, uint32_t size, StorageMode mode) const
{
    RCBufferDesc desc(size, RCBufferUsage::VertexBuffer, mode);
    return CreateBuffer(desc, buffer);
}

RCBufferPtr RenderDevice::CreateVertexBuffer(uint32_t size, StorageMode mode) const
{
    RCBufferDesc desc(size, RCBufferUsage::VertexBuffer, mode);
    return CreateBuffer(desc);
}

RCBufferPtr RenderDevice::CreateIndexBuffer(const void* buffer, uint32_t size,
                                            StorageMode mode) const
{
    RCBufferDesc desc(size, RCBufferUsage::IndexBuffer, mode);
    return CreateBuffer(desc, buffer);
}

ShaderFunction::ShaderFunction(){}

ShaderFunction::~ShaderFunction(){}

TextureSampler::TextureSampler(const SamplerDesc& des){}

TextureSampler::~TextureSampler(){}

GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineDesc& des) : mDesc(des) {}

GraphicsPipeline::~GraphicsPipeline(){}

UniformBuffer::UniformBuffer(){}

UniformBuffer::~UniformBuffer(){}

RenderEncoder::RenderEncoder()
{
}

RenderEncoder::~RenderEncoder()
{
}

CommandBuffer::CommandBuffer()
{
}

CommandBuffer::~CommandBuffer()
{
}

RenderPass::RenderPass()
{
}

RenderPass::~RenderPass()
{
}

GraphicsShader::GraphicsShader()
{
}

GraphicsShader::~GraphicsShader()
{
}

static RenderDevicePtr renderDevicePtr = nullptr;

RenderDevicePtr CreateRenderDevice(RenderDeviceType deviceType, const NativeWindow& nativeWindow)
{
    if (METAL == deviceType)
    {
#ifdef __APPLE__
        renderDevicePtr = createMetalRenderDevice(nativeWindow);
#endif
    }
    else if (VULKAN == deviceType)
    {
    #if !GNX_OS_IOS
        renderDevicePtr = std::make_shared<VKRenderDevice>(nativeWindow);
    #endif
    }
    else if (DX12 == deviceType)
    {
#if GNX_OS_WINDOWS
        renderDevicePtr = std::make_shared<DX12RenderDevice>(nativeWindow);
#else
        LOG_ERROR("[RenderDevice] DX12 backend is only available on Windows");
#endif
    }
    return renderDevicePtr;
}

RenderDevicePtr GetRenderDevice()
{
    return renderDevicePtr;
}

void DestroyRenderDevice()
{
    if (renderDevicePtr)
    {
        renderDevicePtr->FlushPipelineCache();
    }
    renderDevicePtr = nullptr;
}

NAMESPACE_RENDERCORE_END
