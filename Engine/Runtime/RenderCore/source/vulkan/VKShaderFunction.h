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
#include "spirv_reflection.h"

NAMESPACE_RENDERCORE_BEGIN

typedef enum DescriptorType
{
    DESCRIPTOR_TYPE_SAMPLER = 0,
    DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER = 4,
    DESCRIPTOR_TYPE_STORAGE_BUFFER = 5,
    DESCRIPTOR_TYPE_MAX = 6,
} DescriptorType;

// DescriptorSetLayout
struct DescriptorSetLayoutData
{
    uint32_t set_number;
    DescriptorType descriptorType;
    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};
using DescriptorSetLayoutDataVec = std::vector<DescriptorSetLayoutData>;

// 顶点布局
struct VertexInputLayout
{
    std::vector<VkVertexInputBindingDescription> inputBindings;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    
    // 按 location 覆盖属性格式（用于归一化等场景）
    void OverrideAttributeFormat(uint32_t location, VkFormat format, uint32_t stride)
    {
        for (auto& attr : attributeDescriptions)
        {
            if (attr.location == location)
            {
                attr.format = format;
                // 更新对应的 binding stride
                for (auto& binding : inputBindings)
                {
                    if (binding.binding == attr.binding)
                    {
                        binding.stride = stride;
                        break;
                    }
                }
                break;
            }
        }
    }
};

// 工作组线程的大小
struct WorkGroupSize
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct BindMetaData 
{
    uint32_t             set;
    uint32_t             binding;
    uint32_t             descriptorCount;
    VkDescriptorType     descriptorType;
    VkShaderStageFlags   shaderStageFlag;
};

class VKShaderFunction : public ShaderFunction, public std::enable_shared_from_this<VKShaderFunction>
{
public:
    VKShaderFunction(VulkanContextPtr context);
    ~VKShaderFunction();
    
    virtual ShaderFunctionPtr InitWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual std::shared_ptr<VKShaderFunction> initWithShaderSourceInner(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual ShaderStage GetShaderStage() const;
    
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
    
    VkShaderStageFlagBits GetVKShaderStage() const
    {
        return mVKShaderStage;
    }
    
private:
    VulkanContextPtr mContext = nullptr;
    VkShaderModule mShaderFunction = VK_NULL_HANDLE;
    ShaderStage mShaderStage;
    VkShaderStageFlagBits mVKShaderStage;
    
    std::string mEntryName;
    WorkGroupSize mWorkGroupSize;
    
    DescriptorSetLayoutDataVec mDescriptorSets;
    VertexInputLayout mVertexInputLayout;
};

using VKShaderFunctionPtr = std::shared_ptr<VKShaderFunction>;

struct ShaderBufferDesc 
{
    VkBuffer buffer;
	size_t offset;
	size_t range;
};

struct ShaderImageDesc 
{
	VkImageView   image;
    VkImageLayout imageLayout;
	VkSampler sampler;
};

class VKGraphicsShader : public GraphicsShader
{
public:
    using OneFrameDescriptorSets = std::vector<VkDescriptorSet>;   //每一帧的数据

public:
    VKGraphicsShader(VulkanContextPtr context, const ShaderCode& vertexShader, const ShaderCode& fragmentShader);

    // Mesh Shader 构造函数 (Task + Mesh + Fragment)
    VKGraphicsShader(VulkanContextPtr context, const ShaderCode& taskShader, const ShaderCode& meshShader, const ShaderCode& fragmentShader);

    ~VKGraphicsShader();

    virtual std::string GetName() const
    {
        return "";
    }

    VkShaderModule GetVertexShaderModule() const { return mVertexShader; }
    VkShaderModule GetFragmentShaderModule() const { return mFragShader; }

    VkShaderModule GetTaskShaderModule() const { return mTaskShaderModule; }
    VkShaderModule GetMeshShaderModule() const { return mMeshShaderModule; }

    const std::string& GetVertexEntryName() const { return mVertexEntryName; }
    const std::string& GetFragmentEntryName() const { return mFragmentEntryName; }

    const std::string& GetTaskEntryName() const { return mTaskEntryName; }
    const std::string& GetMeshEntryName() const { return mMeshEntryName; }

    bool IsMeshShader() const { return mMeshShaderModule != VK_NULL_HANDLE; }

    auto& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }
    auto& GetCurrentDescriptorSets() const { return mDescriptorSets[mCurrentFrame]; }

	void BindUniformBuffer(VkCommandBuffer commandBuffer,
        const std::string& resourceName, 
        const ShaderBufferDesc& bufferDesc,
        VkPipelineLayout layout);
	void BindTexture(VkCommandBuffer commandBuffer,
        const std::string& resourceName, 
        const ShaderImageDesc& imageDesc,
        VkPipelineLayout layout);
	void BindSampler(VkCommandBuffer commandBuffer,
        const std::string& resourceName, 
        VkSampler sampler,
        VkPipelineLayout layout);

	void SetCurrentFrameIndex(uint32_t currentFrameIndex)
	{
        mCurrentFrame = currentFrameIndex;
	}

	const VertexInputLayout& GetVertexInputLayout() const
	{
		return mVertexInputLayout;
	}

	/**
	 * @brief 按 location 覆盖顶点属性格式
	 * 
	 * 用于处理顶点缓冲区数据格式与 shader 声明格式不一致的情况
	 * 例如：shader 声明 float3，但顶点缓冲区存储 int8_t[3]（需要归一化）
	 * 
	 * @param location 顶点属性的 location
	 * @param format 实际的顶点数据格式（使用 VertexFormatChar3Norm 等归一化格式）
	 */
	void OverrideVertexAttributeFormat(uint32_t location, VertexFormat format);

    // 获取资源绑定索引
    uint32_t GetResourceBindIndex(const std::string& resourceName) const;

    // 获取资源反射数据
    const std::unordered_map<std::string, BindMetaData>& GetReflectionDatas() const
    {
        return mReflectionDatas;
    }

    // 获取指定描述符类型的 set 偏移
    uint32_t GetSetOffset(DescriptorType descriptorType) const;

private:
    void CollectResource(SpvReflectShaderModule shaderModule, ShaderStage shaderStage);

    void GenerateVulkanDescriptorSetLayout();

    void GenerateDescriptorSets();

    VulkanContextPtr mContext = nullptr;
    VkShaderModule mVertexShader = VK_NULL_HANDLE;
    VkShaderModule mFragShader = VK_NULL_HANDLE;
    VkShaderModule mTaskShaderModule = VK_NULL_HANDLE;
    VkShaderModule mMeshShaderModule = VK_NULL_HANDLE;
    uint32_t mCurrentFrame = 0;

	std::string mVertexEntryName;
    std::string mFragmentEntryName;
    std::string mTaskEntryName;
    std::string mMeshEntryName;

    std::unordered_map<std::string, BindMetaData> mReflectionDatas;

	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
	std::vector<OneFrameDescriptorSets> mDescriptorSets;

    // 顶点数据布局
    VertexInputLayout mVertexInputLayout;
};

using VKGraphicsShaderPtr = std::shared_ptr<VKGraphicsShader>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH */
