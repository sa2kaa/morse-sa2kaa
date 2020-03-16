#ifndef MORSE_GUI_H
#define MORSE_GUI_H
#include <ncurses.h>

extern WINDOW *winQuery, *winRespons, *winStat;
extern void initMorseGui( void);
extern void takedownMorseGui( void);
#endif
