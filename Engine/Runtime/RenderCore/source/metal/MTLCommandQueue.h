//
//  MTLCommandQueue.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#ifndef GNX_ENGINE_MTL_COMMAND_QUEUE_INCLUDE_H
#define GNX_ENGINE_MTL_COMMAND_QUEUE_INCLUDE_H

#include "CommandQueue.h"
#include "MTLRenderDefine.h"
#include <string>

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief Metal命令队列实现
 *
 * 封装Metal的id<MTLCommandQueue>，提供RHI统一的命令队列接口。
 * 注意：Metal的CommandQueue本身支持所有类型的命令，这里主要是逻辑上的分类。
 */
class MTLCommandQueue : public CommandQueue
{
public:
    /**
     * @brief 构造函数
     *
     * @param queue Metal命令队列
     * @param type 逻辑队列类型（用于区分用途）
     * @param priority 队列优先级
     * @param queueIndex 在同类队列中的索引
     */
    MTLCommandQueue(id<MTLCommandQueue> queue,
             QueueType type,
             QueuePriority priority,
             uint32_t queueIndex);

    virtual ~MTLCommandQueue();

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
     * @brief 获取Metal命令队列
     */
    id<MTLCommandQueue> GetMTLCommandQueue() const { return mCommandQueue; }

private:
    id<MTLCommandQueue> mCommandQueue = nil;
    QueueType mType;
    QueuePriority mPriority;
    uint32_t mQueueIndex;
};

typedef std::shared_ptr<MTLCommandQueue> MTLCommandQueuePtr;
typedef std::weak_ptr<MTLCommandQueue> MTLCommandQueueWeakPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMMAND_QUEUE_INCLUDE_H */
