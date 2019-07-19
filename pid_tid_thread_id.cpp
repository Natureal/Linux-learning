#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
using namespace std;

void *hello(void *str)
{
    printf("child, pthread_self()=%lu, syscall(SYS_gettid)=%d\n",pthread_self(),syscall(SYS_gettid));
    printf("child, getpid()=%d\n",getpid());
}

int main(int argc, char *argv[])
{
    pthread_t thread_id;
    pthread_create(&thread_id,NULL,hello,NULL);
    printf("parent, pthread_self()=%lu, syscall(SYS_gettid)=%d\n",pthread_self(),syscall(SYS_gettid));
    printf("parent, getpid()=%d\n",getpid());
	printf("pthread_t thread_id=%lu\n", thread_id);
    pthread_join(thread_id,NULL);
    return 0;
}
