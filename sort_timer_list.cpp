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

class sort_timer_list{
public:
	sort_timer_list(): head(NULL), tail(NULL) {}
	// 链表被销毁时，删除其中所有的定时器
	~sort_timer_list(){
		util_timer* tmp = head;
		while(tmp){
			head = tmp -> next;
			delete tmp;
			tmp = head;
		}
	}
	// 将目标定时器timer添加到链表中
	void add_timer(util_timer* timer){
		if(!timer){
			return;
		}
		if(!head){
			head = tail = timer;
			return;
		}
		if(timer->expire < head->expire){
			timer->next = head;
			head->prev = timer;
			head = timer;
			return;
		}
		add_timer(timer, head);
	}
	// 调整定时器状态，只考虑时间延长的情况
	void adjust_timer(util_timer* timer){
		if(!timer){
			return;
		}
		util_timer* tmp = timer->next;
		if(!tmp || (timer->expire < tmp->expire)){
			return;
		}

		if(timer == head){
			head = head->next;
			head->prev = NULL;
			timer->next = NULL;
			add_timer(timer, head);
		}
		else{
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
			add_timer(timer, timer->next);
		}
	}
	// 删除timer
	void del_timer(util_timer* timer){
		if(!timer){
			return;
		}
		if((timer == head) && (timer == tail)){
			delete timer;
			head = tail = NULL;
			return;
		}
		if(timer == head){
			head = head -> next;
			head->prev = NULL;
			delete timer;
			return;
		}
		if(timer == tail){
			tail = tail->prev;
			tail->next = NULL;
			delete timer;
			return;
		}
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		delete timer;
	}

			


