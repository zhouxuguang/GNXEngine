#ifndef ATOMIC_OPS_INCLUDE_H
#define ATOMIC_OPS_INCLUDE_H

//原子操作封装

#include "PreCompile.h"

NS_BASELIB_BEGIN

/**
 *  ԭ�Ӳ����������ӣ��ڴ��ַ����4�ֽڶ���
 *
 *  @param ptrValue   ָ���ֵ
 *  @param nIncrement ���ӵ���
 *  @return ���ز���֮�����ֵ
 */
int BASELIB_API CBLAtomicAdd(volatile int* ptrValue, int nIncrement);

/**
 *  ԭ�Ӳ�����������1����
 *  @param ptrValue   ָ���ֵ
 *  @return ���ز���֮�����ֵ
 */
int BASELIB_API CBLAtomicIncrement(volatile int* ptrValue);

/**
 *  ԭ�Ӳ�����������1����
 *  @param ptrValue   ָ���ֵ
 *  @return ���ز���֮�����ֵ
 */
int BASELIB_API CBLAtomicDecrement(volatile int* ptrValue);

/**
 *  ԭ��CAS���� �ڴ��ַ����4�ֽڶ���
 *  @param ptrValue  ָ���ֵ
 *  @param nExchange ������ֵ����ֵ
 *  @param nCompare  �Ƚϵ�ֵ����ֵ
 *  @return �����޸�֮ǰ��ֵ
 */
int BASELIB_API CBLAtomicCompareSwap(volatile int* ptrValue, int nExchange,int nCompare);

/**
 *  ԭ��CAS����(����ָ��)
 *  @param theValue  ָ��Ķ���ָ��
 *  @param oldValue ������ֵ����ֵ
 *  @param newValue  �Ƚϵ�ֵ����ֵ
 *  @return �����Ƿ�ɹ�
 */
bool BASELIB_API CBLAtomicCompareAndSwapPtr( void *oldValue, void *newValue, void * volatile *theValue );

/**
 *  ԭ�ӽ���ָ�����(����ָ��)
 *  @param oneValue
 *  @param otherValue
 *  @return �����Ƿ�ɹ�
 */
bool BASELIB_API CBLAtomicSwapPtr( void *volatile *oneValue, void *volatile *otherValue );


NS_BASELIB_END

#endif
