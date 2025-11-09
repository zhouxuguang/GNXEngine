#include "AMalloc.h"
#include "MallocTBB.h"

NS_ALLOCATOR_BEGIN

void* UseSystemMallocForNew::operator new(size_t size)
{
	return ::malloc(size);
}

void UseSystemMallocForNew::operator delete(void* ptr)
{
	return ::free(ptr);
}

void* UseSystemMallocForNew::operator new[](size_t size)
{
	return ::malloc(size);
}

void UseSystemMallocForNew::operator delete[](void* ptr)
{
	::free(ptr);
}

static Malloc *gMalloc = nullptr;

static void InitGMalloc()
{
    gMalloc = new MallocTBB();
}

void* Memory::Malloc(size_t size, size_t alignment)
{
    if (!gMalloc)
    {
        InitGMalloc();
    }
    return gMalloc->AlignedAlloc(size, alignment);
}

void Memory::Free(void* ptr)
{
    if (!gMalloc)
    {
        InitGMalloc();
    }
    return gMalloc->Free(ptr);
}

size_t Memory::GetAllocSize(void* ptr)
{
    if (!gMalloc)
    {
        InitGMalloc();
    }
    size_t sizeOut = 0;
    gMalloc->GetAllocationSize(ptr, sizeOut);
    return sizeOut;
}

NS_ALLOCATOR_END

void* operator new(size_t size)
{
    return Allocator::Memory::Malloc(size ? size : 1, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void* operator new[](size_t size)
{
    return Allocator::Memory::Malloc(size ? size : 1, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return Allocator::Memory::Malloc(size ? size : 1, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return Allocator::Memory::Malloc(size ? size : 1, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void* operator new(size_t size, std::align_val_t alignment)
{
    return Allocator::Memory::Malloc(size ? size : 1, (std::size_t)alignment);
}

void* operator new[](size_t size, std::align_val_t alignment)
{
    return Allocator::Memory::Malloc(size ? size : 1, (std::size_t)alignment);
}

void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Allocator::Memory::Malloc(size ? size : 1, (std::size_t)alignment);
}

void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return Allocator::Memory::Malloc(size ? size : 1, (std::size_t)alignment);
}

void operator delete(void* ptr) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, size_t size) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, size_t size, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, size_t size, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, std::align_val_t alignment) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, std::align_val_t alignment) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, size_t size, std::align_val_t alignment) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, size_t size, std::align_val_t alignment) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete(void* ptr, size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}

void operator delete[](void* ptr, size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    Allocator::Memory::Free(ptr);
}
