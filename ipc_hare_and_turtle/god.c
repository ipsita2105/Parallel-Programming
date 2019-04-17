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
    char *myfifo2 = "/tmp/g2";
    char *myfifo3 = "/tmp/repo";
    int fd1, fd2, rd;
    int p1, p2;
    int msg[2];
    mkfifo(myfifo, 0666);
    mkfifo(myfifo2, 0666);
    mkfifo(myfifo3, 0666);

    int finish = 30;

    int t;
    int tcount = 0;
    int p1new, p2new;

    printf("Enter time at which positions change\n");
    scanf("%d",&t);

    p1new = rand()%20 + 1;
    p2new = rand()%20 + 1;

    int p1temp;

    printf("p1new= %d p2new= %d\n", p1new, p2new);

    while(1){

        tcount += 1;

        //wait for p1 from hare
        fd1 = open(myfifo, O_RDONLY);
        read(fd1, &p1, sizeof(p1));
        close(fd1);

        if(tcount == t){
            p2 = p2new;
            p1 = p1new;
            p1temp = p1new;
        }else{
            p2 = -1;
            p1temp = -1;
        }

        //signal turtle
        fd2 = open(myfifo2, O_WRONLY);
        write(fd2, &p2, sizeof(p2));
        close(fd2);

        //wait for turtle
        fd2 = open(myfifo2, O_RDONLY);
        read(fd2, &p2, sizeof(p2));
        close(fd2);

        //send msg to repoter
        msg[0] = p1;
        msg[1] = p2;
        rd = open(myfifo3, O_WRONLY);
        write(rd, &msg, sizeof(msg));
        close(rd);

        //signal hare
        fd1 = open(myfifo, O_WRONLY);
        write(fd1, &p1temp, sizeof(p1temp));
        close(fd1);

    }

}   