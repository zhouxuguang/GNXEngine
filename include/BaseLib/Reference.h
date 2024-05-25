#ifndef REFERENCE_INCLUDE
#define REFERENCE_INCLUDE

//引用计数类的实现

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API Reference
{
public:
	Reference();

    ~Reference();
	
    //减少引用个数
    uint32_t ReleaseReference();
	
    //增加引用个数
    uint32_t AddReference();
    
    //获得引用个数
    uint32_t GetReferenceCount() const;

private:
    //必须对齐在4字节上面
	volatile AlignedUint32 m_nRef;
};

NS_BASELIB_END

#endif
