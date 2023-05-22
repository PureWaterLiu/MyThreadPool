#pragma once
#include <queue>
#include <mutex>

using callback = void(*)(void*);
/**
 * ����ṹ��
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
 * ���������
 */
class TaskQueue
{
public:
	// �������
	void addTask(Task task);
	void addTask(callback f, void* arg);
	// ȡ������
	Task takeTask();
	// ��ȡ�����������
	inline int taskCount()
	{
		return m_taskQ.size();
	}
private:
	std :: queue<Task> m_taskQ;
	std :: mutex m_mutex;
};

