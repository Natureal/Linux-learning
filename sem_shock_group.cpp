/*
   This code is for checking whether sem_t in <semaphore.h> has shock-group problem.
*/
#include <stdio.h>
#include <vector>
#include <iostream>
#include <memory>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>
using namespace std;

static sem_t s;

static void* func1(void* arg){
	cout << "func1 starts" << endl;
	sem_wait(&s);
	cout << "func1 holds the semaphore" << endl;
	sleep(2);
	cout << "func1 posts the semaphore" << endl;
	sem_post(&s);
}

static void* func2(void* arg){
	cout << "func2 starts" << endl;
	sem_wait(&s);
	cout << "func2 holds the semaphore" << endl;
	sleep(2);
	cout << "func2 posts the semaphore" << endl;
	sem_post(&s);
}

int main(){
	
	cout << "Main starts" << endl;

	sem_init(&s, 0, 0);

	pthread_t t1, t2;

	cout << "Creating threads" << endl;

	pthread_create(&t1, NULL, func1, NULL);
	pthread_create(&t2, NULL, func2, NULL);

	sleep(2);
	
	cout << "Main posts!" << endl;
	sem_post(&s);
	
	sleep(5);

	cout << "Join" << endl;
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	cout << "end" << endl;
	return 0;
}


