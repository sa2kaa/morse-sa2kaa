CFLAGS  = -O3

OBJECTS = QSO.o put_form.o make_fre.o grammar.o util.o init_str.o
INCLUDES = antenna.h callsign.h city.h cityh.h heights.h jobs.h misc.h \
           names.h newcity.h rig.h weather.h

all:	QSO

QSO:	$(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

init_str.o:	$(INCLUDES)

clean:
	rm -f *.o QSO
