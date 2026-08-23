//
//  MallocTLSF.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/27.
//

#ifndef GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH
#define GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH

#include "../include/AMalloc.h"
#include "tlsf.h"

NS_ALLOCATOR_BEGIN

class MallocTLSF : public Malloc
{
public:
    MallocTLSF();
    ~MallocTLSF();
	void* Alloc(size_t size) override;
	void* AlignedAlloc(size_t size, size_t alignment) override;
    void Free(void* ptr) override;
    bool GetAllocationSize(void *ptr, size_t &sizeOut) override;
    void Trim(bool bTrimThreadCaches) override;
    bool IsThreadSafe() const override;
    const char* GetDescriptiveName() const override;

private:
    tlsf_t mTLSF = NULL;
    std::vector<void*> mPools;  // 追踪所有分配的pool以便析构时释放
};

NS_ALLOCATOR_END

#endif /* GNXENGINE_MALLOC_TLSF_INCLUDE_FFGNFGNHMGH */
