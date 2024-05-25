//
//  ShadowMapping.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/15.
//

#ifndef RENDERSYSTEM_SHADOWMAPPING_INCLUDE_H
#define RENDERSYSTEM_SHADOWMAPPING_INCLUDE_H

#include "Plane.h"
#include "RenderCore/RenderTexture.h"
#include "RenderCore/TextureSampler.h"
#include "RenderCore/FrameBuffer.h"
#include "RenderCore/RenderDevice.h"
#include "RenderCore/GraphicsPipeline.h"


NS_RENDERSYSTEM_BEGIN

class ShadowMapping
{
public:
    ShadowMapping(const RenderDevicePtr& renderDevice);
    
    ~ShadowMapping();
    
    //准备阴影渲染的程序
    void SetUp();
    
private:
    FrameBufferPtr m_DepthFrameBuffer = 0;
    Texture2DPtr m_DepthTexture = 0;
    TextureSamplerPtr m_DepthTextureSampler = 0;
    GraphicsPipelinePtr m_GraphicsPipeline = 0;
    
};

NS_RENDERSYSTEM_END

#endif /* RENDERSYSTEM_SHADOWMAPPING_INCLUDE_H */
