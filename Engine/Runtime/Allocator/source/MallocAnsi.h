//
//  MallocAnsi.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#ifndef GNXENGINE_MALLOC_ANSI_INCLUDE_DSFJSDJK
#define GNXENGINE_MALLOC_ANSI_INCLUDE_DSFJSDJK

#include "../include/AMalloc.h"

NS_ALLOCATOR_BEGIN

class MallocAnsi : public Malloc
{
public:
    MallocAnsi();
	virtual void* Alloc(size_t size);
	virtual void* AlignedAlloc(size_t size, size_t alignment);
    virtual void Free(void* ptr);
    virtual bool GetAllocationSize(void *ptr, size_t &sizeOut);
    virtual void Trim(bool bTrimThreadCaches);
    virtual bool IsThreadSafe() const;
    virtual const char* GetDescriptiveName() const;
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_ANSI_INCLUDE_DSFJSDJK */
