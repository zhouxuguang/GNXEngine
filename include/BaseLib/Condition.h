//
//  Condition.h 跨平台条件变量的封装
//  BaseLib
//
//  Created by zhouxuguang on 16/8/2.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_CONDITION_INCLUDE_H
#define BASELIB_CONDITION_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

class MutexLock;

class BASELIB_API Condition
{
public:
    explicit Condition(MutexLock* lock);
    ~Condition();

    void Wait();
    void Wait(long long time);
    void Notify();
    void NotifyAll();
    
private:
    MutexLock* lock;
    condition_t condId;
};

NS_BASELIB_END

#endif /* BASELIB_CONDITION_INCLUDE_H */
