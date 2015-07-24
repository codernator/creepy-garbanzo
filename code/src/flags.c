/**************************************************************************
 *   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *   Chastain, Michael Quan, and Mitchell Tse.                              *
 *	                                                                       *
 *   In order to use any part of this Merc Diku Mud, you must comply with   *
 *   both the original Diku license in 'license.doc' as well the Merc	   *
 *   license in 'license.txt'.  In particular, you may not remove either of *
 *   these copyright notices.                                               *
 *                                                                             *
 *   Much time and thought has gone into this software and you are          *
 *   benefitting.  We hope that you share your changes too.  What goes      *
 *   around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#if defined(WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "interp.h"
#include "lookup.h"
#include "olc.h"


extern int flag_value              args((const struct flag_type *flag_table, char *argument));
extern char *flag_string             args((const struct flag_type *flag_table, long bits));

/***************************************************************************
*	declarations
***************************************************************************/
typedef void FLAG_CMD (CHAR_DATA *ch, void *target, char *argument);

/* flag command map */
struct flag_cmd_map {
	char *			flag;
	const struct flag_type *table;
	FLAG_CMD *		flag_fn;
};

/* utility functions */
static void flag_list                   args((CHAR_DATA * ch, const struct flag_cmd_map *flag_table));
static void flag_list_value     args((CHAR_DATA * ch, const struct flag_type *flag_table));

static void flag_set                    args((CHAR_DATA * ch, void *target, char *title,
					      const struct flag_cmd_map *table, char *argument));

/* top-level functions */
static void flag_set_char               args((CHAR_DATA * ch, char *argument));
static void flag_set_room               args((CHAR_DATA * ch, char *argument));
static void flag_set_obj                args((CHAR_DATA * ch, char *argument));


/* mob specific flag functions */
static void flag_char_act                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_off                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_form                      args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_part                      args((CHAR_DATA * ch, void *target, char *argument));

/* shared between mob and char */
static void flag_char_aff                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_imm                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_res                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_vuln                      args((CHAR_DATA * ch, void *target, char *argument));

/* char specific flag functions */
static void flag_char_plr                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_comm                      args((CHAR_DATA * ch, void *target, char *argument));
static void flag_char_deathroom args((CHAR_DATA * ch, void *target, char *argument));

/* room flag functions */
static void flag_room_room                      args((CHAR_DATA * ch, void *target, char *argument));

/* object flag functions */
static void flag_obj_type                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_obj_wear                       args((CHAR_DATA * ch, void *target, char *argument));
static void flag_obj_extra                      args((CHAR_DATA * ch, void *target, char *argument));
static void flag_obj_extra2             args((CHAR_DATA * ch, void *target, char *argument));
static void flag_obj_wpn_class          args((CHAR_DATA * ch, void *target, char *argument));
static void flag_obj_wpn_flags          args((CHAR_DATA * ch, void *target, char *argument));
static void flag_obj_wpn_damage args((CHAR_DATA * ch, void *target, char *argument));


static const struct flag_cmd_map mob_set_flags[] =
{
	{ "act",  act_flags,	flag_char_act  },
	{ "off",  off_flags,	flag_char_off, },
	{ "form", form_flags,	flag_char_form },
	{ "part", part_flags,	flag_char_part },
	{ "aff",  affect_flags, flag_char_aff  },
	{ "imm",  imm_flags,	flag_char_imm  },
	{ "res",  res_flags,	flag_char_res  },
	{ "vuln", vuln_flags,	flag_char_vuln },
	{ "",	  NULL,		NULL	       }
};

static const struct flag_cmd_map char_set_flags[] =
{
	{ "plr",       plr_flags,    flag_char_plr	 },
	{ "comm",      comm_flags,   flag_char_comm	 },
	{ "deathroom", NULL,	     flag_char_deathroom },
	{ "aff",       affect_flags, flag_char_aff	 },
	{ "imm",       imm_flags,    flag_char_imm	 },
	{ "res",       res_flags,    flag_char_res	 },
	{ "vuln",      vuln_flags,   flag_char_vuln	 },
	{ "",	       NULL,	     NULL		 }
};

static const struct flag_cmd_map room_set_flags[] =
{
	{ "room", room_flags, flag_room_room },
	{ "",	  NULL,	      NULL	     }
};


static const struct flag_cmd_map obj_set_flags[] =
{
	{ "type",	   type_flags,	     flag_obj_type	 },
	{ "wear",	   wear_flags,	     flag_obj_wear	 },
	{ "extra",	   extra_flags,	     flag_obj_extra	 },
	{ "extra2",	   extra2_flags,     flag_obj_extra2	 },
	{ "weapon_type",   weapon_class,     flag_obj_wpn_class	 },
	{ "weapon_flags",  weapon_flag_type, flag_obj_wpn_flags	 },
	{ "weapon_damage", damage_flags,     flag_obj_wpn_damage },
	{ "",		   NULL,	     NULL		 }
};

/*	additionaly possible flags - socket_type, socket_flags,
 *      token_flags, furniture_flags */



/***************************************************************************
*	do_flag
*
*	entry point for the flag function
***************************************************************************/
void do_flag(CHAR_DATA *ch, char *argument)
{
	char type[MIL];

	DENY_NPC(ch);

	argument = one_argument(argument, type);
	if (type[0] == '\0' || type[0] == '?' || !str_prefix(type, "help")) {
		do_help(ch, "flag_syntax");
		return;
	}

	if (!str_prefix(type, "mob") || !str_prefix(type, "char")) {
		flag_set_char(ch, argument);
		return;
	}

	if (!str_prefix(type, "obj")) {
		flag_set_obj(ch, argument);
		return;
	}


	if (!str_prefix(type, "room")) {
		flag_set_room(ch, argument);
		return;
	}

	send_to_char("Invalid flag type.\n\r\n\r", ch);

	/* print the help msg */
	do_flag(ch, "");
	return;
}



/***************************************************************************
*	base type processors
***************************************************************************/
/***************************************************************************
*	flag_set_char
*
*	flag affects on a character
***************************************************************************/
void flag_set_char(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch;
	char name[MIL];

	argument = one_argument(argument, name);
	if ((vch = get_char_world(ch, name)) == NULL) {
		send_to_char("That character does not exist.", ch);
		return;
	}

	if (IS_NPC(vch))
		/* we have a mob...attempt to set the flag */
		flag_set(ch, vch, "Mob", mob_set_flags, argument);
	else
		flag_set(ch, vch, "Character", char_set_flags, argument);
	return;
}


/***************************************************************************
*	flag_set_obj
*
*	flag affects on an object
***************************************************************************/
void flag_set_obj(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char name[MIL];

	argument = one_argument(argument, name);
	if ((obj = get_obj_here(ch, name)) == NULL) {
		send_to_char("That object is not here.", ch);
		return;
	}

	flag_set(ch, obj, "Object", obj_set_flags, argument);
	return;
}



/***************************************************************************
*	flag_set_room
*
*	flag affects on a room
***************************************************************************/
void flag_set_room(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *room;
	char index[MIL];
	int vnum;

	one_argument(argument, index);

	if (is_number(index)
	    && ((vnum = atoi(index)) > 0)
	    && ((room = get_room_index(vnum)) != NULL))
		argument = one_argument(argument, index);
	else
		room = ch->in_room;

	if (room == NULL) {
		send_to_char("That room does not exist.\n\r", ch);
		return;
	}

	flag_set(ch, room, "Room", room_set_flags, argument);
	return;
}


/***************************************************************************
*	flag set functions
***************************************************************************/

/***************************************************************************
*	shared set functions
***************************************************************************/
/***************************************************************************
*	flag_char_aff
*
*	flag affects on a character
***************************************************************************/
void flag_char_aff(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if ((value = flag_value(affect_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->affected_by, value);
			send_to_char("Affect flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->affected_by, value);
			send_to_char("Affect flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->affected_by, value);
			send_to_char("Affect flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_char_vuln
*
*	flag vulnerable flags on a character
***************************************************************************/
void flag_char_vuln(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if ((value = flag_value(vuln_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->vuln_flags, value);
			send_to_char("Vulnerability flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->vuln_flags, value);
			send_to_char("Vulnerability flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->vuln_flags, value);
			send_to_char("Vulnerability flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_char_res
*
*	flag resistance flags on a character
***************************************************************************/
void flag_char_res(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if ((value = flag_value(res_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->res_flags, value);
			send_to_char("Resistance flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->res_flags, value);
			send_to_char("Resistance flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->res_flags, value);
			send_to_char("Resistance flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_char_imm
*
*	flag immunity flags on a character
***************************************************************************/
void flag_char_imm(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if ((value = flag_value(imm_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->imm_flags, value);
			send_to_char("Immunity flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->imm_flags, value);
			send_to_char("Immunity flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->imm_flags, value);
			send_to_char("Immunity flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}


/***************************************************************************
*	mob set flags
***************************************************************************/
/***************************************************************************
*	flag_char_part
*
*	flag part flags on a mob
***************************************************************************/
void flag_char_part(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if (!IS_NPC(vch)) {
		send_to_char("You can only set Part flags on a mob.\n\r", ch);
		return;
	}

	if ((value = flag_value(part_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->parts, value);
			send_to_char("Part flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->parts, value);
			send_to_char("Part flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->parts, value);
			send_to_char("Part flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_char_form
*
*	flag form flags on a mob
***************************************************************************/
void flag_char_form(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if (!IS_NPC(vch)) {
		send_to_char("You can only set Form flags on a mob.\n\r", ch);
		return;
	}

	if ((value = flag_value(form_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->form, value);
			send_to_char("Form flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->form, value);
			send_to_char("Form flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->form, value);
			send_to_char("Form flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}


/***************************************************************************
*	flag_char_off
*
*	flag offensive flags on a mob
***************************************************************************/
void flag_char_off(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if (!IS_NPC(vch)) {
		send_to_char("You can only set Offense flags on a mob.\n\r", ch);
		return;
	}

	if ((value = flag_value(off_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->off_flags, value);
			send_to_char("Offense flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->off_flags, value);
			send_to_char("Offense flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->off_flags, value);
			send_to_char("Offense flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_char_act
*
*	flag act flags on a mob
***************************************************************************/
void flag_char_act(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if (!IS_NPC(vch)) {
		send_to_char("You can only set Act flags on a mob.\n\r", ch);
		return;
	}

	if ((value = flag_value(act_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->act, value);
			send_to_char("Act flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->act, value);
			send_to_char("Act flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->act, value);
			send_to_char("Act flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}



/***************************************************************************
*	character set flags
***************************************************************************/


/***************************************************************************
*	flag_char_deathroom
*
*	flag the death room on a character
***************************************************************************/
void flag_char_deathroom(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	ROOM_INDEX_DATA *room;
	long value;

	if (IS_NPC(vch)) {
		send_to_char("You can only set Death Room on a player.\n\r", ch);
		return;
	}


	value = atoi(argument);
	if (is_number(argument) && value > 0) {
		if ((room = get_room_index(value)) != NULL) {
			vch->deathroom = value;
			send_to_char("Death Room set.\n\r", ch);
		} else {
			send_to_char("That room number does not exist.\n\r", ch);
		}
	} else {
		if (vch->deathroom > 0) {
			vch->deathroom = 0;
			send_to_char("Death Room reset to 0.\n\r", ch);
			return;
		}

		send_to_char("Must supply a room number to set the Death Room.\n\r", ch);
	}

	return;
}

/***************************************************************************
*	flag_char_comm
*
*	flag comm flags on a character
***************************************************************************/
void flag_char_comm(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if (IS_NPC(vch)) {
		send_to_char("You can only set Communication flags on a player.\n\r", ch);
		return;
	}


	if ((value = flag_value(comm_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->comm, value);
			send_to_char("Communication flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->comm, value);
			send_to_char("Communication flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->comm, value);
			send_to_char("Communication flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}


/***************************************************************************
*	flag_char_plr
*
*	flag player flags on a character
***************************************************************************/
void flag_char_plr(CHAR_DATA *ch, void *target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)target;
	long value;

	if (IS_NPC(vch)) {
		send_to_char("You can only set Player flags on a player.\n\r", ch);
		return;
	}


	if ((value = flag_value(plr_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(vch->act, value);
			send_to_char("Player flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(vch->act, value);
			send_to_char("Player flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(vch->act, value);
			send_to_char("Player flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}


/***************************************************************************
*	room set flags
***************************************************************************/
/***************************************************************************
*	flag_room_room
*
*	flag room flags on a room
***************************************************************************/
void flag_room_room(CHAR_DATA *ch, void *target, char *argument)
{
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *)target;
	long value;

	if ((value = flag_value(room_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(room->room_flags, value);
			send_to_char("Room flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(room->room_flags, value);
			send_to_char("Room flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(room->room_flags, value);
			send_to_char("Room flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}



/***************************************************************************
*	object set flags
***************************************************************************/
/***************************************************************************
*	flag_obj_type
*
*	flag item type on an object
***************************************************************************/
void flag_obj_type(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;

	if ((value = flag_value(type_flags, argument)) != NO_FLAG) {
		obj->item_type = (int)value;
		send_to_char("Item type set.\n\r", ch);
		return;
	}

	send_to_char("That item type does not exist.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_obj_wear
*
*	flag wear flags on an object
***************************************************************************/
void flag_obj_wear(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;

	if ((value = flag_value(wear_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(obj->wear_flags, value);
			send_to_char("Wear flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(obj->wear_flags, value);
			send_to_char("Wear flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(obj->wear_flags, value);
			send_to_char("Wear flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_obj_extra
*
*	flag extra flags on an object
***************************************************************************/
void flag_obj_extra(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;

	if ((value = flag_value(extra_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(obj->extra_flags, value);
			send_to_char("Extra flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(obj->extra_flags, value);
			send_to_char("Extra flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(obj->extra_flags, value);
			send_to_char("Extra flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

void flag_obj_extra2(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;

	if ((value = flag_value(extra2_flags, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(obj->extra2_flags, value);
			send_to_char("Extra2 flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(obj->extra2_flags, value);
			send_to_char("Extra2 flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(obj->extra2_flags, value);
			send_to_char("Extra2 flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	flag_obj_wpn_class
*
*	flag weapon class flags on an object
***************************************************************************/
void flag_obj_wpn_class(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;


	if (obj->item_type != ITEM_WEAPON) {
		send_to_char("You can only set weapon type on weapons.\n\r", ch);
		return;
	}

	if ((value = flag_value(weapon_class, argument)) != NO_FLAG) {
		obj->value[0] = value;
		send_to_char("Weapon class set.\n\r", ch);
		return;
	}

	send_to_char("That weapon class does not exists or is not settable.\n\r", ch);
	return;
}


/***************************************************************************
*	flag_obj_wpn_flags
*
*	flag weapon flags on a weapon
***************************************************************************/
void flag_obj_wpn_flags(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;


	if (obj->item_type != ITEM_WEAPON) {
		send_to_char("You can only set weapon flags on weapons.\n\r", ch);
		return;
	}

	if ((value = flag_value(weapon_flag_type, argument)) != NO_FLAG) {
		if (*argument == '+') {
			SET_BIT(obj->value[4], value);
			send_to_char("Weapon flags added.\n\r", ch);
		} else if (*argument == '-') {
			REMOVE_BIT(obj->value[4], value);
			send_to_char("Weapon flags removed.\n\r", ch);
		} else {
			TOGGLE_BIT(obj->value[4], value);
			send_to_char("Weapon flags toggled.\n\r", ch);
		}

		return;
	}

	send_to_char("Those flags do not exists or are not settable.\n\r", ch);
	return;
}



/***************************************************************************
*	flag_obj_wpn_damage
*
*	flag damage on a weapon
***************************************************************************/
void flag_obj_wpn_damage(CHAR_DATA *ch, void *target, char *argument)
{
	OBJ_DATA *obj = (OBJ_DATA *)target;
	long value;


	if (obj->item_type != ITEM_WEAPON) {
		send_to_char("You can only set damage type on weapons.\n\r", ch);
		return;
	}

	if ((value = flag_value(damage_flags, argument)) != NO_FLAG) {
		obj->value[3] = value;
		send_to_char("Damage type set.\n\r", ch);
		return;
	}

	send_to_char("That damage type does not exists or is not settable.\n\r", ch);
	return;
}

/***************************************************************************
*	utility functions
***************************************************************************/
/***************************************************************************
*	flag_set
*
*	finds and invokes the proper flag function
***************************************************************************/
void flag_set(CHAR_DATA *ch, void *target, char *title,
	      const struct flag_cmd_map *table, char *argument)
{
	char flag[MIL];
	int idx;

	argument = one_argument(argument, flag);

	/* if we are asking for help at this level, then we need to list the
	 * available flags for the type */
	if (flag[0] != '\0'
	    && flag[0] != '?'
	    && str_prefix(flag, "help")) {
		for (idx = 0; table[idx].flag[0] != '\0'; idx++) {
			if (!str_prefix(flag, table[idx].flag)) {
				/* we found the proper flag - do some stuff */

				if (argument[0] == '\0' || argument[0] == '?' || !str_prefix(argument, "help")) {
					/* print usage for that flag */
					if (table[idx].table != NULL) {
						send_to_char("Available flags:", ch);
						flag_list_value(ch, table[idx].table);
					} else {
						send_to_char("There are no flags for that stat.  You must provide a numeric value.\n\r", ch);
					}

					return;
				}

				/* call the function */
				(*table[idx].flag_fn)(ch, target, argument);
				return;
			}
		}

		/* if we havent returned by now, then we must have not found a match
		 * so print a list of flags for the type */
	}

	printf_to_char(ch, "Available %s Flags:", title);

	flag_list(ch, table);
	return;
}



/***************************************************************************
*	flag_list
*
*	display a list of available flags for a type
***************************************************************************/
void flag_list(CHAR_DATA *ch, const struct flag_cmd_map *flag_table)
{
	int idx;

	for (idx = 0; flag_table[idx].flag[0] != '\0'; idx++) {
		if ((idx % 6) == 0)
			send_to_char("\n\r  ", ch);
		printf_to_char(ch, "%-8.10s  ", flag_table[idx].flag);
	}

	send_to_char("\n\r", ch);
	return;
}


/***************************************************************************
*	flag_list_value
*
*	display a list of available values for a given flag
***************************************************************************/
void flag_list_value(CHAR_DATA *ch, const struct flag_type *flag_table)
{
	int idx;

	for (idx = 0; flag_table[idx].name != NULL; idx++) {
		if ((idx % 4) == 0)
			send_to_char("\n\r  ", ch);

		printf_to_char(ch, "%-13.15s  ", flag_table[idx].name);
	}

	send_to_char("\n\r", ch);
	return;
}
