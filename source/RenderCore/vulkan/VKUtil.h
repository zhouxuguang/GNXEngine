//
//  VKUtil.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VKUTIL_INCLUDE_SDGKDFKHFGKHK
#define GNX_ENGINE_VKUTIL_INCLUDE_SDGKDFKHFGKHK

#include "VKRenderDefine.h"
#include "RenderPass.h"

NAMESPACE_RENDERCORE_BEGIN

#define ASSERT assert

// 将ptr连接到chainStart的next指针，ptr的next指向chainStart原先的pNext
template <typename VulkanStruct1, typename VulkanStruct2>
void AddToPNextChain(VulkanStruct1 *chainStart, VulkanStruct2 *ptr)
{
    // Catch bugs where this function is called with `&pointer` instead of `pointer`.
    static_assert(!std::is_pointer<VulkanStruct1>::value);
    static_assert(!std::is_pointer<VulkanStruct2>::value);

    assert(ptr->pNext == nullptr);

    VkBaseOutStructure *localPtr = reinterpret_cast<VkBaseOutStructure *>(chainStart);
    ptr->pNext                   = localPtr->pNext;
    localPtr->pNext              = reinterpret_cast<VkBaseOutStructure *>(ptr);
}

// 将ptr连接到chainStart的最后的next指针
template <typename VulkanStruct1, typename VulkanStruct2>
void AppendToPNextChain(VulkanStruct1 *chainStart, VulkanStruct2 *ptr)
{
    static_assert(!std::is_pointer<VulkanStruct1>::value);
    static_assert(!std::is_pointer<VulkanStruct2>::value);

    if (!ptr)
    {
        return;
    }

    VkBaseOutStructure *endPtr = reinterpret_cast<VkBaseOutStructure *>(chainStart);
    while (endPtr->pNext)
    {
        endPtr = endPtr->pNext;
    }
    endPtr->pNext = reinterpret_cast<VkBaseOutStructure *>(ptr);
}

// A helper class to disallow copy and assignment operators
class NonCopyable
{
protected:
	constexpr NonCopyable() = default;
	~NonCopyable() = default;

private:
	NonCopyable(const NonCopyable&) = delete;
	void operator=(const NonCopyable&) = delete;
};

// vulkan对象的包装类
template <typename DerivedT, typename HandleT>
class WrappedObject : NonCopyable
{
public:
	HandleT getHandle() const { return mHandle; }
	void setHandle(HandleT handle) { mHandle = handle; }
	bool valid() const { return (mHandle != VK_NULL_HANDLE); }

	const HandleT* ptr() const { return &mHandle; }

	HandleT release()
	{
		HandleT handle = mHandle;
		mHandle = VK_NULL_HANDLE;
		return handle;
	}

protected:
	WrappedObject() : mHandle(VK_NULL_HANDLE) {}
	~WrappedObject() { ASSERT(!valid()); }

	WrappedObject(WrappedObject&& other) : mHandle(other.mHandle)
	{
		other.mHandle = VK_NULL_HANDLE;
	}

	// Only works to initialize empty objects, since we don't have the device handle.
	WrappedObject& operator=(WrappedObject&& other)
	{
		ASSERT(!valid());
		std::swap(mHandle, other.mHandle);
		return *this;
	}

	HandleT mHandle;
};

class VulkanFence final : public WrappedObject<VulkanFence, VkFence>
{
public:
	VulkanFence() = default;
	void destroy(VkDevice device)
	{
		if (valid())
		{
			vkDestroyFence(device, mHandle, nullptr);
			mHandle = VK_NULL_HANDLE;
		}
	}
	using WrappedObject::operator=;

	VkResult init(VkDevice device, const VkFenceCreateInfo& createInfo)
	{
		ASSERT(!valid());
		return vkCreateFence(device, &createInfo, nullptr, &mHandle);
	}

	VkResult reset(VkDevice device)
	{
		ASSERT(valid());
		return vkResetFences(device, 1, &mHandle);
	}

	VkResult getStatus(VkDevice device) const
	{
		ASSERT(valid());
		return vkGetFenceStatus(device, mHandle);
	}

	VkResult wait(VkDevice device, uint64_t timeout) const
	{
		ASSERT(valid());
		return vkWaitForFences(device, 1, &mHandle, true, timeout);
	}
};

using VulkanFencePtr = std::shared_ptr<VulkanFence>;

// fence池,初始化时没有信号
class VulkanFencePool
{
public:
	VulkanFencePtr createFence(VkDevice device);

	void releaseFence(VkDevice device, VulkanFencePtr fence);

private:
	std::vector<VulkanFencePtr> mFreeFences;
	std::vector<VulkanFencePtr> mBusyFences;
	baselib::MutexLock mLock;
};

struct VulkanContext;

//纹理上传的task
class UpLoadTask : public baselib::TaskRunner
{
public:
	UpLoadTask();
	~UpLoadTask();

	VkImageSubresourceRange subresourceRange;
	VkBuffer stageBuffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VkImage mImage = VK_NULL_HANDLE;
	Rect2D rect;

	std::shared_ptr<VulkanContext> mContext = nullptr;
	VulkanFencePtr fence = nullptr;

	void CleanUpResource();
private:
	virtual void Run();

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
};

using UpLoadTaskPtr = std::shared_ptr<UpLoadTask>;

// 资源异步上传线程池
class UpLoadThreadPool
{
public:
	UpLoadThreadPool();
	~UpLoadThreadPool();

	void Start();

	void Execute(const UpLoadTaskPtr task);

	void Update(VkDevice device);
private:
	baselib::ThreadPool mThreadPool;

	baselib::MutexLock mLock;
	std::unordered_set<UpLoadTaskPtr> mLoadingTask;
};

VkAttachmentLoadOp GetLoadOP(AttachmentLoadOp loadOp);

VkAttachmentStoreOp GetStoreOP(AttachmentStoreOp storeOp);

NAMESPACE_RENDERCORE_END

#endif // GNX_ENGINE_VKUTIL_INCLUDE_SDGKDFKHFGKHK