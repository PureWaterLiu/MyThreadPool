#include "TaskQueue.h"

/**
 * 从任务队列中取任务
 */
TaskQueue::Task TaskQueue::taskTake()
{
	Task task;
	lock_guard<mutex> lockGuard(taskQueueMutex);
	if (!taskQueue.empty())
	{
		task = move(taskQueue.front());
		taskQueue.pop();	// 将任务从队列中删除
		return task;
	}
	return nullptr;
}
