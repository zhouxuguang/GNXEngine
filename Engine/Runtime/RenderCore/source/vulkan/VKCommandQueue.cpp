//
//  VKCommandQueue.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#include "VKCommandQueue.h"
#include <sstream>

NAMESPACE_RENDERCORE_BEGIN

VKCommandQueue::VKCommandQueue(VkQueue queue,
                 QueueType type,
                 QueuePriority priority,
                 uint32_t queueIndex,
                 uint32_t queueFamilyIndex)
    : mQueue(queue)
    , mType(type)
    , mPriority(priority)
    , mQueueIndex(queueIndex)
    , mQueueFamilyIndex(queueFamilyIndex)
{
}

std::string VKCommandQueue::GetDescription() const
{
    std::ostringstream oss;
    oss << "VKCommandQueue[type=" << GetQueueTypeName(mType)
        << ", index=" << mQueueIndex
        << ", family=" << mQueueFamilyIndex
        << ", priority=";

    switch (mPriority)
    {
        case QueuePriority::Low: oss << "Low"; break;
        case QueuePriority::Normal: oss << "Normal"; break;
        case QueuePriority::High: oss << "High"; break;
        default: oss << "Unknown"; break;
    }

    oss << ", handle=" << reinterpret_cast<uintptr_t>(mQueue) << "]";
    return oss.str();
}

NAMESPACE_RENDERCORE_END
