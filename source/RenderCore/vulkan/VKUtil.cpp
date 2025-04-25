#include "VKUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VulkanFencePtr VulkanFencePool::createFence(VkDevice device)
{
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
	return NULL;
}

void VulkanFencePool::releaseFence(VkDevice device, VulkanFencePtr fence)
{
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

NAMESPACE_RENDERCORE_END
