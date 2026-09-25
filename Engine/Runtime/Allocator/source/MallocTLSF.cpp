//
//  MallocTBB.cpp
//  Allocator
//
//  Created by zhouxuguang on 2025/10/27.
//

#include "MallocTLSF.h"
#include "tlsf.h"

#include <cstdlib>

NS_ALLOCATOR_BEGIN

namespace
{

const size_t kInitialPoolBytes = 1024 * 1024 * 2;
const size_t kPoolGrowBytes = 1024 * 1024 * 16;
const size_t kBlockHeaderBytes = 4 * sizeof(void*);

size_t NormalizeAlignment(size_t alignment)
{
    size_t result = tlsf_align_size();
    while (result < alignment)
    {
        const size_t next = result << 1;
        if (next <= result)
        {
            break;
        }
        result = next;
    }
    return result;
}

size_t AlignUp(size_t value, size_t align)
{
    return (value + (align - 1)) & ~(align - 1);
}

bool ComputePoolBytesForRequest(size_t size, size_t alignment, size_t& poolBytesOut)
{
    const size_t alignSize = tlsf_align_size();
    const size_t align = (alignment < alignSize) ? alignSize : alignment;

    if (size > SIZE_MAX - (alignSize - 1))
    {
        return false;
    }
    const size_t inner = AlignUp(size, alignSize);
    if (inner >= tlsf_block_size_max())
    {
        return false;
    }

    if (inner > SIZE_MAX - align - kBlockHeaderBytes)
    {
        return false;
    }
    size_t alignedSize = inner + align + kBlockHeaderBytes;
    if (alignedSize > SIZE_MAX - (align - 1))
    {
        return false;
    }
    alignedSize = AlignUp(alignedSize, align);

    const size_t bucketMargin = (alignedSize >> 4) + 1;
    if (alignedSize > SIZE_MAX - bucketMargin)
    {
        return false;
    }
    const size_t need = alignedSize + bucketMargin;

    if (need > SIZE_MAX - tlsf_pool_overhead() - alignSize)
    {
        return false;
    }
    const size_t poolBytes = need + tlsf_pool_overhead() + alignSize;

    if (poolBytes <= tlsf_pool_overhead())
    {
        return false;
    }
    if ((poolBytes - tlsf_pool_overhead()) > tlsf_block_size_max())
    {
        return false;
    }

    poolBytesOut = poolBytes;
    return true;
}

void CountUsedBlocks(void* ptr, size_t size, int used, void* user)
{
    (void)ptr;
    (void)size;
    if (used)
    {
        ++(*static_cast<size_t*>(user));
    }
}

}

MallocTLSF::MallocTLSF()
{
    void* ptr = ::malloc(kInitialPoolBytes);
    if (!ptr)
    {
        return;
    }

    mTLSF = tlsf_create_with_pool(ptr, kInitialPoolBytes);
    if (!mTLSF)
    {
        ::free(ptr);
        return;
    }

    mPools[0].pool = tlsf_get_pool(mTLSF);
    mPools[0].base = ptr;
    mPools[0].bytes = kInitialPoolBytes;
    mPoolCount = 1;
}

MallocTLSF::~MallocTLSF()
{
    std::lock_guard<std::mutex> lock(mMutex);

    mTLSF = nullptr;

    for (size_t i = 0; i < mPoolCount; ++i)
    {
        ::free(mPools[i].base);
    }
    mPoolCount = 0;
}

void* MallocTLSF::Alloc(size_t size, size_t* usableSizeOut)
{
    return AllocInternal(size, VOID_PTR_SIZE, usableSizeOut);
}

void* MallocTLSF::AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut)
{
    return AllocInternal(size, alignment, usableSizeOut);
}

void* MallocTLSF::AllocInternal(size_t size, size_t alignment, size_t* usableSizeOut)
{
    if (size == 0)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    if (!mTLSF)
    {
        return nullptr;
    }

    alignment = NormalizeAlignment(alignment);

    void* newPtr = tlsf_memalign(mTLSF, alignment, size);
    if (!newPtr)
    {
        size_t poolBytes = 0;
        if (!ComputePoolBytesForRequest(size, alignment, poolBytes))
        {
            return nullptr;
        }

        if (!AddPool(poolBytes))
        {
            return nullptr;
        }

        newPtr = tlsf_memalign(mTLSF, alignment, size);
    }

    if (newPtr && usableSizeOut)
    {
        *usableSizeOut = tlsf_block_size(newPtr);
    }
    return newPtr;
}

bool MallocTLSF::AddPool(size_t bytes)
{
    if (mPoolCount >= kMaxPoolCount)
    {
        return false;
    }

    size_t poolBytes = (bytes < kPoolGrowBytes) ? kPoolGrowBytes : bytes;
    if (poolBytes <= tlsf_pool_overhead() ||
        (poolBytes - tlsf_pool_overhead()) > tlsf_block_size_max())
    {
        return false;
    }

    void* mem = ::malloc(poolBytes);
    if (!mem)
    {
        return false;
    }

    if ((reinterpret_cast<uintptr_t>(mem) % tlsf_align_size()) != 0)
    {
        ::free(mem);
        return false;
    }

    pool_t pool = tlsf_add_pool(mTLSF, mem, poolBytes);
    if (!pool)
    {
        ::free(mem);
        return false;
    }

    mPools[mPoolCount].pool = pool;
    mPools[mPoolCount].base = mem;
    mPools[mPoolCount].bytes = poolBytes;
    ++mPoolCount;
    return true;
}

bool MallocTLSF::FreeAndGetSize(void* ptr, size_t& usableSizeOut)
{
    usableSizeOut = 0;
    if (!ptr)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    if (!mTLSF || !OwnsPointerLocked(ptr))
    {
        return false;
    }

    usableSizeOut = tlsf_block_size(ptr);
    tlsf_free(mTLSF, ptr);
    return true;
}

bool MallocTLSF::GetAllocationSize(void *ptr, size_t &sizeOut)
{
    sizeOut = 0;
    if (!ptr)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    if (!mTLSF || !OwnsPointerLocked(ptr))
    {
        return false;
    }

    sizeOut = tlsf_block_size(ptr);
    return true;
}

bool MallocTLSF::OwnsPointer(const void* ptr) const
{
    if (!ptr)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    return (mTLSF != nullptr) && OwnsPointerLocked(ptr);
}

bool MallocTLSF::OwnsPointerLocked(const void* ptr) const
{
    const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    for (size_t i = 0; i < mPoolCount; ++i)
    {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(mPools[i].base);
        const uintptr_t end = begin + mPools[i].bytes;
        if (address >= begin && address < end)
        {
            return true;
        }
    }
    return false;
}

void MallocTLSF::Trim(bool bTrimThreadCaches)
{
    (void)bTrimThreadCaches;

    std::lock_guard<std::mutex> lock(mMutex);
    if (!mTLSF || mPoolCount <= 1)
    {
        return;
    }

    size_t i = mPoolCount;
    while (i-- > 1)
    {
        size_t usedBlocks = 0;
        tlsf_walk_pool(mPools[i].pool, &CountUsedBlocks, &usedBlocks);
        if (usedBlocks != 0)
        {
            continue;
        }

        tlsf_remove_pool(mTLSF, mPools[i].pool);
        ::free(mPools[i].base);
        --mPoolCount;

        if (i != mPoolCount)
        {
            mPools[i] = mPools[mPoolCount];
        }
    }
}

bool MallocTLSF::IsThreadSafe() const
{
    return true;
}

const char* MallocTLSF::GetDescriptiveName() const
{
    return "TLSF";
}

size_t MallocTLSF::GetPoolCount() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mPoolCount;
}

NS_ALLOCATOR_END
