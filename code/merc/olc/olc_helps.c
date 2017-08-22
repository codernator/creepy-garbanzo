#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "lookup.h"
#include "recycle.h"
#include "help.h"



static void olc_help_gettext(void *owner, char *target, size_t maxlen);
static void olc_help_settext(void *owner, const char *text);

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
    { "?",	      show_olc_help	},

    { NULL,	      0		    }
};




void hedit(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    const char *parg;
    char command[MAX_INPUT_LENGTH];
    int cmd;

    strcpy(arg, argument);
    smash_tilde(arg);
    parg = one_argument(arg, command);

    if (ch->pcdata->security < 4) {
        send_to_char("HEdit: Insuficient security to edit helps.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        hedit_show(ch, parg);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    for (cmd = 0; hedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, hedit_table[cmd].name)) {
            (void)(*hedit_table[cmd].olc_fn)(ch, parg);
            return;
        }
    }

    interpret(ch, arg);
}

void do_hedit(struct char_data *ch, const char *argument)
{
    struct help_data *help;
    char cmd[MAX_INPUT_LENGTH];

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
}

EDIT(hedit_show){
    struct help_data *help;

    EDIT_HELP(ch, help);

    printf_to_char(ch, "`&Keyword``:  [%s]\n\r`&Level``:    [%d]\n\r", help->keyword, help->level);
    printf_to_char(ch, "`&Text``:\n\r%s\n\r`1-`!END`1-``\n\r", help->text);
    return false;
}

EDIT(hedit_level){
    struct help_data *help;
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

EDIT(hedit_keyword){
    struct help_data *help;

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

EDIT(hedit_new){
    struct help_data *help;
    char name[MAX_INPUT_LENGTH];

    if (is_help(argument)) {
        send_to_char("Syntax   : new [name]\n\r", ch);
        return false;
    }

    argument = one_argument(argument, name);

    if (help_lookup(name)) {
        send_to_char("HEdit : help already exists.\n\r", ch);
        return false;
    }

    help = helpdata_new();
    help->keyword = str_dup(name);
    help->text = str_dup("");

    ch->desc->ed_data = (struct help_data *)help;
    ch->desc->editor = ED_HELP;

    send_to_char("Ok.\n\r", ch);
    return false;
}

EDIT(hedit_text){
    struct help_data *help;

    EDIT_HELP(ch, help);
    olc_start_string_editor(ch, help, olc_help_gettext, olc_help_settext);
    return true;
}

EDIT(hedit_delete){
    struct help_data *help;
    struct descriptor_data *d;
    struct descriptor_iterator_filter playing_filter = { .all = true, .must_playing = true };
    struct descriptor_data *dpending;

    EDIT_HELP(ch, help);

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
        dpending = descriptor_iterator(d, &playing_filter);
        if (d->editor == ED_HELP && help == (struct help_data *)d->ed_data)
            edit_done(d->character);
    }

    helpdata_free(help);
    send_to_char("Ok.\n\r", ch);
    return true;
}

EDIT(hedit_list){
    static char buf[MAX_INPUT_LENGTH];
    struct buf_type *buffer;
    int cnt = 0;
    struct help_data *help;

    EDIT_HELP(ch, help);

    buffer = new_buf();
    help = helpdata_iteratorstart();
    while (help != NULL) {
        (void)snprintf(buf, MAX_INPUT_LENGTH, "%3d. %-14.14s%s", cnt, help->keyword, cnt % 4 == 3 ? "\n\r" : " ");
        add_buf(buffer, buf);
        cnt++;
        help = helpdata_iteratornext(help);
    }

    if (cnt % 4) {
        add_buf(buffer, "\n\r");
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return false;
}


void olc_help_gettext(void *owner, char *target, size_t maxlen)
{
    struct help_data *help;
    help = (struct help_data *)owner;
    (void)strncpy(target, help->text, maxlen);
    return;
}
void olc_help_settext(void *owner, const char *text)
{
    struct help_data *help;
    help = (struct help_data *)owner;
    free_string(help->text);
    help->text = str_dup(text);
    return;
}
