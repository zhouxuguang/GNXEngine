//
//  ThreadUtil.h 多线程辅助函数
//  BaseLib
//
//  Created by zhouxuguang on 16/12/27.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_THREAD_UTIL_INCLUDE_H_FLDJ87FKHS
#define BASELIB_THREAD_UTIL_INCLUDE_H_FLDJ87FKHS

#include "PreCompile.h"

NS_BASELIB_BEGIN

class Thread;

class BASELIB_API ThreadUtil
{
private:
    ThreadUtil();
    ~ThreadUtil();
    ThreadUtil(const ThreadUtil&);
    ThreadUtil& operator = (const ThreadUtil&);
    
public:
    /**
     *  获得当前线程ID号
     *
     *  @return 返回当前线程ID
     */
    static thread_handle_t GetCurrentID();
    
    /**
     *  睡眠
     *  @param nMiliSeconds 毫秒
     */
    static void Sleep(long long nMiliSeconds);
    
    /**
     *  让出当前线程，如果有其它的相同优先级的线程
     */
    static bool ThreadYield();
    
    /**
     *  检测当前是否是主线程
     *  @return
     */
    static bool IsMainThread();

	typedef void  (* OnceInitProc)(void);

	 /**
     *  任意个线程调用，只调用pInitFunc一次，保证某些资源只初始化一次
     *  @return
     */
	static bool ThreadOnceCall(thread_once_t nInitValue, OnceInitProc pInitFunc);
    
    /**
     * @brief 设置线程名字
     * @return void
     * @author zxg
     * @date 2015
     * @note
     */
    static void SetName(const char* pszName);
    
    /**
     * @brief Stop 获得当前调用线程名字
     * @return void
     * @author zxg
     * @date 2015
     * @note
     */
    static void GetName(char* pszName, int len);
};

NS_BASELIB_END

#endif /* BASELIB_THREAD_UTIL_INCLUDE_H_FLDJ87FKHS */
