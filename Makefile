
#--------------------------------------------------
#
# change the following to reflect your system's layout.
# Make sure they match what's defined in conf.h
#
#----------------------------------------------------

PIDDIR=/var/run
BINDIR=/usr/local/bin
STATEDIR=/var/state/gld
HELPDIR=/usr/local/share/gld

#----------------------------------------------------
# Abandon all hope ye who hack here ... 
#----------------------------------------------------

all:
	echo "make (server | postfix | sendmail | test)"

server:	bin/gld bin/gld-mcp

bin/gld bin/gld-mcp:
	(cd common ; make)
	(cd server ; make)
	(cd control ; make)

postfix: bin/pfc

bin/pfc:
	(cd common ; make)
	(cd postfix ; make)

sendmail: bin/smc

bin/smc:
	(cd common ; make)
	(cd sendmail ; make)

test: test/obj/mktuples test/obj/sendtuples

test/obj/mktuples test/obj/sendtuples:
	(cd common ; make)
	(cd test ; make)

clean:
	(cd common ; make clean)
	(cd postfix ; make clean)
	(cd server ; make clean)
	(cd control ; make clean)
	(cd test ; make clean)
	/bin/rm -f *~
	/bin/rm -f bin/*

tarball:
	(cd .. ; tar czvf /tmp/graylist.tar.gz -X graylist/.exclude graylist/ )

install:
	echo "make (install-server | install-postfix | install-sendmail)"
	
install-server:
	install -d $(PIDDIR)
	install -d $(BINDIR)
	install -d $(STATEDIR)
	install -d $(HELPDIR)
	install bin/gld $(BINDIR)
	install bin/gld-mcp $(BINDIR)
	install COMMANDS $(HELPDIR)
	install LICENSE  $(HELPDIR)

install-postfix:
	install -d $(BINDIR)
	install bin/pfc $(BINDIR)

install-sendmail:
	install -d $(BINDIR)
	install -d $(STATEDIR)
	install bin/smc $(BINDIR)
	