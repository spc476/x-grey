
COPYRIGHT_YEAR = $(shell date +%Y)
PROG_VERSION   = $(shell git describe --tag)

ifeq ($(PROG_VERSION),)
	PROG_VERSION = 1.0.15-t
endif

INSTALL         = /usr/bin/install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA    = $(INSTALL) -m 644

prefix		= /usr/local
exec_prefix	= $(prefix)
bindir		= $(exec_prefix)/bin
datarootdir	= $(prefix)/share
datadir		= $(datarootdir)
localstatedir	= $(prefix)/var
runstatedir	= $(localstatedir)/run

SERVER_STATEDIR        = $(localstatedir)/state/gld
MCP_HELPDIR            = $(datarootdir)/gld
SENDMAIL_FILTERCHANNEL = $(localstatedir)/state/gld/milter

CC      = gcc -std=c99 -Wall -Wextra -pedantic
CFLAGS  = -g
LDFLAGS = -g
LDLIBS  = -lcgi6

override CFLAGS += -DSERVER_STATEDIR='"$(SERVER_STATEDIR)"' -DMCP_HELPDIR='"$(MCP_HELPDIR)"' -DSENDMAIL_FILTERCHANNEL='"$(SENDMAIL_FILTERCHANNEL)"' -DSERVER_PIDFILE='"$(runstatedir)/gld.pid"' -DSENDMAIL_PIDFILE='"$(runstatedir)/smc.pid"' -DPROG_VERSION='"$(PROG_VERSION)"' -DCOPYRIGHT_YEAR='"$(COPYRIGHT_YEAR)"'

#----------------------------------------------------
# Abandon all hope ye who hack here ... 
#----------------------------------------------------

.PHONY: all depend clean server postfix sendmail test

all:
	@echo "make (server | postfix | sendmail | test)"

depend:
	makedepend -Y -- $(CFLAGS) -- `find . -name '*.c'` 2>/dev/null

server:   server/src/gld control/src/gld-mcp
postfix:  postfix/src/pfc
sendmail: sendmail/src/smc
test:	  test/build/mktuples test/build/sendtuples test/build/pcrc

clean:
	$(RM) `find . -name '*.o'`
	$(RM) server/src/gld
	$(RM) control/src/gld-mcp
	$(RM) postfix/src/pfc
	$(RM) sendmail/src/smc

% :
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# ===================================================

server/src/gld: server/src/main.o		\
		server/src/globals.o		\
		server/src/signals.o		\
		server/src/iplist.o		\
		server/src/emaildomain.o	\
		server/src/tuple.o		\
		common/src/globals.o		\
		common/src/util.o		\
		common/src/crc32.o		\
		common/src/bisearch.o

# =====================================================================

control/src/gld-mcp: control/src/main.o		\
		control/src/globals.o		\
		common/src/globals.o		\
		common/src/util.o		\
		common/src/crc32.o
control/src/gld-mcp: override LDLIBS += -lreadline -lcurses

# ===================================================================

postfix/src/pfc: postfix/src/main.o	\
		postfix/src/globals.o	\
		common/src/globals.o	\
		common/src/util.o	\
		common/src/crc32.o

# =====================================================================

sendmail/src/smc: sendmail/src/main.o	\
		sendmail/src/globals.o	\
		common/src/globals.o	\
		common/src/util.o	\
		common/src/crc32.o
sendmail/src/smc: override LDLIBS += -lmilter -lpthread

# ========================================================================

test/src/mktuples: test/src/mktuples.o common/src/crc32.o
test/src/sendtuples: test/src/sendtuples.o common/src/util.o
test/src/pcrc:       test/src/pcrc.o

# ======================================================================

tarball:
	(cd .. ; tar czvf /tmp/x-grey.tar.gz -X x-grey/.exclude x-grey/ )

install:
	@echo "make (install-server | install-postfix | install-sendmail)"

install-all: install-server install-postfix install-sendmail
	
uninstall:
	@echo "make (uninstall-server | uninstall-postfix | uninstall-sendmail)"

uninstall-all: uninstall-server uninstall-postfix uninstall-sendmail

install-server: bin/gld bin/gld-mcp
	$(INSTALL) -d $(DESTDIR)$(runstatedir)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(localstatedir)/state/gld
	$(INSTALL) -d $(DESTDIR)$(datarootdir)/gld
	$(INSTALL_PROGRAM) bin/gld $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) bin/gld-mcp $(DESTDIR)$(bindir)
	$(INSTALL_DATA) COMMANDS $(DESTDIR)$(datarootdir)/gld
	$(INSTALL_DATA) LICENSE $(DESTDIR)$(datarootdir)/gld

uninstall-server:
	$(RM) $(DESTDIR)$(bindir)/gld
	$(RM) $(DESTDIR)$(bindir)/gld-mcp
	$(RM) -r $(DESTDIR)$(datarootdir)/gld
	$(RM) -r $(DESTDIR)$(localstatedir)/state/gld

install-postfix: bin/pfc
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) bin/pfc $(DESTDIR)$(bindir)

uninstall-postfix:
	$(RM) $(DESTDIR)$(bindir)/pfc

install-sendmail: bin/smc
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(localstatedir)/state/gld
	$(INSTALL_PROGRAM) bin/smc $(DESTDIR)$(bindir)
	
uninstall-sendmail:
	$(RM) $(DESTDIR)$(bindir)/smc
	
# DO NOT DELETE

./postfix/src/main.o: ./common/src/greylist.h ./common/src/util.h
./postfix/src/main.o: ./common/src/greylist.h ./common/src/crc32.h
./postfix/src/main.o: ./common/src/globals.h ./common/src/util.h
./postfix/src/main.o: ./postfix/src/globals.h
./postfix/src/globals.o: ./common/src/greylist.h ./common/src/globals.h
./postfix/src/globals.o: ./common/src/util.h ./common/src/util.h
./postfix/src/globals.o: ./common/src/greylist.h ./conf.h
./common/src/crc32.o: ./common/src/greylist.h ./common/src/crc32.h
./common/src/util.o: ./common/src/eglobals.h ./common/src/util.h
./common/src/bisearch.o: ./common/src/bisearch.h
./common/src/globals.o: ./common/src/greylist.h ./common/src/util.h
./control/src/main.o: ./common/src/greylist.h ./common/src/globals.h
./control/src/main.o: ./common/src/util.h ./common/src/util.h
./control/src/main.o: ./common/src/greylist.h ./common/src/crc32.h
./control/src/main.o: ./postfix/src/globals.h
./control/src/globals.o: ./common/src/greylist.h ./common/src/util.h
./control/src/globals.o: ./common/src/greylist.h ./common/src/globals.h
./control/src/globals.o: ./common/src/util.h ./conf.h
./sendmail/src/main.o: ./common/src/greylist.h ./common/src/util.h
./sendmail/src/main.o: ./common/src/greylist.h ./common/src/crc32.h
./sendmail/src/main.o: ./common/src/globals.h ./common/src/util.h
./sendmail/src/main.o: ./postfix/src/globals.h
./sendmail/src/globals.o: ./common/src/greylist.h ./common/src/globals.h
./sendmail/src/globals.o: ./common/src/util.h ./common/src/util.h
./sendmail/src/globals.o: ./common/src/greylist.h ./conf.h
./server/src/main.o: ./common/src/greylist.h ./common/src/globals.h
./server/src/main.o: ./common/src/util.h ./common/src/util.h
./server/src/main.o: ./common/src/greylist.h ./common/src/crc32.h
./server/src/main.o: ./server/src/tuple.h ./server/src/server.h
./server/src/main.o: ./postfix/src/globals.h ./server/src/signals.h
./server/src/main.o: ./server/src/iplist.h ./server/src/emaildomain.h
./server/src/tuple.o: ./common/src/greylist.h ./common/src/util.h
./server/src/tuple.o: ./common/src/greylist.h ./common/src/bisearch.h
./server/src/tuple.o: ./server/src/tuple.h ./common/src/globals.h
./server/src/tuple.o: ./common/src/util.h ./server/src/server.h
./server/src/tuple.o: ./postfix/src/globals.h
./server/src/iplist.o: ./common/src/globals.h ./common/src/util.h
./server/src/iplist.o: ./common/src/util.h ./common/src/greylist.h
./server/src/iplist.o: ./postfix/src/globals.h ./server/src/iplist.h
./server/src/iplist.o: ./common/src/greylist.h
./server/src/emaildomain.o: ./common/src/globals.h ./common/src/util.h
./server/src/emaildomain.o: ./common/src/util.h ./common/src/greylist.h
./server/src/emaildomain.o: ./common/src/bisearch.h
./server/src/emaildomain.o: ./server/src/emaildomain.h
./server/src/emaildomain.o: ./postfix/src/globals.h
./server/src/signals.o: ./common/src/util.h ./common/src/greylist.h
./server/src/signals.o: ./postfix/src/globals.h ./server/src/iplist.h
./server/src/signals.o: ./common/src/greylist.h ./server/src/signals.h
./server/src/globals.o: ./common/src/greylist.h ./common/src/globals.h
./server/src/globals.o: ./common/src/util.h ./common/src/util.h
./server/src/globals.o: ./common/src/greylist.h ./conf.h ./server/src/tuple.h
./server/src/globals.o: ./server/src/server.h ./server/src/signals.h
./server/src/globals.o: ./server/src/iplist.h ./server/src/emaildomain.h
./test/src/sendtuples.o: ./common/src/greylist.h ./common/src/crc32.h
./test/src/sendtuples.o: ./common/src/util.h ./common/src/greylist.h ./conf.h
./test/src/pcrc.o: ./common/src/greylist.h ./common/src/crc32.h
./test/src/pcrc.o: ./common/src/util.h ./common/src/greylist.h ./conf.h
./test/src/mktuples.o: ./common/src/greylist.h ./common/src/crc32.h
