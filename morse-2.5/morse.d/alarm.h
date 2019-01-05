/* alarm.h -- wakeup call interface */

/*
-- Routines for using the system interval timer to time beeps.  Useful
-- for implementing the functions in beep.h on systems that don't provide
-- a more straightforward BeepWait() equivalent.
--
-- These routines use the ALRM signal.
*/


/*
-- Set the alarm for a time specified in ms.
*/
void AlarmSet(time_t);

/*
-- Wait for the alarm, or return immediately if the alarm isn't set.
*/
void AlarmWait(void);
