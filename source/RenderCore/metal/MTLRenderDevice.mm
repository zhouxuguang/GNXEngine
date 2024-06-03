//
//  MTLRenderDevice.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLRenderDevice.h"
#include "MTLTexture2D.h"
#include "MTLTextureCube.h"
#include "MTLVertexBuffer.h"
#include "MTLIndexBuffer.h"
#include "MTLTextureSampler.h"
#include "MTLUniformBuffer.h"
#include "MTLGraphicsPipeline.h"
#include "MTLComputePipeline.h"
#include "MTLShaderFunction.h"
#include "MTLCommandBuffer.h"
#include "MTLRenderTexture.h"
#include "MTLComputeBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLRenderDevice::MTLRenderDevice(CAMetalLayer *metalLayer)
{
    mMetalLayer = metalLayer;
    mCommandQueue = [mMetalLayer.device newCommandQueue];
    
    mMTLDeviceExtension = std::make_shared<MTLDeviceExtension>();
}

MTLRenderDevice::~MTLRenderDevice()
{
    //
}

void MTLRenderDevice::resize(uint32_t width, uint32_t height)
{
    [mMetalLayer setDrawableSize:CGSizeMake(width, height)];
    mDepthTexture = createDepthTexture(mMetalLayer.device,
                                mMetalLayer.drawableSize.width, mMetalLayer.drawableSize.height);
    mStencilTexture = createStencilTexture(mMetalLayer.device,
                                mMetalLayer.drawableSize.width, mMetalLayer.drawableSize.height);
    mDepthStencilTexture = createDepthStencilTexture(mMetalLayer.device,
                                mMetalLayer.drawableSize.width, mMetalLayer.drawableSize.height);
}

DeviceExtensionPtr MTLRenderDevice::getDeviceExtension() const
{
    return mMTLDeviceExtension;
}

RenderDeviceType MTLRenderDevice::getRenderDeviceType() const
{
    return RenderDeviceType::METAL;
}

/**
 以指定长度创建buffer
 
 @param size 申请buffer长度，单位（byte）
 @return 成功申请buffer句柄，失败返回0；
 */
VertexBufferPtr MTLRenderDevice::createVertexBufferWithLength(uint32_t size) const
{
    auto vertexBuffer = std::make_shared<MTLVertexBuffer>(mMetalLayer.device, size, StorageModeShared);
    return vertexBuffer;
}

/**
 以指定buffer和长度以内存拷贝方式创建顶点buffer
 
 @param buffer 指定buffer内容
 @param size buffer长度
 @param mode 申请Buffer类型
 @return 成功申请buffer句柄，失败返回0；
 */
VertexBufferPtr MTLRenderDevice::createVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const
{
    auto vertexBuffer = std::make_shared<MTLVertexBuffer>(mMetalLayer.device, mCommandQueue, buffer, size, mode);
    return vertexBuffer;
}

ComputeBufferPtr MTLRenderDevice::createComputeBuffer(uint32_t size) const
{
    auto vertexBuffer = std::make_shared<MTLComputeBuffer>(mMetalLayer.device, size, StorageModeShared);
    return vertexBuffer;
}

ComputeBufferPtr MTLRenderDevice::createComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const
{
    auto vertexBuffer = std::make_shared<MTLComputeBuffer>(mMetalLayer.device, mCommandQueue, buffer, size, mode);
    return vertexBuffer;
}

/**
 以指定buffer和长度以内存拷贝方式创建索引buffer
 
 @param buffer 指定buffer内容
 @param size buffer长度
 @param indexType 索引类型
 @return 成功申请buffer句柄，失败返回0；
 */
IndexBufferPtr MTLRenderDevice::createIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const
{
    auto indexBuffer = std::make_shared<MTLIndexBuffer>(mMetalLayer.device, mCommandQueue, indexType, buffer, size);
    return indexBuffer;
}

/**
 根据纹理描述创建纹理对象

 @param des the description for texture to be created
 @return shared pointer to texture object
 */
Texture2DPtr MTLRenderDevice::createTextureWithDescriptor(const TextureDescriptor& des) const
{
    auto texture2d = std::make_shared<MTLTexture2D>(mMetalLayer.device, mCommandQueue, des);
    return texture2d;
}

/**
 根据纹理描述创建立方体纹理对象

 @param desArray the description for texture to be created
 @return shared pointer to texturecube object
 */
TextureCubePtr MTLRenderDevice::createTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const
{
    return std::make_shared<MTLTextureCube>(mMetalLayer.device, mCommandQueue, desArray);
}

/**
 根据采样描述创建纹理采样器

 @param des the description for sampler to be created.
 @return shared pointer to sampler object.
 */
TextureSamplerPtr MTLRenderDevice::createSamplerWithDescriptor(const SamplerDescriptor& des) const
{
    auto textureSampler = std::make_shared<MTLTextureSampler>(mMetalLayer.device, des);
    return textureSampler;
}

/**
 创建uniform buffer
 */
UniformBufferPtr MTLRenderDevice::createUniformBufferWithSize(uint32_t bufSize) const
{
    auto uniformBuffer = std::make_shared<MTLUniformBuffer>(mMetalLayer.device, bufSize);
    return uniformBuffer;
}

/**
 创建ShaderFunctionPtr
 */
ShaderFunctionPtr MTLRenderDevice::createShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const
{
    MTLShaderFunctionPtr shaderFunction = std::make_shared<MTLShaderFunction>(mMetalLayer.device);
    return shaderFunction->initWithShaderSource(shaderSource, shaderStage);
}

/**
 创建图形管线
 */
GraphicsPipelinePtr MTLRenderDevice::createGraphicsPipeline(const GraphicsPipelineDescriptor& des) const
{
    return std::make_shared<MTLGraphicsPipeline>(mMetalLayer.device, des);
}

ComputePipelinePtr MTLRenderDevice::createComputePipeline(const ShaderCode& shaderSource) const
{
    ShaderFunctionPtr shaderFunction = createShaderFunction(shaderSource, ShaderStage_Compute);
    return std::make_shared<MTLComputePipeline>(mMetalLayer.device, shaderFunction);
}

CommandBufferPtr MTLRenderDevice::createCommandBuffer()
{
    return std::make_shared<MTLCommandBuffer>(mCommandQueue, mMetalLayer, mDepthTexture, mStencilTexture, mDepthStencilTexture);
}

RenderTexturePtr MTLRenderDevice::createRenderTexture(const TextureDescriptor& des) const
{
    return std::make_shared<MTLRenderTexture>(mMetalLayer.device, des);
}

NAMESPACE_RENDERCORE_END
