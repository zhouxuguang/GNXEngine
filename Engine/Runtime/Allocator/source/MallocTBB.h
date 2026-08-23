//
//  MallocTBB.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#ifndef GNXENGINE_MALLOC_TBB_INCLUDE_JSDVSDJKM
#define GNXENGINE_MALLOC_TBB_INCLUDE_JSDVSDJKM

#include "../include/AMalloc.h"

NS_ALLOCATOR_BEGIN

class MallocTBB : public Malloc
{
public:
    MallocTBB();
	void* Alloc(size_t size) override;
	void* AlignedAlloc(size_t size, size_t alignment) override;
    void Free(void* ptr) override;
    bool GetAllocationSize(void *ptr, size_t &sizeOut) override;
    void Trim(bool bTrimThreadCaches) override;
    bool IsThreadSafe() const override;
    const char* GetDescriptiveName() const override;
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_TBB_INCLUDE_JSDVSDJKM */
