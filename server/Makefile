
#CC=gcc -g -pg
CC=gcc -g
#CC=gcc -fomit-frame-pointer -O3
CFLAGS=-Wall -pedantic 
LFLAGS=-lcgi5

../bin/gld : obj/main.o			\
		obj/globals.o		\
		obj/signals.o		\
		obj/iplist.o		\
		obj/emaildomain.o	\
		obj/tuple.o		
	$(CC) $(CFLAGS) -o $@		\
		obj/main.o		\
		obj/globals.o		\
		obj/signals.o		\
		obj/iplist.o		\
		obj/emaildomain.o	\
		obj/tuple.o		\
		../common/obj/globals.o	\
		../common/obj/util.o	\
		../common/obj/crc32.o	\
		$(LFLAGS)

obj/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

obj/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

obj/signals.o : src/signals.c
	$(CC) $(CFLAGS) -c -o $@ src/signals.c

obj/iplist.o : src/iplist.c
	$(CC) $(CFLAGS) -c -o $@ src/iplist.c

obj/emaildomain.o : src/emaildomain.c
	$(CC) $(CFLAGS) -c -o $@ src/emaildomain.c

obj/tuple.o : src/tuple.c
	$(CC) $(CFLAGS) -c -o $@ src/tuple.c

clean:
	/bin/rm -f ../bin/gld
	/bin/rm -f obj/*.o
	/bin/rm -f src/*~

