//
//  MallocMimalloc.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/29.
//

#include "MallocMimalloc.h"
#include "mimalloc.h"

#include <algorithm>

NS_ALLOCATOR_BEGIN

void* MallocMimalloc::Alloc(size_t size, size_t* usableSizeOut)
{
    return AlignedAlloc(size, VOID_PTR_SIZE, usableSizeOut);
}

void* MallocMimalloc::AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut)
{
#if GNX_OS_MACOS | GNX_OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes
    alignment = std::max((size_t)16, alignment);
#else
    alignment = std::max((size_t)VOID_PTR_SIZE, alignment);
#endif
	void* ptr = mi_malloc_aligned(size, alignment);
	if (ptr && usableSizeOut)
	{
		*usableSizeOut = mi_malloc_size(ptr);
	}
	return ptr;
}

bool MallocMimalloc::FreeAndGetSize(void* ptr, size_t& usableSizeOut)
{
    usableSizeOut = 0;
    if (!ptr || !mi_is_in_heap_region(ptr))
    {
        return false;
    }

    usableSizeOut = mi_malloc_size(ptr);
    mi_free(ptr);
    return true;
}

bool MallocMimalloc::GetAllocationSize(void *ptr, size_t &sizeOut)
{
	sizeOut = 0;
	if (!ptr || !mi_is_in_heap_region(ptr))
	{
		return false;
	}

	sizeOut = mi_malloc_size(ptr);
	return sizeOut > 0;
}

bool MallocMimalloc::OwnsPointer(const void* ptr) const
{
	if (!ptr)
	{
		return false;
	}

	return mi_is_in_heap_region(ptr);
}

void MallocMimalloc::Trim(bool bTrimThreadCaches)
{
    mi_collect(bTrimThreadCaches);
}

bool MallocMimalloc::IsThreadSafe() const
{
    return true;
}

const char* MallocMimalloc::GetDescriptiveName() const
{
    return "Mimalloc";
}

NS_ALLOCATOR_END
