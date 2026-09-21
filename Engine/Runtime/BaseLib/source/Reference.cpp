#include "Reference.h"
#include "AtomicOps.h"

NS_BASELIB_BEGIN

Reference::Reference():m_nRef(1)
{
	
}

Reference::~Reference()
{
}

uint32_t Reference::ReleaseReference()
{
	volatile int* reference = reinterpret_cast<volatile int*>(&m_nRef);
	int current = *reference;
	while (current > 0)
	{
		const int desired = current - 1;
		const int previous = CBLAtomicCompareSwap(reference, desired, current);
		if (previous == current)
			return static_cast<uint32_t>(desired);
		current = previous;
	}

	// 已经为 0 时保持饱和，避免并发 Release 下溢成 UINT_MAX。
	return 0;
}

uint32_t Reference::AddReference()
{
	CBLAtomicIncrement((volatile int*)(&m_nRef));
	return m_nRef;
}

uint32_t Reference::GetReferenceCount() const
{
    return this->m_nRef;
}

NS_BASELIB_END
