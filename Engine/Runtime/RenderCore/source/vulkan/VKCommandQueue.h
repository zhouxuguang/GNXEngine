//
//  VKCommandQueue.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#ifndef GNX_ENGINE_VK_COMMAND_QUEUE_INCLUDE_H
#define GNX_ENGINE_VK_COMMAND_QUEUE_INCLUDE_H

#include "CommandQueue.h"
#include "VKRenderDefine.h"
#include <string>

NAMESPACE_RENDERCORE_BEGIN

// 前向声明
class VKRenderDevice;

/**
 * @brief Vulkan命令队列实现
 *
 * 封装Vulkan的VkQueue，提供RHI统一的命令队列接口
 */
class VKCommandQueue : public CommandQueue
{
public:
    /**
     * @brief 构造函数
     *
     * @param renderDevice 所属的渲染设备
     * @param queue Vulkan队列句柄
     * @param type 队列类型
     * @param priority 队列优先级
     * @param queueIndex 在同类队列中的索引
     * @param queueFamilyIndex 队列族索引
     */
    VKCommandQueue(VKRenderDevice* renderDevice,
            VkQueue queue,
            QueueType type,
            QueuePriority priority,
            uint32_t queueIndex,
            uint32_t queueFamilyIndex);

    virtual ~VKCommandQueue() = default;

    /**
     * @brief 获取队列类型
     */
    virtual QueueType GetType() const override { return mType; }

    /**
     * @brief 获取队列优先级
     */
    virtual QueuePriority GetPriority() const override { return mPriority; }

    /**
     * @brief 获取队列索引
     */
    virtual uint32_t GetQueueIndex() const override { return mQueueIndex; }

    /**
     * @brief 获取队列描述
     */
    virtual std::string GetDescription() const override;

    /**
     * @brief 创建命令缓冲区
     */
    virtual CommandBufferPtr CreateCommandBuffer() override;

    /**
     * @brief 获取Vulkan队列句柄
     */
    VkQueue GetVkQueue() const { return mQueue; }

    /**
     * @brief 获取队列族索引
     */
    uint32_t GetQueueFamilyIndex() const { return mQueueFamilyIndex; }

    /**
     * @brief 获取渲染设备
     */
    VKRenderDevice* GetRenderDevice() const { return mRenderDevice; }

private:
    VKRenderDevice* mRenderDevice = nullptr;
    VkQueue mQueue = VK_NULL_HANDLE;
    QueueType mType;
    QueuePriority mPriority;
    uint32_t mQueueIndex;
    uint32_t mQueueFamilyIndex;
};

typedef std::shared_ptr<VKCommandQueue> VKCommandQueuePtr;
typedef std::weak_ptr<VKCommandQueue> VKCommandQueueWeakPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_COMMAND_QUEUE_INCLUDE_H */
