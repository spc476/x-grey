
COPYRIGHT_YEAR := $(shell date +%Y)
PROG_VERSION   := $(shell git describe --tag)

ifeq ($(PROG_VERSION),)
	PROG_VERSION = 1.0.20
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

CC      = gcc -std=c99 -Wall -Wextra -pedantic -Wwrite-strings
CFLAGS  = -g
LDFLAGS = -g
LDLIBS  = -lcgi6

override CFLAGS += -DSERVER_STATEDIR='"$(SERVER_STATEDIR)"' -DMCP_HELPDIR='"$(MCP_HELPDIR)"' -DSENDMAIL_FILTERCHANNEL='"$(SENDMAIL_FILTERCHANNEL)"' -DSERVER_PIDFILE='"$(runstatedir)/gld.pid"' -DSENDMAIL_PIDFILE='"$(runstatedir)/smc.pid"' -DPROG_VERSION='"$(PROG_VERSION)"' -DCOPYRIGHT_YEAR='"$(COPYRIGHT_YEAR)"'

#----------------------------------------------------
# Abandon all hope ye who hack here ... 
#----------------------------------------------------

.PHONY: all depend clean dist server postfix sendmail test

all:
	@echo "make (server | postfix | sendmail | test)"

depend:
	makedepend -Y -- $(CFLAGS) -- $(shell find . -name '*.c') 2>/dev/null

server:   server/gld control/gld-mcp
postfix:  postfix/pfc
sendmail: sendmail/smc
test:	  test/mktuples test/sendtuples test/pcrc

clean:
	$(RM) $(shell find . -name '*.o')
	$(RM) $(shell find . -name '*~')
	$(RM) $(shell find . -name '*.bak')
	$(RM) server/gld
	$(RM) control/gld-mcp
	$(RM) postfix/pfc
	$(RM) sendmail/smc
	$(RM) test/mktuples
	$(RM) test/pcrc
	$(RM) test/sendtuples

# ===================================================

server/gld: server/gld.o		\
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

control/gld-mcp: control/gld-mcp.o	\
		control/globals.o	\
		common/globals.o	\
		common/util.o		\
		common/crc32.o
control/gld-mcp: override LDLIBS += -lreadline -lcurses

# ===================================================================

postfix/pfc: postfix/pfc.o		\
		postfix/globals.o	\
		common/globals.o	\
		common/util.o		\
		common/crc32.o

# =====================================================================

sendmail/smc: sendmail/smc.o		\
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

dist:
	git archive -o /tmp/x-grey-$(PROG_VERSION).tar.gz --prefix x-grey/ $(PROG_VERSION)

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

./proto2/cbor.o: ./proto2/cbor.h
./proto2/encode_cmd.o: ./proto2/cbor.h
./postfix/globals.o: ./common/greylist.h ./common/globals.h ./common/util.h
./postfix/globals.o: ./common/greylist.h ./common/util.h ./conf.h
./postfix/pfc.o: ./common/greylist.h ./common/util.h ./common/crc32.h
./postfix/pfc.o: ./common/globals.h ./common/util.h ./common/greylist.h
./postfix/pfc.o: ./postfix/globals.h
./common/crc32.o: ./common/greylist.h ./common/crc32.h
./common/util.o: ./common/eglobals.h ./common/util.h ./common/greylist.h
./common/bisearch.o: ./common/bisearch.h
./common/globals.o: ./common/greylist.h ./common/util.h
./control/gld-mcp.o: ./common/greylist.h ./common/globals.h ./common/util.h
./control/gld-mcp.o: ./common/greylist.h ./common/util.h ./common/crc32.h
./control/gld-mcp.o: ./postfix/globals.h
./control/globals.o: ./common/greylist.h ./common/util.h ./common/globals.h
./control/globals.o: ./common/util.h ./common/greylist.h ./conf.h
./sendmail/smc.o: ./common/greylist.h ./common/util.h ./common/crc32.h
./sendmail/smc.o: ./common/globals.h ./common/util.h ./common/greylist.h
./sendmail/smc.o: ./postfix/globals.h
./sendmail/globals.o: ./common/greylist.h ./common/globals.h ./common/util.h
./sendmail/globals.o: ./common/greylist.h ./common/util.h ./conf.h
./server/tuple.o: ./common/greylist.h ./common/util.h ./common/bisearch.h
./server/tuple.o: ./server/tuple.h ./common/globals.h ./common/util.h
./server/tuple.o: ./common/greylist.h ./server/server.h ./postfix/globals.h
./server/iplist.o: ./common/globals.h ./common/util.h ./common/greylist.h
./server/iplist.o: ./common/util.h ./postfix/globals.h ./server/iplist.h
./server/iplist.o: ./common/greylist.h
./server/gld.o: ./common/greylist.h ./common/globals.h ./common/util.h
./server/gld.o: ./common/greylist.h ./common/util.h ./common/crc32.h
./server/gld.o: ./server/tuple.h ./server/server.h ./postfix/globals.h
./server/gld.o: ./server/signals.h ./server/iplist.h ./server/emaildomain.h
./server/emaildomain.o: ./common/globals.h ./common/util.h
./server/emaildomain.o: ./common/greylist.h ./common/util.h
./server/emaildomain.o: ./common/bisearch.h ./server/emaildomain.h
./server/emaildomain.o: ./postfix/globals.h
./server/signals.o: ./common/util.h ./postfix/globals.h ./server/iplist.h
./server/signals.o: ./common/greylist.h ./server/signals.h
./server/globals.o: ./common/greylist.h ./common/globals.h ./common/util.h
./server/globals.o: ./common/greylist.h ./common/util.h ./conf.h
./server/globals.o: ./server/tuple.h ./server/server.h ./server/signals.h
./server/globals.o: ./server/iplist.h ./server/emaildomain.h
./test/sendtuples.o: ./common/greylist.h ./common/crc32.h ./common/util.h
./test/sendtuples.o: ./conf.h
./test/pcrc.o: ./common/greylist.h ./common/crc32.h ./common/util.h ./conf.h
./test/mktuples.o: ./common/greylist.h ./common/crc32.h
