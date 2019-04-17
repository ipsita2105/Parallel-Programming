#include<stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h> 

int main(){

    char *myfifo = "/tmp/g2";
    int fd;
    int t=0;
    int temp;
    int position = 0;
    mkfifo(myfifo, 0666);

    int finish= 30;

    while(1){

        //wait for god
        fd = open(myfifo, O_RDONLY);
        read(fd, &temp, sizeof(temp));
        close(fd);

        position += 1;
        
        if(temp >= 0){
            position = temp;
        }

        //signal god
        fd = open(myfifo, O_WRONLY);
        write(fd, &position, sizeof(position));
        close(fd);

        if(position >= finish){
            printf("Turtle finishes\n");
            break;
        }

    }
}