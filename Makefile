
all:
	(cd common ; make )
	(cd postfix ; make)
	(cd server ; make )
	(cd control ; make)

clean:
	(cd common ; make clean)
	(cd postfix ; make clean)
	(cd server ; make clean)
	(cd control ; make clean)
	/bin/rm -f *~

