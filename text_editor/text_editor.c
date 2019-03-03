/*****************************includes**************************/
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<ctype.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<string.h>
/*****************************defines**************************/

#define CTRL_KEY(k) ((k) & 0x1f) //for mapping ctrl+_ combos

/******************************data***************************/

struct editorConfig{

  int screenrows;
  int screencols;
  struct termios orig_termios;

};

struct editorConfig E;

/*****************************terminal**************************/

void die(const char *s){

	//clear screen at exit
	write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s); // for descriptive error msg
	exit(1);

}

void disableRawMode(){

	//TCSAFLUSH discards any unread input
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
		die("tcsetattr");
}


//we need this function
//beacuse terminal by deafult
//is in cooked mode
//that is input send only after enter pressed
//we want raw mode
void enableRawMode(){

	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

	//atexit from stdlib
	//to call function at end of main
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;

	// IXON: diable ctrl+s and ctrl+q
	// ICRNL: carriage return new line
	raw.c_iflag &= ~(BRKINT| ICRNL | INPCK | ISTRIP | IXON);

	//turn off output processing
	raw.c_oflag &= ~(OPOST);

	raw.c_cflag |= (CS8);

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

	raw.c_cc[VMIN] = 0;     //min bytes on input before read returns
	raw.c_cc[VTIME] = 1;    //max time before timeout 

	//then write it back on terminal
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");

}

//wait for one keypress and return it
char editorReadKey(){

	int nread;
	char c;

	while((nread = read(STDIN_FILENO, &c, 1)) != 1){
	
		if(nread == -1 && errno != EAGAIN) die("read");
	
	}

	return c;
}

int getCursorPosition(int *rows, int *cols){

	char buf[32];
	unsigned int i = 0;

	if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	//parse the cursor position
	while(i < sizeof(buf)-1){
	
		if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if(buf[i] == 'R') break;
		i++;
	}

	buf[i] = '\0';

	//printf("\r\n&buf[1]: '%s'\r\n",&buf[1]); prints cursor position
	
	if(buf[0] != '\x1b' || buf[1] != '[') return -1;

	//parse the two numbers from cursor position
	if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return -1;
}



int getWindowSize(int *rows, int *cols){
	
	struct winsize ws;

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		//move cursor to bottom right
		//first right then bottom
		//999 ensures a very large value
		if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	}else{
	
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*****************************append buffer**************************/

struct abuf{
 
	char *b;  //pointer to buffer
	int len;  //len of buffer
};

#define ABUF_INIT {NULL, 0} //consttuctor empty buffer

void abAppend(struct abuf *ab, const char *s, int len){
	
	char *new = realloc(ab->b, ab->len + len);

	if(new == NULL) return;

	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;

}

void abFree(struct abuf *ab){
	free(ab->b);
}


/*****************************output**************************/

void editorDrawRows(struct abuf *ab){
	
	int y;
	for(y=0; y<E.screenrows; y++){
	
		abAppend(ab, "~", 1);

		if(y < E.screenrows - 1){
		   abAppend(ab, "\r\n", 2);
		}
	
	}

}


void editorRefreshScreen(){

	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[2J", 4); //clear screen
	abAppend(&ab, "\x1b[H", 3);  //reposition cursor	

	editorDrawRows(&ab); //draw ~

	abAppend(&ab, "\x1b[H", 3); //after drawing reposition cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}





/*****************************input**************************/
//waits for key press and handles it
//like by mapping
void editorProcessKeypress(){

	char c = editorReadKey();

	switch(c){
	
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H" ,3);
			exit(0);
			break;
	}
}


/*****************************init**************************/

void initEditor(){

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){

	enableRawMode();
	initEditor();

	//reads 1 byte from stdin
	while(1){

		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}



