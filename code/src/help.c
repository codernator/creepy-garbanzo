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

/***************************************************************************
*	includes
***************************************************************************/
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"

void do_help(CHAR_DATA *ch, char *argument)
{
	HELP_DATA *help;
	char arg[MIL];
	char *txt;
	int index = 0;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);
	if (!str_prefix(arg, "find")
	    || !str_prefix(arg, "search")) {
		BUFFER *buf;
		bool search;

		search = (str_prefix(arg, "search") == 0);
		argument = one_argument(argument, arg);
		buf = new_buf();

		printf_buf(buf, "Helps matching the query: %s\n\r", argument);
		for (help = help_first; help != NULL; help = help->next) {
			if (help->level <= get_trust(ch)) {
				txt = uncolor_str(help->text);
				if (!str_infix(argument, help->keyword)
				    || (search && !str_infix(argument, txt))) {
					index++;
					printf_buf(buf, "[%.3d]  %s\n\r", index, help->keyword);
				}
				free_string(txt);
			}
		}

		if (index == 0)
			add_buf(buf, "     no helps matching the selected criteria.\n\r\n\r");

		page_to_char(buf_string(buf), ch);
		free_buf(buf);
	} else if (!str_prefix(arg, "class")) {
		BUFFER *buf;
		char tmpbuf[MSL];
		int class_idx;

		buf = new_buf();

		printf_buf(buf, " (Classes are alignment-based)\n\r");
		add_buf(buf, " Class    (Alignment)          Description\n\r");
		add_buf(buf, "------------------------------------------------------------------------\n\r");
		for (class_idx = 0; class_idx < MAX_CLASS; class_idx++) {
			if (class_table[class_idx].canCreate) {
				sprintf(tmpbuf, " %-9s", class_table[class_idx].name);
				add_buf(buf, tmpbuf);
				if (class_table[class_idx].dAlign < 0)
					sprintf(tmpbuf, "(E) ");
				else if (class_table[class_idx].dAlign == 0)
					sprintf(tmpbuf, "(N) ");
				else
					sprintf(tmpbuf, "(G) ");
				add_buf(buf, tmpbuf);
				sprintf(tmpbuf, "%s\n\r", class_table[class_idx].shortDesc);
				add_buf(buf, tmpbuf);
			}
		}
		add_buf(buf, "------------------------------------------------------------------------\n\r");
		add_buf(buf, " (Type '`^HELP <CLASS>``' for help on a specific class)\n\r");
		page_to_char(buf_string(buf), ch);
		free_buf(buf);
	} else {
		for (help = help_first; help != NULL; help = help->next) {
			if (help->level <= get_trust(ch)) {
				if (is_name(argument, help->keyword)) {
					if (help->text[0] == '!') {
						send_to_char(help->text + 1, ch);
					} else {
						//if (help->level >= 0 && str_cmp(help->keyword, "imotd")) {
						//	page_to_char(help->keyword, ch);
						//	page_to_char("\n\r", ch);
						//}

						if (help->text[0] == '.')
							page_to_char(help->text + 1, ch);
						else
							page_to_char(help->text, ch);
					}
					return;
				}
			}
		}

		send_to_char("No help on that word.\n\r", ch);
	}
}
