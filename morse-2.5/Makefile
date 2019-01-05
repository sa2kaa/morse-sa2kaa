# Select your audio output device.  Current choices are:
#
#   X11:    The X11 window system.
#   Linux:  IBM PC Console running Linux.
#   OSS:    Open Sound System /dev/dsp device.
#   PA:     PulseAudio using the pulse-simple client API.
#   ALSA:   Advanced Linux Sound Architecture
#
# Running on Linux:
#   Many Linux laptops seem to have console speakers that are unreliable
#   for various hardware and software reasons.  You may be better off
#   using morseALSA or morseOSS than morseLinux.
#
# Running on Mac OS X:
# (1) Use X11.  The user must, as with any X11 client, set the DISPLAY 
#   variable, and have the X server running. Finally, the X11 output 
#   preferences dialog should have "Use system alert effect" unchecked; 
#   otherwise, the system alert (settable, but unlikely to be useful to 
#   copy code in any event) will be used instead of the X beep.
#
# Adding a new device is as simple as creating a new implementation of the
# beep.h interface.  See beep*.c for examples.  Please send any additions
# to the authors!
#
#DEVICE = X11
#DEVICE = Linux
#DEVICE = OSS
#DEVICE = ALSA
DEVICE = PA

VERSION=2.5

MANPAGES = morse.1 QSO.1 
DOCS = README HISTORY COPYING morse.xml $(MANPAGES)
ALL=  $(DOCS) Makefile $(SOURCES) test_input \
	morse.d/*.[ch] morse.d/Makefile \
	qso.d/*.[ch] qso.d/Makefile

default: all
	make testmorse

all: morse QSO morse.1 QSO.1

morse:
	cd morse.d && make DEVICE=${DEVICE}
	ln morse.d/morse ./morse

QSO:
	cd qso.d && make
	ln qso.d/QSO ./QSO

#
# "Jocks find quartz glyph, vex BMW." is my attempt to win Stephen J. Gould's
# prize (a copy of all his books) for the first person who can come up with a
# "perfect pangram": a meaningful sentence consisting entirely of common
# English words, with no abbreviations or proper names, that contains each
# letter exactly once. He rejected it because it contains "BMW", alas, but
# he did say it's the closest he's seen so far. - Joe Dellinger
#
testmorse:	morse QSO
	(cat test_input; qso.d/QSO) | ./morse -w 24 -l -e

testqso:	morse QSO
	qso.d/QSO | ./morse -w 20 -l -e

all:	morse QSO

clean:
	rm -f morse QSO *.1 *.html SHIPPER.*
	cd morse.d; make clean
	cd qso.d; make clean

pristine: clean
	rm -f $(MANPAGES) morse.html

morse.1 QSO.1: morse.xml
	xmlto man morse.xml

morse.html: morse.xml
	xmlto xhtml-nochunks morse.xml

morse-$(VERSION).tar.gz: $(ALL)
	@ls $(ALL) | sed s:^:morse-$(VERSION)/: >MANIFEST
	@(cd ..; ln -s morse-classic morse-$(VERSION))
	(cd ..; tar -czf morse-classic/morse-$(VERSION).tar.gz `cat morse-classic/MANIFEST`)
	@(cd ..; rm morse-$(VERSION))

dist: morse-$(VERSION).tar.gz

cppcheck:
	cppcheck --template gcc --enable=all qso.d/*.[ch]
	cppcheck --template gcc --enable=all morse.d/*.[ch]


release: morse-$(VERSION).tar.gz morse.html
	shipper -u -m -t; make clean
