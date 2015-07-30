/*************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *	                                                                       *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
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
*       Russ Taylor(rtaylor@hypercube.org)                                 *
*       Gabrielle Taylor(gtaylor@hypercube.org)                            *
*       Brian Moore(zump@rom.org)                                          *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"
#include "olc.h"
#include "libstring.h"

extern char *flag_string(const struct flag_type *flag_table, long bits);
extern int flag_value(const struct flag_type *flag_table, char *argument);
extern unsigned int parse_unsigned_int(char *string);
extern long parse_long(char *string);
extern int parse_int(char *string);

/***************************************************************************
*	local functions
***************************************************************************/
static bool set_integer_arg(int *value, char *argument);
static bool set_uint_arg(unsigned int *value, char *argument);
static bool set_long_arg(long *value, char *argument);
static bool set_obj_value_idx(OBJ_DATA * obj, int idx, char *argument);
static void item_type_help(CHAR_DATA * ch, int item_type);

/***************************************************************************
*	set functions
***************************************************************************/
typedef void SET_FN(CHAR_DATA * ch, char *argument);
typedef bool SET_ROOM_FN(CHAR_DATA * ch, ROOM_INDEX_DATA * room, char *argument);
typedef bool SET_CHAR_FN(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);
typedef bool SET_OBJ_FN(CHAR_DATA * ch, OBJ_DATA * obj, char *argument);

static SET_FN set_character;
static SET_FN set_object;
static SET_FN set_room;
static SET_FN set_skill;
static SET_FN set_reboot;
static SET_FN set_copyover;

/***************************************************************************
*	set_cmd_table
***************************************************************************/
static const struct set_cmd_type {
	char *	keyword;
	SET_FN *fn;
}
set_cmd_table[] =
{
	{ "mobile",    set_character },
	{ "character", set_character },
	{ "object",    set_object    },
	{ "room",      set_room	     },
	{ "skill",     set_skill     },
	{ "spell",     set_skill     },
	{ "reboot",    set_reboot    },
	{ "copyover",  set_copyover  },
	{ "",	       NULL	     }
};



/***************************************************************************
*	do_set
*
*	entry level function for set commands
***************************************************************************/
void do_set(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	int idx;

	DENY_NPC(ch);

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		do_help(ch, "SET_CMD_SYNTAX");
		return;
	}

	for (idx = 0; set_cmd_table[idx].keyword[0] != '\0'; idx++) {
		if (!str_prefix(arg, set_cmd_table[idx].keyword)) {
			(*set_cmd_table[idx].fn)(ch, argument);
			return;
		}
	}

	do_set(ch, "");
}



/***************************************************************************
*	characters/mobs
***************************************************************************/
static SET_CHAR_FN set_char_str;
static SET_CHAR_FN set_char_int;
static SET_CHAR_FN set_char_wis;
static SET_CHAR_FN set_char_dex;
static SET_CHAR_FN set_char_con;
static SET_CHAR_FN set_char_luck;
static SET_CHAR_FN set_char_sex;
static SET_CHAR_FN set_char_class;
static SET_CHAR_FN set_char_race;
static SET_CHAR_FN set_char_hp;
static SET_CHAR_FN set_char_mana;
static SET_CHAR_FN set_char_move;
static SET_CHAR_FN set_char_train;
static SET_CHAR_FN set_char_practice;
static SET_CHAR_FN set_char_align;
/* money */
static SET_CHAR_FN set_char_gold;
static SET_CHAR_FN set_char_silver;
static SET_CHAR_FN set_char_bank_gold;
static SET_CHAR_FN set_char_bank_silver;
/* kills/deaths */
static SET_CHAR_FN set_char_pkills;
static SET_CHAR_FN set_char_pdeaths;
static SET_CHAR_FN set_char_mobkills;
static SET_CHAR_FN set_char_mobdeaths;
/* conditions */
static SET_CHAR_FN set_char_full;
SET_CHAR_FN set_char_hunger;
SET_CHAR_FN set_char_thirst;
SET_CHAR_FN set_char_feed;
/* battlefield info */
static SET_CHAR_FN set_char_bfield_enter;
static SET_CHAR_FN set_char_bfield_loss;
static SET_CHAR_FN set_char_bfield_kills;
/* misc properties */
static SET_CHAR_FN set_char_deathroom;
static SET_CHAR_FN set_char_security;
static SET_CHAR_FN set_char_reply;
static SET_CHAR_FN set_char_level;
static SET_CHAR_FN set_char_memory;
static SET_CHAR_FN set_char_extendedexp;

static const struct set_char_cmd_type {
	char *		keyword;
	SET_CHAR_FN *	fn;
}
set_char_cmd_table[] =
{
	/* general stats */
	{ "str",	  set_char_str		},
	{ "int",	  set_char_int		},
	{ "wis",	  set_char_wis		},
	{ "dex",	  set_char_dex		},
	{ "con",	  set_char_con		},
	{ "luck",	  set_char_luck		},
	{ "sex",	  set_char_sex		},
	{ "class",	  set_char_class	},
	{ "race",	  set_char_race		},
	{ "hp",		  set_char_hp		},
	{ "mana",	  set_char_mana		},
	{ "move",	  set_char_move		},
	{ "train",	  set_char_train	},
	{ "practice",	  set_char_practice	},
	{ "align",	  set_char_align	},
	/* money */
	{ "gold",	  set_char_gold		},
	{ "silver",	  set_char_silver	},
	{ "bank_gold",	  set_char_bank_gold	},
	{ "bank_silver",  set_char_bank_silver	},
	/* kills/deaths */
	{ "pkills",	  set_char_pkills	},
	{ "pdeaths",	  set_char_pdeaths	},
	{ "mobkills",	  set_char_mobkills	},
	{ "mobdeaths",	  set_char_mobdeaths	},
	/* conditions */
	{ "full",	  set_char_full		},
	{ "hunger",	  set_char_hunger	},
	{ "thirst",	  set_char_thirst	},
	{ "feed",	  set_char_feed		},
	/* battlefield info */
	{ "bfield_enter", set_char_bfield_enter },
	{ "bfield_loss",  set_char_bfield_loss	},
	{ "bfield_kills", set_char_bfield_kills },
	/* misc properties */
	{ "deathroom",	  set_char_deathroom	},
	{ "security",	  set_char_security	},
	{ "reply",	  set_char_reply	},
	{ "level",	  set_char_level	},
	{ "memory",	  set_char_memory	},
	{ "extendedexp",  set_char_extendedexp	},
	{ "",		  NULL			}
};


/***************************************************************************
*	set_character
*
*	set properties on a character data structure
***************************************************************************/
static void set_character(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch;
	char arg[MIL];
	char cmd[MIL];
	int idx;

	DENY_NPC(ch);

	smash_tilde(argument);
	if (is_help(argument)) {
		int col;

		send_to_char("`#SYNTAX``: set char <name> <field> <value>\n\r\n\r", ch);
		send_to_char("Available fields:\n\r   ", ch);
		col = 0;
		for (idx = 0; set_char_cmd_table[idx].keyword[0] != '\0'; idx++) {
			printf_to_char(ch, "%-15.15s", set_char_cmd_table[idx].keyword);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 4 != 0)
			send_to_char("\n\r", ch);

		return;
	}


	argument = one_argument(argument, arg);
	if ((vch = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	argument = one_argument(argument, cmd);
	if (cmd[0] != '\0') {
		for (idx = 0; set_char_cmd_table[idx].keyword[0] != '\0'; idx++) {
			if (!str_prefix(cmd, set_char_cmd_table[idx].keyword)) {
				if ((*set_char_cmd_table[idx].fn)(ch, vch, argument))
					send_to_char("`1Ok`!.``\n\r", ch);
				return;
			}
		}
	}

	/* generate help message */
	set_character(ch, "");
	return;
}



/***************************************************************************
*	general character sets
***************************************************************************/
/***************************************************************************
*	set_char_str
*
*	set the strength of a character
***************************************************************************/
static bool set_char_str(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: str [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->perm_stat[STAT_STR], argument);
	vch->perm_stat[STAT_STR] = URANGE(3, vch->perm_stat[STAT_STR], get_max_train(vch, STAT_STR));
	return TRUE;
}

/***************************************************************************
*	set_char_int
*
*	set the dexterity of a character
***************************************************************************/
static bool set_char_int(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: int [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->perm_stat[STAT_INT], argument);
	vch->perm_stat[STAT_INT] = URANGE(3, vch->perm_stat[STAT_INT], get_max_train(vch, STAT_INT));
	return TRUE;
}

/***************************************************************************
*	set_char_wis
*
*	set the wisdom of a character
***************************************************************************/
static bool set_char_wis(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: wis [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->perm_stat[STAT_WIS], argument);
	vch->perm_stat[STAT_WIS] = URANGE(3, vch->perm_stat[STAT_WIS], get_max_train(vch, STAT_WIS));
	return TRUE;
}


/***************************************************************************
*	set_char_dex
*
*	set the dexterity of a character
***************************************************************************/
static bool set_char_dex(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: dex [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->perm_stat[STAT_DEX], argument);
	vch->perm_stat[STAT_DEX] = URANGE(3, vch->perm_stat[STAT_DEX], get_max_train(vch, STAT_DEX));
	return TRUE;
}

/***************************************************************************
*	set_char_con
*
*	set the constitution of a character
***************************************************************************/
static bool set_char_con(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: con [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->perm_stat[STAT_CON], argument);
	vch->perm_stat[STAT_CON] = URANGE(3, vch->perm_stat[STAT_CON], get_max_train(vch, STAT_CON));
	return TRUE;
}

/***************************************************************************
*	set_char_luck
*
*	set the luck of a character
***************************************************************************/
static bool set_char_luck(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: luck [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->perm_stat[STAT_LUCK], argument);
	vch->perm_stat[STAT_LUCK] = URANGE(3, vch->perm_stat[STAT_LUCK], get_max_train(vch, STAT_LUCK));
	return TRUE;
}


/***************************************************************************
*	set_char_sex
*
*	set the sex of a character
***************************************************************************/
static bool set_char_sex(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: sex <sex name>\n\r", ch);
		return FALSE;
	}

	value = sex_lookup(argument);
	if (value < 0 || value > 2) {
		send_to_char("That is not a valid sex.\n\r", ch);
		return FALSE;
	}

	vch->sex = value;
	if (!IS_NPC(vch))
		vch->pcdata->true_sex = value;
	return TRUE;
}


/***************************************************************************
*	set_char_race
*
*	set the race of a character
***************************************************************************/
static bool set_char_race(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	OBJ_DATA *obj;
	int value;
	char buf[MSL];
	bool gobbed = FALSE;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: race <race name>\n\r", ch);
		return FALSE;
	}

	value = race_lookup(argument);
	if (value == -1) {
		send_to_char("That is not a valid race.\n\r", ch);
		return FALSE;
	}

	if (!IS_NPC(vch) && !race_table[value].pc_race) {
		send_to_char("That is not a valid player race.\n\r", ch);
		return FALSE;
	}

	/* no sense in beating a dead horse...
	*  added by Monrick, 1/2008            */
	if (vch->race == value) {
		printf_to_char(ch, "%s is already a %s!\n\r",
			       capitalize(IS_NPC(vch) ? vch->short_descr : vch->name),
			       race_table[value].name);
		return FALSE;
	}

	/* check for permanent racial specials to remove
	 * added by Monrick, 1/2008            */
	if (!IS_NPC(vch)) {
		sprintf(buf, "Changing %s from ", capitalize(vch->name));
		strcat(buf, capitalize(race_table[vch->race].name));
		strcat(buf, " to ");
		strcat(buf, capitalize(race_table[value].name));
		strcat(buf, ".\n\r");
		send_to_char(buf, ch);

		printf_to_char(ch, "  Removing %s special effects...\n\r", race_table[vch->race].name);

		/* when you're not a vampire anymore, there's no need
		 *         to feed... auto-checks for gobstopper */
		if (vch->race == race_lookup("vampire")) {
			printf_to_char(ch, "    Changing feed to hunger/thirst...\n\r");

			if (vch->pcdata->condition[COND_FEED] == -151) {
				printf_to_char(ch, "    Gobstopper detected...Transferred...\n\r");
				gobbed = TRUE;
			} else {
				send_to_char("You will never again feel the lust for `1blood``...\n\r", vch);
				gobbed = FALSE;
			}

			vch->pcdata->condition[COND_HUNGER] = vch->pcdata->condition[COND_FEED];
			vch->pcdata->condition[COND_THIRST] = vch->pcdata->condition[COND_FEED];
			vch->pcdata->condition[COND_FEED] = -151;


			send_to_char("    Feed successfully switched to hunger/thirst.\n\r", ch);
		}

		/* when you're not a mutant anymore, get rid of that
		 *     third weapon */
		if (vch->race == race_lookup("mutant")) {
			sprintf(buf, "    Checking for third arm...");

			if (IS_SET(vch->comm2, COMM2_THIRDARM)) {
				strcat(buf, "found... Checking for third-wield weapon...");
				if ((obj = get_eq_char(vch, WEAR_THIRD)) != NULL) {
					strcat(buf, "found... Removing...\n\r");
					send_to_char(buf, ch);
					unequip_char(vch, obj);
					act("$n stops using $p.", vch, obj, NULL, TO_ROOM);
					act("You stop using $p.", vch, obj, NULL, TO_CHAR);
				} else {
					strcat(buf, "none.\n\r");
					send_to_char(buf, ch);
				}
				REMOVE_BIT(vch->comm2, COMM2_THIRDARM);
				send_to_char("    Third arm removed.\n\r", ch);
				act("$n has surgically removed your third arm.", ch, NULL, vch, TO_VICT);
			}
		}

		/* when you're not a human anymore, get rid of those
		 *     extra rings */
		if (vch->race == race_lookup("human")) {
			send_to_char("    Checking for extra rings and removing...\n\r", ch);
			if ((obj = get_eq_char(vch, WEAR_FINGER_L2)) != NULL) {
				unequip_char(vch, obj);
				act("$n stops being so greedy and removes $p.", vch, obj, NULL, TO_ROOM);
				act("You stop being so greedy and remove $p.", vch, obj, NULL, TO_CHAR);
			}
			if ((obj = get_eq_char(vch, WEAR_FINGER_R2)) != NULL) {
				unequip_char(vch, obj);
				act("$n stops being so greedy and removes $p.", vch, obj, NULL, TO_ROOM);
				act("You stop being so greedy and remove $p.", vch, obj, NULL, TO_CHAR);
			}
		}

		printf_to_char(ch, "  All %s special effects removed.\n\r", race_table[vch->race].name);
		printf_to_char(vch, "Your body morphs from a %s into a ",
			       capitalize(race_table[vch->race].name));
		printf_to_char(vch, "%s.\n\r", capitalize(race_table[value].name));
	}

	vch->race = value;

	if (!IS_NPC(vch)) {
		vch->exp = (exp_per_level(vch, vch->pcdata->points) * vch->level);

		sprintf(buf, "$N is now a %s.",
			capitalize(race_table[vch->race].name));
		act(buf, ch, NULL, vch, TO_CHAR);

		printf_to_char(ch, "  Adding %s special effects...\n\r", race_table[vch->race].name);

		/* now that you're a vampire, you'll need to feed instead of
		 * ... auto-checks for gobstopper */
		if (vch->race == race_lookup("vampire")) {
			printf_to_char(ch, "    Changing hunger/thirst to feed...\n\r");

			if ((vch->pcdata->condition[COND_HUNGER] == -151)
			    && (vch->pcdata->condition[COND_THIRST] == -151)) {
				printf_to_char(ch, "    Gobstopper detected...Transferred...\n\r");
				gobbed = TRUE;
			} else {
				send_to_char("You will never again need to eat, though you might feel a bit `1th`!i`1rsty``...\n\r", vch);
				gobbed = FALSE;
			}

			vch->pcdata->condition[COND_FEED] = UMIN(vch->pcdata->condition[COND_HUNGER],
								 vch->pcdata->condition[COND_THIRST]);
			vch->pcdata->condition[COND_HUNGER] = -151;
			vch->pcdata->condition[COND_THIRST] = -151;

			send_to_char("    Hunger/thirst successfully switched to feed.\n\r", ch);
		}

		if (gobbed)
			send_to_char("Your `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`` effect is still in place...\n\r", vch);

		printf_to_char(ch, "  All %s special effects added.\n\r", race_table[vch->race].name);
		do_save(vch, "");
	}

	return TRUE;
}

/***************************************************************************
*	set_char_hp
*
*	set the hp of a character
***************************************************************************/
static bool set_char_hp(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: hp [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->max_hit, argument);
	vch->max_hit = UMAX(1, vch->max_hit);

	if (!IS_NPC(vch)) {
		set_integer_arg(&vch->pcdata->perm_hit, argument);
		vch->pcdata->perm_hit = UMAX(1, vch->pcdata->perm_hit);
	}
	return TRUE;
}

/***************************************************************************
*	set_char_mana
*
*	set the alignment of a character
***************************************************************************/
static bool set_char_mana(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: mana [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->max_mana, argument);
	vch->max_mana = UMAX(1, vch->max_mana);

	if (!IS_NPC(vch)) {
		set_integer_arg(&vch->pcdata->perm_mana, argument);
		vch->pcdata->perm_mana = UMAX(1, vch->pcdata->perm_mana);
	}
	return TRUE;
}

/***************************************************************************
*	set_char_move
*
*	set the alignment of a character
***************************************************************************/
static bool set_char_move(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: move [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->max_move, argument);
	vch->max_move = UMAX(1, vch->max_move);

	if (!IS_NPC(vch)) {
		set_integer_arg(&vch->pcdata->perm_move, argument);
		vch->pcdata->perm_move = UMAX(1, vch->pcdata->perm_move);
	}
	return TRUE;
}


/***************************************************************************
*	set_char_align
*
*	set the alignment of a character
***************************************************************************/
static bool set_char_align(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: align [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->alignment, argument);
	vch->alignment = URANGE(-1000, vch->alignment, 1000);
	return TRUE;
}

/***************************************************************************
*	set_char_gold
*
*	set the gold property of a character
***************************************************************************/
static bool set_char_gold(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: gold [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_uint_arg(&vch->gold, argument);
	vch->gold = UMAX(vch->gold, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_silver
*
*	set the silver in bank property of a character
***************************************************************************/
static bool set_char_silver(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: silver [+/-]<number>\n\r", ch);
		return FALSE;
	}

	set_uint_arg(&vch->silver, argument);
	vch->silver = UMAX(vch->silver, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_reply
*
*	set the reply property for a character
***************************************************************************/
static bool set_char_reply(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	CHAR_DATA *reply;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: reply <character to reply to>\n\r", ch);
		return FALSE;
	}

	if ((reply = get_char_world(ch, argument)) == NULL) {
		send_to_char("That character does not exist.\n\r", ch);
		return FALSE;
	}

	vch->reply = reply;
	return TRUE;
}


/***************************************************************************
*	player-specific sets
***************************************************************************/
/***************************************************************************
*	set_char_class
*
*	set the class of a character
***************************************************************************/
static bool set_char_class(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: class <class name>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	value = class_lookup(argument);
	if (value == -1) {
		send_to_char("That is not a valid class. Valid classes are:\n\r", ch);
		for (value = 0; value < MAX_CLASS; value++)

			printf_to_char(ch, "  %s", class_table[value].name);
		send_to_char("\n\r", ch);
		return FALSE;
	}

	vch->class = value;
	vch->exp = (exp_per_level(vch, vch->pcdata->points) * vch->level);
	return TRUE;
}

/***************************************************************************
*	set_char_train
*
*	set the number of trains for a player
***************************************************************************/
static bool set_char_train(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: train [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->pcdata->train, argument);
	vch->pcdata->train = UMAX(0, vch->pcdata->train);
	return TRUE;
}

/***************************************************************************
*	set_char_practice
*
*	set the number of practices for a player
***************************************************************************/
static bool set_char_practice(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: practice [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->pcdata->practice, argument);
	vch->pcdata->practice = UMAX(0, vch->pcdata->practice);
	return TRUE;
}

/***************************************************************************
*	set_char_bank_gold
*
*	set the gold in bank property of a player
***************************************************************************/
static bool set_char_bank_gold(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: bank_gold [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_uint_arg(&vch->pcdata->gold_in_bank, argument);
	vch->pcdata->gold_in_bank = UMAX(vch->pcdata->gold_in_bank, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_bank_silver
*
*	set the silver in bank property of a player
***************************************************************************/
static bool set_char_bank_silver(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: bank_silver [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_uint_arg(&vch->pcdata->silver_in_bank, argument);
	vch->pcdata->silver_in_bank = UMAX(vch->pcdata->silver_in_bank, 0);
	return TRUE;
}

/***************************************************************************
*	set_char_pkills
*
*	set the player kills property of a player
***************************************************************************/
static bool set_char_pkills(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: pkills [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_long_arg(&vch->pcdata->pkills, argument);
	vch->pcdata->pkills = UMAX(vch->pcdata->pkills, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_pdeaths
*
*	set the player deaths property of a player
***************************************************************************/
static bool set_char_pdeaths(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: pdeaths [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_long_arg(&vch->pcdata->pdeaths, argument);
	vch->pcdata->pdeaths = UMAX(vch->pcdata->pdeaths, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_mobkills
*
*	set the mob kills property of a player
***************************************************************************/
static bool set_char_mobkills(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: mobkills [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_long_arg(&vch->pcdata->mobkills, argument);
	vch->pcdata->mobkills = UMAX(vch->pcdata->mobkills, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_mobdeaths
*
*	set the mob deaths property of a player
***************************************************************************/
static bool set_char_mobdeaths(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: mobdeaths [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_long_arg(&vch->pcdata->mobdeaths, argument);
	vch->pcdata->mobdeaths = UMAX(vch->pcdata->mobdeaths, 0);
	return TRUE;
}


/***************************************************************************
*	set_char_full
*
*	set the full condition of a player
***************************************************************************/
static bool set_char_full(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: full [+/-]<number of ticks>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	if (!is_number(argument)) {
		send_to_char("The supplied value must be numeric.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);

	if (value < -151 || value > 100) {
		send_to_char("The value must be between -151 and 100.\n\r", ch);
		return FALSE;
	}

	if (value == -151 && get_trust(ch) < 609) {
		send_to_char("Only Implementors may set permanent values.\n\r", ch);
		return FALSE;
	}
	vch->pcdata->condition[COND_FULL] = value;
	return TRUE;
}


/***************************************************************************
*	set_char_hunger
*
*	set the hunger condition of a player
***************************************************************************/
bool set_char_hunger(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: hunger [+/-]<number of ticks>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	if (!is_number(argument)) {
		send_to_char("The supplied value must be numeric.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);
	if (value < -151 || value > 100) {
		send_to_char("The value must be between -151 and 100.\n\r", ch);
		return FALSE;
	}

	if (value == -151 && get_trust(ch) < 609) {
		send_to_char("Only Implementors may set permanent values.\n\r", ch);
		return FALSE;
	}
	vch->pcdata->condition[COND_HUNGER] = value;
	return TRUE;
}


/***************************************************************************
*	set_char_thirst
*
*	set the thirst condition of a player
***************************************************************************/
bool set_char_thirst(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: thirst [+/-]<number of ticks>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	if (!is_number(argument)) {
		send_to_char("The supplied value must be numeric.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);
	if (value < -151 || value > 100) {
		send_to_char("The value must be between -151 and 100.\n\r", ch);
		return FALSE;
	}

	if (value == -151 && get_trust(ch) < 609) {
		send_to_char("Only Implementors may set permanent values.\n\r", ch);
		return FALSE;
	}
	vch->pcdata->condition[COND_THIRST] = value;
	return TRUE;
}

/***************************************************************************
*	set_char_feed
*
*	set the feed condition of a player
***************************************************************************/
bool set_char_feed(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: feed [+/-]<number of ticks>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	if (!is_number(argument)) {
		send_to_char("The supplied value must be numeric.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);
	if (value < -151 || value > 100) {
		send_to_char("The value must be between -151 and 100.\n\r", ch);
		return FALSE;
	}

	if (value == -151 && get_trust(ch) < 609) {
		send_to_char("Only Implementors may set permanent values.\n\r", ch);
		return FALSE;
	}
	vch->pcdata->condition[COND_FEED] = value;
	return TRUE;
}


/***************************************************************************
*	set_char_bfield_enter
*
*	set the battlefield enters property of a player
***************************************************************************/
static bool set_char_bfield_enter(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: bfield_enter [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->benter, argument);
	vch->benter = UMAX(vch->benter, 0);
	return TRUE;
}

/***************************************************************************
*	set_char_bfield_loss
*
*	set the battlefield losses property of a player
***************************************************************************/
static bool set_char_bfield_loss(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: bfield_loss [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->bloss, argument);
	vch->bloss = UMAX(vch->bloss, 0);
	return TRUE;
}

/***************************************************************************
*	set_char_bfield_kills
*
*	set the battlefield kills property of a player
***************************************************************************/
static bool set_char_bfield_kills(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: bfield_kills [+/-]<number>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&vch->bkills, argument);
	vch->bkills = UMAX(vch->bkills, 0);
	return TRUE;
}

/***************************************************************************
*	set_char_deathroom
*
*	set the deathroom room for a player
***************************************************************************/
static bool set_char_deathroom(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	ROOM_INDEX_DATA *room;
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: deathroom <room vnum>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);
	if (value != 0
	    && (room = get_room_index(value)) == NULL) {
		send_to_char("That room does not exist.\n\r", ch);
		return FALSE;
	}

	vch->deathroom = value;
	return TRUE;
}

/***************************************************************************
*	set_char_security
*
*	set the security property of a player
***************************************************************************/
static bool set_char_security(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: security <0-9>\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on mobs.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);
	if (value < 0 || value > 9) {
		send_to_char("Security must be between 0 and 9.\n\r", ch);
		return FALSE;
	}

	vch->pcdata->security = value;
	return TRUE;
}






/***************************************************************************
*	set_char_extendedexp
*
*	set the extended exp for a pc (added by Monrick, May 2008)
***************************************************************************/
static bool set_char_extendedexp(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int plusminus = 0;
	int value;
	char arg1[MIL];
	char arg2[MIL];

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: extendedexp <value> [+, - or blank to set a #]\n\r", ch);
		return FALSE;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on NPCs.\n\r", ch);
		return FALSE;
	}

	if (get_trust(ch) < MAX_LEVEL) {
		send_to_char("I don't care if you have \"set\" or not, only `2I`3M`2P``s may set ExtendedExp.\n\r", ch);
		return FALSE;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("`#SYNTAX``: extendedexp <value> [+, - or blank to set a #]\n\r", ch);
		return FALSE;
	}

	if (arg2[0] == '+')
		plusminus = 1;
	else if (arg2[0] == '-')
		plusminus = -1;

	value = parse_int(arg1);

	if (plusminus != 0)
		vch->pcdata->extendedexp += (plusminus * value);
	else
		vch->pcdata->extendedexp = abs(value);

	return TRUE;
}


/***************************************************************************
*	mob-specific sets
***************************************************************************/
/***************************************************************************
*	set_char_level
*
*	set the level property for a mob
***************************************************************************/
static bool set_char_level(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	int value;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: level <level number>\n\r", ch);
		return FALSE;
	}

	if (!IS_NPC(vch)) {
		send_to_char("Not on players.\n\r", ch);
		return FALSE;
	}

	value = parse_int(argument);
	if (value <= 0) {
		send_to_char("The level must be a positive number.\n\r", ch);
		return FALSE;
	}

	vch->level = value;
	return TRUE;
}


/***************************************************************************
*	set_char_memory
*
*	set the memory property for a mob
***************************************************************************/
static bool set_char_memory(CHAR_DATA *ch, CHAR_DATA *vch, char *argument)
{
	CHAR_DATA *mem;

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: memory <character to remember>\n\r", ch);
		return FALSE;
	}

	if (!IS_NPC(vch)) {
		send_to_char("Not on players.\n\r", ch);
		return FALSE;
	}

	if (!str_cmp(argument, "none")) {
		vch->mobmem = NULL;

		send_to_char("Your target is now happy-happy-joy-joy.\n\r", ch);
	} else {
		if ((mem = get_char_world(vch, argument)) == NULL) {
			send_to_char("Mob couldn't locate the victim to remember.\n\r", ch);
			return FALSE;
		}

		vch->mobmem = mem;
	}

	return TRUE;
}


/***************************************************************************
*	objects
***************************************************************************/
static SET_OBJ_FN set_obj_v0;
static SET_OBJ_FN set_obj_v1;
static SET_OBJ_FN set_obj_v2;
static SET_OBJ_FN set_obj_v3;
static SET_OBJ_FN set_obj_v4;
static SET_OBJ_FN set_obj_extra;
static SET_OBJ_FN set_obj_extra2;
static SET_OBJ_FN set_obj_wear;
static SET_OBJ_FN set_obj_level;
static SET_OBJ_FN set_obj_weight;
static SET_OBJ_FN set_obj_cost;
static SET_OBJ_FN set_obj_timer;


static const struct set_obj_cmd_type {
	char *		keyword;
	SET_OBJ_FN *	fn;
}
set_obj_cmd_table[] =
{
	/* general stats */
	{ "v0",	    set_obj_v0	   },
	{ "v1",	    set_obj_v1	   },
	{ "v2",	    set_obj_v2	   },
	{ "v3",	    set_obj_v3	   },
	{ "v4",	    set_obj_v4	   },
	{ "extra",  set_obj_extra  },
	{ "extra2", set_obj_extra2 },
	{ "wear",   set_obj_wear   },
	{ "level",  set_obj_level  },
	{ "weight", set_obj_weight },
	{ "cost",   set_obj_cost   },
	{ "timer",  set_obj_timer  },
	{ "",	    NULL	   }
};


/***************************************************************************
*	set_object
*
*	set a property on an object
***************************************************************************/
static void set_object(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	char cmd[MIL];
	int idx;

	DENY_NPC(ch);

	smash_tilde(argument);
	if (is_help(argument)) {
		int col;

		send_to_char("`#SYNTAX``: set object <object name> <field> <value>\n\r\n\r", ch);
		send_to_char("Available fields:\n\r   ", ch);
		col = 0;
		for (idx = 0; set_obj_cmd_table[idx].keyword[0] != '\0'; idx++) {
			printf_to_char(ch, "%-15.15s", set_obj_cmd_table[idx].keyword);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 3 != 0)
			send_to_char("\n\r", ch);

		return;
	}

	argument = one_argument(argument, arg);
	if ((obj = get_obj_here(ch, arg)) == NULL) {
		send_to_char("That object is not in this room or in your inventory.\n\r", ch);
		return;
	}

	argument = one_argument(argument, cmd);
	if (cmd[0] != '\0') {
		for (idx = 0; set_obj_cmd_table[idx].keyword[0] != '\0'; idx++) {
			if (!str_prefix(cmd, set_obj_cmd_table[idx].keyword)) {
				if ((*set_obj_cmd_table[idx].fn)(ch, obj, argument))
					send_to_char("`1Ok`!.``\n\r", ch);
				return;
			}
		}
	}

	/* generate help message */
	set_object(ch, "");
	return;
}



/***************************************************************************
*	set_obj_v0
*
*	set the value 0 property of an object
***************************************************************************/
static bool set_obj_v0(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: v0 <value>\n\r\n\r", ch);
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	if (!set_obj_value_idx(obj, 0, argument)) {
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	return TRUE;
}


/***************************************************************************
*	set_obj_v1
*
*	set the value 1 property of an object
***************************************************************************/
static bool set_obj_v1(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: v1 <value>\n\r\n\r", ch);
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	if (!set_obj_value_idx(obj, 1, argument)) {
		item_type_help(ch, obj->item_type);
		return FALSE;
	}
	return TRUE;
}


/***************************************************************************
*	set_obj_v2
*
*	set the value 2 property of an object
***************************************************************************/
static bool set_obj_v2(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: v2 <value>\n\r\n\r", ch);
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	if (!set_obj_value_idx(obj, 2, argument)) {
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	return TRUE;
}

/***************************************************************************
*	set_obj_v3
*
*	set the value 3 property of an object
***************************************************************************/
static bool set_obj_v3(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: v3 <value>\n\r\n\r", ch);
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	if (!set_obj_value_idx(obj, 3, argument)) {
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	return TRUE;
}


/***************************************************************************
*	set_obj_v4
*
*	set the value 0 property of an object
***************************************************************************/
static bool set_obj_v4(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: v4 <value>\n\r\n\r", ch);
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	if (!set_obj_value_idx(obj, 4, argument)) {
		item_type_help(ch, obj->item_type);
		return FALSE;
	}

	return TRUE;
}

/***************************************************************************
*	set_obj_extra
*
*	set the extra property of an object
***************************************************************************/
static bool set_obj_extra(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		int idx;
		int col;

		send_to_char("`#SYNTAX``: extra <numeric value|flag value>\n\r", ch);
		send_to_char("Available flag values:\n\r   ", ch);
		col = 0;
		for (idx = 0; extra_flags[idx].name != NULL; idx++) {
			printf_to_char(ch, "%-15.15s", extra_flags[idx].name);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 3 != 0)
			send_to_char("\n\r", ch);
	}

	if (is_number(argument)) {
		obj->extra_flags = parse_long(argument);
	} else {
		long value;

		if ((value = flag_value(extra_flags, argument)) == NO_FLAG) {
			send_to_char("Those flags are not settable.\n\r", ch);
			return FALSE;
		}

		if (*argument == '+')
			SET_BIT(obj->extra_flags, value);
		else if (*argument == '-')
			REMOVE_BIT(obj->extra_flags, value);
		else
			obj->extra_flags = value;
	}
	return TRUE;
}

/***************************************************************************
*       set_obj_extra2
*
*       set the extra2 property of an object
***************************************************************************/
static bool set_obj_extra2(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		int idx;
		int col;

		send_to_char("`#SYNTAX``: extra2 <numeric value|flag value>\n\r", ch);
		send_to_char("Available flag values:\n\r   ", ch);
		col = 0;
		for (idx = 0; extra2_flags[idx].name != NULL; idx++) {
			printf_to_char(ch, "%-15.15s", extra2_flags[idx].name);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 3 != 0)
			send_to_char("\n\r", ch);
	}

	if (is_number(argument)) {
		obj->extra2_flags = parse_long(argument);
	} else {
		long value;

		if ((value = flag_value(extra2_flags, argument)) == NO_FLAG) {
			send_to_char("Those flags are not settable.\n\r", ch);
			return FALSE;
		}

		if (*argument == '+')
			SET_BIT(obj->extra2_flags, value);
		else if (*argument == '-')
			REMOVE_BIT(obj->extra2_flags, value);
		else
			obj->extra2_flags = value;
	}
	return TRUE;
}

/***************************************************************************
*	set_obj_wear
*
*	set the wear flags property of an object
***************************************************************************/
static bool set_obj_wear(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		int idx;
		int col;

		send_to_char("`#SYNTAX``: wear <numeric value|flag value>\n\r", ch);
		send_to_char("Available flag values:\n\r   ", ch);
		col = 0;
		for (idx = 0; wear_flags[idx].name != NULL; idx++) {
			printf_to_char(ch, "%-15.15s", wear_flags[idx].name);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 3 != 0)
			send_to_char("\n\r", ch);

		return FALSE;
	}

	if (is_number(argument)) {
		obj->wear_flags = parse_long(argument);
	} else {
		long value;

		if ((value = flag_value(wear_flags, argument)) == NO_FLAG) {
			send_to_char("Those flags are not settable.\n\r", ch);
			return FALSE;
		}

		if (*argument == '+')
			SET_BIT(obj->wear_flags, value);
		else if (*argument == '-')
			REMOVE_BIT(obj->wear_flags, value);
		else
			obj->wear_flags = value;

	}
	return TRUE;
}

/***************************************************************************
*	set_obj_level
*
*	set the level of an object
***************************************************************************/
static bool set_obj_level(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: level <level>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&obj->level, argument);
	obj->level = UMAX(obj->level, 0);
	return TRUE;
}


/***************************************************************************
*	set_obj_cost
*
*	set the cost of an object
***************************************************************************/
static bool set_obj_cost(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: cost <amount>\n\r", ch);
		return FALSE;
	}

	set_uint_arg(&obj->cost, argument);
	obj->cost = UMAX(obj->cost, 0);
	return TRUE;
}


/***************************************************************************
*	set_obj_weight
*
*	set the weight of an object
***************************************************************************/
static bool set_obj_weight(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: weight <amount>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&obj->weight, argument);
	obj->weight = UMAX(obj->weight * 10, 0);
	return TRUE;
}


/***************************************************************************
*	set_obj_timer
*
*	set the light value of a room
***************************************************************************/
static bool set_obj_timer(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: timer <number of ticks>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&obj->timer, argument);
	obj->timer = UMAX(obj->timer, -1);
	return TRUE;
}





/***************************************************************************
*	rooms
***************************************************************************/
static SET_ROOM_FN set_room_flags;
static SET_ROOM_FN set_room_sector;
static SET_ROOM_FN set_room_mana_rate;
static SET_ROOM_FN set_room_heal_rate;
static SET_ROOM_FN set_room_light;


static const struct set_room_cmd_type {
	char *		keyword;
	SET_ROOM_FN *	fn;
}
set_room_cmd_table[] =
{
	/* general stats */
	{ "flags",     set_room_flags	  },
	{ "sector",    set_room_sector	  },
	{ "mana_rate", set_room_mana_rate },
	{ "heal_rate", set_room_heal_rate },
	{ "light",     set_room_light	  },
	{ "",	       NULL		  }
};

/***************************************************************************
*	set_room
*
*	set a property on a room structure
***************************************************************************/
static void set_room(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *room;
	char arg[MIL];
	char cmd[MIL];
	int idx;

	DENY_NPC(ch);

	smash_tilde(argument);
	if (is_help(argument)) {
		int col;

		send_to_char("`#SYNTAX``: set room [vnum] <field> <value>\n\r\n\r", ch);
		send_to_char("Available fields:\n\r   ", ch);
		col = 0;
		for (idx = 0; set_room_cmd_table[idx].keyword[0] != '\0'; idx++) {
			printf_to_char(ch, "%-15.15s", set_room_cmd_table[idx].keyword);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 4 != 0)
			send_to_char("\n\r", ch);

		return;
	}

	one_argument(argument, arg);
	if (is_number(arg)
	    && (room = get_room_index(parse_int(arg))) != NULL)
		argument = one_argument(argument, arg);
	else
		room = ch->in_room;

	argument = one_argument(argument, cmd);
	if (cmd[0] != '\0') {
		for (idx = 0; set_room_cmd_table[idx].keyword[0] != '\0'; idx++) {
			if (!str_prefix(cmd, set_room_cmd_table[idx].keyword)) {
				if ((*set_room_cmd_table[idx].fn)(ch, room, argument))
					send_to_char("`1Ok`!.``\n\r", ch);
				return;
			}
		}
	}

	/* generate help message */
	set_room(ch, "");
}



/***************************************************************************
*	general room set functions
***************************************************************************/
/***************************************************************************
*	set_room_flags
*
*	set the flags on a room
***************************************************************************/
static bool set_room_flags(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
	long value;

	if (is_help(argument)) {
		int idx;
		int col;

		send_to_char("`#SYNTAX``: flags [+|-]<flag name list>\n\r\n\r", ch);
		send_to_char("Available values:\n\r   ", ch);
		col = 0;
		for (idx = 0; room_flags[idx].name != NULL; idx++) {
			printf_to_char(ch, "%-15.15s", room_flags[idx].name);
			if (++col % 3 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 3 != 0)
			send_to_char("\n\r", ch);

		return FALSE;
	}

	if ((value = flag_value(room_flags, argument)) == NO_FLAG) {
		send_to_char("Those flags do not exist.\n\r", ch);
		return FALSE;
	} else {
		if (*argument == '+') {
			SET_BIT(room->room_flags, value);
		} else if (*argument == '-') {
			REMOVE_BIT(room->room_flags, value);
		} else {
			room->room_flags = value;;
		}
	}

	return TRUE;
}



/***************************************************************************
*	set_room_sector
*
*	set the sector of a room
***************************************************************************/
static bool set_room_sector(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
	int value;

	if (is_help(argument)) {
		int idx;
		int col;

		send_to_char("`#SYNTAX``: sector <sector name >\n\r\n\r", ch);
		send_to_char("Available values:\n\r   ", ch);
		col = 0;
		for (idx = 0; sector_flags[idx].name != NULL; idx++) {
			printf_to_char(ch, "%-11.11s", sector_flags[idx].name);
			if (++col % 4 == 0)
				send_to_char("\n\r   ", ch);
		}

		if (col % 4 != 0)
			send_to_char("\n\r", ch);

		return FALSE;
	}

	if ((value = flag_value(sector_flags, argument)) == NO_FLAG) {
		send_to_char("That sector type does not exist.\n\r", ch);
		return FALSE;
	}

	room->sector_type = value;
	return TRUE;
}

/***************************************************************************
*	set_room_mana_rate
*
*	set the mana heal rate on a room
***************************************************************************/
static bool set_room_mana_rate(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: mana_rate <rate number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&room->mana_rate, argument);
	room->mana_rate = UMAX(room->mana_rate, 0);
	return TRUE;
}

/***************************************************************************
*	set_room_heal_rate
*
*	set the hp heal rate on a room
***************************************************************************/
static bool set_room_heal_rate(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: heal_rate <rate number>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&room->heal_rate, argument);
	room->heal_rate = UMAX(room->heal_rate, 0);
	return TRUE;
}


/***************************************************************************
*	set_room_light
*
*	set the light value of a room
***************************************************************************/
static bool set_room_light(CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument)
{
	if (is_help(argument)) {
		send_to_char("`#SYNTAX``: light <light duration>\n\r", ch);
		return FALSE;
	}

	set_integer_arg(&room->light, argument);
	room->light = UMAX(room->light, -1);
	return TRUE;
}


/***************************************************************************
*	skills
***************************************************************************/
/***************************************************************************
*	set_skill
*
*	set a characters learend skill information
***************************************************************************/
static void set_skill(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch;
	SKILL *skill;
	LEARNED *learned;
	char arg[MIL];
	int percent;

	DENY_NPC(ch);

	if (is_help(argument)) {
		send_to_char("`#SYNTAX``:\n\r", ch);
		send_to_char("  set skill <character name> <spell or skill name> <value>\n\r", ch);
		send_to_char("  set skill <character name> all <value>\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);

	if ((vch = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(vch)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}


	argument = one_argument(argument, arg);
	if (!is_number(argument)) {
		send_to_char("Value must be numeric.\n\r", ch);
		return;
	}

	percent = parse_int(argument);
	if (percent < 0 || percent > 100) {
		send_to_char("Value range is 0 to 100.\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		for (skill = skill_list; skill != NULL; skill = skill->next) {
			learned = new_learned();
			learned->skill = skill;
			learned->percent = percent;
			learned->type = LEARNED_TYPE_SKILL;

			add_learned_skill(vch, learned);
		}
	} else {
		if ((skill = skill_lookup(arg)) == NULL) {
			send_to_char("No such skill or spell.\n\r", ch);
			return;
		}

		if ((learned = get_learned_skill(vch, skill)) != NULL) {
			learned->percent = percent;
		} else {
			learned = new_learned();
			learned->skill = skill;
			learned->percent = percent;
			learned->type = LEARNED_TYPE_SKILL;

			add_learned_skill(vch, learned);
		}
	}
}



/***************************************************************************
*	set_reboot
*
*	initiate or cancel a reboot sequence
***************************************************************************/
static void set_reboot(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[MIL];
	int num_ticks;

	if (get_trust(ch) < MAX_LEVEL) {
		send_to_char("Not at your level.\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (is_number(arg)) {
		num_ticks = parse_int(arg);

		if (num_ticks > 0) {
			if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
				sprintf(buf, " `!R`1eboot sequence initiated by `8someone``.");
			else
				sprintf(buf, " `!R`1eboot sequence initiated by `&%s``.", ch->name);

			do_echo(NULL, buf);

			sprintf(buf, " Reboot in %d ticks.", num_ticks);
			do_echo(NULL, buf);

			reboot_tick_counter = num_ticks;
		} else {
			if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
				sprintf(buf, " Reboot sequence terminated by someone.");
			else
				sprintf(buf, " Reboot sequence terminated by %s.", ch->name);

			do_echo(NULL, buf);

			reboot_tick_counter = -1;
		}
	} else {
		send_to_char("Value must be numeric.\n\r", ch);
	}
}



/***************************************************************************
*	set_copyover
*	shamelessly copied from the above by Dalamar
*	initiate or cancel a copyover sequence
***************************************************************************/
static void set_copyover(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[MIL];
	int num_ticks;

	if (get_trust(ch) < MAX_LEVEL) {
		send_to_char("Not at your level.\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (is_number(arg)) {
		num_ticks = parse_int(arg);

		if (num_ticks > 0) {
			if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
				sprintf(buf, " `!C`1opyover sequence initiated by `8someone``.");
			else
				sprintf(buf, " `!C`1opyover sequence initiated by `&%s``.", ch->name);

			do_echo(NULL, buf);

			sprintf(buf, " Copyover in %d ticks.", num_ticks);
			do_echo(NULL, buf);

			copyover_tick_counter = num_ticks;
		} else {
			if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
				sprintf(buf, " Copyover sequence terminated by someone.");
			else
				sprintf(buf, " Copyover sequence terminated by %s.", ch->name);

			do_echo(NULL, buf);

			copyover_tick_counter = -1;
		}
	} else {
		send_to_char("Value must be numeric.\n\r", ch);
	}
}




/***************************************************************************
*	set function utilities
***************************************************************************/
/***************************************************************************
*	set_integer_arg
*
*	set a integer number argument
***************************************************************************/
static bool set_integer_arg(int *value, char *argument)
{
	switch (argument[0]) {
	case '+':
		argument++;
		*value += parse_int(argument);
		return TRUE;
	case '-':
		argument++;
		*value -= parse_int(argument);
		return TRUE;
	default:
		if (is_number(argument)) {
			*value = parse_int(argument);
			return TRUE;
		}
	}

	return FALSE;
}

static bool set_uint_arg(unsigned int *value, char *argument)
{
	switch (argument[0]) {
	case '+':
		argument++;
		*value += parse_long(argument);
		return TRUE;
	case '-':
		argument++;
		*value -= parse_long(argument);
		return TRUE;
	default:
		if (is_number(argument)) {
			*value = parse_unsigned_int(argument);
			return TRUE;
		}
	}

	return FALSE;
}



/***************************************************************************
*	set_long_arg
*
*	set a long number argument
***************************************************************************/
static bool set_long_arg(long *value, char *argument)
{
	switch (argument[0]) {
	case '+':
		argument++;
		*value += parse_long(argument);
		return TRUE;

	case '-':
		argument++;
		*value -= parse_long(argument);
		return TRUE;

	default:
		if (is_number(argument)) {
			*value = parse_long(argument);
			return TRUE;
		} else {
			return FALSE;
		}
	}
}


/*****************************************************************************
*	set_obj_value_idx
*
*	set an indexed value property of an item based on it's type
*****************************************************************************/
static bool set_obj_value_idx(OBJ_DATA *obj, int idx, char *argument)
{
	SKILL *skill;
	int value;

	if (is_number(argument)) {
		obj->value[idx] = parse_int(argument);
		return TRUE;
	} else {
		switch (obj->item_type) {
		default:
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			switch (idx) {
			default:
				break;
			case 3:
				if ((skill = skill_lookup(argument)) != NULL) {
					obj->value[3] = skill->sn;
					return TRUE;
				}
			}
			break;

		case ITEM_SCROLL:
		case ITEM_POTION:
		case ITEM_PILL:
			switch (idx) {
			default:
				break;
			case 1:
				if ((skill = skill_lookup(argument)) != NULL) {
					obj->value[1] = skill->sn;
					return TRUE;
				}
				break;
			case 2:
				if ((skill = skill_lookup(argument)) != NULL) {
					obj->value[2] = skill->sn;
					return TRUE;
				}
				break;
			case 3:
				if ((skill = skill_lookup(argument)) != NULL) {
					obj->value[3] = skill->sn;
					return TRUE;
				}
				break;
			case 4:
				if ((skill = skill_lookup(argument)) != NULL) {
					obj->value[4] = skill->sn;
					return TRUE;
				}
				break;
			}
			break;
		case ITEM_WEAPON:
			switch (idx) {
			default:
				break;
			case 0:
				ALT_FLAGVALUE_SET(obj->value[0], weapon_class, argument);
				return TRUE;
			case 3:
				obj->value[3] = attack_lookup(argument);
				return TRUE;
			case 4:
				ALT_FLAGVALUE_TOGGLE(obj->value[4], weapon_flag_type, argument);
				return TRUE;
			}
			break;

		case ITEM_PORTAL:
			switch (idx) {
			default:
				break;
			case 1:
				ALT_FLAGVALUE_SET(obj->value[1], exit_flags, argument);
				return TRUE;
			case 2:
				ALT_FLAGVALUE_SET(obj->value[2], portal_flags, argument);
				return TRUE;
			}
			break;
		case ITEM_FURNITURE:
			switch (idx) {
			default:
				break;
			case 2:
				ALT_FLAGVALUE_TOGGLE(obj->value[2], furniture_flags, argument);
				return TRUE;
			}
			break;
		case ITEM_CONTAINER:
			switch (idx) {
			default:
				break;
			case 1:
				if ((value = flag_value(container_flags, argument)) != NO_FLAG) {
					TOGGLE_BIT(obj->value[1], value);
					return TRUE;
				}
				break;
			}
			break;

		case ITEM_DRINK_CON:
		case ITEM_FOUNTAIN:
			switch (idx) {
			default:
				break;
			case 2:
				if ((value = liq_lookup(argument)) >= 0) {
					obj->value[2] = value;
					return TRUE;
				}
				break;
			}
			break;

		case ITEM_SOCKETS:
			switch (idx) {
			default:
				break;
			case 0:
				if ((value = flag_value(socket_flags, argument)) != NO_FLAG) {
					obj->value[0] = value;
					return TRUE;
				}
				break;
			case 1:
				if ((value = flag_value(socket_values, argument)) != NO_FLAG) {
					obj->value[1] = value;
					return TRUE;
				}
				break;
			}
			break;
		}
	}

	return FALSE;
}



/*****************************************************************************
*	item_type_help
*
*	get help for an item type
*****************************************************************************/
static void item_type_help(CHAR_DATA *ch, int item_type)
{
	int idx;

	for (idx = 0; item_table[idx].type != 0; idx++) {
		if (item_table[idx].type == item_type) {
			if (item_table[idx].help_keyword[0] != '\0') {
				do_help(ch, item_table[idx].help_keyword);
				return;
			}
		}
	}
}
