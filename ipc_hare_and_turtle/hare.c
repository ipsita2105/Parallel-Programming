#include<stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h> 

int main(){

    char *myfifo = "/tmp/g1";
    int fd;
    int t=0, temp;
    int position = 0;
    mkfifo(myfifo, 0666);

    int sleep = 0;
    int slept = 0;
    int count = 1;

    srand(time(NULL));
    int sleeptime = rand()%20 + 1;
    printf("sleep time= %d\n", sleeptime);
    int sleep_end_time;

    
    int finish = 30;

       while(1){

        t += 1;

        if(!sleep){
            position += count;
            count += 1;
        }

        if(t >= sleep_end_time){
            sleep = 0;
        }

        if(!slept){

            //TODO: Make it relative
            if(t == 5){
                printf("slept\n");
                sleep= 1;
                slept= 1;
                sleep_end_time = t + sleeptime;
            }
        }

        //signal god
        fd = open(myfifo, O_WRONLY);
        write(fd, &position, sizeof(position));
        close(fd);

        //wait for god
        fd = open(myfifo, O_RDONLY);
        read(fd, &temp, sizeof(temp));
        close(fd);

        if(temp >= 0){
            position = temp;
        }

        if(position >= finish){
            printf("Hare finished\n");
            break;
        }

    }
}