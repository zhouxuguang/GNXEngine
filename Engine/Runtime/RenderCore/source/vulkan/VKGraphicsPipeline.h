//
//  VKGraphicsPipeline.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VKGRAPHICS_PIPELINE_INVSDGVJKGJS
#define GNX_ENGINE_VKGRAPHICS_PIPELINE_INVSDGVJKGJS

#include "VulkanContext.h"
#include "GraphicsPipeline.h"
#include "VKShaderFunction.h"
#include "VKTextureBase.h"

NAMESPACE_RENDERCORE_BEGIN

struct RenderPassFormat
{
    std::vector<VkFormat> colorFormats;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkFormat stencilFormat = VK_FORMAT_UNDEFINED;
};

struct RenderPassImage
{
    bool isPresentStage = false;
    std::vector<VkImage> colorImages;
    VkImage depthImage = VK_NULL_HANDLE;
    VkImage stencilImage = VK_NULL_HANDLE;
};

struct RenderPassTexture
{
    bool isPresentStage = false;
    std::vector<VKTextureBasePtr> colorTextures;
    VKTextureBasePtr depthTexture = nullptr;
    VKTextureBasePtr stencilTexture = nullptr;
};

struct RenderPassImageView
{
    std::vector<VkImageView> colorImages;
    VkImageView depthImage = VK_NULL_HANDLE;
    VkImageView stencilImage = VK_NULL_HANDLE;
};

class VKGraphicsPipeline : public GraphicsPipeline
{
public:
    VKGraphicsPipeline(VulkanContextPtr context, const GraphicsPipelineDesc& des);
    
    ~VKGraphicsPipeline()
    {
        if (mPipeline != VK_NULL_HANDLE)
        {
            SafeDestroyPipeline(*mContext, mPipeline);
            mPipeline = VK_NULL_HANDLE;
        }
        if (mPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(mContext->device, mPipelineLayout, nullptr);
            mPipelineLayout = VK_NULL_HANDLE;
        }
        for (auto& layout : mMeshDescriptorSetLayouts)
        {
            if (layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(mContext->device, layout, nullptr);
            }
        }
        mMeshDescriptorSetLayouts.clear();
    }
    
    virtual void AttachVertexShader(ShaderFunctionPtr shaderFunction);
    
    virtual void AttachFragmentShader(ShaderFunctionPtr shaderFunction);

    virtual void AttachGraphicsShader(GraphicsShaderPtr graphicsShader);
    
    virtual void AttachTaskShader(ShaderFunctionPtr shaderFunction) override;
    virtual void AttachMeshShader(ShaderFunctionPtr shaderFunction) override;
    
    VkPipeline GetPipeline() const
    {
        return mPipeline;
    }
    
    void Generate(const RenderPassFormat& passFormat);
    
    void ContructDes(const RenderPassFormat& passFormat);
    
    void CreatePipelineLayout();
    
    VkPipelineLayout GetPipelineLayout() const
    {
        return mPipelineLayout;
    }
    
    uint32_t GetDescriptorOffset(ShaderStage stage, DescriptorType type) const
    {
        return mStageSetOffsets[stage][type];
    }

    VkDescriptorSet GetDescriptorSet(uint32_t index) const
    {
        return mDescriptorSets[index];
    }

    /*uint32_t GetSetCount() const
    {
        return mDescriptorSets.size();
    }

    const std::vector<VkDescriptorSet>& GetDescriptorSets() const
    {
        return mDescriptorSets;
    }*/
    
    void SetRenderPass(VkRenderPass renderPass)
    {
        mPipeCreateInfo.renderPass = renderPass;
    }
    
    bool IsGenerated() const
    {
        return mGenerated;
    }
    
    void SetPrimitiveType(VkPrimitiveTopology primitiveTopology)
    {
        if (mGenerated)
        {
            return;
        }
        VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo = {};
        vertexAssemblyCreateInfo.topology = primitiveTopology;
        vertexAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        mPipeCreateInfo.pInputAssemblyState = &vertexAssemblyCreateInfo;
    }

    void SetCurrentFrameIndex(uint32_t currentFrameIndex)
    {
        mCurrentFrameIndex = currentFrameIndex;
        if (mShader)
        {
            mShader->SetCurrentFrameIndex(mCurrentFrameIndex);
        }
    }

    VKGraphicsShaderPtr GetCurrentShader() const;
    
    // 获取资源绑定索引
    uint32_t GetResourceBindIndex(const std::string& resourceName) const;
    
    // 获取指定描述符类型的 set 偏移
    uint32_t GetSetOffset(DescriptorType descriptorType) const;

    // Mesh Shader 的 threadgroup 大小（委托给 VKGraphicsShader，来自 SPIR-V 反射）
    const uint32_t* GetMeshThreadgroupSize() const override;
    const uint32_t* GetTaskThreadgroupSize() const override;
    
private:
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    VkGraphicsPipelineCreateInfo mPipeCreateInfo;
    uint32_t mCurrentFrameIndex = 0;
    VKGraphicsShaderPtr mShader = nullptr;
    
    GraphicsPipelineDesc mGraphicsPipelineDes;
    std::vector<ColorAttachmentDesc> mColorAttachmentDescs;
    
    bool mGenerated = false;
    
    std::vector<VKShaderFunctionPtr> mShaders;
    
    // Mesh shader 相关
    VKShaderFunctionPtr mTaskShader = nullptr;
    VKShaderFunctionPtr mMeshShader = nullptr;
    std::vector<VkDescriptorSetLayout> mMeshDescriptorSetLayouts;
    
    uint32_t mStageSetOffsets[ShaderStage_Max][DESCRIPTOR_TYPE_MAX];
    std::vector<VkDescriptorSet> mDescriptorSets;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VKGRAPHICS_PIPELINE_INVSDGVJKGJS */
