CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lm
PROGNAME=tarea2

default : $(PROGNAME)

tarea1 : tarea1.o
	$(CC) $(CFLAGS) tarea2.o -o $(PROGNAME) $(LDFLAGS)

tarea2.o :
	$(CC) $(CFLAGS) tarea2.c -o tarea2.o -c

clean:
	rm -rf *.o *.dat *~ $(PROGNAME)

debug:
	make clean && make