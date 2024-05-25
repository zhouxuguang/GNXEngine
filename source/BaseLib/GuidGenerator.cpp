#include "GuidGenerator.h"
#include "Random.h"

NS_BASELIB_BEGIN

#define GUIDFormatString "%08X-%04X-%04X-%08X-%08X"

#define GUIDStringLength 36

#if !defined(WIN32) && !defined(_WIN64)

#ifndef __ANDROID__
    #include <uuid/uuid.h>
    #include <sys/time.h>
#endif

#elif defined WIN32

#include <sys/timeb.h>

#define snprintf sprintf_s  

#endif

#if !defined(WIN32) && !defined(_WIN64)
struct GUID
{
    uint32_t Data1;
     uint16_t Data2;
     uint16_t Data3;
    uint8_t  Data4[8];
};
#endif


class BASELIB_API_HIDE GUIDGenerator
{
public:
	static uint32_t BytesToUInt32(const uint8_t bytes[]) 
	{
		return ((uint32_t) bytes[0]
		    | ((uint32_t) bytes[1] << 8)
			| ((uint32_t) bytes[2] << 16)
			| ((uint32_t) bytes[3] << 24));
	}
    
    static  uint16_t BytesToUInt16(const uint8_t bytes[])
    {
        return ( uint16_t) bytes[0]
                | ( uint16_t) bytes[1] << 8;
    }

	static void UInt32ToBytes(uint8_t bytes[], uint32_t n) 
	{
		bytes[0] = n & 0xff;
		bytes[1] = (n >> 8) & 0xff;
		bytes[2] = (n >> 16) & 0xff;
		bytes[3] = (n >> 24) & 0xff;
	}

	static bool CreateGUID(GUID *guid) 
	{
        if (NULL == guid)
        {
            return false;
        }
        
        //使用真随机数接口实现
        guid->Data1 = GetRandom();
        guid->Data2 = ( uint16_t)(GetRandom(0,65535));
        guid->Data3 = ( uint16_t)(GetRandom(0,65535));
        UInt32ToBytes(&guid->Data4[0], GetRandom());
        UInt32ToBytes(&guid->Data4[4], GetRandom());
        return true;

	}
};

#ifdef WIN32

#include <Windows.h> 

bool CreateGUID(GUID *guid)
{
    if (NULL == guid)
    {
        return false;
    }
    
	return CoCreateGuid(guid) == S_OK;
}

#else
bool CreateGUID(GUID *guid)
{
    if (NULL == guid)
    {
        return false;
    }
    
#ifdef __ANDROID__
	return GUIDGenerator::CreateGUID(guid);
#else
    uuid_t uuid;
    uuid_generate_random(uuid);
    
    unsigned char *pUUID = uuid;
    
    guid->Data1 = GUIDGenerator::BytesToUInt32(pUUID);
    guid->Data2 = GUIDGenerator::BytesToUInt16(pUUID+4);
    guid->Data3 = GUIDGenerator::BytesToUInt16(pUUID+6);
    memcpy(guid->Data4, pUUID+8, 8 * sizeof(uint8_t));
    return true;
#endif
}

#endif

bool GUIDToString(const GUID *guid, std::string& bufStr)
{
    if (NULL == guid)
    {
        return false;
    }
    
	size_t nLen = GUIDStringLength;
	bufStr.resize(nLen+1);
	int num = sprintf((char *)bufStr.c_str(), GUIDFormatString,
		guid->Data1, guid->Data2, guid->Data3,
		GUIDGenerator::BytesToUInt32(&(guid->Data4[0])),
		GUIDGenerator::BytesToUInt32(&(guid->Data4[4])));

	if (num != GUIDStringLength)
		return false;

	bufStr[num] = '\0';
	return true;
}

bool IsGUIDEqual(const GUID &guid1,const GUID& guid2)
{
    return 0 == memcmp(&guid1, &guid2, sizeof(GUID));
}

NS_BASELIB_END

