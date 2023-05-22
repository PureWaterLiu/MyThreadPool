#include "TaskQueue.h"

// 往任务队列中添加任务
void TaskQueue::addTask(Task task)
{
	m_mutex.lock();			// 加锁 线程同步 按顺序插入任务
	m_taskQ.push(task);		// 插入任务
	m_mutex.unlock();		// 解锁
}

// 重载版本添加
void TaskQueue::addTask(callback f, void* arg)
{
	m_mutex.lock();				// 加锁 线程同步 按顺序插入任务
	m_taskQ.push(Task(f, arg));	// 插入任务
	m_mutex.unlock();			// 解锁
}

// 获取队列中的任务
Task TaskQueue::takeTask()
{
	m_mutex.lock();				// 加锁
	Task t;						// 创建一个临时任务保存需要取出的任务
	// 判断队列中是否有任务
	if (!m_taskQ.empty())
	{
		t = m_taskQ.front();	// 保存需要取出的任务
		m_taskQ.pop();			// 弹出
	}
	m_mutex.unlock();			// 解锁
	return t;					// 返回拿到的任务
}
