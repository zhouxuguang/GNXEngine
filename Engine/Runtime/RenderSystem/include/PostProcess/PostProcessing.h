//
//  PostProcessing.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/21.
//

#ifndef GNX_ENGINE_POST_PROCEESING_INCLUDE_H
#define GNX_ENGINE_POST_PROCEESING_INCLUDE_H

#include "../RSDefine.h"
#include "Runtime/RenderCore/include/VertexBuffer.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API PostProcessing
{
public:
    PostProcessing(RenderDevicePtr renderDevice);
    
    ~PostProcessing();
    
    void SetRenderTexture(const RCTexturePtr texture);
    
    void Process(const RenderEncoderPtr &renderEncoder);
    
private:
    VertexBufferPtr mPositionCoord = nullptr;   //顶点buffer
    VertexBufferPtr mTextureCoord = nullptr;  //纹理坐标buffer
    RenderDevicePtr mRenderDevice = nullptr;  //渲染设备
    TextureSamplerPtr mTextureSampler = nullptr; //纹理采样器
    GraphicsPipelinePtr mPipeline = nullptr;   //渲染管线
    
    RCTexturePtr mTexture = nullptr;
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_POST_PROCEESING_INCLUDE_H */
