//任务封装
#ifndef BASELIB_TASKRUNNER_INCLUDE_H
#define BASELIB_TASKRUNNER_INCLUDE_H 

#include "PreCompile.h"
#include "Thread.h"

NS_BASELIB_BEGIN

class BASELIB_API TaskRunner
{
public:
	TaskRunner(void);

	virtual ~TaskRunner(void);

	//获得任务ID
	size_t GetTaskId() const;

	//设置任务ID
	void SetTaskId(size_t taskId);

	int GetTaskPriority() const;

	void SetTaskPriority(int nPriority);

	//退出，只是设置状态
	virtual void Exit();

	virtual bool IsPendingExit() const volatile;

	//实际运行函数
	virtual void Run() = 0;
    
    /**
     *  是否取消
     *
     *
     */
    bool IsCancel() const;
    
    /**
     *  取消任务
     */
    void Cancel();

protected:
	size_t mTaskId;		//task id
	int mPriority;		//优先级，数值越小，优先级越高
	volatile bool mIsPendingExit;
    bool mCancel;       //是否取消
};

typedef std::shared_ptr<TaskRunner> TaskRunnerPtr;

NS_BASELIB_END

#endif
