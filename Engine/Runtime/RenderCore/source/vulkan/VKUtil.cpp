#include "VKUtil.h"
#include "VulkanContext.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VulkanFencePtr VulkanFencePool::createFence(VkDevice device)
{
	baselib::AutoLock lockGuard(mLock);
	if (mFreeFences.size() > 0)
	{
		VulkanFencePtr fence = mFreeFences.back();
		mFreeFences.pop_back();
		mBusyFences.push_back(fence);
		return fence;
	}

	VulkanFencePtr newFence = std::make_shared<VulkanFence>();

	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	newFence->init(device, createInfo);
	mBusyFences.push_back(newFence);
	return newFence;
}

void VulkanFencePool::releaseFence(VkDevice device, VulkanFencePtr fence)
{
	baselib::AutoLock lockGuard(mLock);
	fence->reset(device);
	for (uint32_t i = 0; i < mBusyFences.size(); ++i)
	{
		if (mBusyFences[i] == fence)
		{
			mBusyFences.erase(mBusyFences.begin() + i);
			break;
		}
	}
	mFreeFences.push_back(fence);
	fence = nullptr;
}

UpLoadTask::UpLoadTask()
{
}

UpLoadTask::~UpLoadTask()
{
}

void UpLoadTask::CleanUpResource()
{
	vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
	vkFreeCommandBuffers(mContext->device, commandPool, 1, &commandBuffer);
	mContext->fencePool.releaseFence(mContext->device, fence);
}

// UpLoadTask实现
void UpLoadTask::Run()
{
	commandPool = mContext->GetTransferCommandPool();
	commandBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, commandPool);

	VulkanBufferUtil::SetImageLayout(
		commandBuffer,
		mImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	VulkanBufferUtil::CopyBufferToImage(mContext->device, commandBuffer, stageBuffer, mImage,
		rect.offsetX, rect.offsetY, rect.width, rect.height, subresourceRange.baseMipLevel);

	// Change texture image layout to shader read after all faces have been copied
	VulkanBufferUtil::SetImageLayout(
		commandBuffer,
		mImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		subresourceRange);

	vkEndCommandBuffer(commandBuffer);

	//放到传输队列进行处理
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	fence = mContext->fencePool.createFence(mContext->device);

	baselib::AutoLock lockGuard(mContext->transferQueuesLock);
	vkQueueSubmit(mContext->availableTransferQueues[0], 1, &submitInfo, fence->getHandle());
}

// UpLoadThreadPool实现
UpLoadThreadPool::UpLoadThreadPool() : mThreadPool(4)
{
}

UpLoadThreadPool::~UpLoadThreadPool()
{
}

void UpLoadThreadPool::Start()
{
	mThreadPool.Start();
}

void UpLoadThreadPool::Execute(const UpLoadTaskPtr task)
{
	mThreadPool.Execute(task);
	baselib::AutoLock lock(mLock);
	mLoadingTask.insert(task);
}

void UpLoadThreadPool::Update(VkDevice device)
{
	// 先拷贝到临时容器，操作临时容器，最后将临时容器拷贝到成员变量，这样可以最大程度的减少锁的区间
	std::unordered_set<UpLoadTaskPtr> tempTask;
	{
		baselib::AutoLock lock(mLock);
		tempTask = mLoadingTask;
	}

	for (auto it = tempTask.begin(); it != tempTask.end();)
	{
		if ((*it)->fence && VK_SUCCESS == (*it)->fence->getStatus(device))
		{
			(*it)->CleanUpResource();
			it = tempTask.erase(it);
		}
		else 
		{
			++it;
		}
	}

	baselib::AutoLock lock(mLock);
	mLoadingTask = tempTask;
}

VkAttachmentLoadOp GetLoadOP(AttachmentLoadOp loadOp)
{
	VkAttachmentLoadOp loadOP = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	switch (loadOp)
	{
	case AttachmentLoadOp::ATTACHMENT_LOAD_OP_CLEAR:
		loadOP = VK_ATTACHMENT_LOAD_OP_CLEAR;
		break;
	case AttachmentLoadOp::ATTACHMENT_LOAD_OP_LOAD:
		loadOP = VK_ATTACHMENT_LOAD_OP_LOAD;
		break;

	case AttachmentLoadOp::ATTACHMENT_LOAD_OP_DONT_CARE:
		loadOP = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		break;
	}
	return loadOP;
}

VkAttachmentStoreOp GetStoreOP(AttachmentStoreOp storeOp)
{
	VkAttachmentStoreOp storeOP = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
	switch (storeOp)
	{
	case AttachmentStoreOp::ATTACHMENT_STORE_OP_STORE:
		storeOP = VK_ATTACHMENT_STORE_OP_STORE;
		break;
	case AttachmentStoreOp::ATTACHMENT_STORE_OP_DONT_CARE:
		storeOP = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		break;
	}
	return storeOP;
}

NAMESPACE_RENDERCORE_END

