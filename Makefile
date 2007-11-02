
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

