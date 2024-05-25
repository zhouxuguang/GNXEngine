//
//  ComputeEncoder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef GNX_ENGINE_COMPUTE_ENCODER_HINFKSG
#define GNX_ENGINE_COMPUTE_ENCODER_HINFKSG

#include "RenderDefine.h"
#include "GraphicsPipeline.h"
#include "ComputeBuffer.h"
#include "Texture2D.h"
#include "RenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

class ComputeEncoder
{
public:
    ComputeEncoder()
    {
    }
    
    virtual ~ComputeEncoder()
    {
    }
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline) {}
    
    virtual void SetBuffer(ComputeBufferPtr buffer, uint32_t index){}
    
    virtual void SetTexture(Texture2DPtr texture, uint32_t index){}
    
    virtual void SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index){}
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ){}
    
    virtual void EndEncode(){}
};

typedef std::shared_ptr<ComputeEncoder> ComputeEncoderPtr;


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_COMPUTE_ENCODER_HINFKSG */
