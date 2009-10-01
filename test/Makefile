
#CINCL=-I /usr/local/include
#LINCL=-L /usr/local/lib

CC=gcc  $(CINCL)
CFLAGS=-Wall -pedantic -g -ansi -Wextra
LFLAGS=$(LINCL) -lcgi5

all : build/mktuples build/sendtuples build/pcrc

build/sendtuples : build/sendtuples.o
	$(CC) $(CFLAGS) -o $@		\
		build/sendtuples.o	\
		../common/build/util.o	\
		$(LFLAGS)

build/mktuples : build/mktuples.o
	$(CC) $(CFLAGS) -o $@		\
		build/mktuples.o	\
		../common/build/crc32.o	\
		$(LFLAGS)

build/pcrc : build/pcrc.o
	$(CC) $(CFLAGS) -o $@		\
		build/pcrc.o		\
		$(LFLAGS)

build/sendtuples.o : src/sendtuples.c
	$(CC) $(CFLAGS) -c -o $@ src/sendtuples.c

build/mktuples.o : src/mktuples.c
	$(CC) $(CFLAGS) -c -o $@ src/mktuples.c

build/pcrc.o : src/pcrc.c
	$(CC) $(CFLAGS) -c -o $@ src/pcrc.c

clean:
	/bin/rm -f build/mktuples
	/bin/rm -f build/sendtuples
	/bin/rm -f build/pcrc
	/bin/rm -f build/*.o
	/bin/rm -f src/*~

