
CGILIB=/home/spc/source/cgi
CINCL=-I $(CGILIB)/src

CC=gcc -g
CFLAGS=-Wall -pedantic $(CINCL) -DDDT
LFLAGS=-L$(CGILIB)/$(HOSTDIR) -lcgi5

all : $(HOSTDIR)/globals.o	\
		$(HOSTDIR)/util.o

$(HOSTDIR)/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

$(HOSTDIR)/util.o : src/util.c
	$(CC) $(CFLAGS) -c -o $@ src/util.c

clean:
	/bin/rm -f $(HOSTDIR)/*.o
	/bin/rm -f src/*~

