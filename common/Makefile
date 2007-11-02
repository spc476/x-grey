
#CC=gcc -g -pg
CC=gcc -g 
#CC=gcc -fomit-frame-pointer -O3
CFLAGS=-Wall -pedantic
LFLAGS=-lcgi5

all : obj/globals.o		\
		obj/util.o	\
		obj/crc32.o

obj/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

obj/util.o : src/util.c
	$(CC) $(CFLAGS) -c -o $@ src/util.c

obj/crc32.o : src/crc32.c
	$(CC) $(CFLAGS) -c -o $@ src/crc32.c

clean:
	/bin/rm -f obj/*.o
	/bin/rm -f src/*~

