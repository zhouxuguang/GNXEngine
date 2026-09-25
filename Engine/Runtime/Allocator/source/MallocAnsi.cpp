//
//  MallocAnsi.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#include "MallocAnsi.h"

#include <algorithm>

#if GNX_OS_WINDOWS
#include <malloc.h>
#endif

NS_ALLOCATOR_BEGIN

MallocAnsi::MallocAnsi()
{
#if GNX_OS_WINDOWS
    // Enable low fragmentation heap - http://msdn2.microsoft.com/en-US/library/aa366750.aspx
    intptr_t    CrtHeapHandle = _get_heap_handle();
    ULONG        EnableLFH = 2;
    HeapSetInformation((void*)CrtHeapHandle, HeapCompatibilityInformation, &EnableLFH, sizeof(EnableLFH));
#endif
}

void* MallocAnsi::Alloc(size_t size, size_t* usableSizeOut)
{
    return AlignedAlloc(size, VOID_PTR_SIZE, usableSizeOut);
}

void* MallocAnsi::AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut)
{
#if GNX_OS_MACOS | GNX_OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8,
	// so on Mac we always have to use scalable_aligned_realloc
	alignment = std::max((size_t)16, alignment);
#else
	alignment = std::max((size_t)VOID_PTR_SIZE, alignment);
#endif
	void* ptr = baselib::AlignedMalloc(size, alignment);
	if (ptr && usableSizeOut)
	{
		*usableSizeOut = baselib::GetAllocationSize(ptr);
	}
	return ptr;
}

bool MallocAnsi::FreeAndGetSize(void* ptr, size_t& usableSizeOut)
{
    usableSizeOut = 0;
    if (!ptr)
    {
        return false;
    }

    usableSizeOut = baselib::GetAllocationSize(ptr);
    baselib::AlignedFree(ptr);
    return true;
}

bool MallocAnsi::GetAllocationSize(void *ptr, size_t &sizeOut)
{
    sizeOut = 0;
    if (!ptr)
    {
        return false;
    }

    sizeOut = baselib::GetAllocationSize(ptr);
    return sizeOut > 0;
}

bool MallocAnsi::OwnsPointer(const void* ptr) const
{
    return ptr != nullptr;
}

void MallocAnsi::Trim(bool bTrimThreadCaches)
{
    (void)bTrimThreadCaches;
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
