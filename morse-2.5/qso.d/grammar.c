#include <stdio.h>

int
is_vowel (first_char)
     char first_char;
{
    if (
	   first_char == 'A' ||
	   first_char == 'E' ||
	   first_char == 'I' ||
	   first_char == 'O' ||
	   first_char == 'U' ||
	   first_char == 'a' ||
	   first_char == 'e' ||
	   first_char == 'i' ||
	   first_char == 'o' ||
	   first_char == 'u'
     )
	return (1);
    return (0);
}

char buffer[200];

char *
A_Or_An (string)
     char *string;
{
    if (is_vowel (string[0]) == 1)
	sprintf (buffer, "an %s", string);
    else
	sprintf (buffer, "a %s", string);
    return ((char *) buffer);
}
