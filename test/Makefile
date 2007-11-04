
#CINCL=-I /usr/local/include
#LINCL=-L /usr/local/lib

CC=gcc  $(CINCL)
CFLAGS=-Wall -pedantic -g
LFLAGS=$(LINCL) -lcgi5

all : build/mktuples build/sendtuples

build/sendtuples : build/sendtuples.o
	$(CC) $(CFLAGS) -o $@		\
		build/sendtuples.o	\
		../common/build/util.o	\
		$(LFLAGS)

build/mktuples : build/mktuples.o
	$(CC) $(CFLAGS) -o $@		\
		build/mktuples.o		\
		../common/build/crc32.o	\
		$(LFLAGS)

build/sendtuples.o : src/sendtuples.c
	$(CC) $(CFLAGS) -c -o $@ src/sendtuples.c

build/mktuples.o : src/mktuples.c
	$(CC) $(CFLAGS) -c -o $@ src/mktuples.c

clean:
	/bin/rm -f build/mktuple
	/bin/rm -f build/sendtuples
	/bin/rm -f build/*.o
	/bin/rm -f src/*~

