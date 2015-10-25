IDIR=../include
ODIR=../obj
BDIR=../bin
CC= /usr/bin/gcc
C_FLAGS_DEBUG= -g2 -Wall -pedantic
C_FLAGS_RELEASE= -Ofast
C_FLAGS= $(C_FLAGS_DEBUG) -std=gnu99 -I$(IDIR)
L_FLAGS= -DLINUX
SYSLIBS=-lm

EXE=$(BDIR)/badtrip
_OBJ=main.o

_FIL=main
SRC=$(patsubst %,%.c,$(_FIL))
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
SYSINTERNALS=$(BDIR)/sysinternals.a
MERC=$(BDIR)/merc.a


.PHONY: clean all prebuild

all: $(EXE)

prebuild:
	splint -DLINUX -I$(IDIR) $(SRC)

$(SYSINTERNALS):
	cd ../sysinternals && $(MAKE)

$(MERC):
	cd ../merc && $(MAKE)

$(EXE): $(OBJ) $(SYSINTERNALS) $(MERC) prebuild | $(BDIR)
	$(CC) $(L_FLAGS) -o $@ $(OBJ) $(MERC) $(SYSINTERNALS) $(SYSLIBS) -I$(IDIR)

$(ODIR)/%.o: %.c | $(ODIR)
	$(CC) -c -o $@ $< $(C_FLAGS)

$(BDIR):
	mkdir $(BDIR) 2>/dev/null

$(ODIR):
	mkdir $(ODIR) 2>/dev/null

clean:
	rm -f $(ODIR)/*.o $(EXE)

