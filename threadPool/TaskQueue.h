#pragma once
#include <queue>
#include <mutex>

using callback = void(*)(void*);
/**
 * 任务结构体
 */
struct Task
{
	Task()
	{
		function = nullptr;
		arg = nullptr;
	}
	Task(callback f, void* arg)
	{
		this->arg = arg;
		this->function = function;
	}

	callback function;
	void* arg;
};
/**
 * 任务队列类
 */
class TaskQueue
{
public:
	// 添加任务
	void addTask(Task task);
	void addTask(callback f, void* arg);
	// 取出任务
	Task takeTask();
	// 获取队列任务个数
	inline int taskCount()
	{
		return m_taskQ.size();
	}
private:
	std :: queue<Task> m_taskQ;
	std :: mutex m_mutex;
};

