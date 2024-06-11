//
//  VKDepthStencilBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VKDepthStencilBuffer.h"
#include "VulkanDeviceUtil.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKDepthStencilBuffer::VKDepthStencilBuffer(VulkanContextPtr context, uint32_t width, uint32_t height)
{
    mContext = context;
    CreateDepthStencilBuffer(context, width, height);
}

VKDepthStencilBuffer::~VKDepthStencilBuffer()
{
    Release();
}

void VKDepthStencilBuffer::Release()
{
    if (mDepthStencilImage)
    {
        vkDestroyImage(mContext->device, mDepthStencilImage, nullptr);
    }
    
    if (mAllocation)
    {
        vmaFreeMemory(mContext->vmaAllocator, mAllocation);
        mAllocation = VK_NULL_HANDLE;
    }
    
    if (mDepthStencilImageView)
    {
        vkDestroyImageView(mContext->device, mDepthStencilImageView, nullptr);
    }
}

void VKDepthStencilBuffer::CreateDepthStencilBuffer(VulkanContextPtr context, uint32_t width, uint32_t height)
{
    Release();
    
    mFormat = VulkanDeviceUtil::FindDepthStencilFormat(*context);
    VulkanBufferUtil::CreateImage2DVMA(context->vmaAllocator, width, height, mFormat,
                                       context->numSamples, 1, VK_IMAGE_TILING_OPTIMAL, 
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                                       mDepthStencilImage, mAllocation);
    
    mDepthStencilImageView = VulkanBufferUtil::CreateImageView(mContext->device, mDepthStencilImage, mFormat, nullptr,
                                                             VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 1);


    VkCommandBuffer cmdBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, mContext->GetCommandPool());
    VulkanBufferUtil::TransitionImageLayout(mContext->device, cmdBuffer, mDepthStencilImage,
                                            mFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    VulkanBufferUtil::EndSingleTimeCommand(mContext->device, mContext->graphicsQueue, mContext->GetCommandPool(), cmdBuffer);
}

VkFormat VKDepthStencilBuffer::GetFormat() const
{
    return mFormat;
}

VkImage VKDepthStencilBuffer::GetImage() const
{
    return mDepthStencilImage;
}

VkImageView VKDepthStencilBuffer::GetImageView() const
{
    return mDepthStencilImageView;
}

static VkCompareOp TransformCompareFunc(CompareFunction compareFunc)
{
    VkCompareOp vkCompareFunc = VK_COMPARE_OP_MAX_ENUM;
    switch (compareFunc)
    {
        case CompareFunctionNever:
            vkCompareFunc = VK_COMPARE_OP_NEVER;
            break;
            
        case CompareFunctionLess:
            vkCompareFunc = VK_COMPARE_OP_LESS;
            break;
            
        case CompareFunctionEqual:
            vkCompareFunc = VK_COMPARE_OP_EQUAL;
            break;
            
        case CompareFunctionLessThanOrEqual:
            vkCompareFunc = VK_COMPARE_OP_LESS_OR_EQUAL;
            break;
            
        case CompareFunctionGreater:
            vkCompareFunc = VK_COMPARE_OP_GREATER;
            break;
            
        case CompareFunctionNotEqual:
            vkCompareFunc = VK_COMPARE_OP_NOT_EQUAL;
            break;
            
        case CompareFunctionGreaterThanOrEqual:
            vkCompareFunc = VK_COMPARE_OP_GREATER_OR_EQUAL;
            break;
            
        case CompareFunctionAlways:
            vkCompareFunc = VK_COMPARE_OP_ALWAYS;
            break;
            
        default:
            assert(false);
            break;
    }
    
    return vkCompareFunc;
}

static VkStencilOp TransformStencilOp(StencilOperation stencilOp)
{
    VkStencilOp vkStencilOp = VK_STENCIL_OP_MAX_ENUM;
    switch (stencilOp)
    {
        case StencilOperationKeep:
            vkStencilOp = VK_STENCIL_OP_KEEP;
            break;
            
        case StencilOperationZero:
            vkStencilOp = VK_STENCIL_OP_ZERO;
            break;
            
        case StencilOperationReplace:
            vkStencilOp = VK_STENCIL_OP_REPLACE;
            break;
            
        case StencilOperationIncrementClamp:
            vkStencilOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            break;
            
        case StencilOperationDecrementClamp:
            vkStencilOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            break;
            
        case StencilOperationInvert:
            vkStencilOp = VK_STENCIL_OP_INVERT;
            break;
            
        case StencilOperationIncrementWrap:
            vkStencilOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
            break;
            
        case StencilOperationDecrementWrap:
            vkStencilOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
            break;
            
        default:
            assert(false);
            break;
    }
    
    return vkStencilOp;
}

VKDepthStencilState::VKDepthStencilState(const DepthStencilDescriptor& des)
{
    mDepthStencilCreateInfo.depthTestEnable = (des.depthCompareFunction != CompareFunctionAlways);
    mDepthStencilCreateInfo.depthWriteEnable = des.depthWriteEnabled;
    mDepthStencilCreateInfo.depthCompareOp = TransformCompareFunc(des.depthCompareFunction);
    mDepthStencilCreateInfo.minDepthBounds = 0;
    mDepthStencilCreateInfo.maxDepthBounds = 1;
    
    mDepthStencilCreateInfo.front.compareMask = des.stencil.readMask;
    mDepthStencilCreateInfo.front.writeMask = des.stencil.writeMask;
    mDepthStencilCreateInfo.front.reference = 0;
    mDepthStencilCreateInfo.front.compareOp = TransformCompareFunc(des.stencil.stencilCompareFunction);
    mDepthStencilCreateInfo.front.depthFailOp = TransformStencilOp(des.stencil.depthFailureOperation);
    mDepthStencilCreateInfo.front.failOp = TransformStencilOp(des.stencil.stencilFailureOperation);
    mDepthStencilCreateInfo.front.passOp = TransformStencilOp(des.stencil.depthStencilPassOperation);
    mDepthStencilCreateInfo.back = mDepthStencilCreateInfo.front;
    mDepthStencilCreateInfo.stencilTestEnable = (VkBool32)des.stencil.stencilEnable;
    
    mDepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    mDepthStencilCreateInfo.pNext = nullptr;
    mDepthStencilCreateInfo.flags = 0;
    mDepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
}

VKDepthStencilState::~VKDepthStencilState()
{
}

NAMESPACE_RENDERCORE_END
