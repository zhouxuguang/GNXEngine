//
//  VKRenderDevice.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VKRenderDevice.h"
#include "BaseLib/LogService.h"
#include "VKTextureSampler.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VKComputeBuffer.h"
#include "VKComputePipeline.h"
#include "VKTexture2D.h"
#include "VKTextureCube.h"
#include "VKRenderTexture.h"
#include "VKUniformBuffer.h"
#include "VKComputePipeline.h"
#include "VKGraphicsPipeline.h"
#include "VKShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

USING_NS_BASELIB

VKRenderDevice::VKRenderDevice(ViewHandle nativeWidow)
{
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        printf("vulkan loader加载失败！\n");
        return;
    }
    mVulkanContext = std::make_shared<VulkanContext>();
    
    uint32_t apiVersions[] = {VK_API_VERSION_1_3, VK_API_VERSION_1_2, VK_API_VERSION_1_1, VK_API_VERSION_1_0};
    
    for (int i = 0; i < sizeof(apiVersions) / sizeof(apiVersions[0]); i ++)
    {
        if (!CreateInstance(*mVulkanContext, apiVersions[i]))
        {
            continue;
        }
        
        uint32_t major = VK_API_VERSION_MAJOR(apiVersions[i]);
        uint32_t minor = VK_API_VERSION_MINOR(apiVersions[i]);
        printf("create instance success : api version : major = %u, minor = %u", major, minor);
        mVulkanContext->apiVersion = apiVersions[i];
        break;
    }
    
    if (mVulkanContext->instance == VK_NULL_HANDLE)
    {
        log_info("instance ERROR");
        return;
    }
    
    if (!SelectPhysicalDevice(*mVulkanContext))
    {
        log_info("SelectPhysicalDevice ERROR");
        return;
    }
    
    // 创建虚拟设备
    if (!CreateVirtualDevice(*mVulkanContext))
    {
        log_info("CreateVirtualDevice ERROR");
        return;
    }
    
    CreateVMA(*mVulkanContext);
    
    // 创建交换链
    if (!CreateSurfaceKHR(*mVulkanContext, nativeWidow))
    {
        log_info("CreateSurfaceKHR ERROR");
        return;
    }
    
    // 测试以下函数指针是否为空
    void * p = (void*)vkCmdBeginRenderingKHR;
    void *p2 = (void*)vkCmdPushDescriptorSetKHR;
    void* p3 = (void*)vkCmdTraceRaysKHR;
    printf("");
}

VKRenderDevice::~VKRenderDevice()
{
}

void VKRenderDevice::resize(uint32_t width, uint32_t height)
{
    vkQueueWaitIdle(mVulkanContext->graphicsQueue);
    
    if (!mSwapChain)
    {
        mSwapChain = std::make_shared<VulkanSwapChain>(mVulkanContext, width, height);
    }
    else
    {
        mSwapChain->CreateSwapChain(mVulkanContext, width, height);
    }
    
    // 创建深度模板缓冲
    if (!mDSBuffer)
    {
        mDSBuffer = std::make_shared<VKDepthStencilBuffer>(mVulkanContext, mSwapChain->GetWidth(), mSwapChain->GetHeight());
    }
    else
    {
        mDSBuffer->CreateDepthStencilBuffer(mVulkanContext, width, height);
    }
    
    // 创建命令缓冲区
    if (mCommandBuffers.empty())
    {
        CreateCommandBufers(mVulkanContext->device, mSwapChain->GetSwapChainImageCount(), mVulkanContext->commandPool);
    }
    mCurrentFrame = 0;
    
    // 创建相关同步对象
    CreateSyncObject();
    
    // for test
    uint8_t data[4] = {4, 1, 2, 3};
    createVertexBufferWithBytes(data, 4, StorageModeShared);
    
    TextureDescriptor des;
    des.width = 1;
    des.height = 1;
    des.bytesPerRow = 4;
    Texture2DPtr texture = createTextureWithDescriptor(des);
    texture->setTextureData(data);
    
    std::vector<TextureDescriptor> desArray;
    desArray.push_back(des);
    TextureCubePtr textureCube = createTextureCubeWithDescriptor(desArray);
    textureCube->setTextureData(kCubeFacePX, 4, data);
    textureCube->setTextureData(kCubeFaceNX, 4, data);
    textureCube->setTextureData(kCubeFacePY, 4, data);
    textureCube->setTextureData(kCubeFaceNY, 4, data);
    textureCube->setTextureData(kCubeFacePZ, 4, data);
    textureCube->setTextureData(kCubeFaceNZ, 4, data);
    
    IndexBufferPtr indexBuffer = createIndexBufferWithBytes(data, 4, IndexType_UInt);
    
    UniformBufferPtr uniformBuffer = createUniformBufferWithSize(4);
    uniformBuffer->setData(data, 0, 4);
    
    ComputeBufferPtr computeBuffer = createComputeBuffer(data, 4, StorageModePrivate);
    
    CommandBufferPtr commandBuffer = createCommandBuffer();
    RenderEncoderPtr renderEncoder = commandBuffer->createDefaultRenderEncoder();
    renderEncoder->EndEncode();
    commandBuffer->presentFrameBuffer();
}

VertexBufferPtr VKRenderDevice::createVertexBufferWithLength(uint32_t size) const
{
    return std::make_shared<VKVertexBuffer>(mVulkanContext, size, StorageModePrivate);
}

VertexBufferPtr VKRenderDevice::createVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const
{
    return std::make_shared<VKVertexBuffer>(mVulkanContext, buffer, size, mode);
}

ComputeBufferPtr VKRenderDevice::createComputeBuffer(uint32_t size) const
{
    return std::make_shared<VKComputeBuffer>(mVulkanContext, size);
}

ComputeBufferPtr VKRenderDevice::createComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const
{
    return std::make_shared<VKComputeBuffer>(mVulkanContext, buffer, size, mode);
}

IndexBufferPtr VKRenderDevice::createIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const
{
    return std::make_shared<VKIndexBuffer>(mVulkanContext, indexType, buffer, size);
}

Texture2DPtr VKRenderDevice::createTextureWithDescriptor(const TextureDescriptor& des) const
{
    return std::make_shared<VKTexture2D>(mVulkanContext, des);
}

TextureCubePtr VKRenderDevice::createTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const
{
    return std::make_shared<VKTextureCube>(mVulkanContext, desArray);
}

TextureSamplerPtr VKRenderDevice::createSamplerWithDescriptor(const SamplerDescriptor& des) const
{
    return std::make_shared<VKTextureSampler>(mVulkanContext, des);
}

UniformBufferPtr VKRenderDevice::createUniformBufferWithSize(uint32_t bufSize) const
{
    return std::make_shared<VKUniformBuffer>(mVulkanContext, bufSize);
}

ShaderFunctionPtr VKRenderDevice::createShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const
{
    return nullptr;
    //return std::make_shared<VKShaderFunction>(mVulkanContext, shaderSource, shaderStage);
}

GraphicsPipelinePtr VKRenderDevice::createGraphicsPipeline(const GraphicsPipelineDescriptor& des) const
{
    return std::make_shared<VKGraphicsPipeline>(mVulkanContext, des);
}

ComputePipelinePtr VKRenderDevice::createComputePipeline(const ShaderCode& shaderSource) const
{
    return std::make_shared<VKComputePipeline>(mVulkanContext, shaderSource);
}

RenderTexturePtr VKRenderDevice::createRenderTexture(const TextureDescriptor& des) const
{
    return std::make_shared<VKRenderTexture>(mVulkanContext, des);
}

CommandBufferPtr VKRenderDevice::createCommandBuffer()
{
    if (mSwapChain == nullptr || mSwapChain->GetSwapChain() == VK_NULL_HANDLE)
    {
        return nullptr;
    }

    VkResult res = vkWaitForFences(mVulkanContext->device, 1, &mFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
    assert(res == VK_SUCCESS);  //这里返回值可能是VK_ERROR_DEVICE_LOST

    res = vkAcquireNextImageKHR(mVulkanContext->device, mSwapChain->GetSwapChain(), UINT64_MAX,
            mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &mNextFrameIndex);

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    {
        log_info("VKRenderDevice::createCommandBuffer vkAcquireNextImageKHR = %d", res);

        if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            resize(mSwapChain->GetWidth(), mSwapChain->GetHeight());
        }
        return nullptr;
    }

    VkCommandBuffer commandBuffer = mCommandBuffers[mNextFrameIndex];
    CommandBufferInfoPtr commandBufferInfo = std::make_shared<CommandBufferInfo>();
    commandBufferInfo->flightFence = mFlightFences[mCurrentFrame];
    commandBufferInfo->imageAvailableSemaphore = mImageAvailableSemaphores[mCurrentFrame];
    commandBufferInfo->nextFrameIndex = mNextFrameIndex;
    commandBufferInfo->renderDevice = this;
    commandBufferInfo->renderFinishSemaphore = mRenderFinishedSemaphores[mCurrentFrame];
    commandBufferInfo->swapChain = mSwapChain;
    commandBufferInfo->vulkanContext = mVulkanContext;
    
    return std::make_shared<VulkanCommandBuffer>(commandBuffer, commandBufferInfo);
}

void VKRenderDevice::CreateSyncObject()
{
    assert(mSwapChain->GetSwapChainImageCount());
    
    //在渲染线程，等待渲染命令提交完毕后交换缓冲区图像
    VkFenceCreateInfo fenceCreateInfo 
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    
    // 在渲染线程使用，需要等待可用的帧缓冲准备好
    VkSemaphoreCreateInfo semaphoreCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    
    size_t imageCount = mSwapChain->GetSwapChainImageCount();

    mImageAvailableSemaphores.resize(imageCount);
    mRenderFinishedSemaphores.resize(imageCount);
    mFlightFences.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++)
    {
        vkCreateSemaphore(mVulkanContext->device, &semaphoreCreateInfo, nullptr, &mImageAvailableSemaphores[i]);
        vkCreateSemaphore(mVulkanContext->device, &semaphoreCreateInfo, nullptr, &mRenderFinishedSemaphores[i]);
        vkCreateFence(mVulkanContext->device, &fenceCreateInfo, nullptr, &mFlightFences[i]);
    }
}

void VKRenderDevice::DestroySyncObject()
{
    assert(mSwapChain->GetSwapChainImageCount());
    size_t imageCount = mSwapChain->GetSwapChainImageCount();
    
    for (size_t i = 0; i < imageCount; i++)
    {
        vkDestroySemaphore(mVulkanContext->device, mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mVulkanContext->device, mRenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(mVulkanContext->device, mFlightFences[i], nullptr);
    }
    mImageAvailableSemaphores.clear();
    mRenderFinishedSemaphores.clear();
    mFlightFences.clear();
}

void VKRenderDevice::CreateCommandBufers(VkDevice device, size_t nImageCount, VkCommandPool commandPool)
{
    mCommandBuffers.resize(nImageCount);
    VkCommandBufferAllocateInfo cmdBufferCreateInfo;
    cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferCreateInfo.pNext = nullptr;
    cmdBufferCreateInfo.commandPool = commandPool;
    cmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferCreateInfo.commandBufferCount = (uint32_t)nImageCount;
    vkAllocateCommandBuffers(device, &cmdBufferCreateInfo, mCommandBuffers.data());
}

void VKRenderDevice::ReleaseCommandBuffers()
{
    if (!mCommandBuffers.empty())
    {
        vkFreeCommandBuffers(mVulkanContext->device, mVulkanContext->commandPool, (uint32_t)mCommandBuffers.size(), mCommandBuffers.data());
    }
    mCommandBuffers.clear();
    mCommandBuffers.shrink_to_fit();
}

NAMESPACE_RENDERCORE_END
