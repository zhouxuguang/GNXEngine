//
//  VKShaderFunction.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH
#define GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH

#include "VulkanContext.h"
#include "ShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

// DescriptorSetLayout
struct DescriptorSetLayoutData
{
    uint32_t set_number;
    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};
using DescriptorSetLayoutDataVec = std::vector<DescriptorSetLayoutData>;

// 顶点布局
struct VertexInputLayout
{
    std::vector<VkVertexInputBindingDescription> inputBindings;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};

// 工作组线程的大小
struct WorkGroupSize
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

class VKShaderFunction : public ShaderFunction, public std::enable_shared_from_this<VKShaderFunction>
{
public:
    VKShaderFunction(VulkanContextPtr context);
    ~VKShaderFunction();
    
    virtual ShaderFunctionPtr initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual std::shared_ptr<VKShaderFunction> initWithShaderSourceInner(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual ShaderStage getShaderStage() const;
    
    VkShaderModule GetShaderModule() const
    {
        return mShaderFunction;
    }
    
    const DescriptorSetLayoutDataVec& GetDescriptorSets() const
    {
        return mDescriptorSets;
    }
    
    const std::string& GetEntryName() const
    {
        return mEntryName;
    }
    
    const WorkGroupSize& GetWorkGroupSize() const
    {
        return mWorkGroupSize;
    }
    
    const VertexInputLayout& GetVertexInputLayout() const
    {
        return mVertexInputLayout;
    }
    
private:
    VulkanContextPtr mContext = nullptr;
    VkShaderModule mShaderFunction = VK_NULL_HANDLE;
    ShaderStage mShaderStage;
    
    std::string mEntryName;
    WorkGroupSize mWorkGroupSize;
    
    DescriptorSetLayoutDataVec mDescriptorSets;
    VertexInputLayout mVertexInputLayout;
};

using VKShaderFunctionPtr = std::shared_ptr<VKShaderFunction>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH */
