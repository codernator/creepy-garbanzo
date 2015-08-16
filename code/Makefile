IDIR=include
ODIR=obj
BDIR=bin
CC= /usr/bin/gcc
PROF= -DLINUX
C_FLAGS= -O2 -g2 -Wall -pedantic -std=gnu99 -I$(IDIR)
L_FLAGS=  $(PROF)
LIBS=-lm

SDIR=src
_DEPS=merc.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS)) $(SYSINTERNALS)
EXE=$(BDIR)/badtrip
_OBJ=act_comm.o act_combat.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     affects.o alias.o at.o auction.o \
     ban.o bank.o bit.o \
     character.o combat_roll.o comm.o const.o \
     db.o disable.o descriptor.o \
     effects.o enchant.o \
     fight.o find.o flags.o for.o \
     handler.o healer.o help.o \
     imprint.o interp.o \
     libfile.o logging.o lookup.o \
     magic.o mem.o mob_cmds.o mob_prog.o mobs.o \
     nanny.o nickname.o note.o \
     ovnum.o \
     race.o random.o recycle.o repair.o restring.o \
     save.o scan.o set.o skills.o socketio.o spells.o stat.o \
     tables.o teleport.o temp.o \
     unique.o update.o variant.o who.o \
     olc/olc.o olc/olc_areas.o olc/olc_groups.o olc/olc_helps.o olc/olc_mobs.o \
     olc/olc_objects.o olc/olc_progs.o olc/olc_rooms.o olc/olc_save.o olc/olc_skills.o  \
     olc/string.o \
	 services/channels.o services/character_service.o services/follower_service.o services/object_service.o

OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(EXE): $(OBJ) $(DEPS)
	$(CC) $(L_FLAGS) -o $@ $(OBJ) $(BDIR)/sysinternals.a $(LIBS) -I$(IDIR)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(C_FLAGS)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(ODIR)/services/*.o $(ODIR)/olc/*.o  $(EXE)

.PHONY: install
install:
	cp $(EXE) ../.
