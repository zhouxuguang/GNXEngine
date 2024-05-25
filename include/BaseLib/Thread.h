/**
* @file              Thread.h
* @brief             跨平台线程定义
* @details           
* @author            zxg (zhouxuguang236@126.com)
* @date              2015年5月14日
* @version           1.0.0.1
* @par               Copyright (c):周旭光 2015
* @par               History:
*/
#ifndef __SYSTHREAD_H_47709FE9_0B67_4F2C_9448_9DC850E62468__
#define __SYSTHREAD_H_47709FE9_0B67_4F2C_9448_9DC850E62468__

#include "PreCompile.h"

NS_BASELIB_BEGIN

//线程回调函数定义
typedef void* (* ThreadFunc)(void* pPara);

enum ThreadState
{
    Thread_Exit = 0,        //线程退出
    Thread_Running,         //正在运行
    Thread_Resumed,         //挂起
    Thread_NotFound         //没有发现
};

//Thread只是提供线程类的接口，真正使用需要继承它实现Run方法
class BASELIB_API Thread
{
public:
	friend class ThreadUtil;
    
    Thread();

	explicit Thread(bool bSuspend, uint32_t unStackSize = 0);

	~Thread();
    
    //设置线程的栈大小，只有在线程还未启动的时候才有效
    void SetStackSize(uint32_t unStackSize);
    
    //获取线程栈的大小
    uint32_t GetStackSize() const;

	/**
	* @brief Start 开启线程，自己传入回调函数去处理
	* @param[in] ThreadFunc pFunc
	* @param[in] void* pArgs
	* @return void
	* @author zxg
	* @date 2015年6月15日
	* @note 
	*/
	bool Start(ThreadFunc pFunc,void* pArgs);
    
    /**
     * @brief Start 开启线程，交由Run函数去处理
     * @return void
     * @author zxg
     * @date 2015年6月15日
     * @note 
     */
    bool Start();

	/**
	* @brief Stop 关闭线程，但是线程函数可能还在继续运行
	* @return void
	* @author zxg
	* @date 2015年6月15日
	* @note 
	*/
	void Stop();

    /**
    * @brief Stop 恢复挂起的线程
    * @return void
    * @author zxg
    * @date 2015年6月15日
    * @note
    */
	void Resume();
    
    /**
     * @brief Stop 挂起线程
     * @return void
     * @author zxg
     * @date 2015年6月15日
     * @note
     */
    void Suspend();
    
    /**
     * @brief GetThreadID 获得线程ID号
     * @return thread_id_t 线程ID号
     * @author zxg
     * @date 2015年6月15日
     * @note 
     */
    thread_id_t GetThreadID() const;
    
    /**
     * @brief GetThreadState 获得线程状态
     * @return ThreadState 线程状态
     * @author zxg
     * @date 2015年6月15日
     * @note
     */
    ThreadState GetThreadState() const;
    
    /**
     * @brief WaitFor 等待此线程结束
     * @return bool
     * @author zxg
     * @date 2015年6月15日
     * @note
     */
    bool WaitFor() const;
    
    /**
     * @brief Run 线程运行函数，子类可以根据需要重写该函数
     * @return void
     * @author zxg
     * @date 2015年6月15日
     * @note
     */
    virtual void Run() const {};

private:    
    ThreadHandle   m_Handle;            //线程句柄
    uint32_t        m_unStackSize;              // 线程栈大小
	bool			m_bCreateSuspend;	//创建的时候是否挂起

};

NS_BASELIB_END

#endif // end of __SYSTHREAD_H_47709FE9_0B67_4F2C_9448_9DC850E62468__
