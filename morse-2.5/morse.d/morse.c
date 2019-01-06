/* How the Ubuntu source was retreived...
apt-get build-dep morse
apt-get source morse
cd morse*
*/
/*
 * Copyright (c) 1988 Regents of the University of California.
 * Copyright (c) 1992 Joe Dellinger, University of Hawaii at Manoa
 * Copyright (c) 2005 Eric S. Raymond.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * A morse code practice utility. (Contains those characters that can appear
 * on the FCC ham license exam.)
 *
 * Running "morse" without arguments or input gives self-doc.
 *
 * It doesn't keep PERFECT time, but it seems reasonably close
 * for reasonable word speeds on my slow SUN IPC!
 *
 * This program has a long history, see the HISTORY file for details.
 */

/*
 * Useful for seeing what the interleaved reading and writing loops are
 * really up to.
 *
 * #define DEBUG
 *
 * If you want to be overwhelmed with information about the probabilities
 * of each letter being chosen.
 *
 * #define DEBUGG
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include "beep.h"

#define FREQUENCY	800.0
#define FREQUENCY2	602.0
#define VOLUME		0.5
#define ERROR_VOLUME		0.5
#define WORDS_PER_MINUTE	20.0
#define MAX_BEHINDNESS	0
#define ERROR_FREQUENCY	2000.0

static int      whichfrequ = 0;
static float    frequency1 = FREQUENCY;
static float    frequency2 = FREQUENCY2;
static float    frequency;
static float    volume = VOLUME;
static float    dot_time;
static float    dash_time;
static float    intra_char_time;
static float    inter_char_time;
static float    inter_word_time;
static float    catchup_time;
static bool     showletters = false;
static bool     showmorse = false;
static bool     wordsbefore = false;
static bool     wordsafter = false;
static bool     fancyending = true;
static bool     noticebad = false;
static bool     testing = false;
static bool     showtesting = false;
static int      keepquiet = 0;
static bool     dynamicspeed = false;
static bool     charbychar = false;
static bool	international = false;
static bool		utf8 = true;
static bool	allprosigns = false;
static bool	allpunctuation = false;
static int      tryagaincount = 1;
static float    words_per_minute = WORDS_PER_MINUTE;
static float	error_frequency = ERROR_FREQUENCY;
static float	error_volume = ERROR_VOLUME;
static float    fwords_per_minute;
static bool     randomletters = false;
#define LETMESEE 2
static bool     typeaway = 0;

static int      totalhitcount = 0;
static int      totalmisscount = 0;
static bool     helpmeflag = false;

#define MAXWORDLEN	20
#define TESTBUFSZ ((MAXWORDLEN+1)*10)
static int      wordlen = MAXWORDLEN;
static int      wordcount = -1;
static time_t   starttime;
static int      timeout = -1;
static int      testpointer = -1;
static int      testlength = 0;
static int      behindness = 0;
static int      max_behindness = MAX_BEHINDNESS;
static unsigned char teststring[TESTBUFSZ];
static int      yourpointer = -1;
static int      yourlength = 0;
static unsigned char yourstring[TESTBUFSZ];
static int      linepos = 0;
static char     *user_charset = NULL;

/*
 * How many times can a given character not be asked before
 * kicking up the probability of asking that one by one randomfactor unit.
 */
#define RIPECOUNT	64

#define TWOFIFTYSIX 256
static char    *(code[TWOFIFTYSIX]);
static int      errorlog[TWOFIFTYSIX];
static int      randomfactor[TWOFIFTYSIX];
static int      randomripe[TWOFIFTYSIX];
static int      testterminal (void);
static int      randomletter (void);

/*
 * Pointers to termcap/terminfo "smso/so" (enter_standout_mode) and "rmso/se"
 * (exit_standout_mode) terminal string capabilities (initialized respectively
 * with ANSI sequences "\E[7m" to turn the inverse video mode on and "\E[0m"
 * to turn all video attributes off).
 */
static const char           *enter_standout_mode = "\033[7m";
static const char           *exit_standout_mode = "\033[0m";

/*
 * Value of (Wrong - Right), which, if exceeded, will cause the program
 * to start prompting you. Above MAX_ERROR_THRESHOLD it will never prompt.
 * Don't let the user bank too much credit for past correct answers;
 * limit it by min(ERROR_FLOOR, error_threshold).
 */
#define MAX_ERROR_THRESHOLD		1000
#define ERROR_FLOOR			-3
static int      error_threshold = MAX_ERROR_THRESHOLD;
static int      error_floor = ERROR_FLOOR;

/*
 * How many characters behind before it decides you're having
 * trouble keeping up.
 */
#define BEHIND		1
#define WAYBEHIND	3
#define TOOFARBEHIND	6
/*
 * If SLOWPOKE or more wpm ticks go by, then it decides you are having lots
 * of trouble remembering this character, and need to be asked it more
 * often.
 */
#define SLOWPOKE	10
/* You aren't slow -- you left and came back! */
#define SLOWPOKEMAX	(50 * SLOWPOKE)
/*
 * If FASTPOKE or less wpm ticks go by, then it decides you are good at this
 * character, and need to be asked it less often.
 */
#define FASTPOKE	4

/*
 * These control how quickly the dynamicspeed option acts when you are
 * fast or slow. Easier to slow down than speed up!
 */
#define ERRORSLOWER	1.04
#define ALOTSLOWER	1.15
#define ALITTLESLOWER	1.02
#define ALITTLEFASTER	1.02

/*
 * How many inter_char_time's to give you to answer after the end of
 * a word before considering that you are not keeping up.
 * Maximum of 2.3, minimum of 0.
 * The bigger the value, the easier it is to kick in the "automatic
 * speedup" when using the "-d" option. The maximum means you have (almost)
 * right up to the beginning of the next word to answer and still have it
 * count as keeping up.
 */
#define SPORTING_RATIO 1.5

/*
 * The bigger, the more evenly things start out.
 * (Must be at least 2)
 */
#define RANDOMBASELEVEL	7
/*
 * RANDOMINCWORSE scales how badly you are punished for being wrong
 * or taking too long. RANDOMINCBETTER scales how you are rewarded for
 * answering quickly or being right.
 */
#define RANDOMINCWORSE	6
#define RANDOMINCBETTER	7
#define RANDOMMAX	(30 * RANDOMBASELEVEL)
/*
 * The average length of a random word (chosen using exponential distribution).
 * After implementing this I'm not so sure an exponential distribution
 * actually models the distribution of real word lengths in English very well.
 * It's not too bad, though, and the words themselves are all garbage anyway,
 * so what the heck.
 */
#define RANDWORDLEN	3.5

/* Put in a newline instead of a space when past this column */
#define LINELENGTH	78

/* An EOF without the EOF (%) sound */
#define SILENTEOF -2
/* Toggle tone frequency on control-G within input file */
#define FREQU_TOGGLE ((int)'\007')

/*
 * If you want the morse code to come out synchronized with the printing
 * of dots and dashes with the -m option, then define this. The problem
 * is that then the morse code sounds ratty on slower CPU's.
 * John Shalamskas (KJ9U) suggested turning the precise morse-code printing
 * synching off because he didn't like the resulting code quality!
 */
#undef FLUSHCODE

static void            new_words_per_minute (void);
static void            dowords (int  c); //OLTA
static void            morse (int c);
static void            show (char *s);
static void            testaddchar (unsigned char c);
static void            youraddchar (int c);
static void            pollyou (void);
static void            tone (float hertz, float duration, float amplitude);
static void            toneflush (void);
static void            openterminal (void);
static int             readterminal (unsigned char **string);
static void            closeterminal (void);
static void            report (void);

static void            die (), suspend ();
static void            cleanup ();

static void processMultiCharStream(unsigned char c, void (*charHandler)(int)); // OLTA added.

static void utf8PrintChar(unsigned char c);
static void utf8PrintLine(unsigned char *s);

int
main (int argc, char **argv)
{
extern char    *optarg;
extern int      optind;
int             ch;
char           *p;
int             ii;
struct sigaction handler;

    if (argc == 1 && isatty (fileno (stdin)))
    {
/*
 * SELF DOC
 */
	printf ("Usage:\n");
	printf ("morse [options] < text_file\n");
	printf ("morse [options] words words words\n");
	printf ("morse [options] -r\n");
	printf ("morse [options] -i\n");
	printf ("Options:\n");
	printf ("-i    Play what you type.\n");
	printf ("-I    Like -i but don't turn off keyboard echoing.\n");
	printf ("-r    Generate random text. Starts out slanted towards easy\n");
	printf ("      letters, then slants towards ones you get wrong.\n");
	printf ("-n NUM (default %d means random length)\n", MAXWORDLEN);
	printf ("      Make words (groups) NUM characters long.\n");
	printf ("      Valid values are between %d and %d.\n", 1, MAXWORDLEN);
	printf ("-R NUM (default 0 means unlimited)\n");
	printf ("      Set the total time (in minutes) to generate text.\n");
	printf ("-N NUM (default 0 means unlimited)\n");
	printf ("      Set the total number of words (groups) to generate.\n");
	printf ("-C \'STRING\' (default all available characters)\n");
	printf ("      Select characters to send from this STRING only.\n");
	printf ("-w words_per_minute (default %g)\n", WORDS_PER_MINUTE);
	printf ("      actual overall sending speed\n");
	printf ("-F Farnsworth_character_words_per_minute\n");
	printf ("      If specified, characters are sent at this speed, with extra\n");
	printf ("      spaces inserted to bring the overall speed down to the -w\n");
	printf ("      value. Ignored if not higher than -w.\n");
	printf ("-f frequency_in_hertz (default %g)\n", FREQUENCY);
	printf ("-v volume (zero to one, rather nonlinear, default %g)\n",
		      VOLUME);
	printf ("-g alternate_frequency (default %g)\n", FREQUENCY2);
	printf ("      (toggles via control-G in input FILE at a word break)\n");
	printf ("-e    leave off the <SK> sound at the end\n");
	printf ("-c    complain about illegal characters instead of just ignoring them\n");
	printf ("-b    print each word before doing it\n");
	printf ("-a    print each word after doing it\n");
	printf ("-l    print each letter just before doing it\n");
	printf ("-m    print morse dots and dashes as they sound\n");
#ifdef FLUSHCODE
	printf ("      (this printing-intensive option slows the wpm down!)\n");
#endif
	printf ("-t    Type along with the morse, but don't see what\n");
	printf ("      you're typing (unless you make a mistake).\n");
	printf ("      You are allowed to get ahead as much as you want.\n");
	printf ("      If you get too far behind it will stop and resync with you.\n");
	printf ("      You can force it to resync at the next word end by hitting control-H.\n");
	printf ("      Hit ESC to see how you are doing, control-D to end.\n");
	printf ("      (The rightmost space in the printout marks where the average is.\n");
	printf ("      Farther left spaces separate off blocks of letters that are\n");
	printf ("      about twice as probable as the average to occur, three times, etc.)\n");
	printf ("-T    Like -t but see your characters (after they are played).\n");
	printf ("-s    Stop after each character and make sure you get it right. (implies -t)\n");
	printf ("-q    Quietly resyncs with your input (after you make a mistake).\n");
	printf ("-p NUM (default 0)\n");
	printf ("      Make you get it right NUM times, for penance. (implies -s)\n");
	printf ("      (Yes, NUM = 0 means you can sin all you want.)\n");
	printf ("-E NUM (default %d)\n", MAX_ERROR_THRESHOLD);
	printf ("      If your count of wrong answers minus right answers for a given character\n");
	printf ("      exceeds this, the program will start prompting you.\n");
	printf ("      If %d or above, it will never prompt. (implies -t)\n", MAX_ERROR_THRESHOLD);
	printf ("-M NUM (default %d)\n", MAX_BEHINDNESS);
	printf ("      If you get more than this number of characters behind, pause until you\n");
	printf ("      do your next letter. (1 behind is normal, 0 behind means never pause.)\n");
	printf ("      (implies -t)\n");
	printf ("-d    Dynamically speed up or slow down depending on how you are doing.\n");
	printf ("      (if also -s, then -d _only speeds up_!)\n");
	printf ("-A    Add ISO 8850-1 (Latin-1) signs to test set.\n");
	printf ("-B    Add uncommon punctuation to test set.\n");
	printf ("-S    Add uncommon prosigns to test set.\n");
	printf ("-X    Set error volume. Defaults to %g.\n", ERROR_VOLUME);
	printf ("      Error volume 0 means use console speaker.\n");
	printf ("-x    Set frequency of error tone, default 2000.0Hz\n");
	exit (0);
    }

    /* DGHJKLMOPQUVWYZhjkouyz are still available */
    while ((ch = getopt (argc, argv, "ABC:E:F:IM:N:R:STX:abcdef:g:ilmn:p:qrstv:w:x:")) != EOF)
	switch ((char) ch)
	{
	case 'A':
	    international = true;
	    break;
	case 'B':
	    allpunctuation = true;
	    break;
	case 'C':
	    user_charset = optarg;
	    break;
	case 'E':
	    testing = true;
	    error_threshold = atoi (optarg);
	    if (error_threshold < error_floor)
		error_floor = error_threshold;
	    break;
	case 'F':
	    fwords_per_minute = atof (optarg);
	    break;
	case 'I':
	    typeaway = LETMESEE;
	    break;
	case 'M':
	    testing = true;
	    max_behindness = atoi (optarg);
	    if (max_behindness < 1)
		max_behindness = 0;
	    break;
	case 'N':
	    wordcount = atoi(optarg);
	    if (wordcount < 1) wordcount = -1;
	    break;
	case 'R':
	    timeout = atoi (optarg);
	    timeout *= 60;
	    if (timeout < 1) timeout = -1;
	    break;
	case 'S':
	    allprosigns = true;
	    break;
	case 'T':
	    testing = true;
	    showtesting = true;
	    break;
	case 'X':
	    error_volume = atof (optarg);
	    break;
	case 'a':
	    wordsafter = true;
	    break;
	case 'b':
	    wordsbefore = true;
	    break;
	case 'c':
	    noticebad = true;
	    break;
	case 'd':
	    dynamicspeed = true;
	    break;
	case 'e':
	    fancyending = false;
	    break;
	case 'f':
	    frequency1 = atof (optarg);
	    break;
	case 'g':
	    frequency2 = atof (optarg);
	    break;
	case 'i':
	    typeaway = true;
	    break;
	case 'l':
	    showletters = true;
	    break;
	case 'm':
	    showmorse = true;
	    break;
	case 'n':
	    wordlen = atoi (optarg);
	    if (wordlen < 1) wordlen = 1;
	    if (wordlen > MAXWORDLEN) wordlen = MAXWORDLEN;
	    break;
	case 'p':
	    charbychar = true;
	    testing = true;
	    tryagaincount = atoi (optarg);
	    break;
	case 'q':
	    keepquiet = 1;
	    break;
	case 'r':
	    randomletters = true;
	    break;
	case 's':
	    charbychar = true;
	    testing = true;
	    break;
	case 't':
	    testing = true;
	    break;
	case 'v':
	    volume = atof (optarg);
	    if (volume < 0.)
		volume = 0.;
	    if (volume > 1.)
		volume = 1.;
	    break;
	case 'w':
	    words_per_minute = atof (optarg);
	    break;
	case 'x':
	    error_frequency = atof (optarg);
	    break;
	default:
	    fprintf (stderr, "Type \"morse\" without arguments to get self-doc!\n");
	    exit (1);
	    break;
	}
    argc -= optind;
    argv += optind;

    if (fwords_per_minute <= 0.)
	fwords_per_minute = words_per_minute;
    new_words_per_minute ();

    frequency = frequency1;

    if (BeepInit () != 0)
    {
	fprintf (stderr, "Can't access speaker.\n");
	exit (1);
    }

    for (ii = 0; ii < TWOFIFTYSIX; ii++)
	code[ii] = NULL;

/* Load in the morse code code */
    code[(int) 'a'] = ".-";
    code[(int) 'b'] = "-...";
    code[(int) 'c'] = "-.-.";
    code[(int) 'd'] = "-..";
    code[(int) 'e'] = ".";
    code[(int) 'f'] = "..-.";
    code[(int) 'g'] = "--.";
    code[(int) 'h'] = "....";
    code[(int) 'i'] = "..";
    code[(int) 'j'] = ".---";
    code[(int) 'k'] = "-.-";
    code[(int) 'l'] = ".-..";
    code[(int) 'm'] = "--";
    code[(int) 'n'] = "-.";
    code[(int) 'o'] = "---";
    code[(int) 'p'] = ".--.";
    code[(int) 'q'] = "--.-";
    code[(int) 'r'] = ".-.";
    code[(int) 's'] = "...";
    code[(int) 't'] = "-";
    code[(int) 'u'] = "..-";
    code[(int) 'v'] = "...-";
    code[(int) 'w'] = ".--";
    code[(int) 'x'] = "-..-";
    code[(int) 'y'] = "-.--";
    code[(int) 'z'] = "--..";

    code[(int) '1'] = ".----";
    code[(int) '2'] = "..---";
    code[(int) '3'] = "...--";
    code[(int) '4'] = "....-";
    code[(int) '5'] = ".....";
    code[(int) '6'] = "-....";
    code[(int) '7'] = "--...";
    code[(int) '8'] = "---..";
    code[(int) '9'] = "----.";
    code[(int) '0'] = "-----";

    /* Punctuation marks */
    code[(int) '.'] = ".-.-.-";
    code[(int) ','] = "--..--";
    code[(int) '?'] = "..--..";
    code[(int) '/'] = "-..-.";
    code[(int) '-'] = "-....-";

    if (allpunctuation) {
    /* Not so commonly used punctuation marks */
    code[(int) ')'] = "-.--.-";
    code[(int) '\"'] = ".-..-.";
    code[(int) '_'] = "..--.-";
    code[(int) '\''] = ".----.";
    code[(int) ':'] = "---...";
    code[(int) ';'] = "-.-.-.";
    code[(int) '$'] = "...-..-";
    code[(int) '!'] = "-.-.--";
    code[(int) '@'] = ".--.-.";
    }

    if (allpunctuation) {
    /* Commonly used procedure signs ("prosigns") */
    code[(int) '+'] = ".-.-.";  /* <AR> end of message */
    code[(int) '*'] = ".-...";  /* <AS> wait, stand-by */
    code[(int) '='] = "-...-";  /* <BT> (double dash) pause, break for text */
    code[(int) '('] = "-.--.";  /* <KN> over-specified station only */
    code[(int) '%'] = "...-.-"; /* <SK> end of contact, known also as <VA> */
    }

    if (allprosigns) {
    /* Not so commonly used procedure signs ("prosigns") */
    code[(int) '^'] = ".-.-";   /* <AA> new line, the same as :a, ae */
    code[(int) '#'] = "-...-.-";/* <BK> invite receiving station to transmit */
    code[(int) '&'] = "-.-.-";  /* <KA> attention */
    code[(int) '~'] = "...-.";  /* <SN> understood */
    }

    if (international) {
    /* Not so commonly used international extensions (ISO 8859-1) */
    code[(int) ((unsigned char)'\344')] = ".-.-";   /* :a (also for ae), the same as <AA> */
    if (code[(int) '^'] != NULL) code[(int) '^'] = NULL;
    code[(int) ((unsigned char)'\340')] = ".--.-";  /* `a, oa (danish a with ring over it) */
//    code[(int) ((unsigned char)'\347')] = "----";   /* ch (bar-ch) */
//    code[(int) ((unsigned char)'\360')] = "..--.";  /* -d (eth, overstrike d with -) */
//    code[(int) ((unsigned char)'\350')] = "..-..";  /* `e */
//    code[(int) ((unsigned char)'\361')] = "--.--";  /* ~n */
    code[(int) ((unsigned char)'\366')] = "---.";   /* :o (also for oe) */
//    code[(int) ((unsigned char)'\374')] = "..--";   /* :u (also for ue) */
//    code[(int) ((unsigned char)'\376')] = ".--..";  /* ]p (thorn, overstrike ] with p) */
//    code[(int) ((unsigned char)'\247')] = ".-.-.."; /* paragraph */
    }

    if (user_charset)
    {
	for (ii = 0; ii < strlen(user_charset); ii++)
	{
	    *(user_charset+ii) = tolower(*(user_charset+ii));
	}
	for (ii = 0; ii < TWOFIFTYSIX; ii++)
	{
	    if (code[ii] != NULL)
	    {
		if (strchr(user_charset, ii) != NULL)
		{
		    randomfactor[ii] = RANDOMBASELEVEL - 1;
		} else {
		    code[ii] = NULL;
		    randomfactor[ii] = 0;
		}
	    }
	}
    }

    for (ii = 0; ii < TWOFIFTYSIX; ii++)
    {
	/* Everything starts equally fresh */
	randomripe[ii] = 0;
	/* Start out assuming you know how everything sounds */
	errorlog[ii] = 0;

	if (code[ii] == NULL)
	{
	    /* Ensures these will never be chosen */
	    randomfactor[ii] = 0;
	}
	else
	{
	    /* Start out favoring easy ones */
	    randomfactor[ii] = RANDOMBASELEVEL - strlen (code[ii]);
	    if (randomfactor[ii] < 1)
		randomfactor[ii] = 1;
	}
    }

    words_per_minute = WORDS_PER_MINUTE;
    fwords_per_minute = -1.;

    time(&starttime);

    handler.sa_handler = die;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    sigaction (SIGINT, &handler, NULL);
    sigaction (SIGTERM, &handler, NULL);
    sigaction (SIGQUIT, &handler, NULL);
    handler.sa_handler = suspend;
    sigaction (SIGTSTP, &handler, NULL);

    if (testing || typeaway)
    {
	openterminal ();
    }

/*
 * Do 0.25 seconds of silence initially to give the workstation time to
 * get settled after the stress of starting this program and opening
 * up everything.
 */
    tone (frequency, 0.25, 0);
    toneflush ();

    if (typeaway)
    {
	bool notdoneyet;

	testing = false;
	showtesting = false;
	charbychar = 0;
	wordsbefore = 0;
	wordsafter = 0;
	randomletters = 0;

	notdoneyet = true;

	while (notdoneyet)
	{
	    int jj;

	    pollyou ();

	    for (jj = 0; jj < yourlength; jj++)
	    {
		int             yourchar;

		yourchar = yourstring[(yourpointer - yourlength + 1 + jj + TESTBUFSZ) % TESTBUFSZ];

		/* Control-D: finished */
		if (yourchar == (int) '\004')
		{
		    toneflush ();
		    notdoneyet = false;
		    break;
		}

		if (isspace (yourchar))
		{
		    if (showletters)
		    {
			toneflush ();
			utf8PrintChar( yourchar );
			fflush (stdout);
		    }

		    tone (frequency, inter_word_time, 0.);

		    continue;
		}
		morse (yourchar);
	    }
	    yourlength -= jj;

	    if (timeout > 0)
	      if (difftime(time(NULL),starttime) > (double)timeout)
		break;
	}
    }
    else if (randomletters)
    {
	float           randexp, randnum;

	srand48(time (NULL));
	randexp = 1. / (1. - 1. / (float) (RANDWORDLEN));
	while (1)
	{
	    if ((wordcount == 0) || timeout == 0) break;

	    dowords (randomletter ());

	    if (wordlen == MAXWORDLEN)
	      {
		/* Knock a few bits off the top so we're sure it won't overflow */
		/* Shift a few bits because the lower bits stink */
		/* Add in the time so it doesn't repeat from run to run */
		randnum = (float) (
				   (((lrand48() >> 9) + (long) (time (NULL))) >> 4)
				   & 0x00FFFFFF);
		randnum = randnum - randexp * (int) (randnum / randexp);
		if ((randnum >= 1.) && (linepos != 0)) dowords ((int) ' ');
	      }
	}
    }
    else
    {
	if (*argv)
	{
	    bool             firsttime;

	    firsttime = true;

	    do
	    {
		if (!firsttime)
		{
		    dowords ((int) ' ');
		}
		else
		    firsttime = false;

		if ((wordcount == 0) || timeout == 0) break;

		for (p = *argv; *p; ++p)
		  {	
			
			processMultiCharStream( (unsigned char) *p, dowords); //OLTA
		    if ((wordcount == 0) || timeout == 0) break;
		  }
		if ((wordcount == 0) || timeout == 0) break;

	    } while (*++argv);
	}
	else
	{
	    while ((ch = getchar ()) != EOF)
	    {
			processMultiCharStream( (unsigned char) ch, dowords); //OLTA
			if ((wordcount == 0) || timeout == 0) break;
	      }
	}
    }

    if (fancyending)
	dowords (EOF);
    else
	dowords (SILENTEOF);

    fflush (stdout);

    if (testing)
    {
	/*
	 * WE'RE completely done, and YOU aren't! Force catch up. (Note if
	 * charbychar = YES we won't get here, since we're always caught up
	 * after each character as it comes out.)
	 */
	while (testlength > 0)
	{
	    tone (frequency, catchup_time, 0.);
	    toneflush ();
	    testterminal ();
	}
    }

    /* Just to be sure! */
    toneflush ();

    if (showmorse || wordsbefore || wordsafter || showletters || showtesting)
	printf ("\n");
    fflush (stdout);

    if (testing)
	report ();

/* If you make any mistakes exit with a return code! */
    cleanup ();
    return (totalmisscount > 0);
}

static void
new_words_per_minute ()
{
float           wtick, ftick, tick;

    tick = 60. / (words_per_minute * 50);

    /*
     * In the limit as wpm goes past fwpm, Farnsworth becomes kosher PARIS
     */
    if (fwords_per_minute <= words_per_minute)
	ftick = 60. / (words_per_minute * 50);
    else
	ftick = 60. / (fwords_per_minute * 50);

    wtick = (50. * tick - 31. * ftick) / 19.;

    /*
     * This time is used when the computer is waiting on you to hit a key; it
     * is useful to scale the granularity with the real overrall words per
     * minute. This also serves as a measuring rod to see if you are
     * responding "fast enough". If you are too slow, then obviously you are
     * having trouble with that character, and should be given it more OFTEN.
     * Heh heh heh...
     */
    catchup_time = tick;

    /*
     * Things between characters and words go at the "remainder" speed,
     * whatever space you need to make the sped-up Farnsworth characters come
     * out with the correct overall words per minute.
     */
    inter_char_time = wtick * 3.;
    inter_word_time = wtick * 7.;

    /* Things within the character go at the Farnsworth speed */
    intra_char_time = ftick;
    dot_time = ftick;
    dash_time = ftick * 3.;
}

static int      tryingagain = 0, slowpoke = 0;

static void
dowords (int c)
{
static int      wordc = 0;
static char     word[MAXWORDLEN+1];
unsigned char           *wordp;

/*
 * Increment the line position counter.
 * If a line gets too long, just cut it off by inserting a new line.
 */
    if (c != EOF && c != SILENTEOF && c != FREQU_TOGGLE) linepos++;
    if (isspace (c) && ((linepos + wordlen) >= LINELENGTH)) c = '\n';
    if (c == '\n') linepos = 0;

    if (isspace (c) || c == EOF || c == SILENTEOF || c == FREQU_TOGGLE)
    {
	if (wordc > 0)
	{
	    int againcount;

	    if (wordcount > 0) wordcount--;
	    if (timeout > 0)
	      if (difftime(time(NULL),starttime) > (double)timeout)
		timeout = 0;

	    word[wordc] = '\0';

/*
 * We have just read in a new complete word from the input, (hopefully)
 * during the time of an inter-word space minus an inter-char space.
 * Now let's go back and see what happened with the PREVIOUS word,
 * the one that we had just finished playing. Did the user keep
 * up with us?
 */
#ifdef DEBUG
	    fprintf (stderr, " [%d] ", behindness);
#endif
	    if (testing && dynamicspeed && !charbychar)
	    {
		/*
		 * (If charbychar then behindness is ALWAYS 0 at this
		 * point...)
		 */
		if (behindness == 0)
		{
		    /* You're a speed demon! Speed up a bit, then! */
		    words_per_minute *= ALITTLEFASTER;
		    new_words_per_minute ();
		}
		else if (behindness > WAYBEHIND)
		{
		    /* You're way behind! Slow way down. */
		    words_per_minute /= ALOTSLOWER;
		    new_words_per_minute ();
		}
		else if (behindness > BEHIND)
		{
		    /* You're behind! Slow down a bit. */
		    words_per_minute /= ALITTLESLOWER;
		    new_words_per_minute ();
		}
	    }


/*
 * If the user was WAY too far behind stop and catch up with
 * the "new" word as the first one.
 */
	    if (testing && (behindness > TOOFARBEHIND || helpmeflag))
	    {
		if (helpmeflag)
		    printf ("\nOK, let's restart.\n");
		else
		    printf ("\nYou are too far behind! Let's restart.\n");
		fflush (stdout);

		toneflush ();

		/* Flush the keyboard buffer */
		pollyou ();

		/* Forget the past */
		helpmeflag = false;
		behindness = 0;
		testlength = 0;
		yourlength = 0;

		/* Give the user a little rest. */
		sleep (2);
		printf ("\nWPM now %d\n", (int) (words_per_minute + .5));
		fflush (stdout);
		sleep (2);
		printf ("\nREADY?\n");
		fflush (stdout);
		sleep (1);
		printf ("\nSET\n");
		fflush (stdout);
		sleep (1);
		printf ("\nGO!\n");
		fflush (stdout);
		linepos = 0;
	    }

/*
 * Start treating the new word.
 */
	    if (wordsbefore)
	    {
		/* Try to keep your out-of-sync text from getting swirled in */
		if (showtesting)
		  {
		    printf ("\n");
		    linepos = 0;
		  }

		utf8PrintLine( word );

		if (showmorse || showletters || wordsafter || showtesting)
		{
		    int             ii;

		    printf ("  ");
		    for (ii = 0; ii < 16 - (wordc + 2); ii++)
		    {
			printf (" ");
		    }
		}

		fflush (stdout);
	    }

	    if (testing && charbychar)
	    {
		againcount = 0;
	    }

	    for (wordp = word; *wordp != '\0'; wordp++)
	    {
		tryingagain = 0;

	tryagain:
		if (testing && !tryingagain && !showletters &&
		    error_threshold < MAX_ERROR_THRESHOLD &&
		    errorlog[(int) *wordp] > error_threshold)
		{
		    toneflush ();
		    /* Give them a quick hint */
		    printf( "[" );
			utf8PrintChar( *wordp );
			printf( "]" );
		    fflush (stdout);

		    morse (*wordp);

		    toneflush ();
		    if (!showmorse)
		    {
			/* Erase the hint */
			printf ("\b\b\b   \b\b\b");
			fflush (stdout);
		    }
		}
		else
		{
		    morse (*wordp);
		}

		if (testing)
		{
		    if (charbychar)
		    {
			toneflush ();
			/* Force catchup */
			slowpoke = 0;
			while (behindness > 0)
			{
			    if (testterminal () && tryagaincount > 0)
			    {
				/*
				 * OOPS! They got it WRONG! MAKE THEM TRY
				 * AGAIN!
				 */
				printf ("Try again.\n");
				linepos = 0;
				/*
				 * Yeah I know gotos are inelegant but I
				 * don't feel like figuring out the "elegant"
				 * way to do this right now.
				 */
				againcount = tryagaincount - 1;
				tryingagain = 1;
				goto tryagain;
			    }
			    else
			    {
				/*
				 * They got it right, or they didn't answer
				 * yet.
				 */
				if (behindness > 0)
				{
				    /*
				     * They are STILL thinking, the
				     * slowpokes. Wait a bit before trying
				     * again.
				     */
				    tone (frequency, catchup_time, 0.);
				    /*
				     * Keep track of how long they're taking
				     * to answer!
				     */
				    if (slowpoke < SLOWPOKEMAX)
					slowpoke++;
				    toneflush ();
				}
				else if (dynamicspeed && !slowpoke && !tryingagain)
				{
				    /*
				     * They got it right without errors the
				     * first time and we didn't have to wait
				     * for them! A speed demon! Speed up a
				     * bit, then!
				     */
				    words_per_minute *= ALITTLEFASTER;
				    new_words_per_minute ();
				}
			    }
			}

			/* Insufficient penance? */
			if (againcount > 0)
			{
			    againcount--;
			    goto tryagain;
			}
		    }
		    else
		    {
			testterminal ();
			/*
			 * Stop if we get more than max_behindness ahead.
			 * max_behindness == 0 means don't worry about them,
			 * they can be as far behind as they want and we
			 * won't stop!
			 */
			if (max_behindness > 0)
			{
			    bool             are_we_repeating = false;

			    while (behindness >= max_behindness)
			    {
#ifdef DEBUG
				fprintf (stderr, " (%d) ", behindness);
#endif
				if (are_we_repeating)
				{
				    /*
				     * Pause for a bit so we don't loop too
				     * fast
				     */
				    tone (frequency, catchup_time, 0.);
				}
				else
				{
				    are_we_repeating = true;
				}
				/* Finish playing whatever we're playing */
				toneflush ();
				/* And give them another chance */
				testterminal ();
			    }
			}
		    }
		}
	    }

	    toneflush ();
	    if (testing)
		testterminal ();

	    if (wordsafter) {	
			printf( "(" );
			utf8PrintLine( word );
			printf( ")" );
	    }

	    if (wordsbefore || wordsafter || showmorse)
	      {
		if (!showmorse && !testing && linepos)
		  {
		    if (wordsafter)
		      linepos += 3;
		    else
		      printf (" ");
		  } else {
		    printf ("\n");
		    linepos = 0;
		  }
	      }
	    else if (showletters || showtesting)
	    {
		if (c != EOF && c != SILENTEOF && c != FREQU_TOGGLE)
		{
		    if (showletters)
			utf8PrintChar( c );

		    if (showtesting)
			testaddchar (c);
		}
	    }

/*
 * WHEW! FINISHED QUEUEING THE WORD FOR PLAYING!
 * Now finish up all the other sundry details...
 */

	    /* Flush the output printing queue... */
	    fflush (stdout);
	    /*
	     * Pause for a bit; this gives the user a sporting chance at
	     * catching up with us.
	     */
	    tone (frequency, SPORTING_RATIO * inter_char_time, 0.);
	    toneflush ();
	    /* Start sounding an inter-word space */
	    tone (frequency, inter_word_time - SPORTING_RATIO * inter_char_time, 0.);

	    /* While that silence is playing check if the user has caught up. */
	    if (testing)
		testterminal ();

	    /* We finished this word; reset the word character count */
	    wordc = 0;
	}
	else if (!(wordsbefore || wordsafter || showmorse)
		 &&
		 (showletters || showtesting))
	{
	    if (c != EOF && c != SILENTEOF && c != FREQU_TOGGLE)
	    {
		if (showletters)
		    utf8PrintChar( c );

		if (showtesting)
		    testaddchar (c);
	    }
	}

	if (c == EOF)
	{
	    morse (EOF);
	    toneflush ();
	}
	else if (c == SILENTEOF)
	{
	    toneflush ();
	}
	else if (c == FREQU_TOGGLE)
	{
	    /* Switch to the other frequency */
	    /* (Won't work from keyboard, only from a file.) */
	    whichfrequ = 1 - whichfrequ;
	    switch (whichfrequ)
	    {
	    case 1:
		frequency = frequency2;
		break;
	    case 0:
	    default:
		frequency = frequency1;
		break;
	    }
	}
    }
    else
    {
	word[wordc++] = c;
    }

/*
 * If a word gets too long, just cut it off by inserting a space.
 * Just call ourselves with the character we wish we'd gotten...
 */
    if (wordc == wordlen && !(isspace (c) || c == EOF || c == SILENTEOF || c == FREQU_TOGGLE))
	dowords ((int) ' ');
}

/*
 * Don't try to test the person DURING the call into morse!
 */
void
morse (int c)
{
    if ((isalpha (c) && (code[tolower(c)] == NULL)) || ((code[(int) '%'] == NULL) && ((c == EOF) || (c == '\004')))) {
    	c = ' ';
	}
    if (showletters) {
    	if ((c == EOF) || (c == '\004')) {
	  		if (showmorse) {
				printf ("<SK>");
	    	} 
			else {
				printf ("%%");
	      	}
	  	}
		else if (c == '.' && showmorse)
		    printf ("<DOT>");
		else if (c == '-' && showmorse)
		    printf ("<DASH>");
		else if (c == '+' && showmorse)
		    printf ("<AR>");
		else if (c == '*' && showmorse)
		    printf ("<AS>");
		else if (c == '=' && showmorse)
		    printf ("<BT>");
		else if (c == '(' && showmorse)
		    printf ("<KN>");
		else if (c == '%' && showmorse)
		    printf ("<SK>");
		else if (allprosigns) {
		    if (c == '^' && (code[(int) '^'] != NULL) && showmorse)
				printf ("<AA>");
	    	else if (c == '#' && showmorse)
				printf ("<BK>");
	    	else if (c == '&' && showmorse)
				printf ("<KA>");
	    	else if (c == '~' && showmorse)
				printf ("<SN>");
    	}
		else
	    	utf8PrintChar( c );

		fflush (stdout);
    }

    if (isalpha (c))
    {
	if (testing)
	    testaddchar (c - (isupper (c) ? 'A' : 'a') + 'a');
	show (code[c - (isupper (c) ? 'A' : 'a') + 'a']);
    }
    else if ((c == EOF) || (c == '\004'))
    {
	show (code[(int) '%']);
    }
    else if (code[c] != NULL)
    {
	if (testing)
	    testaddchar (c);
	show (code[c]);
    }
    else
    {
	/* Oops! This letter is junk! */

	if (noticebad)
	{
	    if (showletters)
	    {
		fflush (stdout);
	    }

	    /* Simulate a stumble */
	    tone (frequency, 2. * inter_word_time, 0.);
	    toneflush ();
	}

	if (showletters)
	{
	    /* Wipe out what we just printed */
	    fflush (stdout);
	    printf ("\b");
	    printf (" ");
	    printf ("\b");
	    fflush (stdout);
	}

	if (noticebad)
	{
	    if (showletters)
	    {
		/* And replace it with an error message */
		printf ("<UNKNOWN_CHARACTER>");
		fflush (stdout);
	    }

	    /* Give the error call */
	    show ("........");

	    /* Regroup */
	    tone (frequency, inter_word_time, 0.);
	}
    }

    if (showmorse)
	printf (" ");
    fflush (stdout);
    toneflush ();
    tone (frequency, inter_char_time - intra_char_time, 0.);
}


/*
 * Don't try to test the person WHILE doing dots and dashes!
 */
static void
show (char *s)
{
char            c;

    while ((c = *s++) != '\0')
    {
	tone (frequency, intra_char_time, 0.);

#ifdef FLUSHCODE
	if (showmorse)
	    toneflush ();
#endif

	switch (c)
	{
	case '.':
	    tone (frequency, dot_time, volume);
	    break;
	case '-':
	    tone (frequency, dash_time, volume);
	    break;
	}

	if (showmorse)
	{
	    utf8PrintChar( c );
	    fflush (stdout);
#ifdef FLUSHCODE
	    toneflush ();
#endif
	}
    }
}

/*
 * This only gets passed valid characters: ones
 * that have a morse code associated with them
 * or ones for which isspace(c) is true.
 */
static void
testaddchar (unsigned char c)
{
    testpointer = (testpointer + 1) % TESTBUFSZ;
    teststring[testpointer] = c;
#ifdef DEBUG
    fprintf (stderr, " (%c,%d,%d) ", c, testlength, behindness); // TODO olta fix utf8 debug
#endif
    testlength++;
    if (testlength > TESTBUFSZ) {
		fprintf (stderr, "\n\nInput buffer queue overflow! Make TESTBUFSZ bigger!\n");
		fprintf (stderr, "(Or don't fall so far behind)\n");

		die ();
    }

/*
 * Since you are never asked to type spaces (you can type them if
 * you want, but they are ignored) spaces in the input file don't
 * count against your "behindness".
 */
    if (!isspace (c))
	behindness++;
}

static void
youraddchar (int c)
{
    yourpointer = (yourpointer + 1) % TESTBUFSZ;
    yourstring[yourpointer] = (unsigned char) c;
#ifdef DEBUG
    fprintf (stderr, " <%c,%d> ", c, yourlength); // TODO OLTA fix uf8 debug
#endif
    yourlength++;
    if (yourlength > TESTBUFSZ)
    {
	fprintf (stderr, "\n\nKeyboard typeahead buffer queue overflow! Make TESTBUFSZ bigger!\n");
	fprintf (stderr, "(Or don't type so far ahead... how did you expect to get them right anyway?)\n");

	die ();
    }
}

static void processMultiCharStream(unsigned char c, void (*charHandler)(int))
{
	static bool multiChar = false;	
	if ( utf8 ) {
		if ( multiChar ) {
			switch ( c ) {
				case 0xa5: 
 					charHandler((int) ((unsigned char)'\340')); // LATIN SMALL LETTER A WITH RING ABOVE
					break;
				case 0xa4:
					charHandler((int) ((unsigned char)'\344')); // LATIN SMALL LETTER A WITH DIAERESIS
					break;
				case 0xb6:
					charHandler((int) ((unsigned char)'\366')); // LATIN SMALL LETTER O WITH DIAERESIS
					break;
			}
			multiChar = false;
		}
		else if ( 0xc3 == c ) {
			multiChar = true;
		}
		else {
			charHandler( c );
		}
	}
}
/*
*  Transcode selected characters from Latin-1 to UTF-8
*/
static void utf8PrintChar(unsigned char c) {
	switch( c ) {
		case (unsigned char) '\340':
			printf("\xc3\xa5");
			break;
		case (unsigned char) '\344':
			printf("\xc3\xa4");
			break;
		case (unsigned char) '\366':
			printf("\xc3\xb6");
			break;
		default:
			printf("%c", c);
			break;
	}
}

static void utf8PrintLine(unsigned char *s)
{
	const size_t length = strlen(s);
	
	for ( int i = 0; i < length; i++ )
		utf8PrintChar( s[i] );
}

static void
pollyou (void)
{
    int ii, num;
    unsigned char *string;

    num = readterminal (&string);
	bool multiChar = false;
    for (ii = 0; ii < num; ii++) {
		processMultiCharStream(string[ii], youraddchar);
	}
}

static int
testterminal (void)
{
int             errorcount;
int             resync;

    errorcount = 0;

/*
 * There is nothing in the input file queue right now,
 * so we can't process any of your keystrokes.
 * Defer processing until we can catch up with YOU!
 */
    if (testlength == 0)
	return errorcount;

    /* We're ready for you; but are you ready for us? */
    pollyou ();

/*
 * Process your entries and the input queue entries in parallel
 */
    if (yourlength > 0 && testlength > 0)
    {
	int             testinc, yourinc;

	for (testinc = 0, yourinc = 0;
	     testinc < testlength && yourinc < yourlength;
	     testinc++, yourinc++)
	{
	    int             correctchar, yourchar, yourcharnocase;

	    correctchar = teststring[(testpointer - testlength + 1 + testinc + TESTBUFSZ) % TESTBUFSZ];

	    /*
	     * The latter half of this if shouldn't be necessary, but just in
	     * case...
	     */
	    if (isspace (correctchar) || code[correctchar] == NULL)
	    {
		if (showtesting)
		{
		    utf8PrintChar( correctchar );
		    fflush (stdout);
		}

		/* White space doesn't count for "behindness" */
		behindness++;
		/* The _other_ pointer wasn't used; don't increment it. */
		yourinc--;

		/* Short circuit the loop */
		continue;
	    }


	    yourchar = yourstring[(yourpointer - yourlength + 1 + yourinc + TESTBUFSZ) % TESTBUFSZ];
	    if (isalpha (yourchar)) {
			yourcharnocase = yourchar - (isupper (yourchar) ? 'A' : 'a') + 'a';
		}
		else {
			yourcharnocase = yourchar;
		}
	    /* Did you type something rude? If so, just ignore it. */
	    if (isspace (yourchar) || code[yourcharnocase] == NULL)
	    {
		/* ESCAPE: dump status info */
		/* Control-D: dump status info and then bye bye */
		/* Control-H: force restart */
		if (yourchar == '\033' || yourchar == (int) '\004')
		{
		    report ();

		    if (yourchar == (int) '\004')
			die ();
		}
		else if (yourchar == '\b')
		{
		    helpmeflag = true;
		}

		/* The _other_ pointer wasn't used; don't increment it. */
		testinc--;
		/* Short circuit the loop */
		continue;
	    }

	    if (keepquiet > 1)
	      {
		for (resync = testinc; resync < testlength; resync++)
		  if ( yourcharnocase == teststring[(testpointer - testlength + 1 + resync + TESTBUFSZ) % TESTBUFSZ])
		    {
		      for (; testinc < resync; testinc++)
			{
			  correctchar = teststring[(testpointer - testlength + 1 + testinc + TESTBUFSZ) % TESTBUFSZ];
			  if (isspace (correctchar) || code[correctchar] == NULL)
			    {
			      if (showtesting) utf8PrintChar ( correctchar );
			    } else {
			      printf("%s", enter_standout_mode );
				  utf8PrintChar( correctchar );
				  printf("%s", exit_standout_mode);
			    }
			}
		      fflush (stdout);
		      correctchar = teststring[(testpointer - testlength + 1 + testinc + TESTBUFSZ) % TESTBUFSZ];
		      keepquiet = 1;
		      break;
		    }
	      }

	    if (yourcharnocase != correctchar)
	    {
		errorcount++;
		totalmisscount++;

		/*
		 * Record that you are having trouble with these.
		 */
		errorlog[correctchar]++;
		if (code[yourcharnocase] != NULL &&
		    errorlog[yourcharnocase] < MAX_ERROR_THRESHOLD)
		    errorlog[yourcharnocase]++;

		if (keepquiet > 0)
		  {
		    printf( "%s", enter_standout_mode );
			utf8PrintChar( correctchar );
		    printf( "%s", exit_standout_mode );

		    fflush (stdout);
		    keepquiet++;
		  } else {
		    /*
		     * Scold them for their mistake.
		     */
		    printf ("\n");

		    if (error_volume) {
			/* Beep using the tone generator */
			toneflush ();
			tone (error_frequency, 0.1, error_volume);
		    } else
			/* Beep using control-G */
			printf( "\007" );
			utf8PrintChar( yourchar );
			printf( " (%s) for ",code[yourcharnocase] );
			utf8PrintChar( correctchar );
		    printf( "(%s)\n", code[correctchar] );
		    fflush( stdout );
		    linepos = 0;
		  }

		if (charbychar)
		{
		    /* Give them a bit of time to think about their error */
		    tone (frequency, inter_word_time, 0.);
		    toneflush ();
		}
		if (dynamicspeed && !charbychar)
		{
		    /*
		     * Slow down. Doesn't make sense to slow down for errors,
		     * though, if you've got all the time you want to think
		     * about each one.
		     */
		    words_per_minute /= ERRORSLOWER;
		    new_words_per_minute ();
		}
		if (randomletters && !tryingagain)
		{
		    /*
		     * Ask ones that confused you more often!
		     */
		    if (code[yourcharnocase] != NULL)
		    {
			randomfactor[yourcharnocase] += (3 * RANDOMINCWORSE / 2);
			if (randomfactor[yourcharnocase] > RANDOMMAX)
			    randomfactor[yourcharnocase] = RANDOMMAX;
		    }

		    randomfactor[correctchar] += RANDOMINCWORSE * 2;
		    if (randomfactor[correctchar] > RANDOMMAX)
			randomfactor[correctchar] = RANDOMMAX;
		}
	    }
	    else
	    {
		/*
		 * Record that you got this right.
		 */
		if (!tryingagain)
		{
		    totalhitcount++;
		    if (errorlog[correctchar] > error_floor)
			errorlog[correctchar]--;
		}

		if (showtesting)
		{
		    utf8PrintChar( yourchar );
		    fflush (stdout);
		}

		if (randomletters && !tryingagain)
		{
		    if (slowpoke == SLOWPOKEMAX)
		    {
			printf ("\nNice to have you back again, I was getting bored!\n");
			linepos = 0;
		    }
		    else if (slowpoke >= SLOWPOKE * 3)
		    {
			/*
			 * Did you take too long thinking about it? If so,
			 * you probably need to be asked this one more
			 * often...
			 */
			randomfactor[correctchar] += (3 * RANDOMINCWORSE / 2);
			if (randomfactor[correctchar] > RANDOMMAX)
			    randomfactor[correctchar] = RANDOMMAX;

			/*
			 * Hits this slow shouldn't count! You were obviously
			 * just guessing! (But it doesn't count as an error
			 * either.)
			 */
			totalhitcount--;
		    }
		    else if (slowpoke > FASTPOKE)
		    {
			randomfactor[correctchar] +=
			 (slowpoke * RANDOMINCWORSE) / (2 * SLOWPOKE);
			if (randomfactor[correctchar] > RANDOMMAX)
			    randomfactor[correctchar] = RANDOMMAX;
		    }
		    else if (slowpoke <= (FASTPOKE / 2))
		    {
			/*
			 * Ask ones that you quickly answer correctly less
			 * often!
			 */
			randomfactor[correctchar] -= (3 * RANDOMINCBETTER / 2);
			/*
			 * Don't let randomfactor hit 0, or you'll NEVER be
			 * asked this one AGAIN!
			 */
			if (randomfactor[correctchar] < 1)
			    randomfactor[correctchar] = 1;
		    }
		    else if (slowpoke <= FASTPOKE)
		    {
			randomfactor[correctchar] -= (RANDOMINCBETTER / 2);
			if (randomfactor[correctchar] < 1)
			    randomfactor[correctchar] = 1;
		    }
		}
	    }

	}
	testlength -= testinc;
	behindness -= testinc;
	yourlength -= yourinc;
    }

/*
 * If there are some extra white space characters in the input queue
 * it's OK, we'll get to them next time or we'll clean them out at the
 * end.
 */
    return errorcount;
}


/*----------------------------------------*/

static void
tone (float hertz, float duration, float amplitude)
{
    Beep ((int) (duration * 1000), (int) (amplitude * 100), (int) hertz);
}


static void
toneflush (void)
{
    BeepWait ();
}


/*----------------------------------------*/

#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
struct termios oldtermgtty;
struct termios termgtty;
static char    *terminal = "/dev/tty";
static int      termfd;
static int      termopen = 0;
static int      oldflgs, newflgs;

static void
openterminal (void)
{
    /* get parameters and open terminal */

    termfd = open (terminal, O_RDWR | O_NDELAY, 0);
    tcgetattr(termfd, &termgtty);
    oldtermgtty = termgtty;
    if (typeaway != LETMESEE)
	termgtty.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    termgtty.c_lflag &= ~ICANON;
    termgtty.c_cc[VMIN] = 1;
    termgtty.c_cc[VTIME] = 0;
    tcsetattr(termfd, TCSADRAIN, &termgtty);
    termopen = 1;
}

static int
readterminal (unsigned char **string)
{
	/* This must be declared static! */
	static char     line[TESTBUFSZ];
	int             n;

    n = read (termfd, line, sizeof (line) - 1);

    if (n > 0)
    {
		line[n] = '\0';
		*string = line;
    }
    else
	*string = NULL;

    return n;
}

static void
closeterminal (void)
{
    tcsetattr(termfd, TCSADRAIN, &oldtermgtty);
    close (termfd);
}

static void
die (void)
{
    cleanup ();
    exit (1);
}

static void
cleanup (void)
{
    if (termopen)
	closeterminal ();
    BeepCleanup ();
}

static void
suspend (int sig)
{
    struct sigaction handler;

    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    handler.sa_handler = suspend;
    sigaction (SIGTSTP, &handler, NULL);
    cleanup ();
    kill (getpid (), SIGSTOP);
    if (termopen)
	openterminal ();
    BeepResume ();
}


/*----------------------------------------*/

static int
randomletter (void)
{
int             ii;
int             sum, sum2;
long            ranspot;
static int      lasttime = -1;
static long     norepeat;

/*
 * This keeps the not-so-random random number generator from ignoring
 * certain characters forever!
 */
    norepeat = ((long) time (NULL) / 31) % 17291;

/*
 * All the usable letters get one unit riper.
 */
    for (ii = 0; ii < TWOFIFTYSIX; ii++)
    {
	if (randomfactor[ii] > 0)
	{
#ifdef DEBUGG
	    fprintf (stderr, "%c: %d %d\n",
		     (char) ii, randomfactor[ii], randomripe[ii]);
#endif
	    randomripe[ii]++;
	}
    }

    sum = 0;
    for (ii = 0; ii < TWOFIFTYSIX; ii++)
	sum += (randomfactor[ii] + (int) (randomripe[ii] / RIPECOUNT));

/*
 * The low bits of random aren't very random, I don't care WHAT
 * the manual claims.
 */
    do
    {
	ranspot = ((lrand48() >> 4) % sum + norepeat) % sum;

	sum2 = 0;
	for (ii = 0; ii < TWOFIFTYSIX - 1; ii++)
	{
	    sum2 += (randomfactor[ii] + (int) (randomripe[ii] / RIPECOUNT));

	    if (sum2 > ranspot)
		break;
	}
	/* Do it again if you got the same as last time! */
    } while (ii == lasttime);

    /* This one is FRESH again. */
    randomripe[ii] = 0;
    /* Remember for next time. */
    lasttime = ii;

    return ii;
}

static int
rancomp (const void *elem1, const void *elem2)
{
    register int *e1 = (int *)elem1;
    register int *e2 = (int *)elem2;
    float           a, b;


    a = (randomfactor[(*e1)] + (randomripe[(*e1)] / (float) RIPECOUNT));
    b = (randomfactor[(*e2)] + (randomripe[(*e2)] / (float) RIPECOUNT));

    if (a == b)
	return 0;
    else if (a > b)
	return -1;
    else
	return 1;
}

static void
report (void)
{
    int randomstr[TWOFIFTYSIX];

    printf ("\nCurrent words per minute: %.1f\n", words_per_minute);

    printf ("Total hits %d, misses %d", totalhitcount, totalmisscount);
    if (totalmisscount > 0)
	printf (", hit per miss ratio %.1f\n", (float) totalhitcount / (float) totalmisscount);
    else
	printf ("\n");

    if (randomletters)
    {
	int ii, jj, count;
	float sum; 

	printf ("Most to least frequent choices:\n");
	count = 0;
	sum = 0.;
	for (ii = 0; ii < TWOFIFTYSIX; ii++)
	{
	    if (randomfactor[ii] > 0)
	    {
		sum += (randomfactor[ii] + (randomripe[ii] / (float) RIPECOUNT));
		randomstr[count] = ii;
		count++;
	    }
	}

	qsort ((char *) randomstr, count, sizeof (randomstr[0]), rancomp);

	for (ii = 0; ii < count; ii++)
	{
/*
 * Insert a space for each jump across an integer.
 * The normalization (count/sum) ensures that if all
 * letters were equally probable, they would all have value 1.
 * Since they are not generally equally probable, then 1 is just the average.
 * Thus the rightmost space in the printout marks where the average is.
 * Further left spaces separate off blocks of letters that are approximately
 * twice as probable as the average, three times, etc.
 */
	    if (ii > 0)
	    {
		for (jj = 0; jj <
		     (int) (
			    (randomfactor[randomstr[ii - 1]] + (randomripe[randomstr[ii - 1]] / (float) RIPECOUNT))
			    * count / sum) -
		     (int) (
			    (randomfactor[randomstr[ii]] + (randomripe[randomstr[ii]] / (float) RIPECOUNT))
			    * count / sum);
		     jj++)
		    printf (" ");
	    }
	    utf8PrintChar( (unsigned char) randomstr[ii]);
	}
	printf ("\n");
    }

    /*
     * So you don't get penalized for being "slow" after this.
     */
    if (charbychar)
	slowpoke = SLOWPOKEMAX + 1;

    fflush (stdout);
}
