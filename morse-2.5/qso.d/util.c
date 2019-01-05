/*
 ********************
 * Utility routines *
 ********************
 */
extern int their_age;

int
CountStrings (char *StringVector[])
     /*
      * Count the number of string values in the supplied vector
      * of pointers. Start with the first pointer and stop when
      * NIL (0) is encountered.
      */
{
register char **SV;
register int Count;

    Count = 0;

    for (SV = StringVector; *SV; SV++)
      {
	  Count++;
      }
    return (Count);
}

int
Roll (int Number)
{
int new_tmp;
double tmp_val;
double drand48 ();
    tmp_val = ((int) (drand48 () * (Number /*-1*/ )));
    new_tmp = (int) tmp_val % 32767;
    if (new_tmp < 2)
	tmp_val = 2;
    return ((int) new_tmp);
}

int
License_Seed (void)
{
    if (their_age > 20)
	return (20);
    if (their_age < 10)
	return (10);
    return (their_age - 8);
}

char *
Choose (char *Words[], int Number)
{
    return (Words[Roll (Number)]);
}
