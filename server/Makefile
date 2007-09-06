
CGILIB=/home/spc/source/cgi
CINCL=-I $(CGILIB)/src

CC=gcc -g
CFLAGS=-Wall -pedantic $(CINCL) -DDDT
LFLAGS=-L$(CGILIB)/$(HOSTDIR) -lcgi5

$(HOSTDIR)/gl : $(HOSTDIR)/main.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/signals.o		\
		$(HOSTDIR)/iplist.o		\
		$(HOSTDIR)/tuple.o		
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/main.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/signals.o		\
		$(HOSTDIR)/iplist.o		\
		$(HOSTDIR)/tuple.o		\
		../common/$(HOSTDIR)/globals.o	\
		../common/$(HOSTDIR)/util.o	\
		$(LFLAGS)

$(HOSTDIR)/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

$(HOSTDIR)/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

$(HOSTDIR)/signals.o : src/signals.c
	$(CC) $(CFLAGS) -c -o $@ src/signals.c

$(HOSTDIR)/iplist.o : src/iplist.c
	$(CC) $(CFLAGS) -c -o $@ src/iplist.c

$(HOSTDIR)/tuple.o : src/tuple.c
	$(CC) $(CFLAGS) -c -o $@ src/tuple.c

clean:
	/bin/rm -f $(HOSTDIR)/gl
	/bin/rm -f $(HOSTDIR)/*.o
	/bin/rm -f src/*~

