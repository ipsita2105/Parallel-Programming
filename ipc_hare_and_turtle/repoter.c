#include<stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h> 

int main(){

        char *myfifo = "/tmp/repo";
        mkfifo(myfifo, 0666);

        int rd, temp;
        int finish= 30;
        int t=0;
        int m[2];

        while(1){

            t += 1;

            rd= open(myfifo, O_RDONLY);
            read(rd, &m, sizeof(m));
            close(rd);

            printf("t= %d p1= %d p2= %d\n", t, m[0], m[1]);

            if(m[0] >= finish){
                printf("Hare finishes\n");
                break;
            }
            else if(m[1] >= finish){
                printf("Turtle finishes\n");
                break;
            }

        } 
}