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
#include "MTLTextureBase.h"

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

void MTLRenderDevice::Resize(uint32_t width, uint32_t height)
{
    [mMetalLayer setDrawableSize:CGSizeMake(width, height)];
    mDepthTexture = createDepthTexture(mMetalLayer.device,
                                mMetalLayer.drawableSize.width, mMetalLayer.drawableSize.height);
    mStencilTexture = createStencilTexture(mMetalLayer.device,
                                mMetalLayer.drawableSize.width, mMetalLayer.drawableSize.height);
    mDepthStencilTexture = createDepthStencilTexture(mMetalLayer.device,
                                mMetalLayer.drawableSize.width, mMetalLayer.drawableSize.height);
}

DeviceExtensionPtr MTLRenderDevice::GetDeviceExtension() const
{
    return mMTLDeviceExtension;
}

RenderDeviceType MTLRenderDevice::GetRenderDeviceType() const
{
    return RenderDeviceType::METAL;
}

/**
 以指定长度创建buffer
 
 @param size 申请buffer长度，单位（byte）
 @return 成功申请buffer句柄，失败返回0；
 */
VertexBufferPtr MTLRenderDevice::CreateVertexBufferWithLength(uint32_t size) const
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
VertexBufferPtr MTLRenderDevice::CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const
{
    auto vertexBuffer = std::make_shared<MTLVertexBuffer>(mMetalLayer.device, mCommandQueue, buffer, size, mode);
    return vertexBuffer;
}

ComputeBufferPtr MTLRenderDevice::CreateComputeBuffer(uint32_t size) const
{
    auto vertexBuffer = std::make_shared<MTLComputeBuffer>(mMetalLayer.device, size, StorageModeShared);
    return vertexBuffer;
}

ComputeBufferPtr MTLRenderDevice::CreateComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const
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
IndexBufferPtr MTLRenderDevice::CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const
{
    auto indexBuffer = std::make_shared<MTLIndexBuffer>(mMetalLayer.device, mCommandQueue, indexType, buffer, size);
    return indexBuffer;
}

/**
 根据纹理描述创建纹理对象

 @param des the description for texture to be created
 @return shared pointer to texture object
 */
Texture2DPtr MTLRenderDevice::CreateTextureWithDescriptor(const TextureDescriptor& des) const
{
    auto texture2d = std::make_shared<MTLTexture2D>(mMetalLayer.device, mCommandQueue, des);
    return texture2d;
}

/**
 根据纹理描述创建立方体纹理对象

 @param desArray the description for texture to be created
 @return shared pointer to texturecube object
 */
TextureCubePtr MTLRenderDevice::CreateTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const
{
    return std::make_shared<MTLTextureCube>(mMetalLayer.device, mCommandQueue, desArray);
}

/**
 根据采样描述创建纹理采样器

 @param des the description for sampler to be created.
 @return shared pointer to sampler object.
 */
TextureSamplerPtr MTLRenderDevice::CreateSamplerWithDescriptor(const SamplerDescriptor& des) const
{
    auto textureSampler = std::make_shared<MTLTextureSampler>(mMetalLayer.device, des);
    return textureSampler;
}

/**
 创建uniform buffer
 */
UniformBufferPtr MTLRenderDevice::CreateUniformBufferWithSize(uint32_t bufSize) const
{
    auto uniformBuffer = std::make_shared<MTLUniformBuffer>(mMetalLayer.device, bufSize);
    return uniformBuffer;
}

/**
 创建ShaderFunctionPtr
 */
ShaderFunctionPtr MTLRenderDevice::CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const
{
    MTLShaderFunctionPtr shaderFunction = std::make_shared<MTLShaderFunction>(mMetalLayer.device);
    return shaderFunction->InitWithShaderSource(shaderSource, shaderStage);
}

GraphicsShaderPtr MTLRenderDevice::CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const
{
    MTLGraphicsShaderPtr shaderPtr = std::make_shared<MTLGraphicsShader>(mMetalLayer.device, vertexShader, fragmentShader);
    return shaderPtr;
}

/**
 创建图形管线
 */
GraphicsPipelinePtr MTLRenderDevice::CreateGraphicsPipeline(const GraphicsPipelineDescriptor& des) const
{
    return std::make_shared<MTLGraphicsPipeline>(mMetalLayer.device, des);
}

ComputePipelinePtr MTLRenderDevice::CreateComputePipeline(const ShaderCode& shaderSource) const
{
    ShaderFunctionPtr shaderFunction = CreateShaderFunction(shaderSource, ShaderStage_Compute);
    return std::make_shared<MTLComputePipeline>(mMetalLayer.device, shaderFunction);
}

CommandBufferPtr MTLRenderDevice::CreateCommandBuffer()
{
    return std::make_shared<MTLCommandBuffer>(mCommandQueue, mMetalLayer, mDepthTexture, mStencilTexture, mDepthStencilTexture);
}

RenderTexturePtr MTLRenderDevice::CreateRenderTexture(const TextureDescriptor& des) const
{
    return std::make_shared<MTLRenderTexture>(mMetalLayer.device, des);
}

RCTexture2DPtr MTLRenderDevice::CreateTexture2D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (format == MTLPixelFormatInvalid)
    {
        assert(false);
        return nullptr;
    }
    
    if (0 == width || 0 == height || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    bool mipmap = (levels > 1);
    
    MTLTextureDescriptor *textureDes = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtlFormat width:width height:height mipmapped:mipmap];
    if (!textureDes)
    {
        return nullptr;
    }
    
    MTLTextureUsage textureUsage = ConvertTextureUsageToMetal(usage);
    textureDes.usage = textureUsage;
    if (HasRenderTargetFlag(textureUsage))
    {
        // 属性设置为shader可读写以及rendertarget
        textureDes.resourceOptions = MTLResourceStorageModePrivate;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }

    return std::make_shared<MTLRCTexture2D>(mMetalLayer.device, mCommandQueue, textureDes);
}

RCTexture3DPtr MTLRenderDevice::CreateTexture3D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t depth,
                                    uint32_t levels) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (format == MTLPixelFormatInvalid)
    {
        assert(false);
        return nullptr;
    }
    
    if (0 == width || 0 == height || 0 == depth || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    bool mipmap = (levels > 1);
    
    MTLTextureDescriptor *textureDes = [MTLTextureDescriptor new];
    if (!textureDes)
    {
        return nullptr;
    }
    
    MTLTextureUsage textureUsage = ConvertTextureUsageToMetal(usage);
    textureDes.usage = textureUsage;
    textureDes.width = width;
    textureDes.height = height;
    textureDes.depth = depth;
    textureDes.mipmapLevelCount = levels;
    textureDes.pixelFormat = mtlFormat;
    textureDes.sampleCount = 1;
    textureDes.textureType = MTLTextureType3D;
    if (HasRenderTargetFlag(textureUsage))
    {
        // 属性设置为shader可读写以及rendertarget
        textureDes.resourceOptions = MTLResourceStorageModePrivate;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }

    return std::make_shared<MTLRCTexture3D>(mMetalLayer.device, mCommandQueue, textureDes);
}

RCTextureCubePtr MTLRenderDevice::CreateTextureCube(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (format == MTLPixelFormatInvalid)
    {
        assert(false);
        return nullptr;
    }
    
    if (0 == width || 0 == height || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    // 立方体纹理宽高需要一致
    if (width != height)
    {
        assert(false);
        return nullptr;
    }
    
    bool mipmap = (levels > 1);
    MTLTextureDescriptor *textureDes = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:mtlFormat size:width mipmapped:mipmap];
    if (!textureDes)
    {
        return nullptr;
    }
    
    MTLTextureUsage textureUsage = ConvertTextureUsageToMetal(usage);
    textureDes.usage = textureUsage;
    if (HasRenderTargetFlag(textureUsage))
    {
        // 属性设置为shader可读写以及rendertarget
        textureDes.resourceOptions = MTLResourceStorageModePrivate;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }
    
    new MTLRCTextureCube(mMetalLayer.device, mCommandQueue, textureDes);

    return std::make_shared<MTLRCTextureCube>(mMetalLayer.device, mCommandQueue, textureDes);
}

RCTexture2DArrayPtr MTLRenderDevice::CreateTexture2DArray(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels,
                                    uint32_t arraySize) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (format == MTLPixelFormatInvalid)
    {
        assert(false);
        return nullptr;
    }
    
    if (0 == width || 0 == height || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    bool mipmap = (levels > 1);
    
    MTLTextureDescriptor *textureDes = [MTLTextureDescriptor new];
    if (!textureDes)
    {
        return nullptr;
    }
    
    MTLTextureUsage textureUsage = ConvertTextureUsageToMetal(usage);
    textureDes.usage = textureUsage;
    textureDes.width = width;
    textureDes.height = height;
    textureDes.depth = 1;
    textureDes.mipmapLevelCount = levels;
    textureDes.pixelFormat = mtlFormat;
    textureDes.sampleCount = 1;
    textureDes.arrayLength = arraySize;
    textureDes.textureType = MTLTextureType2DArray;
    
    if (HasRenderTargetFlag(textureUsage))
    {
        // 属性设置为shader可读写以及rendertarget
        textureDes.resourceOptions = MTLResourceStorageModePrivate;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }
    
    new MTLRCTexture2DArray(mMetalLayer.device, mCommandQueue, textureDes);

    //return std::make_shared<MTLRCTexture2D>(mMetalLayer.device, mCommandQueue, textureDes);
}

NAMESPACE_RENDERCORE_END
