#include "TaskQueue.h"
#include "AtomicOps.h"

NS_BASELIB_BEGIN

TaskQueue::TaskQueue(int nMaxTaskCount) : mFullCondition(&mLock), mEmptyCondition(&mLock),
            mMaxTaskCount(nMaxTaskCount), m_bFullQueue(false), m_bEmptyQueue(true)
{
    Clear();
}

TaskQueue::~TaskQueue(void)
{
    Clear();
}

bool TaskQueue::AddTask(const TaskRunnerPtr &task, ThreadPool::TaskStrategy strategy)
{
    AutoLock lock_guard(mLock);

    //如果超过最大任务数量，根据不同策略做不同的操作
    if(mTaskList.size() >= (size_t)mMaxTaskCount)
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
            while (mTaskList.size() >= (size_t)mMaxTaskCount)
            {
                mFullCondition.Wait();
            }

            mTaskList.push_back(task);
        }

        m_bEmptyQueue = false;
        mEmptyCondition.NotifyAll();
    }

    //如果在最大值范围内，则唤醒等待条件为空的线程
    else
    {
        mTaskList.push_back(task);

        m_bEmptyQueue = false;
        mEmptyCondition.NotifyAll();
    }

    return true;
}

bool TaskQueue::RemoveTask(const TaskRunnerPtr &task)
{
    AutoLock lock_guard(mLock);

    TaskList::iterator iter = mTaskList.begin();
    TaskList::iterator iterEnd = mTaskList.end();
    for (; iter != iterEnd; )
    {
        //找到一个返回
        if (task == *iter)
        {
            iter = mTaskList.erase(iter);
            return true;
        }
        else
            iter++;
    }

    return false;
}

TaskRunnerPtr TaskQueue::GetHeadTask()
{
    AutoLock lock_guard(mLock);

    if (mTaskList.empty())
    {
        return nullptr;
    }

    TaskRunnerPtr task = mTaskList.front();
    mTaskList.pop_front();
    m_bFullQueue = false;
    mFullCondition.NotifyAll();

    return task;
}

bool TaskQueue::IsEmpty() const
{
	AutoLock lock_guard(mLock);
    return mTaskList.empty();
}

void TaskQueue::Clear()
{
    AutoLock lock_guard(mLock);
    mTaskList.clear();
    mFullCondition.NotifyAll();
    mEmptyCondition.NotifyAll();
}

void TaskQueue::SetMaxTaskCount(int nMaxTaskCount)
{
    mMaxTaskCount = nMaxTaskCount;
}

int TaskQueue::GetTaskCount() const
{
	AutoLock lock_guard(mLock);
    return (int)mTaskList.size();
}

NS_BASELIB_END
