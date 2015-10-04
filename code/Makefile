.PHONY: clean install all sysinternals stest merc

all: sysinternals merc  badtrip

sysinternals:
	cd sysinternals && $(MAKE)

sysinttest:
	cd execs && $(MAKE) -f Makefile.sysinternalstest

merc:
	cd merc && $(MAKE)

badtrip: 
	cd execs && $(MAKE)

imm:
	cd execs && $(MAKE) -f Makefile.imm

clean:
	rm -fr bin
	rm -fr obj

install:
	cp bin/badtrip ../badtrip
