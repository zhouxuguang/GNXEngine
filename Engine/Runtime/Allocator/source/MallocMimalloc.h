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

class MallocMimalloc : public Malloc
{
public:
    MallocMimalloc();
	virtual void* Alloc(size_t size);
	virtual void* AlignedAlloc(size_t size, size_t alignment);
    virtual void Free(void* ptr);
    virtual bool GetAllocationSize(void *ptr, size_t &sizeOut);
    virtual void Trim(bool bTrimThreadCaches);
    virtual bool IsThreadSafe() const;
    virtual const char* GetDescriptiveName() const;
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_MIMALLOC_INCLUDE_JHSJDVGHBDF */
