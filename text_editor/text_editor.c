/*****************************includes**************************/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<ctype.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<string.h>
#include<sys/types.h>
#include<time.h>
#include<stdarg.h>

/*****************************defines**************************/

#define EDITOR_VERSION "0.0.1"
#define EDITOR_TAB_STOP 8

#define CTRL_KEY(k) ((k) & 0x1f) //for mapping ctrl+_ combos

enum editorKey{

	ARROW_LEFT  = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

/******************************data***************************/

//for storing row of text editor

typedef struct erow{

	int size;
	int rsize;      //size of render
	char *chars;
	char *render;   //contains line to draw on screen
}erow;

struct editorConfig{

  int cx, cy; // cx is offset into chars
  int rx;     //offset into render
  int rowoff; //current row
  int coloff; //current column
  int screenrows;
  int screencols;
  int numrows;
  erow* row;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
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
int editorReadKey(){

	int nread;
	char c;

	while((nread = read(STDIN_FILENO, &c, 1)) != 1){
	
		if(nread == -1 && errno != EAGAIN) die("read");	
	}

	// mapping arrow keys 
	// therefore read to identify escape seqs	
	if (c == '\x1b'){ //if u see an escape seq
	
		char seq[3];

		//read two more bytes
		if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if(seq[0] == '['){
		
			if(seq[1] >= '0' && seq[1] <= '9'){
				
				if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if(seq[2] == '~'){
				
					switch (seq[1]){
						
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			
			}else{

			  switch(seq[1]){
				
				case 'A': return ARROW_UP;
				case 'B': return ARROW_DOWN;
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			  }


			}
		} else if (seq[0] == 'O'){
		
			switch(seq[1]){
			
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}

		return '\x1b';

	} else {
	
		return c;
	}

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

/*****************************row operations**************************/

//convert Cx index to Rx
int editorRowCxToRx(erow *row, int cx){

	int rx = 0;
	int j;

	for(j=0; j<cx; j++){
	
		//rx % EDITOR_TAB_STOP cols right of last tab stop
		//EDITOR_TAB_STOP - 1 - (above) will give spaces
		//we have to move to reach the next tab stop
		if(row->chars[j] == '\t')
			rx += (EDITOR_TAB_STOP - 1) - (rx % EDITOR_TAB_STOP);
		rx++;
	}

	return rx;
}




//get render from row
//basically handle tabs
void editorUpdateRow(erow *row) {

	int tabs = 0;
	int j;

	for (j=0; j< row->size; j++)
		if (row->chars[j] == '\t') tabs++;

 	 free(row->render);
	 //tab of size 8
	 //1 in \t char
	 //so add 7
 	 row->render = malloc(row->size + tabs*(EDITOR_TAB_STOP - 1) + 1);

 	 
 	 int idx = 0;
 	 for (j = 0; j < row->size; j++) {

		 if (row->chars[j] == '\t'){

			//append a space
		 	row->render[idx++] = ' ';

			//tab end till 8 cols
			while(idx % EDITOR_TAB_STOP != 0) row->render[idx++] = ' ';
 		 
		 }else{
		 
		 	row->render[idx++] = row->chars[j];
		 }
 	  	 
 	 }

 	 row->render[idx] = '\0';
 	 row->rsize = idx;
}


void editorAppendRow(char* s, size_t len){

	//realloc to increase row by one
	E.row = realloc(E.row, sizeof(erow)*(E.numrows+1));
	
	//at is index of new row
	int at = E.numrows;
	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len); //copy the text to allocated memory
	E.row[at].chars[len] = '\0';

	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	editorUpdateRow(&E.row[at]);

	E.numrows ++;

}

// insert a char into a row

void editorRowInsertChar(erow *row, int at, int c){

	//validate index where we insert char
	if(at < 0 || at > row->size) at = row->size;

	//+2 cause one for null byte
	row->chars = realloc(row->chars, row->size+2);
	//make room for new char
	memmove(&row->chars[at+1], &row->chars[at], row->size - at + 1);
	
	row->size ++;
	
	//insert the char
	row->chars[at] = c;

	editorUpdateRow(row);
}

/*****************************editor operations**************************/

void editorInsertChar(int c){

	//if cursor at the last on the ~	
	if(E.cy == E.numrows){
		editorAppendRow("", 0);
	}

	editorRowInsertChar(&E.row[E.cy], E.cx, c);

	//move cursor ahead after the inserted char
	E.cx++;
}


/*****************************file i/o**************************/

void editorOpen(char* filename){

	free(E.filename);
	E.filename = strdup(filename); //makes a copy of given string

	FILE *fp   = fopen(filename, "r");
	if (!fp) die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	//line points to memory allocated
	//linecap is amt of memory allocated	
	while((linelen = getline(&line, &linecap, fp)) != -1){

		//strip of \n or \r
	     	while(linelen > 0 && (line[linelen -1] == '\n' || line[linelen-1] == '\r'))
	     		linelen --;
	     
	     	editorAppendRow(line, linelen);
	}

	free(line);
	fclose(fp);

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

void editorScroll(){

	E.rx = 0;

	if (E.cy < E.numrows){
		
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
	}

	//E.rowoff refers to top of screen
	//E.screenrows refer to bottom of screen

	// if cursor above visible window
	if (E.cy < E.rowoff){

		//go to cursor
		E.rowoff = E.cy;
	}

	//if cursor past bottom
	if (E.cy >= E.rowoff + E.screenrows){
		
		E.rowoff = E.cy - E.screenrows + 1;
	}

	if (E.rx < E.coloff){
		E.coloff = E.rx;
	}

	if (E.rx >= E.coloff + E.screencols){
		E.coloff = E.rx - E.screencols + 1;
	}

}




void editorDrawRows(struct abuf *ab){
	
	int y;
	for(y=0; y<E.screenrows; y++){

	   //get row of file to display at position y
	   int filerow = y + E.rowoff;
	   
	   if (filerow >= E.numrows){
	
	   //draw after text lines
		   
			//display only when no file input
			if(E.numrows == 0 && y == E.screenrows/3){
			
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),
					"Text Editor --version %s", EDITOR_VERSION);
	
				if (welcomelen > E.screencols) welcomelen = E.screencols;
	
				int padding = (E.screencols - welcomelen)/2;
			
				if (padding){
					abAppend(ab, "~", 1);
					padding --;
				}
	
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
	
			}else{
	
			   abAppend(ab, "~", 1);
			}
	
	   }else{
			
		   int len = E.row[filerow].rsize - E.coloff;
		   if (len < 0) len = 0;
		   if (len > E.screencols) len = E.screencols;
		   abAppend(ab, &E.row[filerow].render[E.coloff], len);
 	   }


		abAppend(ab, "\x1b[K", 3); //clear each line

		abAppend(ab, "\r\n", 2);
	
	}

}


void editorDrawStatusBar(struct abuf *ab){

	abAppend(ab, "\x1b[7m", 4); //switch to inverted colours

	char status[80], rstatus[80];

	int len = snprintf(status, sizeof(status), "%.20s - %d lines",
			E.filename ? E.filename : "[No Name]", E.numrows);

	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
				E.cy + 1, E.numrows);

	if (len > E.screencols) len = E.screencols;

	abAppend(ab, status, len);

	//rest of the white back
	while (len < E.screencols){
	
		//so that its on right side
		if (E.screencols - len == rlen){
			
			abAppend(ab, rstatus, rlen);
			break;

		}else{
			abAppend(ab, " ", 1);
			len++;
		}

	}

	abAppend(ab, "\x1b[m", 3); //switch back to normal
	abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab){

	abAppend(ab, "\x1b[K", 3); //first clear msg bar
	int msglen = strlen(E.statusmsg);

	//check msglen within screen
	if (msglen > E.screencols) msglen = E.screencols;

	//if within 5 secs then display
	if (msglen && time(NULL) - E.statusmsg_time < 5)
		abAppend(ab, E.statusmsg, msglen);
}



void editorRefreshScreen(){

	editorScroll();

	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6); //hide cursor before refresh
	abAppend(&ab, "\x1b[H", 3);  //reposition cursor	

	editorDrawRows(&ab); //draw ~
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);

	char buf[32];
	//+1 cause termial 1 indexed
	//move cursor to position 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));

	//abAppend(&ab, "\x1b[H", 3); //after drawing reposition cursor
	abAppend(&ab, "\x1b[?25h", 6); //unhide cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

//any number of arguments
void editorSetStatusMessage(const char* fmt, ...){

	va_list ap;
	va_start(ap, fmt); //pass argument before ... so addr of next arg known

	//takes care of other arguments
	//takes string from argumnet
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);

	va_end(ap);
	E.statusmsg_time = time(NULL);

}



/*****************************input**************************/

void editorMoveCursor(int key){

	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
		
	switch(key){
	
	case ARROW_LEFT:
		if (E.cx != 0){

			E.cx--;

		} else if (E.cy > 0){
			
			//<- on start of line 
			//to goto prev line
			//if not on first line
			E.cy --;
			E.cx = E.row[E.cy].size;
		}

		break;

	case ARROW_RIGHT:
		//dont go past current line
		if (row && E.cx < row->size){
		
			E.cx++;

		}else if (row && E.cx == row->size){
			E.cy++;
			E.cx = 0;
		}
		break;

	case ARROW_UP:
		if(E.cy != 0){
		
			E.cy--;
		}
		break;
	case ARROW_DOWN:
		//can go below screen not below file
		if (E.cy < E.numrows){
			
			E.cy++;
		}
		break;
	}


	//set row again
	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen){
	
		E.cx = rowlen;
	}

}


//waits for key press and handles it
//like by mapping
void editorProcessKeypress(){

	int c = editorReadKey();

	switch(c){
	
		case CTRL_KEY('q'):
			  write(STDOUT_FILENO, "\x1b[2J", 4);
			  write(STDOUT_FILENO, "\x1b[H" ,3);
			  exit(0);
			  break;

		case HOME_KEY:
			  E.cx = 0;
			  break;

		case END_KEY:

			  if  (E.cy < E.numrows)
			  	E.cx = E.row[E.cy].size;
			  break;

		case PAGE_UP:
		case PAGE_DOWN:
			{

				if (c == PAGE_UP){
					E.cy = E.rowoff;

				} else if (c == PAGE_DOWN){
				
					E.cy = E.rowoff + E.screenrows - 1;

					if (E.cy > E.numrows) E.cy = E.numrows;
				}

				int times = E.screenrows;
				while(times--)
					editorMoveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
			
			}
			break;

		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			editorMoveCursor(c);
			break;
		default:
			editorInsertChar(c);
			break;
	}
}


/*****************************init**************************/

void initEditor(){

	//cursor x and y position
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.coloff = 0;
	E.numrows = 0;
	E.row = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	
	//for 2 status bars
	E.screenrows -= 2;
}

int main(int argc, char* argv[]){

	enableRawMode();
	initEditor();

	if(argc >= 2){
		//argv[1] is filename
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-Q = quit");

	//reads 1 byte from stdin
	while(1){

		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}



