#ifndef REFERENCE_INCLUDE
#define REFERENCE_INCLUDE

//���ü������ʵ��

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API Reference
{
public:
	Reference();

    ~Reference();
	
    //�������ø���
    uint32_t ReleaseReference();
	
    //�������ø���
    uint32_t AddReference();
    
    //������ø���
    uint32_t GetReferenceCount() const;

private:
    //���������4�ֽ�����
	volatile AlignedUint32 m_nRef;
};

NS_BASELIB_END

#endif
