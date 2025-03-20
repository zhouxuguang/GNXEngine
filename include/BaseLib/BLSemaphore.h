//
//  Semaphore.h 信号量的封装
//  BaseLib
//
//  Created by zhouxuguang on 16/6/18.
//  Copyright 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_SEMAPHORE_INCLUDE_H
#define BASELIB_SEMAPHORE_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API Semaphore
{
private:
    blsemaphore_t m_sem;
    
public:
    explicit Semaphore(int initValue);
    
    ~Semaphore();
    
    void wait();
    
    void post();
    
    bool tryWait();
    
    bool timedWait(unsigned int s, unsigned int ms);
};

NS_BASELIB_END

#endif /* BASELIB_SEMAPHORE_INCLUDE_H */
