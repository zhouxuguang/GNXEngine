//任务队列的实现
#ifndef BASELIB_TASKQUEUE_INCLUDE_H
#define BASELIB_TASKQUEUE_INCLUDE_H

#include "TaskRunner.h"
#include "MutexLock.h"
#include "Condition.h"
#include "ThreadPool.h"

NS_BASELIB_BEGIN

class BASELIB_API TaskQueue
{
public:
	explicit TaskQueue(int nMaxTaskCount = 100);

	~TaskQueue(void);
    
    /**
     *  添加任务
     *
     *  @param task         任务
     *  @param strategy 策略类型
     */
    bool AddTask(const TaskRunnerPtr& task, ThreadPool::TaskStrategy strategy = ThreadPool::NO_REMOVE);
    
    /**
     *  删除任务
     *
     *  @param task 任务
     *
     *  @return 删除成功返回true
     */
    bool RemoveTask(const TaskRunnerPtr& task);
    
    /**
     *  获得队列头部的任务
     *
     *  @return 任务指针
     */
    TaskRunnerPtr GetHeadTask();
    
    /**
     *  队列是否为空
     *
     *  @return 为空返回true
     */
    bool IsEmpty() const;
    
    /**
     *  清除队列任务
     */
    void Clear();
    
    /**
     *  设置任务队列最大任务数量
     *
     *  @param nMaxTaskCount 最大任务数量
     */
    void SetMaxTaskCount(int nMaxTaskCount);
    
    /**
     *  获得任务数量
     *
     *  @return 返回任务数量
     */
    int GetTaskCount() const;

private:
	typedef std::deque<TaskRunnerPtr> TaskList;
	TaskList mTaskList;         //任务列表
    mutable MutexLock mLock;            //队列的锁
    Condition mFullCondition;       //队列满的条件变量
    bool     m_bFullQueue;
    Condition mEmptyCondition;      //队列空的条件变量
    bool     m_bEmptyQueue;
    
    int mMaxTaskCount;          //最大任务数量
};

NS_BASELIB_END

#endif
