#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "help.h"
#include "sysinternals.h"
#include "interp.h"


/***************************************************************************
 *	local defines
 ***************************************************************************/
/*@temp@*/static char *number_lines(/*@observer@*/const char *);
/*@temp@*/static const char *format_string(/*@observer@*/const char *);

#define MAX_LINE_LENGTH         72
#define EDIT_STRING(ch)   ((ch)->desc->olc_string_editor.text)
#define EDIT_SETTER(ch)   ((ch)->desc->olc_string_editor.setter)
#define EDIT_GETTER(ch)   ((ch)->desc->olc_string_editor.getter)
#define EDIT_OWNER(ch)    ((ch)->desc->olc_string_editor.owner)
#define COMMAND_TOKEN     ":"
#define COMMAND_CTOKEN     ':'

static const char color_token = '`';
static const char space_token = ' ';
static const char newline_token = '\n';
static const char return_token = 'r';
static const char tab_token = '\t';
static const char null_token = '\0';

static void sedit_clear(struct char_data *ch, const char *argument);
static void sedit_show(struct char_data *ch, const char *argument);
static void sedit_replaceone(struct char_data *ch, const char *argument);
static void sedit_formatstring(struct char_data *ch, const char *argument);
static void sedit_lineinsert(struct char_data *ch, const char *argument);
static void sedit_linedelete(struct char_data *ch, const char *argument);
static void sedit_lineappend(struct char_data *ch, const char *argument);
static void sedit_help(struct char_data *ch, const char *argument);
static void sedit_revert(struct char_data *ch, const char *argument);
static void sedit_commit(struct char_data *ch, const char *argument);
static void sedit_abandon(struct char_data *ch, const char *argument);
static const struct editor_cmd_type string_edit_table[] =
{
    { COMMAND_TOKEN"!c", sedit_clear,        COMMAND_TOKEN"!c               - clear text in progress.\n\r" },
    { COMMAND_TOKEN"",   sedit_show,         COMMAND_TOKEN"                 - show text in progress with line numbers.\n\r" },
    { COMMAND_TOKEN"%s", sedit_replaceone,   COMMAND_TOKEN"%s <find> <repl> - substitute <find> with <repl>.\n\r" },
    { COMMAND_TOKEN"!f", sedit_formatstring, COMMAND_TOKEN"!f               - format the string in progress.\n\r" },
    { COMMAND_TOKEN"li", sedit_lineinsert,   COMMAND_TOKEN"li <#> <line>    - insert <line> at line <#>.\n\r" },
    { COMMAND_TOKEN"ld", sedit_linedelete,   COMMAND_TOKEN"ld <#>           - delete line at <#>\n\r." },
    { COMMAND_TOKEN"la", sedit_lineappend,   COMMAND_TOKEN"la <line>        - append <line>... same as entering a line with no command.\n\r" },
    { COMMAND_TOKEN"!!revert", sedit_revert, COMMAND_TOKEN"!!revert         - undo all changes.\n\r" },
    { COMMAND_TOKEN"wq", sedit_commit,       COMMAND_TOKEN"wq               - commit the text and close the editor.\n\r" },
    { COMMAND_TOKEN"q!", sedit_abandon,      COMMAND_TOKEN"q!               - abandon the changes and close the editor.\n\r" },
    { COMMAND_TOKEN"?",  sedit_help,         COMMAND_TOKEN"?                - show this help.\n\r" },
    { "", NULL, "" }
};




void olc_start_string_editor(struct char_data *ch, void *owner, DESC_GETTER *getter, DESC_SETTER *setter)
{
    send_to_char("`3-`#========`3- `@Entering Editor `3-`#=========`3-``\n\r", ch);
    send_to_char("    Type `2"COMMAND_TOKEN"`@?`` on a new line for help\n\r", ch);
    send_to_char("   Terminate with a `"COMMAND_TOKEN"wq`` on a blank line.\n\r", ch);
    send_to_char("`3-`#=========================================`3-``\n\r", ch);
    (*getter)(owner, EDIT_STRING(ch), MAX_STRING_LENGTH);
    EDIT_OWNER(ch) = owner;
    EDIT_GETTER(ch) = getter;
    EDIT_SETTER(ch) = setter;

    ch->desc->interpmode = INTERP_MODE_STRING_EDIT;
    printf_to_char(ch, "%s", number_lines(EDIT_STRING(ch)));
    return;
}

void olc_editstring_interpreter(struct char_data *ch, const char *argument)
{
    if (*argument == COMMAND_CTOKEN) {
        char command[MAX_INPUT_LENGTH];
        int cmdindex;

        argument = one_argument(argument, command);
        for (cmdindex = 0; string_edit_table[cmdindex].name[0] != '\0'; cmdindex++) {
            if (command[0] == string_edit_table[cmdindex].name[0]
                    && !str_cmp(command, string_edit_table[cmdindex].name)) {
                assert(string_edit_table[cmdindex].do_fn != NULL);
                (*string_edit_table[cmdindex].do_fn)(ch, argument);
                return;
            }
        }

        send_to_char("SEdit:  Invalid dot command.\n\r", ch);
        return;
    }

    sedit_lineappend(ch, argument);
    return;
}


/******************
 * LOCALS
 */

void sedit_clear(struct char_data *ch, /*@unused@*/const char *argument)
{
    EDIT_STRING(ch)[0] = '\0';
    send_to_char("String cleared.\n\r", ch);
    return;
}

void sedit_revert(struct char_data *ch, /*@unused@*/const char *argument)
{
    DESC_GETTER *getter = EDIT_GETTER(ch);
    void *owner = EDIT_OWNER(ch);

    assert(getter != NULL);
    assert(owner != NULL);

    (*getter)(owner, EDIT_STRING(ch), MAX_STRING_LENGTH);
    send_to_char("All changes reverted.", ch);
    return;
}

void sedit_show(struct char_data *ch, /*@unused@*/const char *argument)
{
    printf_to_char(ch, "`#String so far``:\n\r%s", number_lines(EDIT_STRING(ch)));
    return;
}

void sedit_replaceone(struct char_data *ch, const char *argument)
{
    char orig[MAX_STRING_LENGTH];
    char *replaced;

    argument = one_argument(argument, orig);

    if (orig[0] == '\0') {
        send_to_char("`#Usage``:  .r 'old string' 'new string'\n\r", ch);
        return;
    }

    replaced = replace_one(EDIT_STRING(ch), orig, argument, MAX_STRING_LENGTH);
    (void)strncpy(EDIT_STRING(ch), replaced, MAX_STRING_LENGTH);
    free(replaced);
    send_to_char("String replace complete.\n\r", ch);
    return;
}

void sedit_formatstring(struct char_data *ch, /*@unused@*/const char *argument)
{
    (void)strncpy(EDIT_STRING(ch), format_string(EDIT_STRING(ch)), MAX_STRING_LENGTH);
    send_to_char("String formatted.\n\r", ch);
    return;
}

void sedit_lineappend(struct char_data *ch, const char *argument)
{
    char buf[MAX_STRING_LENGTH];
    size_t currlen;
    size_t arglen;

    currlen = strlen(EDIT_STRING(ch));
    arglen = currlen + strlen(argument);
    if (currlen + arglen >= MAX_STRING_LENGTH - 4) {
        send_to_char("String too long, last line skipped.\n\r", ch);
        return;
    }


    (void)strncpy(buf, EDIT_STRING(ch), MAX_STRING_LENGTH);
    (void)strncat(buf, argument, arglen);
    (void)strncat(buf, "\n\r", 2);

    (void)strncpy(EDIT_STRING(ch), buf, MAX_STRING_LENGTH);
    send_to_char("Line appended.\n\r", ch);
    return;
}

void sedit_linedelete(struct char_data *ch, const char *argument)
{
    char linenumber[MAX_STRING_LENGTH];
    //char *modified;

    argument = one_argument(argument, linenumber);
    if (!is_number(linenumber)) {
        send_to_char("`#Usage``: .ld [line number]\n\r", ch);
        return;
    }

    //modified = string_line_delete(EDIT_STRING(ch), atoi(linenumber));
    //(void)strncpy(EDIT_STRING(ch), modified, MAX_STRING_LENGTH);
    //free (modified);
    send_to_char("TODO: Line deleted.\n\r", ch);
    return;
}

void sedit_lineinsert(struct char_data *ch, const char *argument)
{
    char linenumber[MAX_STRING_LENGTH];
    //char *modified;

    argument = one_argument(argument, linenumber);
    if (!is_number(linenumber)) {
        send_to_char("`#Usage``: .li [line number] [string]\n\r", ch);
        return;
    }

    //modified = string_line_insert(EDIT_STRING(ch), argument, atoi(linenumber));
    //(void)strncpy(EDIT_STRING(ch), modified, MAX_STRING_LENGTH);
    //free (modified);
    send_to_char("TODO: Line inserted.\n\r", ch);
    return;
}

void sedit_help(struct char_data *ch, /*@unused@*/const char *argument)
{
    int cmdindex;

    send_to_char("`3-`#========`3- `@OLC Text Editor `3-`#=========`3-``\n\r", ch);
    for (cmdindex = 0; string_edit_table[cmdindex].do_fn != NULL; cmdindex++) {
        printf_to_char(ch, "%s\n\r", string_edit_table[cmdindex].lexicon);
    }
    return;
}

void sedit_commit(struct char_data *ch, /*@unused@*/const char *argument)
{
    DESC_SETTER *setter = EDIT_SETTER(ch);
    void *owner = EDIT_OWNER(ch);

    assert(setter != NULL);
    assert(owner != NULL);

    (*setter)(owner, EDIT_STRING(ch));
    EDIT_SETTER(ch) = NULL;
    EDIT_GETTER(ch) = NULL;
    EDIT_STRING(ch)[0] = null_token;
    EDIT_OWNER(ch) = NULL;
    ch->desc->interpmode = INTERP_MODE_STANDARD;
    return;
}

void sedit_abandon(struct char_data *ch, /*@unused@*/const char *argument)
{
    EDIT_SETTER(ch) = NULL;
    EDIT_GETTER(ch) = NULL;
    EDIT_STRING(ch)[0] = null_token;
    EDIT_OWNER(ch) = NULL;
    ch->desc->interpmode = INTERP_MODE_STANDARD;
    return;
}


const char *format_string(const char *orig)
{
    static char output[MAX_STRING_LENGTH];
    char word[MAX_STRING_LENGTH];
    const char *porig;
    char *pword, *pout;
    char c, pc;
    size_t linelength = 0;
    size_t actualwordlength = 0;
    size_t wordlength = 0;
    size_t outlength = 0;

    output[0] = null_token;
    word[0] = null_token;
    porig = orig;
    pword = word;
    pout = output;
    pc = null_token;
    c = *porig;
    while (c != null_token) {
        if (c == space_token || c == tab_token || c == newline_token) {
            if (outlength + actualwordlength + 6 < MAX_STRING_LENGTH) {
                //append word to line or start a new line and append.
                if (linelength + wordlength > MAX_LINE_LENGTH) {
                    *(pout++) = newline_token;
                    *(pout++) = return_token;
                    outlength += 2;
                    linelength = 0;
                }

                if (actualwordlength > 0) {
                    (void)strncpy(pout, word, actualwordlength);
                    pout += actualwordlength;
                    word[0] = null_token;
                    pword = word;
                    linelength += wordlength;
                    outlength += actualwordlength;
                    wordlength = actualwordlength = 0;
                }
                if (c == newline_token) {
                    if (pc == newline_token)
                        *(pout++) = newline_token;
                    else
                        *(pout++) = space_token;
                    outlength++;
                }
                if (c == space_token) {
                    *(pout++) = space_token;
                    outlength++;
                }
                if (c == tab_token) {
                    *(pout++) = space_token;
                    *(pout++) = space_token;
                    *(pout++) = space_token;
                    *(pout++) = space_token;
                    outlength += 4;
                }
                *pout = null_token;
            }
        } else if (c == color_token) {
            actualwordlength++;
            *(pword++) = c;
            *pword = null_token;
        } else if (c != return_token) {
            actualwordlength++;
            if (pc != color_token) 
                wordlength++;
            *(pword++) = c;
            *pword = null_token;
        }

        if (c != return_token)
            pc = c;
        c = *(++porig);
        if (outlength + 6 > MAX_STRING_LENGTH)
            break;
    }

    return output;
}


/***************************************************************************
 *	number_lines
 *
 *	put line numbers at the begining of each line of text
 ***************************************************************************/
static const size_t SERIALIZED_NUMBER_SIZE = 256;
char *number_lines(const char *string)
{
    static char output[MAX_STRING_LENGTH];
    char serialized_line_number[SERIALIZED_NUMBER_SIZE+1];
    const char *pin;
    char *pout;
    char cin;
    unsigned int linenumber = 0;

    pout = output;
    (void)snprintf(serialized_line_number, SERIALIZED_NUMBER_SIZE, "%u", linenumber);
    strncpy(pout, serialized_line_number, SERIALIZED_NUMBER_SIZE);
    pout += strlen(serialized_line_number);

    pin = string;
    cin = *pin;
    while (*pin != null_token) {
        (*pout++) = cin;

        if (cin == '\n') {
            linenumber++;
            (void)snprintf(serialized_line_number, SERIALIZED_NUMBER_SIZE, "%u", linenumber);
            strncpy(pout, serialized_line_number, SERIALIZED_NUMBER_SIZE);
            pout += strlen(serialized_line_number);
        }

        cin = *(++pin);
    }

    return output;
}
