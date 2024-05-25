#ifndef ATOMIC_OPS_INCLUDE_H
#define ATOMIC_OPS_INCLUDE_H

//跨平台原子操作封装函数

#include "PreCompile.h"

NS_BASELIB_BEGIN

/**
 *  原子操作函数，加，内存地址必须4字节对齐
 *
 *  @param ptrValue   指向的值
 *  @param nIncrement 增加的数
 *  @return 返回操作之后的新值
 */
int BASELIB_API CBLAtomicAdd(volatile int* ptrValue, int nIncrement);

/**
 *  原子操作函数，加1操作
 *  @param ptrValue   指向的值
 *  @return 返回操作之后的新值
 */
int BASELIB_API CBLAtomicIncrement(volatile int* ptrValue);

/**
 *  原子操作函数，减1操作
 *  @param ptrValue   指向的值
 *  @return 返回操作之后的新值
 */
int BASELIB_API CBLAtomicDecrement(volatile int* ptrValue);

/**
 *  原子CAS操作 内存地址必须4字节对齐
 *  @param ptrValue  指向的值
 *  @param nExchange 交换的值。新值
 *  @param nCompare  比较的值，旧值
 *  @return 返回修改之前的值
 */
int BASELIB_API CBLAtomicCompareSwap(volatile int* ptrValue, int nExchange,int nCompare);

/**
 *  原子CAS操作(操作指针)
 *  @param theValue  指向的二级指针
 *  @param oldValue 交换的值。旧值
 *  @param newValue  比较的值，新值
 *  @return 返回是否成功
 */
bool BASELIB_API CBLAtomicCompareAndSwapPtr( void *oldValue, void *newValue, void * volatile *theValue );

/**
 *  原子交换指针操作(操作指针)
 *  @param oneValue
 *  @param otherValue
 *  @return 返回是否成功
 */
bool BASELIB_API CBLAtomicSwapPtr( void *volatile *oneValue, void *volatile *otherValue );


NS_BASELIB_END

#endif
