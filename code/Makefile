IDIR=include
ODIR=obj
BDIR=bin
SDIR=src
CC= /usr/bin/gcc
PROF= -DLINUX
C_FLAGS= -O2 -g2 -Wall -pedantic -std=gnu99 -I$(IDIR)
L_FLAGS=  $(PROF)
LIBS=-lm
_DEPS=merc.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
EXE=$(BDIR)/badtrip
_OBJ=act_comm.o act_combat.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     affects.o alias.o at.o auction.o \
     ban.o bank.o bfield.o bit.o \
     combat_roll.o comm.o const.o \
     db.o disable.o \
     effects.o enchant.o \
     fight.o find.o flags.o for.o \
     handler.o healer.o help.o \
     imprint.o interp.o \
     libfile.o librandom.o libstring.o libutils.o logging.o lookup.o \
     magic.o mem.o mob_cmds.o mob_prog.o mobs.o \
     nanny.o nickname.o note.o \
     olc.o olc_areas.o olc_groups.o olc_helps.o olc_mobs.o \
     olc_objects.o olc_progs.o olc_rooms.o olc_save.o olc_skills.o ovnum.o \
     race.o random.o recycle.o repair.o restring.o \
     save.o scan.o set.o skills.o special.o spells.o stat.o string.o \
     tables.o teleport.o temp.o \
     unique.o update.o variant.o who.o
		  
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(EXE): $(OBJ) $(DEPS)
	$(CC) $(L_FLAGS) -o $@ $(OBJ) $(LIBS) -I$(IDIR)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(C_FLAGS)

clean:
	rm -f $(ODIR)/*.o $(EXE)

install:
	cp $(EXE) ../.
