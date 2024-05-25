//任务队列
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
     *  移除任务
     *
     *  @param task 任务
     *
     *  @return 成功返回true
     */
    bool RemoveTask(const TaskRunnerPtr& task);
    
    /**
     *  非阻塞地获得最前面的Task
     *
     *  @return 任务
     */
    TaskRunnerPtr GetHeadTask();
    
    /**
     *  是否为空
     *
     *  @return 为空返回true
     */
    bool IsEmpty() const;
    
    /**
     *  清空任务队列
     */
    void Clear();
    
    /**
     *  设置最大任务数量
     *
     *  @param nMaxTaskCount 任务数量
     */
    void SetMaxTaskCount(int nMaxTaskCount);
    
    /**
     *  获得任务数量
     *
     *  @return 任务数量
     */
    int GetTaskCount() const;

private:
	typedef std::deque< TaskRunnerPtr > TaskList;
	TaskList mTaskList;         //任务列表
    MutexLock mLock;            //队列锁
    Condition mFullCondition;       //队列满的条件变量
    bool     m_bFullQueue;
    Condition mEmptyCondition;      //队列为空的条件变量
    bool     m_bEmptyQueue;
    
    int mMaxTaskCount;          //最大任务数量
};

NS_BASELIB_END

#endif
