//
//  ThreadPoolIMPL.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/29.
//

#include "ThreadPoolIMPL.h"

namespace baselib
{

ThreadPoolIMPL::ThreadPoolIMPL():mShutDown(true),mFullCondition(&mLock),mEmptyCondition(&mLock)
{
    mThreadCount = std::thread::hardware_concurrency();
    mMaxTasks = 65536;
    m_bEmptyQueue = true;
    m_bFullQueue = false;
}

ThreadPoolIMPL::ThreadPoolIMPL(uint32_t nThreadCount, uint32_t nMaxTaskCount):mShutDown(true),mFullCondition(&mLock),mEmptyCondition(&mLock)
{
    mThreadCount = nThreadCount;
    mMaxTasks = nMaxTaskCount;
    m_bEmptyQueue = true;
    m_bFullQueue = false;
}

ThreadPoolIMPL::~ThreadPoolIMPL()
{
    ShutDown();
}

void ThreadPoolIMPL::ShutDown()
{
    mShutDown = true;
    size_t nThreads = mThreads.size();
    for (size_t i = 0; i < nThreads; i ++)
    {
        mThreads[i].detach();
    }
    mThreads.clear();
}

bool ThreadPoolIMPL::IsRunning() const
{
    return !mShutDown;
}

int ThreadPoolIMPL::GetThreadCount()
{
    AutoLock lock_guard(mLock);
    return (int)mThreads.size();
}

int ThreadPoolIMPL::GetTaskCount()
{
    AutoLock lock_guard(mLock);
    return (int)mTaskList.size();
}

void ThreadPoolIMPL::Execute(const TaskRunnerPtr &task, ThreadPool::TaskStrategy strategy)
{
    mLock.Lock();
    int nTaskCount = (int)mTaskList.size();
    mLock.UnLock();
    
    //如果超过最大任务数量，根据不同策略做不同的操作
    if (nTaskCount >= mMaxTasks)
    {
        if (strategy == ThreadPool::REMOVE_LAST)
        {
            AutoLock lockGuard(mLock);
            mTaskList.pop_back();
            mTaskList.push_back(task);
        }
        
        else if (strategy == ThreadPool::REMOVE_FIRST)
        {
            AutoLock lockGuard(mLock);
            mTaskList.pop_front();
            mTaskList.push_back(task);
        }
        
        else if (strategy == ThreadPool::NO_REMOVE)
        {
            AutoLock lockGuard(mLock);
            
            while (m_bFullQueue)
            {
                mFullCondition.Wait();
            }
            
            m_bFullQueue = true;
            
        }
    }
    
    //正常插入
    else if (nTaskCount < mMaxTasks)
    {
        AutoLock lockGuard(mLock);
        mTaskList.push_back(task);
        
        m_bEmptyQueue = false;
        mEmptyCondition.NotifyAll();
    }
}

TaskRunnerPtr ThreadPoolIMPL::GetHeadTask()
{
    AutoLock lockGuard(mLock);
    
    if (mTaskList.empty())
    {
        return NULL;
    }
    
    else
    {
        TaskRunnerPtr task = mTaskList.front();
        mTaskList.pop_front();
        m_bFullQueue = false;
        mFullCondition.NotifyAll();
        
        return task;
    }
}

void* ThreadPoolIMPL::WorkFunc(std::shared_ptr<ThreadPoolIMPL> threadPool)
{
    while (threadPool && threadPool->IsRunning())
    {
        TaskRunnerPtr pTask = threadPool->GetHeadTask();
        if (NULL == pTask)
        {
            //表示队列里面没有数据了，线程阻塞

            threadPool->mLock.Lock();
            while (threadPool->m_bEmptyQueue)
            {
                threadPool->mEmptyCondition.Wait();
            }

            threadPool->m_bEmptyQueue = true;
            threadPool->mLock.UnLock();
        }

        else
        {
            assert(pTask != NULL);
            pTask->Run();
            //printf("Task ID = %d\n",pTask->GetTaskId());
        }

    }

    return nullptr;
}

void ThreadPoolIMPL::Start()
{
    if (!mShutDown)
    {
        return;
    }

    mShutDown = false;

    size_t nThreads = mThreadCount;
    for (int i = 0; i < nThreads; i++)
    {
        mThreads.emplace_back(std::thread(std::bind(ThreadPoolIMPL::WorkFunc, shared_from_this())));
    }
    
}

}

