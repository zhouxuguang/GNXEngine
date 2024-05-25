//
//  AlignedMalloc.h
//  BASELIB
//
//  Created by zhouxuguang on 16/12/9.
//  Copyright 2016 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_ALIGNED_MALLOC_INCLUDE_H
#define BASELIB_ALIGNED_MALLOC_INCLUDE_H

#include "PreCompile.h"

//跨平台对齐内存分配和释放函数

NS_BASELIB_BEGIN

BASELIB_API void* Aligned_Malloc(size_t size, size_t align);

BASELIB_API void Aligned_Free(void *ptr);

NS_BASELIB_END

#endif /* BASELIB_ALIGNED_MALLOC_INCLUDE_H */
