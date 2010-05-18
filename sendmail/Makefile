
CC=gcc -std=c99
CFLAGS=-Wall -pedantic -g -Wextra
LFLAGS=-lmilter -lpthread $(LINCL) -lcgi6

../bin/smc : build/main.o	\
		build/globals.o
	$(CC) $(CFLAGS) -o $@		\
		build/main.o		\
		build/globals.o		\
		../common/build/globals.o	\
		../common/build/util.o	\
		../common/build/crc32.o	\
		$(LFLAGS)

build/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

build/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

clean:
	/bin/rm -f ../bin/smc
	/bin/rm -f build/*.o
	/bin/rm -f src/*~

