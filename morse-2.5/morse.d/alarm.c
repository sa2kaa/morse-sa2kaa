/* alarm.c -- wakeup calls */

/*
-- Implementation of alarm.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

static int alarmPending = 0;  /* Nonzero when the alarm is set. */

static void ualarm();
static void AlarmHandler();

void AlarmSet(time)
    time_t time;
{
    struct sigaction handler;

    alarmPending = 1;
    handler.sa_handler = AlarmHandler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    sigaction(SIGALRM, &handler, NULL);
    ualarm(1000 * time, 0);
}

/*
-- If an alarm signal is lurking (due to a prior call to SetAlarm), then
-- pause until it arrives.  This procedure could have simply been written:
--   if (alarmPending) pause();
-- but that allows a potential race condition.
*/
void AlarmWait()
{
    sighold(SIGALRM);
    if (alarmPending)
      sigpause(SIGALRM);
    sigrelse(SIGALRM);
}


static void ualarm(us)
    unsigned us;
{
    struct itimerval rttimer, old_rttimer;

    rttimer.it_value.tv_sec  = us / 1000000;
    rttimer.it_value.tv_usec = us % 1000000;
    rttimer.it_interval.tv_sec  = 0;
    rttimer.it_interval.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &rttimer, &old_rttimer)) {
	perror("ualarm");
	exit(1);
    }
}


static void AlarmHandler()
{
    alarmPending = 0;
}
