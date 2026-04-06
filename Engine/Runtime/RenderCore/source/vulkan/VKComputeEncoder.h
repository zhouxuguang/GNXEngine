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
#include "VKRCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKComputePipeline;

class VKComputeEncoder : public ComputeEncoder
{
public:
    VKComputeEncoder(VulkanContextPtr context, VkCommandBuffer commandBuffer);
    
    ~VKComputeEncoder();
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline);
    
    virtual void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);
    
    // RCBuffer接口
    virtual void SetStorageBuffer(RCBufferPtr buffer, uint32_t index);
    
    virtual void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer);
    
    // SetTexture - 通过索引
    virtual void SetTexture(RCTexturePtr texture, uint32_t index);
    
    virtual void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index);
    
    // SetTexture - 通过资源名
    virtual void SetTexture(const std::string& resourceName, RCTexturePtr texture);
    
    virtual void SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel);
    
    // SetOutTexture - 通过索引
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t index);
    
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index);
    
    // SetOutTexture - 通过资源名
    virtual void SetOutTexture(const std::string& resourceName, RCTexturePtr texture);
    
    virtual void SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel);
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ);
    
    virtual void EndEncode();
    
private:
    VulkanContextPtr mContext = nullptr;
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    VKComputePipeline *mVKPipeline = nullptr;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_CK_COMPUTE_ENCODER_INCLUDE_JSDFGJDGHHJ */
