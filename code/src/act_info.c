/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
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
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "ansi.h"
#include "interp.h"
#include "libstring.h"

extern byte parse_byte(char *string);
extern byte parse_byte2(char *string, byte min, byte max);


void do_at(CHAR_DATA * ch, char *argument);
HELP_DATA *find_help(CHAR_DATA * ch, char *name);
void print_weather(CHAR_DATA * ch);

static struct {
	int	wear_loc;
	char *	desc;
} where_name[] =
{
	{ WEAR_LIGHT,	  "<used as light>     " },
	{ WEAR_FINGER_L,  "<worn on finger>    " },
	{ WEAR_FINGER_R,  "<worn on finger>    " },
	{ WEAR_FINGER_L2, "<worn on finger>    " },
	{ WEAR_FINGER_R2, "<worn on finger>    " },
	{ WEAR_NECK_1,	  "<worn around neck>  " },
	{ WEAR_NECK_2,	  "<worn around neck>  " },
	{ WEAR_BODY,	  "<worn on torso>     " },
	{ WEAR_HEAD,	  "<worn on head>      " },
	{ WEAR_FACE,	  "<worn on face>      " },
	{ WEAR_EAR_L,	  "<worn in left ear>  " },
	{ WEAR_EAR_R,	  "<worn in right ear> " },
	{ WEAR_LEGS,	  "<worn on legs>      " },
	{ WEAR_FEET,	  "<worn on feet>      " },
	{ WEAR_HANDS,	  "<worn on hands>     " },
	{ WEAR_ARMS,	  "<worn on arms>      " },
	{ WEAR_SHIELD,	  "<worn as shield>    " },
	{ WEAR_ABOUT,	  "<worn about body>   " },
	{ WEAR_WAIST,	  "<worn about waist>  " },
	{ WEAR_WRIST_L,	  "<worn around wrist> " },
	{ WEAR_WRIST_R,	  "<worn around wrist> " },
	{ WEAR_WIELD,	  "<wielded>           " },
	{ WEAR_HOLD,	  "<held>              " },
	{ WEAR_FLOAT,	  "<floating nearby>   " },
	{ WEAR_SECONDARY, "<secondary weapon>  " },
	{ WEAR_THIRD,	  "<third weapon>      " },
	{ WEAR_TATTOO,	  "<worn as a tattoo>  " },
	{ -1,		  ""			 }
};


/*
 * Local functions.
 */
char *format_obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch, bool fShort);
void show_char_to_char_0(CHAR_DATA * victim, CHAR_DATA * ch);
void show_char_to_char_1(CHAR_DATA * victim, CHAR_DATA * ch);
void show_char_to_char_2(CHAR_DATA * victim, CHAR_DATA * ch);
void show_char_to_char(CHAR_DATA * list, CHAR_DATA * ch);
bool check_blind(CHAR_DATA * ch);

#define MAX_NEST        100
static OBJ_DATA *rgObjNest[MAX_NEST];
extern void fread_char(CHAR_DATA * ch, FILE * fp);


/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing)
{
	OBJ_DATA *obj;
	char buf[13000];
	char **prgpstrShow;
	char *pstrShow;
	int *prgnShow;
	int nShow;
	int iShow;
	unsigned int count;
	bool fCombine;

	if (ch->desc == NULL)
		return;

/*
 * Alloc space for output lines.
 */

	count = 0;
	for (obj = list; obj != NULL; obj = obj->next_content)
		count++;


	prgpstrShow = alloc_mem(count * (unsigned int)sizeof(char *));
	prgnShow = alloc_mem(count * (unsigned int)sizeof(int));

	nShow = 0;

/*
 * Format the list of objects.
 */
	for (obj = list; obj != NULL; obj = obj->next_content) {
		if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj)) {
			pstrShow = format_obj_to_char(obj, ch, fShort);

			fCombine = FALSE;

			if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
				/*
				 * Look for duplicates, case sensitive.
				 * Matches tend to be near end so run loop backwords.
				 */
				for (iShow = nShow - 1; iShow >= 0; iShow--) {
					if (!strcmp(prgpstrShow[iShow], pstrShow)) {
						prgnShow[iShow]++;
						fCombine = TRUE;
						break;
					}
				}
			}

			/*
			 * Couldn't combine, or didn't want to.
			 */
			if (!fCombine) {
				prgpstrShow[nShow] = str_dup(pstrShow);
				prgnShow[nShow] = 1;
				nShow++;

				if (nShow > 125) {
					send_to_char("Tell whoever it is you are looking at to drop some crap.\n\r", ch);
					send_to_char("Maximum number of items in a list exceeded.\n\r", ch);
					return;
				}
			}
		}
	}

/*
 * Output the formatted list.
 */
	for (iShow = 0; iShow < nShow; iShow++) {
		if (prgpstrShow[iShow][0] == '\0') {
			free_string(prgpstrShow[iShow]);
			continue;
		}

		if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
			if (prgnShow[iShow] != 1) {
				sprintf(buf, "(%3d) ", prgnShow[iShow]);
				send_to_char(buf, ch);
			} else {
				send_to_char("      ", ch);
			}
		}
		page_to_char(prgpstrShow[iShow], ch);
		send_to_char("\n\r", ch);
		free_string(prgpstrShow[iShow]);
	}

	if (fShowNothing && nShow == 0) {
		if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
			send_to_char("     ", ch);

		send_to_char("Nothing.\n\r", ch);
	}

/*
 * Clean up.
 */
	free_mem(prgpstrShow, count * (int)sizeof(char *));
	free_mem(prgnShow, count * (int)sizeof(int));

	return;
}



void show_char_to_char_0(CHAR_DATA *victim, CHAR_DATA *ch)
{
	char buf[MSL], message[MSL];

	buf[0] = '\0';

	if (IS_SET(victim->comm2, COMM2_AFK)) strcat(buf, "```!A```@F```OK ``");
	if (IS_SET(victim->comm2, COMM2_BUSY))
		strcat(buf, "[`1Busy``] ");

	if (IS_SET(victim->comm2, COMM2_CODING)) strcat(buf, "[`@CODING``] ");
	if (IS_SET(victim->comm2, COMM2_BUILD)) strcat(buf, "[`3BUILDING``] ");
	if (IS_AFFECTED(victim, AFF_INVISIBLE)) strcat(buf, "``(`iInvis``) ");
	if (victim->invis_level >= LEVEL_HERO) strcat(buf, "`8(`wWiZi`8) ``");
	if (!IS_NPC(victim))
		if (IS_AFFECTED(victim, AFF_HIDE)) strcat(buf, "```4(`hHide```4) ``");
	if (IS_AFFECTED(victim, AFF_CHARM)) strcat(buf, "```#(`cCharmed```#) ``");
	if (IS_AFFECTED(victim, AFF_PASS_DOOR)) strcat(buf, "```6(```^Translucent```6) ``");
	if (IS_AFFECTED(victim, AFF_FAERIE_FIRE)) strcat(buf, "```!(```PPink Aura```!) ``");
	if (IS_EVIL(victim)
	    && IS_AFFECTED(ch, AFF_DETECT_EVIL)) strcat(buf, "```8(```1Red Aura```8) ``");
	if (IS_GOOD(victim) && IS_AFFECTED(ch, AFF_DETECT_GOOD)) strcat(buf, "```&(```#Golden Aura```&) ``");
	if (IS_AFFECTED(victim, AFF_SANCTUARY)) strcat(buf, "(```&White Aura``) ");
	if (IS_AFFECTED(victim, AFF_DRUID_CALL)) strcat(buf, "(`8Grey Aura``) ");
	if (IS_AFFECTED(victim, AFF_CALLOUSED)) strcat(buf, "`&(`6C`^a`6ll`^ou`6s`^e`6d`&)`` ");
	if (is_affected(victim, gsp_web)) strcat(buf, "`8(`2St`@i`2c`@k`2y`8)`` ");
	if (is_affected(victim, gsp_voodoo)) strcat(buf, "`6(`8Hexxed`6)`` ");

	if (is_affected(victim, gsp_blink)) strcat(buf, "`^(`8S`7h`&i`7mm`&e`7r`&i`7n`8g`^)`` ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER)) strcat(buf, "-```1K```!i```1LLER``- ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)) strcat(buf, "-```8TH``i```8EF``- ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_IDIOT)) strcat(buf, "`7[`PIDIOT`7] ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_LINKDEAD)) strcat(buf, "`7[`8LINKDEAD`7] ");
	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_IT)) strcat(buf, "`8[`&IT!`8]``");
	if ((victim == ch->target)) strcat(buf, "`&[`7TARGET`&]`7 ");
	if (victim->position == victim->start_pos && victim->long_descr[0] != '\0') {
		strcat(buf, victim->long_descr);
		send_to_char(buf, ch);
		return;
	}

	strcat(buf, PERS(victim, ch));

	if (!IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF)
	    && victim->position == POS_STANDING && ch->on == NULL)
		strcat(buf, victim->pcdata->title);

	switch (victim->position) {
	case POS_DEAD:
		strcat(buf, " is ```1D```8E```1A```8D``!!");
		break;
	case POS_MORTAL:
		strcat(buf, " is mortally wounded.");
		break;
	case POS_INCAP:
		strcat(buf, " is incapacitated.");
		break;
	case POS_STUNNED:
		strcat(buf, " is lying here stunned.");
		break;
	case POS_SLEEPING:
		if (victim->on != NULL) {
			if (IS_SET(victim->on->value[2], SLEEP_AT)) {
				sprintf(message, " is sleeping at %s.", victim->on->short_descr);
				strcat(buf, message);
			} else if (IS_SET(victim->on->value[2], SLEEP_ON)) {
				sprintf(message, " is sleeping on %s.", victim->on->short_descr);
				strcat(buf, message);
			} else {
				sprintf(message, " is sleeping in %s.", victim->on->short_descr);
				strcat(buf, message);
			}
		} else {
			strcat(buf, " is sleeping here.");
		}

		break;
	case POS_RESTING:
		if (victim->on != NULL) {
			if (IS_SET(victim->on->value[2], REST_AT)) {
				sprintf(message, " is resting at %s.", victim->on->short_descr);
				strcat(buf, message);
			} else if (IS_SET(victim->on->value[2], REST_ON)) {
				sprintf(message, " is resting on %s.", victim->on->short_descr);
				strcat(buf, message);
			} else {
				sprintf(message, " is resting in %s.", victim->on->short_descr);
				strcat(buf, message);
			}
		} else {
			strcat(buf, " is resting here.");
		}

		break;
	case POS_SITTING:
		if (victim->on != NULL) {
			if (IS_SET(victim->on->value[2], SIT_AT)) {
				sprintf(message, " is sitting at %s.", victim->on->short_descr);
				strcat(buf, message);
			} else if (IS_SET(victim->on->value[2], SIT_ON)) {
				sprintf(message, " is sitting on %s.", victim->on->short_descr);
				strcat(buf, message);
			} else {
				sprintf(message, " is sitting in %s.", victim->on->short_descr);
				strcat(buf, message);
			}
		} else {
			strcat(buf, " is sitting here.");
		}

		break;
	case POS_STANDING:
		if (victim->on != NULL) {
			if (IS_SET(victim->on->value[2], STAND_AT)) {
				sprintf(message, " is standing at %s.", victim->on->short_descr);
				strcat(buf, message);
			} else if (IS_SET(victim->on->value[2], STAND_ON)) {
				sprintf(message, " is standing on %s.", victim->on->short_descr);
				strcat(buf, message);
			} else {
				sprintf(message, " is standing in %s.", victim->on->short_descr);
				strcat(buf, message);
			}
		} else {
			strcat(buf, " is here.");
		}

		break;
	case POS_FIGHTING:
		strcat(buf, " is here, `!fighting`` ");
		if (victim->fighting == NULL) {
			strcat(buf, "thin air??");
		} else if (victim->fighting == ch) {
			strcat(buf, "```!YOU``!");
		} else if (victim->in_room == victim->fighting->in_room) {
			strcat(buf, PERS(victim->fighting, ch));
			strcat(buf, ".");
		} else {
			strcat(buf, "someone who left??");
		}

		break;
	}

	strcat(buf, "\n\r");
	buf[0] = UPPER(buf[0]);
	send_to_char(buf, ch);
	return;
}



void show_char_to_char_1(CHAR_DATA *victim, CHAR_DATA *ch)
{
	char buf[10000];
	int race;

	if (can_see(victim, ch)) {
		if (ch == victim) {
			act("$n looks at $mself.", ch, NULL, NULL, TO_ROOM);
		} else {
			act("$n looks at you.", ch, NULL, victim, TO_VICT);
			act("$n looks at $N.", ch, NULL, victim, TO_NOTVICT);
		}
	}

	if (victim->description[0] != '\0')
		send_to_char(victim->description, ch);
	else
		act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);

	show_damage_display(ch, victim);
	/*
	 * if(victim->max_hit > 0)
	 * {
	 *      percent =(100 * victim->hit) / victim->max_hit;
	 * }
	 * else
	 * {
	 *      percent = -1;
	 * }
	 *
	 * strcpy(buf, PERS(victim, ch));
	 *
	 * if(percent >= 100)
	 * {
	 *      strcat(buf, " `Cis in excellent condition.``\n\r");
	 * }
	 * else if(percent >= 90)
	 * {
	 *      strcat(buf, " `Chas a few scratches.``\n\r");
	 * }
	 * else if(percent >= 75)
	 * {
	 *      strcat(buf, " `Chas some small wounds and bruises.``\n\r");
	 * }
	 * else if(percent >= 50)
	 * {
	 *      strcat(buf, " `Chas quite a few wounds.``\n\r");
	 * }
	 * else if(percent >= 30)
	 * {
	 *      strcat(buf, " `Chas some big nasty wounds and scratches.``\n\r");
	 * }
	 * else if(percent >= 15)
	 * {
	 *      strcat(buf, "`C looks pretty hurt.``\n\r");
	 * }
	 * else if(percent >= 0)
	 * {
	 *      strcat(buf, "`C is in awful condition.``\n\r");
	 * }
	 * else
	 * {
	 *      strcat(buf, " `Cis bleeding to death.``\n\r");
	 * }
	 *
	 * buf[0] = UPPER(buf[0]);
	 * send_to_char(buf, ch);
	 */

	if ((ch->race == (race = race_lookup("vampire"))) && (IS_NPC(victim))) {
		sprintf(buf, "%s ", victim->name);
		send_to_char(buf, ch);

		if (victim->drained >= (9 * (victim->level / 10))) send_to_char("looks quite healthy and `!pink``.\n\r", ch); else if (victim->drained >= (7 * (victim->level / 10))) send_to_char("looks slightly `&pale``.\n\r", ch); else if (victim->drained >= (5 * (victim->level / 10))) send_to_char("looks rather pale.\n\r", ch); else if (victim->drained >= (3 * (victim->level / 10))) send_to_char("looks quite pale.\n\r", ch);
	}

	return;
}

void show_char_to_char_2(CHAR_DATA *victim, CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	SKILL *skill_peek;
	char buf[MSL];
	int iWear;
	bool found;
	int race;

	if (can_see(victim, ch)) {
		if (ch == victim) {
			act("$n glances at $mself.", ch, NULL, NULL, TO_ROOM);
		} else {
			act("$n glances at you.", ch, NULL, victim, TO_VICT);
			act("$n glances at $N.", ch, NULL, victim, TO_NOTVICT);
		}
	}

	show_damage_display(ch, victim);

	if ((ch->race == (race = race_lookup("vampire"))) && (IS_NPC(victim))) {
		sprintf(buf, "%s ", victim->name);
		send_to_char(buf, ch);
		if (victim->drained >= (9 * (victim->level / 10))) send_to_char("looks quite healthy and `!pink``.\n\r", ch); else if (victim->drained >= (7 * (victim->level / 10))) send_to_char("looks slightly `&pale``.\n\r", ch); else if (victim->drained >= (5 * (victim->level / 10))) send_to_char("looks rather pale.\n\r", ch); else if (victim->drained >= (3 * (victim->level / 10))) send_to_char("looks quite pale.\n\r", ch);
	}

	found = FALSE;
	for (iWear = 0; where_name[iWear].wear_loc >= 0; iWear++) {
		if ((obj = get_eq_char(victim, where_name[iWear].wear_loc)) != NULL
		    && can_see_obj(ch, obj)) {
			if (!found) {
				send_to_char("\n\r", ch);
				act("$N is using:", ch, NULL, victim, TO_CHAR);
				found = TRUE;
			}
			send_to_char(where_name[iWear].desc, ch);
			send_to_char("(", ch);
			send_to_char(format_obj_to_char(obj, ch, TRUE), ch);
			send_to_char(")", ch);
			send_to_char("\n\r", ch);
		}
	}

	if (victim != ch && !IS_NPC(ch)
	    && (skill_peek = gsp_peek) != NULL
	    && number_percent() < get_learned_percent(ch, skill_peek)) {
		send_to_char("\n\rYou peek at the inventory:\n\r", ch);
		check_improve(ch, skill_peek, TRUE, 4);
		show_list_to_char(victim->carrying, ch, TRUE, TRUE);
	}

	return;
}

void show_char_to_char(CHAR_DATA *list, CHAR_DATA *ch)
{
	CHAR_DATA *rch;

	for (rch = list; rch != NULL; rch = rch->next_in_room) {
		if (rch == ch)
			continue;

		if (get_trust(ch) < rch->invis_level)
			continue;

		if (can_see(ch, rch))
			show_char_to_char_0(rch, ch);
		else if (room_is_dark(ch, ch->in_room)
			 && IS_AFFECTED(rch, AFF_INFRARED))
			send_to_char("You see glowing ```1red ``eyes watching YOU!\n\r", ch);
	}

	return;
}



bool check_blind(CHAR_DATA *ch)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return TRUE;

	if (IS_AFFECTED(ch, AFF_BLIND)) {
		send_to_char("You can't see a thing!  You're blind!!!\n\r", ch);
		return FALSE;
	}

	return TRUE;
}

/* One time use only toggle that changes char from NPK to PK
 * Fayth 9/4/00 -- created for use on Crucible, used for Bad Trip by permission from Cruc Owner
 */
void do_gopk(CHAR_DATA *ch, char *argument)
{
	/* NPCs, obviously, are not subject to PK/NPK designation*/
	if (IS_NPC(ch)) {
		send_to_char("Not on NPCs.\n\r", ch);
		return;
	}

	/* No argument with first use of go_pk command*/
	if (argument[0] != '\0') {
		send_to_char("Just type go_pk.  No argument.\n\r", ch);
		return;
	}

	/* Warn player of permanence of PK status*/
	send_to_char("Type go_pk again to confirm this command.\n\r", ch);
	send_to_char("WARNING: this command is irreversible.\n\r", ch);
	send_to_char("Once you are PK, you cannot go back to NPK.\n\r", ch);
	send_to_char("Typing go_pk with an argument will undo go_pk status.\n\r", ch);
	ch->pcdata->confirm_pk = TRUE;
	return;
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[100];
	int lines;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		if (ch->lines == 0) {
			send_to_char("You do not page long messages.\n\r", ch);
		} else {
			sprintf(buf, "You currently display %d lines per page.\n\r",
				ch->lines + 2);
			send_to_char(buf, ch);
		}
		return;
	}

	if (!is_number(arg)) {
		send_to_char("Give me a number or I'll break your arm.\n\r", ch);
		return;
	}

	lines = atoi(arg);

	if (lines == 0) {
		send_to_char("Paging disabled.\n\r", ch);
		ch->lines = 0;
		return;
	}

	if (lines < 10 || lines > 50) {
		send_to_char("You must provide a reasonable number.\n\r", ch);
		return;
	}

	sprintf(buf, "Scroll set to %d lines.\n\r", lines);
	send_to_char(buf, ch);
	ch->lines = lines - 2;
}

/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "motd");
}

void do_imotd(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "imotd");
}

void do_rules(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "rules");
}

void do_story(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "story");
}

void do_wizlist(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
 * replacements for config */
void do_autolist(CHAR_DATA *ch, char *argument)
{
/* lists most player flags */
	if (IS_NPC(ch))
		return;

	send_to_char("   action     status\n\r", ch);
	send_to_char("---------------------\n\r", ch);

	send_to_char("autoassist     ", ch);
	if (IS_SET(ch->act, PLR_AUTOASSIST))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autoeq         ", ch);
	if (IS_SET(ch->act, PLR_AUTOEQ))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autoexit       ", ch);
	if (IS_SET(ch->act, PLR_AUTOEXIT))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autogold       ", ch);
	if (IS_SET(ch->act, PLR_AUTOGOLD))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autotoken      ", ch);
	if (IS_SET(ch->comm2, COMM2_AUTOTOKEN))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);


	send_to_char("autoloot       ", ch);
	if (IS_SET(ch->act, PLR_AUTOLOOT))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autosac        ", ch);
	if (IS_SET(ch->act, PLR_AUTOSAC))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autosplit      ", ch);
	if (IS_SET(ch->act, PLR_AUTOSPLIT))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("compact mode   ", ch);
	if (IS_SET(ch->comm, COMM_COMPACT))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("autoticks      ", ch);
	if (IS_SET(ch->comm2, COMM2_TICKS))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("prompt         ", ch);
	if (IS_SET(ch->comm, COMM_PROMPT))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	send_to_char("combine items  ", ch);
	if (IS_SET(ch->comm, COMM_COMBINE))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);

	if (!IS_SET(ch->act, PLR_CANLOOT))
		send_to_char("Your corpse is safe from thieves.\n\r", ch);
	else
		send_to_char("Your corpse may be looted.\n\r", ch);

	if (IS_SET(ch->act, PLR_NOSUMMON))
		send_to_char("You cannot be summoned.\n\r", ch);
	else
		send_to_char("You can be summoned.\n\r", ch);

	if (IS_SET(ch->act, PLR_NOFOLLOW))
		send_to_char("You do not welcome followers.\n\r", ch);
	else
		send_to_char("You accept followers.\n\r", ch);
}


void do_hidehost(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->comm2, COMM2_PRIV)) {
		send_to_char("Your host name is now visible.\n\r", ch);
		REMOVE_BIT(ch->comm2, COMM2_PRIV);
	} else {
		send_to_char("Your host name is now `!hidden`7.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_PRIV);
	}
}

void do_autoassist(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOASSIST)) {
		send_to_char("Autoassist removed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOASSIST);
	} else {
		send_to_char("You will now assist when needed.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOASSIST);
	}
}

void do_autoeq(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOEQ)) {
		send_to_char("Empty eq slots will no longer be displayed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOEQ);
	} else {
		send_to_char("Empty eq slots will now be displayed.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOEQ);
	}
}

void do_autoexit(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOEXIT)) {
		send_to_char("Exits will no longer be displayed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOEXIT);
	} else {
		send_to_char("Exits will now be displayed.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOEXIT);
	}
}

void do_autotoken(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (!IS_SET(ch->comm2, COMM2_ENABLE)) {
		send_to_char("Player File Tokens Enabled!!! You will only see this message once in your entire character life.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_ENABLE);
		ch->pcdata->tenten = 0;
		ch->pcdata->twohundred = 0;
		ch->pcdata->armorclass = 0;
		ch->pcdata->restring = 0;
		ch->pcdata->weaponflag = 0;
		ch->pcdata->fireproof = 0;
		ch->pcdata->damnoun = 0;
		ch->pcdata->immtrivia = 0;
		ch->pcdata->immhidden = 0;
		ch->pcdata->immwild = 0;
		ch->pcdata->imp = 0;
		ch->pcdata->skillset = 0;
		ch->pcdata->rp = 0;
	}

	if (IS_SET(ch->comm2, COMM2_AUTOTOKEN)) {
		send_to_char("Tokens in inventory screen removed.\n\r", ch);
		REMOVE_BIT(ch->comm2, COMM2_AUTOTOKEN);
	} else {
		send_to_char("Token counts added to inventory screen.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_AUTOTOKEN);
	}
}

void do_autogold(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOGOLD)) {
		send_to_char("Autogold removed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOGOLD);
	} else {
		send_to_char("Automatic gold looting set.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOGOLD);
	}
}

void do_autoloot(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOLOOT)) {
		send_to_char("Autolooting removed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOLOOT);
	} else {
		send_to_char("Automatic corpse looting set.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOLOOT);
	}
}

void do_autosac(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOSAC)) {
		send_to_char("Autosacrificing removed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOSAC);
	} else {
		send_to_char("Automatic corpse sacrificing set.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOSAC);
	}
}

void do_autosplit(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_AUTOSPLIT)) {
		send_to_char("Autosplitting removed.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_AUTOSPLIT);
	} else {
		send_to_char("Automatic gold splitting set.\n\r", ch);
		SET_BIT(ch->act, PLR_AUTOSPLIT);
	}
}

void do_brief(CHAR_DATA *ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_BRIEF)) {
		send_to_char("Full descriptions activated.\n\r", ch);
		REMOVE_BIT(ch->comm, COMM_BRIEF);
	} else {
		send_to_char("Short descriptions activated.\n\r", ch);
		SET_BIT(ch->comm, COMM_BRIEF);
	}
}

void do_compact(CHAR_DATA *ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_COMPACT)) {
		send_to_char("Compact mode removed.\n\r", ch);
		REMOVE_BIT(ch->comm, COMM_COMPACT);
	} else {
		send_to_char("Compact mode set.\n\r", ch);
		SET_BIT(ch->comm, COMM_COMPACT);
	}
}

void send_color_status(CHAR_DATA *ch, char *a, char *c, byte b)
{
	sprintf(c, "%s %s %s\n\r", color_table[b], a, ANSI_NORMAL);
	send_to_char(c, ch);
}

void do_autoticks(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->comm2, COMM2_TICKS)) {
		send_to_char("You will no longer see ticks.\n\r", ch);
		REMOVE_BIT(ch->comm2, COMM2_TICKS);
	} else {
		send_to_char("You will now see the ticks.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_TICKS);
	}
}

void do_color(CHAR_DATA *ch, char *argument)
{
	unsigned int i = 0;
	byte a = (byte)0;
	byte b = (byte)0;
	char cbuf[MIL];

	DENY_NPC(ch);

	if (argument[0] == '\0') {
		if (ch->color) {
			ch->color = 0;
			send_to_char("Ansi color turned off.\n\r", ch);
		} else {
			ch->color = 1;
			send_to_char("Ansi color turned on.\n\r", ch);
		}
	} else {
		argument = one_argument(argument, cbuf);

		if (str_cmp(cbuf, "codes") == 0) {
			send_to_char("These are the numbers/symbols to be used for color codes.\n\r\n\r", ch);
			send_to_char("`1Red uses the color code: 1``\n\r`2Green uses the color code: 2``\n\r`3Brown uses the color code: 3``\n\r`4Blue uses the color code: 4``\n\r`5Purple uses the color code: 5`` \n\r`6Cyan uses the color code: 6``\n\r", ch);
			send_to_char("`7Grey uses the color code: 7``\n\r", ch);
			send_to_char("`8Dark grey uses the code: 8``\n\r`!Bright red uses the code: !``\n\r`@Bright green uses the code: @``\n\r`#Yellow uses the color code: #``\n\r`OBright blue uses the code: O``\n\r`PMagneta uses the code: P``\n\r", ch);
			send_to_char("`^Bright cyan uses the code: ^``\n\r`&White uses $color code: &``\n\r\n\r", ch);
			send_to_char_ascii("To use these color in things like your title, the following code and\n\rformat is required. `<color-code> <text>``  Note the `` at the end.\n\r", ch);
			send_to_char("This is used to clear the color so that it does not run onto the next\n\rline.\n\r\n\r", ch);
			send_to_char_ascii("For example:  emote `!grins`` and `#laughs``.\n\r", ch);
			send_to_char("Will produce: <Your name> `!grins`` and `#laughs``.\n\r", ch);
		} else if (str_cmp(cbuf, "list") == 0) {
			send_to_char("Listing colours.\n\r", ch);
			while (color_table[i]) {
				sprintf(cbuf, "%scolor %d%s\n\r", color_table[i], (int)i, ANSI_NORMAL);
				send_to_char(cbuf, ch);
				i++;
			}
		} else if (str_cmp(cbuf, "show") == 0) {
			send_to_char("Listing color setup.\n\r# of option\n\r", ch);
			send_color_status(ch, "1 combat melee(self)", cbuf, ch->pcdata->color_combat_s);
			send_color_status(ch, "2 combat melee(opponent)", cbuf, ch->pcdata->color_combat_o);
			send_color_status(ch, "3 combat condition(self)", cbuf, ch->pcdata->color_combat_condition_s);
			send_color_status(ch, "4 combat condition(opponent)", cbuf, ch->pcdata->color_combat_condition_o);
			send_color_status(ch, "5 wizi mobs", cbuf, ch->pcdata->color_wizi);
			send_color_status(ch, "6 invis mobs", cbuf, ch->pcdata->color_invis);
			send_color_status(ch, "7 hidden mobs", cbuf, ch->pcdata->color_hidden);
			send_color_status(ch, "8 charmed mobs", cbuf, ch->pcdata->color_charmed);
			send_color_status(ch, "9 hp", cbuf, ch->pcdata->color_hp);
			send_color_status(ch, "10 mana", cbuf, ch->pcdata->color_mana);
			send_color_status(ch, "11 movement", cbuf, ch->pcdata->color_move);
			send_color_status(ch, "12 say", cbuf, ch->pcdata->color_say);
			send_color_status(ch, "13 tell", cbuf, ch->pcdata->color_tell);
			send_color_status(ch, "14 reply", cbuf, ch->pcdata->color_reply);
		} else if (str_cmp(cbuf, "set") == 0) { /* if cbuf */
			argument = one_argument(argument, cbuf);
			a = parse_byte2(cbuf, (byte)0, (byte)15);
			argument = one_argument(argument, cbuf);

			if (cbuf[0] != '\0') { /*cbuf */
				b = parse_byte(cbuf);
				while (color_table[i])
					i++;

				if (((unsigned int)b < i)) {/* check for valid color */
					switch (a) { /* switch a */
					case 1:
						ch->pcdata->color_combat_s = b;
						send_to_char("`acombat melee(self)``\n\r", ch);
						break;
					case 2:
						ch->pcdata->color_combat_o = b;
						send_to_char("`Acombat melee(opponent)``\n\r", ch);
						break;
					case 3:
						ch->pcdata->color_combat_condition_s = b;
						send_to_char("`Ccombat condition(self)``\n\r", ch);
						break;
					case 4:
						ch->pcdata->color_combat_condition_o = b;
						send_to_char("`Dcombat condition(opponent)``\n\r", ch);
						break;
					case 5:
						ch->pcdata->color_wizi = b;
						send_to_char("`wwizi mobs/exits``\n\r", ch);
						break;
					case 6:
						ch->pcdata->color_invis = b;
						send_to_char("`iinvis``\n\r", ch);
						break;
					case 7:
						ch->pcdata->color_hidden = b;
						send_to_char("`hhidden mobs``\n\r", ch);
						break;
					case 8:
						ch->pcdata->color_charmed = b;
						send_to_char("`ccharmed mobs``\n\r", ch);
						break;
					case 9:
						ch->pcdata->color_hp = b;
						send_to_char("`Hhp``\n\r", ch);
						break;
					case 10:
						ch->pcdata->color_mana = b;
						send_to_char("`Mmana``\n\r", ch);
						break;
					case 11:
						ch->pcdata->color_move = b;
						send_to_char("`Vmovement``\n\r", ch);
						break;
					case 12:
						ch->pcdata->color_say = b;
						send_to_char("`ssay``\n\r", ch);
						break;
					case 13:
						ch->pcdata->color_tell = b;
						send_to_char("`ttell``\n\r", ch);
						break;
					case 14:
						ch->pcdata->color_reply = b;
						send_to_char("`rreply``\n\r", ch);
						break;
					default:
						send_to_char("Change color for which option?\n\r", ch);
						break;
					}       /* switch a */
				}               /* if(b...) */
				else {
					send_to_char("What color!!!\n", ch);
				}
			}       /* cbuf */
		}               /* if(set==show) */
		else {
			send_to_char("Color what?\n", ch);
		}
	}
}


void do_show(CHAR_DATA *ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_SHOW_AFFECTS)) {
		send_to_char("Affects will no longer be shown in score.\n\r", ch);
		REMOVE_BIT(ch->comm, COMM_SHOW_AFFECTS);
	} else {
		send_to_char("Affects will now be shown in score.\n\r", ch);
		SET_BIT(ch->comm, COMM_SHOW_AFFECTS);
	}
}

void do_prompt(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	if (argument[0] == '\0') {
		if (IS_SET(ch->comm, COMM_PROMPT)) {
			send_to_char("You will no longer see prompts.\n\r", ch);
			REMOVE_BIT(ch->comm, COMM_PROMPT);
		} else {
			send_to_char("You will now see prompts.\n\r", ch);
			SET_BIT(ch->comm, COMM_PROMPT);
		}
		return;
	}

	if (!strcmp(argument, "all")) {
		strcpy(buf, "<%hhp %mm %vmv> ");
	} else {
		if (strlen(argument) > 150) /*was 50*/
			argument[150] = '\0';
		strcpy(buf, argument);
		smash_tilde(buf);
		if (str_suffix("%c", buf))
			strcat(buf, " ");
	}

	free_string(ch->prompt);
	ch->prompt = str_dup(buf);
	sprintf(buf, "Prompt set to```8: ``%s\n\r", ch->prompt);
	send_to_char(buf, ch);
	return;
}

void do_combine(CHAR_DATA *ch, char *argument)
{
	if (IS_SET(ch->comm, COMM_COMBINE)) {
		send_to_char("Long inventory selected.\n\r", ch);
		REMOVE_BIT(ch->comm, COMM_COMBINE);
	} else {
		send_to_char("Combined inventory selected.\n\r", ch);
		SET_BIT(ch->comm, COMM_COMBINE);
	}
}

void do_noloot(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_CANLOOT)) {
		send_to_char("Your corpse is now safe from `@N`2ecrophileacks``.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_CANLOOT);
	} else {
		send_to_char("Your corpse may now be `!R`1aped`` and `@P`2illeged``.\n\r", ch);
		SET_BIT(ch->act, PLR_CANLOOT);
	}
}

void do_nofollow(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_NOFOLLOW)) {
		send_to_char("You now accept followers.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_NOFOLLOW);
	} else {
		if (IS_AFFECTED(ch, AFF_CHARM)) {
			send_to_char("But you're just so happy the way things are ..\n\r", ch);
			return;
		}
		send_to_char("You no longer accept followers.\n\r", ch);
		SET_BIT(ch->act, PLR_NOFOLLOW);
		die_follower(ch);
	}
}

void do_nosummon(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch)) {
		if (IS_SET(ch->imm_flags, IMM_SUMMON)) {
			send_to_char("You are no longer immune to summon.\n\r", ch);
			REMOVE_BIT(ch->imm_flags, IMM_SUMMON);
		} else {
			send_to_char("You are now immune to summoning.\n\r", ch);
			SET_BIT(ch->imm_flags, IMM_SUMMON);
		}
	} else {
		if (IS_SET(ch->act, PLR_NOSUMMON)) {
			send_to_char("You are no longer immune to summon.\n\r", ch);
			REMOVE_BIT(ch->act, PLR_NOSUMMON);
		} else {
			send_to_char("You are now immune to summoning.\n\r", ch);
			SET_BIT(ch->act, PLR_NOSUMMON);
		}
	}
}

void do_lore(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	SKILL *skill;
	char arg[MIL];
	int percent;

	if ((skill = skill_lookup("lore")) == NULL) {
		send_to_char("Huh?", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("What do you want to lore?\n\r", ch);
		return;
	}

	obj = get_obj_carry(ch, arg);
	if (obj == NULL) {
		send_to_char("You aren't carrying that.\n\r", ch);
		return;
	}

	percent = get_learned_percent(ch, skill);
	if (!IS_NPC(ch) && number_percent() > percent) {
		act("You look at $p, but you can't find out any additional information.",
		    ch, obj, NULL, TO_CHAR);
		act("$n looks at $p but cannot find out anything.", ch, obj, NULL, TO_ROOM);
		return;
	} else {
		act("$n studies $p, discovering all of its hidden powers.", ch, obj, NULL, TO_ROOM);

		identify_item(ch, obj);
		check_improve(ch, skill, TRUE, 4);
	}
}

void do_glance(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	char arg3[MIL];
	CHAR_DATA *victim;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING) {
		send_to_char("You can't see anything but stars!\n\r", ch);
		return;
	}

	if (ch->position == POS_SLEEPING) {
		send_to_char("You can't see anything, you're sleeping!\n\r", ch);
		return;
	}

	if (!check_blind(ch))
		return;

	if (!IS_NPC(ch)
	    && !IS_SET(ch->act, PLR_HOLYLIGHT)
	    && room_is_dark(ch, ch->in_room)) {
		send_to_char("It is pitch `8black...``\n\r", ch);
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg1[0] == '\0') {
		send_to_char("You glance around into thin air.", ch);
		return;
	}


	if ((victim = get_char_room(ch, arg1)) != NULL) {
		show_char_to_char_2(victim, ch);
		return;
	} else {
		send_to_char("That person isn't here.\n\r", ch);
		return;
	}
}

void do_look(CHAR_DATA *ch, char *argument)
{
	EXIT_DATA *pexit;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA *paf;
	char arg1[MIL];
	char arg2[MIL];
	char arg3[MIL];
	char *pdesc;
	int door;
	int number;
	int count;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING) {
		send_to_char("You can't see anything but stars!\n\r", ch);
		return;
	}

	if (ch->position == POS_SLEEPING) {
		send_to_char("You can't see anything, you're sleeping!\n\r", ch);
		return;
	}

	if (!check_blind(ch))
		return;

	if (!IS_NPC(ch)
	    && !IS_SET(ch->act, PLR_HOLYLIGHT)
	    && room_is_dark(ch, ch->in_room)) {
		send_to_char("It is pitch ```8black...`` \n\r", ch);
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);
	number = number_argument(arg1, arg3);
	count = 0;

	if (arg1[0] == '\0' || !str_cmp(arg1, "auto")) {
		/* 'look' or 'look auto' */
		send_to_char(ch->in_room->name, ch);
		if ((IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act, PLR_HOLYLIGHT)))
		    || IS_BUILDER(ch, ch->in_room->area))
			printf_to_char(ch, " [`1Room `!%d``]", ch->in_room->vnum);

		send_to_char("\n\r", ch);
		if (arg1[0] == '\0'
		    || (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF))) {
			send_to_char("  ", ch);
			send_to_char(ch->in_room->description, ch);
		}

		send_to_char("\n\r", ch);

		if (ch->in_room != NULL
		    && (is_affected(ch, gsp_detect_magic)
			|| IS_SET(ch->act, PLR_HOLYLIGHT))) {
			for (paf = ch->in_room->affected; paf != NULL; paf = paf->next)
				send_to_char(room_affect(paf), ch);
			send_to_char("\n\r", ch);
		}

		if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT))
			do_exits(ch, "auto");

		if (ch->in_room != NULL) {
			if (is_affected_room(ch->in_room, gsp_faerie_fog))
				send_to_char("There is a `Ppurple haze`` floating throughout the room.\n\r", ch);

			show_list_to_char(ch->in_room->contents, ch, FALSE, FALSE);
			show_char_to_char(ch->in_room->people, ch);
			return;
		}
	}

	if (!str_cmp(arg1, "i") || !str_cmp(arg1, "in") || !str_cmp(arg1, "on")) {
		OBJ_DATA *portal;
		ROOM_INDEX_DATA *location;

		/* 'look in' */
		if (arg2[0] == '\0') {
			send_to_char("Look in what?\n\r", ch);
			return;
		}

		if ((obj = get_obj_here(ch, arg2)) == NULL) {
			send_to_char("You do not see that here.\n\r", ch);
			return;
		}

		switch (obj->item_type) {
		default:
			send_to_char("That is not a container.\n\r", ch);
			break;

		case ITEM_DRINK_CON:
			if (obj->value[1] == 0) {
				send_to_char("It is empty.\n\r", ch);
			} else {
				printf_to_char(ch, "It's %sfilled with  a %s liquid.\n\r",
					       obj->value[1] < obj->value[0] / 4
					       ? "less than half-" :
					       obj->value[1] < 3 * obj->value[0] / 4
					       ? "about half-" : "more than half-",
					       liq_table[obj->value[2]].liq_color);
			}
			break;

		case ITEM_CONTAINER:
		case ITEM_CORPSE_NPC:
		case ITEM_CORPSE_PC:
			if (IS_SET(obj->value[1], CONT_CLOSED)) {
				send_to_char("It is closed.\n\r", ch);
			} else {
				act("$p holds```8:``", ch, obj, NULL, TO_CHAR);
				show_list_to_char(obj->contains, ch, TRUE, TRUE);
			}
			break;
		case ITEM_PORTAL:
			portal = get_obj_list(ch, argument, ch->in_room->contents);
			if (portal != NULL) {
				location = get_room_index(portal->value[3]);
				if (location == NULL) {
					send_to_char("It looks very empty..\n\r", ch);
				} else {
					send_to_char(location->name, ch);
					send_to_char("\n\r", ch);
					if (!IS_SET(location->room_flags, ROOM_INDOORS))
						print_weather(ch);
					else
						send_to_char("You can not discern weather conditions beyond this portal.\n\r", ch);
					if (arg1[0] == '\0'
					    || (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF)))
						send_to_char(location->description, ch);
					send_to_char("\n\r", ch);

					if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT)) {
						char showexit[100];

						sprintf(showexit, "%ld exits auto", location->vnum);
						do_at(ch, showexit);
					}

					show_list_to_char(location->contents, ch, FALSE, FALSE);
					show_char_to_char(location->people, ch);
				}
			}
			break;
		}

		return;
	}

	if ((victim = get_char_room(ch, arg1)) != NULL) {
		show_char_to_char_1(victim, ch);
		return;
	}

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if (can_see_obj(ch, obj)) {
			/* player can see object */

			pdesc = get_extra_descr(arg3, obj->extra_descr);
			if (pdesc != NULL) {
				if (++count == number) {
					send_to_char(pdesc, ch);
					return;
				} else {
					continue;
				}
			}

			pdesc = get_extra_descr(arg3, obj->obj_idx->extra_descr);
			if (pdesc != NULL) {
				if (++count == number) {
					send_to_char(pdesc, ch);
					return;
				} else {
					continue;
				}
			}
			if (is_name(arg3, obj->name)) {
				if (++count == number) {
					send_to_char(obj->description, ch);
					send_to_char("\n\r", ch);
					return;
				}
			}
		}
	}

	for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content) {
		if (can_see_obj(ch, obj)) {
			pdesc = get_extra_descr(arg3, obj->extra_descr);
			if (pdesc != NULL) {
				if (++count == number) {
					send_to_char(pdesc, ch);
					return;
				}
			}

			pdesc = get_extra_descr(arg3, obj->obj_idx->extra_descr);
			if (pdesc != NULL) {
				if (++count == number) {
					send_to_char(pdesc, ch);
					return;
				}
			}
		}

		if (is_name(arg3, obj->name)) {
			if (++count == number) {
				send_to_char(obj->description, ch);
				send_to_char("\n\r", ch);
				return;
			}
		}
	}

	pdesc = get_extra_descr(arg3, ch->in_room->extra_descr);
	if (pdesc != NULL) {
		if (++count == number) {
			send_to_char(pdesc, ch);
			return;
		}
	}

	if (count > 0 && count != number) {
		if (count == 1)
			printf_to_char(ch, "You only see one %s here.\n\r", arg3);
		else
			printf_to_char(ch, "You only see %d of those here.\n\r", count);

		return;
	}

	if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north")) {
		door = 0;
	} else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east")) {
		door = 1;
	} else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south")) {
		door = 2;
	} else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west")) {
		door = 3;
	} else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up")) {
		door = 4;
	} else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down")) {
		door = 5;
	} else {
		send_to_char("You do not see that here.\n\r", ch);
		return;
	}

	/* 'look direction' */
	if ((pexit = ch->in_room->exit[door]) == NULL) {
		send_to_char("Nothing special there.\n\r", ch);
		return;
	}

	if (pexit->description != NULL && pexit->description[0] != '\0')
		send_to_char(pexit->description, ch);
	else
		send_to_char("Nothing special there.\n\r", ch);

	if (pexit->keyword != NULL
	    && pexit->keyword[0] != '\0'
	    && pexit->keyword[0] != ' ') {
		if (IS_SET(pexit->exit_info, EX_CLOSED))
			act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
		else if (IS_SET(pexit->exit_info, EX_ISDOOR))
			act("The $d is open.", ch, NULL, pexit->keyword, TO_CHAR);
	}

	return;
}

/* RT added back for the hell of it */
void do_read(CHAR_DATA *ch, char *argument)
{
	do_look(ch, argument);
}

void do_examine(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg[MIL];
	OBJ_DATA *obj;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Aww no one wants to play doctor with you?\n\r", ch);
		return;
	}

	do_look(ch, arg);

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		switch (obj->item_type) {
		default:
			break;

		case ITEM_MONEY:
			if (obj->value[0] == 0) {
				if (obj->value[1] == 0)
					sprintf(buf, "Odd...there's no coins in the pile.\n\r");
				else if (obj->value[1] == 1)
					sprintf(buf, "Wow. One gold coin.\n\r");
				else
					sprintf(buf, "There are %ld gold coins in the pile.\n\r", obj->value[1]);
			} else if (obj->value[1] == 0) {
				if (obj->value[0] == 1)
					sprintf(buf, "Wow. One silver coin.\n\r");
				else
					sprintf(buf, "There are %ld silver coins in the pile.\n\r", obj->value[0]);
			} else {
				sprintf(buf, "There are %ld gold and %ld silver coins in the pile.\n\r", obj->value[1], obj->value[0]);
			}
			send_to_char(buf, ch);
			break;

		case ITEM_DRINK_CON:
		case ITEM_CONTAINER:
		case ITEM_CORPSE_NPC:
		case ITEM_CORPSE_PC:
			sprintf(buf, "in %s", argument);
			do_look(ch, buf);
		}
	}

	return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(CHAR_DATA *ch, char *argument)
{
	extern char *const dir_name[];
	EXIT_DATA *pexit;
	char buf[MSL];
	bool found;
	bool fAuto;
	int door;
	bool dClosed;
	char exit_name[10];


	fAuto = !str_cmp(argument, "auto");

	if (!check_blind(ch))
		return;

	if (fAuto)
		sprintf(buf, "`6[`^Exits`6:``");
	else if (IS_IMMORTAL(ch))
		sprintf(buf, "Obvious exits from room %ld`8:``\n\r", ch->in_room->vnum);
	else
		sprintf(buf, "Obvious exits`8:``\n\r");

	found = FALSE;
	for (door = 0; door <= 5; door++) {
		if ((pexit = ch->in_room->exit[door]) != NULL
		    && pexit->u1.to_room != NULL
		    && can_see_room(ch, pexit->u1.to_room)) {
			found = TRUE;
			dClosed = IS_SET(pexit->exit_info, EX_CLOSED);
			if (fAuto) {
				strcat(buf, " `^");
				if (dClosed) strcat(buf, "("); strcat(buf, dir_name[door]);
				if (dClosed) strcat(buf, ")");
			} else {
				sprintf(buf + strlen(buf), "%-5s - %s",
					capitalize(dir_name[door]),
					room_is_dark(ch, pexit->u1.to_room)
					? "Too dark to tell"
					: pexit->u1.to_room->name
					);
				if (IS_IMMORTAL(ch))
					sprintf(buf + strlen(buf), "(room %ld)\n\r", pexit->u1.to_room->vnum);
				else
					sprintf(buf + strlen(buf), "\n\r");
			}
		}
	}

	if (!found)
		strcat(buf, fAuto ? " `&none``" : "`&None``.\n\r");

	if (fAuto)
		strcat(buf, "`6]``\n\r");

	send_to_char(buf, ch);

	for (door = 0; door <= 5; door++) {
		if ((pexit = ch->in_room->exit[door]) != NULL
		    && pexit->u1.to_room != NULL
		    && can_see_room(ch, pexit->u1.to_room)) {
			dClosed = IS_SET(pexit->exit_info, EX_CLOSED);

			if (door == 0) {
				if (dClosed) sprintf(exit_name, "(north)"); else sprintf(exit_name, "north");
			} else if (door == 1) {
				if (dClosed) sprintf(exit_name, "(south)"); else sprintf(exit_name, "south");
			} else if (door == 2) {
				if (dClosed) sprintf(exit_name, "(east)"); else sprintf(exit_name, "east");
			} else if (door == 3) {
				if (dClosed) sprintf(exit_name, "(west)"); else sprintf(exit_name, "west");
			} else if (door == 4) {
				if (dClosed) sprintf(exit_name, "(up)"); else sprintf(exit_name, "up");
			} else {
				if (dClosed) sprintf(exit_name, "(down)"); else sprintf(exit_name, "down");
			}

			found = TRUE;
			return;
		}
		return;
	}
}

void do_worth(CHAR_DATA *ch, char *argument)
{
	if (!IS_NPC(ch)) printf_to_char(ch, "Extended Experience Points: %ld, Extended Level: %d\n\r",
					ch->pcdata->extendedexp, ch->pcdata->extendedlevel);
	if (IS_NPC(ch)) {
		printf_to_char(ch, "You have %ld gold and %ld silver.\n\r", ch->gold, ch->silver);
		return;
	}

	printf_to_char(ch, "You have ```#%ld ``gold, ```&%ld ``silver, and ```@%d experience ```8(```1%d ``exp to level```8)``.\n\r",
		       ch->gold, ch->silver, ch->exp,
		       (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);

	printf_to_char(ch, "You have `#%ld`` gold and `&%ld`` silver in the `2b`@ank``.\n\r",
		       ch->pcdata->gold_in_bank, ch->pcdata->silver_in_bank);

	return;
}


void do_score(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char hours[MSL];
	char mins[MSL];
	char secs[MSL];
	int i;
	int race;

	sprintf(buf, "You are %s%s, level %d, ",
		ch->name,
		IS_NPC(ch) ? "" : ch->pcdata->title, ch->level);
	send_to_char(buf, ch);

	if (!IS_NPC(ch)) {
		sprintf(buf, "%d years old``.\n\r",
/* `8(``%d hours`8)``.\n\r",*/
			get_age(ch));
/*		(ch->played +(int)(current_time - ch->logon)) / 3600);*/
		send_to_char(buf, ch);
	}
	sprintf(buf,
		"`O=================================================================``\n\r");

	send_to_char(buf, ch);


	sprintf(buf, "`7You have played a total of:`6 %d `7hours,`6 %d `7minutes,`6 %d `7seconds``\n\r",
		get_hours_played(ch),
		get_minutes_played(ch),
		get_seconds_played(ch));
	send_to_char(buf, ch);

	sprintf(hours, "%2.2d", get_session_hours(ch));
	sprintf(mins, "%2.2d", get_session_minutes(ch));
	sprintf(secs, "%2.2d", get_session_seconds(ch));

	sprintf(buf, "`7This session:`6 %s `7hours,`6 %s `7minutes,`6 %s `7seconds``\n\r", hours, mins, secs);
	send_to_char(buf, ch);

	if (get_trust(ch) != ch->level) {
		sprintf(buf, "You are trusted at level ```#%d``.\n\r",
			get_trust(ch));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Race```8: ``%s  Sex```8: ``%s  Class`8: ``%s\n\r",
		race_table[ch->race].name,
		ch->sex == 0 ? "Pat" : ch->sex == 1 ? "dude" : "chick",
		class_table[ch->class].name);
	send_to_char(buf, ch);

	sprintf(buf,
		"`O=================================================================``\n\r");

	send_to_char(buf, ch);



	if (!IS_NPC(ch)) {
		sprintf(buf, "You have %d hours left in `!jail``.\n\r",
			ch->pcdata->jail_time);
		send_to_char(buf, ch);
	}

	sprintf(buf,
		"`O=================================================================``\n\r");

	send_to_char(buf, ch);


	sprintf(buf,
		"You have ```!%d``/```1%d ``hit, ```@%d``/```2%d ``mana, ```O%d``/```4%d ``movement.\n\r",
		ch->hit, ch->max_hit,
		ch->mana, ch->max_mana,
		ch->move, ch->max_move);
	send_to_char(buf, ch);

	if (!IS_NPC(ch)) {
		sprintf(buf,
			"You have ```6%d ``practices and ```6%d ``training sessions.\n\r",
			ch->pcdata->practice, ch->pcdata->train);
		send_to_char(buf, ch);
	}

	sprintf(buf,
		"You are carrying ```P%d``/```5%d ``items with weight ```P%d``/```5%d ``pounds.\n\r",
		ch->carry_number, can_carry_n(ch),
		ch->carry_weight / 10, can_carry_w(ch) / 10);
	send_to_char(buf, ch);

	sprintf(buf,
		"Str```8: ``%d```8(``%d```8)  ``Int```8: ``%d```8(``%d```8)  ``Wis```8: ``%d```8(``%d```8)  ``Dex```8: ``%d```8(``%d```8)  ``Con```8: ``%d```8(``%d```8)  ``Luck```8: ``%d```8(``%d```8)``\n\r",
		ch->perm_stat[STAT_STR],
		get_curr_stat(ch, STAT_STR),
		ch->perm_stat[STAT_INT],
		get_curr_stat(ch, STAT_INT),
		ch->perm_stat[STAT_WIS],
		get_curr_stat(ch, STAT_WIS),
		ch->perm_stat[STAT_DEX],
		get_curr_stat(ch, STAT_DEX),
		ch->perm_stat[STAT_CON],
		get_curr_stat(ch, STAT_CON),
		ch->perm_stat[STAT_LUCK],
		get_curr_stat(ch, STAT_LUCK));
	send_to_char(buf, ch);

	sprintf(buf,
		"`O=================================================================``\n\r");

	send_to_char(buf, ch);


	sprintf(buf,
		"You have scored ```@%d ``exp, and have ```#%u ``gold and ```&%u ``silver coins.\n\r",
		ch->exp, ch->gold, ch->silver);
	send_to_char(buf, ch);



	sprintf(buf,
		"`O=================================================================``\n\r");

	send_to_char(buf, ch);


	if (!IS_NPC(ch) && ((ch->duelwin != 0) || (ch->duelloss != 0))) {
		sprintf(buf, "You have won `O%d`7 duels and lost `O%d`7 duels.\n\r", ch->duelwin, ch->duelloss);
		send_to_char(buf, ch);
	}

	if (!IS_NPC(ch)) {
		sprintf(buf, "You have entered %d battlefields, killed %d of those who entered,\n\rand you've been killed %d times.\n\r",
			ch->benter, ch->bkills, ch->bloss);
		send_to_char(buf, ch);
	}

/* RT shows exp to level */
	if (!IS_NPC(ch) && ch->level < LEVEL_HERO) {
		sprintf(buf,
			"You need ```!%d ``exp to level.\n\r",
			((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Wimpy set to ```1%d ``hit points.\n\r", ch->wimpy);
	send_to_char(buf, ch);

	if (!IS_NPC(ch) && (ch->race != (race = race_lookup("vampire")))
	    && ch->pcdata->condition[COND_DRUNK] > 10)
		send_to_char("You are ```2d```3r```2u```3n```2k``.\n\r", ch);
	if (!IS_NPC(ch) && (ch->race != (race = race_lookup("vampire")))
	    && ch->pcdata->condition[COND_THIRST] == 0)
		send_to_char("You are thirsty.\n\r", ch);
	if (!IS_NPC(ch) && (ch->race == (race = race_lookup("vampire")))
	    && ch->pcdata->condition[COND_HUNGER] == 0)
		send_to_char("You must `1feed`7.\n\r", ch);
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_IDIOT))
		send_to_char("You are an `Pidiot``.\n\r", ch);

	switch (ch->position) {
	case POS_DEAD:
		send_to_char("You are ```1D```8E```1A```8D``!!\n\r", ch);
		break;
	case POS_MORTAL:
		send_to_char("You are ```!mortally wounded``.\n\r", ch);
		break;
	case POS_INCAP:
		send_to_char("You are ```1incapacitated``.\n\r", ch);
		break;
	case POS_STUNNED:
		send_to_char("You are ```Pstunned``.\n\r", ch);
		break;
	case POS_SLEEPING:
		send_to_char("You are sleeping.\n\r", ch);
		break;
	case POS_RESTING:
		send_to_char("You are resting.\n\r", ch);
		break;
	case POS_STANDING:
		send_to_char("You are standing.\n\r", ch);
		break;
	case POS_FIGHTING:
		send_to_char("You are ```#fighting``.\n\r", ch);
		break;
	}

	sprintf(buf,
		"`O=================================================================``\n\r");

	send_to_char(buf, ch);



	if (IS_SET(ch->comm2, COMM2_AUTOTOKEN)) {
		sprintf(buf, "You have `!%u`` Fireproof Tokens.\n\r", ch->pcdata->fireproof);
		send_to_char(buf, ch);
		sprintf(buf, "You have `^%u`` AC Tokens.\n\r", ch->pcdata->armorclass);
		send_to_char(buf, ch);
		sprintf(buf, "You have `#%u`` 200/200 Tokens.\n\r", ch->pcdata->twohundred);
		send_to_char(buf, ch);
		sprintf(buf, "You have `@%u`` 10/10 Tokens.\n\r", ch->pcdata->tenten);
		send_to_char(buf, ch);
		sprintf(buf, "You have `O%u`` Weapon Flag Tokens.\n\r", ch->pcdata->weaponflag);
		send_to_char(buf, ch);
		sprintf(buf, "You have `P%u`` Damage Noun Tokens.\n\r", ch->pcdata->damnoun);
		send_to_char(buf, ch);
		sprintf(buf, "You have `&%u`` Restring Tokens.\n\r", ch->pcdata->restring);
		send_to_char(buf, ch);
		sprintf(buf, "You have `8%u`` Skill Set Tokens. (good for one skill each)\n\r", ch->pcdata->skillset);
		send_to_char(buf, ch);
		sprintf(buf, "`PImpLuvin`` Tokens: `2%u`` \n\r", ch->pcdata->imp);
		send_to_char(buf, ch);
		sprintf(buf, "You have been nominated for Excellent Role Playing `!%u`` times!\n\r", ch->pcdata->rp);
		send_to_char(buf, ch);
		sprintf(buf, "`O=================================================================``\n\r");
		send_to_char(buf, ch);
	}

/* print AC values */
	if (ch->level >= 25) {
		sprintf(buf, "Armor`8: ``pierce`8: `1%ld  ``bash`8: `1%ld  ``slash`8: `1%ld  ``magic`8: `1%ld``\n\r",
			GET_AC(ch, AC_PIERCE),
			GET_AC(ch, AC_BASH),
			GET_AC(ch, AC_SLASH),
			GET_AC(ch, AC_EXOTIC));
		send_to_char(buf, ch);
	}
	if (ch->level <= 25) {
		for (i = 0; i < 4; i++) {
			char *temp;

			switch (i) {
			case (AC_PIERCE):
				temp = "piercing";
				break;
			case (AC_BASH):
				temp = "bashing";
				break;
			case (AC_SLASH):
				temp = "slashing";
				break;
			case (AC_EXOTIC):
				temp = "magic";
				break;
			default:
				temp = "error";
				break;
			}

			send_to_char("You are ", ch);

			if (GET_AC(ch, i) >= 101)
				sprintf(buf, "hopelessly vulnerable to %s.\n\r", temp);
			else if (GET_AC(ch, i) >= 80)
				sprintf(buf, "defenseless against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= 50)
				sprintf(buf, "barely protected from %s.\n\r", temp);
			else if (GET_AC(ch, i) >= 25)
				sprintf(buf, "slightly armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= 0)
				sprintf(buf, "somewhat armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= 0)
				sprintf(buf, "armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -25)
				sprintf(buf, "well-armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -50)
				sprintf(buf, "very well-armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -80)
				sprintf(buf, "heavily armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -110)
				sprintf(buf, "superbly armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -150)
				sprintf(buf, "almost invulnerable to %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -250)
				sprintf(buf, "divinely armored against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -500)
				sprintf(buf, "near invinceable against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -1000)
				sprintf(buf, "armored like a tank against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -3000)
				sprintf(buf, "armored like an immortal against %s.\n\r", temp);
			else if (GET_AC(ch, i) >= -30000)
				sprintf(buf, "armored beyond belief against %s.\n\r", temp);
			else
				sprintf(buf, "an immortal.  Damage from %s means squat.\n\r", temp);

			send_to_char(buf, ch);
		}
	}

	if (ch->imm_flags)
		printf_to_char(ch, "Immune: %s\n\r", imm_bit_name(ch->imm_flags));

	if (ch->res_flags)
		printf_to_char(ch, "Resist: %s\n\r", imm_bit_name(ch->res_flags));

	if (ch->vuln_flags)
		printf_to_char(ch, "Vulnerable: %s\n\r", imm_bit_name(ch->vuln_flags));

	if (ch->affected_by)
		printf_to_char(ch, "Affected by: %s\n\r", affect_bit_name(ch->affected_by));
	if (ch->mLag != 0)
		printf_to_char(ch, "My Lag modified by: `P%d%``\n\r", ch->mLag);
	if (ch->tLag != 0)
		printf_to_char(ch, "Their Lag modified by: `#%d%``\n\r", ch->tLag);
	if (!IS_NPC(ch)) {
		sprintf(buf,
			"`O=================================================================``\n\r");

		send_to_char(buf, ch);


		sprintf(buf, "You have `!killed`` `P%ld`` mobs, and have been `!killed by`` `5%ld`` mobs.\n\r", ch->pcdata->mobkills, ch->pcdata->mobdeaths);
		send_to_char(buf, ch);
		sprintf(buf, "You have `!killed`` `P%ld`` players, and have been `!killed by`` `5%ld`` players.\n\r", ch->pcdata->pkills, ch->pcdata->pdeaths);
		send_to_char(buf, ch);
	}

/* RT wizinvis and holy light */
	if (IS_IMMORTAL(ch)) {
		send_to_char("Holy Light```8: ``", ch);
		if (IS_SET(ch->act, PLR_HOLYLIGHT))
			send_to_char("on", ch);
		else
			send_to_char("off", ch);

		if (ch->invis_level) {
			sprintf(buf, "  ```@W```Pi```@Z```Pi```8: ``level %d", ch->invis_level);
			send_to_char(buf, ch);
		}

		if (ch->incog_level) {
			sprintf(buf, "  ```6Incog```^nito```8: ``level %d", ch->incog_level);
			send_to_char(buf, ch);
		}
		send_to_char("\n\r", ch);
	}

	if (ch->level >= 15) {
		sprintf(buf, "```^Hitroll```6: ``%d  ```^Damroll```6: ``%d ```^Saves```6: ``%d.\n\r",
			GET_HITROLL(ch), GET_DAMROLL(ch), ch->saving_throw);
		send_to_char(buf, ch);
	}

	if (ch->level >= 10) {
		sprintf(buf, "Alignment```8: ``%d.  ", ch->alignment);
		send_to_char(buf, ch);
	}

	send_to_char("You are ", ch);
	if (ch->alignment > 900)
		send_to_char("```&angelic``.\n\r", ch);
	else if (ch->alignment > 700)
		send_to_char("```#saintly``.\n\r", ch);
	else if (ch->alignment > 350)
		send_to_char("```@good``.\n\r", ch);
	else if (ch->alignment > 100)
		send_to_char("```^kind``.\n\r", ch);
	else if (ch->alignment > -100)
		send_to_char("```Pneutral``.\n\r", ch);
	else if (ch->alignment > -350)
		send_to_char("```1mean``.\n\r", ch);
	else if (ch->alignment > -700)
		send_to_char("```!evil``.\n\r", ch);
	else if (ch->alignment > -900)
		send_to_char("```8DEMON``i```8C``.\n\r", ch);
	else
		send_to_char("```1SATAN```!i```1C``.\n\r", ch);

	if (!IS_NPC(ch)) printf_to_char(ch, "Extended Experience Points: %ld, Extended Level: %d\n\r",
					ch->pcdata->extendedexp, ch->pcdata->extendedlevel); if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
		do_affects(ch, "");
	if (ch->pk_timer > 0) {
		sprintf(buf, "`7You have `6%d `7ticks left on your pkill timer.``\n\r", ch->pk_timer);
		send_to_char(buf, ch);
	}
}

void do_affects(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_last = NULL;
	SKILL *skill;
	char buf[MSL];

	if (ch->affected != NULL) {
		send_to_char("You are affected by the following spells```8:``\n\r", ch);
		for (paf = ch->affected; paf != NULL; paf = paf->next) {
			if ((skill = resolve_skill_affect(paf)) == NULL)
				continue;

			if (paf_last != NULL
			    && paf->type == paf_last->type) {
				if (ch->level >= 20)
					sprintf(buf, "                       ");
				else
					continue;
			} else {
				sprintf(buf, "Spell```8: ```O%-16s``", skill->name);
			}

			send_to_char(buf, ch);

			if (ch->level >= 20) {
				sprintf(buf, "```8: ``modifies ```!%s ``by ```1%ld`` ",
					affect_loc_name((long)paf->location),
					paf->modifier);
				send_to_char(buf, ch);

				if (paf->duration == -1)
					sprintf(buf, "```&FOREVER!!!``");
				else
					sprintf(buf, "for ```O%d ``hours.", paf->duration);

				send_to_char(buf, ch);
			}

			send_to_char("\n\r", ch);
			paf_last = paf;
		}
	} else {
		send_to_char("You are not affected by any spells``.\n\r", ch);
	}

	return;
}



char *const day_name[] =
{
	"the Moon",	  "the Bull", "Deception", "Thunder", "Freedom",
	"the Great Gods", "the Sun"
};

char *const month_name[] =
{
	"Winter",	      "the Winter Wolf",      "the Frost Giant", "the Old Forces",
	"the Grand Struggle", "the Spring",	      "Nature",		 "Futility",	   "the Dragon",
	"the Sun",	      "the Heat",	      "the Battle",	 "the Dark Shades","the Shadows",
	"the Long Shadows",   "the Ancient Darkness", "the Great Evil"
};

void do_time(CHAR_DATA *ch, char *argument)
{
	extern char boot_time[];
	char *suf;
	int day;

	day = time_info.day + 1;
	switch (day) {
	case 1:
		suf = "st";
		break;
	case 2:
		suf = "rd";
		break;
	case 3:
		suf = "rd";
		break;
	default:
		suf = "th";
		break;
	}

	/*
	 */

	printf_to_char(ch, "\n\rIt is the Day of %s, %d%s day in the Month of %s.\n\r",
		       day_name[day % 7],
		       day,
		       suf,
		       month_name[time_info.month]);

	printf_to_char(ch, "The current time is %d o'clock %s.\n\r",
		       (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
		       time_info.hour >= 12 ? "pm" : "am");

	send_to_char("\n\r\n\r`1-`!-----------------------------------------------------------------------------`1-``\n\r", ch);

	printf_to_char(ch, "This `#B`Pa`@d `#T`Pr`@i`#p`` kicked in at `O%s``\r"
		       "The `#B`Pa`@d `#T`Pr`@i`#p`` Server clock is screaming, `&%s``\r",
		       boot_time, (char *)ctime(&current_time));

	return;
}


void do_count(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	int count;
	extern int max_on;

	count = 0;
	for (d = descriptor_list; d != NULL; d = d->next)
		if (d->connected == CON_PLAYING && can_see(ch, d->character))
			count++;

	if (max_on < count)
		max_on = count;

	if (max_on == count)
		printf_to_char(ch, "There are ```@%d ``characters on, the most so far today.\n\r", count);

	else
		printf_to_char(ch, "There are ```2%d ``characters on, the most on today was ```@%d``.\n\r",
			       count, max_on);
}

void do_inventory(CHAR_DATA *ch, char *argument)
{
	send_to_char("You are carrying```8:``\n\r", ch);
	show_list_to_char(ch->carrying, ch, TRUE, TRUE);
	return;
}



/* New do_equipment() function. Shows eq slots not worn.*/
void do_equipment(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	int iWear;

	send_to_char("`&  You are using: \n\r ------------------``\n\r", ch);
	for (iWear = 0; where_name[iWear].wear_loc >= 0; iWear++) {
		if ((obj = get_eq_char(ch, where_name[iWear].wear_loc)) == NULL) {
			if (!(IS_SET(ch->act, PLR_AUTOEQ)))
				continue;

			send_to_char("`1", ch);
			send_to_char(where_name[iWear].desc, ch);

			if (where_name[iWear].wear_loc == WEAR_THIRD) {
				if ((ch->race) == (race_lookup("mutant")))
					send_to_char("`1     --Empty--``\r\n", ch);
				else
					send_to_char("`1     --Not Available--``\r\n", ch);
			} else {
				if ((where_name[iWear].wear_loc == WEAR_FINGER_L2)) {
					if ((ch->race) == (race_lookup("human")))
						send_to_char("`1     --Empty--``\r\n", ch);
					else
						send_to_char("`1     --Not Available--``\r\n", ch);
				} else {
					if ((where_name[iWear].wear_loc == WEAR_FINGER_R2)) {
						if ((ch->race) == (race_lookup("human")))
							send_to_char("`1     --Empty--``\r\n", ch);
						else
							send_to_char("`1     --Not Available--``\r\n", ch);
					} else {
						send_to_char("`1     --Empty--``\r\n", ch);
					}
				}
			}
			continue;
		}

		send_to_char("`7", ch);
		send_to_char(where_name[iWear].desc, ch);
		send_to_char("`&", ch);
		if (can_see_obj(ch, obj)) {
			send_to_char("`&", ch);
			send_to_char(format_obj_to_char(obj, ch, TRUE), ch);
			send_to_char("``\n\r", ch);
		} else {
			send_to_char("`&something.\n\r", ch);
		}
	}

	send_to_char("`8", ch);
	return;
}




void do_compare(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	OBJ_DATA *obj1;
	OBJ_DATA *obj2;
	long value1;
	long value2;
	char *msg;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("Compare what to what to what to what?\n\r", ch);
		return;
	}

	if ((obj1 = get_obj_carry(ch, arg1)) == NULL) {
		send_to_char("You do not have one, dorkus.\n\r", ch);
		return;
	}

	if (arg2[0] == '\0') {
		for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content) {
			if (obj2->wear_loc != WEAR_NONE
			    && can_see_obj(ch, obj2)
			    && obj1->item_type == obj2->item_type
			    && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
				break;
		}

		if (obj2 == NULL) {
			send_to_char("You aren't wearing anything comparable.\n\r", ch);
			return;
		}
	} else if ((obj2 = get_obj_carry(ch, arg2)) == NULL) {
		send_to_char("You do not have one, dorkus.\n\r", ch);
		return;
	}

	msg = NULL;
	value1 = 0;
	value2 = 0;

	if (obj1 == obj2) {
		msg = "You compare $p to itself.  It looks about the same.";
	} else if (obj1->item_type != obj2->item_type) {
		msg = "You can't compare $p and $P.";
	} else {
		switch (obj1->item_type) {
		default:
			msg = "You can't compare $p and $P.";
			break;

		case ITEM_ARMOR:
			value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
			value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
			break;

		case ITEM_WEAPON:
			if (obj1->obj_idx->new_format)
				value1 = (1l + obj1->value[2]) * obj1->value[1];
			else
				value1 = obj1->value[1] + obj1->value[2];

			if (obj2->obj_idx->new_format)
				value2 = (1l + obj2->value[2]) * obj2->value[1];
			else
				value2 = obj2->value[1] + obj2->value[2];
			break;
		}
	}

	if (msg == NULL) {
		if (value1 == value2)
			msg = "$p and $P look about the same.";
		else if (value1 > value2)
			msg = "$p looks kickass compared to $P.";
		else
			msg = "$p looks like cow dung compared to $P.";
	}

	act(msg, ch, obj1, obj2, TO_CHAR);
	return;
}



void do_credits(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "diku");
	return;
}


void do_here(CHAR_DATA *ch, char *argument)
{
	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING) {
		send_to_char("You can't see anything but stars!\n\r", ch);
		return;
	}

	if (ch->position == POS_SLEEPING) {
		send_to_char("You can't see anything, you're sleeping!\n\r", ch);
		return;
	}

	if (!check_blind(ch))
		return;

	if (!IS_NPC(ch)
	    && !IS_SET(ch->act, PLR_HOLYLIGHT)
	    && room_is_dark(ch, ch->in_room)) {
		send_to_char("It is pitch ```8black...`` \n\r", ch);
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	send_to_char(" People in the room:\n\r", ch);
	show_char_to_char(ch->in_room->people, ch);
}


void do_consider(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	CHAR_DATA *victim;
	char *msg;
	int diff;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Consider killing whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They're not here.\n\r", ch);
		return;
	}

	if (is_safe(ch, victim)) {
		send_to_char("Don't even think about it.\n\r", ch);
		return;
	}

	diff = victim->level - ch->level;

	if (diff <= -10)
		msg = "You can kill $N bound and gagged... hey, kinky.";
	else if (diff <= -5)
		msg = "$N is a wuss... kick his ass.";
	else if (diff <= -2)
		msg = "$N looks like an easy kill.";
	else if (diff <= 1)
		msg = "Even odds, unless you rock...";
	else if (diff <= 4)
		msg = "$N mutters 'Do you feel yucky, spunk?'.";
	else if (diff <= 9)
		msg = "$N laughs at you mercilessly.  You will die a funny death.";
	else
		msg = "Death will thank you for your soul, and your cigarette will most likely go out.";

	act(msg, ch, NULL, victim, TO_CHAR);
	return;
}



void set_title(CHAR_DATA *ch, char *title)
{
	char buf[MSL];

	if (IS_NPC(ch)) {
		bug("Set_title: NPC.", 0);
		return;
	}

	if (title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' && title[0] != ';') {
		buf[0] = ' ';
		strcpy(buf + 1, title);
	} else {
		strcpy(buf, title);
	}

	free_string(ch->pcdata->title);
	ch->pcdata->title = str_dup(buf);
	return;
}



/***************************************************************************
*	do_title
***************************************************************************/
void do_title(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (argument[0] == '\0') {
		send_to_char("Change your title to what?\n\r", ch);
		return;
	}

	if (strlen(argument) > MAX_TITLE_LENGTH)
		argument[MAX_TITLE_LENGTH] = '\0';

	smash_tilde(argument);
	set_title(ch, argument);
	send_to_char("Ok.\n\r", ch);
}


/***************************************************************************
*	do_deathcry
***************************************************************************/
void do_deathcry(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	int rand;

	if (IS_NPC(ch))
		return;

	if ((argument[0] == '\0')
	    && (ch->pcdata->deathcry == NULL)) {
		send_to_char("Change your death cry to what?\n\r", ch);
		return;
	} else if (argument[0] == '\0') {
		sprintf(buf, "Your `1death `!cry`` is`8:`` %s\n\r", ch->pcdata->deathcry);
		send_to_char(buf, ch);
		return;
	}

	if (!str_prefix(argument, "reset")) {
		rand = number_range(0, 9);
		switch (rand) {
		case 1:
			do_deathcry(ch, "!@#$!@#$%!@#$!!@@!");
			break;
		case 2:
			do_deathcry(ch, "Oh my. I seem to have been killed. Buttworth will be MOST displeased.");
			break;
		case 3:
			do_deathcry(ch, "`2*`@gleep`2*`! I'm DEAD.");
			break;
		case 4:
			do_deathcry(ch, "GoddamnitIreallyamgettingsickandtiredofDYING!");
			break;
		case 5:
			do_deathcry(ch, "Dammit, I HATE it when that happens!");
			break;
		case 6:
			do_deathcry(ch, "Oh NO! Not again!!");
			break;
		case 7:
			do_deathcry(ch, "The weasels! THE WEASELS!!! AAAGH!!");
			break;
		case 8:
			do_deathcry(ch, "Oh `#SH`&i`#T`!!!");
			break;
		default:
			do_deathcry(ch, "AAAAAAAAAAAAAAAAAARGH! I'm DEAD!!");
			break;
		}

		return;
	}

	if (strlen(argument) > MIL)
		argument[MIL] = '\0';

	smash_tilde(argument);
	ch->pcdata->deathcry = str_dup(argument);

	sprintf(buf, "Your `1death `!cry`` is`8:`` %s\n\r", ch->pcdata->deathcry);
	send_to_char(buf, ch);
}


/***************************************************************************
*	do_description
***************************************************************************/
void do_description(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	if (IS_NPC(ch))
		return;

	if (argument[0] != '\0') {
		buf[0] = '\0';
		smash_tilde(argument);

		if (!str_prefix(argument, "edit")) {
			string_append(ch, &ch->description);
			return;
		}

		if (argument[0] == '-') {
			int len, buf_len;
			bool found = FALSE;

			if (ch->description == NULL || ch->description[0] == '\0') {
				send_to_char("No lines left to remove.\n\r", ch);
				return;
			}

			strcpy(buf, ch->description);
			buf_len = (int)strlen(buf);
			for (len = buf_len; len > 0; len--) {
				if (buf[len] == '\r') {
					if (!found) {            /* back it up */
						if (len > 0)
							len--;
						found = TRUE;
					} else {
						/* found the second one */
						buf[len + 1] = '\0';
						free_string(ch->description);
						ch->description = str_dup(buf);
						send_to_char("Your description is```8:``\n\r", ch);
						send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
						return;
					}
				}
			}
			buf[0] = '\0';
			free_string(ch->description);
			ch->description = str_dup(buf);
			send_to_char("Description cleared.\n\r", ch);
			return;
		}

		if (argument[0] == '+') {
			if (ch->description != NULL)
				strcat(buf, ch->description);
			argument++;
			while (is_space(*argument))
				argument++;
		}

		if (strlen(buf) + strlen(argument) >= 1024) {
			send_to_char("Description too long.\n\r", ch);
			return;
		}

		strcat(buf, argument);
		strcat(buf, "\n\r");
		free_string(ch->description);
		ch->description = str_dup(buf);
	}

	send_to_char("Your description is```8:``\n\r", ch);
	send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
	return;
}



void do_report(CHAR_DATA *ch, char *argument)
{
	char buf[MIL];

	sprintf(buf,
		"You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\n\r",
		ch->hit, ch->max_hit,
		ch->mana, ch->max_mana,
		ch->move, ch->max_move,
		ch->exp);

	send_to_char(buf, ch);

	sprintf(buf, "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'",
		ch->hit, ch->max_hit,
		ch->mana, ch->max_mana,
		ch->move, ch->max_move,
		ch->exp);

	act(buf, ch, NULL, NULL, TO_ROOM);

	return;
}






/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	int wimpy;

	one_argument(argument, arg);

	if (arg[0] == '\0')
		wimpy = ch->max_hit / 5;
	else
		wimpy = atoi(arg);

	if (wimpy < 0) {
		send_to_char("Your courage exceeds your wisdom.\n\r", ch);
		return;
	}

	if (wimpy > ch->max_hit / 2) {
		send_to_char("Such cowardice ill becomes you.  Grow some nads.\n\r", ch);
		return;
	}

	ch->wimpy = wimpy;
	printf_to_char(ch, "You will run like a dog at %d hit points.\n\r", wimpy);
	return;
}



void do_password(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	char *pArg;
	char *pwdnew;
	char *p;
	char cEnd;

	if (IS_NPC(ch))
		return;

/*
 * Can't use one_argument here because it smashes case.
 * So we just steal all its code.  Bleagh.
 */
	pArg = arg1;
	while (is_space(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	pArg = arg2;
	while (is_space(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Syntax```8: ``password <old> <new>.\n\r", ch);
		return;
	}

	if (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd)) {
		WAIT_STATE(ch, 40);
		send_to_char("Wrong password.  Wait 10 seconds.\n\r", ch);
		return;
	}

	if (strlen(arg2) < 5) {
		send_to_char(
			"New password must be at least five characters long.\n\r", ch);
		return;
	}

/*
 * No tilde allowed because of player file format.
 */
	pwdnew = crypt(arg2, ch->name);
	for (p = pwdnew; *p != '\0'; p++) {
		if (*p == '~') {
			send_to_char(
				"New password not acceptable, try again.\n\r", ch);
			return;
		}
	}

	free_string(ch->pcdata->pwd);
	ch->pcdata->pwd = str_dup(pwdnew);
	save_char_obj(ch);
	send_to_char("Password changed... Hope you remember it.\n\r", ch);
	return;
}

void do_finger(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	FILE *fp;
	char arg[MIL];
	char buf[MSL];
	char hours[MSL];
	char mins[MSL];
	char secs[MSL];
	bool fOld;
	bool vOnline = TRUE;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax: finger <name>\n\r\n\r", ch);
		send_to_char("The name you enter `!MUST`7\n\r", ch);
		send_to_char("be entered in full.\n\r", ch);
		return;
	}

	/* Use the currently loaded character in the game, as long as they're
	 *      not a mob.  Added by Monrick, 1/2008 */

	if ((victim = get_char_world(ch, arg)) == NULL || IS_NPC(victim)) {
		if ((victim != NULL) && IS_NPC(victim)) {
			sprintf(buf,
				"`@%s `@tells you '`tPull that again and I'm gonna kick your ass ..`@'``\n\r",
				capitalize(victim->short_descr));
			send_to_char(buf, ch);
			return;
		}

		victim = new_char();
		victim->pcdata = new_pcdata();
		fOld = FALSE;
		vOnline = FALSE;

		sprintf(buf, "%s%s", PLAYER_DIR, capitalize(arg));
		if ((fp = fopen(buf, "r")) != NULL) {
			int iNest;

			for (iNest = 0; iNest < MAX_NEST; iNest++)
				rgObjNest[iNest] = NULL;

			fOld = TRUE;
			for (;; ) {
				char letter;
				char *word;

				letter = fread_letter(fp);
				if (letter == '*') {
					fread_to_eol(fp);
					continue;
				}

				if (letter != '#') {
					bug("Load_char_obj: # not found.", 0);
					break;
				}

				word = fread_word(fp);
				if (!str_cmp(word, "PLAYER"))
					fread_char(victim, fp);
				else
					break;
			}

			fclose(fp);
		}

		if (!fOld) {
			send_to_char("Invalid character.\n\r", ch);
			free_pcdata(victim->pcdata);
			free_char(victim);
			return;
		}
	}

	send_to_char("`8 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-``\n\r", ch);
	sprintf(buf, "`& Total       `7Hours`8:`6 %-10d      `7Minutes`8:`6 %-3d           `7Seconds`8:`6 %-3d``\n\r",
		get_hours_played(victim),
		get_minutes_played(victim),
		get_seconds_played(victim));
	send_to_char(buf, ch);
	sprintf(hours, "%2.2d", get_session_hours(victim));
	sprintf(mins, "%2.2d", get_session_minutes(victim));
	sprintf(secs, "%2.2d", get_session_seconds(victim));
	sprintf(buf, "`& Today      `7 Hours`8:`6 %-10s     `7 Minutes`8:`6 %-3s           `7Seconds`8:`6 %-3s``\n\r", hours, mins, secs);
	send_to_char(buf, ch);
	send_to_char("`8 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-``\n\r", ch);
	sprintf(buf, "             `7Level`8:`6 %-3d                `7Race`8:`6 %-9s       `7Class`8:`6 %-7s``\n\r",
		victim->level, race_table[victim->race].name, class_table[victim->class].name);
	send_to_char(buf, ch);
	sprintf(buf, "      `7Player Kills`8:`6 %-4ld     `7 Player Deaths`8:`6 %-4ld        `7Mob Kills`8:`6 %ld``\n\r",
		victim->pcdata->pkills, victim->pcdata->pdeaths, victim->pcdata->mobkills);
	send_to_char(buf, ch);
	sprintf(buf, "`7 Battlefield Kills`8:`6 %-3d  `7Battlefield Deaths`8:`6 %-3d        `7Mob Deaths`8:`6 %ld``\n\r",
		victim->bkills, victim->bloss, victim->pcdata->mobdeaths);
	send_to_char(buf, ch);
	send_to_char("`8 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-``\n\r", ch);

	send_to_char(victim->description, ch);
	if (!vOnline) {
		free_pcdata(victim->pcdata);
		free_char(victim);
	}
	return;
}

void do_finger2(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char buf[MSL];
	char argh[MIL];

	argument = one_argument(argument, argh);
	victim = get_char_world(ch, argh);

	if (argh[0] == '\0') {
		do_ewho(ch, "");
		return;
	}

	if (victim == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		sprintf(buf,
			"`@%s tells you '`tPull that again and I'm gonna kick your ass ..`@'`7\n\r",
			capitalize(victim->short_descr));
		send_to_char(buf, ch);
		return;
	}

	sprintf(buf, "Pkills: `!%ld``\n\rPdeaths: `O%ld``\n\rLevel: `#%d``\n\r",
		victim->pcdata->pkills,
		victim->pcdata->pdeaths,
		victim->level);
	send_to_char(buf, ch);

	if (victim->description[0] != '\0') {
		sprintf(buf, "%s's description:\n\r--------------------------------------------------------------------------------\n\r", victim->name);
		send_to_char(buf, ch);
		send_to_char(victim->description, ch);
	} else {
		sprintf(buf, "There is nothing special about %s.\n\r", victim->name);
		send_to_char(buf, ch);
	}
	return;
}

void do_laston(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim = ch;
	FILE *fp;
	char arg[MIL];
	char buf[MSL];
	bool fOld;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax: laston <name>\n\r\n\r", ch);
		send_to_char("The name you enter `!MUST`7\n\r", ch);
		send_to_char("be entered in full.\n\r", ch);
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = CH(d);

		if (!can_see(ch, wch))
			continue;

		if (!str_cmp(arg, wch->name)) {
			if (victim->name == ch->name) {
				send_to_char("`@Acid-Fiend-1 tells you '`tAre you stupid?`@'`7\n\r", ch);
				return;
			}

			if (!can_see(ch, wch)) {
				sprintf(buf, "`@Acid-Fiend-1 tells you '`t%s was last here on %s`@'`7\n\r",
					victim->name, (char *)ctime(&victim->llogoff));
				send_to_char(buf, ch);
				return;
			}

			act("`@Acid-Fiend-1 tells you '`t$N is currently connected ..`@'`7", ch, NULL, victim, TO_CHAR);
			return;
		}
	}

	victim = new_char();
	victim->pcdata = new_pcdata();
	fOld = FALSE;

	sprintf(buf, "%s%s", PLAYER_DIR, capitalize(arg));
	if ((fp = fopen(buf, "r")) != NULL) {
		int iNest;

		for (iNest = 0; iNest < MAX_NEST; iNest++)
			rgObjNest[iNest] = NULL;

		fOld = TRUE;
		for (;; ) {
			char letter;
			char *word;

			letter = fread_letter(fp);
			if (letter == '*') {
				fread_to_eol(fp);
				continue;
			}

			if (letter != '#') {
				bug("Load_char_obj: # not found.", 0);
				break;
			}

			word = fread_word(fp);
			if (!str_cmp(word, "PLAYER"))
				fread_char(victim, fp);
			else
				break;
		}

		fclose(fp);
	}

	if (!fOld) {
		send_to_char("`@Acid-Fiend-1 tells you '`tNo player by that name exists.`@'`7\n\r", ch);
		free_pcdata(victim->pcdata);
		free_char(victim);
		return;
	}

	if (victim->name == ch->name) {
		send_to_char("`@Acid-Fiend-1 tells you '`tAre you stupid?`@'`7\n\r", ch);
		free_pcdata(victim->pcdata);
		free_char(victim);
		return;
	}

	sprintf(buf, "`@Acid-Fiend-1 tells you '`t%s was last here on %s`@'`7",
		victim->name, (char *)ctime(&victim->llogoff));
	send_to_char(buf, ch);
	free_pcdata(victim->pcdata);
	free_char(victim);
	return;
}

void print_weather(CHAR_DATA *ch)
{
	static char buf[MSL];
	static char *const sky_look[4] =
	{
		"`Ocloudless``",
		"`Opa`&rtl`Oy c`&lo`Oudy``",
		"`8r`4a`8i`4n`8y`` as `1hell``",
		"lit by flashes of `8l`&i`8ghtning``"
	};

	buf[0] = '\0';


	sprintf(buf, "The sky is %s and %s\n\r",
		sky_look[weather_info.sky],
		weather_info.change >= 0
		? "rainy."
		: "snowing."
		);
	send_to_char(buf, ch);
}

void do_weather(CHAR_DATA *ch, char *argument)
{
	if (!IS_OUTSIDE(ch)) {
		send_to_char("You can't see the weather indoors.\n\r", ch);
		return;
	}

	print_weather(ch);
}


char *format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort)
{
	static char buf[MSL * 2];

	buf[0] = '\0';

	if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
	    || (obj->description == NULL || obj->description[0] == '\0'))
		return buf;





	if (IS_OBJ_STAT(obj, ITEM_INVIS) ||
	    (IS_AFFECTED(ch, AFF_DETECT_EVIL) && IS_OBJ_STAT(obj, ITEM_EVIL)) ||
	    (IS_AFFECTED(ch, AFF_DETECT_GOOD) && IS_OBJ_STAT(obj, ITEM_BLESS)) ||
	    (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC)) ||
	    IS_OBJ_STAT(obj, ITEM_GLOW) ||
	    IS_OBJ_STAT(obj, ITEM_HUM) ||
	    IS_OBJ_STAT(obj, ITEM_CACHE) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY1) ||
	    IS_OBJ_STAT2(obj, ITEM2_RELIC) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY2))
		strcat(buf, "(");
	;


	if (IS_OBJ_STAT(obj, ITEM_INVIS))
		strcat(buf, "```8I``");
	if (IS_AFFECTED(ch, AFF_DETECT_EVIL)
	    && IS_OBJ_STAT(obj, ITEM_EVIL))
		strcat(buf, "```1E`8``");
	if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
	    && IS_OBJ_STAT(obj, ITEM_BLESS))
		strcat(buf, "```OG``");
	if (IS_AFFECTED(ch, AFF_DETECT_MAGIC)
	    && IS_OBJ_STAT(obj, ITEM_MAGIC))
		strcat(buf, "```^M``");
	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		strcat(buf, "```@G``");
	if (IS_OBJ_STAT(obj, ITEM_HUM))
		strcat(buf, "```PH``");
	if (IS_OBJ_STAT(obj, ITEM_CACHE))
		strcat(buf, "`@C``");
	if (IS_OBJ_STAT(obj, ITEM_INLAY1))
		strcat(buf, "```2G``");
	if (IS_OBJ_STAT2(obj, ITEM2_RELIC))
		strcat(buf, "`!Relic``");
	if (IS_OBJ_STAT(obj, ITEM_INLAY2))
		strcat(buf, "```4B``");

	if (IS_OBJ_STAT(obj, ITEM_INVIS) ||
	    (IS_AFFECTED(ch, AFF_DETECT_EVIL) && IS_OBJ_STAT(obj, ITEM_EVIL)) ||
	    (IS_AFFECTED(ch, AFF_DETECT_GOOD) && IS_OBJ_STAT(obj, ITEM_BLESS)) ||
	    (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC)) ||
	    IS_OBJ_STAT(obj, ITEM_GLOW) ||
	    IS_OBJ_STAT(obj, ITEM_HUM) ||
	    IS_OBJ_STAT(obj, ITEM_CACHE) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY1) ||
	    IS_OBJ_STAT2(obj, ITEM2_RELIC) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY2))
		strcat(buf, ") ");
	;


	if (fShort) {
		if (obj->short_descr != NULL)
			strcat(buf, obj->short_descr);
	} else {
		if (obj->description != NULL)
			strcat(buf, obj->description);
	}

	return buf;
}


void do_die(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "BLOW");
	return;
}


HELP_DATA *find_help(CHAR_DATA *ch, char *name)
{
	HELP_DATA *pHelp;


	for (pHelp = help_first; pHelp != NULL; pHelp = pHelp->next) {
		if (pHelp->level > get_trust(ch))
			continue;

		if (is_name(name, pHelp->keyword))
			return pHelp;
	}

	return NULL;
}



/* add this function - add definition in merc.h*/
void show_damage_display(CHAR_DATA *ch, CHAR_DATA *victim)
{
	char buf[MSL];
	int percent;

	/* if the victims race is human and it is a PC, it shows nothing*/
	if (IS_NPC(victim) || victim->race != race_lookup("human")) {
		strcpy(buf, PERS(victim, ch));

		if (victim->max_hit > 0)
			percent = (100 * victim->hit) / victim->max_hit;
		else
			percent = -1;

		if (percent >= 100)
			strcat(buf, " `Cis in excellent condition.``\n\r");
		else if (percent >= 90)
			strcat(buf, " `Chas a few scratches.``\n\r");
		else if (percent >= 75)
			strcat(buf, " `Chas some small wounds and bruises.``\n\r");
		else if (percent >= 50)
			strcat(buf, " `Chas quite a few wounds.``\n\r");
		else if (percent >= 30)
			strcat(buf, " `Chas some big nasty wounds and scratches.``\n\r");
		else if (percent >= 15)
			strcat(buf, "`C looks pretty hurt.``\n\r");
		else if (percent >= 0)
			strcat(buf, "`C is in awful condition.``\n\r");
		else
			strcat(buf, " `Cis bleeding to death.``\n\r");

		buf[0] = UPPER(buf[0]);
		send_to_char(buf, ch);

		/* add this to allow humans to see actual stats	- does not*/
		/* work with a human victim if the victim is a PC*/
		if (ch->race == race_lookup("human"))
			printf_to_char(ch, "\n\r%s `^has `6%ld`^ hit points and `6%ld `^mana.``\n\r", PERS(victim, ch), victim->hit, victim->mana);
	}
}

/*
 * Allows PC to make and store a history.
 * by TAKA
 * (c) 1999 TAKA of the Ghost Dancer MUD Project
 *   Modified (rewritten) by Monrick: January 6, 2008
 *    for Bad Trip
 */
void do_history(CHAR_DATA *ch, char *argument)
{
	char cmd[MSL];
	int len;
	bool found = FALSE;
	size_t MAX_HIST_LENGTH = 4096;

	if (IS_NPC(ch))
		return;

	if ((ch->in_room->vnum > ROOM_VNUM_JAIL_START)
	    && (ch->in_room->vnum < ROOM_VNUM_JAIL_END)
	    && (!IS_IMMORTAL(ch))) {
		send_to_char("Not in jail...\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
print_hist_help:
		send_to_char("Syntax: history [cmd]\n\r\n\r", ch);
		send_to_char("Available commands:\n\r", ch);
		printf_to_char(ch, "%-10.10s", "+");
		printf_to_char(ch, "%-10.10s", "-");
		/*printf_to_char(ch, "%-10.10s", "edit");*/
		printf_to_char(ch, "%-10.10s", "clear");
		/*send_to_char("\n\r", ch);*/
		printf_to_char(ch, "%-10.10s", "show");
		send_to_char("\n\r\n\r", ch);
		return;
	}

	smash_tilde(argument);
	argument = one_argument(argument, cmd);

	if (cmd[0] == '\0') {
		send_to_char("Your history is:\n\r", ch);
		page_to_char(ch->pcdata->history ? ch->pcdata->history : "(None).\n\r", ch);
		return;
	} else {
		if (cmd[0] == '+') {
			char buf[MSL];

			if (ch->pcdata->history == NULL || ch->pcdata->history[0] == '\0') {
				if (strlen(argument) >= MAX_HIST_LENGTH) {
					send_to_char("History too long.\n\r", ch);
					return;
				}
			} else if (strlen(ch->pcdata->history) + strlen(argument) >= MAX_HIST_LENGTH) {
				send_to_char("History too long.\n\r", ch);
				return;
			}

			strcpy(buf, ch->pcdata->history);
			strcat(buf, argument);
			strcat(buf, "\n\r");

			free_string(ch->pcdata->history);
			ch->pcdata->history = str_dup(buf);

			send_to_char("Ok.\n\r", ch);
			return;
		} else if (cmd[0] == '-') {
			char buf[MSL];
			int buf_len;

			if (ch->pcdata->history == NULL || ch->pcdata->history[0] == '\0') {
				send_to_char("No lines left to remove.\n\r", ch);
				return;
			}


			strcpy(buf, ch->pcdata->history);
			buf_len = (int)strlen(buf);
			for (len = buf_len; len > 0; len--) {
				if (buf[len] == '\r') {
					/* ignore the first '\r' since we are starting at the end */
					if (!found) {
						if (len > 0)
							len--;
						found = TRUE;
					} else {
						/* crop off everything after the second '\r' from the end */
						buf[len + 1] = '\0';
						break;
					}
				}
			}

			if (len <= 0)
				buf[0] = '\0';

			free_string(ch->pcdata->history);
			ch->pcdata->history = str_dup(buf);

			send_to_char("Ok.\n\r", ch);
			return;
		} else if (!strcmp(cmd, "edit")) {
/* MUST KILL GOTO */
			goto print_hist_help;

/*
 *                      strcpy(buf, ch->pcdata->history);
 *
 *                      string_append(ch, &buf);
 *                      while (strlen(buf) >= MAX_HIST_LENGTH)
 *                      {
 *                              send_to_char("History too long.  Please make it shorter.\n\r", ch);
 *                              string_append( ch, &buf);
 *                      }
 *
 *                      free_string(ch->pcdata->history);
 *                      ch->pcdata->history = str_dup(buf);
 *
 *                      send_to_char("Ok.\n\r.", ch);
 *                      return;
 */
		} else if (!strcmp(cmd, "clear")) {
			if (ch->pcdata->history != NULL && ch->pcdata->history[0] != '\0') {
				free_string(ch->pcdata->history);
				ch->pcdata->history = NULL;
			}
			send_to_char("History cleared.\n\r", ch);
			return;
		} else if (strcmp(cmd, "show")) {
			goto print_hist_help;
		}
	}

	if ((ch->pcdata->history != NULL) && (ch->pcdata->history[0] != '\0')) {
		send_to_char("Your history is:\n\r", ch);
		page_to_char(ch->pcdata->history, ch);
	} else {
		send_to_char("You have no history.  Perhaps you should write one...\n\r", ch);
	}
	return;
}

void do_viewhist(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0') {
		send_to_char("`!Whose history do you want to read?``\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("`!They aren't here.``\n\r", ch);
		return;
	}

	if (IS_NPC(victim))
		return;

	if (victim->pcdata->history != NULL && victim->pcdata->history[0] != '\0') {
		printf_to_char(ch, "%s's history is:\n\r", victim->name);
		page_to_char(victim->pcdata->history, ch);
	} else {
		printf_to_char(ch, "%s doesn't have a history.\n\r", victim->name);
	}
	return;
}

void do_radio(CHAR_DATA *ch, char *argument)
{
	send_to_char("You can connect to BTRR at btrr.strangled.net:8000/listen.pls\n\r", ch);
	send_to_char("Note: This is not a mud. You will need to use a media player\n\r", ch);
	send_to_char("to connect. Please see help radio_connect for more info.\n\r", ch);
	return;
}
