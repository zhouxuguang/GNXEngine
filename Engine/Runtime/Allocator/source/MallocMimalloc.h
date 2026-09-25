//
//  MallocMimalloc.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/29.
//

#ifndef GNXENGINE_MALLOC_MIMALLOC_INCLUDE_JHSJDVGHBDF
#define GNXENGINE_MALLOC_MIMALLOC_INCLUDE_JHSJDVGHBDF

#include "../include/AMalloc.h"

NS_ALLOCATOR_BEGIN

class ALLOCATOR_API MallocMimalloc : public Malloc
{
public:
    MallocMimalloc() = default;
	void* Alloc(size_t size, size_t* usableSizeOut = nullptr) override;
	void* AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut = nullptr) override;
    bool FreeAndGetSize(void* ptr, size_t& usableSizeOut) override;
    bool GetAllocationSize(void *ptr, size_t &sizeOut) override;
    bool OwnsPointer(const void* ptr) const override;
    void Trim(bool bTrimThreadCaches) override;
    bool IsThreadSafe() const override;
    const char* GetDescriptiveName() const override;
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_MIMALLOC_INCLUDE_JHSJDVGHBDF */
