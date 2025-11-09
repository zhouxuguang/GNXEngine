//
//  Malloc.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#ifndef ALLOCATOR_MALLOC_INCLUDE_NSDJJKDSGVJ_H
#define ALLOCATOR_MALLOC_INCLUDE_NSDJJKDSGVJ_H

#include "AllocatorDefine.h"

NS_ALLOCATOR_BEGIN

class UseSystemMallocForNew
{
public:
    void* operator new(size_t size);

    void operator delete(void* ptr);

    void* operator new[](size_t size);

    void operator delete[](void* ptr);
};

class Malloc : public UseSystemMallocForNew
{
public:	
	virtual void* Alloc(size_t size) = 0;
	virtual void* AlignedAlloc(size_t size, size_t alignment) = 0;
	virtual void Free(void* ptr) = 0;
	virtual bool GetAllocationSize(void *ptr, size_t &sizeOut) = 0;
	virtual void Trim(bool bTrimThreadCaches) = 0;
	virtual bool IsThreadSafe() const = 0;
	virtual const char* GetDescriptiveName() const = 0;
};

// 封装的内存分配类
class Memory
{
public:
    static void* Malloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
    static void Free(void* ptr);
    static size_t GetAllocSize(void* ptr);
};

NS_ALLOCATOR_END

#endif
