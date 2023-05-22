#include "TaskQueue.h"

// ������������������
void TaskQueue::addTask(Task task)
{
	m_mutex.lock();			// ���� �߳�ͬ�� ��˳���������
	m_taskQ.push(task);		// ��������
	m_mutex.unlock();		// ����
}

// ���ذ汾���
void TaskQueue::addTask(callback f, void* arg)
{
	m_mutex.lock();				// ���� �߳�ͬ�� ��˳���������
	m_taskQ.push(Task(f, arg));	// ��������
	m_mutex.unlock();			// ����
}

// ��ȡ�����е�����
Task TaskQueue::takeTask()
{
	m_mutex.lock();				// ����
	Task t;						// ����һ����ʱ���񱣴���Ҫȡ��������
	// �ж϶������Ƿ�������
	if (!m_taskQ.empty())
	{
		t = m_taskQ.front();	// ������Ҫȡ��������
		m_taskQ.pop();			// ����
	}
	m_mutex.unlock();			// ����
	return t;					// �����õ�������
}
