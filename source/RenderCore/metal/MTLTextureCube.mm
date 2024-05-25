//
//  MTLTextureCube.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#include "MTLTextureCube.h"

NAMESPACE_RENDERCORE_BEGIN

MTLTextureCube::MTLTextureCube(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const std::vector<TextureDescriptor>& desArray) : TextureCube(desArray)
{
    mDevice = device;
    mCommandQueue = commandQueue;
    // metal texture 的每个面的宽高都必须是一样的，然后宽和高也必须是一样的
    if (desArray.size() != 6)
    {
        return;
    }
    
    TextureFormat format = desArray[0].format;
    MTLPixelFormat mtlFormat = ConvertTextureFormatToMetal(format);
    if (mtlFormat == MTLPixelFormatInvalid)
    {
        NSLog(@"立方体纹理格式不正确！！！");
        assert(false);
        return;
    }
    
    if (desArray[0].width != desArray[0].height || 0 == desArray[0].width || 0 == desArray[0].height)
    {
        NSLog(@"立方体纹理宽和高不一样！！！");
        assert(false);
        return;
    }
    
    //压缩格式不支持实时生成mipmap
    
    mTextureDes = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:mtlFormat size:desArray[0].width mipmapped:false];
    mTexture = [mDevice newTextureWithDescriptor : mTextureDes];
    mBytesPerRow = desArray[0].bytesPerRow;
}

MTLTextureCube::~MTLTextureCube()
{
    //
}

/**
  set image data

 @param imageData image data
 */
void MTLTextureCube::setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData)
{
    if (0 == imageSize || nullptr == imageData)
    {
        return;
    }
    
    MTLRegion region = MTLRegionMake2D(0, 0, mTextureDes.width, mTextureDes.height);
    NSUInteger bytesPerImage = mBytesPerRow * mTextureDes.width;
    
    [mTexture replaceRegion:region
               mipmapLevel:0
                     slice:(NSUInteger)cubeFace
                 withBytes:imageData
               bytesPerRow:mBytesPerRow
             bytesPerImage:bytesPerImage];
    
//    id<MTLCommandBuffer> mipmapCommandBuffer = [mCommandQueue commandBuffer];
//    id<MTLBlitCommandEncoder> blitCommandEncoder = [mipmapCommandBuffer blitCommandEncoder];
//    [blitCommandEncoder generateMipmapsForTexture:mTexture];
//    [blitCommandEncoder endEncoding];
//    [mipmapCommandBuffer commit];
}

/**
 纹理是否有效

 @return ture or false
 */
bool MTLTextureCube::isValid() const
{
    return mTexture != nil;
}

NAMESPACE_RENDERCORE_END
