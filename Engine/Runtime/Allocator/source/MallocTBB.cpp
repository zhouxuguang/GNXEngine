//
//  MallocTBB.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#include "MallocTBB.h"
#include <tbb/scalable_allocator.h>

#include <cstdlib>

extern "C"
{
    void   __TBB_malloc_safer_free(void* ptr, void (*original_free)(void*));
    size_t __TBB_malloc_safer_msize(void* ptr, size_t (*original_msize)(void*));
}

NS_ALLOCATOR_BEGIN

void* MallocTBB::Alloc(size_t size, size_t* usableSizeOut)
{
    return AlignedAlloc(size, VOID_PTR_SIZE, usableSizeOut);
}

void* MallocTBB::AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut)
{
#if GNX_OS_MACOS | GNX_OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8, 
	// so on Mac we always have to use scalable_aligned_realloc
    alignment = std::max((size_t)16, alignment);
#else
    alignment = std::max((size_t)VOID_PTR_SIZE, alignment);
#endif
	void* ptr = scalable_aligned_malloc(size, alignment);
	if (ptr && usableSizeOut)
	{
		*usableSizeOut = __TBB_malloc_safer_msize(ptr, nullptr);
	}
	return ptr;
}

bool MallocTBB::FreeAndGetSize(void* ptr, size_t& usableSizeOut)
{
    usableSizeOut = 0;
    if (!ptr)
    {
        return false;
    }

    const size_t size = __TBB_malloc_safer_msize(ptr, nullptr);
    if (size == 0)
    {
        return false;
    }

    __TBB_malloc_safer_free(ptr, nullptr);

    usableSizeOut = size;
    return true;
}

bool MallocTBB::GetAllocationSize(void *ptr, size_t &sizeOut)
{
	sizeOut = 0;
	if (!ptr)
	{
		return false;
	}

	sizeOut = __TBB_malloc_safer_msize(ptr, nullptr);
	return sizeOut > 0;
}

bool MallocTBB::OwnsPointer(const void* ptr) const
{
	if (!ptr)
	{
		return false;
	}

	return __TBB_malloc_safer_msize(const_cast<void*>(ptr), nullptr) > 0;
}

void MallocTBB::Trim(bool bTrimThreadCaches)
{
    scalable_allocation_command(bTrimThreadCaches ? TBBMALLOC_CLEAN_ALL_BUFFERS : TBBMALLOC_CLEAN_THREAD_BUFFERS, 0);
}

bool MallocTBB::IsThreadSafe() const
{
    return true;
}

const char* MallocTBB::GetDescriptiveName() const
{
    return "TBB";
}

NS_ALLOCATOR_END
