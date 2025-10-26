#include "Malloc.h"

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

NS_ALLOCATOR_END
