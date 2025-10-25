//
//  VulkanRenderPass.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/6/4.
//

#ifndef GNX_ENGINE_VULKAN_RENDERPASS_INCLUDE_NDGHGJJDF
#define GNX_ENGINE_VULKAN_RENDERPASS_INCLUDE_NDGHGJJDF

#include "VulkanContext.h"
#include "VKGraphicsPipeline.h"

NAMESPACE_RENDERCORE_BEGIN

class VulkanRenderPass
{
public:
    VulkanRenderPass(VulkanContextPtr context, const RenderPassFormat& passFormat);
    
    ~VulkanRenderPass();
    
    VkRenderPass GetRenderPass() const
    {
        return mRenderPass;
    }
    
private:
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    
    void CreateRenderPass(VulkanContextPtr context, const RenderPassFormat& passFormat);
};

using VulkanRenderPassPtr = std::shared_ptr<VulkanRenderPass>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VULKAN_RENDERPASS_INCLUDE_NDGHGJJDF */
