#include <fcntl.h>

int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL); // get status flas of fd
	int new_option = old_option | O_NONBLOCK; // set nonblocking flag
	fcntl(fd, F_SETFL, new_option);
	return old_option; // return old options for recovering in the future
}
