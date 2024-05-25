//
//  JenkinsHash.cpp
//  baselib
//
//  Created by zhouxuguang on 16/7/20.
//  Copyright © 2016年 . All rights reserved.
//

#include "JenkinsHash.h"

namespace baselib 
{
    
#ifdef __clang__
    __attribute__((no_sanitize("integer")))
#endif
    uint32_t JenkinsHashWhiten(uint32_t hash)
    {
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

uint32_t JenkinsHashMix(uint32_t hash, uint32_t data)
{
    hash += data;
    hash += (hash << 10);
    hash ^= (hash >> 6);
    return hash;
}

uint32_t JenkinsHashMixBytes(uint32_t hash, const uint8_t* bytes, size_t size)
{
    hash = JenkinsHashMix(hash, (uint32_t)size);
    size_t i;
    for (i = 0; i < (size & -4); i += 4)
    {
        uint32_t data = bytes[i] | (bytes[i+1] << 8) | (bytes[i+2] << 16) | (bytes[i+3] << 24);
        hash = JenkinsHashMix(hash, data);
    }
    if (size & 3)
    {
        uint32_t data = bytes[i];
        data |= ((size & 3) > 1) ? (bytes[i+1] << 8) : 0;
        data |= ((size & 3) > 2) ? (bytes[i+2] << 16) : 0;
        hash = JenkinsHashMix(hash, data);
    }
    return hash;
} 
    
}
