//
//  PostProcessing.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/21.
//

#ifndef GNX_ENGINE_POST_PROCEESING_INCLUDE_H
#define GNX_ENGINE_POST_PROCEESING_INCLUDE_H

#include "../RSDefine.h"
#include "RenderCore/VertexBuffer.h"
#include "RenderCore/RenderDevice.h"

NS_RENDERSYSTEM_BEGIN

class PostProcessing
{
public:
    PostProcessing(RenderDevicePtr renderDevice);
    
    ~PostProcessing();
    
    void SetRenderTexture(const RenderTexturePtr texture);
    
    void Process(const RenderEncoderPtr &renderEncoder);
    
private:
    VertexBufferPtr mPositionCoord = nullptr;   //顶点buffer
    VertexBufferPtr mTextureCoord = nullptr;  //纹理坐标buffer
    RenderDevicePtr mRenderDevice = nullptr;  //渲染设备
    TextureSamplerPtr mTextureSampler = nullptr; //纹理采样器
    GraphicsPipelinePtr mPipeline = nullptr;   //渲染管线
    
    RenderTexturePtr mTexture = nullptr;
    
    Texture2DPtr mTextures[2];
    Texture2DPtr mPingPangTexture = nullptr;
    
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_POST_PROCEESING_INCLUDE_H */
