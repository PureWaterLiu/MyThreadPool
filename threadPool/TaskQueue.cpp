#include "TaskQueue.h"

/**
 * �����������ȡ����
 */
TaskQueue::Task TaskQueue::taskTake()
{
	Task task;
	lock_guard<mutex> lockGuard(taskQueueMutex);
	if (!taskQueue.empty())
	{
		task = move(taskQueue.front());
		taskQueue.pop();	// ������Ӷ�����ɾ��
		return task;
	}
	return nullptr;
}
