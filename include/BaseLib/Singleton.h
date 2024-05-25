//
//  Singleton.h
//  BaseLib 单例模式模板基类
//
//  Created by zhouxuguang on 16/5/30.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_SINGLETON_INCLUDE_H
#define BASELIB_SINGLETON_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

template <typename T> class Singleton
{
protected:
	Singleton(const Singleton<T> &){}

	Singleton& operator=(const Singleton<T> &rhs)
	{
		return *this;
	}

    Singleton( void ){}

    ~Singleton( void ){}
    
public:
    static T& GetInstance( void )
    {
        static T sInstance;
        return sInstance;
    }
    static T* GetInstancePtr( void )
    {
        static T sInstance;
        return &sInstance;
    }
};

NS_BASELIB_END


#endif /* BASELIB_SINGLETON_INCLUDE_H */
