
COPYRIGHT_YEAR = $(shell date +%Y)
PROG_VERSION   = $(shell git describe --tag)

ifeq ($(PROG_VERSION),)
	PROG_VERSION = 1.0.18
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

server:   server/gld control/gld-mcp
postfix:  postfix/pfc
sendmail: sendmail/smc
test:	  test/mktuples test/sendtuples test/pcrc

clean:
	$(RM) `find . -name '*.o'`
	$(RM) `find . -name '*~'`
	$(RM) `find . -name '*.bak'`
	$(RM) server/gld
	$(RM) control/gld-mcp
	$(RM) postfix/pfc
	$(RM) sendmail/smc
	$(RM) test/mktuples
	$(RM) test/pcrc
	$(RM) test/sendtuples

% :
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# ===================================================

server/gld: server/main.o		\
		server/globals.o	\
		server/signals.o	\
		server/iplist.o		\
		server/emaildomain.o	\
		server/tuple.o		\
		common/globals.o	\
		common/util.o		\
		common/crc32.o		\
		common/bisearch.o

# =====================================================================

control/gld-mcp: control/main.o		\
		control/globals.o	\
		common/globals.o	\
		common/util.o		\
		common/crc32.o
control/gld-mcp: override LDLIBS += -lreadline -lcurses

# ===================================================================

postfix/pfc: postfix/main.o		\
		postfix/globals.o	\
		common/globals.o	\
		common/util.o		\
		common/crc32.o

# =====================================================================

sendmail/smc: sendmail/main.o		\
		sendmail/globals.o	\
		common/globals.o	\
		common/util.o		\
		common/crc32.o
sendmail/smc: override LDLIBS += -lmilter -lpthread

# ========================================================================

test/mktuples:   test/mktuples.o common/crc32.o
test/sendtuples: test/sendtuples.o common/util.o
test/pcrc:       test/pcrc.o

# ======================================================================

tarball:
	(cd .. ; tar czvf /tmp/x-grey.tar.gz -X x-grey/.exclude x-grey/ )

install:
	@echo "make (install-server | install-postfix | install-sendmail)"

install-all: install-server install-postfix install-sendmail
	
uninstall:
	@echo "make (uninstall-server | uninstall-postfix | uninstall-sendmail)"

uninstall-all: uninstall-server uninstall-postfix uninstall-sendmail

install-server: server/gld control/gld-mcp
	$(INSTALL) -d $(DESTDIR)$(runstatedir)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(localstatedir)/state/gld
	$(INSTALL) -d $(DESTDIR)$(datarootdir)/gld
	$(INSTALL_PROGRAM) server/gld $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) control/gld-mcp $(DESTDIR)$(bindir)
	$(INSTALL_DATA) COMMANDS $(DESTDIR)$(datarootdir)/gld
	$(INSTALL_DATA) LICENSE $(DESTDIR)$(datarootdir)/gld

uninstall-server:
	$(RM) $(DESTDIR)$(bindir)/gld
	$(RM) $(DESTDIR)$(bindir)/gld-mcp
	$(RM) -r $(DESTDIR)$(datarootdir)/gld
	$(RM) -r $(DESTDIR)$(localstatedir)/state/gld

install-postfix: postfix/pfc
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) postfix/pfc $(DESTDIR)$(bindir)

uninstall-postfix:
	$(RM) $(DESTDIR)$(bindir)/pfc

install-sendmail: sendmail/smc
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(localstatedir)/state/gld
	$(INSTALL_PROGRAM) sendmail/smc $(DESTDIR)$(bindir)
	
uninstall-sendmail:
	$(RM) $(DESTDIR)$(bindir)/smc
	
# DO NOT DELETE

./postfix/main.o: ./common/greylist.h ./common/util.h ./common/greylist.h
./postfix/main.o: ./common/crc32.h ./common/globals.h ./common/util.h
./postfix/main.o: ./postfix/globals.h
./postfix/globals.o: ./common/greylist.h ./common/globals.h ./common/util.h
./postfix/globals.o: ./common/util.h ./common/greylist.h ./conf.h
./common/crc32.o: ./common/greylist.h ./common/crc32.h
./common/util.o: ./common/eglobals.h ./common/util.h
./common/bisearch.o: ./common/bisearch.h
./common/globals.o: ./common/greylist.h ./common/util.h
./control/main.o: ./common/greylist.h ./common/globals.h ./common/util.h
./control/main.o: ./common/util.h ./common/greylist.h ./common/crc32.h
./control/main.o: ./postfix/globals.h
./control/globals.o: ./common/greylist.h ./common/util.h ./common/greylist.h
./control/globals.o: ./common/globals.h ./common/util.h ./conf.h
./sendmail/main.o: ./common/greylist.h ./common/util.h ./common/greylist.h
./sendmail/main.o: ./common/crc32.h ./common/globals.h ./common/util.h
./sendmail/main.o: ./postfix/globals.h
./sendmail/globals.o: ./common/greylist.h ./common/globals.h ./common/util.h
./sendmail/globals.o: ./common/util.h ./common/greylist.h ./conf.h
./server/main.o: ./common/greylist.h ./common/globals.h ./common/util.h
./server/main.o: ./common/util.h ./common/greylist.h ./common/crc32.h
./server/main.o: ./server/tuple.h ./server/server.h ./postfix/globals.h
./server/main.o: ./server/signals.h ./server/iplist.h ./server/emaildomain.h
./server/tuple.o: ./common/greylist.h ./common/util.h ./common/greylist.h
./server/tuple.o: ./common/bisearch.h ./server/tuple.h ./common/globals.h
./server/tuple.o: ./common/util.h ./server/server.h ./postfix/globals.h
./server/iplist.o: ./common/globals.h ./common/util.h ./common/util.h
./server/iplist.o: ./common/greylist.h ./postfix/globals.h ./server/iplist.h
./server/iplist.o: ./common/greylist.h
./server/emaildomain.o: ./common/globals.h ./common/util.h ./common/util.h
./server/emaildomain.o: ./common/greylist.h ./common/bisearch.h
./server/emaildomain.o: ./server/emaildomain.h ./postfix/globals.h
./server/signals.o: ./common/util.h ./common/greylist.h ./postfix/globals.h
./server/signals.o: ./server/iplist.h ./common/greylist.h ./server/signals.h
./server/globals.o: ./common/greylist.h ./common/globals.h ./common/util.h
./server/globals.o: ./common/util.h ./common/greylist.h ./conf.h
./server/globals.o: ./server/tuple.h ./server/server.h ./server/signals.h
./server/globals.o: ./server/iplist.h ./server/emaildomain.h
./test/sendtuples.o: ./common/greylist.h ./common/crc32.h ./common/util.h
./test/sendtuples.o: ./common/greylist.h ./conf.h
./test/pcrc.o: ./common/greylist.h ./common/crc32.h ./common/util.h
./test/pcrc.o: ./common/greylist.h ./conf.h
./test/mktuples.o: ./common/greylist.h ./common/crc32.h
