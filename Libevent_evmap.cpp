#ifdef EVMAP_USE_HE
#include "ht-internal.h"
struct event_map_entry;
// 如果定义了EVMAP_USE_HT，则event_io_map为哈希表（hash table），用于fd和I/O事件处理器之间的映射
HT_HEAD(event_io_map, event_map_entry);
#else
#define event_io_map event_signal_map
#endif

struct event_signal_map {
	void** entries;
	int nentries; // size
};

struct event_map_entry {
	HT_ENTRY(event_map_entry) map_node;
	evutil_socket_t fd;
	union {
		struct evmap_io evmap_io;
	}ent;
};

TAILQ_HEAD(event_list, event);

// I/O事件队列
struct evmap_io {
	struct event_list events;
	ev_uint16_t nread;
	ev_uint16_t nwrite;
};

// 信号事件队列
struct evmap_signal {
	struct event_list events;
};

int evmap_io_add(struct event_base* base, evutil_socket_t fd, struct event* ev) {
	const struct eventop* evsel = base->evsel;
	struct event_io_map* io = &base->io; // 获取fd与I/O事件队列的映射表（哈希表或数组）
	// fd对应的I/O事件队列
	struct evmap_io* ctx = NULL;
	int nread, nwrite, retval = 0;
	short res = 0, old = 0;
	struct event* old_ev;

	EVUTIL_ASSERT(fd == ev->ev_fd);

	if (fd < 0)
		return 0;

#ifndef EVMAP_USE_HT
	// 当io.entries中放不下fd了，扩容
	if (fd >= io->nentries) {
		if (evmap_make_space(io, fd, sizeof(struct evmap_io*)) == -1)
			return -1;
	}
#endif

	// 创建ctx
	GET_IO_SLOT_AND_CTOR(ctx, io, fd, evmap_io, evmap_io_init, evsel->fdinfo_len);
	
	nread = ctx->nread;
	nwrite = ctx->nwrite;

	if (nread)
		old |= EV_READ;
	if (nwrite)
		old |= EV_WRITE;

	if (ev->ev_events & EV_READ) {
		if (++nread == 1)
			res |= EV_READ;
	}
	if (ev->ev_events & EV_WRITE) {
		if (++nwrite == 1)
			res |= EV_WRITE;
	}
	if (EVUTIL_UNLIKELY(nread > 0xffff || nwrite > 0xffff)) {
		event_warnx("Too many events reading or writing on fd %d", (int)fd);
		return -1;
	}
	if (EVENT_DEBUG_MODE_IS_ON() &&
		(old_ev == TAILQ_FIRST(&ctx->events)) &&
		(old_ev->ev_events & EV_ET) != (ev->ev_events & EV_ET)) {
		event_warnx("Tried to mix edge-triggered and non-edge-triggered events on fd %d", (int)fd);
		return -1;
	}
	if (res) {
		void* extra = ((char*)ctx) + sizeof(struct evmap_io);
		if (evsel->add(base, ev->ev_fd, old, (ev->ev_events & EV_ET) | res, extra) == -1)
			return -1;
		retval = 1;
	}

	ctx->nread = (ev_uint16_t)nread;
	ctx->nwrite = (ev_uint16_t)nwrite;

	TAILQ_INSERT_TAIL(&ctx->events, ev, ev_io_next);

	return (retval);
}



