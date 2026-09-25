//
//  Malloc.h
//  Allocator
//
//  Created by zhouxuguang on 2025/10/26.
//

#ifndef ALLOCATOR_MALLOC_INCLUDE_NSDJJKDSGVJ_H
#define ALLOCATOR_MALLOC_INCLUDE_NSDJJKDSGVJ_H

#include <new>
#include <cstdint>

#include "AllocatorDefine.h"

NS_ALLOCATOR_BEGIN

class ALLOCATOR_API UseSystemMallocForNew
{
public:
    void* operator new(size_t size);
    void* operator new[](size_t size);

    void* operator new(size_t size, const std::nothrow_t&) noexcept;
    void* operator new[](size_t size, const std::nothrow_t&) noexcept;

    void* operator new(size_t size, std::align_val_t alignment);
    void* operator new[](size_t size, std::align_val_t alignment);

    void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
    void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;

    void operator delete(void* ptr) noexcept;
    void operator delete[](void* ptr) noexcept;

    void operator delete(void* ptr, const std::nothrow_t&) noexcept;
    void operator delete[](void* ptr, const std::nothrow_t&) noexcept;

    void operator delete(void* ptr, size_t size) noexcept;
    void operator delete[](void* ptr, size_t size) noexcept;

    void operator delete(void* ptr, std::align_val_t alignment) noexcept;
    void operator delete[](void* ptr, std::align_val_t alignment) noexcept;

    void operator delete(void* ptr, size_t size, std::align_val_t alignment) noexcept;
    void operator delete[](void* ptr, size_t size, std::align_val_t alignment) noexcept;

    void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;
    void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept;

    void operator delete(void* ptr, size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
    void operator delete[](void* ptr, size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
};

class ALLOCATOR_API Malloc : public UseSystemMallocForNew
{
public:
	virtual ~Malloc() = default;

	virtual void* Alloc(size_t size, size_t* usableSizeOut = nullptr) = 0;
	virtual void* AlignedAlloc(size_t size, size_t alignment, size_t* usableSizeOut = nullptr) = 0;

	virtual bool FreeAndGetSize(void* ptr, size_t& usableSizeOut) = 0;

	void Free(void* ptr);

	virtual bool GetAllocationSize(void *ptr, size_t &sizeOut) = 0;
	virtual bool OwnsPointer(const void* ptr) const = 0;

	virtual void Trim(bool bTrimThreadCaches) = 0;
	virtual bool IsThreadSafe() const = 0;
	virtual const char* GetDescriptiveName() const = 0;
};

using MallocPtr = Malloc*;

struct MemoryStats
{
	uint64_t allocationCount = 0;
	uint64_t freeCount = 0;
	uint64_t liveBlockCount = 0;
	uint64_t liveBytes = 0;
	uint64_t peakLiveBytes = 0;
	uint64_t foreignFreeCount = 0;
};

// 封装的内存分配类
class ALLOCATOR_API Memory
{
public:
    static void* Malloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
    static void Free(void* ptr);
    static size_t GetAllocSize(void* ptr);

    static void SetMalloc(MallocPtr allocator);
    static MallocPtr GetMalloc();

    static const char* GetBackendName();
    static void Trim(bool bTrimThreadCaches);

    static MemoryStats GetStats();
    static void ResetStats();
    static void LogBackendInfo();
    static void LogStats(const char* tag);
};

NS_ALLOCATOR_END

#endif
