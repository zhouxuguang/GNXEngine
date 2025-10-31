//
//  MallocTBB.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#include "MallocTBB.h"
#include <tbb/scalable_allocator.h>

NS_ALLOCATOR_BEGIN

MallocTBB::MallocTBB()
{
}

void* MallocTBB::Alloc(size_t size)
{
    void* newPtr = nullptr;
    size_t alignment = VOID_PTR_SIZE;
#if OS_MACOS | OS_IOS
    // macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8, 
    // so on Mac we always have to use scalable_aligned_realloc
    alignment = std::max((size_t)16, alignment);
#else
#endif
    newPtr = scalable_aligned_malloc(size, alignment);
    return newPtr;
}

void* MallocTBB::AlignedAlloc(size_t size, size_t alignment)
{
	void* newPtr = nullptr;
#if OS_MACOS | OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8, 
	// so on Mac we always have to use scalable_aligned_realloc
	alignment = std::max((size_t)16, alignment);
#else
#endif
	newPtr = scalable_aligned_malloc(size, alignment);
	return newPtr;
}

void MallocTBB::Free(void* ptr)
{
    if (!ptr)
	{
		return;
	}

	scalable_free(ptr);
}

bool MallocTBB::GetAllocationSize(void *ptr, size_t &sizeOut)
{
    sizeOut = scalable_msize(ptr);
	return true;
}

void MallocTBB::Trim(bool bTrimThreadCaches)
{
    // TBB memory trimming might impact performance so it is only enabled in editor for now where large thread pools are used
    // and more likely to do allocation migration between threads.
#if WITH_EDITOR
	scalable_allocation_command(bTrimThreadCaches ? TBBMALLOC_CLEAN_ALL_BUFFERS : TBBMALLOC_CLEAN_THREAD_BUFFERS, 0);
#endif
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
