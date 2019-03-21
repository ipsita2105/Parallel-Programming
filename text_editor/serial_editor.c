/*****************************includes**************************/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

FILE* output_file;

/*****************************defines**************************/

#define EDITOR_VERSION "0.0.1"
#define EDITOR_TAB_STOP 8
#define KILO_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f) //for mapping ctrl+_ combos

enum editorKey{

	BACKSPACE = 127,
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

enum editorHighlight{
	
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

/******************************data***************************/

struct editorSyntax{
	
	char*  filetype;
	char** filematch; //array of strings to match the pattern
	char** keywords;
	char* singleline_comment_start;
	char* multiline_comment_start;
	char* multiline_comment_end;
	int flags; //whether to highlight or not
};


//for storing row of text editor

typedef struct erow{

	int idx; //each row knows its index in file
	int size;
	int rsize;      //size of render
	char *chars;
	char *render;   //contains line to draw on screen
	unsigned char* hl;
	int hl_open_comment;
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
  int dirty;     //to record the fact that file has been modified
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax* syntax;
  struct termios orig_termios;

};

struct editorConfig E;

/*****************************filetypes**************************/

char *C_HL_extensions[] = {".c", ".h",".cpp", NULL};

char *C_HL_keywords[] = {
  
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct editorSyntax HLDB[] = {

	//filetype, filematch, flag	
	{
	  "c",			
	  C_HL_extensions,      
	  C_HL_keywords,
	  "//","/*","*/",
	  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
};

//stores length of HLDB array
#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))


/*****************************prototypes**************************/

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

void editorRefreshScreen();

char *editorPrompt(char* prompt, void (*callback)(char*, int));

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

/**************************syntax highlighting*************************/

int is_separator(int c){

	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}


void editorUpdateSyntax(erow *row){

	row->hl = realloc(row->hl, row->rsize);
	//set all chars as normal by default
	memset(row->hl, HL_NORMAL, row->rsize);

	if (E.syntax == NULL) return;

	char** keywords = E.syntax->keywords;

	char* scs = E.syntax->singleline_comment_start;
	char* mcs = E.syntax->multiline_comment_start;
	char* mce = E.syntax->multiline_comment_end;

	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;
	int mce_len = mce ? strlen(mce) : 0;

	//start of line is separator
	int prev_sep   = 1; //to keep track if last step was a separator
	int in_string  = 0; //already inside a string to keep highlighting
	//if inside a multi line comment
	//true if unclosed comment
	int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment); 

	int i = 0;
	while(i < row->rsize){
		char c = row->render[i];
		//prev_hl is hl prev char
		unsigned char prev_hl = (i>0) ? row->hl[i-1]:HL_NORMAL;

		//no single line comment in multiline
		if (scs_len && !in_string && !in_comment){
	
			//check if that char is start of single line comment	
			if(!strncmp(&row->render[i], scs, scs_len)){
				//if so set the rest as comment
				memset(&row->hl[i], HL_COMMENT, row->rsize - i);
				break;
			}
		}


		//comment cant be in string
		if (mcs_len && mce_len && !in_string){
		
			//if inside comment
			if(in_comment){
			
				row->hl[i] = HL_MLCOMMENT;

				//if at end
				if(!strncmp(&row->render[i], mce, mce_len)){
					
					memset(&row->hl[i], HL_MLCOMMENT, mce_len);
					i += mce_len;
					in_comment = 0;
					continue;
				
				}else{
					//if not at end
					//simply consume the character
					i++;
					continue;
				}
			
			 //if not in comment check if start of comment
			}else if (!strncmp(&row->render[i], mcs, mcs_len)){
				
				memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
				i += mcs_len;
				in_comment = 1;
				continue;
			}
		
		}


		if (E.syntax->flags & HL_HIGHLIGHT_STRINGS){
			
			//if in_string set
			if(in_string){
				
				//keep highlighting
				row->hl[i] = HL_STRING;

				//id current char is \ and there is atleast
				//one more char after it
				if(c == '\\' && i+1 < row->rsize){
					
					row->hl[i+1] = HL_STRING;
					i += 2;
					continue;
				}

				//if end of string then stop
				if (c == in_string) in_string = 0;

				i++;
				prev_sep = 1; //since closing code is separator
				continue;
			}else{
			
				if(c == '"' || c == '\''){

				    in_string = c; //in_string stores " or '
				    row->hl[i] = HL_STRING;
				    i++;
				    continue;
				}
			}
		
		}


		if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS){
			//highlight when prev_char is sep or last is also number
			//also for floats
			if((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER))
				      || (c == '.' && prev_hl == HL_NUMBER)){

				row->hl[i] = HL_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}

		}

		//keywords should have separtor
		//both before and after 
		//if separator before
		if(prev_sep){
		
			int j;
			//loop over all possible keywords
			for(j=0; keywords[j]; j++){
			
				int klen = strlen(keywords[j]);
				int kw2  = keywords[j][klen - 1] == '|';

				if(kw2) klen--;

				//if keyword exists and end is a separator
				if(!strncmp(&row->render[i], keywords[j], klen) &&
				   is_separator(row->render[i + klen])){
					
					memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
					i += klen;
					break;
				}
			}

			//if at end of keywords
			if (keywords[j] != NULL){
			
				prev_sep = 0;
				continue;
			}
		}

		prev_sep = is_separator(c);
		i++;
	}

	int changed = (row->hl_open_comment != in_comment);
	//open comment state is in comment
	//state after processing row
	row->hl_open_comment = in_comment;

	//if comment is on
	//then rest of the lines will also be comment
	if(changed && row->idx + 1 < E.numrows)
		editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl){

	switch(hl){

		case HL_COMMENT  :
		case HL_MLCOMMENT:  return 36;
		case HL_KEYWORD1 :  return 33;
		case HL_KEYWORD2 :  return 32;
		case HL_STRING   :  return 35;		
		case HL_NUMBER   :  return 31;
		case HL_MATCH    :  return 34;

		default: return 37;
	}
}


void editorSelectSyntaxHighlight(){

	E.syntax = NULL;
	if (E.filename == NULL) return;

	//pointer to last occurrence
	char* ext = strrchr(E.filename,'.');

	//loop in HLDB array
	for(unsigned int j=0; j<HLDB_ENTRIES; j++){
		
		struct editorSyntax* s = &HLDB[j];
		unsigned int i = 0;

		//go through file match array
		while(s->filematch[i]){
		
			//if pattern starts with a dot
			int is_ext = (s->filematch[i][0] == '.');

			//check if filename ends with this extension
			//or if its not an ext check if anywhere in filename
			if((is_ext  && ext && !strcmp(ext, s->filematch[i])) ||
			   (!is_ext && strstr(E.filename,  s->filematch[i]))){
					
				E.syntax = s;
				clock_t begin = clock();

				int filerow;
				for(filerow = 0; filerow < E.numrows; filerow++){
					editorUpdateSyntax(&E.row[filerow]);
				}

				clock_t end = clock();
			
				char smm[35];
				snprintf(smm, sizeof(smm),"syntax highlight time = %f\n",(double)(end-begin)/CLOCKS_PER_SEC);
				fwrite(smm, sizeof(char), sizeof(smm), output_file);
		
				return;
			}
			i++;
		}
	}
}


/*****************************row operations**************************/
//Cx is into chars
//Rx is into render
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

int editorRowRxToCx(erow *row, int rx){


	int cur_rx = 0;
	int cx;

	for(cx = 0; cx < row->size; cx++){
		
		if(row->chars[cx] == '\t')
			cur_rx += (EDITOR_TAB_STOP - 1) - (cur_rx % EDITOR_TAB_STOP);

		cur_rx++;
	
		//when we hacve reached rx
		if (cur_rx > rx) return cx;
	}

	return cx;

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

	 //after making render we can update syntax
	editorUpdateSyntax(row);
}


void editorInsertRow(int at, char* s, size_t len){

	if (at < 0 || at > E.numrows) return;

	//add mem for one more row
	E.row = realloc(E.row, sizeof(erow)*(E.numrows + 1));

	//make space shift
	memmove(&E.row[at + 1], &E.row[at], sizeof(erow)*(E.numrows - at));

	//set idx as inserted
	for(int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

	E.row[at].idx = at;
	
	//at is index of new row
	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len); //copy the text to allocated memory
	E.row[at].chars[len] = '\0';

	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	E.row[at].hl = NULL;
	E.row[at].hl_open_comment = 0;
	editorUpdateRow(&E.row[at]);

	E.numrows ++;
	E.dirty++;

}

void editorFreeRow(erow *row){

	free(row->render);
	free(row->chars);
	free(row->hl);
}

void editorDelRow(int at){

	if (at < 0 || at >= E.numrows) return;

	editorFreeRow(&E.row[at]);

	//shift file one line up
	//TODO: scope of improvement
	memmove(&E.row[at], &E.row[at+1], sizeof(erow)*(E.numrows - at - 1));

	for(int j = at; j < E.numrows - 1; j++) E.row[j].idx--;

	E.numrows--;
	E.dirty++;

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
	E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len){

	//increase size of this row
	row->chars = realloc(row->chars, row->size + len + 1);

	//memcopy given string at end
	memcpy(&row->chars[row->size], s, len);

	row->size += len;
	row->chars[row->size] = '\0';
	
	editorUpdateRow(row);
	E.dirty++;
}


void editorRowDelChar(erow *row, int at){

	if (at < 0 || at >= row->size) return;

	memmove(&row->chars[at], &row->chars[at+1], row->size - at);

	row->size--;
	editorUpdateRow(row);
	E.dirty++;

}




/*****************************editor operations**************************/

void editorInsertChar(int c){

	//if cursor at the last on the ~	
	if(E.cy == E.numrows){
		editorInsertRow(E.numrows, "", 0);
	}

	editorRowInsertChar(&E.row[E.cy], E.cx, c);

	//move cursor ahead after the inserted char
	E.cx++;
}

void editorInsertNewline(){

	//if at start
	if(E.cx == 0){
		
		//just insert a new line
		editorInsertRow(E.cy, "", 0);

	} else{
	
		erow *row = &E.row[E.cy];

		//line right of cursor
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);

		//reassign to start of line
		row = &E.row[E.cy];

		//truncate this line
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row);
	}

	E.cy++;
	E.cx = 0;

}



//delet char left of cursor
void editorDelChar(){

	//if cursor past eof
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;

	//get the row
	erow *row = &E.row[E.cy];

	if(E.cx > 0){
	
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
	}else{

		//goto end of prev line	
		E.cx = E.row[E.cy - 1].size;

		//append row to prev row
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
	}
}



/*****************************file i/o**************************/

//convert all rows to array of strings to write to file
char* editorRowsToString(int *buflen){

	int totlen = 0;
	int j;

	//+1 for newline char
	for(j=0; j< E.numrows; j++)
		totlen += E.row[j].size + 1;

	//tell caller how long is the string
	*buflen = totlen;

	char *buf = malloc(totlen);
	char *p = buf;

	for(j=0; j<E.numrows; j++){
		
		//copy the row to p
		memcpy(p, E.row[j].chars, E.row[j].size);
		
		//goto end
		p += E.row[j].size;

		//append new line
		*p = '\n';

		//go 1 step
		p++;
	}

	return buf;

}


void editorOpen(char* filename){

	free(E.filename);
	E.filename = strdup(filename); //makes a copy of given string

	editorSelectSyntaxHighlight();

	FILE *fp   = fopen(filename, "r");
	if (!fp) die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	//line points to memory allocated
	//linecap is amt of memory allocated	
	
	clock_t begin = clock();
	while((linelen = getline(&line, &linecap, fp)) != -1){

		//strip of \n or \r
	     	while(linelen > 0 && (line[linelen -1] == '\n' || line[linelen-1] == '\r'))
	     		linelen --;
	     
	     	editorInsertRow(E.numrows, line, linelen);
	}

	clock_t end = clock();
	
	char smm[30];
	snprintf(smm, sizeof(smm),"time to open file = %f\n",(double)(end-begin)/CLOCKS_PER_SEC);
	fwrite(smm, sizeof(char), sizeof(smm), output_file);

	free(line);
	fclose(fp);
	//fclose(fout);

	//to account for call to editorAppendRow above
	E.dirty = 0;

}

void editorSave(){


	if (E.filename == NULL){
		
		E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);

		if (E.filename == NULL){
			
			editorSetStatusMessage("Save aborted");
			return;
		}

		editorSelectSyntaxHighlight();
	}

	clock_t begin = clock();

	int len;
	char *buf = editorRowsToString(&len);

	clock_t end = clock();
	
	char smm[30];
	snprintf(smm, sizeof(smm),"time to save file = %f\n",(double)(end-begin)/CLOCKS_PER_SEC);
	fwrite(smm, sizeof(char), sizeof(smm), output_file);
	
	//o_creat create new file if not exist
	//o_rdwr open for r&w
	//0644 mode permission
	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);


	//error handling
	if (fd != -1){
	
		//ftruncate sets file size
		if(ftruncate(fd, len) != -1){
		
			if(write(fd, buf, len) == len){
				
				close(fd);
				free(buf);
				
				E.dirty = 0;

				//save status msg
				editorSetStatusMessage("%d bytes written to disk", len);
				return;
			}
		}

		close(fd);
	
	}
	
	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));

}

/*****************************find**************************/

void editorFindCallback(char *query, int key){

	//last match has index of row of last match
	static int last_match = -1;
	static int direction  =  1; // 1 forward -1 backward


	static int saved_hl_line;   //is there something to restore
	static char* saved_hl = NULL;

	if(saved_hl){
	
		memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;

	}

	//we are about to leave search
	//reset values
	if (key == '\r' || key == '\x1b'){
		last_match = -1;
		direction = 1;
		return;
	}

	else if (key == ARROW_RIGHT || key == ARROW_DOWN){
		direction = 1;
	}

	else if (key == ARROW_LEFT || key == ARROW_UP){
		direction = -1;
	}
	else{
		last_match = -1;
		direction  =  1;
	}

	if (last_match == -1) direction = 1;
	//start on the last match
	int current = last_match;

	int i;
	for(i=0; i< E.numrows; i++){
	
		current += direction;

		//wrap around the file
		if (current == -1) current = E.numrows - 1;
		else if (current == E.numrows) current = 0;

		erow *row = &E.row[current];
		char *match = strstr(row->render, query);

		if(match){
			
			last_match = current;
			E.cy = current;
			//subtract pointer to get position
			E.cx = editorRowRxToCx(row, match - row->render);
			//scroll to end
			//then editorScroll goes up on
			//next screen refresh
			//so matching line at start
			E.rowoff = E.numrows;


			saved_hl_line = current;
			saved_hl = malloc(row->rsize);
			memcpy(saved_hl, row->hl, row->rsize);
			//match - row->render match into render
			memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
			break;
		}
	
	}

}


void editorFind(){

	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int saved_coloff = E.coloff;
	int saved_rowoff = E.rowoff;

	char* query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)", editorFindCallback);

	//on esc null returned
	if (query){
		free(query);
	}else{
	
		E.cx = saved_cx;
		E.cy = saved_cy;
		E.coloff = saved_coloff;
		E.rowoff = saved_rowoff;
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
		
	           //get the line in c
		   char* c = &E.row[filerow].render[E.coloff];
		   unsigned char* hl = &E.row[filerow].hl[E.coloff];

		   int current_color = -1;//default color

		   int j;
		   //loop to find digits
		   for(j=0; j<len; j++){
		  
			//for ctrl + chars 
			if(iscntrl(c[j])){
				
				char sym = (c[j] <= 26) ? '@' + c[j] : '?';
				abAppend(ab, "\x1b[7m", 4); //switch to inverted color
				abAppend(ab, &sym, 1);      //print char
				abAppend(ab, "\x1b[m", 3); //back to current color

				//since last line turns off color formatting
				//we go back to current color
				if (current_color != -1){
				   char buf[16];
				   int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
				   abAppend(ab, buf, clen);

				}
			}
   
			else if (hl[j] == HL_NORMAL){

				if (current_color != -1){
				
					abAppend(ab, "\x1b[39m", 5);
					current_color = -1;
					
				}
				abAppend(ab, &c[j], 1);

			}else{

				int color = editorSyntaxToColor(hl[j]);

				if(color != current_color){
					
					current_color = color;
					
					char buf[16];
					//write the colour to buffer
					int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
					abAppend(ab, buf, clen);
					
				}

				abAppend(ab, &c[j], 1);
			}
		   }

		   abAppend(ab, "\x1b[39m", 5);

 	   }


		abAppend(ab, "\x1b[K", 3); //clear each line
		abAppend(ab, "\r\n", 2);
	
	}

}


void editorDrawStatusBar(struct abuf *ab){

	abAppend(ab, "\x1b[7m", 4); //switch to inverted colours

	char status[80], rstatus[80];

	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
			E.filename ? E.filename : "[No Name]", E.numrows,
			E.dirty ? "(modified)": "");

	int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
			    E.syntax ? E.syntax->filetype : "no ft", E.cy + 1, E.numrows);

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

	clock_t begin = clock();
	editorDrawRows(&ab); //draw ~
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);
	clock_t end = clock();
				
	char smm[35];
	snprintf(smm, sizeof(smm),"refresh time = %f\n",(double)(end-begin)/CLOCKS_PER_SEC);
	fwrite(smm, sizeof(char), sizeof(smm), output_file);


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


/*****************************input**************************/
//prompt for save as
char *editorPrompt(char *prompt, void (*callback)(char *, int)){

	size_t bufsize = 128;
	char *buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while(1){
	
	
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();

		int c = editorReadKey();

		//user can press backspace in prompt
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE){
		
			if (buflen != 0) buf[--buflen] = '\0';
		}
		//cancel if user presses esc
		else if(c == '\x1b'){
			
			editorSetStatusMessage("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		}

		//user presses enter
		else if(c == '\r'){
		
			//input is not empty
			if(buflen != 0){
				
				//clear status msg
				editorSetStatusMessage("");
				if (callback) callback(buf, c);
				//return user input
				return buf;

			}

		}else if (!iscntrl(c) && c < 128){
		
			//come here when they enter printable char
		
			//if buflen max capacity
			if (buflen == bufsize - 1){
				
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}

			buf[buflen++] = c;
			buf[buflen] = '\0';
		
		}

	if (callback) callback(buf, c);
	
	}
}



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
	static int quit_times = KILO_QUIT_TIMES;

	int c = editorReadKey();

	switch(c){

		//the enter key
		case '\r':
			editorInsertNewline();
			break;
	
		case CTRL_KEY('q'):
			
			  if (E.dirty && quit_times > 0){
				
			      editorSetStatusMessage("WARNING!! File has unsaved changes."
				    "Press Ctrl-Q %d more times to quit.", quit_times);					
				  quit_times--;
				  return;
			  }

			  write(STDOUT_FILENO, "\x1b[2J", 4);
			  write(STDOUT_FILENO, "\x1b[H" ,3);
			  exit(0);
			  break;

		case CTRL_KEY('s'):
			  editorSave();
			  break;

		case HOME_KEY:
			  E.cx = 0;
			  break;

		case END_KEY:

			  if  (E.cy < E.numrows)
			  	E.cx = E.row[E.cy].size;
			  break;

		case CTRL_KEY('f'):
			  editorFind();
			  break;

		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
			  if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
			  editorDelChar();
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

		case CTRL_KEY('l'):

		case '\x1b':
			break;
		default:
			editorInsertChar(c);
			break;
	}

	quit_times = KILO_QUIT_TIMES;
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
	E.dirty = 0;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	E.syntax = NULL;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	
	//for 2 status bars
	E.screenrows -= 2;
}

int main(int argc, char* argv[]){

	output_file = fopen("output_serial","w");
	enableRawMode();
	initEditor();

	if(argc >= 2){
		//argv[1] is filename
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP:Ctrl-s = save |  Ctrl-Q = quit | Ctrl-F = find");

	//reads 1 byte from stdin
	while(1){

		editorRefreshScreen();
		editorProcessKeypress();
	}
	fclose(output_file);
	return 0;
}



