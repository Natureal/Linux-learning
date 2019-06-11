#ifndef MIN_HEAP
#define MIN_HEAP

#include <iostream>
#include <netinet/in.h>
#include <time.h>
using std:exception;

#define BUFFER_SIZE 64

class heap_timer;

struct client_data {
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	heap_timer* timer;
};

class heap_timer {
public:
	heap_timer(int delay) {
		expire = time(NULL) + delay;
	}
public:
	time_t expire;
	void(*cb_func)(client_data*);
	client_data* user_data;
};

class time_heap {
public:
	// 初始化1：初始化大小为cap的空堆
	time_heap(int cap) throw(std::exception) : capacity(cap), cur_size(0) {
		array = new heap_timer* [capacity]; // 初始化head_timer指针数组
		if (!array) {
			throw std::exception();
		}
		for (int i = 0; i < capacity; ++i) {
			array[i] = NULL;
		}
	}
	// 初始化2：用已有数组在初始化堆
	time_heap(heap_timer** init_array, int size, int capacity) throw(std::exception) :
		cur_size(size), capacity(capacity) {
		if (capacity < size) {
			throw std::exception();
		}
		array = new heap_timer* [capacity]; // 初始化head_timer指针数组
		if (!array) {
			throw std::exception();
		}
		for (int i = 0; i < capacity; ++i) {
			array[i] = NULL;
		}
		if (size != 0) {
			for (int i = 0; i < size; ++i) {
				array[i] = init_array[i];
			}
			for (int i = (cur_size - 1) / 2; i >= 0; --i) {
				percolate_down(i);
			}
		}
	}
	// 销毁时间堆
	~time_heap() {
		for (int i = 0; i < cur_size; ++i) {
			delete array[i];
		}
		delete[] array;
	}

public:
	/// 添加timer
	void add_timer(heap_timer* timer) throw(std::exception) {
		if (!timer) {
			return;
		}
		if (cur_size >= capacity) { // 如果当前堆数组容量不够，则将其扩大一倍
			resize();
		}
		int hole = cur_size++;
		int parent = 0;
		for (; hole > 0; hole = parent) {
			parent = (hole - 1) / 2;
			if (array[parent]->expire <= timer->expire) {
				break;
			}
			array[hole] = parent;
		}
		array[hole] = timer;
	}
	// 删除timer
	void del_timer(heap_timer* timer) {
		if (!timer) {
			return;
		}
		// 仅将对应的回调函数设置为空，即：延迟销毁，节省真正删除时的开销，但容易使得数组膨胀
		timer->cb_func = NULL;
	}
	// 获取top
	heap_timer* top() const { // const 表示该函数不能改变类成员变量
		if (empty()) {
			return NULL;
		}
		return array[0];
	}
	// 删除top
	void pop_timer() {
		if (empty()) {
			return;
		}
		if (array[0]) {
			delete array[0];
			array[0] = array[--cur_size]; // 0-index, 所以元素编号为 0~cur_size-1
			percolate_down(0); // 对top进行下滤
		}
	}
	// 滴答函数
	void tick() {
		heap_timer* tmp = array[0];
		time_t cur = time(NULL);
		while (!empty()) {
			if (!tmp) {
				break;
			}
			// top没到期
			if (tmp->expire > cur) {
				break;
			}
			// top 到期
			if (array[0]->cb_func) {
				array[0]->cb_func(array[0]->user_data);
			}
			// 删除top，并调整堆
			pop_timer();
			tmp = array[0];
		}
	}
	bool empty() const {
		return cur_size == 0;
	}

private:
	// 下滤操作
	void percolate_down(int hole) {
		heap_timer* tmp = array[hole];
		int child = 0;
		for (; (hole * 2 + 1) <= (cur_size - 1); hole = child) {
			child = hole * 2 + 1;
			if (child < cur_size - 1 && array[child + 1]->expire < array[child]->expire) {
				child++;
			}
			if (array[child]->expire < tmp->expire) {
				array[hold] = array[child];
			}
			else {
				break;
			}
		}
		array[hole] = tmp;
	}
	// 将堆数组容量扩大一倍
	void resize() throw(std::exception) {
		heap_timer** tmp = new heap_timer*[2 * capacity]; // 初始化head_timer指针数组
		for (int i = 0; i < 2 * capacity; ++i) {
			tmp[i] = NULL;
		}
		if (!tmp) {
			throw std::exception();
		}
		capacity = 2 * capacity;
		for (int i = 0; i < cur_size; ++i) {
			tmp[i] = array[i];
		}
		delete[] array;
		array = tmp;
	}

private:
	heap_timer** array; // 堆数组（实际为head_timer的指针数组）
	int capacity;
	int cur_size;

};

#endif