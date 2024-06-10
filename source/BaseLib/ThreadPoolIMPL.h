//
//  ThreadPoolIMPL.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/29.
//

#ifndef BASELIB_THREADPOOL_IMPL_INCLUDE_H_FFGD3FGGHR
#define BASELIB_THREADPOOL_IMPL_INCLUDE_H_FFGD3FGGHR


#include <thread>
#include <deque>
#include <vector>
#include "TaskRunner.h"
#include "MutexLock.h"
#include "Condition.h"
#include "ThreadPool.h"

namespace baselib {

class ThreadPoolIMPL : public std::enable_shared_from_this<ThreadPoolIMPL>
{
public:
    ThreadPoolIMPL();
    
    ThreadPoolIMPL(uint32_t nThreadCount, uint32_t nMaxTaskCount = 0xffffffff);
    
    ~ThreadPoolIMPL();
    
    void ShutDown();
    
    /**
     *  将任务放入任务队列，线程池从中取任务
     *
     *  @param task        任务
     *  @param strategy 策略类型
     */
    void Execute(const TaskRunnerPtr& task, ThreadPool::TaskStrategy strategy);
    
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
    int GetThreadCount();
    
    /**
     *  线程池中还没执行的任务个数
     */
    int GetTaskCount();
    
    /**
     *  取消没有执行的任务
     */
    void CancelAllTasks();
    
private:
    typedef std::vector<std::thread> ThreadQueue;
    ThreadQueue mThreads;
    int mThreadCount;
    
    typedef std::deque< TaskRunnerPtr > TaskList;
    TaskList mTaskList;         //任务队列
    uint32_t  mMaxTasks;        //最大任务个数
    
    std::atomic_bool mShutDown;             //是否关闭线程池
    
    MutexLock mLock;
    
    Condition mFullCondition;       //任务队列满的条件变量
    bool      m_bFullQueue;            //是否满的标志
    Condition mEmptyCondition;      //
    bool      m_bEmptyQueue;
    
    //下列函数禁止复制
private:
    ThreadPoolIMPL(const ThreadPoolIMPL&);
    ThreadPoolIMPL& operator = (const ThreadPoolIMPL&);
    
    static void* WorkFunc(std::shared_ptr<ThreadPoolIMPL> threadPool);
};

}

#endif /* BASELIB_THREADPOOL_IMPL_INCLUDE_H_FFGD3FGGHR */

