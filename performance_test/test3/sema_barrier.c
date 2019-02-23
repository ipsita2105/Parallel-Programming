#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<pthread.h>
#include<time.h>
#include<semaphore.h>
#include<unistd.h>

#define NUM_THREADS 10
#define BARRIER_COUNT 100

int counter;
sem_t barrier_sems[BARRIER_COUNT];
sem_t count_sem;

void* thread_fun(void* tnum){

	int my_rank = (int)tnum;
	
	//imaginary work here

	for(int i=0; i< BARRIER_COUNT; i++){

		sem_wait(&count_sem);

		if(counter == NUM_THREADS -1){
			counter = 0;
			sem_post(&count_sem);

			for(int j=0; j<NUM_THREADS-1; j++){
				sem_post(&barrier_sems[i]);
			}
		}else{
		
		
			counter++;
			sem_post(&count_sem);
			sem_wait(&barrier_sems[i]);
		}

		if(my_rank == 0){
			printf("All threads reached barrier %d\n",i);
		}
	}

}

void main(){

	for(int i=0; i< BARRIER_COUNT; i++){
	
		sem_init(&barrier_sems[i], 0, 0);
	}

	sem_init(&count_sem, 0, 1);

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

	for(int i=0; i< BARRIER_COUNT; i++){
		sem_destroy(&barrier_sems[i]);
	}

}
