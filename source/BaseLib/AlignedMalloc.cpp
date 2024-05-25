//
//  AlignedMalloc.cpp
//  BASELIB
//
//  Created by zhouxuguang on 16/12/9.
//  Copyright 漏 2016骞� zhouxuguang. All rights reserved.
//

#include "AlignedMalloc.h"

NS_BASELIB_BEGIN

void* Aligned_Malloc(size_t size, size_t align)
{
    void *result = NULL;
#ifdef _MSC_VER
    result = _aligned_malloc(size, align);
#elif defined(__MACH__) || defined(__APPLE__ )
    if (posix_memalign(&result, align, size))
    {
        result = NULL;
    }
#elif __linux__
    result = memalign(align,size);
#endif
    return result;
}

void Aligned_Free(void *ptr)
{
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

NS_BASELIB_END
