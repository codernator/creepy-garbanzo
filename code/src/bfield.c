#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "magic.h"


#include <stdio.h>

/***************************************************************************
*	definitions
***************************************************************************/
#define BATTLEFIELD_ROOM_NORMAL_L       800
#define BATTLEFIELD_ROOM_NORMAL_U       891
#define BATTLEFIELD_ROOM_CHAOS_L        800
#define BATTLEFIELD_ROOM_CHAOS_U        891

static struct battlefield_data battlefield;


/***************************************************************************
*	set definitions
***************************************************************************/
typedef bool BATTLEFIELD_SET_FN (CHAR_DATA *ch, char *arg);

BATTLEFIELD_SET_FN battlefield_set_llevel;
BATTLEFIELD_SET_FN battlefield_set_ulevel;
BATTLEFIELD_SET_FN battlefield_set_disabled;
BATTLEFIELD_SET_FN battlefield_set_special;
BATTLEFIELD_SET_FN battlefield_set_openticks;

struct battlefield_set_data {
	char *			cmd;
	BATTLEFIELD_SET_FN *	fn;
};


const struct battlefield_set_data battlefield_set_cmds[] =
{
	{ "llevel",    battlefield_set_llevel	 },
	{ "ulevel",    battlefield_set_ulevel	 },
	{ "disabled",  battlefield_set_disabled	 },
	{ "special",   battlefield_set_special	 },
	{ "openticks", battlefield_set_openticks },
	{ NULL,	       NULL			 }
};


/***************************************************************************
*	function declarations
***************************************************************************/
int battlefield_count(void);
bool in_battlefield(CHAR_DATA * ch);
bool battlefield_check_disabled(CHAR_DATA * ch, int type, char *name);
void battlefield_notify(char *buf);
int battlefield_participants(void);
void battlefield_update(void);
void battlefield_clear(void);

static void battlefield_set(CHAR_DATA * ch, char *arg);
static void battlefield_show(CHAR_DATA * ch);
static void battlefield_trans(CHAR_DATA * ch, ROOM_INDEX_DATA * loc);
static void battlefield_open(CHAR_DATA * ch);
static void battlefield_close(CHAR_DATA * ch);
static void battlefield_cancel(CHAR_DATA * ch);
static void battlefield_enter(CHAR_DATA * ch);
static void battlefield_special(CHAR_DATA * ch);


/***************************************************************************
*	do_battlefield
***************************************************************************/
void do_battlefield(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (IS_NPC(ch)) {
		send_to_char("No can do.\n\r", ch);
		return;
	}

	if (arg[0] != '\0' && !str_prefix(arg, "on")) {
		send_to_char("The `#Battlefield`` channel is now `@ON``.\n\r", ch);
		REMOVE_BIT(ch->comm, COMM_NOBATTLEFIELD);
		return;
	}

	if (arg[0] != '\0' && !str_prefix(arg, "off")) {
		send_to_char("The `#Battlefield`` channel is now `!OFF``.\n\r", ch);
		SET_BIT(ch->comm, COMM_NOBATTLEFIELD);
		return;
	}

	if (arg[0] == '\0' || !str_prefix(arg, "show")) {
		battlefield_show(ch);
		return;
	}

	if (battlefield.open && !IS_IMMORTAL(ch)) {
		battlefield_enter(ch);
		return;
	}


	if (!IS_IMMORTAL(ch))
		return;

	if (arg[0] == '?' || !str_cmp(arg, "help")) {
		do_help(ch, "battlefield_syntax");
		return;
	}

	if (!str_prefix(arg, "open")) {
		battlefield_open(ch);
		return;
	}

	if (!str_prefix(arg, "close")) {
		battlefield_close(ch);
		return;
	}

	if (!str_prefix(arg, "cancel")) {
		battlefield_cancel(ch);
		return;
	}

	if (!str_prefix(arg, "set"))
		argument = one_argument(argument, arg);

	battlefield_set(ch, argument);
}


/***************************************************************************
*	battlefield_trans
*
*	trans a character to a location in the battlefield
*	given a vnum
***************************************************************************/
void battlefield_trans(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
	if (room == NULL) {
		send_to_char("Battlefield transfer failed.\n\r", ch);
		return;
	}

	if (IS_NPC(ch)) {
		send_to_char("You're not good enough to enter the field.\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		send_to_char("Wouldn't you rather look for your master insted?\n\r", ch);
		return;
	}

	if (ch->level < battlefield.llevel) {
		send_to_char("You are not high enough level to enter this battlefield..\n\r", ch);
		return;
	}

	if (ch->level > battlefield.ulevel && !IS_IMMORTAL(ch)) {
		send_to_char("Your level is too high to enter this battlefield..\n\r", ch);
		return;
	}

	char_from_room(ch);
	char_to_room(ch, room);
	do_look(ch, "auto");
}


/***************************************************************************
*	battlefield_open
*
*	open the battlefield
***************************************************************************/
void battlefield_open(CHAR_DATA *ch)
{
	char buf[MSL];

	if (!battlefield.dirty) {
		battlefield_clear();
		battlefield.dirty = TRUE;
	}

	if (battlefield.open) {
		if (ch != NULL)
			send_to_char("The `#Battlefield`7 is already open ..\n\r", ch);
		return;
	}

	if (battlefield_count() > 0) {
		send_to_char("The `#Battlefield`7 is still running...wait for it to clear.\n\r", ch);
		return;
	}

	battlefield.open = TRUE;
	if (ch != NULL)
		sprintf(log_buf, "%s opened the battlefield ..", ch->name);
	else
		sprintf(log_buf, "The battlefield has been closed ..");

	log_string(log_buf);

	if (ch != NULL && !IS_NPC(ch) && IS_IMMORTAL(ch))
		battlefield.immortal_opens++;
	else
		battlefield.mortal_opens++;

	send_to_char("The field is now open ..\n\r", ch);
	if (battlefield.llevel != LEVEL_NEWBIE || battlefield.ulevel != LEVEL_HERO) {
		sprintf(buf, "%s has opened the `#Battlefield`7 for levels `!%d`` - `!%d``! ..\n\r",
			(IS_NPC(ch)) ? ch->short_descr : ch->name,
			battlefield.llevel, battlefield.ulevel);
	} else {
		sprintf(buf, "%s has opened the `#Battlefield`7! ..\n\r", (IS_NPC(ch)) ? ch->short_descr : ch->name);
	}
	battlefield_notify(buf);

	if (battlefield.open_ticks > 0) {
		sprintf(buf, "The `#Battlefield`` will close in %d ticks.\n\r", battlefield.open_ticks);
		battlefield_notify(buf);
	}

	battlefield.opened_by[0] = '\0';
	sprintf(battlefield.opened_by, "%s", IS_NPC(ch) ? ch->short_descr : ch->name);
}


/***************************************************************************
*	battlefield_close
*
*	close the battlefield
***************************************************************************/
void battlefield_close(CHAR_DATA *ch)
{
	DESCRIPTOR_DATA *d;
	ROOM_INDEX_DATA *room;
	char buf[MSL];
	int randvnum;


	if (!battlefield.open && ch != NULL) {
		send_to_char("The `#Battlefield`7 isn't open ..\n\r", ch);
		return;
	}

	battlefield.open = FALSE;
	battlefield.dirty = FALSE;
	battlefield.running_ticks = 0;

	if (ch != NULL) {
		sprintf(log_buf, "%s has closed the battlefield ..", ch->name);
		sprintf(buf, "%s has closed the `#Battlefield`7 ..\n\r", ch->name);

		send_to_char("The field is now closed ..\n\r", ch);
	} else {
		sprintf(log_buf, "The battlefield has been closed ..\n\r");
		sprintf(buf, "The `#Battlefield`7 has been closed ..\n\r");
	}
	log_string(log_buf);
	battlefield_notify(buf);

	battlefield_special(ch);

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING)
			continue;

		wch = CH(d);

		if (wch->in_room == get_room_index(ROOM_VNUM_WARPREP)) {
			room = NULL;
			while (room == NULL) {
				randvnum = number_range(battlefield.lroom, battlefield.uroom);
				room = get_room_index(randvnum);
			}

			if (!IS_SET(wch->act, PLR_BATTLE))
				SET_BIT(wch->act, PLR_BATTLE);
			battlefield_trans(wch, room);
		}
	}

	battlefield.participants = battlefield_count();
}


/***************************************************************************
*	battlefield_cancel
*
*	cancel the battlefield
***************************************************************************/
void battlefield_cancel(CHAR_DATA *ch)
{
	DESCRIPTOR_DATA *d;
	char buf[MSL];

	if (!battlefield.open) {
		send_to_char("The `#Battlefield`7 is already closed ..\n\r", ch);
		return;
	}

	sprintf(log_buf, "%s has canceled the battle ..", ch->name);
	log_string(log_buf);

	send_to_char("The field is now closed ..\n\r", ch);
	sprintf(buf, "%s has canceled the `#Battlefield`7 ..\n\r", ch->name);
	battlefield_notify(buf);

	battlefield.open = FALSE;
	battlefield.dirty = FALSE;

	battlefield_clear();

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING)
			continue;

		wch = CH(d);

		if (IS_SET(wch->act, PLR_BATTLE) && (in_battlefield(wch) || wch->in_room == get_room_index(ROOM_VNUM_WARPREP))) {
			REMOVE_BIT(wch->act, PLR_BATTLE);
			wch->benter--;
			char_from_room(wch);
			char_to_room(wch, get_room_index(ROOM_VNUM_ALTAR));
			return;
		}
	}
}


/***************************************************************************
*	battlefield_enter
*
*	enters a character in the battlefield
***************************************************************************/
void battlefield_enter(CHAR_DATA *ch)
{
	char buf[MAX_STRING_LENGTH];

	if (!battlefield.open) {
		send_to_char("The `#Battlefield`7 is not open at the moment ..\n\r", ch);
		return;
	}


	if (ch->level < battlefield.llevel) {
		send_to_char("Your level is not high enough to enter this battlefield.\n\r", ch);
		return;
	}

	if (ch->level > battlefield.ulevel) {
		send_to_char("Your level is too high to enter this battlefield.\n\r", ch);
		return;
	}

	if (ch->in_room == get_room_index(ROOM_VNUM_WARPREP)) {
		send_to_char("But you are already there!\n\r", ch);
		return;
	}

	SET_BIT(ch->act, PLR_BATTLE);
	act("$n goes off to the battlefield ..", ch, NULL, NULL, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, get_room_index(ROOM_VNUM_WARPREP));
	act("$n arrives ready for battle ..", ch, NULL, NULL, TO_ROOM);
	do_look(ch, "auto");

	ch->benter++;

	battlefield.participants++;

	sprintf(buf, "Battlefield Update: `#%s`7 has entered the field.\n\r", ch->name);
	battlefield_notify(buf);

	restore_char(ch);
	return;
}


/***************************************************************************
*	battlefield_clear
*
*	clears the data entered into the field
***************************************************************************/
void battlefield_clear()
{
	battlefield.lroom = BATTLEFIELD_ROOM_NORMAL_L;
	battlefield.uroom = BATTLEFIELD_ROOM_NORMAL_U;
	battlefield.llevel = LEVEL_NEWBIE;
	battlefield.ulevel = LEVEL_HERO;
	battlefield.dirty = FALSE;
	battlefield.open = FALSE;
	battlefield.participants = 0;
	battlefield.special = FALSE;
	battlefield.open_ticks = -1;
	battlefield.running_ticks = -1;

	if (battlefield.disabled != NULL) {
		DISABLED_DATA *disabled;
		DISABLED_DATA *disabled_next;

		for (disabled = battlefield.disabled; disabled != NULL; disabled = disabled_next) {
			disabled_next = disabled->next;
			free_disabled(disabled);
		}

		battlefield.disabled = NULL;
	}
}


/***************************************************************************
*	battlefield_show
*
*	shows the options selected for the field
***************************************************************************/
void battlefield_show(CHAR_DATA *ch)
{
	int count;

	count = battlefield_count();

	if (!IS_IMMORTAL(ch) && in_battlefield(ch))
		return;

	send_to_char("\n\r`#Battlefield`` Stats:\n\r", ch);
	send_to_char("`1==============================================================================``\n\r", ch);

	if (battlefield.open) {
		send_to_char("status:                `@open``.\n\r", ch);
		printf_to_char(ch, "opened by:             %s\n\r", battlefield.opened_by);
		printf_to_char(ch, "open ticks:            %d\n\r", battlefield.open_ticks);
	} else if (count > 0) {
		send_to_char("status:                `#running``.\n\r", ch);
		printf_to_char(ch, "running for:           %d `#ticks``\n\r", battlefield.running_ticks);
	} else {
		send_to_char("status:                `1closed``.\n\r", ch);
	}

	printf_to_char(ch, "# opened by imms:      %d\n\r", battlefield.immortal_opens);
	printf_to_char(ch, "# opened by mortals:   %d\n\r", battlefield.mortal_opens);

	if (battlefield.llevel > LEVEL_NEWBIE || battlefield.ulevel < LEVEL_HERO) {
		printf_to_char(ch, "level range:           %d - %d\n\r",
			       battlefield.llevel,
			       battlefield.ulevel);
	} else {
		send_to_char("level range:           all\n\r", ch);
	}

	printf_to_char(ch, "specials:              %s\n\r", (battlefield.special) ? "`@ON``" : "`!OFF``");

	if (battlefield.open || count > 0) {
		send_to_char("\n\r", ch);
		printf_to_char(ch, "participants:          %d\n\r", battlefield.participants);

		printf_to_char(ch, "left in field:         %d\n\r", count);
	}

	send_to_char("`1==============================================================================``\n\r", ch);

	disable_show(ch, battlefield.disabled);

	send_to_char("`1==============================================================================``\n\r", ch);

	if (count > 0) {
		DESCRIPTOR_DATA *d;
		CHAR_DATA *bch;

		send_to_char("\n\rPlayer information:\n\r"
			     "Name         Room    Enters Kills Deaths  Hp    Mana   Position\n\r", ch);
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->connected != CON_PLAYING)
				continue;

			bch = CH(d);

			if (IS_SET(bch->act, PLR_BATTLE)
			    && in_battlefield(bch)) {
				printf_to_char(ch, "%-12.11s %5.5d     %3d   %3d   %3d    %3d%%  %3d%%   %-11.10s `!%s``\n\r",
					       bch->name,
					       bch->in_room->vnum,
					       bch->benter,
					       bch->bkills,
					       bch->bloss,
					       (bch->hit > 0) ? (bch->hit * 100) / bch->max_hit : 0,
					       (bch->mana > 0) ? (bch->mana * 100) / bch->max_mana : 0,
					       capitalize(position_table[bch->position].name),
					       (bch->position == POS_FIGHTING) ? bch->fighting->name : "");
			}
		}
	}
}


/***************************************************************************
*	battlefield_special
*
*	sets up the battlefield with special affects...
***************************************************************************/
void battlefield_special(CHAR_DATA *ch)
{
	ROOM_INDEX_DATA *room;
	ROOM_INDEX_DATA *src;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
	int iter;

	if (ch == NULL)
		return;

	src = ch->in_room;
	battlefield.affected = FALSE;
	for (iter = battlefield.lroom; iter < battlefield.uroom; iter++) {
		room = get_room_index(iter);
		if (room != NULL) {
			if (room->affected) {
				for (paf = room->affected; paf != NULL; paf = paf_next) {
					paf_next = paf->next;
					affect_remove_room(room, paf);
				}
			}

			if (battlefield.special && number_range(0, 1) == 0) {
				AFFECT_DATA af;
				SKILL *skill;
				int rand;
				int level;
				int duration;

				char_from_room(ch);
				char_to_room(ch, room);

				rand = number_range(0, 15);
				duration = -1;

				switch (rand) {
				default:
					skill = skill_lookup("mana vortex");
					level = battlefield.ulevel;
					break;
				case 0:
				case 4:
					skill = skill_lookup("displacement");
					level = (int)UMAX((battlefield.ulevel * 6) / 5, 300);
					break;
				case 5:
				case 7:
					skill = skill_lookup("haven");
					level = (int)battlefield.ulevel;
					duration = (int)number_range(10, 15);
					break;
				case 8:
				case 9:
				case 12:
					skill = skill_lookup("parasitic cloud");
					level = battlefield.ulevel;
					break;
				}


				if (skill != NULL) {
					af.where = TO_AFFECTS;
					af.duration = duration;
					af.location = 0;
					af.modifier = 0;
					af.bitvector = 0;
					af.type = skill->sn;
					af.skill = skill;
					af.level = level;

					battlefield.affected = TRUE;

					affect_to_room(room, &af);
				}
			}
		}
	}

	char_from_room(ch);
	char_to_room(ch, src);
}


/***************************************************************************
*	battlefield_set
*
*	set options for the battlefield
***************************************************************************/
void battlefield_set(CHAR_DATA *ch, char *argument)
{
	char arg[MSL];
	int iter;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		do_help(ch, "battlefield_set_syntax");
		return;
	}

	if (!battlefield.dirty && battlefield_count() <= 0)
		battlefield_clear();

	for (iter = 0; battlefield_set_cmds[iter].cmd != NULL; iter++) {
		if (!str_prefix(arg, battlefield_set_cmds[iter].cmd)) {
			if ((*battlefield_set_cmds[iter].fn)(ch, argument))
				battlefield.dirty = TRUE;
			break;
		}
	}
}




/***************************************************************************
*	battlefield_set_llevel
*
*	sets the lower-level range for the battlefield
***************************************************************************/
bool battlefield_set_llevel(CHAR_DATA *ch, char *arg)
{
	bool success = FALSE;
	int level;

	if (battlefield_count() > 0) {
		send_to_char("There are still people in the battlefield.\n\r", ch);
	} else if (battlefield.open) {
		send_to_char("The battlefield is already open.\n\r", ch);
	} else {
		if (arg[0] != '\0' && is_number(arg)) {
			level = parse_int(arg);

			if (level > battlefield.ulevel) {
				send_to_char("The lower level must be lower than the upper level.\n\r", ch);
			} else {
				send_to_char("Lower level set.\n\r", ch);
				battlefield.llevel = level;
				success = TRUE;
			}
		}
	}

	return success;
}


/***************************************************************************
*	battlefield_set_ulevel
*
*	sets the upper-level range for the battlefield
***************************************************************************/
bool battlefield_set_ulevel(CHAR_DATA *ch, char *arg)
{
	bool success = FALSE;
	int level;

	if (battlefield_count() > 0) {
		send_to_char("There are still people in the battlefield.\n\r", ch);
	} else if (battlefield.open) {
		send_to_char("The battlefield is already open.\n\r", ch);
	} else {
		if (arg[0] != '\0' && is_number(arg)) {
			level = parse_int(arg);

			if (level < battlefield.llevel) {
				send_to_char("The upper level must be higher than the lower level.\n\r", ch);
			} else {
				send_to_char("Upper level set.\n\r", ch);
				battlefield.ulevel = level;
				success = TRUE;
			}
		}
	}

	return success;
}


/***************************************************************************
*	battlefield_set_special
*
*	sets the special bit for the battlefield
***************************************************************************/
bool battlefield_set_special(CHAR_DATA *ch, char *arg)
{
	bool success = FALSE;
	bool set = FALSE;

	if (battlefield_count() > 0) {
		send_to_char("There are still people in the battlefield.\n\r", ch);
	} else {
		if (arg[0] == '\0') {
			if (!battlefield.special)
				set = TRUE;
		} else if (!str_prefix(arg, "on") || !str_prefix(arg, "true")) {
			set = TRUE;
		}

		printf_to_char(ch, "Battlefield special %s\n\r", (set) ? "set" : "removed");
		battlefield.special = set;
		success = TRUE;
	}

	return success;
}


/***************************************************************************
*	battlefield_set_openticks
*
*	sets the number of open ticks for the battlefield
***************************************************************************/
bool battlefield_set_openticks(CHAR_DATA *ch, char *arg)
{
	bool success = FALSE;

	if (battlefield_count() > 0) {
		send_to_char("There are still people in the battlefield.\n\r", ch);
	} else {
		int count;

		count = parse_int(arg);
		if (count > 0) {
			printf_to_char(ch, "Battlefield open ticks set: %d\n\r", count);
			battlefield.open_ticks = count;
		} else {
			printf_to_char(ch, "Battlefield open ticks cleared.\n\r");
			battlefield.open_ticks = -1;
		}

		success = TRUE;
	}

	return success;
}


/***************************************************************************
*	battlefield_set_disabled
*
*	sets disabled commands/spells for the battlefield
***************************************************************************/
bool battlefield_set_disabled(CHAR_DATA *ch, char *arg)
{
	disable_all(ch, arg, &battlefield.disabled);

	return FALSE;
}




/***************************************************************************
*	battlefield_count
*
*	counts the number of characters in the battlefield
***************************************************************************/
int battlefield_count()
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *ch;
	int count = 0;

	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected != CON_PLAYING)
			continue;

		ch = CH(d);

		if (IS_SET(ch->act, PLR_BATTLE)
		    && in_battlefield(ch))
			count++;
	}

	return count;
}


/***************************************************************************
*	battlefield_participants
*
*	the total number of characters that entered the battlefield
***************************************************************************/
int battlefield_participants()
{
	return battlefield.participants;
}


/***************************************************************************
*	in_battlefield
*
*	determines if a character is in the battlefield
***************************************************************************/
bool in_battlefield(CHAR_DATA *ch)
{
	if (ch->in_room->vnum >= battlefield.lroom
	    && ch->in_room->vnum <= battlefield.uroom)
		return TRUE;

	return FALSE;
}


/***************************************************************************
*	battlefield_check_disabled
*
*	checks to see if a battlefield commands/spells is disabled
***************************************************************************/
bool battlefield_check_disabled(CHAR_DATA *ch, int type, char *name)
{
	DISABLED_DATA *disabled;
	bool is_disabled = FALSE;

	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_BATTLE)
	    && (in_battlefield(ch) || ch->in_room->vnum == ROOM_VNUM_WARPREP)) {
		for (disabled = battlefield.disabled; disabled != NULL; disabled = disabled->next) {
			if (disabled->type == type
			    && !str_cmp(name, disabled->command)) {
				is_disabled = TRUE;
				break;
			}
		}
	}

	return is_disabled;
}


/***************************************************************************
*	battlefield_notify
*
*	checks to see if a battlefield commands/spells is disabled
***************************************************************************/
void battlefield_notify(char *msg)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *tch;

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING) {
			tch = CH(d);
			if (!IS_NPC(tch) && !IS_SET(tch->comm, COMM_NOBATTLEFIELD))
				send_to_char(msg, d->character);
		}
	}

	return;
}



/***************************************************************************
*	battlefield_update
*
*	update the battlefield statistics
***************************************************************************/
void battlefield_update()
{
	int count;

	count = battlefield_count();
	if (battlefield.open && battlefield.open_ticks > 0) {
		if (--battlefield.open_ticks == 0)
			battlefield_close(NULL);
		if (battlefield.open_ticks > 0) {
			char buf[MSL];

			sprintf(buf, "`#Battlefield Update``: Closing in `1>`# %d `1<`` ticks.\n\r", battlefield.open_ticks);
			battlefield_notify(buf);
		}
	}

	if (count > 0) {
		if ((++battlefield.running_ticks % 2) == 0) {
			char buf[MSL];

			sprintf(buf, "`#Battlefield Update``: `1>`#* `!%d `#*`1<`` people still left in the `#Battlefield``.\n\r", count);
			battlefield_notify(buf);
		}
	} else {
		ROOM_INDEX_DATA *room;
		AFFECT_DATA *paf;
		AFFECT_DATA *paf_next;
		int iter;

		if (battlefield.affected) {
			for (iter = battlefield.lroom; iter < battlefield.uroom; iter++) {
				room = get_room_index(iter);
				if (room != NULL) {
					if (room->affected) {
						for (paf = room->affected; paf != NULL; paf = paf_next) {
							paf_next = paf->next;
							affect_remove_room(room, paf);
						}
					}
				}
			}
		}
	}

	return;
}


/***************************************************************************
*	battlefield_is_open
*
*	checks to see if the battlefield is open
***************************************************************************/
bool battlefield_is_open()
{
	return battlefield.open;
}
