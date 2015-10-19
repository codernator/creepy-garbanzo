#include "merc.h"
#include "help.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


typedef void HELP_HANDLER(/*@partial@*/struct char_data *, int trust, /*@observer@*/const char *argument);
struct help_table_entry {
    const char const *key;
    HELP_HANDLER *handler;
};


static HELP_DATA head_node;
static int help_count = 0;

static /*@observer@*/const struct help_table_entry *findcustom(/*@observer@*/const char *topic);
static void handle_help_keyfind(struct char_data *ch, int trust, /*@observer@*/const char *argument);
static void handle_help_fullsearch(struct char_data *ch, int trust, /*@observer@*/const char *argument);
static void handle_help_other(struct char_data *ch, int trust, /*@observer@*/const char *argument);
static void headlist_add(/*@owned@*/HELP_DATA *entry);


static const struct help_table_entry help_table[] = {
    { .key = "find", .handler = handle_help_keyfind },
    { .key = "search", .handler = handle_help_fullsearch },
    { .key = NULL, .handler = NULL }
};


inline int count_helps()
{
    return help_count;
}

HELP_DATA *helpdata_new()
{
    HELP_DATA *helpdata;
    helpdata = malloc(sizeof(HELP_DATA));
    assert(helpdata != NULL);
    
    memset(helpdata, 0, sizeof(HELP_DATA));

    headlist_add(helpdata);
    return helpdata;
}

KEYVALUEPAIR_ARRAY *helpdata_serialize(const HELP_DATA *helpdata)
{
    KEYVALUEPAIR_ARRAY *answer;

    answer = keyvaluepairarray_create(5);

    keyvaluepairarray_append(answer, "keyword", helpdata->keyword);
    keyvaluepairarray_append(answer, "text", helpdata->text);
    if (helpdata->trust != 0) {
        keyvaluepairarray_appendf(answer, 32, "trust", "%d", helpdata->trust);
    }
    if (helpdata->level != 0) {
        keyvaluepairarray_appendf(answer, 32, "level", "%d", helpdata->level);
    }
    if (helpdata->category != NULL) {
        keyvaluepairarray_append(answer, "cateogry", helpdata->category);
    }

    return answer;
}

HELP_DATA *helpdata_deserialize(const KEYVALUEPAIR_ARRAY *data)
{
    HELP_DATA *helpdata;
    const char *value;

    helpdata = malloc(sizeof(HELP_DATA));
    assert(helpdata != NULL);
    
    value = keyvaluepairarray_find(data, "keyword");
    assert(value != NULL);
    helpdata->keyword = string_copy(value);

    value = keyvaluepairarray_find(data, "text");
    assert(value != NULL);
    helpdata->text = string_copy(value);

    /** optional fields */
    value = keyvaluepairarray_find(data, "trust");
    helpdata->trust = (value != NULL) ? parse_int(value) : 0;

    value = keyvaluepairarray_find(data, "level");
    helpdata->level = (value != NULL) ? parse_int(value) : 0;

    value = keyvaluepairarray_find(data, "category");
    helpdata->category = (value != NULL) ? string_copy(value) : NULL;

    headlist_add(helpdata);
    return helpdata;
}

void helpdata_free(HELP_DATA *helpdata)
{
    HELP_DATA *prev = helpdata->prev;
    HELP_DATA *next = helpdata->next;

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

HELP_DATA *help_lookup(const char *keyword)
{
    HELP_DATA *help;

    for (help = head_node.next; help != NULL; help = help->next) {
        if (!str_infix(keyword, help->keyword)
            || (keyword[0] == help->keyword[0] && !str_cmp(keyword, help->keyword)))
            return help;
    }

    return NULL;
}

struct helpdata_iterator *helpdata_iteratorstart()
{
    HELP_DATA *current = head_node.next;
    struct helpdata_iterator *iterator;
    if (current == NULL)
        return NULL;

    iterator = malloc(sizeof (struct helpdata_iterator));
    assert(iterator != NULL);
    iterator->current = current;
    return iterator;
}

struct helpdata_iterator *helpdata_iteratornext(struct helpdata_iterator *iterator)
{
    HELP_DATA *current = iterator->current->next;
    if (current == NULL) {
        free(iterator);
        return NULL;
    }

    iterator->current = current;
    return iterator;
}

void show_help(struct descriptor_data *descriptor, const char *topic, const char *argument)
{
    /*@observer@*/const struct help_table_entry *custom;
    int trust;
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

void handle_help_keyfind(struct char_data *ch, int trust, const char *argument)
{
    HELP_DATA *help;
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

void handle_help_fullsearch(struct char_data *ch, int trust, const char *argument)
{
    HELP_DATA *help;
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

void handle_help_other(struct char_data *ch, int trust, const char *topic)
{
    HELP_DATA *help;

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

void headlist_add(HELP_DATA *entry)
{
    HELP_DATA *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

