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
    //如果超过最大任务数量，根据不同策略做不同的操作
    if(mTaskList.size() >= mMaxTaskCount)
    {
        if (strategy == ThreadPool::REMOVE_LAST)
        {
            mLock.Lock();
            mTaskList.pop_back();
            mTaskList.push_back(task);
            mLock.UnLock();
            //mFullCondition.notifyAll();
        }
        
        else if (strategy == ThreadPool::REMOVE_FIRST)
        {
            mLock.Lock();
            mTaskList.pop_front();
            mTaskList.push_back(task);
            mLock.UnLock();
            //mFullCondition.notifyAll();
        }
        
        else if (strategy == ThreadPool::NO_REMOVE)
        {
            mLock.Lock();
            
            if (mTaskList.size() == mMaxTaskCount)
            {
                while (m_bFullQueue)
                {
                    mFullCondition.Wait();
                }
                
                m_bFullQueue = true;
			}
            mLock.UnLock();
            
        }
    }
    
    //如果在最大值范围内，则唤醒等待条件为空的线程
    else if (mTaskList.size() < mMaxTaskCount)
    {
		bool bNeedNotify = mTaskList.empty();
        mLock.Lock();
        mTaskList.push_back(task);
        
        m_bEmptyQueue = false;
        
		if (bNeedNotify)
		{
			//mEmptyCondition.NotifyAll();
		}
        
        mLock.UnLock();
    }
    
    return true;
}

bool TaskQueue::RemoveTask(const TaskRunnerPtr &task)
{
    AutoLock lock_guard(&mLock);
    
    //todo  这里面也需要处理条件变量
    TaskList::iterator iter = mTaskList.begin();
    TaskList::iterator iterEnd = mTaskList.begin();
    for (; iter != iterEnd; )
    {
        //找到一个返回
        if (task == *iter)
        {
            iter = mTaskList.erase(iter);
            return true;
        }
        else
            iter ++;
    }
    
    return false;
}

TaskRunnerPtr TaskQueue::GetHeadTask()
{
	mLock.Lock();
	bool bEmpty = mTaskList.empty();
	mLock.UnLock();

    if (bEmpty)
    {
        return NULL;
    }
	
    else
    {
        mLock.Lock();
        TaskRunnerPtr task = mTaskList.front();
        mTaskList.pop_front();
        m_bFullQueue = false;
        mFullCondition.NotifyAll();
        mLock.UnLock();
        
        return task;
    }
    
    return NULL;
}

bool TaskQueue::IsEmpty() const
{
	AutoLock lock_guard(&mLock);
    return mTaskList.empty();
}

void TaskQueue::Clear()
{
    AutoLock lock_guard(&mLock);
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
	AutoLock lock_guard(&mLock);
    return (int)mTaskList.size();
}

NS_BASELIB_END
