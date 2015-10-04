.PHONY: clean install all sysinternals stest merc sysinttest imm 

BIN=bin
OBJ=obj


all: sysinternals merc badtrip

$(BIN): 
	mkdir $(BIN) 2>/dev/null

$(OBJ):
	mkdir $(OBJ) 2>/dev/null


sysinternals: $(BIN) $(OBJ)
	cd sysinternals && $(MAKE)

sysinttest: $(BIN) $(OBJ)
	cd execs && $(MAKE) -f Makefile.sysinternalstest

merc: $(BIN) $(OBJ)
	cd merc && $(MAKE)

badtrip: $(BIN) $(OBJ)
	cd execs && $(MAKE)

imm: $(BIN) $(OBJ)
	cd execs && $(MAKE) -f Makefile.imm

clean:
	rm -fr bin
	rm -fr obj

install:
	cp bin/badtrip ../badtrip
