#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<pthread.h>
#include<time.h>
#include<semaphore.h>
#include<unistd.h>

int NUM_THREADS;
#define BARRIER_COUNT 100

int thread_count = 0;
pthread_mutex_t barrier_mutex;
pthread_cond_t cond_var;

void* thread_fun(void* tnum){

	int my_rank = (int)tnum;
	
	//imaginary work here

	for(int i=0; i< BARRIER_COUNT; i++){
		pthread_mutex_lock(&barrier_mutex);
		thread_count++;

		if(thread_count == NUM_THREADS){

			thread_count = 0;
			pthread_cond_broadcast(&cond_var);

		}else{
		
			while(pthread_cond_wait(&cond_var, &barrier_mutex) != 0);	
		}

		pthread_mutex_unlock(&barrier_mutex);

	}

}

void main(char argc, char* argv[]){
	
	NUM_THREADS = atoi(argv[1]);

	pthread_mutex_init(&barrier_mutex, NULL);
	pthread_cond_init(&cond_var, NULL);

	clock_t begin = clock();

	pthread_t work_threads[NUM_THREADS];
        for(int t=0; t<NUM_THREADS; t++){

                pthread_create(&work_threads[t], NULL, thread_fun, (void *)t);
        }

        for(int t=0; t<NUM_THREADS; t++){
                pthread_join(work_threads[t], NULL);
        }

	clock_t end = clock();
	printf("%f\n",(double)(end-begin)/CLOCKS_PER_SEC);

	pthread_mutex_destroy(&barrier_mutex);
	pthread_cond_destroy(&cond_var);
}
