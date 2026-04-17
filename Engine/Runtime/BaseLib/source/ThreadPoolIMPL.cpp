//
//  ThreadPoolIMPL.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/29.
//

#include "ThreadPoolIMPL.h"

NS_BASELIB_BEGIN

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

    //唤醒所有等待在空队列条件变量上的工作线程，使其退出循环
    {
        AutoLock lockGuard(mLock);
        m_bEmptyQueue = false;
        mEmptyCondition.NotifyAll();
    }

    for (size_t i = 0; i < mThreads.size(); i++)
    {
        if (mThreads[i].joinable())
        {
            mThreads[i].join();
        }
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

void ThreadPoolIMPL::CancelAllTasks()
{
    AutoLock lock_guard(mLock);
    mTaskList.clear();
}

void ThreadPoolIMPL::Execute(const TaskRunnerPtr &task, ThreadPool::TaskStrategy strategy)
{
    AutoLock lockGuard(mLock);

    //如果超过最大任务数量，根据不同策略做不同的操作
    if (mTaskList.size() >= mMaxTasks)
    {
        if (strategy == ThreadPool::REMOVE_LAST)
        {
            mTaskList.pop_back();
            mTaskList.push_back(task);
        }

        else if (strategy == ThreadPool::REMOVE_FIRST)
        {
            mTaskList.pop_front();
            mTaskList.push_back(task);
        }

        else if (strategy == ThreadPool::NO_REMOVE)
        {
            while (mTaskList.size() >= mMaxTasks && !mShutDown)
            {
                mFullCondition.Wait();
            }

            if (!mShutDown)
            {
                mTaskList.push_back(task);
            }
        }

        m_bEmptyQueue = false;
        mEmptyCondition.NotifyAll();
    }

    //正常插入
    else
    {
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
        return nullptr;
    }

    TaskRunnerPtr task = mTaskList.front();
    mTaskList.pop_front();
    mFullCondition.NotifyAll();

    if (mTaskList.empty())
    {
        m_bEmptyQueue = true;
    }

    return task;
}

void* ThreadPoolIMPL::WorkFunc(std::weak_ptr<ThreadPoolIMPL> weakThreadPool)
{
    std::shared_ptr<ThreadPoolIMPL> threadPool = weakThreadPool.lock();
    if (!threadPool)
    {
        return nullptr;
    }

    while ((threadPool = weakThreadPool.lock()) && threadPool->IsRunning())
    {
        TaskRunnerPtr pTask = threadPool->GetHeadTask();
        if (nullptr == pTask)
        {
            //表示队列里面没有数据了，线程阻塞
            AutoLock lockGuard(threadPool->mLock);
            while (threadPool->m_bEmptyQueue && threadPool->IsRunning())
            {
                threadPool->mEmptyCondition.Wait();
            }
        }
        else
        {
            pTask->Run();
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
        mThreads.emplace_back(std::thread(std::bind(ThreadPoolIMPL::WorkFunc, std::weak_ptr(shared_from_this()))));
    }
    
}

NS_BASELIB_END
