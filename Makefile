
CGILIB=/home/spc/source/cgi
CINCL=-I $(CGILIB)/src

CC=gcc -g
CFLAGS=-Wall -pedantic $(CINCL)
LFLAGS=-L$(CGILIB)/$(HOSTDIR) -lcgi5

$(HOSTDIR)/gl : $(HOSTDIR)/main.o	\
		$(HOSTDIR)/globals.o	\
		$(HOSTDIR)/signals.o	\
		$(HOSTDIR)/report.o	\
		$(HOSTDIR)/util.o
	$(CC) $(CFLAGS) -o $@		\
		$(HOSTDIR)/main.o	\
		$(HOSTDIR)/globals.o	\
		$(HOSTDIR)/signals.o	\
		$(HOSTDIR)/report.o	\
		$(HOSTDIR)/util.o	\
		$(LFLAGS)

$(HOSTDIR)/main.o : src/main.c	
	$(CC) $(CFLAGS) -c -o $@ src/main.c

$(HOSTDIR)/globals.o : src/globals.c
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

$(HOSTDIR)/signals.o : src/signals.c
	$(CC) $(CFLAGS) -c -o $@ src/signals.c

$(HOSTDIR)/report.o : src/report.c
	$(CC) $(CFLAGS) -c -o $@ src/report.c

$(HOSTDIR)/util.o : src/util.c
	$(CC) $(CFLAGS) -c -o $@ src/util.c

clean:
	/bin/rm -f $(HOSTDIR)/gl
	/bin/rm -f $(HOSTDIR)/*.o
	/bin/rm -f src/*~

