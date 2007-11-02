
CC=gcc -g
CFLAGS=-Wall -pedantic
LFLAGS=-lmilter -lpthread -lcgi5

../bin/smc : obj/main.o	\
		obj/globals.o
	$(CC) $(CFLAGS) -o $@		\
		obj/main.o		\
		obj/globals.o		\
		../common/obj/globals.o	\
		../common/obj/util.o	\
		../common/obj/crc32.o	\
		$(LFLAGS)

obj/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

obj/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

clean:
	/bin/rm -f obj/smc
	/bin/rm -f obj/*.o
	/bin/rm -f src/*~

