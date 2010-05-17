
CC=gcc -std=c99
CFLAGS=-Wall -pedantic -g -Wextra
LFLAGS=$(LINCL) -lcgi6

../bin/gld : build/main.o			\
		build/globals.o		\
		build/signals.o		\
		build/iplist.o		\
		build/emaildomain.o	\
		build/tuple.o		
	$(CC) $(CFLAGS) -o $@		\
		build/main.o		\
		build/globals.o		\
		build/signals.o		\
		build/iplist.o		\
		build/emaildomain.o	\
		build/tuple.o		\
		../common/build/globals.o	\
		../common/build/util.o	\
		../common/build/crc32.o	\
		$(LFLAGS)

build/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

build/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

build/signals.o : src/signals.c
	$(CC) $(CFLAGS) -c -o $@ src/signals.c

build/iplist.o : src/iplist.c
	$(CC) $(CFLAGS) -c -o $@ src/iplist.c

build/emaildomain.o : src/emaildomain.c
	$(CC) $(CFLAGS) -c -o $@ src/emaildomain.c

build/tuple.o : src/tuple.c
	$(CC) $(CFLAGS) -c -o $@ src/tuple.c

clean:
	/bin/rm -f ../bin/gld
	/bin/rm -f build/*.o
	/bin/rm -f src/*~

