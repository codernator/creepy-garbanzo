#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "lookup.h"
#include "recycle.h"


extern void string_append(CHAR_DATA * ch, char **string);
extern HELP_AREA *had_list;


/***************************************************************************
*	local defines
***************************************************************************/

const struct olc_cmd_type hedit_table[] =
{
/*	{	command		function		}, */

	{ "keyword",  hedit_keyword },
	{ "text",     hedit_text    },
	{ "new",      hedit_new	    },
	{ "level",    hedit_level   },
	{ "commands", show_commands },
	{ "delete",   hedit_delete  },
	{ "list",     hedit_list    },
	{ "show",     hedit_show    },
	{ "area",     hedit_area    },
	{ "?",	      show_help	    },

	{ NULL,	      0		    }
};



/***************************************************************************
*	get_help_area
*
*	get the area for the help file
***************************************************************************/
static HELP_AREA *get_help_area(HELP_DATA *help)
{
	HELP_AREA *temp;
	HELP_DATA *thelp;

	for (temp = had_list; temp; temp = temp->next) {
		for (thelp = temp->first; thelp; thelp = thelp->next_area)
			if (thelp == help)
				return temp;
	}

	return NULL;
}



/***************************************************************************
*	hedit
*
*	command interpreter for the help editor
***************************************************************************/
void hedit(CHAR_DATA *ch, char *argument)
{
	HELP_DATA *help;
	HELP_AREA *had;
	char arg[MIL];
	char command[MIL];
	int cmd;

	smash_tilde(argument);
	strcpy(arg, argument);
	argument = one_argument(argument, command);

	EDIT_HELP(ch, help);
	had = get_help_area(help);
	if (had == NULL) {
		log_bug("hedit : had for help %s NULL", help->keyword);
		edit_done(ch);
		return;
	}

	if (ch->pcdata->security < 4) {
		send_to_char("HEdit: Insuficient security to edit helps.\n\r", ch);
		edit_done(ch);
		return;
	}

	if (command[0] == '\0') {
		hedit_show(ch, argument);
		return;
	}


	if (!str_cmp(command, "done")) {
		edit_done(ch);
		return;
	}

	for (cmd = 0; hedit_table[cmd].name != NULL; cmd++) {
		if (!str_prefix(command, hedit_table[cmd].name)) {
			if ((*hedit_table[cmd].olc_fn)(ch, argument))
				had->changed = true;
			return;
		}
	}

	interpret(ch, arg);
	return;
}


/***************************************************************************
*	do_hedit
*
*	entry point for the help editor
***************************************************************************/
void do_hedit(CHAR_DATA *ch, char *argument)
{
	HELP_DATA *help;
	char cmd[MIL];

	if (IS_NPC(ch))
		return;

	one_argument(argument, cmd);
	if (!str_cmp(cmd, "new")) {
		argument = one_argument(argument, cmd);
		hedit_new(ch, argument);
		return;
	}

	if ((help = help_lookup(argument)) == NULL) {
		send_to_char("HEdit : Help does not exist.\n\r", ch);
		return;
	}

	ch->desc->ed_data = (void *)help;
	ch->desc->editor = ED_HELP;

	return;
}

/***************************************************************************
*	hedit_show
*
*	show the properties for the help
***************************************************************************/
EDIT(hedit_show){
	HELP_DATA *help;
	HELP_AREA *had;

	EDIT_HELP(ch, help);

	had = get_help_area(help);
	printf_to_char(ch, "`&Keyword``:  [%s]\n\r"
		       "`&Level``:    [%d]\n\r"
		       "`&Area``:     [%s]\n\r",
		       help->keyword,
		       help->level,
		       (had != NULL && had->area != NULL) ? had->area->file_name : "none");
	printf_to_char(ch, "`&Text``:\n\r"
		       "%s\n\r`1-`!END`1-``\n\r", help->text);
	return false;
}



/***************************************************************************
*	hedit_level
*
*	edit the level of the help
***************************************************************************/
EDIT(hedit_level){
	HELP_DATA *help;
	int lev;

	EDIT_HELP(ch, help);
	if (IS_NULLSTR(argument) || !is_number(argument)) {
		send_to_char("Syntax : level [-1..MAX_LEVEL]\n\r", ch);
		return false;
	}

	lev = parse_int(argument);
	if (lev < -1 || lev > MAX_LEVEL) {
		printf_to_char(ch, "HEdit : levels between -1 and %d only.\n\r", MAX_LEVEL);
		return false;
	}

	help->level = lev;
	send_to_char("Ok.\n\r", ch);
	return true;
}


/***************************************************************************
*	hedit_keyword
*
*	edit the keyword of the help
***************************************************************************/
EDIT(hedit_keyword){
	HELP_DATA *help;

	EDIT_HELP(ch, help);
	if (IS_NULLSTR(argument)) {
		send_to_char("Syntax : keyword [keywords]\n\r", ch);
		return false;
	}

	free_string(help->keyword);
	help->keyword = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return true;
}



/***************************************************************************
*	hedit_new
*
*	create a new help
***************************************************************************/
EDIT(hedit_new){
	extern HELP_DATA *help_last;
	HELP_AREA *had;
	HELP_DATA *help;
	char name[MIL];
	char area[MIL];

	if (is_help(argument)) {
		send_to_char("Syntax   : new [name]\n\r", ch);
		send_to_char("           new [area] [name]\n\r", ch);
		return false;
	}

	argument = one_argument(argument, name);
	area[0] = '\0';
	if (argument[0] != '\0') {
		strcpy(area, name);
		argument = one_argument(argument, name);
	}


	if ((had = had_lookup(area)) == NULL)
		had = had_lookup(HELP_FILE);

	if (help_lookup(name)) {
		send_to_char("HEdit : help already exists.\n\r", ch);
		return false;
	}

	if (!had) {
		had = new_had();
		had->filename = str_dup(ch->in_room->area->file_name);
		had->area = ch->in_room->area;
		had->first = NULL;
		had->last = NULL;
		had->changed = true;
		had->next = had_list;
		had_list = had;
		ch->in_room->area->helps = had;
		SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
	}

	help = new_help();
	help->level = 0;
	help->keyword = str_dup(name);
	help->text = str_dup("");
	if (help_last)
		help_last->next = help;

	if (help_first == NULL)
		help_first = help;

	help_last = help;
	help->next = NULL;
	if (had->first == NULL)
		had->first = help;

	if (had->last == NULL) {
		had->last = help;
	} else {
		had->last->next_area = help;
		had->last = help;
	}

	help->next_area = NULL;
	ch->desc->ed_data = (HELP_DATA *)help;
	ch->desc->editor = ED_HELP;

	send_to_char("Ok.\n\r", ch);
	return false;
}


/***************************************************************************
*	hedit_text
*
*	edit the text of the help
***************************************************************************/
EDIT(hedit_text){
	HELP_DATA *help;

	EDIT_HELP(ch, help);

	string_append(ch, &help->text);
	return true;
}



/***************************************************************************
*	hedit_area
*
*	edit the text of the help
***************************************************************************/
EDIT(hedit_area){
	HELP_DATA *help;
	HELP_AREA *had_old;
	HELP_AREA *had_new;
	HELP_DATA *help_idx;
	char *area;

	EDIT_HELP(ch, help);

	if (is_help(argument)) {
		send_to_char("Syntax : area <area filename|none>\n\r", ch);
		return false;
	}


	had_old = get_help_area(help);
	if (!str_prefix(argument, "none"))
		area = HELP_FILE;
	else
		area = argument;

	if ((had_new = had_lookup(area)) != NULL
	    && had_new != had_old) {
		if (had_old != NULL) {
			if (help == had_old->first) {
				had_old->first = help->next_area;
			} else {
				HELP_DATA *help_last = NULL;

				for (help_idx = had_old->first;
				     help_idx != NULL;
				     help_idx = help_idx->next_area) {
					if (help_idx == help)
						break;

					help_last = help_idx;
				}

				if (help_last != NULL && help_idx != NULL)
					help_last->next_area = help->next_area;
			}
			had_old->changed = true;
		}


		/* add it to the new area data */
		if (had_new->first == NULL)
			had_new->first = help;

		if (had_new->last == NULL) {
			had_new->last = help;
		} else {
			had_new->last->next_area = help;
			had_new->last = help;
		}
		help->next_area = NULL;
		had_new->changed = true;
	}
	return true;
}

/***************************************************************************
*	hedit_delete
*
*	delete a help file
***************************************************************************/
EDIT(hedit_delete){
	HELP_DATA *help;
	HELP_DATA *temp;
	HELP_AREA *had;
	DESCRIPTOR_DATA *d;
	bool found = false;

	EDIT_HELP(ch, help);

	for (d = globalSystemState.descriptor_head; d; d = d->next)
		if (d->editor == ED_HELP && help == (HELP_DATA *)d->ed_data)
			edit_done(d->character);


	if (help_first == help) {
		help_first = help_first->next;
	} else {
		for (temp = help_first; temp; temp = temp->next)
			if (temp->next == help)
				break;

		if (!temp) {
			log_bug("HEdit delete : help %s not found in help_first", help->keyword);
			return false;
		}

		temp->next = help->next;
	}

	for (had = had_list; had; had = had->next) {
		if (help == had->first) {
			found = true;
			had->first = had->first->next_area;
		} else {
			for (temp = had->first; temp; temp = temp->next_area)
				if (temp->next_area == help)
					break;

			if (temp) {
				temp->next_area = help->next_area;
				found = true;
				break;
			}
		}
	}

	if (!found) {
		log_bug("hedit_delete : help %s not found in had_list", help->keyword);
		return false;
	}

	free_help(help);

	send_to_char("Ok.\n\r", ch);
	return true;
}



/***************************************************************************
*	hedit_list
*
*	show the list of helps
***************************************************************************/
EDIT(hedit_list){
	HELP_DATA *help;
	BUFFER *buffer;
	char buf[MIL];
	int cnt = 0;

	EDIT_HELP(ch, help);
	if (!str_cmp(argument, "all")) {
		buffer = new_buf();

		for (help = help_first; help; help = help->next) {
			sprintf(buf, "%3d. %-14.14s%s", cnt, help->keyword, cnt % 4 == 3 ? "\n\r" : " ");
			add_buf(buffer, buf);
			cnt++;
		}

		if (cnt % 4)
			add_buf(buffer, "\n\r");

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return false;
	}

	if (!str_cmp(argument, "area")) {
		if (ch->in_room->area->helps == NULL) {
			send_to_char("There are no helps in this area.\n\r", ch);
			return false;
		}

		buffer = new_buf();

		for (help = ch->in_room->area->helps->first; help; help = help->next_area) {
			sprintf(buf, "%3d. %-14.14s%s", cnt, help->keyword, cnt % 4 == 3 ? "\n\r" : " ");
			add_buf(buffer, buf);
			cnt++;
		}

		if (cnt % 4)
			add_buf(buffer, "\n\r");

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return false;
	}

	send_to_char("Syntax : list all\n\r", ch);
	send_to_char("         list area\n\r", ch);
	return false;
}
