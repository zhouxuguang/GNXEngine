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

NAMESPACE_RENDERCORE_BEGIN

// DepthConfig 静态成员定义
bool DepthConfig::UseReverseZ = true;

RenderDevice::RenderDevice() {}

RenderDevice::~RenderDevice() {}

DeviceExtension::DeviceExtension(){}

DeviceExtension::~DeviceExtension(){}

VertexBuffer::VertexBuffer(){}

VertexBuffer::~VertexBuffer(){}

ShaderFunction::ShaderFunction(){}

ShaderFunction::~ShaderFunction(){}

TextureSampler::TextureSampler(const SamplerDesc& des){}

TextureSampler::~TextureSampler(){}

IndexBuffer::IndexBuffer(IndexType indexType, const void* pData, uint32_t dataLen){}

IndexBuffer::~IndexBuffer(){}

GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineDesc& des){}

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

RenderDevicePtr CreateRenderDevice(RenderDeviceType deviceType, ViewHandle viewHandle)
{
    if (METAL == deviceType)
    {
#ifdef __APPLE__
        renderDevicePtr = createMetalRenderDevice(viewHandle);
#endif
    }
    else if (VULKAN == deviceType)
    {
        renderDevicePtr = std::make_shared<VKRenderDevice>(viewHandle);
    }
    return renderDevicePtr;
}

RenderDevicePtr GetRenderDevice()
{
    return renderDevicePtr;
}

NAMESPACE_RENDERCORE_END
