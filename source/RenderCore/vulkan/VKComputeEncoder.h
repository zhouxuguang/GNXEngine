//
//  VKComputeEncoder.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#ifndef GNX_ENGINE_CK_COMPUTE_ENCODER_INCLUDE_JSDFGJDGHHJ
#define GNX_ENGINE_CK_COMPUTE_ENCODER_INCLUDE_JSDFGJDGHHJ

#include "VulkanContext.h"
#include "ComputeEncoder.h"

NAMESPACE_RENDERCORE_BEGIN

class VKComputeEncoder : public ComputeEncoder
{
public:
    VKComputeEncoder();
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline) {}
    
    virtual void SetBuffer(ComputeBufferPtr buffer, uint32_t index){}
    
    virtual void SetTexture(Texture2DPtr texture, uint32_t index){}
    
    virtual void SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index){}
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ){}
    
    virtual void EndEncode(){}
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_CK_COMPUTE_ENCODER_INCLUDE_JSDFGJDGHHJ */
