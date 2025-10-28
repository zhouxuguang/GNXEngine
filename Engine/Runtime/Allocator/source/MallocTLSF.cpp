//
//  MallocTBB.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/27.
//

#include "MallocTLSF.h"
#include "tlsf.h"

NS_ALLOCATOR_BEGIN

MallocTLSF::MallocTLSF()
{
    void* ptr = malloc(1024 * 1024 * 2);
    mTLSF = tlsf_create_with_pool(ptr, 1024 * 1024 * 2);
}

MallocTLSF::~MallocTLSF()
{
	if (mTLSF)
	{
        tlsf_destroy(mTLSF);
        mTLSF = nullptr;
	}
}

void* MallocTLSF::Alloc(size_t size)
{
    void* newPtr = nullptr;
    size_t alignment = VOID_PTR_SIZE;
#if OS_MACOS | OS_IOS
    // macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8, 
    // so on Mac we always have to use scalable_aligned_realloc
    alignment = std::max((size_t)16, alignment);
#else
#endif
    newPtr = tlsf_memalign(mTLSF, alignment, size);
	while (!newPtr)
    {
		void* add_pool = malloc(104857600);   //100MB
		tlsf_add_pool(mTLSF, add_pool, 104857600);
        newPtr = tlsf_memalign(mTLSF, alignment, size);
	}
    return newPtr;
}

void* MallocTLSF::AlignedAlloc(size_t size, uint32_t alignment)
{
	void* newPtr = nullptr;
#if OS_MACOS | OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes, but TBBs default alignment is 8, 
	// so on Mac we always have to use scalable_aligned_realloc
	alignment = std::max((uint32_t)16, alignment);
#else
#endif
    newPtr = tlsf_memalign(mTLSF, alignment, size);
	return newPtr;
}

void MallocTLSF::Free(void* ptr)
{
    if (!ptr)
	{
		return;
	}

    tlsf_free(mTLSF, ptr);
}

bool MallocTLSF::GetAllocationSize(void *ptr, size_t &sizeOut)
{
    sizeOut = tlsf_block_size(ptr);
	return true;
}

void MallocTLSF::Trim(bool bTrimThreadCaches)
{
    // TBB memory trimming might impact performance so it is only enabled in editor for now where large thread pools are used
    // and more likely to do allocation migration between threads.
#if WITH_EDITOR
	scalable_allocation_command(bTrimThreadCaches ? TBBMALLOC_CLEAN_ALL_BUFFERS : TBBMALLOC_CLEAN_THREAD_BUFFERS, 0);
#endif
}

bool MallocTLSF::IsThreadSafe() const
{
    return false;
}

const char* MallocTLSF::GetDescriptiveName() const
{
    return "TLSF";
}

NS_ALLOCATOR_END
