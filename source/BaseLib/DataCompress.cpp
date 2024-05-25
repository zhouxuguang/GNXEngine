#include "DataCompress.h"
#include <zlib.h>

//#include "lzma/LzmaLib.h"
#include "lz4/lz4.h"


NS_BASELIB_BEGIN

unsigned char szProp[5];
size_t oSize = 5;

#if 1

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

bool DataUnCompress(const void* pSrcData,size_t nLen,void* pDstData,size_t* pOutLen,COMPRESS_TYPE eType)
{
	switch (eType)
	{
	case COMPRESS_GZIP:
		{
			uLongf nDestSize = 0;
			int nErr = uncompress((Bytef *)pDstData, &nDestSize, (const Bytef*)pSrcData, nLen);
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
			//unsigned char szProp[5];
			//szProp,oSize ������ѹ��ǰ������
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

#endif


NS_BASELIB_END
