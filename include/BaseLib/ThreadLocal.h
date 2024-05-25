//
//  ThreadLocal.h
//  BaseLib
//
//  Created by Zhou,Xuguang on 2018/1/21.
//  Copyright © 2018年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_THREADLOCAL_INCLIDE_H_FDNVJBNV88EINVB_H
#define BASELIB_THREADLOCAL_INCLIDE_H_FDNVJBNV88EINVB_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

typedef void (*DestroyFunc)(void*);

//线程局部存储
class BASELIB_API ThreadLocal
{
public:
    ThreadLocal(DestroyFunc pDesFunc = NULL);
    
    ~ThreadLocal();
    
    void set(void* value);
    
    void* get();
    
private:
    ThreadLocal(const ThreadLocal&);
    ThreadLocal& operator = (const ThreadLocal&);
    
    void *m_pThreadLocal;       //impl实现的指针
};

NS_BASELIB_END

#endif /* BASELIB_THREADLOCAL_INCLIDE_H_FDNVJBNV88EINVB_H */
