//
//  ThreadPoolIMPL.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/29.
//

#include "ThreadPoolIMPL.h"

#include <algorithm>

NS_BASELIB_BEGIN

ThreadPoolIMPL::ThreadPoolIMPL():mShutDown(true),mFullCondition(&mLock),mEmptyCondition(&mLock)
{
    mThreadCount = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    mMaxTasks = 65536;
    m_bEmptyQueue = true;
    m_bFullQueue = false;
}

ThreadPoolIMPL::ThreadPoolIMPL(uint32_t nThreadCount, uint32_t nMaxTaskCount):mShutDown(true),mFullCondition(&mLock),mEmptyCondition(&mLock)
{
    mThreadCount = static_cast<int>(std::max(1u, nThreadCount));
    mMaxTasks = std::max(1u, nMaxTaskCount);
    m_bEmptyQueue = true;
    m_bFullQueue = false;
}

ThreadPoolIMPL::~ThreadPoolIMPL()
{
    ShutDown();
}

void ThreadPoolIMPL::ShutDown()
{
    std::lock_guard<std::mutex> lifecycleGuard(mLifecycleLock);

    {
        AutoLock lockGuard(mLock);
        mAcceptTasks.store(false);
        mShutDown.store(true);
        m_bEmptyQueue = false;
        mEmptyCondition.NotifyAll();
        // Execute(NO_REMOVE) 也可能在满队列条件上等待。
        mFullCondition.NotifyAll();
    }

    const auto currentThread = std::this_thread::get_id();
    for (std::thread& thread : mThreads)
    {
        if (!thread.joinable())
            continue;

        // 任务内部可能触发 ThreadPool 析构，不能 join 当前线程。
        if (thread.get_id() == currentThread)
        {
            thread.detach();
        }
        else
        {
            thread.join();
        }
    }
    mThreads.clear();
}

bool ThreadPoolIMPL::IsRunning() const
{
    return !mShutDown.load();
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
    m_bEmptyQueue = true;
    mFullCondition.NotifyAll();
}

void ThreadPoolIMPL::Execute(const TaskRunnerPtr &task, ThreadPool::TaskStrategy strategy)
{
    if (!task)
    {
        return;
    }

    AutoLock lockGuard(mLock);

    if (!mAcceptTasks.load())
    {
        return;
    }

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

        if (!mShutDown.load())
        {
            m_bEmptyQueue = false;
            mEmptyCondition.NotifyAll();
        }
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
    std::lock_guard<std::mutex> lifecycleGuard(mLifecycleLock);
    if (!mShutDown.load())
    {
        return;
    }

    mShutDown.store(false);
    mAcceptTasks.store(true);

    const int nThreads = mThreadCount;
    for (int i = 0; i < nThreads; i++)
    {
        mThreads.emplace_back(std::thread(std::bind(ThreadPoolIMPL::WorkFunc, std::weak_ptr(shared_from_this()))));
    }
    
}

NS_BASELIB_END
