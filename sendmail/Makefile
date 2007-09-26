
CGILIB=/home/spc/source/cgi
CINCL=-I $(CGILIB)/src

CC=gcc -g
CFLAGS=-Wall -pedantic $(CINCL)
LFLAGS=-lmilter -lpthread -L$(CGILIB)/$(HOSTDIR) -lcgi5

$(HOSTDIR)/smc : $(HOSTDIR)/main.o	\
		$(HOSTDIR)/globals.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/main.o		\
		$(HOSTDIR)/globals.o		\
		../common/$(HOSTDIR)/globals.o	\
		../common/$(HOSTDIR)/util.o	\
		../common/$(HOSTDIR)/crc32.o	\
		$(LFLAGS)

$(HOSTDIR)/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

$(HOSTDIR)/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

clean:
	/bin/rm -f $(HOSTDIR)/smc
	/bin/rm -f $(HOSTDIR)/*.o
	/bin/rm -f src/*~

