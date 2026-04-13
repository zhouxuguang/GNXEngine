//
//  SkyBox.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#ifndef GNX_ENGINE_SKYBOX_INCLUDE_HNSNF
#define GNX_ENGINE_SKYBOX_INCLUDE_HNSNF

#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "RSDefine.h"
#include "SceneObject.h"

USING_NS_RENDERCORE
USING_NS_IMAGECODEC

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API SkyBox : public SceneObject
{
public:
    SkyBox();
    
    ~SkyBox();
    
    static SkyBox* create(RenderDevicePtr renderDevice, const char* positive_x, const char* negative_x, const char* positive_y,
                          const char* negative_y, const char* positive_z, const char* negative_z);
    
    static SkyBox* create(RenderDevicePtr renderDevice, VImagePtr positive_x, VImagePtr negative_x, VImagePtr positive_y,
                          VImagePtr negative_y, VImagePtr positive_z, VImagePtr negative_z);

    /**
     * @brief 从已加载的 KTX Cubemap 纹理创建天空盒
     * @param renderDevice 渲染设备
     * @param textureCube 已加载的 Cubemap 纹理（如通过 ImageTextureUtil::LoadKTXCubemapTexture 加载）
     */
    static SkyBox* createFromTexture(RenderDevicePtr renderDevice, RCTextureCubePtr textureCube);

    static void destroy(SkyBox* skybox);
    
    void Render(const RenderEncoderPtr &renderEncoder, UniformBufferPtr cameraUBO);
    
private:
    bool init(RenderDevicePtr renderDevice, VImagePtr positive_x, VImagePtr negative_x, VImagePtr positive_y,
              VImagePtr negative_y, VImagePtr positive_z, VImagePtr negative_z);

    bool initFromTexture(RenderDevicePtr renderDevice, RCTextureCubePtr textureCube);

    void initBuffers(RenderDevicePtr renderDevice);
    
    RCTextureCubePtr mTextureCube = nullptr;
    TextureSamplerPtr mTextureSampler = nullptr;
    VertexBufferPtr mVertexBuffer = nullptr;
    
    GraphicsPipelinePtr mPipeline = nullptr;
    RenderDevicePtr mRenderDevice = nullptr;
};

typedef std::shared_ptr<SkyBox> SkyBoxPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SKYBOX_INCLUDE_HNSNF */
