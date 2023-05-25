#pragma once
/*
 @Author: Marvin
 @Date: 2023/05/26 1:06:00

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

	  https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include <queue>
#include <functional>
#include <mutex>
#include <future>
#include <iostream>
using namespace std;
class TaskQueue
{
public:
	using Task = function<void()>; // ������
	template<typename F, typename ...Args>
	auto addTask(F& f, Args &&...args)->future<decltype(f(args...))>;
	Task taskTake();				// ȡ����
	bool empty() { return taskQueue.empty(); }
private:
	mutex taskQueueMutex;		// ������л�����
	queue<Task> taskQueue;		// �������
};

template <typename F, typename ...Args>
auto TaskQueue::addTask(F& f, Args &&...args)->future<decltype(f(args...))> {
	using RetType = decltype(f(args...));		// ��ȡ��������ֵ����
	// ��������װΪ���βε����� std::bind(f, std::forward<Args>(args)...)���������뺯������
	// packaged_task<RetType()>(std::bind(f, std::forward<Args>(args)...)); ���󶨲�����ĺ�����װΪֻ�з���ֵû���βε��������\
	��������ʹ��get_future�õ�future����Ȼ��future�������ͨ��get������ȡ����ֵ��
	// std::make_shared<std::packaged_task<RetType()>>(std::bind(f, std::forward<Args>(args)...)); ��������ָ�룬�뿪�������Զ�����
	auto task = make_shared<packaged_task<RetType()>>(bind(f, forward<Args>(args)...));
	lock_guard<mutex> lockGuard(taskQueueMutex); // ����ʱ��������ֹ����߳�ͬʱ����
	// ��������װΪ�޷������β����ͣ�ͨ��lamdba���ʽ�����÷�װ��ĺ�����ע�⣬��ʱ����һ�����β��޷���ֵ�ĺ�������
	taskQueue.emplace([task] {(*task)(); });	// emplaceֱ���ڶ�β����Ԫ�ع��첢����
	return task->get_future();
}

#endif // !_TASKQUEUE_H_