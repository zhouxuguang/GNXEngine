//
//  MTLTextureBase.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLTextureBase.h"
#include <algorithm>
#include <vector>

NAMESPACE_RENDERCORE_BEGIN

// Metal replaceRegion 要求 bytesPerRow 满足最小对齐约束（macOS/iOS 通常为 64 字节）
static const NSUInteger kMinBytesPerRowAlignment = 64;

// 块压缩格式（BC6H/BC7/BC1/BC3/BC5/ETC/EAC/ASTC/PVRTC 等）。块压缩纹理数据按固定
// 尺寸的“块”(block) 组织（BC/ETC=4x4，ASTC 可为 4x4/5x5/6x6/8x8/10x10/12x12，
// PVRTC 2bpp=8x4 …）。mip 较小时 bytesPerRow 会 < 64，此时若按“像素行”逐行 padding
// 会越界/损坏，必须按“块行”(block row) 对齐处理；块行数与对齐宽度须按真实块尺寸计算，
// 不能写死 4x4。
static bool IsBlockCompressedFormat(TextureFormat fmt)
{
    return IsAnyCompressedTextureFormat(fmt);
}

// 返回某压缩格式的块高（texel）。用于把“块行数”算对：ASTC 8x8 的块行数 = ceil(h/8) 等。
static uint32_t GetBlockHeightForFormat(TextureFormat fmt)
{
    TextureBlockInfo info = GetCompressedTextureBlockInfo(fmt);
    return (info.bytesPerBlock > 0) ? info.blockHeight : 1u;
}

MTLTextureBase::MTLTextureBase(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes) :
    RCTexture(TextureType_Unkown)
{
    //command需要产生mipmap的纹理
    mCommandQueue = commandQueue;
    mDevice = device;
    
    /*
     All feature sets support the MTLPixelFormatDepth32Float_Stencil8 pixel format. Only some devices that support the OSX_GPUFamily1_v1 feature set
     also support the MTLPixelFormatDepth24Unorm_Stencil8 pixel format. Query the depth24Stencil8PixelFormatSupported property of a MTLDevice object
     to determine whether the pixel format is supported or not.
     
     bool issupportd = mDevice.depth24Stencil8PixelFormatSupported;
     issupportd : false mac mini 2018
     
     https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/WhatsNewiniOS9andOSX1011/WhatsNewiniOS9andOSX1011.html
     */
    if (textureDes)
    {
        mTexture = [device newTextureWithDescriptor:textureDes];
    }
}

MTLTextureBase::~MTLTextureBase()
{
}

void MTLTextureBase::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    if (mTexture == nil || pixelBytes == nullptr)
    {
        return;
    }
    
    MTLRegion region = MTLRegionMake3D(rect.offsetX, rect.offsetY, 0, rect.width, rect.height, 1);
    
    // Metal replaceRegion 要求 bytesPerRow >= 64
    // 只在 bytesPerRow < 64 时才需要做对齐处理
    if (bytesPerRow >= kMinBytesPerRowAlignment)
    {
        // 已满足对齐要求，直接上传
        [mTexture replaceRegion:region mipmapLevel:level slice:slice
                      withBytes:pixelBytes bytesPerRow:bytesPerRow bytesPerImage:bytesPerImage];
    }
    else if (IsBlockCompressedFormat(GetTextureFormat()))
    {
        // 块压缩纹理（BC6H/BC7/BC1/ASTC/PVRTC 等）在低 mip 时 bytesPerRow < 64。
        // 数据按固定尺寸“块”组织：块行数 = ceil(height / blockHeight)，每块行宽 =
        // bytesPerRow（一横排块的总字节，loader 已按 imageSize / blockRows 计算）。
        // 块尺寸随格式变化（BC/ETC=4x4，ASTC 可 5x5/6x6/8x8/10x10/12x12，
        // PVRTC 2bpp=8x4 …），因此必须用真实块高算块行数，不能写死 4x4。
        const uint32_t blockHeight = GetBlockHeightForFormat(GetTextureFormat());
        const uint32_t blockRows = (rect.height + blockHeight - 1) / blockHeight;

        // 把每块行拷贝到按 64 字节对齐的缓冲区。64 字节是各压缩格式块字节
        // （8 或 16 字节）的整数倍，满足 Metal 对压缩纹理 bytesPerRow 的要求。
        NSUInteger paddedRowBytes = kMinBytesPerRowAlignment;   // 64，>= 实际 bytesPerRow
        std::vector<uint8_t> paddedBuffer(paddedRowBytes * blockRows);
        for (uint32_t r = 0; r < blockRows; ++r)
        {
            memcpy(paddedBuffer.data() + r * paddedRowBytes,
                   pixelBytes + r * bytesPerRow,
                   bytesPerRow);
        }
        NSUInteger alignedBytesPerImage = paddedRowBytes * blockRows;
        [mTexture replaceRegion:region mipmapLevel:level slice:slice
                      withBytes:paddedBuffer.data() bytesPerRow:paddedRowBytes
                   bytesPerImage:alignedBytesPerImage];
    }
    else
    {
        // bytesPerRow < 64，需要逐行拷贝到对齐缓冲区（非压缩格式）
        uint32_t height = rect.height;
        std::vector<uint8_t> paddedBuffer(kMinBytesPerRowAlignment * height);
        
        for (uint32_t row = 0; row < height; ++row)
        {
            memcpy(paddedBuffer.data() + row * kMinBytesPerRowAlignment,
                   pixelBytes + row * bytesPerRow,
                   bytesPerRow);
            // 多余部分自动被 zero-init（std::vector），不影响 GPU 采样
        }
        
        NSUInteger alignedBytesPerImage = kMinBytesPerRowAlignment * height;
        
        [mTexture replaceRegion:region mipmapLevel:level slice:slice
                      withBytes:paddedBuffer.data() bytesPerRow:kMinBytesPerRowAlignment
                   bytesPerImage:alignedBytesPerImage];
    }
}

void MTLTextureBase::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    ReplaceRegion(rect, level, 0, pixelBytes, bytesPerRow, 0);
}

uint32_t MTLTextureBase::GetWidth() const
{
    return (uint32_t)mTexture.width;
}

uint32_t MTLTextureBase::GetHeight() const
{
    return (uint32_t)mTexture.height;
}

uint32_t MTLTextureBase::GetDepth() const
{
    return (uint32_t)mTexture.depth;
}

uint32_t MTLTextureBase::GetMipLevels() const
{
    return (uint32_t)mTexture.mipmapLevelCount;
}

uint32_t MTLTextureBase::GetLayerCount() const
{
    return (uint32_t)mTexture.arrayLength;
}

void MTLTextureBase::SetName(const char* name)
{
    @autoreleasepool
    {
        if (name)
        {
            mTexture.label = [NSString stringWithUTF8String:name];
        }
    }
}

id<MTLTexture> MTLTextureBase::getMTLTextureView(uint32_t mipLevel, uint32_t slice)
{
    // mipLevel=0 且 slice=0 直接返回原始纹理，无需创建 view
    if ((mipLevel == 0 && slice == 0) || mTexture == nil)
    {
        return mTexture;
    }

    // 查找缓存
    TextureViewKey key = { mipLevel, slice };
    auto it = mTextureViews.find(key);
    if (it != mTextureViews.end())
    {
        return it->second;
    }

    // 创建新 view 并缓存
    NSRange levelRange = NSMakeRange(mipLevel, 1);
    NSRange sliceRange = NSMakeRange(slice, 1);
    id<MTLTexture> view = [mTexture newTextureViewWithPixelFormat:mTexture.pixelFormat
                                                      textureType:MTLTextureType2D
                                                           levels:levelRange
                                                           slices:sliceRange];
    mTextureViews[key] = view;
    return view;
}

#pragma mark MTLRCTexture2D

MTLRCTexture2D::MTLRCTexture2D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes), RCTexture(TextureType_2D)
{
}

MTLRCTexture2D::~MTLRCTexture2D()
{
    //
}

void MTLRCTexture2D::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    MTLTextureBase::ReplaceRegion(rect, level, 0, pixelBytes, bytesPerRow, 0);
}

#pragma mark MTLRCTexture3D

MTLRCTexture3D::MTLRCTexture3D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes), RCTexture(TextureType_3D)
{
}

MTLRCTexture3D::~MTLRCTexture3D()
{
    //
}

void MTLRCTexture3D::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    MTLTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

#pragma mark MTLRCTextureCube

MTLRCTextureCube::MTLRCTextureCube(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes), RCTexture(TextureType_CUBE)
{
}

MTLRCTextureCube::~MTLRCTextureCube()
{
    //
}

void MTLRCTextureCube::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    MTLTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

#pragma mark MTLRCTexture2DArray

MTLRCTexture2DArray::MTLRCTexture2DArray(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes), RCTexture(TextureType_2D_ARRAY)
{
}

MTLRCTexture2DArray::~MTLRCTexture2DArray()
{
    //
}

void MTLRCTexture2DArray::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    MTLTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

static id<MTLTexture> createDefaultDepthStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height, MTLPixelFormat pixelFormat)
{
    //创建深度纹理
    MTLTextureDescriptor *depthStencilDescriptor
                            = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:NO];
    
    if (@available(iOS 9.0, macOS 10.11, *))
    {
        depthStencilDescriptor.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
        depthStencilDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        depthStencilDescriptor.storageMode = MTLStorageModePrivate;
        
#if TARGET_OS_IOS
        if (@available(iOS 10.0, *))
        {
            depthStencilDescriptor.storageMode = MTLStorageModeMemoryless;
        }
#endif
    }
    depthStencilDescriptor.textureType = MTLTextureType2D;
    id<MTLTexture> depthStencilTexture = [device newTextureWithDescriptor:depthStencilDescriptor];
    
    return depthStencilTexture;
}

id<MTLTexture> createDepthStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
    id<MTLTexture> depthTexture = createDefaultDepthStencilTexture(device, width, height, MTLPixelFormatDepth32Float_Stencil8);
    [depthTexture setLabel:@"Default Depth Stencil Texture"];
    
    return depthTexture;
}

//创建深度纹理
id<MTLTexture> createDepthTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
    id<MTLTexture> depthTexture = createDefaultDepthStencilTexture(device, width, height, MTLPixelFormatDepth32Float);
    [depthTexture setLabel:@"Default Depth Texture"];
    
    return depthTexture;
}

//创建模板纹理
id<MTLTexture> createStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
    id<MTLTexture> stencilTexture = createDefaultDepthStencilTexture(device, width, height, MTLPixelFormatStencil8);
    [stencilTexture setLabel:@"Default Stencil Texture"];
    
    return stencilTexture;
}

NAMESPACE_RENDERCORE_END
