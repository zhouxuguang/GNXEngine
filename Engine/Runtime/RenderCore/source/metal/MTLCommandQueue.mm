//
//  MTLCommandQueue.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#import <Metal/Metal.h>
#include "MTLCommandQueue.h"
#include <sstream>

NAMESPACE_RENDERCORE_BEGIN

MTLCommandQueue::MTLCommandQueue(id<MTLCommandQueue> queue,
                   QueueType type,
                   QueuePriority priority,
                   uint32_t queueIndex)
    : mCommandQueue(queue)
    , mType(type)
    , mPriority(priority)
    , mQueueIndex(queueIndex)
{
}

MTLCommandQueue::~MTLCommandQueue()
{
    if (mCommandQueue != nil)
    {
        mCommandQueue = nil;
    }
}

std::string MTLCommandQueue::GetDescription() const
{
    std::ostringstream oss;
    oss << "MTLCommandQueue[type=" << GetQueueTypeName(mType)
        << ", index=" << mQueueIndex
        << ", priority=";

    switch (mPriority)
    {
        case QueuePriority::Low: oss << "Low"; break;
        case QueuePriority::Normal: oss << "Normal"; break;
        case QueuePriority::High: oss << "High"; break;
        default: oss << "Unknown"; break;
    }

    oss << ", handle=" << reinterpret_cast<uintptr_t>(mCommandQueue) << "]";
    return oss.str();
}

NAMESPACE_RENDERCORE_END
