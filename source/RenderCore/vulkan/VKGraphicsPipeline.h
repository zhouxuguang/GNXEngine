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

struct RenderPassImageView
{
    //bool isPresentStage = false;
    std::vector<VkImageView> colorImages;
    VkImageView depthImage = VK_NULL_HANDLE;
    VkImageView stencilImage = VK_NULL_HANDLE;
};

class VKGraphicsPipeline : public GraphicsPipeline
{
public:
    VKGraphicsPipeline(VulkanContextPtr context, const GraphicsPipelineDescriptor& des);
    
    ~VKGraphicsPipeline()
    {
        if (mPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(mContext->device, mPipeline, nullptr);
            mPipeline = VK_NULL_HANDLE;
        }
    }
    
    virtual void attachVertexShader(ShaderFunctionPtr shaderFunction);
    
    virtual void attachFragmentShader(ShaderFunctionPtr shaderFunction);
    
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
    
private:
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    VkGraphicsPipelineCreateInfo mPipeCreateInfo;
    
    GraphicsPipelineDescriptor mGraphicsPipelineDes;
    
    bool mGenerated = false;
    
    std::vector<VKShaderFunctionPtr> mShaders;
    
    uint32_t mStageSetOffsets[ShaderStage_Max][DESCRIPTOR_TYPE_MAX];
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VKGRAPHICS_PIPELINE_INVSDGVJKGJS */
