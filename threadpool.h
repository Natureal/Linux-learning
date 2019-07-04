#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"

// 半同步/半反应堆形式

// 线程池类
template<typename T>
class threadpool{
public:
	threadpool(int thread_number = 8, int max_requests = 10000);
	~threadpool();
	bool append(T* request);

private:
	// 不断从工作队列中取出任务并执行
	// worker 作为 pthread_create 的第三个参数，必须为静态函数
	static void *worker(void *arg);
	void run();

private:
	int m_thread_number; // 线程池中的线程数
	int m_max_requests; // 允许的最大请求数
	pthread_t *m_threads; // 进程池数组
	std::list<T*> m_workqueue; // 请求队列
	locker m_queuelocker; // 保护请求队列的互斥锁
	sem m_queuestat; // 是否有任务需要处理
	bool m_stop; // 是否结束线程
};

// 构造函数
template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
	m_thread_number(thread_number), m_max_requests(max_requests),
	m_stop(false), m_threads(NULL)
{
	if((thread_number <= 0) || (max_requests <= 0)){
		throw std::exception();
	}
	
	// 初始化线程数组
	m_threads = new pthread_t[m_thread_number];
	if(!m_threads){
		throw std::exception();
	}

	// 创建 thread_number 个线程，并将它们都设置为脱离线程
	for(int i = 0; i < thread_number; ++i){
		printf("create the %dth thread\n", i);
		if(pthread_create(m_threads + i, NULL, worker, this) != 0){
			delete[] m_threads;
			throw std::exception();
		}
		// 将所有进程都标记为脱离模式
		if(pthread_detach(m_threads[i])){
			delete[] m_threads;
			throw std::exception();
		}
	}
}

// 析构函数
template<typename T>
threadpool<T>::~threadpool(){
	delete[] m_threads;
	m_stop = true;
}

// 往请求队列里添加任务
template<typename T>
bool threadpool<T>::append(T *request){
	// 操作工作队列时一定要加锁，因为它被所有线程共享
	m_queuelocker.lock();
	if(m_workqueue.size() > m_max_requests){
		m_queuelocker.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}

// 工作线程运行的函数
template<typename T>
void *threadpool<T>::worker(void *arg){
	threadpool *pool = (threadpool *)arg;
	pool->run();
	return pool;
}

template<typename T>
void threadpool<T>::run(){
	// 线程循环（阻塞）等待工作队列来任务
	while(!m_stop){
		m_queuestat.wait(); // 等待任务处理的信号
		m_queuelocker.lock(); // 锁住工作队列
		if(m_workqueue.empty()){
			m_queuelocker.unlock();
			continue;
		}
		// 取出工作队列中的元素
		T *request = m_workqueue.front();
		m_workqueue.pop_front();
		m_queuelocker.unlock();
		if(!request){
			continue;
		}
		request->process();
	}
}

#endif

	









