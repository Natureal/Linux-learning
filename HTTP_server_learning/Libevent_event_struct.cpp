#define TAILQ_ENTRY(type)
struct {
	struct type* tqe_next; // 下一个元素
	struct type** tqe_prev; // 上一个元素的地址
}

// 虽然名字叫event，但实际上是事件处理器
struct event {
	// 所有被激活的事件处理器串联成一个尾队列（活动事件队列），有不同优先级的活动事件队列
	TAILQ_ENTRY(event) ev_active_next;
	// 已经注册的事件处理器串联成一个尾队列（注册事件队列）
	TAILQ_ENTRY(event) ev_next;

	// 一个定时器是否是通用定时器取决于其超时值大小
	union {
		// 对于通用定时器，该变量指出其在尾队列（通用定时器队列）中的位置
		TAILQ_ENTRY(event) ev_next_with_common_timeout;
		// 对于其他定时器，该变量指出其在时间堆中的位置
		int min_heap_idx;
	}ev_timeout_pos; // 仅用于定时事件处理器
	
	evutil_socket_t ev_fd; // fd for I/O， signal value for signals

	struct event_base* ev_base;

	union {
		// 相同fd的I/O事件处理器串联成一个尾队列（I/O事件队列）
		struct {
			TAILQ_ENTRY(event) ev_io_next;
			struct timeval ev_timeout;
		}ev_io;

		// 具有相同信号值的信号事件处理器串联成一个尾队列（信号事件队列）
		struct {
			TAILQ_ENTRY(event) ev_signal_next;
			short ev_ncalls; // Reactor需要执行多少次该事件处理器中的回调函数
			short *ev_pncalls; // NULL 或者指向 ev_ncalls
		}ev_signal;
	}_ev; 

	short ev_events; // 事件类型，标志的按位或（互斥事件除外）
	short ev_res; // 当前激活事件的类型
	short ev_flags; // 一些事件标志，参考如下
	// #define EVLIST_TIMEOUT 0x01 // 从属于通用定时器队列或时间堆
	// #define EVLIST_INSERTED 0x02 // 从属于注册事件队列
	// #define EVLIST_SIGNAL 0x04 // no use
	// #define EVLIST_ACTIVE 0x08 // 从属于活动事件队列
	// #define EVLIST_INTERNAL 0x10 // 内部使用
	// #define EVLIST_INIT 0x80 // 事件处理器已经被初始化
	// #define EVLIST_ALL (0xf000 | 0x9f) // 定义所有状态

	ev_unit8_t ev_pri; // 指定事件处理器优先级，值越小优先级越高
	ev_util8_t ev_closure; // 指定执行回调函数时的行为，选值from event-internal.h
	// 默认行为
	// #define EV_CLOSURE_NONE 0
	// 调用ev.ev_signal.ev_ncalls次
	// #define EV_CLOSURE_SIGNAL 1
	// 执行完回调函数后，再次将事件处理器加入注册事件队列中
	// #define EV_CLOSURE_PERSIST 2

	struct timeval ev_timeout; // 超时值

	void(*ev_callback)(evutil_socket_t, short, void* arg); // 回调函数
	void* ev_arg; // 回调函数的参数
};


// 封装了I/O复用机制必要的一些操作，为event_base支持的所有后端I/O复用机制提供了一个统一的接口
struct eventop {
	const char* name;
	void* (*init)(struct event_base*);
	// 注册事件
	int (*add)(struct event_base*, evutil_socket_t fd, short old, short events, void* fdinfo);
	// 删除事件
	int(*del)(struct event_base*, evutil_socket_t fd, short old, short events, void* fdinfo);
	// 等待事件
	int(*dispatch)(struct event_base*, struct timeval*);
	// 释放I/O复用机制所用的资源
	void(*dealloc)(struct event_base*);
	// fork后之后需要重新初始化event_base
	int need_reinit;
	enum event_method_feature features;
	size_t fdinfo_len;
};

struct event_base {
	// 选择一种I/O复用机制
	const struct eventop* evsel;
	// I/O复用机制真正存储的数据
	void* evbase;
	// 缓冲，利于同个fd上注册的事件被多次修改
	struct event_changelist changelist;
	// 指向信号的后端处理机制
	const struct eventop* evsigsel;
	// 信号事件处理器使用的数据结构，封装了socketpair管道，用于信号处理函数和事件多路分发器之间的通信
	struct evsig_info sig;
	// 添加到该event_base的所有事件数量
	int virtual_event_count;
	int event_count;
	int event_count_active;
	
	// 是否执行完活动事件队列上剩余任务后就退出事件循环
	int event_gotterm;
	
	// 是否立即退出循环
	int event_break;
	
	// 是否启动新的事件循环
	int event_continue;
	
	// 目前正在处理的活动事件队列的优先级
	int event_running_priority;
	
	// 事件循环是否已经启动
	int running_loop;
	
	// 活动事件队列，索引值越小，优先级越高
	struct event_list* activequeues;
	
	// 不同优先级的队列个数
	int nactivequeues;

	// 通用定时器队列
	struct common_timeout_list** common_timeout_queues;
	int n_common_timeouts;
	int n_common_timeouts_allocated;
	// 存放延迟回调函数的链表，每成功处理完一个活动事件队列的所有事件后，就调用一次延迟回调函数
	struct deferred_cb_queue defer_queue;

	// mappings between fds and I/O events
	struct event_io_map io;
	// mappings between signal values and signal events
	struct event_signal_map sigmap;
	// 注册事件队列
	struct event_list eventqueue;
	struct min_heap timeheap;

	struct timeval event_tv;
	struct timeval tv_cache;
#if defined(_EVENT_HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	struct timeval tv_clock_diff;
	time_t last_updated_clock_diff;
#endif

// 多线程支持
#ifndef _EVENT_DISABLE_THREAD_SUPPORT
	unsigned long th_owner_id; // 运行该event base的事件循环的线程
	void* th_base_lock; // 对event base的独占锁
	// 当前执行那个事件处理器的回调函数
	struct event *current_event;
	// 用来唤醒正在等待的线程
	void *current_event_cond;
	int current_event_waiters; // 等待的线程数
#endif
	
#ifdef WIN32
	struct event_iocp_port *iocp;
#endif

	// event base的配置参数
	enum event_base_config_flag flags;
	// 下面这组成员变量给工作线程唤醒主线程提供了方法（用socketpair）
	int is_notify_pending;
	evutil_socket_t th_notify_fd[2];
	struct event th_notify;
	int(*th_notify_fn)(struct event_base *bae);

};
