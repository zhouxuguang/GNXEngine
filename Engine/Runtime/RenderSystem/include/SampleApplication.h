//
//  SampleApplication.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/6/4.
//

#ifndef GNX_ENGINE_SAMPLE_APPLICATION_INCLUDE_HGH
#define GNX_ENGINE_SAMPLE_APPLICATION_INCLUDE_HGH

#include "RSDefine.h"

#include "SceneManager.h"
#include "SceneNode.h"
#include "ArcballManipulate.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "SkyBoxNode.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "RenderEngine.h"
#include "Runtime/BaseLib/include/DateTime.h"

NS_RENDERSYSTEM_BEGIN

class SampleApplication
{
public:
    SampleApplication(RenderDeviceType deviceType, ViewHandle nativeWindow);
    
    virtual ~SampleApplication();
    
    virtual void Init();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual void Render(float deltaTime);
    
protected:
    RenderDevicePtr mRenderdevice;
    uint32_t mWidth;
    uint32_t mHeight;
    
//    RenderTexturePtr mRenderTexture;
//    RenderTexturePtr mDepthStencilTexture;
    
    SceneManager* mSceneManager;
    
    //RenderTexturePtr mComputeTexture;
    ComputePipelinePtr mComputePipeline;
    
    uint64_t mLastTime;
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SAMPLE_APPLICATION_INCLUDE_HGH */
