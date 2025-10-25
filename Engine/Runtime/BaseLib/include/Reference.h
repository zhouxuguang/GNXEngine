#ifndef REFERENCE_INCLUDE
#define REFERENCE_INCLUDE

//引用计数

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API Reference
{
public:
	Reference();

    ~Reference();
	
    //引用计数减1
    uint32_t ReleaseReference();
	
    //引用计数+1
    uint32_t AddReference();
    
    //获得当前的引用计数
    uint32_t GetReferenceCount() const;

private:
    //引用计数
	volatile AlignedUint32 m_nRef;
};

NS_BASELIB_END

#endif
