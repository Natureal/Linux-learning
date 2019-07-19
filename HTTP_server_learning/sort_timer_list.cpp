#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#define BUFFER_SIZE 64
class until_timer;

struct client_data{
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	util_timer* timer;
};

// timer
class util_timer{
public:
	util_timer(): prev(NULL), next(NULL){}

public:
	time_t expire; // 任务超时（绝对）时间
	void (*cb_func)(client_data*); // 任务回调函数
	client_data* user_data;
	util_timer* prev;
	util_timer* next;
};

class sort_timer_list {
public:
	sort_timer_list() : head(NULL), tail(NULL) {}
	// 链表被销毁时，删除其中所有的定时器
	~sort_timer_list() {
		util_timer* tmp = head;
		while (tmp) {
			head = tmp->next;
			delete tmp;
			tmp = head;
		}
	}
	// 将目标定时器timer添加到链表中
	void add_timer(util_timer* timer) {
		if (!timer) {
			return;
		}
		if (!head) {
			head = tail = timer;
			return;
		}
		if (timer->expire < head->expire) {
			timer->next = head;
			head->prev = timer;
			head = timer;
			return;
		}
		add_timer(timer, head);
	}
	// 调整定时器状态，只考虑时间延长的情况
	void adjust_timer(util_timer* timer) {
		if (!timer) {
			return;
		}
		util_timer* tmp = timer->next;
		if (!tmp || (timer->expire < tmp->expire)) {
			return;
		}

		if (timer == head) {
			head = head->next;
			head->prev = NULL;
			timer->next = NULL;
			add_timer(timer, head);
		}
		else {
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
			add_timer(timer, timer->next);
		}
	}
	// 删除timer
	void del_timer(util_timer* timer) {
		if (!timer) {
			return;
		}
		// timer是头部，且只有一个定时器
		if ((timer == head) && (timer == tail)) {
			delete timer;
			head = tail = NULL;
			return;
		}
		// timer是头部，且至少有两个定时器
		if (timer == head) {
			head = head->next;
			head->prev = NULL;
			delete timer;
			return;
		}
		// timer是尾部，且至少有两个定时器
		if (timer == tail) {
			tail = tail->prev;
			tail->next = NULL;
			delete timer;
			return;
		}
		// 如果定时器在链表中间
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		delete timer;
	}
	// 每次SIGALRM信号被触发，就在其信号处理函数中执行一次tick函数，以处理链表上到期的任务
	void tick() {
		if (!head) {
			return;
		}
		printf("timer tick\n");
		time_t cur = time(NULL); // current system time
		util_timer* tmp = head;
		while (tmp) {
			// 都使用绝对时间
			if (cur < tmp->expire) {
				break;
			}
			tmp->cb_func(tmp->user_data);
			// 执行完定时器中的定时任务后，就将其删除
			head = tmp->next;
			if (head) {
				head->prev = NULL;
			}
			delete tmp;
			tmp = head;
		}
	}

private:
	// 重载的辅助函数，被add_timer和adjust_timer函数调用
	// 将timer添加到节点list_head之后的链表中
	void add_timer(util_timer* timer, util_timer* list_head) {
		util_timer* prev = list_head;
		util_timer* tmp = prev->next;
		while (tmp) {
			if (timer->expire < tmp->expire) {
				prev->next = timer;
				timer->next = tmp;
				tmp->prev = timer;
				timer->prev = prev;
				break;
			}
			prev = tmp;
			tmp = tmp->next;
		}
		// 遍历完还没插入，则插入到尾部
		if (!tmp) {
			prev->next = timer;
			timer->next = NULL;
			timer->prev = prev;
			tail = timer;
		}
	}

private:
	util_timer* head;
	util_timer* tail;
};

#endif
			

