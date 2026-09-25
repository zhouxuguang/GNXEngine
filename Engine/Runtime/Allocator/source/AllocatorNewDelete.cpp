#include "AMalloc.h"

#include <new>

namespace
{

#ifndef __STDCPP_DEFAULT_NEW_ALIGNMENT__
#define __STDCPP_DEFAULT_NEW_ALIGNMENT__ alignof(std::max_align_t)
#endif

constexpr size_t kDefaultNewAlignment = static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__);

inline void* NewOrThrow(size_t size, size_t alignment)
{
    void* ptr = Allocator::Memory::Malloc(size, alignment);
    if (!ptr)
    {
        return nullptr;
    }
    return ptr;
}

inline void* NewNoThrow(size_t size, size_t alignment) noexcept
{
    return Allocator::Memory::Malloc(size, alignment);
}

inline void Delete(void* ptr) noexcept
{
    Allocator::Memory::Free(ptr);
}

} // namespace

ALLOCATOR_API void* operator new(size_t size)
{
    return NewOrThrow(size, kDefaultNewAlignment);
}

ALLOCATOR_API void* operator new[](size_t size)
{
    return NewOrThrow(size, kDefaultNewAlignment);
}

ALLOCATOR_API void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return NewNoThrow(size, kDefaultNewAlignment);
}

ALLOCATOR_API void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return NewNoThrow(size, kDefaultNewAlignment);
}

ALLOCATOR_API void* operator new(size_t size, std::align_val_t alignment)
{
    return NewOrThrow(size, static_cast<size_t>(alignment));
}

ALLOCATOR_API void* operator new[](size_t size, std::align_val_t alignment)
{
    return NewOrThrow(size, static_cast<size_t>(alignment));
}

ALLOCATOR_API void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return NewNoThrow(size, static_cast<size_t>(alignment));
}

ALLOCATOR_API void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return NewNoThrow(size, static_cast<size_t>(alignment));
}

ALLOCATOR_API void operator delete(void* ptr) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete(void* ptr, size_t) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr, size_t) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete(void* ptr, std::align_val_t) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr, std::align_val_t) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete(void* ptr, size_t, std::align_val_t) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr, size_t, std::align_val_t) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete(void* ptr, size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
    Delete(ptr);
}

ALLOCATOR_API void operator delete[](void* ptr, size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
    Delete(ptr);
}
