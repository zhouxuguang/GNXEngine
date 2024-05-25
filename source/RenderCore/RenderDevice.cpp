//
//  RenderDevice.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/1.
//

#include "RenderDevice.h"
#include "ShaderFunction.h"
#include "TextureCube.h"
#include "RenderEncoder.h"
#include "CommandBuffer.h"
#include "FrameBuffer.h"
#include "RenderPass.h"
#include "RenderTexture.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_IPHONE || __ANDROID__
#include "gles/GLRenderDevice.h"
#endif
#include "metal/MTLRenderDeviceWrapper.h"
#include "vulkan/VKRenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

RenderDevice::RenderDevice() {}

RenderDevice::~RenderDevice() {}

DeviceExtension::DeviceExtension(){}

DeviceExtension::~DeviceExtension(){}

FrameBuffer::FrameBuffer(){}

FrameBuffer::~FrameBuffer(){}

VertexBuffer::VertexBuffer(){}

VertexBuffer::~VertexBuffer(){}

ShaderFunction::ShaderFunction(){}

ShaderFunction::~ShaderFunction(){}

TextureSampler::TextureSampler(const SamplerDescriptor& des){}

TextureSampler::~TextureSampler(){}

IndexBuffer::IndexBuffer(IndexType indexType, const void* pData, uint32_t dataLen){}

IndexBuffer::~IndexBuffer(){}

Texture2D::Texture2D(const TextureDescriptor& des){}

Texture2D::~Texture2D(){}

TextureCube::TextureCube(const std::vector<TextureDescriptor>& des){}

TextureCube::~TextureCube(){}

RenderTexture::RenderTexture(const TextureDescriptor& des){}

RenderTexture::~RenderTexture(){}

GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineDescriptor& des){}

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

static RenderDevicePtr renderDevicePtr = nullptr;

RenderDevicePtr createRenderDevice(RenderDeviceType deviceType, ViewHandle viewHandle)
{
    
    if (GLES == deviceType)
    {
        #if TARGET_OS_IPHONE || __ANDROID__
        renderDevicePtr = std::make_shared<GLRenderDevice>(viewHandle);
        #endif
    }
    else if (METAL == deviceType)
    {
        renderDevicePtr = std::make_shared<VKRenderDevice>(viewHandle);
        //renderDevicePtr = createMetalRenderDevice(viewHandle);
    }
    else if (VULKAN == deviceType)
    {
        // VkResult result = volkInitialize();
        // printf("");
    }
    return renderDevicePtr;
}

RenderDevicePtr getRenderDevice()
{
    return renderDevicePtr;
}

NAMESPACE_RENDERCORE_END
