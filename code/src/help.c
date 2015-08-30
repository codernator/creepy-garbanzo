#include <stdio.h>
#include "merc.h"
#include "recycle.h"



void do_help(CHAR_DATA *ch, const char *argument)
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

	add_buf(buf, " Class          Description\n\r");
	add_buf(buf, "------------------------------------------------------------------------\n\r");
	for (class_idx = 0; class_idx < MAX_CLASS; class_idx++) {
	    if (class_table[class_idx].canCreate) {
		sprintf(tmpbuf, " %-9s", class_table[class_idx].name);
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
