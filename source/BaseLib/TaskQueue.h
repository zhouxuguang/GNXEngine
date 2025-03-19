//�������
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
     *  ��������
     *
     *  @param task         ����
     *  @param strategy ��������
     */
    bool AddTask(const TaskRunnerPtr& task, ThreadPool::TaskStrategy strategy = ThreadPool::NO_REMOVE);
    
    /**
     *  �Ƴ�����
     *
     *  @param task ����
     *
     *  @return �ɹ�����true
     */
    bool RemoveTask(const TaskRunnerPtr& task);
    
    /**
     *  �������ػ����ǰ���Task
     *
     *  @return ����
     */
    TaskRunnerPtr GetHeadTask();
    
    /**
     *  �Ƿ�Ϊ��
     *
     *  @return Ϊ�շ���true
     */
    bool IsEmpty() const;
    
    /**
     *  ����������
     */
    void Clear();
    
    /**
     *  ���������������
     *
     *  @param nMaxTaskCount ��������
     */
    void SetMaxTaskCount(int nMaxTaskCount);
    
    /**
     *  �����������
     *
     *  @return ��������
     */
    int GetTaskCount() const;

private:
	typedef std::deque< TaskRunnerPtr > TaskList;
	TaskList mTaskList;         //�����б�
    MutexLock mLock;            //������
    Condition mFullCondition;       //����������������
    bool     m_bFullQueue;
    Condition mEmptyCondition;      //����Ϊ�յ���������
    bool     m_bEmptyQueue;
    
    int mMaxTaskCount;          //�����������
};

NS_BASELIB_END

#endif
