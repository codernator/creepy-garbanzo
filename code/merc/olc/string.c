#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "help.h"


/***************************************************************************
 *	local defines
 ***************************************************************************/
static char *string_line_delete(char *, int);
static char *string_line_insert(char *, const char *, int);
static char *number_lines(const char *);
char *repeater(const char *s, int i);
char *format_string(char *oldstring);

#define MAX_LINE_LENGTH         72


/***************************************************************************
 *	string_edit
 *
 *	clears a string and puts the player into editing mode
 ***************************************************************************/
void string_edit(struct char_data *ch, char **string)
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
}



/***************************************************************************
 *	string_append
 *
 *	puts a player into append mode for a given string
 ***************************************************************************/
void string_append(struct char_data *ch, char **string)
{
    send_to_char("`3-`#========`3- `@Entering `2APPEND `@Mode `3-`#=========`3-``\n\r", ch);
    send_to_char("    Type `2.`@h`` on a new line for help\n\r", ch);
    send_to_char("   Terminate with a `@@`` on a blank line.\n\r", ch);
    send_to_char("`3-`#=========================================`3-``\n\r", ch);

    if (*string == NULL)
        *string = str_dup("");

    printf_to_char(ch, "%s", number_lines(*string));
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
    char buf[MAX_STRING_LENGTH];
    int len;

    smash_tilde(replace);
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
void string_add(struct char_data *ch, const char *argument)
{
    char buf[MAX_STRING_LENGTH];
    bool done = false;

    /* this is entirely on drugs - the original string editor
     * says to do '@' or '~' on a line by itself then we
     * conveniently call smash_tilde making the ~ not work
     * kind of humorous - '~' of course no longer allows us
     * to exit the editor */

    if (*argument == '.') {
        char cmd[MAX_INPUT_LENGTH];

        argument = one_argument(argument, cmd);
        if (!str_cmp(cmd, ".c")) {
            send_to_char("String cleared.\n\r", ch);
            free_string(*ch->desc->ed_string);
            *ch->desc->ed_string = str_dup("");
            return;
        }

        if (!str_cmp(cmd, ".s")) {
            printf_to_char(ch, "`#String so far``:\n\r%s", number_lines(*ch->desc->ed_string));
            return;
        }

        if (!str_cmp(cmd, ".r")) {
            char orig[MAX_STRING_LENGTH];
            char repl[MAX_STRING_LENGTH];

            argument = first_arg(argument, orig, false);
            argument = first_arg(argument, repl, false);

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
            char line[MAX_STRING_LENGTH];

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
            char line[MAX_STRING_LENGTH];

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
            char line[MAX_STRING_LENGTH];

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
            show_help(ch->desc, "STRING_EDITOR", NULL);
            return;
        }

        if (!str_cmp(cmd, ".x"))
            /* kind of hackish way to use '.x' as an exit command */
            done = true;

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
            struct mob_index_data *mob;
            int hash;
            struct mprog_list *mpl;
            struct mprog_code *mpc;

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
    if (strlen(buf) + strlen(argument) >= (MAX_STRING_LENGTH - 4)) {
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
    struct buf_type *buf;
    char *ptr;
    char *tmp;
    int tmp_ln;
    char word[MAX_STRING_LENGTH];
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
                          if (word_len >= MAX_STRING_LENGTH)
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
const char *first_arg(const char *argument, char *arg_first, bool fCase)
{
    char cEnd;

    while (*argument == ' ')
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"' || *argument == '%') {
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
    char buf[MAX_STRING_LENGTH];

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
    char buf[MAX_STRING_LENGTH];
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
char *string_line_insert(char *string, const char *newstr, int line)
{
    const char *strtmp = string;
    char buf[MAX_STRING_LENGTH];
    int cnt = 1;
    int tmp = 0;
    bool done = false;

    buf[0] = '\0';
    for (; *strtmp != '\0' || (!done && cnt == line); strtmp++) {
        if (cnt == line && !done) {
            strcat(buf, newstr);
            strcat(buf, "\n\r");

            tmp += strlen(newstr) + 2;
            cnt++;
            done = true;
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
static const char *getlinefrombuf(const char *str, char *buf)
{
    int tmp = 0;

    /* unused var bool	found	= false; */

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
char *number_lines(const char *string)
{
    struct buf_type *buf;
    static char out[MAX_STRING_LENGTH];
    int cnt = 1;
    char tmp[MAX_STRING_LENGTH];
    char line[MAX_STRING_LENGTH];

    buf = new_buf();
    out[0] = '\0';
    while (*string != '\0') {
        string = getlinefrombuf(string, line);
        sprintf(tmp, "%2d. %s\n\r", cnt++, line);
        add_buf(buf, tmp);
    }

    strncpy(out, buf_string(buf), MAX_STRING_LENGTH);
    free_buf(buf);
    return out;
}

char *repeater(const char *s, int i)
{
    char buf[MAX_STRING_LENGTH] = "";
    int iter;

    for (iter = 1; iter <= i; iter++)
        strcat(buf, s);

    return str_dup(buf);
}
