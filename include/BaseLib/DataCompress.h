#ifndef BASELIB_DATACOMPRESS_INCLUDE_H_KF45G6HKL
#define BASELIB_DATACOMPRESS_INCLUDE_H_KF45G6HKL

//数据压缩和解压缩的接口

#include "PreCompile.h"

NS_BASELIB_BEGIN

enum COMPRESS_TYPE
{
	COMPRESS_LZ4,
	COMPRESS_GZIP,
	COMPRESS_7Z,
	COMPRESS_LZMA
};

BASELIB_API bool DataCompress(const void* pSrcData,size_t nLen,void* pDstData,size_t* pOutLen,COMPRESS_TYPE eType);

BASELIB_API bool DataUnCompress(const void* pSrcData,size_t nLen,void* pDstData,size_t* pOutLen,COMPRESS_TYPE eType);

NS_BASELIB_END

#endif
