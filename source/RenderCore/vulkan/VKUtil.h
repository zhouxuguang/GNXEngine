//
//  VKUtil.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VKUTIL_INCLUDE_SDGKDFKHFGKHK
#define GNX_ENGINE_VKUTIL_INCLUDE_SDGKDFKHFGKHK

#include "VKRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

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

NAMESPACE_RENDERCORE_END

#endif // GNX_ENGINE_VKUTIL_INCLUDE_SDGKDFKHFGKHK