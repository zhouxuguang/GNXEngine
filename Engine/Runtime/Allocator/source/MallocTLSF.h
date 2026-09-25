//
//  MallocTLSF.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/27.
//

#ifndef GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH
#define GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH

#include <cstdint>
#include <mutex>

#include "../include/AMalloc.h"
#include "tlsf.h"

NS_ALLOCATOR_BEGIN

class ALLOCATOR_API MallocTLSF : public Malloc
{
public:
    MallocTLSF();
    ~MallocTLSF() override;

	void* Alloc(size_t size, size_t* usableSizeOut = nullptr) override;
	void* AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut = nullptr) override;
    bool FreeAndGetSize(void* ptr, size_t& usableSizeOut) override;
    bool GetAllocationSize(void *ptr, size_t &sizeOut) override;
    bool OwnsPointer(const void* ptr) const override;
    void Trim(bool bTrimThreadCaches) override;
    bool IsThreadSafe() const override;
    const char* GetDescriptiveName() const override;

    size_t GetPoolCount() const;

private:
    struct PoolRange
    {
        pool_t pool;
        void*  base;
        size_t bytes;
    };

    static constexpr size_t kMaxPoolCount = 128;

    void* AllocInternal(size_t size, size_t alignment, size_t* usableSizeOut);
    bool  AddPool(size_t bytes);
    bool  OwnsPointerLocked(const void* ptr) const;

    tlsf_t    mTLSF = nullptr;
    PoolRange mPools[kMaxPoolCount];
    size_t    mPoolCount = 0;
    mutable std::mutex mMutex;
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH */
