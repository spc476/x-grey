
CC=gcc -std=c99
CFLAGS=-Wall -pedantic -g -Wextra
LFLAGS=$(LINCL) -lcgi6

../bin/pfc : build/main.o	\
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
	/bin/rm -f ../bin/pfc
	/bin/rm -f build/*.o
	/bin/rm -f src/*~

