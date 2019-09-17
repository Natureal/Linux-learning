#include <iostream>
#include <pthread.h>
#include <thread>
#include <atomic>
using namespace std;

atomic<int> num(0);

void run(){
	for(int i = 0; i < 10000000; ++i){
		num++;
	}
}

int main(){

	cout << "CPU core(s): " << thread::hardware_concurrency() << endl;
	
	thread t1(run);
	thread t2(run);

	t1.join();
	t2.join();

	cout << "num: " << num << endl;

	return 0;
}
