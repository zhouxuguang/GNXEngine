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

    /**
     * @brief 运行时生成 BRDF LUT 纹理（Split-Sum 近似预积分表）
     * @param imageSize LUT 分辨率（通常 256 或 512，必须是 2 的幂）
     * @param samples Monte Carlo 采样数（建议 1024）
     * @return RG16Float 格式的 2D 纹理（R=scale, G=bias）
     */
    static RCTexture2DPtr CreateBRDFLUTTexture(uint32_t imageSize = 512, uint32_t samples = 1024);
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_IMAGE_TEXTURE_UTIL_INCLUDE_JKFNNN */
