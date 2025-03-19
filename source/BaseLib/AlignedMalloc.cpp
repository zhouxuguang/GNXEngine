//
//  AlignedMalloc.cpp
//  BASELIB
//
//  Created by zhouxuguang on 16/12/9.
//  Copyright@ 2016年 zhouxuguang. All rights reserved.
//

#include "AlignedMalloc.h"
#include <assert.h>

NS_BASELIB_BEGIN

void* AlignedMalloc(size_t size, size_t alignment)
{
	assert(size > 0);
    assert((alignment & (alignment - 1)) == 0);
    assert((alignment % sizeof(void*)) == 0);

    void *result = NULL;
#ifdef _WIN32
    result = _aligned_malloc(size, alignment);
#elif defined(__MACH__) || defined(__APPLE__ )
    if (posix_memalign(&result, alignment, size))
    {
        result = NULL;
    }
#elif __linux__
    result = memalign(alignment, size);
#endif

	if (!result)
	{
		assert(false);
	}
    assert((reinterpret_cast<uintptr_t>(result) & (alignment - 1)) == 0);

    return result;
}

void AlignedFree(void *ptr)
{
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

NS_BASELIB_END
