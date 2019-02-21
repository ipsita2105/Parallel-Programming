#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

#define INPUT_SIZE 1000000
#define NUM_BINS 100
#define LBOUND 0.0
#define UBOUND 10.0
long long int Hfinal[NUM_BINS];

int get_bin(float num){

        return (int)(((num - LBOUND)/(UBOUND - LBOUND))*(NUM_BINS));
}

void calc_hist(){

    //srand(time(NULL));
    int bin_num;
    for(long long int i= 0; i< INPUT_SIZE; i++){
        bin_num = get_bin(((float)rand()/(float)(RAND_MAX))*(UBOUND));
        Hfinal[bin_num] += 1;
    }
}

int main(){

    clock_t begin = clock();

    calc_hist();

    for(int b=0 ; b<NUM_BINS; b++){
        printf("B:%i = %lld\n",b, Hfinal[b]);
    }

    clock_t end = clock();
    printf("Time = %f s\n",(double)(end-begin)/CLOCKS_PER_SEC);

}
