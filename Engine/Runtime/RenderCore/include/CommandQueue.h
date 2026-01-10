//
//  CommandQueue.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#ifndef GNX_ENGINE_COMMAND_QUEUE_INCLUDE_H
#define GNX_ENGINE_COMMAND_QUEUE_INCLUDE_H

#include "RenderDefine.h"
#include <memory>
#include <string>

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief 队列类型枚举
 *
 * 定义不同类型的命令队列，用于提交不同类型的GPU工作
 */
enum class QueueType
{
    Graphics,   // 图形队列：用于渲染命令（Draw调用、Pipeline绑定等）
    Compute,    // 计算队列：用于Compute Shader dispatch
    Transfer,   // 传输队列：用于资源上传/下载（Buffer/Texture数据传输）
};

/**
 * @brief 获取队列类型的名称字符串
 */
inline const char* GetQueueTypeName(QueueType type)
{
    switch (type)
    {
        case QueueType::Graphics: return "Graphics";
        case QueueType::Compute: return "Compute";
        case QueueType::Transfer: return "Transfer";
        default: return "Unknown";
    }
}

/**
 * @brief 队列优先级
 */
enum class QueuePriority
{
    Low,        // 低优先级：后台任务
    Normal,     // 正常优先级：默认渲染任务
    High,       // 高优先级：需要快速完成的任务
};

/**
 * @brief 命令队列抽象接口
 *
 * 这是RHI层的命令队列抽象，用于封装不同图形API的队列概念。
 * Vulkan有独立的Graphics、Compute、Transfer队列
 * Metal的CommandQueue支持所有类型的命令，但在逻辑上可以区分
 */
class CommandQueue
{
public:
    CommandQueue() = default;
    virtual ~CommandQueue() = default;

    /**
     * @brief 获取队列类型
     */
    virtual QueueType GetType() const = 0;

    /**
     * @brief 获取队列优先级
     */
    virtual QueuePriority GetPriority() const = 0;

    /**
     * @brief 获取队列索引（在同类队列中的索引）
     */
    virtual uint32_t GetQueueIndex() const = 0;

    /**
     * @brief 获取队列的描述信息
     */
    virtual std::string GetDescription() const = 0;
};

typedef std::shared_ptr<CommandQueue> CommandQueuePtr;
typedef std::weak_ptr<CommandQueue> CommandQueueWeakPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_COMMAND_QUEUE_INCLUDE_H */
