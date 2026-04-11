//
//  MTLRenderDevice.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLRenderDevice.h"
#include "MTLVertexBuffer.h"
#include "MTLIndexBuffer.h"
#include "MTLTextureSampler.h"
#include "MTLUniformBuffer.h"
#include "MTLGraphicsPipeline.h"
#include "MTLComputePipeline.h"
#include "MTLShaderFunction.h"
#include "MTLCommandBuffer.h"
#include "MTLTextureBase.h"
#include "MTLCommandQueue.h"
#include "MTLRCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLRenderDevice::MTLRenderDevice(CAMetalLayer *metalLayer)
{
    mMetalLayer = metalLayer;
    mMetalCommandQueue = [mMetalLayer.device newCommandQueue];

    // 填充结构化设备特性
    InitializeFeatures();

    // 初始化队列管理（Metal的CommandQueue支持所有类型命令）
    // 创建图形队列（默认队列，用于渲染）
    MTLCommandQueuePtr graphicsQueue = std::make_shared<MTLCommandQueue>(
        this,
        mMetalCommandQueue,
        QueueType::Graphics,
        QueuePriority::Normal,
        0
    );
    mGraphicsQueues.push_back(graphicsQueue);

    // 创建计算队列（复用同一个CommandQueue，但在逻辑上区分）
    MTLCommandQueuePtr computeQueue = std::make_shared<MTLCommandQueue>(
        this,
        mMetalCommandQueue,
        QueueType::Compute,
        QueuePriority::Normal,
        0
    );
    mComputeQueues.push_back(computeQueue);

    // 创建传输队列（复用同一个CommandQueue，但在逻辑上区分）
    MTLCommandQueuePtr transferQueue = std::make_shared<MTLCommandQueue>(
        this,
        mMetalCommandQueue,
        QueueType::Transfer,
        QueuePriority::Normal,
        0
    );
    mTransferQueues.push_back(transferQueue);
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
    auto vertexBuffer = std::make_shared<MTLVertexBuffer>(mMetalLayer.device, mMetalCommandQueue, buffer, size, mode);
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
    auto indexBuffer = std::make_shared<MTLIndexBuffer>(mMetalLayer.device, mMetalCommandQueue, indexType, buffer, size);
    return indexBuffer;
}

RCBufferPtr MTLRenderDevice::CreateBuffer(const RCBufferDesc& desc) const
{
    return std::make_shared<MTLRCBuffer>(mMetalLayer.device, desc);
}

RCBufferPtr MTLRenderDevice::CreateBuffer(const RCBufferDesc& desc, const void* data) const
{
    return std::make_shared<MTLRCBuffer>(mMetalLayer.device, mMetalCommandQueue, desc, data);
}

/**
 根据采样描述创建纹理采样器

 @param des the description for sampler to be created.
 @return shared pointer to sampler object.
 */
TextureSamplerPtr MTLRenderDevice::CreateSamplerWithDescriptor(const SamplerDesc& des) const
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
GraphicsPipelinePtr MTLRenderDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& des) const
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
    return std::make_shared<MTLCommandBuffer>(mMetalCommandQueue, mMetalLayer, mDepthTexture, mStencilTexture, mDepthStencilTexture);
}

RCTexture2DPtr MTLRenderDevice::CreateTexture2D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (mtlFormat == MTLPixelFormatInvalid)
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
        textureDes.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }
    
    MTLRCTexture2DPtr texture = std::make_shared<MTLRCTexture2D>(mMetalLayer.device, mMetalCommandQueue, textureDes);
    texture->SetFormat(format);

    return texture;
}

RCTexture3DPtr MTLRenderDevice::CreateTexture3D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t depth,
                                    uint32_t levels) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (mtlFormat == MTLPixelFormatInvalid)
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
        textureDes.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }
    
    MTLRCTexture3DPtr texture = std::make_shared<MTLRCTexture3D>(mMetalLayer.device, mMetalCommandQueue, textureDes);
    texture->SetFormat(format);

    return texture;
}

RCTextureCubePtr MTLRenderDevice::CreateTextureCube(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (mtlFormat == MTLPixelFormatInvalid)
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
        textureDes.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }
    
    MTLRCTextureCubePtr texture = std::make_shared<MTLRCTextureCube>(mMetalLayer.device, mMetalCommandQueue, textureDes);
    texture->SetFormat(format);

    return texture;
}

RCTexture2DArrayPtr MTLRenderDevice::CreateTexture2DArray(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels,
                                    uint32_t arraySize) const
{
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (mtlFormat == MTLPixelFormatInvalid)
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
        textureDes.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
        textureDes.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        textureDes.storageMode = MTLStorageModePrivate;
        if (@available(iOS 10.0, *))
        {
            //textureDes.storageMode = MTLStorageModeMemoryless;
        }
    }
    
    MTLRCTexture2DArrayPtr texture = std::make_shared<MTLRCTexture2DArray>(mMetalLayer.device, mMetalCommandQueue, textureDes);
    texture->SetFormat(format);

    return texture;
}

CommandQueuePtr MTLRenderDevice::GetCommandQueue(QueueType type, uint32_t index) const
{
    switch (type)
    {
        case QueueType::Graphics:
            if (index < mGraphicsQueues.size())
            {
                return mGraphicsQueues[index];
            }
            break;

        case QueueType::Compute:
            if (index < mComputeQueues.size())
            {
                return mComputeQueues[index];
            }
            break;

        case QueueType::Transfer:
            if (index < mTransferQueues.size())
            {
                return mTransferQueues[index];
            }
            break;

        default:
            return nullptr;
    }

    return nullptr;
}

uint32_t MTLRenderDevice::GetCommandQueueCount(QueueType type) const
{
    switch (type)
    {
        case QueueType::Graphics:
            return (uint32_t)mGraphicsQueues.size();
        case QueueType::Compute:
            return (uint32_t)mComputeQueues.size();
        case QueueType::Transfer:
            return (uint32_t)mTransferQueues.size();
        default:
            return 0;
    }
}

// ============================================================================
// Metal Feature Set 能力查询表
//
// 数据来源: Apple "Metal Feature Set Tables" 官方 PDF 文档
// 查询策略: 从最新 Feature Set 向下逐级检测，取第一个匹配的值。
//
// macOS GPU Family 分级（按能力递增）:
//   Mac1 → Mac2(M1/M2/M3/M4, 统一为 Mac2 family)
// iOS GPU Family 分级:
//   iOS Fam1(A7) → iOS Fam2(A8) → iOS Fam3(A9) → iOS Fam4(A10/A11) → iOS Fam5(A12+)
//
// 注意: 当前 SDK (Xcode 15, macOS 14 SDK) 中可用的 GPU Family 常量:
//   macOS: MTLGPUFamilyMac1, MTLGPUFamilyMac2
//   iOS:   MTLGPUFamilyApple1~5
//   更高版本 (M1/M2/M3/M4 分离 family) 将在后续 SDK 中提供
// ============================================================================

namespace {

/// 辅助函数：设置 maxComputeWorkGroupSize[3] 数组
inline void SetWorkGroupSize(RenderDeviceFeatures::Limits& limits,
                            uint32_t x, uint32_t y, uint32_t z)
{
    limits.maxComputeWorkGroupSize[0] = x;
    limits.maxComputeWorkGroupSize[1] = y;
    limits.maxComputeWorkGroupSize[2] = z;
}

} // anonymous namespace

void MTLRenderDevice::InitializeFeatures()
{
    if (!mMetalLayer.device)
        return;

    id<MTLDevice> device = mMetalLayer.device;
    auto& L = mFeatures.limits;

    // ================================================================
    // 一、设备基本信息
    // ================================================================
    mFeatures.deviceInfo.deviceName = [device.name UTF8String];

    // Metal 无 vendorName/vendorID 属性，需从 device.name 解析厂商
    //   Apple GPU:   "Apple M1/M2/M3/M4 ...", "Apple GPU"
    //   AMD:         "Radeon", "Radeon Pro"
    //   NVIDIA:      "GeForce", "Quadro", "Tesla"
    //   Intel:       "Intel HD/Iris/UHD/Arc"
    {
        NSString *name = device.name;
        if ([name containsString:@"Apple"])
            mFeatures.deviceInfo.vendorName = "Apple";
        else if ([name containsString:@"Radeon"] || [name containsString:@"radeon"])
            mFeatures.deviceInfo.vendorName = "AMD";
        else if ([name containsString:@"GeForce"] || [name containsString:@"Quadro"] ||
                 [name containsString:@"Tesla"] || [name containsString:@"NVIDIA"])
            mFeatures.deviceInfo.vendorName = "NVIDIA";
        else if ([name containsString:@"Intel"] || [name containsString:@"intel"])
            mFeatures.deviceInfo.vendorName = "Intel";
        else
            mFeatures.deviceInfo.vendorName = "Unknown";  // 兜底
    }

    // Apple Silicon 全是 SoC 集成 GPU；Intel Mac 可通过名称判断
#if TARGET_OS_MAC
    bool isDiscrete = ([device.name containsString:@"Radeon"] ||
                       [device.name containsString:@"Radeon Pro"] ||
                       [device.name containsString:@"GeForce"] ||
                       [device.name containsString:@"Quadro"]);
    mFeatures.deviceInfo.deviceType = isDiscrete
        ? RenderDeviceFeatures::DeviceInfo::DeviceType::Discrete
        : RenderDeviceFeatures::DeviceInfo::DeviceType::Integrated;
#else
    mFeatures.deviceInfo.deviceType = RenderDeviceFeatures::DeviceInfo::DeviceType::Integrated;
#endif

    // ================================================================
    // 二、基于 Metal Feature Set 的 Limits 填充
    //
    // 策略：从最高版本开始检测，首次匹配即采用该组数值。
    // [device supportsFamily:] 运行时 API 在 macOS 11+ / iOS 13+ 全部可用。
    // ================================================================

    // --- 2a. macOS 平台 ---
#if TARGET_OS_MAC
    // macOS GPU Family 2 (Apple Silicon M1+/AMD RDNA2+/NVIDIA Ampere+)
    //   覆盖所有 Apple Silicon (M1/M2/M3/M4) 以及现代桌面独立显卡
    //   注: 未来 SDK 若分离出 MTLGPUFamilyAppleM1/M2/M3/M4 可在此处做更细粒度区分
    if (@available(macOS 11.0, *) && [device supportsFamily:MTLGPUFamilyMac2])
    {
        L.maxTextureSize2D                 = 16384;
        L.maxTextureSize3D                 = 2048;
        L.maxTextureArrayLayers            = 2048;
        L.maxCubeMapSize                   = 16384;
        L.maxColorAttachments              = 8;
        L.maxComputeWorkGroupInvocations   = 1024;
        SetWorkGroupSize(L, 1024, 1024, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0); // 1x/2x/4x MSAA
        mFeatures.shader.int8              = true;  // Apple Silicon 均支持 int8
    }
    // macOS GPU Family 1 (Intel HD/Iris, AMD GCN, NVIDIA Kepler/Maxwell)
    else if (@available(macOS 11.0, *) && [device supportsFamily:MTLGPUFamilyMac1])
    {
        L.maxTextureSize2D                 = 16384;
        L.maxTextureSize3D                 = 2048;
        L.maxTextureArrayLayers            = 2048;
        L.maxCubeMapSize                   = 16384;
        L.maxColorAttachments              = 4;
        L.maxComputeWorkGroupInvocations   = 512;
        SetWorkGroupSize(L, 512, 512, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0);
    }

    // --- 2b. iOS 平台 ---
#elif TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
    // iOS GPU Family 5 (A12 Bionic+, iOS 13+) — 最新 iOS GPU 家族
    if (@available(iOS 13.0, *) && [device supportsFamily:MTLGPUFamilyApple5])
    {
        L.maxTextureSize2D                 = 16384;
        L.maxTextureSize3D                 = 2048;
        L.maxTextureArrayLayers            = 2048;
        L.maxCubeMapSize                   = 16384;
        L.maxColorAttachments              = 8;
        L.maxComputeWorkGroupInvocations   = 1024;
        SetWorkGroupSize(L, 1024, 1024, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0);
        mFeatures.shader.int8              = true;
    }
    // iOS GPU Family 4 (A10/A10X/A11, iOS 11+)
    else if (@available(iOS 11.0, *) && [device supportsFamily:MTLGPUFamilyApple4])
    {
        L.maxTextureSize2D                 = 8192;
        L.maxTextureSize3D                 = 2048;
        L.maxTextureArrayLayers            = 2048;
        L.maxCubeMapSize                   = 8192;
        L.maxColorAttachments              = 8;
        L.maxComputeWorkGroupInvocations   = 512;
        SetWorkGroupSize(L, 512, 512, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0);
    }
    // iOS GPU Family 3 (A9, iOS 10+)
    else if (@available(iOS 10.0, *) && [device supportsFamily:MTLGPUFamilyApple3])
    {
        L.maxTextureSize2D                 = 8192;
        L.maxTextureSize3D                 = 2048;
        L.maxTextureArrayLayers            = 2048;
        L.maxCubeMapSize                   = 8192;
        L.maxColorAttachments              = 4;
        L.maxComputeWorkGroupInvocations   = 512;
        SetWorkGroupSize(L, 512, 512, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0);
    }
    // iOS GPU Family 2 (A8, iOS 9+)
    else if (@available(iOS 9.0, *) && [device supportsFamily:MTLGPUFamilyApple2])
    {
        L.maxTextureSize2D                 = 8192;
        L.maxTextureSize3D                 = 1024;
        L.maxTextureArrayLayers            = 2048;
        L.maxCubeMapSize                   = 8192;
        L.maxColorAttachments              = 4;
        L.maxComputeWorkGroupInvocations   = 256;
        SetWorkGroupSize(L, 256, 256, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0);
    }
    // iOS GPU Family 1 (A7, iOS 8+) — 兜底最低配置
    else if (@available(iOS 8.0, *))
    {
        L.maxTextureSize2D                 = 4096;
        L.maxTextureSize3D                 = 1024;
        L.maxTextureArrayLayers            = 512;
        L.maxCubeMapSize                   = 4096;
        L.maxColorAttachments              = 4;
        L.maxComputeWorkGroupInvocations   = 256;
        SetWorkGroupSize(L, 256, 256, 64);
        L.maxSampleCountMaskBits           = (1u << 2) | (1u << 1) | (1u << 0);
    }
#endif /* TARGET_OS_MAC / TARGET_OS_IPHONE */

    // ================================================================
    // 三、通用字段（不随 Feature Set 变化，或由运行时 API 直接返回）
    // ================================================================

    // 顶点输入：Metal 规范统一为 31 个 buffer + 31 个 attribute slot
    L.maxVertexBufferBindings = 31;
    L.maxVertexInputAttributes = 31;

    // 各向异性过滤：Metal 所有支持 family 的 GPU 均为 16x
    L.maxSamplerAnisotropy = 16.0f;

    // Buffer 对齐：Metal 统一为 4 字节（与 Vulkan 256 不同）
    L.minUniformBufferOffsetAlignment  = 4;
    L.minStorageBufferOffsetAlignment  = 4;

    // Compute 共享内存：直接从设备 API 获取运行时真实值
    L.maxComputeSharedMemorySize = static_cast<uint32_t>(device.maxThreadgroupMemoryLength);

    // Compute WorkGroup Count：Metal 不设上限（受 VRAM 约束）
    L.maxComputeWorkGroupCount[0] = UINT32_MAX;
    L.maxComputeWorkGroupCount[1] = UINT32_MAX;
    L.maxComputeWorkGroupCount[2] = UINT32_MAX;

    // Framebuffer 尺寸上限 = 2D 纹理上限
    L.maxFramebufferWidth  = L.maxTextureSize2D;
    L.maxFramebufferHeight = L.maxTextureSize2D;
    L.maxFramebufferLayers = L.maxTextureArrayLayers;

    // 时间戳周期：Metal MTLCounter 近似 1ns 粒度（Apple Silicon）
    L.timestampPeriod = 1;

    // ================================================================
    // 四、Shader 能力（基于 Feature Set + OS 版本组合判断）
    // ================================================================

    // SIMD-group 操作：所有 Apple GPU 均原生支持 quad-shuffle / wave ops
    mFeatures.shader.waveIntrinsics = true;

    // float16 (half)：Metal shading language 内建 half 类型，全平台支持
    mFeatures.shader.float16 = true;

    // int64 atomics：Apple GPU 不支持 64-bit atomic，仅部分桌面 GPU 通过扩展支持
    mFeatures.shader.int64Atomics = false;

    // float64 (double)：Metal shading language 不支持 double
    mFeatures.shader.float64 = false;

    // Mesh / Task Shader (Object / Mesh Pipeline):
    //   要求: macOS 14+ (M3+) 或 iOS 17+ (A17+), 且需 Apple GPU (Mac2 family)
    mFeatures.shader.meshShader = false;
    mFeatures.shader.taskShader = false;
#if TARGET_OS_MAC
    #if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000
    if (@available(macOS 14.0, *))
    {
        // Mac2 family 包含 Apple Silicon M1+，均支持 Object/Mesh Pipeline
        if ([device supportsFamily:MTLGPUFamilyMac2])
        {
            mFeatures.shader.meshShader = true;
            mFeatures.shader.taskShader = true;
        }
    }
    #endif
#elif TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
    #if defined(__IPHONE_OS_VERSION_MAX_ALLOWED) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 170000
    if (@available(iOS 17.0, *))
    {
        if ([device supportsFamily:MTLGPUFamilyApple5])
        {
            mFeatures.shader.meshShader = true;
            mFeatures.shader.taskShader = true;
        }
    }
    #endif
#endif

    // Ray Tracing: MPSAccelerationStructure (macOS 12+ / iOS 16+, Apple Silicon)
    mFeatures.shader.rayTracing = false;
#if TARGET_OS_MAC
    if (@available(macOS 12.0, *))
    {
        mFeatures.shader.rayTracing = [device supportsFamily:MTLGPUFamilyMac2];
    }
#elif TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
    if (@available(iOS 16.0, *))
    {
        mFeatures.shader.rayTracing = [device supportsFamily:MTLGPUFamilyApple5]
                                    || [device supportsFamily:MTLGPUFamilyApple6]; // A17+ 预留
    }
#endif

    // ================================================================
    // 五、资源与格式能力
    // ================================================================

    // BC 压缩: Apple GPU 不原生支持 BC；Intel/AMD/NVIDIA Mac 通过硬件解码支持
    mFeatures.resource.textureCompressionBC = false;
#if TARGET_OS_MAC
    if (@available(macOS 11.0, *))
    {
        // Mac2 中非 Apple Silicon 的设备（Intel AMD NVIDIA GPU）通常支持 BC 硬件解码
        if (![device supportsFamily:MTLGPUFamilyMac2] ||
            ![device.name containsString:@"Apple"])
        {
            mFeatures.resource.textureCompressionBC = true;
        }
    }
#endif

    // ASTC: Apple GPU 全家族原生支持 ASTC（HDR/LDR Sliced/Full block）
    mFeatures.resource.textureCompressionASTC = false;
#if TARGET_OS_MAC
    // macOS 上仅 Apple Silicon 支持 ASTC（通过 Mac2 family + 设备名判断）
    if (@available(macOS 11.0, *))
    {
        mFeatures.resource.textureCompressionASTC =
            ([device supportsFamily:MTLGPUFamilyMac2] &&
             [device.name containsString:@"Apple"]);
    }
#elif TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
    mFeatures.resource.textureCompressionASTC =
        ([device supportsFamily:MTLGPUFamilyApple1] ||
         [device supportsFamily:MTLGPUFamilyApple2] ||
         [device supportsFamily:MTLGPUFamilyApple3] ||
         [device supportsFamily:MTLGPUFamilyApple4] ||
         [device supportsFamily:MTLGPUFamilyApple5]);
#endif

    // ETC2: Apple GPU 家族 3+ 支持（iOS 10+），macOS Apple Silicon 也支持
    mFeatures.resource.textureCompressionETC2 = true;

    // PVRTC: 仅旧 iOS GPU（Fam1~3），Apple Silicon 已弃用但兼容
    mFeatures.resource.textureCompressionPVRTC = false;
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
    mFeatures.resource.textureCompressionPVRTC =
        [device supportsFamily:MTLGPUFamilyApple1] ||
        [device supportsFamily:MTLGPUFamilyApple2] ||
        [device supportsFamily:MTLGPUFamilyApple3];
#endif

    // Bindless: Metal 使用 Argument Buffer 机制实现等价功能
    //   Argument Buffer 是 Metal 的"bindless"方案（encapsulated indexed resource table）
    mFeatures.resource.bindlessResources = true;

    // Sparse Texture: Metal 不支持 Tiled/Sparse Residency（Vulkan sparse binding 等价物）
    mFeatures.resource.sparseTextures = false;

    // BGRA8: MTLPixelFormatBGRA8Unorm 在所有 Metal 设备上可用
    mFeatures.resource.formatBGRA8 = true;

    // ================================================================
    // 六、高级渲染功能
    // ================================================================

    // VRS (Variable Rate Shading): 目前 Metal 不暴露 VRS API
    mFeatures.advanced.variableRateShading = false;

    // Async Compute: Metal 使用统一 Command Queue 架构，
    //   图形/计算命令可在同一队列并行提交（依赖 MTLCommandBuffer.enqueue）
    mFeatures.advanced.asyncCompute = true;

    // Multiview: Metal 通过 [[render_target_array_index]] + instanced rendering 实现
    //   在 macOS 10.13+ / iOS 11+ 全平台可用
    mFeatures.advanced.multiview = true;
}

NAMESPACE_RENDERCORE_END
