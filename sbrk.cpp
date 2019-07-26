#include <stdio.h>
#include <unistd.h>

// sbrk()：申请一个没有被占用的虚拟内存空间

int main(){
	int *p1 = sbrk(0);
	int *p2 = sbrk(4);
	int *p3 = sbrk(4);
	printf("pid: %d\n", getpid());
	printf("p1: %d\n", p1);
	printf("p2: %d\n", p2);
	printf("p3: %d\n", p3);

	return 0;
}
