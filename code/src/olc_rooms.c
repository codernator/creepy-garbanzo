/***************************************************************************
*  File: olc_redit.c                                                      *
*                                                                         *
*  Much time and thought has gone into this software and you are          *
*  benefitting.  We hope that you share your changes too.  What goes      *
*  around, comes around.                                                  *
*                                                                         *
*  This code was freely distributed with the The Isles 1.1 source code,   *
*  and has been used here for OLC - OLC would not be what it is without   *
*  all the previous coders who released their source code.                *
*                                                                         *
***************************************************************************/


/***************************************************************************
*	includes
***************************************************************************/
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"
#include "interp.h"

/***************************************************************************
*	external functions
***************************************************************************/
extern int wear_bit(int loc);
extern char *flag_string(const struct flag_type *flag_table, long bits);
extern int flag_value(const struct flag_type *flag_table, char *argument);


/***************************************************************************
*	change_exit
*
*	change the values of an exit
***************************************************************************/
static bool change_exit(CHAR_DATA *ch, char *argument, int door)
{
	ROOM_INDEX_DATA *room;
	char command[MIL];
	char arg[MIL];
	int value;

	EDIT_ROOM(ch, room);

	/* set the exit flags - needs full argument */
	if ((value = flag_value(exit_flags, argument)) != NO_FLAG) {
		ROOM_INDEX_DATA *pToRoom;
		int rev;

		if (!room->exit[door]) {
			send_to_char("Exit does not exist.\n\r", ch);
			return FALSE;
		}

		/* this room */
		TOGGLE_BIT(room->exit[door]->rs_flags, value);
		room->exit[door]->exit_info = room->exit[door]->rs_flags;

		/* connected room */
		pToRoom = room->exit[door]->u1.to_room;
		rev = rev_dir[door];

		if (pToRoom->exit[rev] != NULL) {
			pToRoom->exit[rev]->rs_flags = room->exit[door]->rs_flags;
			pToRoom->exit[rev]->exit_info = room->exit[door]->exit_info;
		}

		send_to_char("Exit flag toggled.\n\r", ch);
		return TRUE;
	}

	/* parse the arguments */
	argument = one_argument(argument, command);
	one_argument(argument, arg);

	if (command[0] == '\0' && argument[0] == '\0') {
		move_char(ch, door, TRUE);
		return FALSE;
	}

	if (command[0] == '?') {
		do_help(ch, "OLC_EXIT");
		return FALSE;
	}

	if (!str_cmp(command, "delete")) {
		ROOM_INDEX_DATA *pToRoom;
		int rev;

		if (!room->exit[door]) {
			send_to_char("REdit:  Cannot delete a null exit.\n\r", ch);
			return FALSE;
		}

		/* remove ToRoom exit */
		rev = rev_dir[door];
		pToRoom = room->exit[door]->u1.to_room;

		if (pToRoom->exit[rev]) {
			free_exit(pToRoom->exit[rev]);
			pToRoom->exit[rev] = NULL;
		}

		/* remove this exit */
		free_exit(room->exit[door]);
		room->exit[door] = NULL;

		send_to_char("Exit unlinked.\n\r", ch);
		return TRUE;
	}

	if (!str_cmp(command, "link")) {
		EXIT_DATA *pExit;
		ROOM_INDEX_DATA *toRoom;

		if (arg[0] == '\0' || !is_number(arg)) {
			send_to_char("Syntax:  [direction] link [vnum]\n\r", ch);
			return FALSE;
		}

		value = atoi(arg);

		if (!(toRoom = get_room_index(value))) {
			send_to_char("REdit:  Cannot link to non-existant room.\n\r", ch);
			return FALSE;
		}

		if (!IS_BUILDER(ch, toRoom->area)) {
			send_to_char("REdit:  Cannot link to that area.\n\r", ch);
			return FALSE;
		}

		if (toRoom->exit[rev_dir[door]]) {
			send_to_char("REdit:  Remote side's exit already exists.\n\r", ch);
			return FALSE;
		}

		if (!room->exit[door])
			room->exit[door] = new_exit();

		room->exit[door]->u1.to_room = toRoom;
		room->exit[door]->orig_door = door;

		door = rev_dir[door];
		pExit = new_exit();
		pExit->u1.to_room = room;
		pExit->orig_door = door;
		toRoom->exit[door] = pExit;

		send_to_char("Two-way link established.\n\r", ch);
		return TRUE;
	}

	if (!str_cmp(command, "dig")) {
		char buf[MSL];

		if (arg[0] == '\0' || !is_number(arg)) {
			send_to_char("Syntax: [direction] dig <vnum>\n\r", ch);
			return FALSE;
		}

		redit_create(ch, arg);
		sprintf(buf, "link %s", arg);
		change_exit(ch, buf, door);
		return TRUE;
	}

	if (!str_cmp(command, "room")) {
		ROOM_INDEX_DATA *toRoom;

		if (arg[0] == '\0' || !is_number(arg)) {
			send_to_char("Syntax:  [direction] room [vnum]\n\r", ch);
			return FALSE;
		}
		value = atoi(arg);

		if (!(toRoom = get_room_index(value))) {
			send_to_char("REdit:  Cannot link to non-existant room.\n\r", ch);
			return FALSE;
		}

		if (!room->exit[door])
			room->exit[door] = new_exit();

		room->exit[door]->u1.to_room = toRoom;
		room->exit[door]->orig_door = door;

		send_to_char("One-way link established.\n\r", ch);
		return TRUE;
	}

	if (!str_cmp(command, "key")) {
		OBJ_INDEX_DATA *key;

		if (arg[0] == '\0' || !is_number(arg)) {
			send_to_char("Syntax:  [direction] key [vnum]\n\r", ch);
			return FALSE;
		}

		if (!room->exit[door]) {
			send_to_char("Exit does not exist.\n\r", ch);
			return FALSE;
		}

		value = atoi(arg);

		if (!(key = get_obj_index(value))) {
			send_to_char("REdit:  Key doesn't exist.\n\r", ch);
			return FALSE;
		}

		if (key->item_type != ITEM_KEY) {
			send_to_char("REdit:  Object is not a key.\n\r", ch);
			return FALSE;
		}

		room->exit[door]->key = value;

		send_to_char("Exit key set.\n\r", ch);
		return TRUE;
	}

	if (!str_cmp(command, "name")) {
		if (arg[0] == '\0') {
			send_to_char("Syntax:  [direction] name [string]\n\r", ch);
			send_to_char("         [direction] name none\n\r", ch);
			return FALSE;
		}

		if (!room->exit[door]) {
			send_to_char("Exit does not exist.\n\r", ch);
			return FALSE;
		}

		free_string(room->exit[door]->keyword);

		if (str_cmp(arg, "none"))
			room->exit[door]->keyword = str_dup(arg);
		else
			room->exit[door]->keyword = str_dup("");

		send_to_char("Exit name set.\n\r", ch);
		return TRUE;
	}

	if (!str_prefix(command, "description")) {
		if (arg[0] == '\0') {
			if (!room->exit[door]) {
				send_to_char("Exit does not exist.\n\r", ch);
				return FALSE;
			}

			string_append(ch, &room->exit[door]->description);
			return TRUE;
		}

		send_to_char("Syntax:  [direction] desc\n\r", ch);
		return FALSE;
	}

	return FALSE;
}


/*****************************************************************************
*	do_redit
*
*	entry level function into the room editor
*****************************************************************************/
void do_redit(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *room;
	char arg[MSL];

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg);
	room = ch->in_room;

	if (!str_cmp(arg, "reset")) { /* redit reset */
		if (!IS_BUILDER(ch, room->area)) {
			send_to_char("Insufficient security to edit rooms.\n\r", ch);
			return;
		}

		reset_room(room);
		send_to_char("Room reset.\n\r", ch);

		return;
	} else {
		if (!str_cmp(arg, "create")) {   /* redit create <vnum> */
			if (argument[0] == '\0' || atoi(argument) == 0) {
				send_to_char("Syntax:  edit room create [vnum]\n\r", ch);
				return;
			}

			if (redit_create(ch, argument)) {
				ch->desc->editor = ED_ROOM;
				char_from_room(ch);
				char_to_room(ch, ch->desc->ed_data);
				SET_BIT(((ROOM_INDEX_DATA *)ch->desc->ed_data)->area->area_flags, AREA_CHANGED);
			}

			return;
		} else if (!str_cmp(arg, "clone")) {
			if (argument[0] == '\0') {
				send_to_char("Syntax:  edit room clone [new vnum] [existing vnum]\n\r", ch);
				return;
			}

			if (redit_create(ch, argument)) {
				argument = one_argument(argument, arg);
				ch->desc->editor = ED_ROOM;

				char_from_room(ch);
				char_to_room(ch, ch->desc->ed_data);

				SET_BIT(((ROOM_INDEX_DATA *)ch->desc->ed_data)->area->area_flags, AREA_CHANGED);
				redit_clone(ch, argument);
			}

			return;
		} else if (!IS_NULLSTR(arg)) {  /* redit <vnum> */
			room = get_room_index(atoi(arg));

			if (!room) {
				send_to_char("REdit : room does not exist.\n\r", ch);
				return;
			}

			if (!IS_BUILDER(ch, room->area)) {
				send_to_char("REdit : insufficient security to edit rooms.\n\r", ch);
				return;
			}

			char_from_room(ch);
			char_to_room(ch, room);
		}
	}

	if (!IS_BUILDER(ch, room->area)) {
		send_to_char("REdit : Insufficient security to edit rooms.\n\r", ch);
		return;
	}

	ch->desc->ed_data = (void *)room;
	ch->desc->editor = ED_ROOM;

	return;
}


/***************************************************************************
*	redit functions
***************************************************************************/
/***************************************************************************
*	redit_create
*
*	create a new room
***************************************************************************/
EDIT(redit_create){
	AREA_DATA *area;
	ROOM_INDEX_DATA *room;
	long value;
	long hash_idx;

	EDIT_ROOM(ch, room);
	value = atol(argument);
	if (argument[0] == '\0' || value <= 0) {
		send_to_char("Syntax:  create [vnum > 0]\n\r", ch);
		return FALSE;
	}

	area = get_vnum_area(value);
	if (!area) {
		send_to_char("REdit:  That vnum is not assigned an area.\n\r", ch);
		return FALSE;
	}

	if (!IS_BUILDER(ch, area)) {
		send_to_char("REdit:  Vnum in an area you cannot build in.\n\r", ch);
		return FALSE;
	}

	if (get_room_index(value)) {
		send_to_char("REdit:  Room vnum already exists.\n\r", ch);
		return FALSE;
	}

	room = new_room_index();
	room->area = area;
	room->vnum = value;

	if (value > top_vnum_room)
		top_vnum_room = value;

	hash_idx = value % MAX_KEY_HASH;
	room->next = room_index_hash[hash_idx];
	room_index_hash[hash_idx] = room;
	ch->desc->ed_data = (void *)room;

	send_to_char("Room created.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	redit_clone
*
*	Clone the properties from some room into the builder's current room.
***************************************************************************/
EDIT(redit_clone){
	ROOM_INDEX_DATA *room = NULL;
	ROOM_INDEX_DATA *clone = NULL;
	long room_idx;
	char buf[100];

	EDIT_ROOM(ch, room);
	if (room == NULL) {
		send_to_char("REdit: Cloning a room copies over the room you are in, but you are not in a room.", ch);
		return FALSE;
	}

	room_idx = atol(argument);
	if (argument[0] == '\0') {
		send_to_char("REdit: Syntax:  clone [existing vnum]\n\r", ch);
		return FALSE;
	}

	if ((clone = get_room_index(room_idx)) == NULL) {
		send_to_char("REdit:  Room to clone does not exist.\n\r", ch);
		return FALSE;
	}

	free_string(room->name);
	free_string(room->description);

	room->name = str_dup(clone->name);
	room->description = str_dup(clone->description);
	room->room_flags = clone->room_flags;
	room->light = clone->light;
	room->sector_type = clone->sector_type;
	room->heal_rate = clone->heal_rate;
	room->mana_rate = clone->mana_rate;

	snprintf(buf, 100, "REdit: This room has become a clone of room %ld.\n\r", room_idx);
	send_to_char(buf, ch);
	return TRUE;
}


/***************************************************************************
*	redit_rlist
*
*	display a list of rooms
***************************************************************************/
EDIT(redit_rlist){
	ROOM_INDEX_DATA *room;
	AREA_DATA *area;
	BUFFER *buf;
	char *unclr;
	char arg[MIL];
	bool found;
	long vnum;
	int col = 0;

	(void)one_argument(argument, arg);

	area = ch->in_room->area;
	buf = new_buf();
	found = FALSE;

	for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
		if ((room = get_room_index(vnum))) {
			found = TRUE;

			unclr = uncolor_str(capitalize(room->name));
			printf_buf(buf, "[`1%5d``] %-17.16s", vnum, unclr);
			free_string(unclr);

			if (++col % 3 == 0)
				add_buf(buf, "\n\r");
		}
	}

	if (!found) {
		send_to_char("Room(s) not found in this area.\n\r", ch);
		return FALSE;
	}

	if (col % 3 != 0)
		add_buf(buf, "\n\r");

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return FALSE;
}


/***************************************************************************
*	redit_mlist
*
*	list all of the mobs in an area
***************************************************************************/
EDIT(redit_mlist){
	MOB_INDEX_DATA *mob;
	AREA_DATA *area;
	BUFFER *buf;
	char arg[MIL];
	char *unclr;
	bool all;
	bool found;
	long vnum;
	int col = 0;


	one_argument(argument, arg);
	if (is_help(arg)) {
		send_to_char("Syntax:  mlist <all/name>\n\r", ch);
		return FALSE;
	}

	buf = new_buf();
	area = ch->in_room->area;
	all = !str_cmp(arg, "all");
	found = FALSE;

	for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
		if ((mob = get_mob_index(vnum)) != NULL) {
			if (all || is_name(arg, mob->player_name)) {
				found = TRUE;
				unclr = uncolor_str(capitalize(mob->short_descr));

				printf_buf(buf, "[`1%5d``] %-17.16s",
					   mob->vnum,
					   unclr);

				free_string(unclr);
				if (++col % 3 == 0)
					add_buf(buf, "\n\r");
			}
		}
	}

	if (!found) {
		send_to_char("Mobile(s) not found in this area.\n\r", ch);
		return FALSE;
	}

	if (col % 3 != 0)
		add_buf(buf, "\n\r");


	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return FALSE;
}



/***************************************************************************
*	redit_olist
*
*	list all the objects in an area
***************************************************************************/
EDIT(redit_olist){
	OBJ_INDEX_DATA *obj;
	AREA_DATA *area;
	BUFFER *buf;
	char arg[MIL];
	char *unclr;
	bool all;
	bool found;
	long vnum;
	int col = 0;

	(void)one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Syntax:  olist <all/name/item_type>\n\r", ch);
		return FALSE;
	}

	area = ch->in_room->area;
	buf = new_buf();
	all = !str_cmp(arg, "all");
	found = FALSE;

	for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
		if ((obj = get_obj_index(vnum))) {
			if (all || is_name(arg, obj->name)
			    || flag_value(type_flags, arg) == obj->item_type) {
				found = TRUE;
				unclr = uncolor_str(capitalize(obj->short_descr));
				printf_buf(buf, "[`1%5d``] %-17.16s",
					   obj->vnum,
					   unclr);
				free_string(unclr);
				if (++col % 3 == 0)
					add_buf(buf, "\n\r");
			}
		}
	}

	if (!found) {
		send_to_char("Object(s) not found in this area.\n\r", ch);
		return FALSE;
	}

	if (col % 3 != 0)
		add_buf(buf, "\n\r");

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return FALSE;
}


/***************************************************************************
*	redit_mshow
*
*	show a mob in the area
***************************************************************************/
EDIT(redit_mshow){
	MOB_INDEX_DATA *mob;
	int value;

	if (argument[0] == '\0') {
		send_to_char("Syntax:  mshow <vnum>\n\r", ch);
		return FALSE;
	}

	if (!is_number(argument)) {
		send_to_char("REdit: Enter a mobs vnum.\n\r", ch);
		return FALSE;
	}

	if (is_number(argument)) {
		value = atoi(argument);
		if (!(mob = get_mob_index(value))) {
			send_to_char("REdit:  That mobile does not exist.\n\r", ch);
			return FALSE;
		}

		ch->desc->ed_data = (void *)mob;
	}

	medit_show(ch, argument);
	ch->desc->ed_data = (void *)ch->in_room;

	return FALSE;
}


/***************************************************************************
*	redit_oshow
*
*	show an object in an area
***************************************************************************/
EDIT(redit_oshow){
	OBJ_INDEX_DATA *obj;
	int value;

	if (argument[0] == '\0') {
		send_to_char("Syntax:  oshow <vnum>\n\r", ch);
		return FALSE;
	}

	if (!is_number(argument)) {
		send_to_char("REdit: Enter an object vnum.\n\r", ch);
		return FALSE;
	}

	if (is_number(argument)) {
		value = atoi(argument);
		if (!(obj = get_obj_index(value))) {
			send_to_char("REdit:  That object does not exist.\n\r", ch);
			return FALSE;
		}

		ch->desc->ed_data = (void *)obj;
	}

	oedit_show(ch, argument);

	ch->desc->ed_data = (void *)ch->in_room;
	return FALSE;
}


/***************************************************************************
*	redit_show
*
*	show the details for a room
***************************************************************************/
EDIT(redit_show){
	ROOM_INDEX_DATA *room;
	OBJ_DATA *obj;
	CHAR_DATA *rch;
	char buf[MSL];
	int door;
	bool fcnt;

	EDIT_ROOM(ch, room);

	printf_to_char(ch, "`&Name``:        [%s]\n\r`&Area``:        [%5d] %s\n\r",
		       room->name, room->area->vnum, room->area->name);
	printf_to_char(ch, "`&Vnum``:        [%5d]\n\r`&Sector``:      [%s]\n\r",
		       room->vnum, flag_string(sector_flags, room->sector_type));
	printf_to_char(ch, "`&Room flags``:  [%s]\n\r",
		       flag_string(room_flags, room->room_flags));

	if (room->heal_rate != 100 || room->mana_rate != 100)
		printf_to_char(ch, "`&Health rate``: [%d]\n\r`&Mana rate``:   [%d]\n\r",
			       room->heal_rate, room->mana_rate);


	if (!IS_NULLSTR(room->owner))
		printf_to_char(ch, "`&Owner``:       [%s]\n\r", room->owner);

	if (room->extra_descr) {
		EXTRA_DESCR_DATA *ed;


		printf_to_char(ch, "`&Desc Kwds``:   [");
		for (ed = room->extra_descr; ed; ed = ed->next) {
			printf_to_char(ch, ed->keyword);
			if (ed->next)
				printf_to_char(ch, " ");

		}
		printf_to_char(ch, "]\n\r");
	}

	printf_to_char(ch, "`&Characters``:  [");
	fcnt = FALSE;
	for (rch = room->people; rch; rch = rch->next_in_room) {
		one_argument(rch->name, buf);
		if (rch->next_in_room)
			printf_to_char(ch, "%s ", buf);
		else
			printf_to_char(ch, "%s]\n\r", buf);
		fcnt = TRUE;
	}

	if (!fcnt)
		send_to_char("none]\n\r", ch);

	printf_to_char(ch, "`&Objects``:     [");
	fcnt = FALSE;
	for (obj = room->contents; obj; obj = obj->next_content) {
		one_argument(obj->name, buf);
		if (obj->next_content)
			printf_to_char(ch, "%s ", buf);
		else
			printf_to_char(ch, "%s]\n\r", buf);

		fcnt = TRUE;
	}

	if (!fcnt)
		send_to_char("none]\n\r", ch);

	printf_to_char(ch, "`&Description``:\n\r%s", room->description);

	if (room->affected != NULL) {
		AFFECT_DATA *paf;
		SKILL *skill;
		int iter = 0;

		send_to_char("\n\r`&Number Level  Spell          ``\n\r", ch);
		send_to_char("`1------ ------ -------------------``\n\r", ch);

		for (paf = room->affected; paf != NULL; paf = paf->next, iter++) {
			if ((skill = resolve_skill_affect(paf)) != NULL) {
				printf_to_char(ch, "[%4d] %-6d %s\n\r",
					       iter,
					       paf->level,
					       skill->name);
			}
		}
	}

	send_to_char("\n\r`&EXITS:\n\r", ch);
	for (door = 0; door < MAX_DIR; door++) {
		EXIT_DATA *pexit;

		if ((pexit = room->exit[door])) {
			char word[MIL];
			char reset_state[MSL];
			char *state;
			int iter;
			int length;

			printf_to_char(ch, "`#%s``:\n\r [%5d] Key: [%5d] ",
				       capitalize(dir_name[door]),
				       pexit->u1.to_room ? pexit->u1.to_room->vnum : 0,
				       pexit->key);
			/*
			 * Format up the exit info.
			 * Capitalize all flags that are not part of the reset info.
			 */
			strcpy(reset_state, flag_string(exit_flags, pexit->rs_flags));
			state = flag_string(exit_flags, pexit->exit_info);

			fcnt = FALSE;
			printf_to_char(ch, " Exit flags: [");
			for (;; ) {
				state = one_argument(state, word);

				if (word[0] != '\0') {
					if (str_infix(word, reset_state)) {
						length = (int)strlen(word);
						for (iter = 0; iter < length; iter++)
							word[iter] = UPPER(word[iter]);
					}
					if (!fcnt) {
						printf_to_char(ch, "%s", word);
						fcnt = TRUE;
					} else {
						printf_to_char(ch, " %s", word);
					}
				} else {
					printf_to_char(ch, "]\n\r");
					break;
				}
			}

			if (pexit->keyword && pexit->keyword[0] != '\0')
				printf_to_char(ch, " `&Keywords``: [%s]\n\r", pexit->keyword);

			if (pexit->description && pexit->description[0] != '\0')
				printf_to_char(ch, " %s", pexit->description);
		}
	}

	return FALSE;
}



/***************************************************************************
*	exits
***************************************************************************/
/***************************************************************************
*	redit_north
***************************************************************************/
EDIT(redit_north){
	return change_exit(ch, argument, DIR_NORTH);
}

/***************************************************************************
*	redit_south
***************************************************************************/
EDIT(redit_south){
	return change_exit(ch, argument, DIR_SOUTH);
}

/***************************************************************************
*	redit_east
***************************************************************************/
EDIT(redit_east){
	return change_exit(ch, argument, DIR_EAST);
}

/***************************************************************************
*	redit_west
***************************************************************************/
EDIT(redit_west){
	return change_exit(ch, argument, DIR_WEST);
}

/***************************************************************************
*	redit_up
***************************************************************************/
EDIT(redit_up){
	return change_exit(ch, argument, DIR_UP);
}

/***************************************************************************
*	redit_down
***************************************************************************/
EDIT(redit_down){
	return change_exit(ch, argument, DIR_DOWN);
}



/***************************************************************************
*	redit_ed
*
*	edit the rooms extra description
***************************************************************************/
EDIT(redit_ed){
	ROOM_INDEX_DATA *room;
	EXTRA_DESCR_DATA *ed;
	char command[MIL];
	char keyword[MIL];

	EDIT_ROOM(ch, room);

	argument = one_argument(argument, command);
	one_argument(argument, keyword);

	if (command[0] == '\0' || keyword[0] == '\0') {
		send_to_char("Syntax:  ed add [keyword]\n\r", ch);
		send_to_char("         ed edit [keyword]\n\r", ch);
		send_to_char("         ed delete [keyword]\n\r", ch);
		send_to_char("         ed format [keyword]\n\r", ch);
		return FALSE;
	}

	if (!str_cmp(command, "add")) {
		if (keyword[0] == '\0') {
			send_to_char("Syntax:  ed add [keyword]\n\r", ch);
			return FALSE;
		}

		ed = new_extra_descr();
		ed->keyword = str_dup(keyword);
		ed->description = str_dup("");
		ed->next = room->extra_descr;
		room->extra_descr = ed;

		string_append(ch, &ed->description);
		return TRUE;
	}


	if (!str_cmp(command, "edit")) {
		if (keyword[0] == '\0') {
			send_to_char("Syntax:  ed edit [keyword]\n\r", ch);
			return FALSE;
		}

		for (ed = room->extra_descr; ed; ed = ed->next)
			if (is_name(keyword, ed->keyword))
				break;

		if (!ed) {
			send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
			return FALSE;
		}

		string_append(ch, &ed->description);
		return TRUE;
	}


	if (!str_cmp(command, "delete")) {
		EXTRA_DESCR_DATA *ped = NULL;

		if (keyword[0] == '\0') {
			send_to_char("Syntax:  ed delete [keyword]\n\r", ch);
			return FALSE;
		}

		for (ed = room->extra_descr; ed; ed = ed->next) {
			if (is_name(keyword, ed->keyword))
				break;
			ped = ed;
		}

		if (!ed) {
			send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
			return FALSE;
		}

		if (!ped)
			room->extra_descr = ed->next;
		else
			ped->next = ed->next;

		free_extra_descr(ed);

		send_to_char("Extra description deleted.\n\r", ch);
		return TRUE;
	}


	if (!str_cmp(command, "format")) {
		if (keyword[0] == '\0') {
			send_to_char("Syntax:  ed format [keyword]\n\r", ch);
			return FALSE;
		}

		for (ed = room->extra_descr; ed; ed = ed->next)
			if (is_name(keyword, ed->keyword))
				break;

		if (!ed) {
			send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
			return FALSE;
		}
		ed->description = format_string(ed->description);

		send_to_char("Extra description formatted.\n\r", ch);
		return TRUE;
	}

	redit_ed(ch, "");
	return FALSE;
}


/***************************************************************************
*	redit_name
*
*	set the name of the room
***************************************************************************/
EDIT(redit_name){
	ROOM_INDEX_DATA *room;

	EDIT_ROOM(ch, room);
	if (argument[0] == '\0') {
		send_to_char("Syntax:  name [name]\n\r", ch);
		return FALSE;
	}

	free_string(room->name);
	room->name = str_dup(argument);

	send_to_char("Name set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	redit_desc
*
*	set the description of the room
***************************************************************************/
EDIT(redit_desc){
	ROOM_INDEX_DATA *room;

	EDIT_ROOM(ch, room);
	if (argument[0] == '\0') {
		string_append(ch, &room->description);
		return TRUE;
	}

	send_to_char("Syntax:  desc\n\r", ch);
	return FALSE;
}


/***************************************************************************
*	redit_heal
*
*	set the healing rate of the room
***************************************************************************/
EDIT(redit_heal){
	ROOM_INDEX_DATA *room;

	EDIT_ROOM(ch, room);
	if (is_number(argument)) {
		room->heal_rate = atoi(argument);
		send_to_char("Heal rate set.\n\r", ch);
		return TRUE;
	}

	send_to_char("Syntax : heal <#xnumber>\n\r", ch);
	return FALSE;
}

/***************************************************************************
*	redit_mana
*
*	set the rate at which mana is healed
***************************************************************************/
EDIT(redit_mana){
	ROOM_INDEX_DATA *room;

	EDIT_ROOM(ch, room);
	if (is_number(argument)) {
		room->mana_rate = atoi(argument);
		send_to_char("Mana rate set.\n\r", ch);
		return TRUE;
	}

	send_to_char("Syntax : mana <#xnumber>\n\r", ch);
	return FALSE;
}

/***************************************************************************
*	redit_owner
*
*	set the owner of a room
***************************************************************************/
EDIT(redit_owner){
	ROOM_INDEX_DATA *room;

	EDIT_ROOM(ch, room);
	if (argument[0] == '\0') {
		send_to_char("Syntax:  owner [owner]\n\r", ch);
		send_to_char("         owner none\n\r", ch);
		return FALSE;
	}

	free_string(room->owner);
	if (!str_cmp(argument, "none"))
		room->owner = str_dup("");
	else
		room->owner = str_dup(argument);

	send_to_char("Owner set.\n\r", ch);
	return TRUE;
}



/***************************************************************************
*	redit_room
*
*	toggle the room flags
***************************************************************************/
EDIT(redit_room){
	ROOM_INDEX_DATA *room;
	int value;

	EDIT_ROOM(ch, room);

	if ((value = flag_value(room_flags, argument)) == NO_FLAG) {
		send_to_char("Syntax: room [flags]\n\r", ch);
		return FALSE;
	}

	TOGGLE_BIT(room->room_flags, value);

	send_to_char("Room flags toggled.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	redit_sector
*
*	set the sector type of a room
***************************************************************************/
EDIT(redit_sector){
	ROOM_INDEX_DATA *room;
	int value;

	EDIT_ROOM(ch, room);

	if ((value = flag_value(sector_flags, argument)) == NO_FLAG) {
		send_to_char("Syntax: sector [type]\n\r", ch);
		return FALSE;
	}

	room->sector_type = value;
	send_to_char("Sector type set.\n\r", ch);

	return TRUE;
}

/***************************************************************************
*	redit_addaffect
*
*	add an affect to the room
***************************************************************************/
EDIT(redit_addaffect){
	ROOM_INDEX_DATA *room;
	AFFECT_DATA af;
	SKILL *skill;
	char arg[MSL];

	EDIT_ROOM(ch, room);
	argument = one_argument(argument, arg);
	skill = skill_lookup(arg);
	argument = one_argument(argument, arg);

	if (!is_number(arg)
	    || skill == NULL
	    || skill->target != TAR_ROOM) {
		send_to_char("Syntax: addaffect [spell] [level]\n\r", ch);
		return FALSE;
	}

	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = atoi(arg);
	af.duration = -1;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_room(room, &af);
	send_to_char("Affect added.\n\r", ch);

	return TRUE;
}


/***************************************************************************
*	redit_delaffect
*
*	remove an affect to the room
***************************************************************************/
EDIT(redit_delaffect){
	ROOM_INDEX_DATA *room;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
	char arg[MSL];
	int number;
	int count;

	EDIT_ROOM(ch, room);
	argument = one_argument(argument, arg);
	if (!is_number(arg)) {
		send_to_char("Syntax: delaffect [# affect]\n\r", ch);
		return FALSE;
	}

	number = atoi(arg);
	count = 0;
	for (paf = room->affected; paf != NULL; paf = paf_next) {
		paf_next = paf->next;

		if (count++ == number) {
			affect_remove_room(room, paf);
			printf_to_char(ch, "Affect #%d removed.\n\r", number);
			return TRUE;
		}
	}

	send_to_char("Affect number does not exist.\n\r", ch);
	return FALSE;
}

/***************************************************************************
*	redit_format
*
*	format the room description
***************************************************************************/
EDIT(redit_format){
	ROOM_INDEX_DATA *room;

	EDIT_ROOM(ch, room);

	room->description = format_string(room->description);

	send_to_char("String formatted.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	redit_mreset
*
*	edit the rooms mob resets
***************************************************************************/
EDIT(redit_mreset){
	ROOM_INDEX_DATA *room;
	MOB_INDEX_DATA *mob;
	CHAR_DATA *newmob;
	RESET_DATA *pReset;
	char arg[MIL];
	char arg2[MIL];

	EDIT_ROOM(ch, room);

	argument = one_argument(argument, arg);
	argument = one_argument(argument, arg2);
	if (arg[0] == '\0' || !is_number(arg)) {
		send_to_char("Syntax:  mreset <vnum> <max #x> <mix #x>\n\r", ch);
		return FALSE;
	}

	if (!(mob = get_mob_index(atoi(arg)))) {
		send_to_char("REdit: No mobile has that vnum.\n\r", ch);
		return FALSE;
	}

	if (mob->area != room->area) {
		send_to_char("REdit: No such mobile in this area.\n\r", ch);
		return FALSE;
	}

	/*
	 * Create the mobile reset.
	 */
	pReset = new_reset_data();
	pReset->command = 'M';
	pReset->arg1 = mob->vnum;
	pReset->arg2 = is_number(arg2) ? atoi(arg2) : MAX_MOB;
	pReset->arg3 = room->vnum;
	pReset->arg4 = is_number(argument) ? atoi(argument) : 1;
	add_reset(room, pReset, 0);

	/*
	 * Create the mobile.
	 */
	newmob = create_mobile(mob);
	char_to_room(newmob, room);

	printf_to_char(ch, "%s(%d) has been loaded and added to resets.\n\rThere will be a maximum of %d loaded to this room.\n\r",
		       capitalize(mob->short_descr),
		       mob->vnum,
		       pReset->arg2);

	act("$n has created $N!", ch, NULL, newmob, TO_ROOM);
	return TRUE;
}


/***************************************************************************
*	redit_oreset
*
*	edit the rooms obj resets
***************************************************************************/
EDIT(redit_oreset){
	ROOM_INDEX_DATA *room;
	OBJ_INDEX_DATA *obj;
	OBJ_DATA *newobj;
	OBJ_DATA *to_obj;
	CHAR_DATA *to_mob;
	RESET_DATA *pReset;
	char arg1[MIL];
	char arg2[MIL];
	int olevel = 0;

	EDIT_ROOM(ch, room);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || !is_number(arg1)) {
		send_to_char("Syntax:  oreset <vnum> <args>\n\r", ch);
		send_to_char("        -no_args               = into room\n\r", ch);
		send_to_char("        -<obj_name>            = into obj\n\r", ch);
		send_to_char("        -<mob_name> <wear_loc> = into mob\n\r", ch);
		return FALSE;
	}

	if (!(obj = get_obj_index(atoi(arg1)))) {
		send_to_char("REdit: No object has that vnum.\n\r", ch);
		return FALSE;
	}

	if (obj->area != room->area) {
		send_to_char("REdit: No such object in this area.\n\r", ch);
		return FALSE;
	}

	/*
	 * Load into room.
	 */
	if (arg2[0] == '\0') {
		pReset = new_reset_data();
		pReset->command = 'O';
		pReset->arg1 = obj->vnum;
		pReset->arg2 = 0;
		pReset->arg3 = room->vnum;
		pReset->arg4 = 0;
		add_reset(room, pReset, 0 /* Last slot*/);

		newobj = create_object(obj, number_fuzzy(olevel));
		obj_to_room(newobj, room);

		printf_to_char(ch, "%s(%d) has been loaded and added to resets.\n\r",
			       capitalize(obj->short_descr),
			       obj->vnum);
		act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
		return TRUE;
	}

	/*
	 * Load into object's inventory.
	 */
	if (argument[0] == '\0'
	    && ((to_obj = get_obj_list(ch, arg2, room->contents)) != NULL)) {
		pReset = new_reset_data();
		pReset->command = 'P';
		pReset->arg1 = obj->vnum;
		pReset->arg2 = 0;
		pReset->arg3 = to_obj->obj_idx->vnum;
		pReset->arg4 = 1;
		add_reset(room, pReset, 0);

		newobj = create_object(obj, number_fuzzy(olevel));
		newobj->cost = 0;
		obj_to_obj(newobj, to_obj);

		printf_to_char(ch, "%s(%d) has been loaded into "
			       "%s(%d) and added to resets.\n\r",
			       capitalize(newobj->short_descr),
			       newobj->obj_idx->vnum,
			       to_obj->short_descr,
			       to_obj->obj_idx->vnum);
		act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
		return TRUE;
	}

	if ((to_mob = get_char_room(ch, arg2)) != NULL) {
		int wear_loc;

		if ((wear_loc = flag_value(wear_loc_flags, argument)) == NO_FLAG) {
			send_to_char("REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch);
			return FALSE;
		}

		/*
		 * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
		 */
		if (!IS_SET(obj->wear_flags, wear_bit(wear_loc))) {
			printf_to_char(ch, "%s(%d) has wear flags: [%s]\n\r",
				       capitalize(obj->short_descr),
				       obj->vnum,
				       flag_string(wear_flags, obj->wear_flags));
			return FALSE;
		}

		/*
		 * Can't load into same position.
		 */
		if (get_eq_char(to_mob, wear_loc)) {
			send_to_char("REdit:  Object already equipped.\n\r", ch);
			return FALSE;
		}

		pReset = new_reset_data();
		pReset->arg1 = obj->vnum;
		pReset->arg2 = wear_loc;

		if (pReset->arg2 == WEAR_NONE)
			pReset->command = 'G';
		else
			pReset->command = 'E';
		pReset->arg3 = wear_loc;
		add_reset(room, pReset, 0);

		olevel = URANGE(0, to_mob->level - 2, LEVEL_HERO);
		newobj = create_object(obj, number_fuzzy(olevel));

		if (IS_SHOPKEEPER(to_mob)) {
			switch (obj->item_type) {
			default:
				olevel = 0;
				break;
			case ITEM_PILL:
				olevel = number_range(0, 10);
				break;
			case ITEM_POTION:
				olevel = number_range(0, 10);
				break;
			case ITEM_SCROLL:
				olevel = number_range(5, 15);
				break;
			case ITEM_WAND:
				olevel = number_range(10, 20);
				break;
			case ITEM_STAFF:
				olevel = number_range(15, 25);
				break;
			case ITEM_ARMOR:
				olevel = number_range(5, 15);
				break;
			case ITEM_WEAPON:
				if (pReset->command == 'G')
					olevel = number_range(5, 15);
				else
					olevel = number_fuzzy(olevel);
				break;
			}

			newobj = create_object(obj, olevel);
			if (pReset->arg2 == WEAR_NONE)
				SET_BIT(newobj->extra_flags, ITEM_INVENTORY);
		} else {
			newobj = create_object(obj, number_fuzzy(olevel));
		}

		obj_to_char(newobj, to_mob);
		if (pReset->command == 'E')
			equip_char(to_mob, newobj, (int)pReset->arg3);

		printf_to_char(ch, "%s(%d) has been loaded "
			       "%s of %s(%d) and added to resets.\n\r",
			       capitalize(obj->short_descr),
			       obj->vnum,
			       flag_string(wear_loc_strings, pReset->arg3),
			       to_mob->short_descr,
			       to_mob->mob_idx->vnum);
	}


	send_to_char("REdit:  That mobile isn't here.\n\r", ch);
	return FALSE;
}

/***************************************************************************
*	redit_flagall
*
*	set or toggle room flags throughout entire area
*	added by Monrick, 2/2008
***************************************************************************/
EDIT(redit_flagall){
	ROOM_INDEX_DATA *room;                          /* individual room to edit */
	AREA_DATA *area;                                /* area being edited */
	BUFFER *buf;                                    /* text to return to ch */
	char *unclr;                                    /* uncolored room name */
	char rFlag[MIL];                                /* name of room flag to set */
	int iFlag;                                      /* int value of room flag */
	char rType[MIL];                                /* type of set (on, off, toggle) */
	bool found;                                     /* room exists? */
	long vnum;                                      /* vnum of room */
	int col = 0;                                    /* display columns */

	argument = one_argument(argument, rFlag);
	argument = one_argument(argument, rType);

	if ((iFlag = flag_value(room_flags, rFlag)) == NO_FLAG) {
		send_to_char("Syntax:  flagall [flag] <toggle>\n\r", ch);
		send_to_char("        `&Toggle can be `@on`&, `!off`&, or left out to toggle``\n\r", ch);
		send_to_char("        Type '`^? room``' to see a list of available room flags.\n\r", ch);
		return FALSE;
	}

	area = ch->in_room->area;
	buf = new_buf();
	found = FALSE;

	for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
		if ((room = get_room_index(vnum))) {
			found = TRUE;

			if (!strcmp(rType, "on"))
				SET_BIT(room->room_flags, iFlag);
			else if (!strcmp(rType, "off"))
				REMOVE_BIT(room->room_flags, iFlag);
			else
				TOGGLE_BIT(room->room_flags, iFlag);

			unclr = uncolor_str(capitalize(room->name));
			printf_buf(buf, "[");

			if (IS_SET(room->room_flags, iFlag))
				printf_buf(buf, "`@");
			else
				printf_buf(buf, "`!");

			printf_buf(buf, "%5d``] %-17.16s", vnum, unclr);
			free_string(unclr);

			if (++col % 3 == 0)
				add_buf(buf, "\n\r");
		}
	}

	if (!found) {
		send_to_char("Room(s) not found in this area.\n\r", ch);
		return FALSE;
	}

	if (col % 3 != 0)
		add_buf(buf, "\n\r");

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return TRUE;
}

/***************************************************************************
*	redit_showrooms
*
*	show room flags throughout entire area
*	added by Monrick, 2/2008
***************************************************************************/
EDIT(redit_showrooms){
	ROOM_INDEX_DATA *room;                          /* individual room to edit */
	AREA_DATA *area;                                /* area being edited */
	BUFFER *buf;                                    /* text to return to ch */
	char *unclr;                                    /* uncolored room name */
	char rFlag[MIL];                                /* name of room flag to see */
	int iFlag;                                      /* int value of room flag */
	bool found;                                     /* room exists? */
	long vnum;                                      /* vnum of room */
	int col = 0;                                    /* display columns */

	argument = one_argument(argument, rFlag);

	if ((iFlag = flag_value(room_flags, rFlag)) == NO_FLAG) {
		send_to_char("Syntax:  showrooms [flag]\n\r", ch);
		send_to_char("        Type '`^? room``' to see a list of available room flags.\n\r", ch);
		return FALSE;
	}

	area = ch->in_room->area;
	buf = new_buf();
	found = FALSE;

	for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
		if ((room = get_room_index(vnum))) {
			found = TRUE;

			unclr = uncolor_str(capitalize(room->name));
			printf_buf(buf, "[");

			if (IS_SET(room->room_flags, iFlag))
				printf_buf(buf, "`@");
			else
				printf_buf(buf, "`!");

			printf_buf(buf, "%5d``] %-17.16s", vnum, unclr);
			free_string(unclr);

			if (++col % 3 == 0)
				add_buf(buf, "\n\r");
		}
	}

	if (!found) {
		send_to_char("Room(s) not found in this area.\n\r", ch);
		return FALSE;
	}

	if (col % 3 != 0)
		add_buf(buf, "\n\r");

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return FALSE;
}
