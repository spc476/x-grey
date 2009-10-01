
#CINCL=-I /usr/local/include
#LINCL=-L /usr/local/lib

#CC=gcc -g -pg
CC=gcc $(CINCL)
#CC=gcc -fomit-frame-pointer -O3
CFLAGS=-Wall -pedantic -g -ansi -Wextra
LFLAGS=$(LINCL) -lcgi5

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

