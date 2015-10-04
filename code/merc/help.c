#include <stdio.h>
#include "merc.h"
#include "recycle.h"


typedef void HELP_HANDLER(/*@partial@*/CHAR_DATA *, int trust, /*@observer@*/const char *argument);
struct help_table_entry {
    const char const *key;
    HELP_HANDLER *handler;
};

static /*@observer@*/const struct help_table_entry *findcustom(/*@observer@*/const char *topic);
static void handle_help_keyfind(CHAR_DATA *ch, int trust, /*@observer@*/const char *argument);
static void handle_help_fullsearch(CHAR_DATA *ch, int trust, /*@observer@*/const char *argument);
static void handle_help_other(CHAR_DATA *ch, int trust, /*@observer@*/const char *argument);

const struct help_table_entry help_table[] = {
    { .key = "find", .handler = handle_help_keyfind },
    { .key = "search", .handler = handle_help_fullsearch },
    { .key = NULL, .handler = NULL }
};


void do_help(CHAR_DATA *ch, const char *argument)
{
    char topic[MIL];

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, topic);
    show_help(ch->desc, topic, argument);
}


/**
 * see if a character string is a cry for help
 */
inline bool is_help(const char *argument)
{
    return (argument[0] == '\0' || argument[0] == '?' || !str_prefix(argument, "help"));
}

void show_help(DESCRIPTOR_DATA *descriptor, const char *topic, const char *argument)
{
    /*@observer@*/const struct help_table_entry *custom;
    int trust;
    CHAR_DATA *actual;

    actual = CH(descriptor);
    trust = get_trust(actual);
    custom = findcustom(topic);
    if (custom->key == NULL) {
        handle_help_other(actual, trust, topic);
    } else {
        (*custom->handler)(actual, trust, argument);
    }
}

const struct help_table_entry *findcustom(const char *topic) 
{
    const struct help_table_entry *custom;
    int i = 0;
    custom = &help_table[i];
    while (custom->key != NULL && str_cmp(custom->key, topic)) {
        custom = &help_table[++i];
    }
    return custom;
}

void handle_help_keyfind(CHAR_DATA *ch, int trust, const char *argument)
{
    HELP_DATA *help;
    BUFFER *buf;
    int index = 0;

    buf = new_buf();

    printf_buf(buf, "Helps matching the query: %s\n\r", argument);
    for (help = help_first; help != NULL; help = help->next) {
        if (help->level <= trust) {
            if (!str_infix(argument, help->keyword)) {
                index++;
                printf_buf(buf, "[%.3d]  %s\n\r", index, help->keyword);
            }
        }
    }

    if (index == 0)
        (void)add_buf(buf, "  ...no helps matching the selected criteria.\n\r\n\r");

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
}

void handle_help_fullsearch(CHAR_DATA *ch, int trust, const char *argument)
{
    HELP_DATA *help;
    BUFFER *buf;
    char *txt;
    int index = 0;

    buf = new_buf();

    printf_buf(buf, "Helps matching the query: %s\n\r", argument);
    for (help = help_first; help != NULL; help = help->next) {
        if (help->level <= trust) {
            txt = uncolor_str(help->text);
            if (!str_infix(argument, txt)) {
                index++;
                printf_buf(buf, "[%.3d]  %s\n\r", index, help->keyword);
            }
            free_string(txt);
        }
    }

    if (index == 0)
        (void)add_buf(buf, "  ...no helps matching the selected criteria.\n\r\n\r");

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
}

void handle_help_other(CHAR_DATA *ch, int trust, const char *topic)
{
    HELP_DATA *help;

    for (help = help_first; help != NULL; help = help->next) {
        if (help->level <= trust) {
            if (is_name(topic, help->keyword)) {
                if (help->text[0] == '!') {
                    send_to_char(help->text + 1, ch);
                } else {
                    if (help->text[0] == '.')
                        page_to_char(help->text + 1, ch);
                    else
                        page_to_char(help->text, ch);
                }
                return;
            }
        }
    }

    printf_to_char(ch, "No help on '%s'.\n\r", topic);
}

