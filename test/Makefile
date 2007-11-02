
CC=gcc -g
CFLAGS=-Wall -pedantic
LFLAGS=-lcgi5

all : obj/mktuples obj/sendtuples

obj/sendtuples : obj/sendtuples.o
	$(CC) $(CFLAGS) -o $@		\
		obj/sendtuples.o	\
		../common/obj/util.o	\
		$(LFLAGS)

obj/mktuples : obj/mktuples.o
	$(CC) $(CFLAGS) -o $@		\
		obj/mktuples.o		\
		../common/obj/crc32.o	\
		$(LFLAGS)

obj/sendtuples.o : src/sendtuples.c
	$(CC) $(CFLAGS) -c -o $@ src/sendtuples.c

obj/mktuples.o : src/mktuples.c
	$(CC) $(CFLAGS) -c -o $@ src/mktuples.c

clean:
	/bin/rm -f obj/mktuple
	/bin/rm -f obj/sendtuples
	/bin/rm -f obj/*.o
	/bin/rm -f src/*~

