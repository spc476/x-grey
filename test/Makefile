
CGILIB=/home/spc/source/cgi
CINCL=-I $(CGILIB)/src

CC=gcc -g
CFLAGS=-Wall -pedantic $(CINCL)
LFLAGS=-L$(CGILIB)/$(HOSTDIR) -lcgi5

all : $(HOSTDIR)/mktuples $(HOSTDIR)/sendtuples

$(HOSTDIR)/sendtuples : $(HOSTDIR)/sendtuples.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/sendtuples.o		\
		../common/$(HOSTDIR)/util.o	\
		$(LFLAGS)

$(HOSTDIR)/mktuples : $(HOSTDIR)/mktuples.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/mktuples.o		\
		../common/$(HOSTDIR)/crc32.o	\
		$(LFLAGS)

$(HOSTDIR)/sendtuples.o : src/sendtuples.c
	$(CC) $(CFLAGS) -c -o $@ src/sendtuples.c

$(HOSTDIR)/mktuples.o : src/mktuples.c
	$(CC) $(CFLAGS) -c -o $@ src/mktuples.c

clean:
	/bin/rm -f $(HOSTDIR)/mktuple
	/bin/rm -f $(HOSTDIR)/*.o
	/bin/rm -f src/*~

