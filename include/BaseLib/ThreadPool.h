//
//  ThreadPool.h
//  BaseLib
//
//  Created by zhouxuguang on 16/9/28.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_THREADPOOL_INCLUDE_H_FFGD3FGGHR
#define BASELIB_THREADPOOL_INCLUDE_H_FFGD3FGGHR

#include "Thread.h"
#include "TaskRunner.h"
#include "Condition.h"
#include "MutexLock.h"

NS_BASELIB_BEGIN

class ThreadPoolIMPL;

class BASELIB_API ThreadPool
{
public:
    enum TaskStrategy
    {
        REMOVE_LAST = 0,
        REMOVE_FIRST = 1,
        NO_REMOVE = 2
    };
    
    ThreadPool();
    
    ThreadPool(uint32_t nThreadCount, uint32_t nMaxTaskCount = 0xffffffff);
    
    ~ThreadPool();
    
    void ShutDown();
    
    /**
     *  将任务放入任务队列，线程池从中取任务
     *
     *  @param task        任务
     *  @param strategy 策略类型
     */
    void Execute(const TaskRunnerPtr& task, TaskStrategy strategy = NO_REMOVE);
    
    /**
     *  获得队列中第一个线程
     */
    TaskRunnerPtr GetHeadTask();
    
    /**
     *  启动函数
     */
    void Start();
    
    /**
     *  线程池是否在运行
     */
    bool IsRunning() const;
    
    /**
     *  线程池中线程的个数
     */
    int GetThreadCount() const;
    
    /**
     *  线程池中还没执行的任务个数
     */
    int GetTaskCount() const;
    
private:
    std::shared_ptr<ThreadPoolIMPL> mIMPL = nullptr;
};

NS_BASELIB_END

#endif /* BASELIB_THREADPOOL_INCLUDE_H_FFGD3FGGHR */
