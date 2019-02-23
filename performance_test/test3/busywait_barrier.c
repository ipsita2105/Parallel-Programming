#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>

#define NUM_THREADS 10
#define BARRIER_COUNT 100

int thread_counts[BARRIER_COUNT];
pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_fun(void* tnum){

	int my_rank = (int)tnum;
	
	//imaginary work here

	for(int i=0; i< BARRIER_COUNT; i++){

	    	pthread_mutex_lock(&barrier_mutex);
	    	      thread_counts[i]++;
	    	pthread_mutex_unlock(&barrier_mutex);

	    	
	    	while(thread_counts[i] < NUM_THREADS);

	    	if(my_rank ==0){
	    		printf("All threads reached barrier %d\n", i);
	    	}

	}

}

void main(){

	clock_t begin = clock();

	pthread_t work_threads[NUM_THREADS];
        for(int t=0; t<NUM_THREADS; t++){

                pthread_create(&work_threads[t], NULL, thread_fun, (void *)t);
        }

        for(int t=0; t<NUM_THREADS; t++){
                pthread_join(work_threads[t], NULL);
        }

	clock_t end = clock();
	printf("Time = %f s\n",(double)(end-begin)/CLOCKS_PER_SEC);

}
