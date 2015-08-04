/***************************************************************************
*  File: string.c                                                         *
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "sysinternals.h"

extern void do_help(CHAR_DATA *ch, char *argument);

/***************************************************************************
*	local defines
***************************************************************************/
static char *string_line_delete(char *, int);
static char *string_line_insert(char *, char *, int);
static char *number_lines(char *);
char *repeater(char *s, int i);
char *format_string(char *oldstring);

#define MAX_LINE_LENGTH         72


byte parse_byte(char *string)
{
	int raw = atoi(string);

	return (byte)UMAX(UMIN(raw, 255), 0);
}

byte parse_byte2(char *string, byte min, byte max)
{
	int raw = atoi(string);

	return (byte)UMAX(UMIN(raw, (int)max), (int)min);
}

int parse_int(char *string)
{
	return atoi(string);
}

long parse_long(char *string)
{
	return atol(string);
}

unsigned int parse_unsigned_int(char *string)
{
	return (unsigned int)UMAX(0, atol(string));
}

unsigned long parse_unsigned_long(char *string)
{
	return UMAX(0, (unsigned long)atoll(string));
}




/***************************************************************************
*	string_edit
*
*	clears a string and puts the player into editing mode
***************************************************************************/
void string_edit(CHAR_DATA *ch, char **string)
{
	send_to_char("`3-`#========`3- `@Entering `2EDIT `@Mode `3-`#=========`3-``\n\r", ch);
	send_to_char("    Type `2.`@h`` on a new line for help\n\r", ch);
	send_to_char("   Terminate with a `@@`` on a blank line.\n\r", ch);
	send_to_char("`3-`#=========================================`3-``\n\r", ch);

	if (*string == NULL)
		*string = str_dup("");
	else
		**string = '\0';

	ch->desc->ed_string = string;
	return;
}



/***************************************************************************
*	string_append
*
*	puts a player into append mode for a given string
***************************************************************************/
void string_append(CHAR_DATA *ch, char **string)
{
	send_to_char("`3-`#========`3- `@Entering `2APPEND `@Mode `3-`#=========`3-``\n\r", ch);
	send_to_char("    Type `2.`@h`` on a new line for help\n\r", ch);
	send_to_char("   Terminate with a `@@`` on a blank line.\n\r", ch);
	send_to_char("`3-`#=========================================`3-``\n\r", ch);

	if (*string == NULL)
		*string = str_dup("");

	send_to_char(number_lines(*string), ch);
	ch->desc->ed_string = string;
	return;
}


/***************************************************************************
*	string_replace
*
*	replace one string for another
***************************************************************************/
char *string_replace(char *orig, char *find, char *replace)
{
	char buf[MSL];
	int len;

	buf[0] = '\0';
	strcpy(buf, orig);
	if (strstr(orig, find) != NULL) {
		len = (int)strlen(orig) - (int)strlen(strstr(orig, find));
		buf[len] = '\0';

		strcat(buf, replace);
		strcat(buf, &orig[len + strlen(find)]);
		free_string(orig);
	}

	return str_dup(buf);
}



/***************************************************************************
*	string_add
*
*	interpreter for string editing
*	i kind of think it is a little kludgey, but i really dont
*	feel like re-writing it from scratch -- i will just clean it
*	up a bit, fix a couple of things that i think are really
*	tanked and let it be
***************************************************************************/
void string_add(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	bool done = FALSE;

	/* this is entirely on drugs - the original string editor
	 * says to do '@' or '~' on a line by itself then we
	 * conveniently call smash_tilde making the ~ not work
	 * kind of humorous - '~' of course no longer allows us
	 * to exit the editor */

	smash_tilde(argument);
	if (*argument == '.') {
		char cmd[MIL];

		argument = one_argument(argument, cmd);
		if (!str_cmp(cmd, ".c")) {
			send_to_char("String cleared.\n\r", ch);
			free_string(*ch->desc->ed_string);
			*ch->desc->ed_string = str_dup("");
			return;
		}

		if (!str_cmp(cmd, ".s")) {
			send_to_char("`#String so far``:\n\r", ch);
			send_to_char(number_lines(*ch->desc->ed_string), ch);
			return;
		}

		if (!str_cmp(cmd, ".r")) {
			char orig[MSL];
			char repl[MSL];

			argument = first_arg(argument, orig, FALSE);
			argument = first_arg(argument, repl, FALSE);

			if (orig[0] == '\0') {
				send_to_char("`#Usage``:  .r 'old string' 'new string'\n\r", ch);
				return;
			}

			*ch->desc->ed_string = string_replace(*ch->desc->ed_string, orig, repl);
			printf_to_char(ch, "'%s' replaced with '%s'.\n\r", orig, repl);
			return;
		}

		if (!str_cmp(cmd, ".f")) {
			*ch->desc->ed_string = format_string(*ch->desc->ed_string);
			send_to_char("String formatted.\n\r", ch);
			return;
		}

		if (!str_cmp(cmd, ".ld")) {
			char line[MSL];

			argument = one_argument(argument, line);
			if (!is_number(line)) {
				send_to_char("`#Usage``: .ld [line number]\n\r", ch);
				return;
			}

			*ch->desc->ed_string = string_line_delete(*ch->desc->ed_string, atoi(line));
			send_to_char("Line deleted.\n\r", ch);
			return;
		}

		if (!str_cmp(cmd, ".li")) {
			char line[MSL];

			argument = one_argument(argument, line);
			if (!is_number(line)) {
				send_to_char("`#Usage``: .li [line number] [string]\n\r", ch);
				return;
			}
			*ch->desc->ed_string = string_line_insert(*ch->desc->ed_string, argument, atoi(line));
			send_to_char("Line inserted.\n\r", ch);
			return;
		}

		if (!str_cmp(cmd, ".lr")) {
			char line[MSL];

			argument = one_argument(argument, line);
			if (!is_number(line)) {
				send_to_char("`#Usage``: .lr [line number] [string]\n\r", ch);
				return;
			}

			*ch->desc->ed_string = string_line_delete(*ch->desc->ed_string, atoi(line));
			*ch->desc->ed_string = string_line_insert(*ch->desc->ed_string, argument, atoi(line));

			send_to_char("Line replaced.\n\r", ch);
			return;
		}

		if (!str_cmp(cmd, ".h")) {
			do_help(ch, "STRING_EDITOR");
			return;
		}

		if (!str_cmp(cmd, ".x"))
			/* kind of hackish way to use '.x' as an exit command */
			done = TRUE;

		if (!done) {
			send_to_char("SEdit:  Invalid dot command.\n\r", ch);
			return;
		}
	}

	if (*argument == '@' || done) {
		/* update every mobile in the game with the mob progam
		 * with the new string - kind of a pain in the ass if
		 * you ask me */
		if (ch->desc->editor == ED_MPCODE) {
			MOB_INDEX_DATA *mob;
			int hash;
			MPROG_LIST *mpl;
			MPROG_CODE *mpc;

			EDIT_MPCODE(ch, mpc);
			if (mpc != NULL) {
				for (hash = 0; hash < MAX_KEY_HASH; hash++) {
					for (mob = mob_index_hash[hash]; mob; mob = mob->next) {
						for (mpl = mob->mprogs; mpl; mpl = mpl->next) {
							if (mpl->vnum == mpc->vnum) {
								sprintf(buf, "Updated mob %ld.\n\r", mob->vnum);
								send_to_char(buf, ch);
								mpl->code = mpc->code;
							}
						}
					}
				}
			}
		}
		ch->desc->ed_string = NULL;
		return;
	}


	strcpy(buf, *ch->desc->ed_string);
	if (strlen(buf) + strlen(argument) >= (MSL - 4)) {
		send_to_char("String too long, last line skipped.\n\r", ch);
		ch->desc->ed_string = NULL;
		return;
	}

	strcat(buf, argument);
	strcat(buf, "\n\r");

	free_string(*ch->desc->ed_string);
	*ch->desc->ed_string = str_dup(buf);
	return;
}



/***************************************************************************
*	format_string
*
*	format a string to conform to width standards - replacement
*	for the one that comes stock with OLC which wasnt working for
*	us due to the fact that it did not ignore color codes in the
*	width calculation.  The old one did a lot of formatting surrounding
*	punctuation that I just dont care to do - people using it should
*	be okay without the punctuation
***************************************************************************/
char *format_string(char *orig)
{
	BUFFER *buf;
	char *ptr;
	char *tmp;
	int tmp_ln;
	char word[MSL];
	int line_len;
	int word_len;
	int word_idx;

	word[0] = '\0';
	buf = new_buf();
	ptr = orig;
	line_len = 0;
	word_len = 0;
	word_idx = 0;
	while (*ptr != '\0') {
		switch (*ptr) {
		case '\n':
			tmp = ptr;
			tmp_ln = 0;
			while (*tmp == '\n' || *tmp == '\r' || *tmp == ' ') {
				tmp++;
				tmp_ln++;
			}
			/* if we have a double line break then add it to the word */
			if (tmp_ln > 2) {
				while (*ptr == '\n' || *ptr == '\r' || *ptr == ' ') {
					if (*ptr != ' ') {
						word[word_idx++] = *ptr;
						word_len++;
						if (word_len >= MSL)
							break;
					}
					ptr++;
				}

				if (line_len > 0)
					add_buf(buf, " ");

				line_len = 0;
				word_len = 0;
				ptr--;
			}
		/* fall through to end of the word - the line feeds go along */
		case ' ':
			if (word_idx > 0) {
				word[word_idx] = '\0';

				if (line_len + word_len > MAX_LINE_LENGTH) {
					add_buf(buf, "\n\r");
					line_len = 0;
				}

				if (line_len > 0) {
					add_buf(buf, " ");
					word_len++;
				}

				line_len += word_len;
				add_buf(buf, word);
				word_idx = 0;
				word_len = 0;
			}
			break;
		case '.':
		case '!':
		case '?':
			word[word_idx++] = *ptr;
			word_len++;
			if (*ptr != *(ptr + 1)
			    && *ptr != *(ptr - 1)) {
				int idx;

				for (idx = 0; idx < 1; idx++) {
					/* put some spaces in */
					if (line_len + 1 < MAX_LINE_LENGTH) {
						word[word_idx++] = ' ';
						word_len++;
					}
				}
			}

			break;
		case '\r':
			/* completely ignore */
			break;
		case '`':
			word[word_idx++] = *ptr++;
			word[word_idx++] = *ptr;
			break;
		default:
			word[word_idx++] = *ptr;
			word_len++;
			break;
		}

		ptr++;
	}


	if (line_len > 0)
		add_buf(buf, "\n\r");
	ptr = str_dup(buf_string(buf));

	free_buf(buf);
	free_string(orig);

	return ptr;
}

/***************************************************************************
*	first_arg
*
*	picks off the first word from a string and return the rest
*	understands quotes, parenthesis, and percentages
***************************************************************************/
char *first_arg(char *argument, char *arg_first, bool fCase)
{
	char cEnd;

	while (*argument == ' ')
		argument++;

	cEnd = ' ';
	if (*argument == '\''
	    || *argument == '"'
	    || *argument == '%') {
		cEnd = *argument++;
	} else if (*argument == '(') {
		cEnd = ')';
		argument++;
	}

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}

		*arg_first = (fCase) ? LOWER(*argument) : *argument;
		arg_first++;
		argument++;
	}

	*arg_first = '\0';
	while (*argument == ' ')
		argument++;

	return argument;
}




/***************************************************************************
*	string_unpad
*
*	trims leading and trailing spaces
***************************************************************************/
char *string_unpad(char *argument)
{
	char *iter;
	char buf[MSL];

	iter = argument;

	while (*iter == ' ')
		iter++;

	strcpy(buf, iter);
	iter = buf;

	if (*iter != '\0') {
		/* go to the end of the string */
		while (*iter != '\0')
			iter++;
		iter--;

		/* work backwards, removing spaces */
		while (*iter == ' ')
			*iter-- = '\0';
	}

	free_string(argument);
	return str_dup(buf);
}


/***************************************************************************
*	string_proper
*
*	capitalizes the first character of each word in a string
***************************************************************************/
char *string_proper(char *argument)
{
	char *iter;

	iter = argument;
	while (*iter != '\0') {
		if (*iter != ' ') {
			*iter = UPPER(*iter);
			while (*iter != ' ' && *iter != '\0')
				iter++;
		} else {
			iter++;
		}
	}

	return argument;
}



/***************************************************************************
*	string_line_delete
*
*	delete line n from the string
***************************************************************************/
static char *string_line_delete(char *string, int line)
{
	char *strtmp = string;
	char buf[MSL];
	int cnt = 1;
	int tmp = 0;

	buf[0] = '\0';
	for (; *strtmp != '\0'; strtmp++) {
		if (cnt != line)
			buf[tmp++] = *strtmp;

		if (*strtmp == '\n') {
			if (*(strtmp + 1) == '\r') {
				if (cnt != line)
					buf[tmp++] = *(++strtmp);
				else
					++strtmp;
			}
			cnt++;
		}
	}

	buf[tmp] = '\0';
	free_string(string);
	return str_dup(buf);
}



/***************************************************************************
*	string_line_insert
*
*	insert a line at line n
***************************************************************************/
static char *string_line_insert(char *string, char *newstr, int line)
{
	char *strtmp = string;
	char buf[MSL];
	int cnt = 1;
	int tmp = 0;
	bool done = FALSE;

	buf[0] = '\0';
	for (; *strtmp != '\0' || (!done && cnt == line); strtmp++) {
		if (cnt == line && !done) {
			strcat(buf, newstr);
			strcat(buf, "\n\r");

			tmp += strlen(newstr) + 2;
			cnt++;
			done = TRUE;
		}

		buf[tmp++] = *strtmp;
		if (done && *strtmp == '\0')
			break;

		if (*strtmp == '\n') {
			if (*(strtmp + 1) == '\r')
				buf[tmp++] = *(++strtmp);
			cnt++;
		}
		buf[tmp] = '\0';
	}

	free_string(string);
	return str_dup(buf);
}

/***************************************************************************
*	getlinefrombuf
*
*	get the first line of text
*	does not return the \n\r
***************************************************************************/
static char *getlinefrombuf(char *str, char *buf)
{
	int tmp = 0;

	/* unused var bool	found	= FALSE; */

	while (*str != '\0' && *str != '\n')
		buf[tmp++] = *(str++);

	if (*str == '\n') {
		if (*(++str) == '\r')
			str++;
	}

	buf[tmp] = '\0';
	return str;
}

/***************************************************************************
*	number_lines
*
*	put line numbers at the begining of each line of text
***************************************************************************/
static char *number_lines(char *string)
{
	BUFFER *buf;
	static char out[MSL];
	int cnt = 1;
	char tmp[MSL];
	char line[MSL];

	buf = new_buf();
	out[0] = '\0';
	while (*string != '\0') {
		string = getlinefrombuf(string, line);
		sprintf(tmp, "%2d. %s\n\r", cnt++, line);
		add_buf(buf, tmp);
	}

	strncpy(out, buf_string(buf), MSL);
	free_buf(buf);
	return out;
}

char *repeater(char *s, int i)
{
	char buf[MSL] = "";
	int iter;

	for (iter = 1; iter <= i; iter++)
		strcat(buf, s);

	return str_dup(buf);
}
