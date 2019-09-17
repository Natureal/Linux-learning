/*
This code is aiming at finding out whether there is a shock-group problem of pthread_mutex
*/
#include <stdio.h>
#include <vector>
#include <iostream>
#include <memory>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void* func1(void* arg){
	cout << "func1 starts" << endl;
	pthread_mutex_lock(&mtx);
	cout << "func1 locks mutex" << endl;
	sleep(2);
	cout << "func1 unlocks mutex" << endl;
	pthread_mutex_unlock(&mtx);
	return NULL;
}

static void* func2(void* arg){
	sleep(1);
	cout << "func2 starts" << endl;
	pthread_mutex_lock(&mtx);
	cout << "func2 locks mutex" << endl;
	sleep(2);
	cout << "func2 unlocks mutex" << endl;
	pthread_mutex_unlock(&mtx);
	return NULL;
}

int main(){

	sleep(1);

	pthread_mutex_lock(&mtx);

	pthread_t t1, t2;

	cout << "Creating threads" << endl;
	pthread_create(&t1, NULL, func1, NULL);
	pthread_create(&t2, NULL, func2, NULL);

	sleep(2);

	cout << "UNLOCK!!!" << endl;
	pthread_mutex_unlock(&mtx);

	sleep(8);

	cout << "join" << endl;
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	cout << "end" << endl;
	return 0;
}
