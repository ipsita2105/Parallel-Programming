#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

#define FINISH 100

sem_t sem_hare1;
sem_t sem_hare2;
sem_t sem_turtle1;
sem_t sem_turtle2;
sem_t sem_repoter;

int p1=0;
int p2=0;

int p1new, p2new, change_time;

void *fun_hare(){

    int t=0;
    int s =0;
    int slept = 0;
    int count = 1;

    srand(time(NULL));
    int sleeptime = rand()%FINISH + 1;
    printf("Randomly generated sleep time for hare= %d\n", sleeptime);
    int sleep_end_time;

    while(1){

        t += 1;

        if(t == change_time){
            p1 = p1new;
        }

        else{

            if(!s){
                //update position
                p1 += count;
                count += 1;
            }

            if(t >= sleep_end_time){
                s= 0;
            }

            if(!slept){

                if(t == 5){
                    s = 1;
                    slept = 1;
                    sleep_end_time = t+ sleeptime;
                }
            }
        }

        //signal god_hare_thread
        sem_post(&sem_hare1);

        //wait for signal from god
        sem_wait(&sem_hare2);

    }
}

void *fun_turtle(){

    int t= 0;
    while(1){

            t += 1;
            //wait for signal from god
            sem_wait(&sem_turtle1);

            p2 += 1;

            if(t == change_time){
                p2 = p2new;
            }

            //signal god
            sem_post(&sem_turtle2);

    }

}

void *fun_repoter(){

    while(1){

           sem_wait(&sem_repoter); 

           printf("p1 = %d p2 = %d\n", p1, p2);

            if(p1 >= FINISH){
                printf("Hare won\n");
                printf("p1 = %d p2 = %d\n", p1, p2);
                break;
            }
            else if(p2 >= FINISH){
                printf("Turtle won\n");
                printf("p1 = %d p2 = %d\n", p1, p2);
                break;
            }
    }

}


void *fun_god(){

    printf("Enter time at which positions will be randomly changed\n");
    scanf("%d",&change_time);

    p1new = rand()%FINISH + 1;
    p2new = rand()%FINISH + 1;

    printf("God changes positions to- (Random)\n");
    printf("p1new_hare= %d p2new_turtle= %d\n", p1new, p2new);

    while(1){

            //wait for signal from hare
            sem_wait(&sem_hare1);

            //signal turtle
            sem_post(&sem_turtle1);

            sem_wait(&sem_turtle2);

            //send signal to repoter
            sem_post(&sem_repoter);

            sem_post(&sem_hare2);
    }

}

int main(){

    sem_init(&sem_hare1, 0, 0);
    sem_init(&sem_hare2, 0, 0);

    sem_init(&sem_turtle1, 0, 0);
    sem_init(&sem_turtle2, 0, 0);

    sem_init(&sem_repoter, 0, 0);

    pthread_t thread_hare, thread_god, thread_turtle, thread_repoter;

    pthread_create(&thread_hare,    NULL, fun_hare,    NULL);
    pthread_create(&thread_turtle,  NULL, fun_turtle,  NULL);
    pthread_create(&thread_repoter, NULL, fun_repoter, NULL);
    pthread_create(&thread_god,     NULL, fun_god,     NULL);

    pthread_join(thread_repoter, NULL);
    
}
