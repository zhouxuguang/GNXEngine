//
//  MTLTexture2D.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLTexture2D.h"
#include "BitHacks.h"

NAMESPACE_RENDERCORE_BEGIN

MTLTexture2D::MTLTexture2D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const TextureDescriptor& des) : Texture2D(des)
{
    //command需要产生mipmap的纹理
    mCommandQueue = commandQueue;
    
    MTLPixelFormat format = ConvertTextureFormatToMetal(des.format);
    if (format == MTLPixelFormatInvalid)
    {
        assert(false);
        return;
    }
    
    if (0 == des.width || 0 == des.height)
    {
        assert(false);
        return;
    }
    
    //需要检查宽高，创建mipmap纹理时
    BOOL mipmap = (des.mipmaped && mathutil::IsPowerOfTwo(des.width) && mathutil::IsPowerOfTwo(des.height));
    mBytesPerRow = des.bytesPerRow;
    
    MTLTextureDescriptor *textureDes = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:des.width height:des.height mipmapped:mipmap];
    
    if (@available(iOS 9.0, macOS 10.11, *))
    {
        //textureDes.resourceOptions = MTLResourceStorageModePrivate;
        textureDes.usage = des.usage;
        //textureDes.storageMode = MTLStorageModePrivate;
    }
    
    if (textureDes)
    {
        mTexture = [device newTextureWithDescriptor:textureDes];
    }
    
    if (mipmap)
    {
        GenerateMipmapsTexture();
    }
    
    mDevice = device;
}

MTLTexture2D::~MTLTexture2D()
{
}

void MTLTexture2D::setTextureData(const uint8_t* imageData)
{
    if (mTexture == nil || imageData == nullptr)
    {
        return;
    }
    
    MTLRegion region = MTLRegionMake2D(0, 0, mTexture.width, mTexture.height);
    [mTexture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:mBytesPerRow];
    
    if (mTexture.mipmapLevelCount > 1)
    {
        GenerateMipmapsTexture();
    }
}

void MTLTexture2D::replaceRegion(const Rect2D& rect, const uint8_t* imageData, uint32_t mipMapLevel)
{
    if (mTexture == nil || imageData == nullptr)
    {
        return;
    }
    
    MTLRegion region = MTLRegionMake2D(rect.offsetX, rect.offsetY, rect.width, rect.height);
    [mTexture replaceRegion:region mipmapLevel:mipMapLevel withBytes:imageData bytesPerRow:mBytesPerRow];
    
    if (mTexture.mipmapLevelCount > 1)
    {
        GenerateMipmapsTexture();
    }
}

bool MTLTexture2D::isValid() const
{
    return mTexture != nil;
}

void MTLTexture2D::GenerateMipmapsTexture()
{
    if (nil == mCommandQueue)
    {
        return;
    }
    
    id<MTLCommandBuffer> commandBuffer = [mCommandQueue commandBuffer];
    id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
    [blitEncoder generateMipmapsForTexture : mTexture];
    [blitEncoder endEncoding];
    [commandBuffer commit];

    // block
    [commandBuffer waitUntilCompleted];
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
        depthStencilDescriptor.resourceOptions = MTLResourceStorageModePrivate;
        depthStencilDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        depthStencilDescriptor.storageMode = MTLStorageModePrivate;
        
#ifndef TARGET_OS_MAC
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
