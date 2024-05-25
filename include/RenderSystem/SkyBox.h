//
//  SkyBox.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#ifndef GNX_ENGINE_SKYBOX_INCLUDE_HNSNF
#define GNX_ENGINE_SKYBOX_INCLUDE_HNSNF

#include "RenderCore/RenderDevice.h"
#include "ImageCodec/VImage.h"
#include "RSDefine.h"
#include "SceneObject.h"

USING_NS_RENDERCORE
USING_NS_IMAGECODEC

NS_RENDERSYSTEM_BEGIN

class SkyBox : public SceneObject
{
public:
    SkyBox();
    
    ~SkyBox();
    
    static SkyBox* create(RenderDevicePtr renderDevice, const char* positive_x, const char* negative_x, const char* positive_y,
                          const char* negative_y, const char* positive_z, const char* negative_z);
    
    static SkyBox* create(RenderDevicePtr renderDevice, VImagePtr positive_x, VImagePtr negative_x, VImagePtr positive_y,
                          VImagePtr negative_y, VImagePtr positive_z, VImagePtr negative_z);
    
    static void destroy(SkyBox* skybox);
    
    void Render(const RenderEncoderPtr &renderEncoder, UniformBufferPtr cameraUBO);
    
private:
    bool init(RenderDevicePtr renderDevice, VImagePtr positive_x, VImagePtr negative_x, VImagePtr positive_y,
              VImagePtr negative_y, VImagePtr positive_z, VImagePtr negative_z);
    
    void initBuffers(RenderDevicePtr renderDevice);
    
    TextureCubePtr mTextureCube = nullptr;
    TextureSamplerPtr mTextureSampler = nullptr;
    VertexBufferPtr mVertexBuffer = nullptr;
    
    GraphicsPipelinePtr mPipeline = nullptr;
    RenderDevicePtr mRenderDevice = nullptr;
};

typedef std::shared_ptr<SkyBox> SkyBoxPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SKYBOX_INCLUDE_HNSNF */
