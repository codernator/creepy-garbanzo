/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael          *
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
*       Russ Taylor(rtaylor@hypercube.org)                                *
*       Gabrielle Taylor(gtaylor@hypercube.org)                           *
*       Brian Moore(zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "lookup.h"
#include "tables.h"
#include "libstring.h"


/***************************************************************************
*	is_whois_target
***************************************************************************/
static bool is_whois_target(CHAR_DATA *ch, char *arg)
{
	if (!str_prefix(arg, "it") && IS_SET(ch->act, PLR_IT))
		return TRUE;

	if (!str_prefix(arg, ch->name))
		return TRUE;


	return FALSE;
}


/***************************************************************************
*	who_string
***************************************************************************/
char *who_string(CHAR_DATA *ch)
{
	const char *class;
	static char buf[512];
	char tmp[512];

	buf[0] = '\0';
	if (IS_NPC(ch))
		return buf;

	class = class_table[ch->class].who_name;

	if (!str_prefix("Wit", class)
	    && ch->sex == sex_lookup("male"))
		class = "Wrl";

	switch (ch->level) {
	case MAX_LEVEL - 0:
		class = "`2I`8M`2P``";
		break;
	case MAX_LEVEL - 1:
		class = "`4C`OR`4E``";
		break;
	case MAX_LEVEL - 2:
		class = "`1S`!U`1P``";
		break;
	case MAX_LEVEL - 3:
		class = "`@D`7E`@I``";
		break;
	case MAX_LEVEL - 4:
		class = "`5G`PO`5D``";
		break;
	case MAX_LEVEL - 5:
		class = "`^IMM``";
		break;
	case MAX_LEVEL - 6:
		class = "`ODEM``";
		break;
	case MAX_LEVEL - 7:
		class = "`#ANG``";
		break;
	case MAX_LEVEL - 8:
		class = "`&AVA``";
		break;
	}

	if (IS_SET(ch->act, PLR_IT))
		class = "`&IT!``";

	if (ch->pcdata->who_thing[0] != '\0') {
		sprintf(buf, "%s", ch->pcdata->who_thing);
	} else {
		sprintf(buf,
			"%s``%3d %s %s%s ",
			"`!(",
			ch->level,
			ch->race < MAX_PC_RACE ? pc_race_table[ch->race].who_name : "     ",
			class,
			"`!)");
	}


	sprintf(tmp,
		"``%s%s%s%s%s%s%s%s%s%s%s%s``",
		IS_SET(ch->comm2, COMM2_RP) ? "`8[`!RP`8]`` " : "",
		IS_SET(ch->comm2, COMM2_OOC) ? "`8[`#OOC`8]`` " : "",
		ch->incog_level >= LEVEL_HERO ? "`6(`^Incog`6) ``" : "",
		ch->invis_level >= 1 ? "`8(`@W`Pi`@Z`Pi`8) ``" : "",
		IS_SET(ch->comm2, COMM2_AFK) ? "`!A`@F`OK`` " : "",
		IS_SET(ch->comm2, COMM2_CODING) ? "[`@CODING``] " : "",
		IS_SET(ch->comm2, COMM2_BUILD) ? "[`3BUILDING``] " : "",
		IS_SET(ch->comm2, COMM2_BUSY) ? "[`1BUSY``] " : "",
		IS_SET(ch->act, PLR_KILLER) ? "-`1K`!i`1LLER``- " : "",
		IS_SET(ch->act, PLR_THIEF) ? "-`8TH``i`8EF``- " : "",
		IS_SET(ch->comm2, COMM2_TARGET) ? "`7[`PTARGET`7]`` " : "",
		ch->name);

	strcat(buf, tmp);

    if (strlen(buf) + strlen(ch->pcdata->title) < 512)
        strcat(buf, ch->pcdata->title);

	strcat(buf, "\n\r");
	return buf;
}

/***************************************************************************
*	do_whois
***************************************************************************/
void do_whois(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	BUFFER *output;
	char arg[MIL];
	bool found = FALSE;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("You must provide a name.\n\r", ch);
		return;
	}

	send_to_char("`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo ``[`5B`Pad `2T`@rip``] `Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o``\n\r", ch);
	output = new_buf();

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = CH(d);

		if (!can_see(ch, wch))
			continue;

		if (is_whois_target(wch, arg)) {
			found = TRUE;

			add_buf(output, who_string(wch));
		}
	}

	if (!found) {
		send_to_char("No one of that name is playing.\n\r", ch);
		return;
	}

	page_to_char(buf_string(output), ch);
	free_buf(output);
}


/***************************************************************************
*	do_who
***************************************************************************/
void do_who(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	BUFFER *output;
	char buf[2 * MSL];
	int iClass;
	int iRace;
	int iLevelLower;
	int iLevelUpper;
	int nNumber;
	int nMatch;
	bool restricted_classes[MAX_CLASS];
	bool restricted_races[MAX_PC_RACE];
	bool class_restrict = FALSE;
	bool race_restrict = FALSE;
	bool immortal_restrict = FALSE;


	output = new_buf();
	iLevelLower = 0;
	iLevelUpper = MAX_LEVEL;
	for (iClass = 0; iClass < MAX_CLASS; iClass++)
		restricted_classes[iClass] = FALSE;

	for (iRace = 0; iRace < MAX_PC_RACE; iRace++)
		restricted_races[iRace] = FALSE;

	/* parse the arguments */
	nNumber = 0;
	for (;; ) {
		char arg[MSL];

		argument = one_argument(argument, arg);
		if (arg[0] == '\0')
			break;

		if (is_number(arg)) {
			switch (++nNumber) {
			case 1:
				iLevelLower = atoi(arg);
				break;
			case 2:
				iLevelUpper = atoi(arg);
				break;
			default:
				send_to_char("Only two level numbers allowed.\n\r", ch);
				return;
			}
		} else {
			if (arg[0] == 'i')
				immortal_restrict = TRUE;
			else
				iClass = class_lookup(arg);
			if (iClass == -1) {
				iRace = race_lookup(arg);

				if (iRace == 0 || iRace >= MAX_PC_RACE) {
					send_to_char("That's not a valid race or class.\n\r", ch);
					return;
				} else {
					race_restrict = TRUE;
					restricted_races[iRace] = TRUE;
				}
			} else {
				class_restrict = TRUE;
				restricted_classes[iClass] = TRUE;
			}
		}
	}
	/* show the matching characters */
	nMatch = 0;
	buf[0] = '\0';

	send_to_char("`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo`1O`!O`4o`Oo ``[`5B`Pad `2T`@rip``] `Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o`!O`1O`Oo`4o``\n\r", ch);

	sprintf(buf, "`2I`@m`7m`8ort`7a`@l`2s`7:``\n\r");
	add_buf(output, buf);

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		/*
		 * Check for match against restrictions.
		 * Don't use trust as that exposes trusted mortals.
		 */

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch, wch))
			continue;


		if (wch->level < LEVEL_IMMORTAL)
			continue;

		if (wch->level < iLevelLower
		    || wch->level > iLevelUpper
		    || (immortal_restrict && wch->level < LEVEL_IMMORTAL)
		    || (class_restrict && !restricted_classes[wch->class])
		    || (race_restrict && !restricted_races[wch->race]))
			continue;
		nMatch++;
		add_buf(output, who_string(wch));
	}

	sprintf(buf, "\n\r`5M`2o`Pr`@t`Pa`2l`5s`7:``\n\r");
	add_buf(output, buf);

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		/*
		 * Check for match against restrictions.
		 * Don't use trust as that exposes trusted mortals.
		 */

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch, wch))
			continue;


		if (wch->level >= LEVEL_IMMORTAL)
			continue;

		if (wch->level < iLevelLower
		    || wch->level > iLevelUpper
		    || (immortal_restrict && wch->level < LEVEL_IMMORTAL)
		    || (class_restrict && !restricted_classes[wch->class])
		    || (race_restrict && !restricted_races[wch->race]))
			continue;
		nMatch++;
		add_buf(output, who_string(wch));
	}

	sprintf(buf, "\n\rTotal Players found```8: ``%d\n\r", nMatch);
	add_buf(output, buf);
	page_to_char(buf_string(output), ch);
	free_buf(output);
	return;
}



/***************************************************************************
*	do_ewho
*
*	extended who
***************************************************************************/
void do_ewho(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	BUFFER *output;
	char buf[MSL];
	int iClass;
	int iRace;
	int iLevelLower;
	int iLevelUpper;
	int nNumber;
	int nMatch;
	bool restricted_classes[MAX_CLASS];
	bool restricted_races[MAX_PC_RACE];
	bool class_restrict = FALSE;
	bool race_restrict = FALSE;
	bool immortal_restrict = FALSE;

	iLevelLower = 0;
	iLevelUpper = MAX_LEVEL;
	for (iClass = 0; iClass < MAX_CLASS; iClass++)
		restricted_classes[iClass] = FALSE;

	for (iRace = 0; iRace < MAX_PC_RACE; iRace++)
		restricted_races[iRace] = FALSE;

	/* parse the arguments */
	nNumber = 0;
	for (;; ) {
		char arg[MSL];

		argument = one_argument(argument, arg);
		if (arg[0] == '\0')
			break;

		if (is_number(arg)) {
			switch (++nNumber) {
			case 1:
				iLevelLower = atoi(arg);
				break;
			case 2:
				iLevelUpper = atoi(arg);
				break;
			default:
				send_to_char("Only two level numbers allowed.\n\r", ch);
				return;
			}
		} else {
			/* look for classes to turn on */
			if (arg[0] == 'i') {
				immortal_restrict = TRUE;
			} else if ((iClass = class_lookup(arg)) > -1 && iClass < MAX_CLASS) {
				class_restrict = TRUE;
				restricted_classes[iClass] = TRUE;
			} else if ((iRace = race_lookup(arg)) != 0 && iRace <= MAX_PC_RACE) {
				race_restrict = TRUE;
				restricted_races[iRace] = TRUE;
			} else {
				send_to_char("That's not a valid race or class.\n\r", ch);
				return;
			}
		}
	}

	send_to_char("+-----------------------------------------------------------------------------+\n\r", ch);
	send_to_char(" Name            | Lvl | Pkills | Pdeaths | Mobkills | Mobdeaths | Guild\n\r", ch);
	send_to_char("+-----------------------------------------------------------------------------+\n\r", ch);

	nMatch = 0;
	buf[0] = '\0';
	output = new_buf();
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		/*
		 * Check for match against restrictions.
		 * Don't use trust as that exposes trusted mortals.
		 */
		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch, wch))
			continue;

		if (wch->level < iLevelLower
		    || wch->level > iLevelUpper
		    || (immortal_restrict && wch->level < LEVEL_IMMORTAL)
		    || (class_restrict && !restricted_classes[wch->class])
		    || (race_restrict && !restricted_races[wch->race]))
			continue;

		nMatch++;

		sprintf(buf, " %s%-15d   %-3ld   %-6ld   %-7ld   %-8ld``\n\r",
			wch->name,
			wch->level,
			wch->pcdata->pkills,
			wch->pcdata->pdeaths,
			wch->pcdata->mobkills,
			wch->pcdata->mobdeaths);
		add_buf(output, buf);
	}

	sprintf(buf, "\n\rPlayers found: %d\n\r", nMatch);
	add_buf(output, buf);
	page_to_char(buf_string(output), ch);
	free_buf(output);
	return;
}



/***************************************************************************
*	do_where
*
*	check to see what characters are in the area
***************************************************************************/
void do_where(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	char arg[MIL];
	bool found;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Players in your area```8:``\n\r", ch);
		found = FALSE;
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected == CON_PLAYING
			    && (victim = d->character) != NULL
			    && !IS_NPC(victim)
			    && victim->in_room != NULL
			    && (is_room_owner(ch, victim->in_room)
				|| !room_is_private(victim->in_room))
			    && victim->in_room->area == ch->in_room->area
			    && can_see(ch, victim)) {
				found = TRUE;
				printf_to_char(ch, "%s%-28s %s``\n\r",
					       IS_SET(victim->act, PLR_IT) ? "`&" : "",
					       victim->name, victim->in_room->name);
			}
		}

		if (!found)
			send_to_char("None.\n\r", ch);
	} else {
		found = FALSE;
		for (victim = char_list; victim != NULL; victim = victim->next) {
			if (victim->in_room != NULL
			    && victim->in_room->area == ch->in_room->area
			    && can_see(ch, victim)
			    && is_name(arg, victim->name)) {
				found = TRUE;
				printf_to_char(ch, "%s%-28s %s``\n\r",
					       IS_SET(victim->act, PLR_IT) ? "`&" : "",
					       PERS(victim, ch), victim->in_room->name);
				break;
			}
		}
		if (!found)
			act("You didn't find any $T.", ch, NULL, arg, TO_CHAR);
	}

	return;
}
