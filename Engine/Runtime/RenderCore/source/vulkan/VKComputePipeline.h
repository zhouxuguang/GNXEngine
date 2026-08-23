//
//  VKComputePipeline.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#ifndef GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH
#define GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH

#include "VulkanContext.h"
#include "GraphicsPipeline.h"
#include "VKShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

class VKComputePipeline : public ComputePipeline
{
public:
    VKComputePipeline(VulkanContextPtr context, const ShaderCode& shaderSource);
    
    ~VKComputePipeline();
    
    //获得计算着色器的线程组的大小
    void GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z) override;
    
    VkPipeline GetPipeline() const
    {
        return mPipeline;
    }
    
    const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const
    {
        return mDescriptorSetLayouts;
    }
    
    VkPipelineLayout GetPipelineLayout() const
    {
        return mPipelineLayout;
    }
    
    uint32_t GetSetOffset(DescriptorType descriptorType) const
    {
        return mStageSetOffsets[descriptorType];
    }
    
    void CollectResource(SpvReflectShaderModule shaderModule);
    
    /**
     * @brief 通过资源名获取绑定索引
     * @param resourceName Shader中的资源名
     * @return 绑定索引，如果找不到返回-1
     */
    uint32_t GetResourceBindIndex(const std::string& resourceName) const;
    
    // Push constant 查询接口
    const PushConstantMeta* GetPushConstantByName(const std::string& resourceName) const;
    const PushConstantMeta* GetPushConstantByBinding(uint32_t set, uint32_t binding) const;
    const std::vector<PushConstantMeta>& GetPushConstants() const { return mPushConstants; }
    
private:
    VulkanContextPtr mContext = nullptr;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
    WorkGroupSize mWorkGroupSize;
    
    uint32_t mStageSetOffsets[DESCRIPTOR_TYPE_MAX];   //每一种资源所在的set的索引
    std::unordered_map<std::string, BindMetaData> mReflectionDatas;
    
    // Push constant 数据
    std::vector<PushConstantMeta> mPushConstants;
    // {set, binding} → PushConstantMeta 的反向索引（用于按 index 绑定）
    PushConstantBindingMap mPushConstantBindings;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH */
