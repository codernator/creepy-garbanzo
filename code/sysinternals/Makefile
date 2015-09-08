IDIR=../include
ODIR=../obj/sysinternals
BDIR=../bin
SDIR=.
CC= /usr/bin/gcc
PROF= -DLINUX
C_FLAGS= -O2 -g2 -Wall -pedantic -std=gnu99 -I$(IDIR)
L_FLAGS=  $(PROF)
LIBS=-lm

_DEPS=sysinternals.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
_FIL=librandom libstring libutils keyvaluepair hashing
SRC=$(patsubst %,$(SDIR)/%,$(patsubst %,%.c,$(_FIL)))
OBJ=$(patsubst %,$(ODIR)/%,$(patsubst %,%.o,$(_FIL)))
SYSINTERNALS=$(BDIR)/sysinternals.a


.PHONY: clean all prebuild

all: $(SYSINTERNALS)

prebuild:
	splint -DLINUX -I$(IDIR) $(SRC)

$(SYSINTERNALS): $(OBJ) $(DEPS) prebuild | $(BDIR)
	ar rcs $(SYSINTERNALS) $@ $(OBJ)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(C_FLAGS)

$(BDIR):
	mkdir $(BDIR) 2>/dev/null

$(ODIR):
	mkdir $(ODIR) 2>/dev/null

clean:
	rm -f $(ODIR)/*.o $(SYSINTERNALS)
