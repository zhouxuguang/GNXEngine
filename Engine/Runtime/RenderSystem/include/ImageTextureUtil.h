//
//  ImageTextureUtil.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#ifndef GNX_ENGINE_IMAGE_TEXTURE_UTIL_INCLUDE_JKFNNN
#define GNX_ENGINE_IMAGE_TEXTURE_UTIL_INCLUDE_JKFNNN

#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "RSDefine.h"

USING_NS_RENDERCORE
USING_NS_IMAGECODEC

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API ImageTextureUtil
{
public:
    static TextureDesc getTextureDescriptor(const VImage& image);
    
    static RCTexture2DPtr TextureFromFile(const char *filename);
    
    static RCTexture2DPtr CreateDiffuseTexture(float r, float g, float b);

    static RCTexture2DPtr CreateMetalRoughTexture();

    static RCTexture2DPtr CreateNormalTexture();

    static RCTexture2DPtr CreateEmmisveTexture();

    static RCTexture2DPtr CreateAOTexture();
};

RCTextureCubePtr LoadEquirectangularMap(const std::string& fileName);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_IMAGE_TEXTURE_UTIL_INCLUDE_JKFNNN */
