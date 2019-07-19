#include <unistd.h>
#include <stdio.h>

int main(){
	pid_t pid;
	printf("fork from pid: %d\n", getpid());
	int val = 0;

	pid = fork();
	if(pid == -1){
		perror("fork failed");
		return 1;
	}
	else if(pid == 0){
		val++;
		printf("child, pid: %d, ppid:%d, val:%d\n", getpid(), getppid(), val);
	}
	else if(pid > 0){
		val++;
		printf("parent, pid: %d, val:%d\n", getpid(), val);
		sleep(1);
	}

	return 0;
}
