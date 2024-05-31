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

NAMESPACE_RENDERCORE_BEGIN

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
    
    void Generate();
    
    void ContructDes();
    
private:
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    VkGraphicsPipelineCreateInfo mPipeCreateInfo;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VKGRAPHICS_PIPELINE_INVSDGVJKGJS */
