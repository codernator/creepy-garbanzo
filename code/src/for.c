/***************************************************************************
*	BadTrip original code.
*	1999-2000 by Joe Hall
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"



/***************************************************************************
*	declarations
***************************************************************************/
typedef void FOR_CMD (CHAR_DATA *ch, char *name, char *argument);

static void for_all(CHAR_DATA * ch, char *name, char *argument);
static void for_gods(CHAR_DATA * ch, char *name, char *argument);
static void for_morts(CHAR_DATA * ch, char *name, char *argument);
static void for_room(CHAR_DATA * ch, char *name, char *argument);
static void for_name(CHAR_DATA * ch, char *name, char *argument);
static void for_count(CHAR_DATA * ch, char *name, char *argument);

static const struct for_cmds {
	char *		cmd;
	FOR_CMD *	fn;
} fcmd_table [] = {
	{ "all",     for_all	},
	{ "gods",    for_gods	},
	{ "mortals", for_morts	},
	{ "room",    for_room	},
	{ NULL,	     NULL	}
};

/***************************************************************************
*	utility functions
***************************************************************************/
bool vaild_cmd(CHAR_DATA * vcn, char *cmd);
bool expand_cmd(CHAR_DATA * vch, char *arg, char *buf, char find);
static const char *get_name(CHAR_DATA * vch);

#define MAX_TARGETS             100
#define FOR_WAIT                36

/***************************************************************************
*	invalid commands
***************************************************************************/
static const char *invalid_cmds[] =
{
	"quit", "for", "fry", "ffry", "string", "disconnect", "purge", NULL
};

/***************************************************************************
*	valid_cmd
*
*	determines if the command intered is valid
***************************************************************************/
static bool valid_cmd(CHAR_DATA *vch, char *cmd)
{
	const CMD *cmd_base;
	bool success = true;
	int iter;

	if ((cmd_base = cmd_lookup(vch, cmd)) != NULL) {
		for (iter = 0; invalid_cmds[iter] != NULL; iter++) {
			if (!str_cmp(invalid_cmds[iter], cmd_base->name)) {
				success = false;
				break;
			}
		}
	}

	return success;
}

/***************************************************************************
*	do_for
*
*	entry point for the "for" command
***************************************************************************/
void do_for(CHAR_DATA *ch, char *argument)
{
	char name[MIL];
	int iter;

	argument = one_argument(argument, name);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (!name[0] || !argument[0]) {
		do_help(ch, "immortal_for");
		return;
	}

	if (!valid_cmd(ch, argument)) {
		send_to_char("The command you have entered is not safe for "
			     "the `2FOR`` command.\n\r", ch);
		return;
	}

	for (iter = 0; fcmd_table[iter].cmd != NULL; iter++) {
		if (!str_prefix(name, fcmd_table[iter].cmd)) {
			(*fcmd_table[iter].fn)(ch, name, argument);

			if (ch->level < MAX_LEVEL)
				WAIT_STATE(ch, FOR_WAIT);
			return;
		}
	}


	if (is_number(name)) {
		for_count(ch, name, argument);
	} else if (strlen(name) > 1) {
		for_name(ch, name, argument);
	} else {
		do_help(ch, "for");
		return;
	}

	if (ch->level < MAX_LEVEL)
		WAIT_STATE(ch, FOR_WAIT);

	return;
}



/***************************************************************************
*	for_all
*
*	for all players -- gods and mortals
***************************************************************************/
static void for_all(CHAR_DATA *ch, char *name, char *argument)
{
	for_gods(ch, name, argument);
	for_morts(ch, name, argument);
}


/***************************************************************************
*	for_gods
*
*	for all imms
***************************************************************************/
static void for_gods(CHAR_DATA *ch, char *name, char *argument)
{
	DESCRIPTOR_DATA *d;
	ROOM_INDEX_DATA *origin;
	AREA_DATA *area;
	char cmd[MSL];
	int count = 0;
	char check[MIL];


	origin = ch->in_room;
	area = NULL;
	one_argument(argument, check);
	if (!str_prefix(check, "area")) {
		argument = one_argument(argument, check);
		area = ch->in_room->area;
	}

	for (d = globalSystemState.descriptor_head; d != NULL; d = descriptor_playing_iterator(d)) {
		if (d->character != ch
		    && IS_IMMORTAL(d->character)
		    && d->character->in_room != NULL
		    && !room_is_private(d->character->in_room)
		    && (area == NULL || d->character->in_room->area == area)) {
			if (++count > MAX_TARGETS) {
				send_to_char("Maximum number of targets exceeded.\n\r", ch);
				break;
			}

			char_from_room(ch);
			char_to_room(ch, d->character->in_room);

			if (expand_cmd(d->character, argument, cmd, '#'))
				interpret(ch, cmd);
		}
	}

	char_from_room(ch);
	char_to_room(ch, origin);
}


/***************************************************************************
*	for_morts
*
*	for all morts
***************************************************************************/
static void for_morts(CHAR_DATA *ch, char *name, char *argument)
{
	DESCRIPTOR_DATA *d;
	ROOM_INDEX_DATA *origin;
	AREA_DATA *area;
	char cmd[MSL];
	int count = 0;
	char check[MIL];

	origin = ch->in_room;
	area = NULL;
	one_argument(argument, check);
	if (!str_prefix(check, "area")) {
		argument = one_argument(argument, check);
		area = ch->in_room->area;
	}

	for (d = globalSystemState.descriptor_head; d != NULL; d = descriptor_playing_iterator(d)) {
		if (d->character != NULL
		    && d->character != ch
		    && !IS_IMMORTAL(d->character)
		    && d->character->in_room != NULL
		    && !room_is_private(d->character->in_room)
		    && (area == NULL || d->character->in_room->area == area)) {
			if (++count > MAX_TARGETS) {
				send_to_char("Maximum number of targets exceeded.\n\r", ch);
				break;
			}

			char_from_room(ch);
			char_to_room(ch, d->character->in_room);

			if (expand_cmd(d->character, argument, cmd, '#'))
				interpret(ch, cmd);
		}
	}

	char_from_room(ch);
	char_to_room(ch, origin);
}


/***************************************************************************
*	for_room
*
*	for each person in the room - affect mobiles
***************************************************************************/
static void for_room(CHAR_DATA *ch, char *name, char *argument)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	char cmd[MSL];
	int count = 0;

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;;

		if (vch != ch) {
			if (++count > MAX_TARGETS) {
				send_to_char("Maximum number of targets exceeded.\n\r", ch);
				break;
			}

			if (expand_cmd(vch, argument, cmd, '#'))
				interpret(ch, cmd);
		}
	}
}


/***************************************************************************
*	for_name
*
*	for each character with a given name -- PC and NPCs
***************************************************************************/
static void for_name(CHAR_DATA *ch, char *name, char *argument)
{
	ROOM_INDEX_DATA *origin;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	AREA_DATA *area;
	char cmd[MSL];
	int count = 0;
	char check[MIL];

	origin = ch->in_room;
	area = NULL;
	one_argument(argument, check);
	if (!str_prefix(check, "area")) {
		argument = one_argument(argument, check);
		area = ch->in_room->area;
	}

	for (vch = char_list; vch != NULL; vch = vch_next) {
		vch_next = vch->next;

		if (is_name(name, vch->name)
		    && vch != ch
		    && !room_is_private(vch->in_room)
		    && (area == NULL || vch->in_room->area == area)) {
			if (++count > MAX_TARGETS) {
				send_to_char("Maximum number of targets exceeded.\n\r", ch);
				break;
			}

			char_from_room(ch);
			char_to_room(ch, vch->in_room);

			if (expand_cmd(vch, argument, cmd, '#'))
				interpret(ch, cmd);
		}
	}

	char_from_room(ch);
	char_to_room(ch, origin);
}




/***************************************************************************
*	for_count
*
*	do a command parse_int(name) times
***************************************************************************/
static void for_count(CHAR_DATA *ch, char *name, char *argument)
{
	int count = 0;
	int number;
	int iter;

	if (is_number(name)) {
		number = parse_int(name);
		if (number > 201) {
			send_to_char("There is a maximum of 200 targets when using the for command.\n\r", ch);
			return;
		}
		if (number > 0) {
			for (iter = 0; iter < number; iter++) {
				if (++count > MAX_TARGETS) {
					send_to_char("Maximum number of targets exceeded.\n\r", ch);
					break;
				}

				interpret(ch, argument);
			}
		}
	}
}

/***************************************************************************
*	expand_cmd
*
*	expands any special characters(defined by "find") in a
*	command to a valid target
***************************************************************************/
bool expand_cmd(CHAR_DATA *vch, char *arg, char *buf, char find)
{
	const char *name = get_name(vch);
	char *orig = arg;
	char *dest = buf;
	int len = 0;

	*dest = '\0';
	while (*orig != '\0') {
		if (++len >= MSL)
			break;

		if (*orig == find) {
			const char *tmp = name;
			while (*tmp != '\0')
				*dest++ = *tmp++;
			orig++;
		} else {
			*dest++ = *orig++;
		}
	}

	*dest = '\0';
	return len < MSL;
}


/***************************************************************************
*	get_name
*
*	gets the proper name(with #.xxx notation) for a character
***************************************************************************/
static const char *get_name(CHAR_DATA *vch)
{
	CHAR_DATA *rch;
	char name[MIL];
	static char outbuf[MIL];


	if (!IS_NPC(vch))
		return vch->name;

	one_argument(vch->name, name);
	if (!name[0]) {
		strcpy(outbuf, "");
		return outbuf;
	} else {
		int count = 1;

		for (rch = vch->in_room->people; rch && (rch != vch); rch = rch->next_in_room)
			if (is_name(name, rch->name))
				count++;

		sprintf(outbuf, "%d.%s", count, name);
	}

	return outbuf;
}
