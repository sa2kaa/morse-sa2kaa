/* beepOSS.c -- Jacek M. Holeczek 3/2000 */
/* A lot of code taken from the beepSun.c */

/*
  Implementation of beep for the Open Sound System /dev/dsp device.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <math.h>

#include "beep.h"

#define DEVICE_NAME "/dev/dsp"
#define RATE	(8000)	/* 8 K samples = 1 second */
#define RAMP	(RATE * 5 / 1000)	/* 5 millisecond ramp */
#define MAXTIME	(2 * RATE)	/* 2 seconds max tone time */

static int audio = 0;
static int channels = 0; /* 1=mono, 2=stereo */
static unsigned char silence[2*MAXTIME];
static unsigned char soundbuf[2*(MAXTIME - RAMP)];
static unsigned char ramp_down[2*RAMP];

int BeepInit (void)
{
  int i;
  
  /*
   * Initialize the sound of silence
   */
  for (i = 0; i < 2*MAXTIME; i++)
    silence[i] = 128;
  
  audio = open(DEVICE_NAME, O_WRONLY, 0);
  if (audio < 0)
    {
      return 1; /* not o.k. */
    } else {
      
      i = 0x00ff0008; /* 255 fragments, 256 bytes each */
      if (ioctl(audio, SNDCTL_DSP_SETFRAGMENT, &i))
	{
	  fprintf(stderr, "SNDCTL_DSP_SETFRAGMENT error : 0x%08x\n", i);
	  return 2;
	}
      
      channels = 1; /* 1 channel = mono */
      if (ioctl(audio, SNDCTL_DSP_CHANNELS, &channels)==-1)
	{
	  fprintf(stderr, "SNDCTL_DSP_CHANNELS error : %i\n", channels);
	  return 3;
	}
      if (channels != 1)
	{
	  channels = 2; /* 2 channels = stereo */
	  if (ioctl(audio, SNDCTL_DSP_CHANNELS, &channels)==-1)
	    {
	      fprintf(stderr, "SNDCTL_DSP_CHANNELS error : %i\n", channels);
	      return 4;
	    }
	}
      if (!(channels == 1 || channels == 2))
	{
	  fprintf(stderr, "Neither mono nor stereo mode could be set.\n");
	  return 5;
	}
      
      i = AFMT_U8; /* standard unsigned 8 bit audio encoding */
      if (ioctl(audio, SNDCTL_DSP_SETFMT, &i)==-1)
	{
	  fprintf(stderr, "SNDCTL_DSP_SETFMT error : %i\n", i);
	  return 6;
	}
      
      i = RATE; /* sampling rate RATE */
      if (ioctl(audio, SNDCTL_DSP_SPEED, &i)==-1)
	{
	  fprintf(stderr, "SNDCTL_DSP_SPEED error : %i\n", i);
	  return 7;
	}
      
      write (audio, silence, channels*2);
      BeepWait();
      return 0; /* o.k. */
    }
}

int Beep (int time, int volume, int pitch)
{
    int n;
  /*
   * Finagle the number of samples
   */
  n = (time / 1000.) * RATE;	/* Number samples in tone time */
  n = n < MAXTIME ? n : MAXTIME;	/* clip to buffer size */
  n = n < 2 * RAMP ? 2 * RAMP : n;	/* leave room for ramps */
  
  /*
   * Catch stupidity
   */
  if (pitch <= 0)
    volume = 0;
  
  if (volume <= 0)
    {
      write (audio, silence, channels*n);
    }
  else
    {
      static int      last_pitch = -1, last_n = -1, last_volume = -1;
      int             first_len, down_len, cycle;
      /*
       * clip to Nyquist
       * (Of course this means that if you ask for too high a frequency you
       * just get silence, since you sample all the zero-crossings of the
       * sine wave.)
       */
      pitch = pitch < RATE / 2 ? pitch : RATE / 2;
      cycle = ((RATE + 1.e-6) / pitch); /* samples per cycle */
      
      if (cycle > MAXTIME / 2)
	cycle = MAXTIME / 2;
      
      /* round down length of (rampup + mesa top) to integral cycle */
      first_len = ((n - RAMP) / cycle) * cycle;
      if (first_len < cycle)
	first_len = cycle;
      if (first_len > MAXTIME - RAMP)
	first_len = MAXTIME - RAMP;
      
      /* round down length of (rampdown) to integral cycle */
      down_len = ((RAMP) / cycle) * cycle;
      if (down_len < cycle)
	down_len = cycle;
      if (down_len > RAMP)
	down_len = RAMP;
      
      /*
       * Can we just reuse what we had before?
       */
      if (pitch != last_pitch || n > last_n || volume != last_volume)
	{
	  last_pitch = pitch;
	  last_n = n;
	  last_volume = volume;
	  int i;
	  double          dt;
 
	  dt = 2. * M_PI / cycle;	/* sine scale factor */
	  
	  /* Ramp up; begin with silence */
	  
	  for (i = 0; i < RAMP; i++)
	    {
	      soundbuf[i*channels] = rint( 128. +
					   ((float) i / RAMP) * (volume * 1.26) * sin (i * dt)
					   );
	      if (channels == 2)
		soundbuf[i*channels+1] = soundbuf[i*channels];
	    }
	  
	  /* Mesa top */
	  for (i = RAMP; i < first_len; i++)
	    {
	      soundbuf[i*channels] = rint( 128. +
					   1. * (volume * 1.26) * sin (i * dt)
					   );
	      if (channels == 2)
		soundbuf[i*channels+1] = soundbuf[i*channels];
	    }
	  
	  /* Ramp down; end with silence */
	  for (i = 0; i < down_len; i++)
	    {
	      ramp_down[i*channels] = rint( 128. +
					    (1. - (float) (i + 1) / down_len) * (volume * 1.26) * sin (i * dt)
					    );
	      if (channels == 2)
		ramp_down[i*channels+1] = ramp_down[i*channels];
	    }
	}
      
      write (audio, soundbuf, channels*first_len);
      write (audio, ramp_down, channels*down_len);
    }
  
  return 0;
}

int BeepWait (void)
{
  ioctl (audio, SNDCTL_DSP_SYNC, 0);
  return 0;
}

int BeepCleanup (void)
{
  return 0;
}

int BeepResume (void)
{
  return 0;
}
