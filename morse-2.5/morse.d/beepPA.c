/*
 * beep for pulseaudio
 *
 * by Thomas Horsten <thomas@horsten.com>
 *
 */

/* beepPA.c -- 10/2010 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <pthread.h>
#include <math.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include "beep.h"

#if 0
#define dprintf(args...) printf(args)
#else
#define dprintf(args...)
#endif

static pthread_t beep_thread;
static pthread_mutex_t beep_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t beep_cv = PTHREAD_COND_INITIALIZER;

static const pa_sample_spec sample_format = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 1
};

static pa_simple *snddev;

static struct {
    int time;
    int volume;
    int pitch;
} beep_info = {0,0,0};

static void play_tone()
{
    int len = (int)(sample_format.rate*beep_info.time/1000.0);
    int16_t *sample;

    sample = malloc(len*sizeof(sample[0]));
    if (!sample)
	return;
    if (beep_info.volume == 0) {
	memset(sample, 0, len*sizeof(sample[0]));
    } else {
	double t, tt;
	int c;
	double v;
	tt = beep_info.time/1000.0;
	for (c=0; c<len; c+=2) {
	    t = c/((double)sample_format.rate); /* Time in s from start */
	    v = (beep_info.volume/100.0) * sin(M_PI * 2 * t * beep_info.pitch);
	    if (t < 0.01) {
		/* Fade in softly over the first 10 ms */
		v = v * (t/0.01);
	    }
	    if ((tt-t) < 0.005) {
		/* Fade out softly over last 5 ms (avoids "click") */
		v = v * ((tt-t)/0.005);
	    }
	    sample[c] = sample[c+1] = (int)(v * 32768.0);
	}
    }
    dprintf("playing %d hz tone vol %d for %d (%f) ms\n", beep_info.pitch, beep_info.volume, beep_info.time, tt);
    if (pa_simple_write(snddev, (uint8_t *)sample, len*sizeof(int16_t), NULL) < 0) {
	fprintf(stderr, "pa_simple_Write failed\n");
	exit(1);
    }
    dprintf("play done\n");
    free(sample);
}

static void *beep_threadfunc(void *opaque)
{
    pthread_cond_signal(&beep_cv);
    while (1) {
	dprintf("waiting in audio thread\n");
	pthread_mutex_lock(&beep_mutex);
	if (!beep_info.time)
	    pthread_cond_wait(&beep_cv, &beep_mutex);
	dprintf("woke\n");
	if (beep_info.time < 0) {
	    dprintf("audio thread ending\n");
	    break;
	}
	play_tone();
	beep_info.time = 0;
	pthread_cond_signal(&beep_cv);
	pthread_mutex_unlock(&beep_mutex);
    }
    pthread_exit(0);
}

int BeepInit()
{
    int r, error;
    pthread_attr_t bt_attr;
    struct sched_param bt_sched_param;

    snddev = pa_simple_new(NULL, "Morse Code Trainer", PA_STREAM_PLAYBACK,
			   NULL, "beep", &sample_format, NULL, NULL, &error);
    if (snddev == NULL) {
	fprintf(stderr, "Could not initialize audio: %s\n", pa_strerror(error));
	return 1;
    }

    pthread_attr_init(&bt_attr);
    pthread_attr_setschedpolicy(&bt_attr, SCHED_FIFO);
    pthread_attr_getschedparam(&bt_attr, &bt_sched_param);
    bt_sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&bt_attr, &bt_sched_param);
    pthread_mutex_lock(&beep_mutex);
    r = pthread_create(&beep_thread, &bt_attr, beep_threadfunc, NULL);
    if (r) {
	fprintf(stderr, "Could not create audio thread: %d\n", r);
	return 1;
    }

    /* Wait for thread to signal us that it's ready */
    pthread_cond_wait(&beep_cv, &beep_mutex);
    pthread_mutex_unlock(&beep_mutex);

#if 0
    /* Play a test tune and exit */
    Beep(500, 50, 440);
    Beep(500, 50, 680);
    Beep(500, 50, 440);
    Beep(500, 50, 680);
    Beep(500, 50, 440);
    Beep(500, 0, 440);
    Beep(2000, 50, 880);
    BeepWait();
    BeepCleanup();
    exit(0);
#endif

    return 0;
}

int Beep(int time, int volume, int pitch)
{
    dprintf("-->Beep(%d,%d,%d)\n",time,volume,pitch);
    pthread_mutex_lock(&beep_mutex);
    if (beep_info.time) {
	pthread_cond_wait(&beep_cv, &beep_mutex);
    }
    beep_info.time = time;
    beep_info.volume = volume;
    beep_info.pitch = pitch;
    pthread_mutex_unlock(&beep_mutex);
    pthread_cond_signal(&beep_cv);
    dprintf("<--Beep()\n");
}

int BeepWait()
{
    dprintf("-->BeepWait() %d\n", beep_info.time);
    pthread_mutex_lock(&beep_mutex);
    if (beep_info.time) {
	dprintf("<<<<<<<<<<<<<\n");
	pthread_cond_wait(&beep_cv, &beep_mutex);
	dprintf(">>>>>>>>>>>>>\n");
    }
    pthread_mutex_unlock(&beep_mutex);
    dprintf("<--BeepWait()\n");
}

int BeepCleanup()
{
    BeepWait();
    Beep(-1, 0, 0);
    pthread_join(beep_thread, NULL);
    pa_simple_drain(snddev, NULL);
    pa_simple_free(snddev);
    return 0;
}

int BeepResume()
{
    return 0;
}
