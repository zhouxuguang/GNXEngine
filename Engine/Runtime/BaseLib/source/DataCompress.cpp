#include "DataCompress.h"

#if GNX_OS_WINDOWS
#include "zlib.h"
#else
#include <zlib.h>
#endif
//#include "lzma/LzmaLib.h"
#include "lz4/lz4.h"

extern "C"
{
	#include "minigzip/mini_gzip.h"
}

NS_BASELIB_BEGIN

BASELIB_API size_t CompressBound(const void* pSrcData, size_t nLen, COMPRESS_TYPE eType)
{
	(void)pSrcData;
	switch (eType)
	{
	case COMPRESS_LZ4:
		return (size_t)LZ4_compressBound((int)nLen);

	case COMPRESS_GZIP:
		return compressBound((uLong)nLen);

	case COMPRESS_LZMA:
	case COMPRESS_7Z:
	default:
		// LZMA/7Z 暂不支持，返回 0 表示不可用
		return 0;
	}
}

BASELIB_API size_t UnCompressBound(const void* pSrcData, size_t nLen, COMPRESS_TYPE eType)
{
	switch (eType)
	{
	case COMPRESS_GZIP:
	{
		// gzip 尾部 4 字节（小端）保存了原始数据的长度（模 2^32）
		size_t uncompressedSize = estimate_gzip_uncompressed_size((const uint8_t*)pSrcData, nLen);
		return uncompressedSize;
	}

	case COMPRESS_LZMA:
	case COMPRESS_7Z:
	case COMPRESS_LZ4:
		// LZ4/LZMA 无法从压缩数据直接推断解压后大小，调用方需自行提供
		return 0;

	default:
		return 0;
	}
}

BASELIB_API bool DataCompress(const void* pSrcData, size_t nLen, void* pDstData, size_t* pOutLen, COMPRESS_TYPE eType)
{
	if (pSrcData == nullptr || pDstData == nullptr || pOutLen == nullptr)
	{
		return false;
	}

	switch (eType)
	{
	case COMPRESS_GZIP:
		{
			uLongf nDestSize = (uLongf)(*pOutLen);
			int nErr = compress((Bytef *)pDstData, &nDestSize, (const Bytef*)pSrcData, nLen);
			if (nErr != Z_OK)
			{
				*pOutLen = 0;
				return false;
			}
			*pOutLen = nDestSize;

			return true;
		}
		break;

	case COMPRESS_LZMA:
	case COMPRESS_7Z:
		// LZMA/7Z 暂未接入具体库，明确返回失败，避免误以为压缩成功
		*pOutLen = 0;
		return false;

	case COMPRESS_LZ4:
		{
			int nMaxSize = (int)(*pOutLen);
			int nCompressed = LZ4_compress_fast((const char*)pSrcData, (char*)pDstData, (int)nLen, nMaxSize, 1);
			if (nCompressed <= 0)
			{
				*pOutLen = 0;
				return false;
			}
			*pOutLen = (size_t)nCompressed;
			return true;
		}
		break;

	default:
		*pOutLen = 0;
		return false;
	}
}

bool DataUnCompress(const void* pSrcData, size_t nLen, void* pDstData, size_t* pOutLen, COMPRESS_TYPE eType)
{
	if (pSrcData == nullptr || pDstData == nullptr || pOutLen == nullptr)
	{
		return false;
	}

	switch (eType)
	{
	case COMPRESS_GZIP:
	{
		// 使用 zlib 的 inflateInit2(16+MAX_WBITS) 解压 gzip 流
		z_stream strm = {};
		if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
		{
			*pOutLen = 0;
			return false;
		}

		strm.next_in = (Bytef*)pSrcData;
		strm.avail_in = (uInt)nLen;
		strm.next_out = (Bytef*)pDstData;
		strm.avail_out = (uInt)(*pOutLen);

		int ret = inflate(&strm, Z_FINISH);
		if (ret != Z_STREAM_END)
		{
			inflateEnd(&strm);
			*pOutLen = 0;
			return false;
		}

		*pOutLen = strm.total_out;
		inflateEnd(&strm);
		return true;
	}
	break;

	case COMPRESS_LZMA:
	case COMPRESS_7Z:
		// LZMA/7Z 暂未接入具体库，明确返回失败
		*pOutLen = 0;
		return false;

	case COMPRESS_LZ4:
		{
			int nDecompressed = LZ4_decompress_safe((const char*)pSrcData, (char*)pDstData, (int)nLen, (int)(*pOutLen));
			if (nDecompressed < 0)
			{
				*pOutLen = 0;
				return false;
			}
			*pOutLen = (size_t)nDecompressed;
			return true;
		}
		break;

	default:
		*pOutLen = 0;
		return false;
	}
}

NS_BASELIB_END
