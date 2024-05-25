#ifndef BASELIB_TIMECOST_INCLUDE_H
#define BASELIB_TIMECOST_INCLUDE_H


#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API TimeCost
{
public:
	TimeCost(void);

	~TimeCost(void);

	void Begin();

	void End();

    //获得的时间是微秒
    uint64_t GetCostTime() const;
    
    //获得的时间是纳秒
    uint64_t GetCostTimeNano() const;

private:
    uint64_t m_nStart;
    uint64_t m_nEnd;
};

NS_BASELIB_END

#endif
