
CC=gcc -std=c99
CFLAGS=-Wall -pedantic -g -Wextra

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

