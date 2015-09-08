.PHONY: clean install all

all: badtrip

sysinternals:
	cd sysinternals && $(MAKE)

merc:
	cd src && $(MAKE)

badtrip:
	cd execs && $(MAKE)

clean:
	rm -fr bin
	rm -fr obj

install:
	cp $(EXE) ../.
