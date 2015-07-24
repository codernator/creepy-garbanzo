/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
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
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/
/*  Nickname by Spaceboy    */

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

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

extern char *lower(char *s);

void add_to_nicknames(CHAR_DATA *ch, char *nickname, char *name)
{
	NICKNAME_DATA *temp = ch->nicknames;
	char buf[MAX_STRING_LENGTH];

	while (temp && (str_cmp(temp->nickname, nickname)))
		temp = temp->next;
	if (temp) {
		free(temp->name);
		temp->name = name;
		sprintf(buf, "Replacing nickname %s.\n\r", temp->nickname);
		send_to_char(buf, ch);
	} else {
		temp = malloc(sizeof(NICKNAME_DATA));
		temp->nickname = nickname;
		temp->name = name;
		temp->next = ch->nicknames;
		ch->nicknames = temp;
	}
}

void free_nicknames(CHAR_DATA *ch)
{
	NICKNAME_DATA *temp = ch->nicknames, *temp1;

	while (temp) {
		temp1 = temp;
		temp = temp->next;
		free(temp1->nickname);
		free(temp1->name);
		free(temp1);
	}
}

char *check_nickname(CHAR_DATA *ch, char *nickname)
{
	NICKNAME_DATA *temp;

	if (IS_NPC(ch))
		return NULL;

	temp = ch->nicknames;

	while (temp) {
		if (!strcmp(nickname, temp->nickname))
			return temp->name; /* return expanded of nickname */
		temp = temp->next;
	}

/* not a nickname */
	return NULL;
}

void do_nickname(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *tch;
	NICKNAME_DATA *temp1, *temp = ch->nicknames;

	if (IS_NPC(ch))
		return;

	if (IS_LINK_DEAD(ch))
		return;

	if (argument[0] == '\0') {
		send_to_char("   add a nickname: nickname <person> <nickname>\n\r", ch);
		send_to_char("remove a nickname: nickname <nickname>\n\r", ch);
		send_to_char("   list nicknames: nickname list\n\r", ch);
		return;
	}

	smash_tilde(argument);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!strcmp(arg1, "list")) {
		/* list nicknames */

		if (!temp) {
			send_to_char("You have no nicknames set.\n\r", ch);
		} else {
			while (temp) {
				sprintf(buf, "%s is nicknamed %s.\n\r", temp->name, temp->nickname);
				send_to_char(buf, ch);
				temp = temp->next;
			}
		}
		return;
	}


	if (arg2[0] == '\0' && check_nickname(ch, arg1)) {
		/* remove the nickname */

		sprintf(buf, "%s is no longer nicknamed %s.\n\r",
			check_nickname(ch, arg1), arg1);
		send_to_char(buf, ch);

		if (!str_cmp(temp->nickname, arg1)) {
			ch->nicknames = ch->nicknames->next;
			free(temp->nickname);
			free(temp->name);
			free(temp);
			return;
		}

		while (temp->next && str_cmp(temp->next->nickname, arg1))
			temp = temp->next;
		if (temp->next) {
			temp1 = temp->next;
			temp->next = temp1->next;
			free(temp1->name);
			free(temp1->nickname);
			free(temp1);
		}
		return;
	}

	if ((tch = get_char_world(ch, arg1)) == NULL) {
		send_to_char("I don't see that person here.\n\r", ch);
		return;
	}

	if (IS_NPC(tch)) {
		send_to_char("You can only nickname players.\n\r", ch);
		return;
	}

	if (arg2[0] == '\0') {
		act("What do you want $N's nickname to be?", ch, NULL, tch, TO_CHAR);
		return;
	}

	sprintf(buf, "Nicknaming %s to %s.\n\r", tch->name, arg2);
	send_to_char(buf, ch);

	add_to_nicknames(ch, strdup(arg2), strdup(tch->name));
}
