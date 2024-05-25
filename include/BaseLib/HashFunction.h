#ifndef BASELIB_HASHFUNCTION_INCLUDE_H
#define BASELIB_HASHFUNCTION_INCLUDE_H

/**
 *  哈希函数实现
 */

#include "PreCompile.h"

NS_BASELIB_BEGIN

BASELIB_API uint32_t HashFunction(const void* pKey, int nLen);

BASELIB_API uint32_t GetHashCode(const std::string& key);

template<typename KEY>
uint32_t GetHashCode(const KEY& key)
{
	return HashFunction(&key,sizeof(key));
}

NS_BASELIB_END

#endif
