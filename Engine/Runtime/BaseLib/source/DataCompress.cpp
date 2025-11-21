#include "DataCompress.h"

#if OS_WINDOWS
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
	return size_t();
}

BASELIB_API size_t UnCompressBound(const void* pSrcData, size_t nLen, COMPRESS_TYPE eType)
{
	size_t uncompressedSize = 0;
	switch (eType)
	{
	case COMPRESS_GZIP:
	{
		uncompressedSize = estimate_gzip_uncompressed_size((const uint8_t*)pSrcData, nLen);
	}
	break;

	case COMPRESS_LZMA:
	{
	}

	break;

	case COMPRESS_LZ4:
	{
	}

	break;

	default:
		break;
	}
	return uncompressedSize;
}

BASELIB_API bool DataCompress(const void* pSrcData, size_t nLen, void* pDstData, size_t* pOutLen, COMPRESS_TYPE eType)
{
	switch (eType)
	{
	case COMPRESS_GZIP:
		{
			uLongf nDestSize = 0;
			int nErr = compress((Bytef *)pDstData,&nDestSize,(const Bytef*)pSrcData,nLen);
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
		{
			
			/*if (SZ_OK != LzmaCompress((unsigned char*)pDstData,pOutLen,(const uint8_t*)pSrcData,nLen,
				szProp,&oSize,9,(1<<24),3,0,2,32,2))
			{
				*pOutLen = 0;
				printf("shibai\n");
				return 0;
			}*/

			return true;
			
		}

		break;

	case COMPRESS_LZ4:
		{
			int nMaxSize = nLen;
			*pOutLen = LZ4_compress_fast((const char*)pSrcData,(char*)pDstData,nLen,nMaxSize,1);
			if (*pOutLen == 0)
			{
				return false;
			}
		}

		break;

	default:
		*pOutLen = 0;
		return 0;
		break;
	}
	
	return 1;
}

bool DataUnCompress(const void* pSrcData, size_t nLen, void* pDstData, size_t* pOutLen, COMPRESS_TYPE eType)
{
	switch (eType)
	{
	case COMPRESS_GZIP:
	{
		mini_gzip gzip;
		mini_gz_init(&gzip);
		mini_gz_start(&gzip, (void*)pSrcData, nLen);
		size_t outLen = *pOutLen;
		mini_gz_unpack(&gzip, pDstData, outLen);

		//z_stream strm = {};

		//// 1. 初始化 zlib 流并指定 gzip 格式
		//if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
		//{
		//	return Z_ERRNO; // 初始化失败
		//}

		//strm.next_in = (Bytef*)pSrcData;
		//strm.avail_in = nLen;
		//strm.avail_out = *pOutLen;
		//strm.next_out = (Bytef*)pDstData;


		//int ret = Z_OK;
		//do 
		//{
		//	ret = inflate(&strm, Z_NO_FLUSH);
		//	if (ret < 0) break; // 出错
		//} while (ret != Z_STREAM_END);

		//// 3. 清理资源
		//inflateEnd(&strm);

		//if (ret == Z_STREAM_END) 
		//{
		//	*pOutLen = strm.total_out; // 实际解压大小
		//	return true;
		//}

		//return false;

		/*uLongf nDestSize = 0;
		int nErr = uncompress((Bytef*)pDstData, &nDestSize, (const Bytef*)pSrcData, nLen);
		if (nErr != Z_OK)
		{
			*pOutLen = 0;
			return false;
		}
		*pOutLen = nDestSize;*/

		return true;
	}
	break;

	case COMPRESS_LZMA:
		{
			//unsigned char szProp[5];
			//szProp,oSize
			/*if (SZ_OK != LzmaUncompress((unsigned char*)pDstData,pOutLen,(const uint8_t*)pSrcData,&nLen,szProp,oSize))
			{
				*pOutLen = 0;
				printf("shibai\n");
				return 0;
			}*/

			return true;
			
		}

		break;

	case COMPRESS_LZ4:
		{
			//int LZ4_decompress_fast (const char* source, char* dest, int originalSize);
			int nFlag = LZ4_decompress_fast((const char*)pSrcData,(char*)pDstData,nLen);
			if (0 == nFlag)
			{
				*pOutLen = 0;
				return false;
			}

			*pOutLen = nLen;
		}

		break;

	default:
		*pOutLen = 0;
		return 0;
		break;
	}
	
	return 1;
}

NS_BASELIB_END
