IDIR=../include
ODIR=../obj
BDIR=../bin
CC= /usr/bin/gcc
PROF= -DLINUX
C_FLAGS= -O2 -g2 -Wall -pedantic -std=gnu99 -I$(IDIR)
L_FLAGS=  $(PROF)
SYSLIBS=-lm

SDIR=.
EXE=$(BDIR)/badtrip
_OBJ=main.o

OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
SYSINTERNALS=$(BDIR)/sysinternals.a
MERC=$(BDIR)/merc.a


.PHONY: clean all install prebuild

all: $(EXE)

prebuild:
	splint -DLINUX -I$(IDIR) -exportlocal main.c

$(SYSINTERNALS):
	cd ../sysinternals && $(MAKE)

$(MERC):
	cd ../src && $(MAKE)

$(EXE): $(OBJ) $(SYSINTERNALS) $(MERC) prebuild | $(BDIR)
	$(CC) $(L_FLAGS) -o $@ $(OBJ) $(MERC) $(SYSINTERNALS) $(SYSLIBS) -I$(IDIR)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(C_FLAGS)

$(BDIR):
	mkdir $(BDIR) 2>/dev/null

$(ODIR):
	mkdir $(ODIR) 2>/dev/null

clean:
	rm -f $(ODIR)/*.o $(EXE)

