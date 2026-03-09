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
#include "RCBuffer.h"
#include "RCTexture.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

// 计算着色器的encoder
class ComputeEncoder
{
public:
    ComputeEncoder()
    {
    }
    
    virtual ~ComputeEncoder()
    {
    }
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline) = 0;
    
    virtual void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) = 0;
    
    /**
     * @brief 设置RCBuffer作为存储缓冲区（SSBO）
     * @param buffer RCBuffer指针
     * @param index 绑定索引
     */
    virtual void SetStorageBuffer(RCBufferPtr buffer, uint32_t index) = 0;
    
    virtual void SetTexture(RCTexturePtr texture, uint32_t index) = 0;
    
    virtual void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) = 0;
    
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t index) = 0;
    
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) = 0;
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) = 0;
    
    virtual void EndEncode() = 0;
};

using ComputeEncoderPtr = std::shared_ptr<ComputeEncoder>;


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_COMPUTE_ENCODER_HINFKSG */
