
// POLL
int ret = poll(fds, MAX_EVENT_NUMBER, -1);
// iterate all registered fd, and find the ready ones
for(int i = 0; i < MAX_EVENT_NUMBER; ++i){
	if(fds[i].revents & POLLIN){ // judge if ith fd is ready
		int sockfd = fds[i].fd;
		// handle sockfd
	}
}

// EPOLL
int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
// only iterate ret ready fds
for(int i = 0; i < ret; ++i){
	int sockfd = events[i].data.fd;
	// handle this ready sockfd
}
