DEVICE=PA

BEEPERS = beepLinux.c beepOSS.c beepX11.c beepALSA.c
SOURCES = alarm.c morse.c alarm.h beep.h $(BEEPERS)

# The flags necessary to link with the X11 libraries.
X11LIBS = -L/usr/X11R6/lib -lX11

# The flags necessary to link with PulseAudio and support pthread
PA_CFLAGS = -pthread $(shell pkg-config --cflags libpulse-simple)
PA_LIBS = $(shell pkg-config --libs libpulse-simple) -pthread

# The flags necessary to link with ALSA
ALSA_CFLAGS = $(shell pkg-config --cflags alsa)
ALSA_LIBS = $(shell pkg-config --libs alsa)

# Any additional flags your favorite C compiler requires to work.
CFLAGS  = -O3 -I/usr/X11R6/include $($(device)_EXTRA_CFLAGS)

morse:	morse${DEVICE}
	rm -f $@
	ln morse${DEVICE} $@

morseX11:	morse.o beepX11.o alarm.o
	$(CC) $(LDFLAGS) -o $@ morse.o beepX11.o alarm.o $(X11LIBS)

morseLinux:	morse.o beepLinux.o alarm.o
	$(CC) $(LDFLAGS) -o $@ morse.o beepLinux.o alarm.o -lm

morseOSS:	morse.o beepOSS.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ morse.o beepOSS.o -lm

morsePA:	morse.o beepPA.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(PA_CFLAGS) -o $@ morse.o beepPA.o -lm $(PA_LIBS) -lncurses

morseALSA:	morse.o beepALSA.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(ALSA_CFLAGS) -o $@ morse.o beepALSA.o -lm $(ALSA_LIBS)

morse.o:	beep.h Makefile
beepX11.o:	beep.h alarm.h
beepLinux.o:	beep.h alarm.h
beepOSS.o:	beep.h
beepALSA.o:	beep.h
alarm.o:	alarm.h

clean:
	rm -f *.o morse QSO morseX11 morseLinux morseOSS morsePA morseALSA
