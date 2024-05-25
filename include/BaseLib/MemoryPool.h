//
//  MemoryPool.h
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/7/9.
//  Copyright © 2017年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_MEMORYPOOL_INCLUDE_H_DKNFJN464G
#define BASELIB_MEMORYPOOL_INCLUDE_H_DKNFJN464G

//内存池实现

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API MemoryPool
{
public:
    static void* Allocate(size_t size);
    
    static void* ReAllocate(void *mem, size_t size);
    
    static void* AlignedAllocate(size_t alignment, size_t bytes);
    
    static void Free(void *mem);
    
private:
    MemoryPool();
    ~MemoryPool();
    MemoryPool(const MemoryPool&);
    MemoryPool& operator = (const MemoryPool&);
};

NS_BASELIB_END

#endif /* BASELIB_MEMORYPOOL_INCLUDE_H_DKNFJN464G */
