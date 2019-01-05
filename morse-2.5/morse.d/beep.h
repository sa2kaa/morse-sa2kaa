/* beep.h -- seligman 5/92 */

/*
-- Machine-dependent code for sounding a beep.
*/

#ifndef _BEEP_H
#define _BEEP_H


/*
-- Called exactly once, before any other function in this interface.
-- Returns nonzero on error.
*/
int BeepInit(void);


/*
-- Sound a beep for a time specified in ms.
-- The volume is in the range [0..100], and the pitch is in Hz.
--
-- May return immediately, after the sounding of the beep is completed,
-- or any time in between.  May be called while a previous beep is still
-- sounding, in which case the previous beep finishes before the new one
-- begins. Overall timing will be much better if this routine can return
-- during the sounding of the beep, especially if it's a "zero-volume beep",
-- meaning it's really just the timed pause of silence between tones.
-- ("morse.c" tries to do all its thinking during the pauses between beeps,
-- mostly in the longer ones between words.)
--
-- May use the ALRM signal for timing.
--
-- Returns nonzero on error.
*/
int Beep(int time, int volume, int pitch);


/*
-- Wait until any currently sounding beeps have completed.
-- Returns nonzero on error.
*/
int BeepWait(void);


/*
-- Clean up any altered state before exiting or suspending the program.
-- Returns nonzero on error.
*/
int BeepCleanup(void);


/*
-- Restore the world when program is resumed after having been suspended.
-- Returns nonzero on error.
*/
int BeepResume(void);


#endif /* _BEEP_H */
