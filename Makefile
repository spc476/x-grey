
#--------------------------------------------------
#
# change the following to reflect your system's layout.
# Make sure they match what's defined in conf.h
#
#----------------------------------------------------

PIDDIR   = /var/run
BINDIR   = /usr/local/bin
STATEDIR = /var/state/gld
HELPDIR  = /usr/local/share/gld

CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -g

#----------------------------------------------------
# Abandon all hope ye who hack here ... 
#----------------------------------------------------

all:
	@echo "make (server | postfix | sendmail | test)"

server: 	bin/gld bin/gld-mcp

postfix:	bin/pfc

sendmail:	bin/smc

test:	test/build/mktuples test/build/sendtuples test/build/pcrc

clean:	server-clean		\
		postfix-clean	\
		sendmail-clean	\
		test-clean	\
		common-clean
	/bin/rm -rf *~

# ===================================================

server-clean:
	/bin/rm -rf bin/gld bin/gld-mcp
	/bin/rm -rf server/build/*
	/bin/rm -rf server/src/*~
	/bin/rm -rf control/build/*
	/bin/rm -rf control/src/*~

bin/gld: server/build/main.o			\
		server/build/globals.o		\
		server/build/signals.o		\
		server/build/iplist.o		\
		server/build/emaildomain.o	\
		server/build/tuple.o		\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o
	$(CC) $(CFLAGS) -o $@			\
		server/build/main.o		\
		server/build/globals.o		\
		server/build/signals.o		\
		server/build/iplist.o		\
		server/build/emaildomain.o	\
		server/build/tuple.o		\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o		\
		-lcgi6
		
server/build/main.o : server/src/main.c 	\
		common/src/greylist.h		\
		common/src/globals.h		\
		common/src/util.h		\
		common/src/crc32.h		\
		server/src/tuple.h		\
		server/src/globals.h		\
		server/src/signals.h		\
		server/src/server.h		\
		server/src/iplist.h		\
		server/src/emaildomain.h
	$(CC) $(CFLAGS) -c -o $@ server/src/main.c
	
server/build/globals.o : server/src/globals.c	\
		common/src/greylist.h		\
		common/src/globals.h		\
		common/src/util.h		\
		conf.h				\
		server/src/tuple.h		\
		server/src/signals.h		\
		server/src/iplist.h		\
		server/src/emaildomain.h
	$(CC) $(CFLAGS) -c -o $@ server/src/globals.c
	
server/build/signals.o : server/src/signals.c	\
		common/src/util.h		\
		server/src/globals.h		\
		server/src/iplist.h		\
		server/src/signals.h
	$(CC) $(CFLAGS) -c -o $@ server/src/signals.c
	
server/build/iplist.o : server/src/iplist.c	\
		common/src/globals.h		\
		common/src/util.h		\
		server/src/globals.h		\
		server/src/iplist.h
	$(CC) $(CFLAGS) -c -o $@ server/src/iplist.c
	
server/build/emaildomain.o : server/src/emaildomain.c	\
		common/src/globals.h		\
		common/src/util.h		\
		server/src/emaildomain.h	\
		server/src/globals.h
	$(CC) $(CFLAGS) -c -o $@ server/src/emaildomain.c
	
server/build/tuple.o : server/src/tuple.c	\
		common/src/greylist.h		\
		common/src/util.h		\
		server/src/tuple.h		\
		server/src/globals.h
	$(CC) $(CFLAGS) -c -o $@ server/src/tuple.c
	
# =====================================================================

bin/gld-mcp: control/build/main.o		\
		control/build/globals.o		\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o
	$(CC) $(CFLAGS) -o $@ 			\
		control/build/main.o		\
		control/build/globals.o		\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o		\
		-lcgi6 -lreadline -lcurses
		
control/build/main.o : control/src/main.c	\
		common/src/greylist.h		\
		common/src/globals.h		\
		common/src/util.h		\
		common/src/crc32.h		\
		control/src/globals.h
	$(CC) $(CFLAGS) -c -o $@ control/src/main.c
	
control/build/globals.o : control/src/globals.c	\
		common/src/greylist.h		\
		common/src/util.h		\
		common/src/globals.h		\
		conf.h
	$(CC) $(CFLAGS) -c -o $@ control/src/globals.c
	
# ===================================================================

postfix-clean:
	/bin/rm -rf bin/pfc
	/bin/rm -rf postfix/build/*
	/bin/rm -rf postfix/src/*~
	
bin/pfc: postfix/build/main.o			\
		postfix/build/globals.o		\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o
	$(CC) $(CFLAGS) -o $@			\
		postfix/build/main.o		\
		postfix/build/globals.o		\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o		\
		-lcgi6
		
postfix/build/main.o : postfix/src/main.c	\
		common/src/greylist.h		\
		common/src/util.h		\
		common/src/crc32.h		\
		common/src/globals.h		\
		postfix/src/globals.h
	$(CC) $(CFLAGS) -c -o $@ postfix/src/main.c
	
postfix/build/globals.o : postfix/src/globals.c	\
		common/src/greylist.h		\
		common/src/globals.h		\
		common/src/util.h		\
		conf.h
	$(CC) $(CFLAGS) -c -o $@ postfix/src/globals.c

# =====================================================================

sendmail-clean:
	/bin/rm -rf bin/smc
	/bin/rm -rf sendmail/build/*
	/bin/rm -rf sendmail/src/*~
	
bin/smc: sendmail/build/main.o			\
		sendmail/build/globals.o	\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o
	$(CC) $(CFLAGS) -o $@			\
		sendmail/build/main.o		\
		sendmail/build/globals.o	\
		common/build/globals.o		\
		common/build/util.o		\
		common/build/crc32.o		\
		-lmilter -lpthread -lcgi6

sendmail/build/main.o : sendmail/src/main.c	\
		common/src/greylist.h		\
		common/src/util.h		\
		common/src/crc32.h		\
		common/src/globals.h		\
		sendmail/src/globals.h
	$(CC) $(CFLAGS) -c -o $@ sendmail/src/main.c
	
sendmail/build/globals.o : sendmail/src/globals.c	\
		common/src/greylist.h		\
		common/src/globals.h		\
		common/src/util.h		\
		conf.h
	$(CC) $(CFLAGS) -c -o $@ sendmail/src/globals.c
	
# ========================================================================

test-clean:
	/bin/rm -rf test/build/*
	/bin/rm -rf test/src/*~
	
test/build/mktuples: test/build/mktuples.o	\
		common/build/crc32.o
	$(CC) $(CFLAGS) -o $@			\
		test/build/mktuples.o		\
		common/build/crc32.o		\
		-lcgi6

test/build/sendtuples: test/build/sendtuples.o	\
		common/build/util.o
	$(CC) $(CFLAGS) -o $@			\
		test/build/sendtuples.o		\
		common/build/util.o		\
		-lcgi6

test/build/pcrc: test/build/pcrc.o
	$(CC) $(CFLAGS) -o $@ test/build/pcrc.o -lcgi6
	
test/build/sendtuples.o : test/src/sendtuples.c	\
		common/src/greylist.h		\
		common/src/crc32.h		\
		common/src/util.h		\
		conf.h
	$(CC) $(CFLAGS) -c -o $@ test/src/sendtuples.c
	
test/build/mktuples.o: test/src/mktuples.c	\
		common/src/greylist.h		\
		common/src/crc32.h
	$(CC) $(CFLAGS) -c -o $@ test/src/mktuples.c

test/build/pcrc.o: test/src/pcrc.c		\
		common/src/greylist.h		\
		common/src/crc32.h		\
		common/src/util.h		\
		conf.h
	$(CC) $(CFLAGS) -c -o $@ test/src/pcrc.c
	
# ======================================================================

common-clean:
	/bin/rm -rf common/build/*
	/bin/rm -rf common/src/*~

common/build/globals.o : common/src/globals.c	\
		common/src/greylist.h		\
		common/src/util.h
	$(CC) $(CFLAGS) -c -o $@ common/src/globals.c
	
common/build/util.o: common/src/util.c		\
		common/src/eglobals.h		\
		common/src/util.h
	$(CC) $(CFLAGS) -c -o $@ common/src/util.c
	
common/build/crc32.o: common/src/crc32.c	\
		common/src/greylist.h		\
		common/src/crc32.h
	$(CC) $(CFLAGS) -c -o $@ common/src/crc32.c

common/build/bisearch.o : common/src/bisearch.c	\
		common/src/bisearch.h
	$(CC) $(CFLAGS) -c -o $@ common/src/bisearch.c

# =======================================================================

tarball:
	(cd .. ; tar czvf /tmp/x-grey.tar.gz -X x-grey/.exclude x-grey/ )

install:
	@echo "make (install-server | install-postfix | install-sendmail)"
	
remove:
	@echo "make (remove-server | remove-postfix | remove-sendmail)"

install-server:
	install -d $(PIDDIR)
	install -d $(BINDIR)
	install -d $(STATEDIR)
	install -d $(HELPDIR)
	install bin/gld $(BINDIR)
	install bin/gld-mcp $(BINDIR)
	install COMMANDS $(HELPDIR)
	install LICENSE  $(HELPDIR)

remove-server:
	/bin/rm -rf $(BINDIR)/gld
	/bin/rm -rf $(BINDIR)/gld-mcp
	/bin/rm -rf $(HELPDIR)/COMMANDS
	/bin/rm -rf $(HELPDIR)/LICENSE
	@echo "You may also want to remove $(STATEDIR) and $(HELPDIR)"
	
install-postfix:
	install -d $(BINDIR)
	install bin/pfc $(BINDIR)

remove-postfix:
	/bin/rm -rf $(BINDIR)/pfc

install-sendmail:
	install -d $(BINDIR)
	install -d $(STATEDIR)
	install bin/smc $(BINDIR)
	
remove-sendmail:
	/bin/rm -rf $(BINDIR)/smc
	@echo "You may also want to remove $(STATEDIR)"
	
