//
//  ThreadPool.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/9/28.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "ThreadPool.h"
#include "AtomicOps.h"
#include "ThreadPoolIMPL.h"

NS_BASELIB_BEGIN

ThreadPool::ThreadPool()
{
    mIMPL = std::make_shared<ThreadPoolIMPL>();
}

ThreadPool::ThreadPool(uint32_t nThreadCount, uint32_t nMaxTaskCount)
{
    mIMPL = std::make_shared<ThreadPoolIMPL>(nThreadCount, nMaxTaskCount);
}

ThreadPool::~ThreadPool()
{
    ShutDown();
}

void ThreadPool::ShutDown()
{
    mIMPL->ShutDown();
}

bool ThreadPool::IsRunning() const
{
    return mIMPL->IsRunning();
}

int ThreadPool::GetThreadCount() const
{
    return mIMPL->GetThreadCount();
}

int ThreadPool::GetTaskCount() const
{
    return mIMPL->GetTaskCount();
}

void ThreadPool::CancelAllTasks()
{
    mIMPL->CancelAllTasks();
}

void ThreadPool::Execute(const TaskRunnerPtr &task, TaskStrategy strategy)
{
    mIMPL->Execute(task, strategy);
}

TaskRunnerPtr ThreadPool::GetHeadTask()
{
    return mIMPL->GetHeadTask();
}

void ThreadPool::Start()
{
    mIMPL->Start();
}

NS_BASELIB_END
