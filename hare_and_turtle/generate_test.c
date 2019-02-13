#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define FINISH 100

int main(){

	srand(time(NULL));
	int change_time = rand()%FINISH + 1;

	printf("%d",change_time);

}
