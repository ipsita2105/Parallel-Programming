#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<ctype.h>

struct termios orig_termios;


void disableRawMode(){

	//TCSAFLUSH discards any unread input
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


//we need this function
//beacuse terminal by deafult
//is in cooked mode
//that is input send only after enter pressed
//we want raw mode
void enableRawMode(){

	tcgetattr(STDIN_FILENO, &orig_termios);

	//atexit from stdlib
	//to call function at end of main
	atexit(disableRawMode);

	struct termios raw = orig_termios;

	// IXON: diable ctrl+s and ctrl+q
	// ICRNL: carriage return new line
	raw.c_iflag &= ~(ICRNL | IXON);

	//turn off output processing
	raw.c_oflag &= ~(OPOST);

	//with echo whatever we type is showed on terminal
	//there its turned off
	//this is a bit operation
	//c_lfag is local flag
	//we take not of echo and and of flag
	//
	//ICANON flag to turn of canonical mode
	//now input is read byte by byte
	//insted of line by line
	//so program exits when we press q
	//ISIG to stop ctrl+c ctrl+z
	//IEXTEN for ctrl+v
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	//then write it back on terminal
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);


}


int main(){

	enableRawMode();

	char c;
	//reads 1 byte from stdin
	
	while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
		
		//iscntl = check if control charachter
		if(iscntrl(c)){
		   printf("%d\n",c);
		} else{
		 
		   // \r returns to left	
		   printf("%d ('%c')\r\n",c, c);
		}	
	}

	return 0;
}



