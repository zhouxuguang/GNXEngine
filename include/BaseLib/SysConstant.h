/**
* @file              SysConstant.h
* @brief             基本宏定义等
* @details           
* @author            ����� (zhouxuguang@.com)
* @date              2015��12��4��
* @version           1.0.0.1
* @par               Copyright (c): 2015
* @par               History:
*/
#ifndef __SYSCONSTANT_H_47406346_D7ED_4C5A_B209_F6D31AE07D92__
#define __SYSCONSTANT_H_47406346_D7ED_4C5A_B209_F6D31AE07D92__


// Microsoft Visual Studio versions:
//  MSVC  6   is version 12.00
//  MSVC  7.0 is version 13.00 (MSVS 2002)
//  MSVC  7.1 is version 13.10 (MSVS 2003)
//  MSVC  8.0 is version 14.00 (MSVS 2005)
//  MSVC  9.0 is version 15.00 (MSVS 2008)
//  MSVC 10.0 is version 16.00 (MSVS 2010)
//  MSVC 11.0 is version 17.00 (MSVS 2012)

/*基本数据类型*/

#if (defined _WIN32 ||  defined _WIN64) && defined (_MSC_VER) || defined __CYGWIN__
	typedef __int64				 int64_t;
	typedef unsigned __int64     uint64_t;
	typedef signed   __int32     int32_t;
	typedef unsigned __int32     uint32_t;
	typedef signed   __int16     int16_t;
	typedef unsigned __int16     uint16_t;
    typedef signed char          int8_t;
    typedef unsigned char        uint8_t;

#endif

//�ֽ���
enum ByteOrder
{
	BL_LITTLE_ENDIAN = 0,		//Сβ��
	BL_BIG_ENDIAN    = 1		//��ΰ��
};


//���廡�ȺͽǶ�֮���໥ת����ϵ��
#ifdef DEGTORAD
	#undef DEGTORAD
#else
	#define DEGTORAD		0.0174532925199432958			//�Ƕ�תΪ���ȵ�ϵ��
#endif

#ifdef RADTODEG
	#undef RADTODEG
#else
	#define RADTODEG		57.29577951308232			//����ת��Ϊ�Ƕȵ�ϵ��
#endif

#ifdef FLOAT_ZERO
	#undef FLOAT_ZERO
#else
	#define FLOAT_ZERO		1e-10			
#endif


//跨平台目录分割符
#if defined _WIN32 || defined _WIN64
	#define PATHSPLIT '\\'
#elif defined __linux__ || (defined(__MACH__) && defined(__APPLE__) )
	#define PATHSPLIT '/'
#endif

//跨平台字节对齐
#if defined _WIN32 || defined _WIN64
    #define BASELIB_ALIGNED(x) __declspec(align(x))
#elif defined __linux__ || (defined(__MACH__) && defined(__APPLE__) )
    #define BASELIB_ALIGNED(x) __attribute__(__aligned__(x)))
#endif


typedef void* ThreadHandle;
typedef void* MutexHandle;

//跨平台线程定义
typedef unsigned int thread_id_t;
#ifdef __linux__
#include <pthread.h>
#include <semaphore.h>
    typedef pthread_t thread_handle_t;
    typedef struct Mutex_Type
    {
        pthread_mutex_t mutex_t;
        pthread_mutexattr_t mutex_attr_t;
    }mutex_lock_t;
    typedef sem_t blsemaphore_t;
    typedef pthread_cond_t condition_t;
    typedef pthread_rwlock_t rwlock_t;
    typedef volatile  int32_t spin_lock_t;

#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#include <libkern/OSSpinLockDeprecated.h>
#include <pthread.h>
    typedef pthread_t thread_handle_t;
    typedef struct Mutex_Type
    {
        pthread_mutex_t mutex_t;
        pthread_mutexattr_t mutex_attr_t;
    }mutex_lock_t;
    typedef dispatch_semaphore_t blsemaphore_t;
    typedef pthread_cond_t condition_t;
    typedef pthread_rwlock_t rwlock_t;
    typedef volatile OSSpinLock spin_lock_t;

#elif defined WIN32
#include <windows.h>
    typedef uintptr_t thread_handle_t;
    typedef CRITICAL_SECTION mutex_lock_t;
    typedef HANDLE blsemaphore_t;
    typedef SRWLOCK rwlock_t;
	typedef volatile long spin_lock_t;

	
#if _WIN32_WINNT >= 0x0600
	typedef CONDITION_VARIABLE condition_t;
#else

	typedef struct condition_variable_with_event
	{
		HANDLE event;	
		CRITICAL_SECTION mutex;
		int n_waiters;
		int release_count;
		int epoch;
	}condition_variable_with_event;
	typedef condition_variable_with_event condition_t;
#endif

#endif

//线程一次调用的相关类型
#ifdef WIN32 
    typedef volatile long thread_once_t;
    #define THREAD_INIT_VAL 0
#else
    typedef pthread_once_t thread_once_t;
    #define THREAD_INIT_VAL PTHREAD_ONCE_INIT
#endif



//��ʱ����,����Ҫ�ع�
#if defined _WIN32 || defined _WIN64
    #include <process.h>
    #include <Windows.h>

    //Windows ����С˯�߾���
    #define MIN_SLEEP 10

#elif defined __linux__ || (defined(__MACH__) && defined(__APPLE__) )
    #include <pthread.h>

    #define MIN_SLEEP 1
#endif

#endif // end of __SYSCONSTANT_H_47406346_D7ED_4C5A_B209_F6D31AE07D92__
