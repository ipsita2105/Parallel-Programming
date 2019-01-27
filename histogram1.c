#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

long long int INPUT_SIZE = 10;
#define NUM_BINS  5
#define NUM_THREADS  5
float LBOUND = 0.0;
float UBOUND =  1.0;
float input[10];
long long int Hfinal[5];
long long int Htemp[NUM_THREADS][5];

void generate_input(){

    srand(time(NULL));

    for(long long int i=0; i<INPUT_SIZE; i++){
        input[i] = ((float)rand()/(float)(RAND_MAX))*(UBOUND);
    }


}

int get_bin(float num){

        return (int)(((num - LBOUND)/(UBOUND - LBOUND))*(NUM_BINS));
}

void *calc_hist(void *tnum){

    int thread_num = (int)tnum;
    long long int my_start = (thread_num)*(INPUT_SIZE/NUM_THREADS);
    long long int my_end   = (thread_num + 1)*(INPUT_SIZE/NUM_THREADS);
    int bin_num;
    printf("thread num = %d my_start = %lld my_end = %lld\n", thread_num, my_start, my_end);

    for(long long int i= my_start; i< my_end; i++){
        bin_num = get_bin(input[i]);
        printf("input = %f bin = %d\n",input[i],bin_num);
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


int main(){

    generate_input();
    printf("Input is\n");
    for(long long int i=0; i<INPUT_SIZE; i++){
        printf("in%lli = %f\n",i,input[i]);
    }

    pthread_t h_threads[NUM_THREADS];
    for(int t=0; t<NUM_THREADS; t++){

        pthread_create(&h_threads[t], NULL, calc_hist, (void *)t);

    }

    for(int t=0; t<NUM_THREADS; t++){
        pthread_join(h_threads[t], NULL);
    }

    //calculate final histogram here
    calc_final_histogram();

    for(int b=0 ; b<NUM_BINS; b++){
        printf("B:%i = %lld\n",b, Hfinal[b]);
    }

}