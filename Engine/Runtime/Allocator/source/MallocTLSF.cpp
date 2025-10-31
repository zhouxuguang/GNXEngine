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
    // macOS expects all allocations to be aligned to 16 bytes
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

void* MallocTLSF::AlignedAlloc(size_t size, size_t alignment)
{
	void* newPtr = nullptr;
#if OS_MACOS | OS_IOS
	// macOS expects all allocations to be aligned to 16 bytes
	alignment = std::max((size_t)16, alignment);
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
