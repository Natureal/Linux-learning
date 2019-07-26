#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

int main(){
	int *p0 = (int*)sbrk(0);
	int *p1 = (int*)malloc(500);
	int *p2 = (int*)mmap(NULL, 30, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
	int local = 111;
	int *p3 = (int*)mmap(NULL, 4097, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
	int *p4 = (int*)mmap(NULL, 4096, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
	printf("brk start, p0: %p\n", p0);
	printf("malloc 500B, p1: %p\n", p1);
	printf("mmap 30B, p1: %p\n", p2);
	printf("mmap 4KB, p2: %p\n", p3);
	printf("mmap 4KB, p3: %p\n", p4);
	printf("local address: %p\n", &local);
	return 0;
}
