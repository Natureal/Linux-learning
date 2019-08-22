// Reference: APUE
#include <pthread.h>
struct msg{
	struct msg *m_next;
	int info;
	/* .. more stuff ..*/
};

struct msg *work_queue;
pthread_cond_t q_ready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;

void pop_front_msg(){
	struct msg *m;
	while(true){
		pthread_mutex_lock(&q_lock);
		while(work_queue == NULL)
			pthread_cond_wait(&q_ready, &q_lock);
		m = work_queue;
		work_queue = m->m_next;
		pthread_mutex_unlock(&q_lock);
		// process the message *m
	}
}

void push_front_msg(struct msg *m){
	pthread_mutex_lock(&q_lock);
	// insert into the front
	m->m_next = work_queue;
	work_queue = m;
	pthread_mutex_unlock(&q_lock);
	pthread_cond_signal(&q_ready);
}


