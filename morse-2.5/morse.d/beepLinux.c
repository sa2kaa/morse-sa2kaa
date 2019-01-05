/*
 * From: jmorriso@bogomips.ee.ubc.ca (John Paul Morrison)
 * Subject: beepLinux.c
 * To: joe@montebello.soest.hawaii.edu
 * Date: Fri, 18 Nov 1994 15:32:33 -0800 (PST)
 * X-Linux: watch for it: Linux 94 aka "Helsinki"
 * 
 * beepLinux.c for morse (lightly tested, seems to work fine)
 */

/*
 * beep for the Linux console (PC Speaker) 
 *
 * (beepSun.c might work if you have a sound card installed and /dev/audio)
 *
 * by John Paul Morrison <jmorriso@ve7jpm.ampr.org>
 *
 */

/* beepLinux.c -- 11/94 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/kd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "alarm.h"
#include "beep.h"


static int fd;

int BeepInit()
{
  if ((fd = open("/dev/console",O_WRONLY)) == -1)
    {
      fprintf(stderr,"You have no permissions to use /dev/console (chmod a+w).\n");
      return 1;
    }
  return 0;
}

int Beep(time, volume, pitch)
int time, volume, pitch;
{
    int count;

    AlarmWait();
    if (volume == 0)
	count = 0;
    else
	count = (1193180 / pitch) & 0xffff;

    ioctl(fd, KDMKTONE, (time << 16) | count);
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
    close(fd);
    return 0;
}

int BeepResume()
{
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * BogoMIPS Research Labs  --  bogosity research & simulation  --  VE7JPM  -- 
 * jmorriso@bogomips.ee.ubc.ca ve7jpm@ve7jpm.ampr.org jmorriso@ve7ubc.ampr.org
 * ---------------------------------------------------------------------------
 */
