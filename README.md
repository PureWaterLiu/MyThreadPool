# C++手写线程池

## 0. 前置学习

### 条件变量（condition_variable）

和unique_lock一起用

```c++
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <condition_variable>

using namespace std;

mutex mtx;
deque<int> q;
condition_variable cv;

// 生产者
void task1()
{
	int i = 0;
	while (true) {
		unique_lock<mutex> lock(mtx); // 申请加锁，配合cv用

		q.push_back(i);
		cv.notify_one();			// 唤醒一个因条件变量而被阻塞的线程
		if (i < 99999999) {
			i++;
		}
		else {
			i = 0;
		}
	}
}

// 消费者
void task2()
{
	int data = 0;
	while (true) {
		unique_lock<mutex> lock(mtx);		
		while (q.empty()) {
			cv.wait(lock);				// 阻塞线程，防止资源消耗
		}
		if (!q.empty()) {
			data = q.front();
			q.pop_front();
			cout << "从队列获得数据：" << data << endl;
		}
	}
}

int main()
{
	thread t1(task1);
	thread t2(task2);

	t1.join();
	t2.join();
}
```

### promise future

```c++
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
using namespace std;

void task(int a, future<int>& b/*后提供的值*/, promise<int>& ret/*需要的值*/) {
	int ret_a = a * a;
	int ret_b = b.get() * 2;		// 阻塞获取未来提供到的值

	ret.set_value(ret_a + ret_b);	// 提供需要的值
}

int main()
{
	int ret = 0;
	promise<int> p_ret;
	future<int> f_ret = p_ret.get_future();

	promise<int> p_in;
	future<int> f_in = p_in.get_future();

	p_in.set_value(2);

	thread t(task, 1, ref(f_in), ref(p_ret));
	cout << f_ret.get() << endl;		// 阻塞直到p被赋值
	t.join();
}
```

future的get只能为一个线程使用，所以需要使用shared_future

### async

async函数可以获取线程中的函数返回值，并且类型为future

```c++
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
using namespace std;

int task(int a, int b) {
	int ret_a = a * a;
	int ret_b = b * 2;		
	return ret_a + ret_b;	
}

int main()
{
    // launch::async和launch::deffered区别在于
    // async 是创建一个新线程执行task
    // deffered是延迟调用task（在get的时候执行）
	future<int> f = async(launch::async | launch::deffered, task, 1, 2);
	cout << f.get();
}
```

### package_task

```c++
#include <iostream>
#include <thread>
#include <future>
using namespace std;

int task(int a, int b) {
	int ret_a = a * a;
	int ret_b = b * 2;		
	return ret_a + ret_b;	
}

int main()
{
	packaged_task<int(int, int)> pt(task);	// 包装task，使它能够在其他地方使用
	pt(1, 2);								// 传参
	cout << pt.get_future().get();			// 获得返回值
}
```

如何在定义packaged_task的时候就传递参数呢？

#### bind

使用bind绑定函数和参数就可以实现上述问题。但是它的实际原理使将参数转换成在函数内定义。

```c++
// 原本
int task(int a, int b) {
	int ret_a = a * a;
	int ret_b = b * 2;		
	return ret_a + ret_b;	
}
// bind(task,1,2)绑定后
int task() {
    int a = 1, b = 2;
	int ret_a = a * a;
	int ret_b = b * 2;		
	return ret_a + ret_b;	
}
```

所以在新的packaged_task中，就需要修改一下模板类型

```c++
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
using namespace std;

int task(int a, int b) {
	int ret_a = a * a;
	int ret_b = b * 2;		
	return ret_a + ret_b;	
}

int main()
{
	packaged_task<int(/*此处不需要参数类型*/)> pt(bind(task,1,2));	// 包装task，使它能够在其他地方使用
	pt();										// 调用
	cout << pt.get_future().get();				// 获得返回值
```

### packaged_task和future可以实现异步编程和任务结果转递

```c++
#include <iostream>
#include <future>

int sum(int a, int b) {
    return a + b;
}

int main() {
    // 创建一个packaged_task，并绑定sum函数
    std::packaged_task<int(int, int)> task(sum);

    // 获取task的future
    std::future<int> future = task.get_future();

    // 创建一个线程，执行task
    std::thread t(std::move(task), 2, 3);

    // 等待任务执行完成并获取结果
    int result = future.get();

    // 打印结果
    std::cout << "Result: " << result << std::endl;

    // 等待线程结束
    t.join();

    return 0;
}
```

- 注意：

  ​	由于packaged_task不可拷贝构造，所以需要通过移动语义（move）传递给线程构造函数





## 1. 线程池的原理

当使用线程的时候就需要创建一个线程，但如果并发的线程量很多，并且每个线程都执行一个很短的任务就结束了，这样频繁的创建和销毁线程会大大影响系统的效率。

所以`线程池`可以帮助我们`复用线程`，就是执行完一个任务不销毁，从而继续执行下一个任务。

处理的工程中将任务添加至队列，然后再创建线程后自动启动这些任务。每个线程都是用默认堆栈大小，以默认的优先级运行。

如果某个线程在托管代码中空闲（如正在等待某个事件），则线程池将插入另一个辅助线程来使所有线程保持繁忙。

如果所有线程池线程始终`保持繁忙`，但队列中`包含挂起的工作`，则线程池在一段时间后创建另一个辅助线程但`线程总数不超过最大值`。超过最大值的线程可以排队，但他们要等待其他线程完成后才能启动。

**问：线程运行完函数后自动就被系统回收了，怎么才能实现复用呢？**

**答**：让线程执行一个死循环任务，当任务队列为空时，就让他阻塞防止资源浪费，当有任务时，解除阻塞，让线程向下执行，当执行完当前函数后，又会再次运行到死循环的的上方，继续向下执行，从而周而复始的不断接任务--完成任务--接任务的循环

==线程池主要维护一个任务队列以及若干个消费者线程==

- 线程池的组成主要分为 `3` 个部分，这`三部分配合工作`就可以得到一个`完整的线程池`：

  1. 任务队列(STL容器)：存储需要处理的任务（`回调函数地址`），由工作的线程来处理这些任务

     ==（有任务队列的地方就离不开生产者和消费者模型）==

  - `生产者（谁使用谁就是）`通过线程池提供的 `API 函数`，将一个`待处理的任务添加`到`任务队列`，或者从任务队列中`删除`

  - `已处理的任务`会被从任务队列中`删除`

  - 后续消费者线程通过一个循环，不停的从任务队列中取任务（`调用回调函数`）（队列为空需要阻塞）

  - 一句话：生产者的数据或者说商品进行存储，准备让对应的消费者消费。

    ==（生产者将数据放入任务队列，消费者从里面取出来 后面再处理掉）==

 	2. 工作的线程（任务队列任务的`消费者`） ，N个
 	 - 线程池中维护了一定数量的工作线程，他们的作用是是不停的`读任务队列`(调用回调函数，从里边取出任务并处理
 	 - 工作的线程相当于是任务队列的`消费者`角色，
 	 - 如果`任务队列为空`，工作的线程将会`被阻塞` (`使用条件变量 / 信号量阻塞`)
 	 - 如果阻塞之后有了新的任务，由`生产者将阻塞解除`，工作线程开始工作

3. 管理者线程（`不处理任务队列中的任务=不工作`），1个
   - 它的任务是周期性的对`任务队列`中的`任务数量`以及处于`忙状态的工作线程个数`进行`检测`
     - 当任务过`多`的时候，可以适当的`创建`一些新的工作线程
     - 当任务过`少`的时候，可以适当的`销毁`一些工作的线程

![查看源图像](./C:/Users/Marvin/Desktop/编程学习/C++/C++11/assets/R9c00030b842edb1ae3d6a2b286e53916)



## 2. 写一个C++线程池

### 任务队列

#### TaskQueue.h

首先先写一个任务的结构体

```c++
#pragma once
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
```

#### TaskQueue.cpp

```c++
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

```

