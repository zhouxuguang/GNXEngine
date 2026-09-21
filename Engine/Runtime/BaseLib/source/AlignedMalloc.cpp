//
//  AlignedMalloc.cpp
//  BASELIB
//
//  Created by zhouxuguang on 16/12/9.
//  Copyright@ 2016年 zhouxuguang. All rights reserved.
//

#include "AlignedMalloc.h"
#include "LogService.h"
#include <assert.h>
#include <cstdlib>
#include <limits>

#if GNX_OS_WINDOWS || GNX_OS_LINUX
#include <malloc.h>
#endif

#if GNX_OS_MACOS
#include <malloc/malloc.h>
#endif

NS_BASELIB_BEGIN

#if GNX_OS_WINDOWS
namespace
{
struct AlignedAllocationHeader
{
    void* base = nullptr;
    size_t usableSize = 0;
};
}
#endif

void* AlignedMalloc(size_t size, size_t alignment)
{
    if (0 == alignment || 0 != (alignment & (alignment - 1)))
    {
        LOG_ERROR("AlignedMalloc: invalid alignment=%zu "
                  "(must be a non-zero power of two), size=%zu", alignment, size);
        return nullptr;
    }

    if (0 != (alignment % sizeof(void*)))
    {
        LOG_ERROR("AlignedMalloc: invalid alignment=%zu "
                  "(must be a multiple of sizeof(void*)=%zu), size=%zu",
                  alignment, sizeof(void*), size);
        return nullptr;
    }

    if (0 == size)
    {
        LOG_WARN("AlignedMalloc: size=0, allocating 1 byte instead");
        size = 1;
    }

    void *result = NULL;
#if GNX_OS_WINDOWS
    // _aligned_msize requires callers to repeat the original alignment, while this API only
    // receives the returned pointer. Reading the CRT's private header is not supported and can
    // report a size smaller than the requested allocation. Keep the metadata ourselves instead.
    constexpr size_t headerSize = sizeof(AlignedAllocationHeader);
    if (size > (std::numeric_limits<size_t>::max)() - headerSize - (alignment - 1))
    {
        LOG_ERROR("AlignedMalloc: size overflow, size=%zu alignment=%zu", size, alignment);
        return nullptr;
    }

    const size_t allocationSize = size + headerSize + alignment - 1;
    void* base = malloc(allocationSize);
    if (base)
    {
        const uintptr_t start = reinterpret_cast<uintptr_t>(base) + headerSize;
        const uintptr_t alignedAddress = (start + alignment - 1) & ~(uintptr_t)(alignment - 1);
        auto* header = reinterpret_cast<AlignedAllocationHeader*>(alignedAddress) - 1;
        header->base = base;
        header->usableSize = size;
        result = reinterpret_cast<void*>(alignedAddress);
    }
#elif GNX_OS_MACOS || GNX_OS_IOS
    if (posix_memalign(&result, alignment, size))
    {
        result = NULL;
    }
#elif GNX_OS_LINUX || GNX_OS_ANDROID
    result = memalign(alignment, size);
#endif

	if (!result)
	{
		assert(false);
	}
    assert((reinterpret_cast<uintptr_t>(result) & (alignment - 1)) == 0);

    return result;
}

void AlignedFree(void *ptr)
{
#if GNX_OS_WINDOWS
    if (ptr)
    {
        const auto* header = reinterpret_cast<const AlignedAllocationHeader*>(ptr) - 1;
        free(header->base);
    }
#else
    free(ptr);
#endif
}

size_t GetAllocationSize(void* ptr)
{
	if (nullptr == ptr)
	{
		return 0;
	}

#if GNX_OS_WINDOWS
    const auto* header = reinterpret_cast<const AlignedAllocationHeader*>(ptr) - 1;
    const size_t blockSize = _msize(header->base);
    const size_t offset = reinterpret_cast<const char*>(ptr) -
                          static_cast<const char*>(header->base);
    if (blockSize == static_cast<size_t>(-1) || blockSize < offset)
    {
        return header->usableSize;
    }

    const size_t usableSize = blockSize - offset;
    return usableSize >= header->usableSize ? usableSize : header->usableSize;
#elif GNX_OS_MACOS || GNX_OS_IOS
    return malloc_size(ptr);
#elif GNX_OS_LINUX || GNX_OS_ANDROID
    return malloc_usable_size(ptr);
#else
    return 0;
#endif
}

NS_BASELIB_END
