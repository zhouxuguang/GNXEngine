#include "TaskRunner.h"

NS_BASELIB_BEGIN

TaskRunner::TaskRunner(void): mTaskId(0),mIsPendingExit(false),mPriority(0),mCancel(false)
{
}

TaskRunner::~TaskRunner(void)
{
}

size_t TaskRunner::GetTaskId() const
{
	return mTaskId;
}

void TaskRunner::SetTaskId(size_t taskId)
{
	mTaskId = taskId;
}

void TaskRunner::Exit()
{
	mIsPendingExit = true;
}

bool TaskRunner::IsPendingExit() const volatile
{
	return mIsPendingExit;
}

int TaskRunner::GetTaskPriority() const
{
	return mPriority;
}

void TaskRunner::SetTaskPriority(int nPriority)
{
	mPriority = nPriority;
}

static void* ThreadCallbackFunc(void* pPara)
{
	TaskRunner* pThis = (TaskRunner*)pPara;
	if (pThis != NULL)
	{
		pThis->Run();
	}

	return pThis;
}

bool TaskRunner::IsCancel() const
{
    return mCancel;
}

void TaskRunner::Cancel()
{
    mCancel = true;
}

NS_BASELIB_END
