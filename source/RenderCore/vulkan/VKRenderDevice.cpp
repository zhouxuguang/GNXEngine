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
//    const char* env_p = std::getenv("PATH");
//    putenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS");
//    const char* name = getenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS");
//    setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1);
//    name = getenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS");
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        LOG_INFO("vulkan loader加载失败！\n");
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
        LOG_INFO("create instance success : api version : major = %u, minor = %u", major, minor);
        mVulkanContext->apiVersion = apiVersions[i];
        break;
    }
    
    if (mVulkanContext->instance == VK_NULL_HANDLE)
    {
        LOG_INFO("instance ERROR");
        return;
    }
    
    // 选择物理设备
    if (!SelectPhysicalDevice(*mVulkanContext))
    {
        LOG_INFO("SelectPhysicalDevice ERROR");
        return;
    }

    // 初始化vulkanfeature
	mVulkanContext->features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	mVulkanContext->features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    mVulkanContext->features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    mVulkanContext->features_11.pNext = &mVulkanContext->features_12;
    mVulkanContext->features2.pNext = &mVulkanContext->features_11;
	vkGetPhysicalDeviceFeatures2(mVulkanContext->physicalDevice, &mVulkanContext->features2);
    
    // 初始化扩展信息
    mVulkanContext->vulkanExtension.Init(mVulkanContext->physicalDevice, mVulkanContext->physicalDeviceProperties);
    
    // 创建虚拟设备
    if (!CreateVirtualDevice(*mVulkanContext))
    {
        LOG_INFO("CreateVirtualDevice ERROR");
        return;
    }
    
    CreateVMA(*mVulkanContext);
    
    mVulkanContext->GetCommandPool();
    mVulkanContext->upLoadPool.Start();
    
    // 创建交换链
    if (!CreateSurfaceKHR(*mVulkanContext, nativeWidow))
    {
        LOG_INFO("CreateSurfaceKHR ERROR");
        return;
    }
    
    CreateGraphicsDescriptorPool(*mVulkanContext);
    CreateComputeDescriptorPool(*mVulkanContext);
    
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
    //vkDeviceWaitIdle(mVulkanContext->device);
    
    if (!mSwapChain)
    {
        mSwapChain = std::make_shared<VulkanSwapChain>(mVulkanContext, width, height);
    }
    else
    {
        mSwapChain->Release();
        mSwapChain->CreateSwapChain(mVulkanContext, width, height);
    }
    
    // 创建命令缓冲区
    if (mCommandBuffers.empty())
    {
        CreateCommandBufers(mVulkanContext->device, mSwapChain->GetSwapChainImageCount(), mVulkanContext->GetCommandPool());
    }
    mCurrentFrame = 0;
    
    // 创建相关同步对象
    CreateSyncObject();
    
    return;
    
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
    VKShaderFunctionPtr shaderPtr = std::make_shared<VKShaderFunction>(mVulkanContext);
    shaderPtr = shaderPtr->initWithShaderSourceInner(shaderSource, shaderStage);
    
    return shaderPtr;
}

GraphicsShaderPtr VKRenderDevice::createGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const
{
    VKGraphicsShaderPtr shader = std::make_shared<VKGraphicsShader>(mVulkanContext, vertexShader, fragmentShader);
    return shader;
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
    mVulkanContext->upLoadPool.Update(mVulkanContext->device);
    if (mSwapChain == nullptr || mSwapChain->GetSwapChain() == VK_NULL_HANDLE)
    {
        return nullptr;
    }

    //VkResult res = vkResetFences(mVulkanContext->device, 1, &mFlightFences[mCurrentFrame]);

    VkResult res = vkWaitForFences(mVulkanContext->device, 1, &mFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
    //assert(res == VK_SUCCESS);  //这里返回值可能是VK_ERROR_DEVICE_LOST
    if (res == VK_ERROR_DEVICE_LOST && mVulkanContext->vulkanExtension.enableDeviceFault)
    {
        PFN_vkGetDeviceFaultInfoEXT vkGetDeviceFaultInfoEXT = (PFN_vkGetDeviceFaultInfoEXT)vkGetDeviceProcAddr(mVulkanContext->device, "vkGetDeviceFaultInfoEXT");
        VkDeviceFaultCountsEXT count = {};
        count.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;
        VkResult res = vkGetDeviceFaultInfoEXT(mVulkanContext->device, &count, nullptr);

        VkDeviceFaultInfoEXT info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
        res = vkGetDeviceFaultInfoEXT(mVulkanContext->device, &count, &info);

        LOG_INFO("error = %s", info.pVendorInfos->description);
    }
    VK_CHECK(vkResetFences(mVulkanContext->device, 1, &mFlightFences[mCurrentFrame]));

    res = vkAcquireNextImageKHR(mVulkanContext->device, mSwapChain->GetSwapChain(), UINT64_MAX,
            mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &mNextFrameIndex);

	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
	{
		resize(mSwapChain->GetWidth(), mSwapChain->GetHeight());
        //res = vkResetFences(mVulkanContext->device, 1, &mFlightFences[mCurrentFrame]);
        return nullptr;
	}

	if (res == VK_TIMEOUT)
    {
        return nullptr;
	}

    VkCommandBuffer commandBuffer = mCommandBuffers[mCurrentFrame];
    //res = vkResetCommandBuffer(commandBuffer, 0);
    CommandBufferInfoPtr commandBufferInfo = std::make_shared<CommandBufferInfo>();
    commandBufferInfo->flightFence = mFlightFences[mCurrentFrame];
    commandBufferInfo->imageAvailableSemaphore = mImageAvailableSemaphores[mCurrentFrame];
    commandBufferInfo->currentFrameIndex = mCurrentFrame;
    commandBufferInfo->nextFrameIndex = mNextFrameIndex;
    commandBufferInfo->renderDevice = this;
    commandBufferInfo->renderFinishSemaphore = mRenderFinishedSemaphores[mCurrentFrame];
    commandBufferInfo->swapChain = mSwapChain;
    commandBufferInfo->vulkanContext = mVulkanContext;
    commandBufferInfo->depthStencilBuffer = mSwapChain->GetDSBuffer();
    
    return std::make_shared<VulkanCommandBuffer>(commandBuffer, commandBufferInfo);
}

void VKRenderDevice::CreateSyncObject()
{
    assert(mSwapChain->GetSwapChainImageCount());
    
    //在渲染线程，等待渲染命令提交完毕后交换缓冲区图像
    VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    // 在渲染线程使用，需要等待可用的帧缓冲准备好
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
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
        vkFreeCommandBuffers(mVulkanContext->device, mVulkanContext->GetCommandPool(), (uint32_t)mCommandBuffers.size(), mCommandBuffers.data());
    }
    mCommandBuffers.clear();
    mCommandBuffers.shrink_to_fit();
}

NAMESPACE_RENDERCORE_END
