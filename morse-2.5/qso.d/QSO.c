/*
 * Copyright (c) 1991 Paul J. Drongowski.
 * Copyright (c) 1992 Joe Dellinger.
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
 * Return-Path: <pjd@cadillac.siemens.com>
 * Received: from cadillac.siemens.com by montebello.soest.hawaii.edu (4.1/montebello-MX-1.9)
 *      id AA01487; Mon, 10 Aug 92 03:21:41 HST
 * Received: from kevin.siemens.com by cadillac.siemens.com (4.1/SMI-4.0)
 *      id AA25847; Mon, 10 Aug 92 09:21:37 EDT
 * Date: Mon, 10 Aug 92 09:21:37 EDT
 * From: pjd@cadillac.siemens.com (paul j. drongowski)
 * Message-Id: <9208101321.AA25847@cadillac.siemens.com>
 * To: joe@montebello.soest.hawaii.edu
 * Status: RO
 */

/*
 * This copy is slightly hacked by Joe Dellinger, August 1992
 * And some more... November 1992
 *
 * Partially re-written by "murray@vs6.scri.fsu.edu"
 * to use better grammar and to avoid some nonsensical
 * responses (like claiming to be a person who's been licensed
 * longer than they've been alive!), Jan 1994
 *
 * Those improvements merged with others by Joe Dellinger, Oct 1994
 */

/*
 * Generate QSO
 */

/*
 * When run, this program generates a single QSO. The form of the
 * QSO is similar to QSO's one would expect to hear at a code test.
 * It begins with a series of V's (commented out in this version),
 * callsigns of the receiver and
 * sender, followed by a few sentences about weather, name,
 * occupation, etc. The QSO ends with the callsigns of the receiver
 * and sender.
 *
 * All output is produced using "printf." This should make the
 * program easy to port. Output can be piped into another program
 * such as sparc-morse on the Sun or it can be redirected into
 * a file (without viewing the contents of course!)
 *
 * The program design is similar to a "random poetry generator"
 * or "mad-libs." Each QSO form is generated by its own C function,
 * such as "PutForm1." Each function calls other C functions to
 * produce the sentences in the QSO. The sentence forms are
 * selected somewhat randomly as well as any blanks to be filled.
 * Words and phrases are selected from several lists such as
 * "Transceiver," "Antenna," "Job," etc. Sometimes this scheme is
 * clever as in the formation of city names. Sometimes it is
 * stupidly simple-minded and grammatical agreement is lost.
 * By the way, the callsigns are real and were picked from
 * rec.radio.amateur.misc on USENET.
 *
 * The program was constructed in C for Sun workstations. It uses
 * the library function "drand48" in function "Roll" to produce
 * pseudo-random numbers. The library function "srand48" and "time"
 * in "main" are used to set the pseudo-random number seed.
 *
 * Known problems and caveats? Hey, it`s software! All Morse
 * training programs handle the procedural signs (e.g., AR, SK)
 * differently. The function "PutQSO" currently prints "+ %"
 * for the AR and SK at the end of the QSO. These may be ignored,
 * mapped into something else, or just plain cause your training
 * program to roll over and play dead. I don`t know. This is a
 * cheap hack.
 *
 * And speaking of cheap... The program will not generate all
 * characters and pro-signs that are found on an "official" code
 * test. This program is for practice only and should be supplemented
 * with lots of random code.
 *
 * Always have fun!
 */


#include <sys/types.h>
#include <stdio.h>
#include <time.h>

char *A_Or_An (char *);
char *Choose (char *Words[], int Number);
int License_Seed (void);

int their_age;			/* PERSON'S AGE, SO THEY AREN'T LICENSED MORE
                                 * THAN THEIR AGE */

extern char *Transceiver[];
extern char *Antenna[];
extern char *UpFeet[];
extern char *Weather1[];
extern char *Weather2[];
extern char *Power[];
extern char *Job[];
extern char *Name[];
extern char *CallSign[];
extern char *License[];
extern char *City[];
extern char *NewCity[];
extern char *CityHeights[];
extern char *New[];
extern char *Heights[];
extern char *State[];
extern char *Frqmisc[];
extern char *Callmisc[];
extern char *NumMisc[];
extern char *Miscellaneous[];
extern char *RST[];

int NXCVR;
int NANTENNA;
int NUPFEET;
int NWX1;
int NWX2;
int NPOWER;
int NJOB;
int NNAME;
int NCALLSIGN;
char *Sender, *Receiver;
int NLICENSE;
int NCITY;
int NNEWCITY;
int NCITYHTS;
int NNEW;
int NHEIGHTS;
int NSTATE;
int NFRQMISC;
int NFRQCALLMISC;

char *FrqCallmisc[] =
{
    "QRW %s %d",
    "QRZ %s %d",
    "QSN %s %d",
    "QSX %s %d",
    "QRW %s %d?",
    "QRZ %s %d?",
    "QSN %s %d?",
    "QSX %s %d?",
    0
};

int NCALLMISC;
int NNUMMISC;
int NMISC;
int NRST;


/*
 *************************************
 * Routines to put sentences/clauses *
 *************************************
  */

void
PutMisc (void)
{
    printf ("%s\n", Choose (Miscellaneous, NMISC));
}

void
PutThanks (void)
{
    switch (Roll (6))
      {
      case 2:
	  printf ("Thanks for your call.\n");
	  break;

      case 3:
	  printf ("Tnx for ur call.\n");
	  break;

      case 4:
	  printf ("Tnx for the call.\n");
	  break;

      case 5:
	  printf ("Thanks for the call.\n");
	  break;

      default:
	  printf ("Thanks %s for the call.\n", Choose (Name, NNAME));
	  break;
      }
}

void
PutName (void)
{
    switch (Roll (6))
      {
      case 2:
	  printf ("Name is %s.\n", Choose (Name, NNAME));
	  break;

      case 4:
	  printf ("This is %s.\n", Choose (Name, NNAME));
	  break;

      case 5:
	  printf ("%s here.\n", Choose (Name, NNAME));
	  break;

      default:
	  printf ("My name is %s.\n", Choose (Name, NNAME));
	  break;
      }
}

void
PutJob (void)
{
    switch (Roll (20))
      {
      case 2:
      case 3:
	  printf ("My occupation is %s.\n", Choose (Job, NJOB));
	  break;

      case 4:
      case 5:
	  printf ("I work as %s.\n", A_Or_An (Choose (Job, NJOB)));
	  break;

      case 6:
	  printf ("I was %s, now unemployed.\n", A_Or_An (Choose (Job, NJOB)));
	  break;

      case 11:
      case 12:
	  printf ("Occupation is %s.\n", Choose (Job, NJOB));
	  break;

      default:
	  printf ("I am %s.\n", A_Or_An (Choose (Job, NJOB)));
	  break;
      }
}

void
PutAge (void)
{

    their_age = Roll (60) + 16;

    switch (Roll (5))
      {
      case 3:
	  printf ("My age is %d.\n", their_age);
	  break;

      case 4:
	  printf ("I am %d years old.\n", their_age);
	  break;

      default:
	  printf ("Age is %d.\n", their_age);
	  break;
      }
}

void
PutLicense (void)
{
int get_years_licence;

    get_years_licence = Roll (License_Seed ());
    if (get_years_licence < 2)
	get_years_licence = 2;

    switch (Roll (12))
      {
      case 1:
	  printf ("I have %s class licence.\n",
		  A_Or_An (Choose (License, NLICENSE)));
	  break;

      case 2:
	  printf ("I am %s license ham.\n",
		  A_Or_An (Choose (License, NLICENSE)));
	  break;

      case 3:
	  printf ("I am %s licence ham.\n",
		  A_Or_An (Choose (License, NLICENSE)));
	  break;

      case 4:
	  printf ("I have been licenced %d years as %s class.\n",
		  get_years_licence, Choose (License, NLICENSE));
	  break;

      case 5:
	  printf ("I have %s class license.\n",
		  A_Or_An (Choose (License, NLICENSE)));
	  break;

      case 6:
	  printf ("I am %s class ham.\n",
		  A_Or_An (Choose (License, NLICENSE)));
	  break;

      case 7:
	  printf ("I have been licensed %d years as %s class.\n",
		  get_years_licence, Choose (License, NLICENSE));
	  break;

      default:
	  printf ("I have been %s class ham for %d years.\n",
		  A_Or_An (Choose (License, NLICENSE)), get_years_licence);
	  break;
      }
}

void
PutTemperature (void)
{
    printf ("Temperature is %d.\n", Roll (80) + 10);
}

void
PutWeather1 (void)
{
    switch (Roll (17))
      {
      case 2:
	  printf ("WX is %s.\n", Choose (Weather1, NWX1));
	  PutTemperature ();
	  break;

      case 3:
	  printf ("Weather here is %s.\n", Choose (Weather1, NWX1));
	  break;

      case 4:
	  printf ("Weather is %s.\n", Choose (Weather1, NWX1));
	  break;

      case 5:
	  printf ("WX is %s.\n", Choose (Weather1, NWX1));
	  break;

      case 6:
	  PutTemperature ();
	  printf ("Weather here is %s.\n", Choose (Weather1, NWX1));
	  break;

      case 7:
	  PutTemperature ();
	  printf ("Weather is %s.\n", Choose (Weather1, NWX1));
	  break;

      case 8:
	  PutTemperature ();
	  printf ("WX is %s.\n", Choose (Weather1, NWX1));
	  break;

      case 9:
	  printf ("Weather here is %s and temperature is %d.\n",
		  Choose (Weather1, NWX1), Roll (80) + 10);
	  break;

      case 10:
	  printf ("Weather is %s, temperature %d.\n",
		  Choose (Weather1, NWX1), Roll (80) + 10);
	  break;

      case 11:
	  printf ("WX is %d degrees and %s.\n",
		  Roll (80) + 10, Choose (Weather1, NWX1));
	  break;

      case 12:
	  printf ("The WX is %s and the temp is %d degrees.\n",
		  Choose (Weather1, NWX1), Roll (80) + 10);
	  break;

      case 14:
	  printf ("Weather is %s.\n", Choose (Weather1, NWX1));
	  PutTemperature ();
	  break;

      case 15:
	  printf ("Weather here is %s.\n", Choose (Weather1, NWX1));
	  PutTemperature ();
	  break;

      default:
	  printf ("WX is %s and %d degrees.\n",
		  Choose (Weather1, NWX1), Roll (80) + 10);
      }
}

void
PutWeather2 (void)
{

    switch (Roll (10))
      {
      case 0:
	  printf ("It is %s.\n", Choose (Weather2, NWX2));
	  break;

      case 1:
	  printf ("It is %s and %d degrees.\n",
		  Choose (Weather2, NWX2), Roll (80) + 10);
	  break;

      case 2:
	  printf ("The WX is %s and the temp is %d degrees.\n",
		  Choose (Weather2, NWX2), Roll (80) + 10);
	  break;

      case 3:
	  printf ("WX is %s and the temp is %d degrees.\n",
		  Choose (Weather2, NWX2), Roll (80) + 10);
	  break;

      case 4:
	  printf ("It is %s today.\n", Choose (Weather2, NWX2));
	  break;

      case 5:
	  printf ("It is %s and %d degrees.\n",
		  Choose (Weather2, NWX2), Roll (100) + 3);
	  break;

      case 6:
	  printf ("The WX is %s and the temp is %d degrees.\n",
		  Choose (Weather2, NWX2), Roll (90) + 10);
	  break;

      case 7:
	  printf ("WX is %s and the temp is %d degrees.\n",
		  Choose (Weather2, NWX2), Roll (80) + 10);
	  break;

      default:
	  printf ("It is %s here.\n", Choose (Weather2, NWX2));
	  break;
      }
}

void
PutWeather (void)
{
    switch (Roll (4))
      {
      case 3:
	  PutWeather1 ();
	  break;

      default:
	  PutWeather2 ();
	  break;
      }
}

void
PutCityState (void)
{
    switch (Roll (6))
      {
      case 4:
	  printf ("%s %s, ",
		Choose (CityHeights, NCITYHTS), Choose (Heights, NHEIGHTS));
	  break;

      case 5:
	  printf ("%s %s, ", Choose (New, NNEW), Choose (NewCity, NNEWCITY));
	  break;

      default:
	  printf ("%s, ", Choose (City, NCITY));
	  break;
      }
    printf ("%s.\n", Choose (State, NSTATE));
}

void
PutLocation (void)
{

    switch (Roll (5))
      {
      case 3:
	  printf ("My QTH is ");
	  break;

      case 4:
	  printf ("My location is ");
	  break;

      default:
	  printf ("QTH is ");
	  break;

      }
    PutCityState ();
}

void
PutRig (void)

{
    switch (Roll (19))
      {
      case 0:
      case 1:
	  printf ("My rig runs %s watts into %s up %s feet.\n",
	       Choose (Power, NPOWER), A_Or_An (Choose (Antenna, NANTENNA)),
		  Choose (UpFeet, NUPFEET));
	  break;

      case 2:
      case 3:
	  printf ("Rig is a %s watt %s and antenna is %s.\n",
		  Choose (Power, NPOWER), Choose (Transceiver, NXCVR),
		  A_Or_An (Choose (Antenna, NANTENNA)));
	  break;

      case 4:
      case 5:
	  printf ("My transceiver is %s.\n", A_Or_An (Choose (Transceiver, NXCVR)));
	  printf ("It runs %s watts into %s.\n",
	      Choose (Power, NPOWER), A_Or_An (Choose (Antenna, NANTENNA)));
	  break;

      case 6:
      case 7:
	  printf ("The rig is %s running %s watts.\n",
	     A_Or_An (Choose (Transceiver, NXCVR)), Choose (Power, NPOWER));
	  printf ("Antenna is %s up %s m.\n",
	    A_Or_An (Choose (Antenna, NANTENNA)), Choose (UpFeet, NUPFEET));
	  break;

      case 8:
      case 9:
      case 10:
      case 11:
	  printf ("My rig runs %s watts into %s up %s meters.\n",
	       Choose (Power, NPOWER), A_Or_An (Choose (Antenna, NANTENNA)),
		  Choose (UpFeet, NUPFEET));
	  break;

      case 12:
	  printf ("My rig runs %s watts into %s up %s feet, but\nthe antenna has partly fallen.\n",
	       Choose (Power, NPOWER), A_Or_An (Choose (Antenna, NANTENNA)),
		  Choose (UpFeet, NUPFEET));
	  break;

      case 13:
	  printf ("Rig is %s running %s watts into %s up %s ft.\n",
	      A_Or_An (Choose (Transceiver, NXCVR)), Choose (Power, NPOWER),
	    A_Or_An (Choose (Antenna, NANTENNA)), Choose (UpFeet, NUPFEET));
	  break;

      case 14:
	  printf ("My rig runs %s watts into %s up %s feet.\n",
	       Choose (Power, NPOWER), A_Or_An (Choose (Antenna, NANTENNA)),
		  Choose (UpFeet, NUPFEET));
	  break;

      case 15:
	  printf ("Rig is %s watt %s and antenna is %s.\n",
		  A_Or_An (Choose (Power, NPOWER)),
		  Choose (Transceiver, NXCVR),
		  Choose (Antenna, NANTENNA));
	  break;

      case 16:
	  printf ("My transceiver is %s.\n",
		  A_Or_An (Choose (Transceiver, NXCVR)));
	  printf ("It runs %s watts into %s.\n",
		  Choose (Power, NPOWER),
		  A_Or_An (Choose (Antenna, NANTENNA)));
	  break;

      case 17:
	  printf ("The rig is %s running %s watts.\n",
		  A_Or_An (Choose (Transceiver, NXCVR)),
		  Choose (Power, NPOWER));
	  printf ("Antenna is %s up %s feet.\n",
		  A_Or_An (Choose (Antenna, NANTENNA)),
		  Choose (UpFeet, NUPFEET));
	  break;

      default:
	  printf ("Rig is %s ",
		  A_Or_An (Choose (Transceiver, NXCVR)));
	  printf ("running %s watts into %s up %s feet.\n",
		  Choose (Power, NPOWER),
		  A_Or_An (Choose (Antenna, NANTENNA)),
		  Choose (UpFeet, NUPFEET));
	  break;
      }
}

void
PutRST (void)
{
register char *TheRST;

    TheRST = Choose (RST, NRST);

    switch (Roll (8))
      {
      case 0:
	  printf ("UR RST %s=%s.\n", TheRST, TheRST);
	  break;

      case 1:
	  printf ("RST is %s=%s.\n", TheRST, TheRST);
	  break;

      case 2:
	  printf ("RST %s=%s.\n", TheRST, TheRST);
	  break;

      case 3:
	  printf ("Your RST %s=%s.\n", TheRST, TheRST);
	  break;

      case 4:
	  printf ("Your RST is %s=%s.\n", TheRST, TheRST);
	  break;

      case 5:
	  printf ("Your signal is RST %s/%s.\n", TheRST, TheRST);
	  break;

      case 6:
	  printf ("UR signal is RST %s,%s.\n", TheRST, TheRST);
	  break;

      default:
	  printf ("Your RST is %s/%s.\n", TheRST, TheRST);
	  break;
      }
}


void
PutQ_And_Freq (void)
{
    switch (Roll (8))
      {
      case 2:
	  printf (Frqmisc[Roll (NFRQMISC)], make_freq ());
	  printf ("\n");
	  break;

      case 3:
	  printf (Callmisc[Roll (NFRQMISC)],
		  Choose (CallSign, NCALLSIGN));
	  printf ("\n");
	  break;

      case 4:
	  printf (FrqCallmisc[Roll (NFRQCALLMISC)],
		  Choose (CallSign, NCALLSIGN),
		  make_freq ());
	  printf ("\n");
	  break;

      case 5:
	  printf (NumMisc[Roll (NNUMMISC)],
		  Roll (3) + Roll (2) + 1);
	  printf ("\n");
	  break;

      default:
	  break;

      }
}

void
PutFirstCallsign (void)
{
    Sender = Choose (CallSign, NCALLSIGN);
    Receiver = Choose (CallSign, NCALLSIGN);
    printf ("%s de %s\n", Receiver, Sender);
}

void
PutLastCallsign (void)
{
    printf ("%s de %s\n", Receiver, Sender);
}

main (int argc, char **argv)
{

    NXCVR = CountStrings (Transceiver);
    NANTENNA = CountStrings (Antenna);
    NUPFEET = CountStrings (UpFeet);
    NPOWER = CountStrings (Power);
    NRST = CountStrings (RST);
    NWX1 = CountStrings (Weather1);
    NWX2 = CountStrings (Weather2);
    NJOB = CountStrings (Job);
    NNAME = CountStrings (Name);
    NSTATE = CountStrings (State);
    NCITY = CountStrings (City);
    NCITYHTS = CountStrings (CityHeights);
    NNEW = CountStrings (New);
    NHEIGHTS = CountStrings (Heights);
    NNEWCITY = CountStrings (NewCity);
    NLICENSE = CountStrings (License);
    NMISC = CountStrings (Miscellaneous);
    NCALLSIGN = CountStrings (CallSign);
    NFRQMISC = CountStrings (Frqmisc);
    NFRQCALLMISC = CountStrings (FrqCallmisc);
    NCALLMISC = CountStrings (Callmisc);
    NNUMMISC = CountStrings (NumMisc);

    srand48 ((long) time (0));

    PutQSO ();

    return 0;
}