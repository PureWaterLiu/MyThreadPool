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
		if (q.empty()) {
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





## 1. 线程池的原理

当使用线程的时候就需要创建一个线程，但如果并发的线程量很多，并且每个线程都执行一个很短的任务就结束了，这样频繁的创建和销毁线程会大大影响系统的效率。

所以`线程池`可以帮助我们`复用线程`，就是执行完一个任务不销毁，从而继续执行下一个任务。

处理的工程中将任务添加至队列，然后再创建线程后自动启动这些任务。每个线程都是用默认堆栈大小，以默认的优先级运行。

如果某个线程在托管代码中空闲（如正在等待某个事件），则线程池将插入另一个辅助线程来使所有线程保持繁忙。

如果所有线程池线程始终`保持繁忙`，但队列中`包含挂起的工作`，则线程池在一段时间后创建另一个辅助线程但`线程总数不超过最大值`。超过最大值的线程可以排队，但他们要等待其他线程完成后才能启动。

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
// 函数指针类型起别名
using callback = void(*)(void*);
// 任务结构体
struct Task
{
    // 无参构造
	Task()
	{
		function = nullptr; 
		arg = nullptr;
	}
    // 有参构造
	Task(callback f, void* arg)
	{
		this->arg = arg;
		this->function = function;
	}

	callback function; // 回调函数指针
	void* arg; //回调函数参数
};
```

声明一个任务队列的类

```c++
class TaskQueue
{
public:
	// 添加任务
	void addTask(Task task);
    // 重载
	void addTask(callback f, void* arg);
	// 取出任务
	Task takeTask();
	// 获取队列任务个数
	inline int taskCount()
	{
		return m_taskQ.size();
	}
private:
	std :: queue<Task> m_taskQ; // 声明任务队列
	std :: mutex m_mutex;       // 锁
};
```

#### TaskQueue.cpp

```c++
#include "TaskQueue.h" //头文件

void TaskQueue::addTask(Task task)
{
	m_mutex.lock();
	m_taskQ.push(task);
	m_mutex.unlock();
}

void TaskQueue::addTask(callback f, void* arg)
{
	m_mutex.lock();
	m_taskQ.push(Task(f, arg));
	m_mutex.unlock();
}

Task TaskQueue::takeTask()
{
	m_mutex.lock();
	Task t;
	if (!m_taskQ.empty())
	{
		t = m_taskQ.front();
		m_taskQ.pop();
	}
	m_mutex.unlock();
	return t;
}
```

