#include "merc.h"
#include "help.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "entityload.h"


typedef void HELP_HANDLER(/*@partial@*/struct char_data *, unsigned int trust, /*@observer@*/const char *argument);
struct help_table_entry {
    const char const *key;
    HELP_HANDLER *handler;
};


static struct help_data head_node;
static int help_count = 0;

static /*@observer@*/const struct help_table_entry *findcustom(/*@observer@*/const char *topic);
static void handle_help_keyfind(struct char_data *ch, unsigned int trust, /*@observer@*/const char *argument);
static void handle_help_fullsearch(struct char_data *ch, unsigned int trust, /*@observer@*/const char *argument);
static void handle_help_other(struct char_data *ch, unsigned int trust, /*@observer@*/const char *argument);
static void headlist_add(/*@owned@*/struct help_data *entry);


static const struct help_table_entry help_table[] = {
    { .key = "find", .handler = handle_help_keyfind },
    { .key = "search", .handler = handle_help_fullsearch },
    { .key = NULL, .handler = NULL }
};


inline int count_helps()
{
    return help_count;
}

struct help_data *helpdata_new()
{
    struct help_data *helpdata;
    helpdata = malloc(sizeof(struct help_data));
    assert(helpdata != NULL);
    
    memset(helpdata, 0, sizeof(struct help_data));

    headlist_add(helpdata);
    return helpdata;
}

struct array_list *helpdata_serialize(const struct help_data *helpdata)
{
    struct array_list *answer;

    answer = kvp_create_array(5);

    kvp_array_append_copy(answer, "keyword", helpdata->keyword);
    kvp_array_append_copy(answer, "text", helpdata->text);
    if (helpdata->trust != 0) {
        kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "trust", "%u", helpdata->trust);
    }
    if (helpdata->level != 0) {
        kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "level", "%u", helpdata->level);
    }
    if (helpdata->category != NULL) {
        kvp_array_append_copy(answer, "cateogry", helpdata->category);
    }

    return answer;
}

struct help_data *helpdata_deserialize(const struct array_list *data)
{
    struct help_data *helpdata;
    const char *entry;

    helpdata = malloc(sizeof(struct help_data));
    assert(helpdata != NULL);
    
    ASSIGN_STRING_KEY(data, helpdata->keyword, "keyword");
    if (helpdata->keyword == NULL) {
        helpdata->keyword = strdup("BORKED");
        log_bug("Missing keyword in helpdata.");
    }
    ASSIGN_STRING_KEY(data, helpdata->text, "text");
    if (helpdata->text == NULL) {
        helpdata->text = strdup("BORKED");
        log_bug("Missing text in helpdata.");
    }

    /** optional fields */
    ASSIGN_STRING_KEY(data, helpdata->category, "category");
    ASSIGN_UINT_KEY(data, helpdata->trust, "trust");
    ASSIGN_UINT_KEY(data, helpdata->level, "level");

    headlist_add(helpdata);
    return helpdata;
}

void helpdata_free(struct help_data *helpdata)
{
    struct help_data *prev = helpdata->prev;
    struct help_data *next = helpdata->next;

    assert(helpdata != NULL);
    assert(helpdata != &head_node);
    assert(prev != NULL); /** because only the head node has no previous. */

    prev->next = next;
    if (next != NULL)
        next->prev = prev;

    if (helpdata->keyword != NULL)
        free(helpdata->keyword);
    if (helpdata->text != NULL)
        free(helpdata->text);

    if (helpdata->category != NULL)
        free(helpdata->category);

    free(helpdata);
}


inline bool is_help(const char *argument)
{
    char first;

    first = argument[0];
    switch (first) {
    case '\0': // TODO an empty argument is a cry for help only in some cases, no?
    case '?': // ?jump is a cry for help on the term 'jump'
        return true;

    default:
        {
            /* We only care if the argument starts with the literal string "help ".
             * The check must be case-insensitive.
             * Lowering at most the first 5 characters (and terminating on the 6th) gives
             * all the data we need.
             */
            char lowered[6];
            string_lower(argument, lowered, 6);
            return strncmp(lowered, "help", 5) == 0;
        }
    }
}

struct help_data *help_lookup(const char *keyword)
{
    struct help_data *help;

    for (help = head_node.next; help != NULL; help = help->next) {
        if (!str_infix(keyword, help->keyword)
            || (keyword[0] == help->keyword[0] && !str_cmp(keyword, help->keyword)))
            return help;
    }

    return NULL;
}

struct help_data *helpdata_iteratorstart()
{
    return head_node.next;
}

struct help_data *helpdata_iteratornext(struct help_data *current)
{
    return current->next;;
}

void show_help(struct descriptor_data *descriptor, const char *topic, const char *argument)
{
    /*@observer@*/const struct help_table_entry *custom;
    unsigned int trust;
    struct char_data *actual;

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

void handle_help_keyfind(struct char_data *ch, unsigned int trust, const char *argument)
{
    struct help_data *help;
    struct buf_type *buf;
    int index = 0;

    buf = new_buf();

    printf_buf(buf, "Helps matching the query: %s\n\r", argument);
    for (help = head_node.next; help != NULL; help = help->next) {
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

void handle_help_fullsearch(struct char_data *ch, unsigned int trust, const char *argument)
{
    struct help_data *help;
    struct buf_type *buf;
    char *txt;
    int index = 0;

    buf = new_buf();

    printf_buf(buf, "Helps matching the query: %s\n\r", argument);
    for (help = head_node.next; help != NULL; help = help->next) {
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

void handle_help_other(struct char_data *ch, unsigned int trust, const char *topic)
{
    struct help_data *help;

    for (help = head_node.next; help != NULL; help = help->next) {
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

void headlist_add(struct help_data *entry)
{
    struct help_data *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

