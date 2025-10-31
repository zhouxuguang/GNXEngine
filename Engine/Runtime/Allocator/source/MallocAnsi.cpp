//
//  MallocAnsi.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#include "MallocAnsi.h"

#if OS_WINDOWS
#include <malloc.h>
#endif

NS_ALLOCATOR_BEGIN

MallocAnsi::MallocAnsi()
{
#if OS_WINDOWS
    // Enable low fragmentation heap - http://msdn2.microsoft.com/en-US/library/aa366750.aspx
    intptr_t    CrtHeapHandle = _get_heap_handle();
    ULONG        EnableLFH = 2;
    HeapSetInformation((void*)CrtHeapHandle, HeapCompatibilityInformation, &EnableLFH, sizeof(EnableLFH));
#endif
}

void* MallocAnsi::Alloc(size_t size)
{
    void* newPtr = nullptr;
    size_t alignment = VOID_PTR_SIZE;
#if OS_MACOS | OS_IOS
    // macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8,
    // so on Mac we always have to use scalable_aligned_realloc
    alignment = std::max((size_t)16, alignment);
#else
#endif
    newPtr = baselib::AlignedMalloc(size, alignment);
    return newPtr;
}

void* MallocAnsi::AlignedAlloc(size_t size, size_t alignment)
{
	void* newPtr = nullptr;
#if OS_MACOS | OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8,
	// so on Mac we always have to use scalable_aligned_realloc
	alignment = std::max((size_t)16, alignment);
#else
#endif
	newPtr = baselib::AlignedMalloc(size, alignment);
	return newPtr;
}

void MallocAnsi::Free(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    baselib::AlignedFree(ptr);
}

bool MallocAnsi::GetAllocationSize(void *ptr, size_t &sizeOut)
{
    sizeOut = baselib::GetAllocationSize(ptr);
    return true;
}

void MallocAnsi::Trim(bool bTrimThreadCaches)
{
}

bool MallocAnsi::IsThreadSafe() const
{
    return true;
}

const char* MallocAnsi::GetDescriptiveName() const
{
    return "ANSI";
}

NS_ALLOCATOR_END
