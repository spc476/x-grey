
#CINCL=-I /usr/local/include
#LINCL=-L /usr/local/lib
#CC=gcc -pg
CC=gcc $(CINCL)
#CC=gcc -fomit-frame-pointer -O3
CFLAGS=-Wall -pedantic -g -ansi -Wextra
LFLAGS=$(LINCL) -lcgi5

all : build/globals.o		\
		build/util.o	\
		build/crc32.o

build/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

build/util.o : src/util.c
	$(CC) $(CFLAGS) -c -o $@ src/util.c

build/crc32.o : src/crc32.c
	$(CC) $(CFLAGS) -c -o $@ src/crc32.c

clean:
	/bin/rm -f build/*.o
	/bin/rm -f src/*~

