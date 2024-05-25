//
//  RWLock.h
//  BaseLib
//
//  Created by zhouxuguang on 16/12/23.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_RWLOCK_INCLUDE_H_FDDLJH
#define BASELIB_RWLOCK_INCLUDE_H_FDDLJH

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API RWLock
{
public:
    RWLock();
    
    ~RWLock();
    
    //读锁定
    bool RLock();
    
    //读解锁
    bool RUnLock();
    
    //写锁定
    bool WLock();
    
    //写解锁
    bool WUnLock();
    
private:
    rwlock_t mLock;
};

NS_BASELIB_END

#endif /* BASELIB_RWLOCK_INCLUDE_H_FDDLJH */
