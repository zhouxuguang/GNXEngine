//
//  MallocTLSF.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/27.
//

#ifndef GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH
#define GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH

#include "AMalloc.h"
#include "tlsf.h"

NS_ALLOCATOR_BEGIN

class MallocTLSF : public Malloc
{
public:
    MallocTLSF();
    ~MallocTLSF();
	virtual void* Alloc(size_t size);
	virtual void* AlignedAlloc(size_t size, uint32_t alignment);
    virtual void Free(void* ptr);
    virtual bool GetAllocationSize(void *ptr, size_t &sizeOut);
    virtual void Trim(bool bTrimThreadCaches);
    virtual bool IsThreadSafe() const;
    virtual const char* GetDescriptiveName() const;

private:
    tlsf_t mTLSF = NULL;
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH */
