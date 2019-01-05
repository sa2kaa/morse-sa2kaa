/* beepX11.c -- seligman 5/92 */

/*
-- Implementation of beep.h for X11.
--
-- Compile with the library "-lX11".
*/

#include <stdio.h>
#include <X11/Xlib.h>
#include "alarm.h"
#include "beep.h"

static Display *dpy = 0;
static XKeyboardControl initialState;

#define BellFlags (KBBellPercent | KBBellPitch | KBBellDuration)


int BeepInit()
{
    XKeyboardState state;

    if (! (dpy = XOpenDisplay(0))) {
	perror("Couldn't open display");
	return 1;
    }

    /* Save initial state so it can be restored later. */
    XGetKeyboardControl(dpy, &state);
    initialState.bell_duration = state.bell_duration;
    initialState.bell_percent  = state.bell_percent;
    initialState.bell_pitch    = state.bell_pitch;

    return 0;
}


int Beep(time, volume, pitch)
    int time, volume, pitch;
{
    XKeyboardControl values;

    AlarmWait();

    if (volume != 0  &&  pitch != 0) {
	values.bell_duration = time;
	values.bell_percent  = 100;
	values.bell_pitch    = pitch;

	XChangeKeyboardControl(dpy, BellFlags, &values);
	XBell(dpy, volume - 100);
	XFlush(dpy);
    }

    AlarmSet(time);
    return 0;
}


int BeepWait()
{
    AlarmWait();
    return 0;
}


int BeepCleanup()
{
    if (dpy != 0) {
	XChangeKeyboardControl(dpy, BellFlags, &initialState);
	XFlush(dpy);
    }
    return 0;
}


int BeepResume()
{
    return 0;
}
