//
//  RWLock.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/12/23.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "RWLock.h"

NS_BASELIB_BEGIN

#ifdef _WIN32

RWLock::RWLock()
{
	InitializeSRWLock(&mLock);
}

RWLock::~RWLock()
{
}

bool RWLock::RLock()
{
	AcquireSRWLockShared(&mLock);
	return true;
}

bool RWLock::RUnLock()
{
	ReleaseSRWLockShared(&mLock);
	return true;
}

bool RWLock::WLock()
{
	AcquireSRWLockExclusive(&mLock);
	return true;
}

bool RWLock::WUnLock()
{
	ReleaseSRWLockExclusive(&mLock);
	return true;
}

#else

RWLock::RWLock()
{
    pthread_rwlock_init(&mLock, NULL);
}

RWLock::~RWLock()
{
    pthread_rwlock_destroy(&mLock);
}

bool RWLock::RLock()
{
    return 0 == pthread_rwlock_rdlock(&mLock);
}

bool RWLock::RUnLock()
{
    return 0 == pthread_rwlock_unlock(&mLock);
}

bool RWLock::WLock()
{
    return 0 == pthread_rwlock_wrlock(&mLock);
}

bool RWLock::WUnLock()
{
    return 0 == pthread_rwlock_unlock(&mLock);
}

#endif

NS_BASELIB_END
