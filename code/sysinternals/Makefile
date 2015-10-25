IDIR=../include
ODIR=../obj/sysinternals
BDIR=../bin
CC= /usr/bin/gcc
C_FLAGS_DEBUG= -g2 -Wall -pedantic
C_FLAGS_RELEASE= -Ofast
C_FLAGS= $(C_FLAGS_DEBUG) -std=gnu99 -I$(IDIR)
L_FLAGS= -DLINUX
LIBS=-lm

_DEPS=sysinternals.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
_FIL=librandom libstring libutils keyvaluepair hashing logging database
SRC=$(patsubst %,%.c,$(_FIL))
OBJ=$(patsubst %,$(ODIR)/%,$(patsubst %,%.o,$(_FIL)))
SYSINTERNALS=$(BDIR)/sysinternals.a


.PHONY: clean all prebuild

all: $(SYSINTERNALS)

prebuild:
	splint -DLINUX -I$(IDIR) $(SRC)

$(SYSINTERNALS): $(OBJ) $(DEPS) prebuild | $(BDIR)
	ar rcs $(SYSINTERNALS) $@ $(OBJ)

$(ODIR)/%.o: %.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(C_FLAGS)

$(BDIR):
	mkdir $(BDIR) 2>/dev/null

$(ODIR):
	mkdir $(ODIR) 2>/dev/null

clean:
	rm -f $(ODIR)/*.o $(SYSINTERNALS)
