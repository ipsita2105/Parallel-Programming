#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

long long int N = 100000000;
double PSUM = 0;
double NSUM = 0;

void *fun_psum(){

    long long int my_N = N/2;
    long long int i;

    for(i=0; i< my_N; i++){

        PSUM += (1.0/(5.0+4.0*i));
    }

}

void *fun_nsum(){

    long long int my_N = N/2;
    long long int i;

    for(i=0; i< my_N; i++){
        NSUM += (1.0/(3.0+4.0*i));
    }

}

void main(){

    pthread_t thread_psum, thread_nsum;

    pthread_create(&thread_psum, NULL, fun_psum, NULL);
    pthread_create(&thread_nsum, NULL, fun_nsum, NULL);

    pthread_join(thread_psum, NULL);
    pthread_join(thread_nsum, NULL);

    double final_sum ;
    final_sum = 4.0*(1 + PSUM - NSUM);
    printf("Value = %lf\n",final_sum);

}