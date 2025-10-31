//
//  MallocMimalloc.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/29.
//

#include "MallocMimalloc.h"
#include "mimalloc.h"
#include "mimalloc/internal.h"

NS_ALLOCATOR_BEGIN

MallocMimalloc::MallocMimalloc()
{
}

void* MallocMimalloc::Alloc(size_t size)
{
    void* newPtr = nullptr;
    size_t alignment = VOID_PTR_SIZE;
#if OS_MACOS | OS_IOS
    // macOS expects all allocations to be aligned to 16 bytes
    alignment = std::max((size_t)16, alignment);
#else
#endif
    newPtr = mi_malloc_aligned(size, alignment);
    return newPtr;
}

void* MallocMimalloc::AlignedAlloc(size_t size, size_t alignment)
{
	void* newPtr = nullptr;
#if OS_MACOS | OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes
	alignment = std::max((size_t)16, alignment);
#else
#endif
	newPtr = mi_malloc_aligned(size, alignment);
	return newPtr;
}

void MallocMimalloc::Free(void* ptr)
{
    if (!ptr)
	{
		return;
	}

	mi_free(ptr);
}

bool MallocMimalloc::GetAllocationSize(void *ptr, size_t &sizeOut)
{
    sizeOut = mi_malloc_size(ptr);
	return true;
}

void MallocMimalloc::Trim(bool bTrimThreadCaches)
{
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
