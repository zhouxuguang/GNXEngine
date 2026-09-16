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
    VKComputeEncoder(VulkanContextPtr context, VkCommandBuffer commandBuffer, uint32_t frameIndex = 0);
    
    ~VKComputeEncoder();
    
    void SetComputePipeline(ComputePipelinePtr computePipeline) override;
    
    void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;
    
    // RCBuffer接口
    void SetStorageBuffer(RCBufferPtr buffer, uint32_t index) override;
    
    void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer) override;
    
    // SetTexture - 通过索引
    void SetTexture(RCTexturePtr texture, uint32_t index) override;
    
    void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) override;
    
    // SetTexture - 通过资源名
    void SetTexture(const std::string& resourceName, RCTexturePtr texture) override;
    
    void SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) override;
    
    // SetOutTexture - 通过索引
    void SetOutTexture(RCTexturePtr texture, uint32_t index) override;
    
    void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) override;
    
    // SetOutTexture - 通过资源名
    void SetOutTexture(const std::string& resourceName, RCTexturePtr texture) override;
    
    void SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) override;
    
    void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) override;
    
    void EndEncode() override;
    
private:
    VulkanContextPtr mContext = nullptr;
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    VKComputePipeline *mVKPipeline = nullptr;
    uint32_t mFrameIndex = 0;   // 当前帧槽位索引（uniform buffer 按帧分环使用）
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_CK_COMPUTE_ENCODER_INCLUDE_JSDFGJDGHHJ */
