CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lm
PROGNAME=tarea2

default : $(PROGNAME)

tarea1 : tarea1.o
	$(CC) $(CFLAGS) tarea2.o btree.o hashext.o hashlin.o -o $(PROGNAME) $(LDFLAGS)

tarea2.o :
	$(CC) $(CFLAGS) tarea2.c -o tarea2.o -c

btree.o :
	$(CC) $(CFLAGS) btree.c -o btree.o -c

hashext.o :
	$(CC) $(CFLAGS) hash_extendible.c -o hashext.o -c

hashlin.o :
	$(CC) $(CFLAGS) hash_lineal.c -o hashlin.o -c

clean:
	rm -rf *.o *.dat *~ $(PROGNAME)

debug:
	make clean && make