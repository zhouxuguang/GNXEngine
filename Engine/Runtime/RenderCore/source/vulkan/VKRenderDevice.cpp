//
//  VKRenderDevice.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VKRenderDevice.h"
#include "VulkanGarbageCollector.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "VKTextureSampler.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VKComputeBuffer.h"
#include "VKComputePipeline.h"
#include "VKUniformBuffer.h"
#include "VKGraphicsPipeline.h"
#include "VKShaderFunction.h"
#include "VKTextureBase.h"
#include "VulkanBufferUtil.h"
#include "VKCommandQueue.h"
#include "VKRCBuffer.h"

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
    
    // 创建垃圾收集器
    CreateGarbageCollector(*mVulkanContext);
    
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

    // 初始化队列管理
    InitializeCommandQueues();

    // 测试以下函数指针是否为空
    void * p = (void*)vkCmdBeginRenderingKHR;
    void *p2 = (void*)vkCmdPushDescriptorSetKHR;
    void* p3 = (void*)vkCmdTraceRaysKHR;
}

VKRenderDevice::~VKRenderDevice()
{
    if (!mVulkanContext)
    {
        return;
    }
    
    // 等待设备空闲
    vkDeviceWaitIdle(mVulkanContext->device);
    
    // 销毁异步计算信号量
    if (mVulkanContext->asyncComputeSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(mVulkanContext->device, mVulkanContext->asyncComputeSemaphore, nullptr);
        mVulkanContext->asyncComputeSemaphore = VK_NULL_HANDLE;
    }
    
    // 销毁描述符池
    if (mVulkanContext->graphicsDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(mVulkanContext->device, mVulkanContext->graphicsDescriptorPool, nullptr);
        mVulkanContext->graphicsDescriptorPool = VK_NULL_HANDLE;
    }
    
    if (mVulkanContext->computeDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(mVulkanContext->device, mVulkanContext->computeDescriptorPool, nullptr);
        mVulkanContext->computeDescriptorPool = VK_NULL_HANDLE;
    }
    
    // 销毁 VMA 分配器
    if (mVulkanContext->vmaAllocator)
    {
        vmaDestroyAllocator(mVulkanContext->vmaAllocator);
        mVulkanContext->vmaAllocator = nullptr;
    }
    
    // 销毁设备
    if (mVulkanContext->device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(mVulkanContext->device, nullptr);
        mVulkanContext->device = VK_NULL_HANDLE;
    }
    
    // 销毁实例
    if (mVulkanContext->instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(mVulkanContext->instance, nullptr);
        mVulkanContext->instance = VK_NULL_HANDLE;
    }
}

void VKRenderDevice::Resize(uint32_t width, uint32_t height)
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
    if (mComputeCommandBuffers.empty())
    {
        CreateComputeCommandBuffers(mVulkanContext->device, mSwapChain->GetSwapChainImageCount(), mVulkanContext->GetComputeCommandPool());
    }
    mCurrentFrame = 0;
    
    // 创建相关同步对象
    CreateSyncObject();
}

VertexBufferPtr VKRenderDevice::CreateVertexBufferWithLength(uint32_t size) const
{
    return std::make_shared<VKVertexBuffer>(mVulkanContext, size, StorageModePrivate);
}

VertexBufferPtr VKRenderDevice::CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const
{
    return std::make_shared<VKVertexBuffer>(mVulkanContext, buffer, size, mode);
}

ComputeBufferPtr VKRenderDevice::CreateComputeBuffer(uint32_t size, StorageMode mode) const
{
    return std::make_shared<VKComputeBuffer>(mVulkanContext, size, mode);
}

ComputeBufferPtr VKRenderDevice::CreateComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const
{
    return std::make_shared<VKComputeBuffer>(mVulkanContext, buffer, size, mode);
}

IndexBufferPtr VKRenderDevice::CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const
{
    return std::make_shared<VKIndexBuffer>(mVulkanContext, indexType, buffer, size);
}

RCBufferPtr VKRenderDevice::CreateBuffer(const RCBufferDesc& desc) const
{
    return std::make_shared<VKRCBuffer>(mVulkanContext, desc);
}

RCBufferPtr VKRenderDevice::CreateBuffer(const RCBufferDesc& desc, const void* data) const
{
    return std::make_shared<VKRCBuffer>(mVulkanContext, desc, data);
}

TextureSamplerPtr VKRenderDevice::CreateSamplerWithDescriptor(const SamplerDesc& des) const
{
    return std::make_shared<VKTextureSampler>(mVulkanContext, des);
}

UniformBufferPtr VKRenderDevice::CreateUniformBufferWithSize(uint32_t bufSize) const
{
    return std::make_shared<VKUniformBuffer>(mVulkanContext, bufSize);
}

ShaderFunctionPtr VKRenderDevice::CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const
{
    VKShaderFunctionPtr shaderPtr = std::make_shared<VKShaderFunction>(mVulkanContext);
    shaderPtr = shaderPtr->initWithShaderSourceInner(shaderSource, shaderStage);
    
    return shaderPtr;
}

GraphicsShaderPtr VKRenderDevice::CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const
{
    VKGraphicsShaderPtr shader = std::make_shared<VKGraphicsShader>(mVulkanContext, vertexShader, fragmentShader);
    return shader;
}

GraphicsPipelinePtr VKRenderDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& des) const
{
    return std::make_shared<VKGraphicsPipeline>(mVulkanContext, des);
}

ComputePipelinePtr VKRenderDevice::CreateComputePipeline(const ShaderCode& shaderSource) const
{
    return std::make_shared<VKComputePipeline>(mVulkanContext, shaderSource);
}

RCTexture2DPtr VKRenderDevice::CreateTexture2D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const
{
    if (0 == width || 0 == height || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    VkFormat vkformat = VulkanBufferUtil::ConvertTextureFormat(format);
    
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = levels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = vkformat;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VulkanBufferUtil::ConvertTextureUsage(usage, vkformat);
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags = 0;
    
    auto texture = std::make_shared<VKRCTexture2D>(mVulkanContext, imageCreateInfo);
    texture->SetFormat(format);
    texture->CreateImageViews(imageCreateInfo);
    return texture;
}

RCTexture3DPtr VKRenderDevice::CreateTexture3D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t depth,
                                    uint32_t levels) const
{
    if (0 == width || 0 == height || 0 == depth || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    VkFormat vkformat = VulkanBufferUtil::ConvertTextureFormat(format);
    
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = depth;
    imageCreateInfo.mipLevels = levels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = vkformat;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VulkanBufferUtil::ConvertTextureUsage(usage, vkformat);
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;    // 3d纹理的每一层需要创建imageview时需要这个标志
    
    auto texture = std::make_shared<VKRCTexture3D>(mVulkanContext, imageCreateInfo);
    texture->SetFormat(format);
	texture->CreateImageViews(imageCreateInfo);
	return texture;
}

RCTextureCubePtr VKRenderDevice::CreateTextureCube(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const
{
    if (0 == width || 0 == height || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    // 立方体纹理宽高需要一致
    if (width != height)
    {
        assert(false);
        return nullptr;
    }
    
    VkFormat vkformat = VulkanBufferUtil::ConvertTextureFormat(format);
    
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = levels;
    // Cube faces count as array layers in Vulkan
    imageCreateInfo.arrayLayers = 6;
    imageCreateInfo.format = vkformat;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VulkanBufferUtil::ConvertTextureUsage(usage, vkformat);
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // This flag is required for cube map images
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    auto texture = std::make_shared<VKRCTextureCube>(mVulkanContext, imageCreateInfo);
    texture->SetFormat(format);
	texture->CreateImageViews(imageCreateInfo);
	return texture;
}

RCTexture2DArrayPtr VKRenderDevice::CreateTexture2DArray(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels,
                                    uint32_t arraySize) const
{
    if (0 == width || 0 == height || 0 == levels)
    {
        assert(false);
        return nullptr;
    }
    
    VkFormat vkformat = VulkanBufferUtil::ConvertTextureFormat(format);
    
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = levels;
    imageCreateInfo.arrayLayers = arraySize;
    imageCreateInfo.format = vkformat;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VulkanBufferUtil::ConvertTextureUsage(usage, vkformat);
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags = 0;
    
    auto texture = std::make_shared<VKRCTexture2DArray>(mVulkanContext, imageCreateInfo);
	texture->CreateImageViews(imageCreateInfo);
    texture->SetFormat(format);
	return texture;
}

CommandBufferPtr VKRenderDevice::CreateCommandBuffer()
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
		Resize(mSwapChain->GetWidth(), mSwapChain->GetHeight());
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


void VKRenderDevice::CreateComputeCommandBuffers(VkDevice device, size_t nImageCount, VkCommandPool commandPool)
{
    mComputeCommandBuffers.resize(nImageCount);
    VkCommandBufferAllocateInfo cmdBufferCreateInfo;
    cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferCreateInfo.pNext = nullptr;
    cmdBufferCreateInfo.commandPool = commandPool;
    cmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferCreateInfo.commandBufferCount = (uint32_t)nImageCount;
    vkAllocateCommandBuffers(device, &cmdBufferCreateInfo, mComputeCommandBuffers.data());
}

void VKRenderDevice::InitializeCommandQueues()
{
    // 创建图形队列（Vulkan通常只有一个主图形队列）
    if (mVulkanContext->graphicsQueue != VK_NULL_HANDLE)
    {
        VKCommandQueuePtr queue = std::make_shared<VKCommandQueue>(
            this,
            mVulkanContext->graphicsQueue,
            QueueType::Graphics,
            QueuePriority::Normal,
            0,
            mVulkanContext->graphicsQueueFamilyIndex
        );
        mGraphicsQueues.push_back(queue);
        LOG_INFO("Created Graphics Queue: %s", queue->GetDescription().c_str());
    }

    // 创建计算队列
    for (uint32_t i = 0; i < mVulkanContext->availableComputeQueues.size(); ++i)
    {
        VKCommandQueuePtr queue = std::make_shared<VKCommandQueue>(
            this,
            mVulkanContext->availableComputeQueues[i],
            QueueType::Compute,
            QueuePriority::Normal,
            i,
            mVulkanContext->computeQueueFamilyIndex
        );
        mComputeQueues.push_back(queue);
        LOG_INFO("Created Compute Queue: %s", queue->GetDescription().c_str());
    }

    // 创建传输队列
    for (uint32_t i = 0; i < mVulkanContext->availableTransferQueues.size(); ++i)
    {
        VKCommandQueuePtr queue = std::make_shared<VKCommandQueue>(
            this,
            mVulkanContext->availableTransferQueues[i],
            QueueType::Transfer,
            QueuePriority::Normal,
            i,
            mVulkanContext->transferQueueFamilyIndex
        );
        mTransferQueues.push_back(queue);
        LOG_INFO("Created Transfer Queue: %s", queue->GetDescription().c_str());
    }
}

CommandQueuePtr VKRenderDevice::GetCommandQueue(QueueType type, uint32_t index) const
{
    switch (type)
    {
        case QueueType::Graphics:
            if (index < mGraphicsQueues.size())
            {
                return mGraphicsQueues[index];
            }
            break;
            
        case QueueType::Compute:
            if (index < mComputeQueues.size())
            {
                return mComputeQueues[index];
            }
            break;
            
        case QueueType::Transfer:
            if (index < mTransferQueues.size())
            {
                return mTransferQueues[index];
            }
            break;
            
        default:
            LOG_INFO("GetQueue: Invalid queue type");
            return nullptr;
    }

    LOG_INFO("GetCommandQueue: Queue index %u out of range for type %s",
             index, GetQueueTypeName(type));
    return nullptr;
}

uint32_t VKRenderDevice::GetCommandQueueCount(QueueType type) const
{
    switch (type)
    {
        case QueueType::Graphics:
            return (uint32_t)mGraphicsQueues.size();
        case QueueType::Compute:
            return (uint32_t)mComputeQueues.size();
        case QueueType::Transfer:
            return (uint32_t)mTransferQueues.size();
        default:
            return 0;
    }
}

void VKRenderDevice::ReleaseCommandBuffers()
{
    if (!mCommandBuffers.empty())
    {
        vkFreeCommandBuffers(mVulkanContext->device, mVulkanContext->GetCommandPool(), (uint32_t)mCommandBuffers.size(), mCommandBuffers.data());
    }
    mCommandBuffers.clear();
    mCommandBuffers.shrink_to_fit();
    
    if (!mComputeCommandBuffers.empty())
    {
        vkFreeCommandBuffers(mVulkanContext->device, mVulkanContext->GetComputeCommandPool(), (uint32_t)mComputeCommandBuffers.size(), mComputeCommandBuffers.data());
    }
    mComputeCommandBuffers.clear();
    mComputeCommandBuffers.shrink_to_fit();
}

void VKRenderDevice::UpdateCurrentIndex()
{
	// 更新当前帧的索引
	mCurrentFrame = (mCurrentFrame + 1) % mSwapChain->GetSwapChainImageCount();

	// 清理垃圾收集器中的资源
	if (mVulkanContext->garbageCollector)
	{
		mVulkanContext->garbageCollector->AdvanceFrame();
		mVulkanContext->garbageCollector->Cleanup();
	}
}

NAMESPACE_RENDERCORE_END
