/**************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,        *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael          *
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
*       Russ Taylor(rtaylor@hypercube.org)                                *
*       Gabrielle Taylor(gtaylor@hypercube.org)                           *
*       Brian Moore(zump@rom.org)                                         *
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
#include "magic.h"
#include "interp.h"

static const struct heal_cmds {
	char *		cmd;
	char *		skill;
	char *		words;
	unsigned int	cost;
}
heal_cmd_table[] =
{
	{ "light",     "cure light",	    "fazoozle gwaargh",	    20000 },
	{ "serious",   "cure serious",	    "fnord!",		    30000 },
	{ "critical",  "cure critical",	    "judicandus scrubette", 50000 },
	{ "heal",      "heal",		    "scrubus maximus",	    40000 },
	{ "blindness", "cure blindness",    "judicandus noselacri", 25000 },
	{ "disease",   "cure disease",	    "judicandus eugzagz",   25000 },
	{ "blood",     "cure blood",	    "judicandus jhena",	    55000 },
	{ "poison",    "cure poison",	    "judicandus sausabru",  25000 },
	{ "curse",     "remove curse",	    "candussido judifgz",   50000 },
	{ "uncurse",   "remove curse",	    "candussido judifgz",   50000 },
	{ "mana",      "heal mana",	    "energizer maximus",    25000 },
	{ "energze",   "heal mana",	    "sumixam rezigrene",    25000 },
	{ "refresh",   "refresh",	    "candusima",	    15000 },
	{ "moves",     "refresh",	    "amisudnac",	    15000 },
	{ "flames",    "extinguish flames", "pie sesu domine",	    10000 },
	{ "dispel",    "dispel magic",	    "eugszr waouq",	    50000 },
	{ "",	       "",		    "",			    0	  },
};

void do_heal(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *acid;
	SKILL *skill;
	char arg[MIL];
	unsigned int cost;
	char *words;
	int idx;

	/* check for healer */
	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER))
			break;

	if (mob == NULL) {
		send_to_char("You can't do that here.\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	mob->alignment = ch->alignment;

	if (argument[0] == '\0') {
		/* display price list */
		act("$N says '```&I offer the following spells:``'", ch, NULL, mob, TO_CHAR);
		send_to_char("  light```8:   ``cure light wounds     ```#200``gold\n\r", ch);
		send_to_char("  serious```8: ``cure serious wounds   ```#300``gold\n\r", ch);
		send_to_char("  critic```8:  ``cure critical wounds  ```#500``gold\n\r", ch);
		send_to_char("  heal```8:    ``healing spell         ```#400``gold\n\r", ch);
		send_to_char("  blind```8:   ``cure blindness        ```#250``gold\n\r", ch);
		send_to_char("  disease```8: ``cure disease          ```#250``gold\n\r", ch);
		send_to_char("  poison```8:  ``cure poison           ```#250``gold\n\r", ch);
		send_to_char("  blood`8:   ``cure blood diseases   ```#550``gold\n\r", ch);
		send_to_char("  uncurse```8: ``remove curse          ```#500``gold\n\r", ch);
		send_to_char("  refresh```8: ``restore movement      ```#150``gold\n\r", ch);
		send_to_char("  mana```8:    ``restore mana          ```#250``gold\n\r", ch);
		send_to_char("  flames```8:  ``extinguish flames     ```#100``gold\n\r", ch);
		send_to_char("  dispel```8:  ``dispel magic	     ```#500``gold\n\r", ch);
		send_to_char(" Type heal <type> to be healed.\n\r", ch);
		return;
	}


	skill = NULL;
	for (idx = 0; heal_cmd_table[idx].cmd[0] != '\0'; idx++) {
		if (!str_prefix(arg, heal_cmd_table[idx].cmd)) {
			skill = skill_lookup(heal_cmd_table[idx].skill);
			words = heal_cmd_table[idx].words;
			cost = heal_cmd_table[idx].cost;
			break;
		}
	}


	if (skill == NULL) {
		act("$N says '```&Type 'heal' for a list of spells.``'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_IDIOT)) {
		act("$N says 'You're an `#idiot`7!  I don't want your gold!'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (cost > ((unsigned int)(ch->gold * 100) + ch->silver)) {
		if ((ch->gold > 20000000u) || (ch->silver > 20000000u)) {
			/* hack by dalamar to compensate for overflows in the calculation.
			 * act("$N says 'Dalamar's hack is 1337.'", ch, NULL, mob, TO_CHAR);
			 */
		} else {
			act("$N says 'You do not have enough giznold for my services.'", ch, NULL, mob, TO_CHAR);
			return;
		}
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	deduct_cost(ch, cost);
	mob->gold += cost;
	act("$n utters the words '```@$T``'.", mob, NULL, words, TO_ROOM);

	if (number_range(1, 300) == 13) {
		send_to_char("A quivering cube of `@green`` jello whispers '`8Psst! here!``'\nA quivering cube of `@green`` jello hands you something.\n", ch);

		acid = create_object(get_obj_index(32), 0);
		obj_to_char(acid, ch);
	}

	if (number_range(1, 300) == 7) {
		send_to_char("A quivering cube of `@green`` jello whispers '`8Psst! here!``'\nA quivering cube of `@green`` jello hands you something.\n", ch);

		acid = create_object(get_obj_index(470), 0);
		obj_to_char(acid, ch);
	}

	cast_spell(mob, skill, mob->level, ch, TARGET_CHAR, "");
}
