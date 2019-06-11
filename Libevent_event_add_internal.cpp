static inline int event_add_internal(struct event* ev, const struct timeval* tv, int tv_is_absolute) {
	struct event_base* base = ev->ev_base;
	int res = 0;
	int notify = 0;

	EVENT_BASE_ASSERT_LOCKED(base);
	_event_debug_assert_is_setup(ev);

	event_debug((
		"event_add: event: %p (fd %d), %s%s%scall %p",
		ev, (int)ev->ev_fd, ev->ev_events & EV_READ ? "EV_READ " : " ",
		ev->ev_events & EV_WRITE ? "EV_WRITE " : " ",
		tv ? "EV_TIMEOUT " : " ",
		ev->ev_callback));
	
	EVUTIL_ASSERT(!(ev->ev_flags & ~EVLIST_ALL));

	// 如果新添加的是定时器，且尚未被添加到通用定时器队列或时间堆中，则为其在时间堆上预留一个位置
	if (tv != NULL && !(ev->ev_flags & EVLIST_TIMEOUT)) {
		if (min_heap_reserve(&base->timeheap, 1 + min_heap_size(&base->timeheap)) == -1)
			return -1;
	}

	/* 如果当前调用者不是主线程（执行事件循环的线程），且该事件处理器为信号事件处理器，并且主线程
	 此时正在执行该信号处理器的回调函数，则当前调用者必须等待主线程完成调用，避免竞态条件 */
#ifndef _EVENT_DISABLE_THREAD_SUPPORT
	if (base->current_event == ev && (ev->ev_events & EV_SIGNAL) && !EVBASE_IN_THREAD(base)) {
		++base->current_event_waiters;
		EVTHREAD_COND_WAIT(base->current_event_cond, base->th_base_lock);
	}
#endif

	if ((ev->ev_events & (EV_READ | EV_WRITE | EV_SIGNAL)) &&
		!(ev->ev_flags & (EVLIST_INSERTED | EVLIST_ACTIVE))) {
		if (ev->ev_events & (EV_READ | EV_WRITE))
			// 添加I/O事件和I/O事件处理器的映射关系
			res = evmap_io_add(base, ev->ev_fd, ev);
		else if (ev->ev_events & EV_SIGNAL)
			// 添加信号事件和信号事件处理器的映射关系
			res = evmap_signal_add(base, (int)ev->ev_fd, ev);
		if (res != -1)
			// 将事件处理器插入注册事件队列
			event_queue_insert(base, ev, EVLIST_INSERTED);
		if (res == 1) {
			// 事件多路分发器中添加了新的事件，通知主线程
			notify = 1;
			res = 0;
		}
	}

	// 将事件处理器添加至通用定时队列或者时间堆中
	if (res != -1 && tv != NULL) {
		struct timeval now;
		int common_timeout;
		
		// 对于永久性事件处理器，且其超时时间不是绝对时间
		if (ev->ev_closure == EV_CLOSURE_PERSIST && !tv_is_absolute) {
			ev->ev_io_timeout = *tv;
		}

		// 如果该事件处理器已经被插入通用定时器队列或时间堆中，则删除它
		if (ev->ev_flags & EVLIST_TIMEOUT) {
			if (min_heap_elt_is_top(ev))
				notify = 1;
			event_queue_remove(base, ev, EVLIST_TIMEOUT);
		}

		// 如果待添加的事件处理器已经被激活，且原因是超时，则从活动事件队列中删除它，以避免回调函数执行。
		// 对于信号事件处理器，必要时还需将其ncalls成员设置为0
		if ((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) {
			if (ev->ev_events & EV_SIGNAL) {
				if (ev->ev_ncalls && ev->ev_pncalls) {
					*ev->ev_pncalls = 0;
				}
			}
			event_queue_remove(base, ev, EVLIST_ACTIVE);
		}

		gettime(base, &now);

		common_timeout = is_common_timeout(tv, base);
		if (tv_is_absolute) {
			ev->ev_timeout = *tv;
		}
		else if (common_timeout) {
			struct timeval tmp = *tv;
			tmp.tv_usec &= MICROSECONDS_MASK;
			evutil_timeradd(&now, &tmp, &ev->ev_timeout);
			ev->ev_timeout.tv_usec |= (tv->tv_usec & ~MICROSECONDS_MASK);
		}
		else {
			// 加上当前系统时间，以取得超时绝对时间
			evutil_timeradd(&now, tv, &ev->ev_timeout);
		}

		event_debug((
			"event_add: timeout in %d seconds, call %p",
			(int)tv->tv_sec, ev->ev_callback));
		event_queue_insert(base, ev, EVLIST_TIMEOUT); // 插入定时器
		// 如果被插入的事件处理器是通用定时器队列中的第一个元素，则通过common_timeout_schedule将其
		// 转移到时间堆中，这样通用定时器链表和时间堆中的定时器就得到了统一处理
		if (common_timeout) {
			struct common_timeout_list *ctl = get_common_timeout_list(base, &ev->ev_timeout);
			if (ev == TAILQ_FIRST(&ctl->events)) {
				common_timeout_schedule(ctl, &now, ev);
			}
		}
		else {
			if (min_heap_etl_is_top(ev)) {
				notify = 1;
			}
		}

		// if necessary, wake up main thread
		if (res != -1 && notify && EVBASE_NEED_NOTIFY(base))
			evthread_notify_base(base);

		_event_debug_note_add(ev);

		return (res);

	}
}

// 将事件处理器添加到各种事件队列中，如
// I/O事件处理器，信号事件处理器插入注册事件队列
// 定时器插入通用定时器队列或时间堆
// 被激活的事件处理器添加到活动事件队列
static void event_queue_insert(struct event_base* base, struct event* ev, int queue) {
	EVENT_BASE_ASSERT_LOCKED(base);
	// 避免重复插入
	if (ev->ev_flags & queue) {
		// double insertion is possible for active events
		if (queue & EVLIST_ACTIVE) {
			return;
		}
		event_errx(1, "%s: %p(fd %d) already on queue %x", __func__, ev, ev->ev_fd, queue);
		return;
	}
	if (~ev->ev_flags & EVLIST_INTERNAL)
		base->event_count++; // event base 拥有的事件处理器总数+1

	ev->ev_flags |= queue; // 标记此事件已被添加过

	switch (queue) {
	case EVLIST_INSERTED:
		TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
		break;
	case EVLIST_ACTIVE:
		base->event_count_active++;
		TAILQ_INSERT_TAIL(&base->activequeue[ev->ev_pri], ev, ev_active_next);
		break;
	case EVLIST_TIMEOUT: {
		if (is_common_timeout(&ev->ev_timeout, base)) {
			struct common_timeout_list *ctl = get_common_timeout_list(base, &ev->ev_timeout);
			insert_common_timeout_inorder(ctl, ev);
		}
		else {
			min_heap_push(&base->timeheap, ev);
		}
		break;
	}
	default:
		event_errx(1, "%s: unknown queue %x", __func__, queue);
	}
}