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
	using Task = function<void()>; // 任务类
	template<typename F, typename ...Args>
	auto addTask(F& f, Args &&...args)->future<decltype(f(args...))>;
	Task taskTake();				// 取任务
	bool empty() { return taskQueue.empty(); }
private:
	mutex taskQueueMutex;		// 任务队列互斥锁
	queue<Task> taskQueue;		// 任务队列
};

template <typename F, typename ...Args>
auto TaskQueue::addTask(F& f, Args &&...args)->future<decltype(f(args...))> {
	using RetType = decltype(f(args...));		// 获取函数返回值类型
	// 将函数封装为无形参的类型 std::bind(f, std::forward<Args>(args)...)：将参数与函数名绑定
	// packaged_task<RetType()>(std::bind(f, std::forward<Args>(args)...)); 将绑定参数后的函数封装为只有返回值没有形参的任务对象，\
	这样就能使用get_future得到future对象，然后future对象可以通过get方法获取返回值了
	// std::make_shared<std::packaged_task<RetType()>>(std::bind(f, std::forward<Args>(args)...)); 生成智能指针，离开作用域自动析构
	auto task = make_shared<packaged_task<RetType()>>(bind(f, forward<Args>(args)...));
	lock_guard<mutex> lockGuard(taskQueueMutex); // 插入时上锁，防止多个线程同时插入
	// 将函数封装为无返回无形参类型，通过lamdba表达式，调用封装后的函数，注意，此时返回一个无形参无返回值的函数对象
	taskQueue.emplace([task] {(*task)(); });	// emplace直接在队尾进行元素构造并插入
	return task->get_future();
}

#endif // !_TASKQUEUE_H_