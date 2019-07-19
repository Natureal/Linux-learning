#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
static int pipefd[2];

int setnonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void addfd(int epollfd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

// signal processing func
void sig_handler(int sig) {
	// remain original errno, and restore it at last to guarantee reentrancy
	int save_errno = errno;
	int msg = sig;
	send(pipefd[1], (char*)&msg, 1, 0); // write the signal into pipe to inform main cycle
	errno = save_errno;
}

// set signal-processing func
void addsig(int sig) {
	struct sigaction sa;
	memest(&sa, '\0', sizeof(sa));
	sa.sa_handler |= SA_RESTART;
	sigfillset(&sa.sa_mask); // set all signals in the signal set
	assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[]) {
	if (argc <= 2) {
		printf("usage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	
	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	if (ret == -1) {
		printf("errno is %d\n", errno);
		return 1;
	}
	
	ret = listen(listenfd, 5);
	assert(ret != -1);

	epoll_event events[MAX_EVENT_NUMBER]; // epoll event table
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd);

	// use socketpair func to create a pipe, registerring readable event on pipefd[0]
	ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
	assert(ret != -1);
	setnonblocking(pipefd[1]);
	addfd(epollfd, pipefd[0]);

	// set some signal-processing funcs
	addsig(SIGHUP); // 控制终端挂起
	addsig(SIGCHLD); // 子进程状态发生变化（退出或暂停）
	addsig(SIGTERM); // 终止进程
	addsig(SIGINT); // 键盘输入以中断进程（Ctrl+C）
	bool stop_server = false;

	while (!stop_server) {
		int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if ((number < 0) && (errno != EINTR)) {
			printf("epoll failure\n");
			break;
		}
		for (int i = 0; i < number; ++i) {
			int sockfd = events[i].data.fd;
			// if the ready fd is listenfd, accept a new connection
			if (sockfd == listenfd) {
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				int connfd = accept(listenfd, (struct sockaddr*)&client_address,
					&client_addrlength);
				addfd(epollfd, connfd);
			}
			// if the ready fd is pipefd[0], handle the signal
			else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
				int sig;
				char signals[1024];
				ret = recv(pipefd[0], signals, sizoef(signals), 0);
				if (ret == -1) {
					continue;
				}
				else if (ret == 0) {
					continue;
				}
				else {
					// receive signals by byte
					for (int i = 0; i < ret; ++i) {
						switch (signals[i]) {
						case SIGCHLD:
						case SIGHUP: {
							continue;
						}
						case SIGTERM:
						case SIGINT: {
							stop_server = true;
						}
					}
				}
			}
			else {
			
			}
		}
	}
	
	printf("close fds\n");
	close(listenfd);
	close(pipefd[1]);
	close(pipefd[0]);
	
	return 0;
}

