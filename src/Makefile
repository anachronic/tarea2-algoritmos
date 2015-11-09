CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lm
PROGNAME=tarea2

default : $(PROGNAME)

extmem.o :
	$(CC) $(CFLAGS) extmem.c -o extmem.o -c

tarea2.o :
	$(CC) $(CFLAGS) tarea2.c -o tarea2.o -c

btree.o :
	$(CC) $(CFLAGS) btree.c -o btree.o -c

hashext.o :
	$(CC) $(CFLAGS) hash_extendible.c -o hashext.o -c

hashlin.o :
	$(CC) $(CFLAGS) hash_lineal.c -o hashlin.o -c

tarea2 : tarea2.o btree.o hashext.o hashlin.o extmem.o
	$(CC) $(CFLAGS) tarea2.o btree.o hashext.o hashlin.o extmem.o -o $(PROGNAME) $(LDFLAGS)

clean:
	rm -rf *.o *.dat *~ $(PROGNAME)

debug:
	make clean && make