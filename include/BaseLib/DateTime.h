//
//  DateTime.h
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/3.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#ifndef BASELIB_DATATIME_INCLUDE_KFNDSMSDFM4DFR3
#define BASELIB_DATATIME_INCLUDE_KFNDSMSDFM4DFR3

#include "PreCompile.h"

NS_BASELIB_BEGIN

//获得从1970年1月1日凌晨到现在的UTC时间(经过的毫秒数)
BASELIB_API double GetTimeSince1970();

//获得纳秒的"嘀嗒"数
BASELIB_API uint64_t GetTickNanoSeconds();

NS_BASELIB_END

#endif /* BASELIB_DATATIME_INCLUDE_KFNDSMSDFM4DFR3 */
