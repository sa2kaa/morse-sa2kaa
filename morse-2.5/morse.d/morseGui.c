#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include "morseGui.h"

static WINDOW *create_newwin(int height, int width, int starty, int startx);
static void destroy_win(WINDOW *local_win);
static void handle_winch(int sig);
static void makeWindows(const int lines, const int cols);
static void updateWinFromBuffer(WINDOW  * const w, const int winHeight, const int currentPtr, char const * const * const buf, const int bufferSize);
static void takeDownWindows(void);
static void initLineBuffer(void);
static void addStringToBuffer( char const * const str, int * currentPtr, char * const * const buf, const int bufferSize);

const int statWinHeight = 6;
WINDOW *winQuery, *winRespons, *winStat;
static WINDOW *winResponsBox, *winQueryBox, *winStatBox;

#define lineBufferSize  80
const int lineSize = 32;
const char *queryBuf[lineBufferSize];
const char const * responsBuf[lineBufferSize];
int currQueryPtr = 0, currResponsPtr = 0;

void initMorseGui(void)
//int main(int argc, char *argv[])
{
	initLineBuffer(); /* Really this function needs to be called! */
	initscr();			/* Start curses mode. */
	/*
    struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = handle_winch;
	sigaction(SIGWINCH, &sa, NULL);
	*/
	cbreak();			/* Line buffering disabled, pass on every thing to me. */
	// nodelay(stdscr, TRUE); // Make getch() non blocking.
	noecho();
	curs_set(0); /* Hide the cursor */
	keypad(stdscr, TRUE);		/* I need that nifty F1 	*/

	//	printw("Press F1 to exit");
	refresh(); // Need to refresh the base window before adding new ones!
	makeWindows(LINES, COLS);

    wprintw( winQuery, "Query!\n");
	wprintw( winRespons, "Response!\n");
	wrefresh(winQuery);
	wrefresh(winRespons);
}

static void morseGuiLoop( void)
{
	int ch;
	int i = 0;
    char s[lineSize];
	while((ch = getch()) != KEY_F(1))
	{
		int p, row, col;
		switch(ch)
		{
		case KEY_LEFT:
			sprintf(s, "\nQuery %d?", i);
			addStringToBuffer( s, &currQueryPtr, queryBuf, lineBufferSize);
			wprintw(winQuery, "%s", s);
			wrefresh(winQuery);
			break;
		case KEY_RIGHT:
			sprintf(s, "\nAns %d.", i);
			addStringToBuffer( s, &currResponsPtr, responsBuf, lineBufferSize);
			wprintw(winRespons, "%s", s);
			wrefresh(winRespons);
			break;
		case KEY_RESIZE: // Handle resize evnents here
			takeDownWindows();
			endwin();
			refresh();
			makeWindows(LINES, COLS);
			/* Restore content */
			getmaxyx( winQuery, row, col);
			updateWinFromBuffer(winQuery, row, currQueryPtr, queryBuf, lineBufferSize);
			updateWinFromBuffer(winRespons, row, currResponsPtr, responsBuf, lineBufferSize);
			break;
		case KEY_DOWN:
			break;	
		}
		i++;
		wmove(winStat,1,1); wprintw(winStat,"%d",i);
		wrefresh(winStat);
	}
	endwin(); /* End curses mode */
}

void takedownMorseGui( void) {
	takeDownWindows();
	endwin(); /* End curses mode */
}

static WINDOW *create_newwin(int height, int width, int starty, int startx)
{
	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0);		/* 0, 0 gives default characters 
					 * for the vertical and horizontal
					 * lines			*/
	wrefresh(local_win);		/* Show that box 		*/

	return local_win;
}

static void destroy_win(WINDOW *local_win)
{
	if (NULL != local_win) {
		/* box(local_win, ' ', ' '); : This won't produce the desired
		 * result of erasing the window. It will leave it's four corners 
		 * and so an ugly remnant of window. 
		 */
		wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
		/* The parameters taken are 
		 * 1. win: the window on which to operate
		 * 2. ls: character to be used for the left side of the window 
		 * 3. rs: character to be used for the right side of the window 
		 * 4. ts: character to be used for the top side of the window 
		 * 5. bs: character to be used for the bottom side of the window 
		 * 6. tl: character to be used for the top left corner of the window 
		 * 7. tr: character to be used for the top right corner of the window 
		 * 8. bl: character to be used for the bottom left corner of the window 
		 * 9. br: character to be used for the bottom right corner of the window
		 */
		wrefresh(local_win);
		delwin(local_win);
	}
}

static void handle_winch(int sig)
{
	takeDownWindows();
	endwin();
	refresh();
	makeWindows(LINES, COLS); // Restore the application windows.
	//	mvprintw(1, 1, "COLS = %d, LINES = %d", COLS, LINES);
	//	refresh();
}

static void takeDownWindows(void)
{
	destroy_win(winQuery); destroy_win(winRespons);
	destroy_win(winQueryBox); destroy_win(winResponsBox);
}
	
static void makeWindows(const int lines, const int cols) {
	int startxRespons, starty, width, height;

	height = lines - statWinHeight;
	width = COLS / 2;
	//	starty = (LINES - height) / 2;	/* Calculating for a center placement */
	startxRespons = cols / 2; // The middle of the window.
	winQueryBox = create_newwin(height, width, 0, 0); // Create a window for the surrounding border.
    winResponsBox = create_newwin(height, width, 0, startxRespons); // Create a window for the surrounding border.
	winQuery = newwin(height-2, width-2, 1, 1);
    winRespons = newwin(height-2, width-2, 1, startxRespons+1);
	width = cols;
	if ( 0 != cols %2 ) width--; // Use the lower even width to match the two upper windows widhts.
	winStatBox = create_newwin(statWinHeight, width, lines - statWinHeight, 0);
	winStat = newwin(statWinHeight-2, width-2, 1+lines - statWinHeight, 1);
	scrollok(winQuery, TRUE);
	scrollok(winRespons, TRUE);
	scrollok(winStat, TRUE);
}

static void initLineBuffer(void)
{
	char *t = (char*)  malloc( lineBufferSize * lineSize );
	memset(t,0, lineBufferSize * lineSize);
	for( int i = 0; i < lineBufferSize; i++ ) {
		queryBuf[i] = t + i * lineSize;
	}
	t = (char*) malloc( lineBufferSize * lineSize );
	memset(t,0, lineBufferSize * lineSize);
	for( int i = 0; i < lineBufferSize; i++ ) {
		responsBuf[i] = t + i * lineSize;
	}
}

static void
updateWinFromBuffer(WINDOW  * const w, const int winHeight, const int currentPtr, char const * const * const buf, const int bufferSize)
{
	int p = currentPtr - winHeight;
	if (p < 0 ) p = bufferSize + p;
	for ( int i = 0; i < winHeight; i++ ) {
		if ( strlen(buf[p]) > 0 ) {
			wprintw(w, "%s", buf[p]);
		}
		p = (p + 1) % bufferSize;
	}
	wrefresh(w);
}

static void
addStringToBuffer( char const * const str, int * currentPtr, char * const * const buf, const int bufferSize)
{
	if (strlen(buf[*currentPtr]) > lineSize )
		strncpy(buf[*currentPtr], str, lineSize);
	else
		strcpy(buf[*currentPtr], str);
	*currentPtr = ( *currentPtr + 1 ) % bufferSize;	
}
