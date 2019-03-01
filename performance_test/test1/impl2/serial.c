#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

long long int INPUT_SIZE;
#define NUM_THREADS 4
#define NUM_BINS 100
#define LBOUND 0.0
#define UBOUND 10.0
long long int Hfinal[NUM_BINS];
long long int Htemp[NUM_THREADS][NUM_BINS];


int get_bin(float num){

        return (int)(((num - LBOUND)/(UBOUND - LBOUND))*(NUM_BINS));
}

void *calc_hist(void *tnum){

    int thread_num = (int)tnum;
    long long int my_start = (thread_num)*(INPUT_SIZE/NUM_THREADS);
    long long int my_end   = (thread_num + 1)*(INPUT_SIZE/NUM_THREADS);
    int bin_num;

    srand(time(NULL));
    for(long long int i= my_start; i< my_end; i++){
        bin_num = get_bin(((float)rand()/(float)(RAND_MAX))*(UBOUND));
        Htemp[thread_num][bin_num] += 1;
    }

}

void calc_final_histogram(){

    for(int b=0; b<NUM_BINS; b++){
        Hfinal[b] = 0;
        for(int t=0; t<NUM_THREADS; t++){
            Hfinal[b] += Htemp[t][b];
        }
    }

}


int main(int argc, char* argv[]){

    INPUT_SIZE  = atoll(argv[1]);
    clock_t begin = clock();

    pthread_t h_threads[NUM_THREADS];
    for(int t=0; t<NUM_THREADS; t++){

        pthread_create(&h_threads[t], NULL, calc_hist, (void *)t);

    }

    for(int t=0; t<NUM_THREADS; t++){
        pthread_join(h_threads[t], NULL);
    }

    //calculate final histogram here
    calc_final_histogram();

   /* for(int b=0 ; b<NUM_BINS; b++){
        printf("B:%i = %lld\n",b, Hfinal[b]);
    }
   */

    clock_t end = clock();
    printf("%f\n",(double)(end-begin)/CLOCKS_PER_SEC);

}
