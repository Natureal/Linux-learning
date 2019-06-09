#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class tw_timer;

// 绑定timer和socket
struct client_data {
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	tw_timer* timer;
};

// 定时器类
class tw_timer {
public:
	tw_timer(int rot, int ts) :
		next(NULL), prev(NULL), rotation(rot), time_slot(ts) {}
public:
	int rotation; // 记录定时器在时间轮转多少圈后生效
	int time_slot; // 属于哪个时间槽
	void(*cb_func)(client_data);
	client_data* user_data;
	tw_timer* prev;
	tw_timer* next;
};

class time_wheel {
public:
	time_wheel() : cur_slot(0) {
		for (int i = 0; i < N; ++i) {
			slots[i] = NULL; // 初始化每个槽
		}
	}
	~time_wheel() {
		for (int i = 0; i < N; ++i) {
			tw_timer* tmp = slots[i];
			while (tmp) {
				slots[i] = tmp->next;
				delete tmp;
				tmp = slots[i];
			}
		}
	}
	// 创建定时器，并插入合适的槽
	tw_timer* add_timer(int timeout) {
		if (timeout < 0) {
			return NULL;
		}
		// ticks：时间轮转动多少个si后触发
		// 向下取整，除非timeout < si
		if (timeout < SI) {
			ticks = 1;
		}
		else {
			ticks = timeout / SI;
		}
		int rotation = ticks / N;
		int ts = (cur_slot + (ticks % N)) % N;
		tw_timer* timer = new tw_timer(rotation, ts);
		if (!slots[ts]) { // 作为头
			printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
			slots[ts] = timer;
		}
		else { // 插入
			timer->next = slots[ts];
			slots[ts]->prev = timer;
			slots[ts] = timer;
		}
		return timer;
	}
	void del_timer(tw_timer* timer) {
		if (!timer) {
			return;
		}
		int ts = timer->time_slot;
		if (timer == slots[ts]) { // 如果timer就是该ts的头
			slots[ts] = slots[ts]->next;
			if (slots[ts]) {
				slots[ts]->prev = NULL;
			}
			delete timer;
		}
		else {
			timer->prev->next = timer->next;
			if (timer->next) {
				timer->next->prev = timer->prev;
			}
			delete timer;
		}
	}
	// 每经过si时间，时间轮向前滚动一个time slot
	void tick() {
		tw_timer* tmp = slots[cur_slot];
		printf("current slot is %d\n", cur_slot);
		while (tmp) {
			printf("tick the timer once\n");
			if (tmp->rotation > 0) {
				tmp->rotation--;
				tmp = tmp->next;
			}
			else {
				tmp->cb_func(tmp->user_data);
				if (tmp == slots[cur_slot]) {
					printf("delete header in cur_slot %d\n", cur_slot);
					slots[cur_slot] = tmp->next;
					if (slots[cur_slot]) {
						slots[cur_slot]->prev = NULL;
					}
					delete tmp;
					tmp = slots[cur_slot];
				}
				else {
					tmp->prev->next = tmp->next;
					if (tmp->next) {
						tmp->next->prev = tmp->prev;
					}
					tw_timer* tmp2 = tmp->next;
					delete tmp;
					tmp = tmp2;
				}
			}
		}
		cur_slot = ++cur_slot % N;
	}
private:
	static const int N = 60;
	static const int SI = 1;
	tw_timer* slots[N];
	int cur_slot;
};

#endif