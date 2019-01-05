/* beepALSA.c -- Neale Pickett 8/2007 */
/* A lot of code taken from the beepOSS.c, which said: */
/* * A lot of code taken from the beepSun.c */

/*
  Implementation of beep for ALSA.
*/

#include <alsa/asoundlib.h>
#include <math.h>

static char *device = "default";

#include "beep.h"

#define RATE	(44100)        /* 44.1K samples = 1 second  (CD quality) */
#define RAMP	(RATE * 5 / 1000)                  /* 5 millisecond ramp */
#define MAXTIME	(2 * RATE)                    /* 2 seconds max tone time */

static int err;
static snd_pcm_t *handle;
static snd_pcm_sframes_t frames;
static int channels = 1; /* 1=mono, 2=stereo */
static float silence[2*MAXTIME];
static float soundbuf[2*(MAXTIME - RAMP)];
static float ramp_down[2*RAMP];

int BeepInit (void)
{
  int i;

  /*
   * Initialize the sound of silence
   */
  for (i = 0; i < 2*MAXTIME; i++) {
    silence[i] = 0;
  }

  /*
   * Set up ALSA
   */
  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(err));
    return 1;                   /* Not OK */
  }
  if ((err = snd_pcm_set_params(handle,
                                SND_PCM_FORMAT_FLOAT,
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                channels,
                                RATE,
                                1,
                                500000)) < 0) {   /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    return 1;                   /* Not OK */
  }

  BeepWait();
  return 0;                     /* OK */
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
  if (pitch <= 0) {
    volume = 0;
  }

  if (volume <= 0) {
    snd_pcm_writei(handle, silence, channels*n);
  } else {
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

    if (cycle > MAXTIME / 2) {
      cycle = MAXTIME / 2;
    }

    /* round down length of (rampup + mesa top) to integral cycle */
    first_len = ((n - RAMP) / cycle) * cycle;
    if (first_len < cycle) {
      first_len = cycle;
    }
    if (first_len > MAXTIME - RAMP) {
      first_len = MAXTIME - RAMP;
    }

    /* round down length of (rampdown) to integral cycle */
    down_len = ((RAMP) / cycle) * cycle;
    if (down_len < cycle) {
	down_len = cycle;
    }
    if (down_len > RAMP) {
      down_len = RAMP;
    }

    /*
     * Can we just reuse what we had before?
     */
    if (pitch != last_pitch || n > last_n || volume != last_volume) {
      last_pitch = pitch;
      last_n = n;
      last_volume = volume;
      double          dt;
      int i;

      dt = 2. * M_PI / cycle;	/* sine scale factor */

      /* Ramp up; begin with silence */

      for (i = 0; i < RAMP; i++) {
        soundbuf[i*channels] = ((float)i / RAMP) * (volume/100.0) * sin(i * dt);
        if (channels == 2)
          soundbuf[i*channels+1] = soundbuf[i*channels];
      }

      /* Mesa top */
      for (i = RAMP; i < first_len; i++) {
        soundbuf[i*channels] = (volume/100.0) * sin(i * dt);
        if (channels == 2) {
          soundbuf[i*channels+1] = soundbuf[i*channels];
        }
      }

      /* Ramp down; end with silence */
      for (i = 0; i < down_len; i++) {
        ramp_down[i*channels] = (1. - ((float)i / RAMP)) * (volume/100.0) * sin(i * dt);
        if (channels == 2) {
          ramp_down[i*channels+1] = ramp_down[i*channels];
        }
      }
    }

    snd_pcm_writei(handle, soundbuf, channels*first_len);
    snd_pcm_writei(handle, ramp_down, channels*down_len);
  }

  return 0;
}

int BeepWait (void)
{
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
