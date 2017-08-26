#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"

const struct area_filter area_empty_filter;

static struct area_data head_node;
static bool passes(struct area_data *testee, /*@null@*//*@partial@*/const struct area_filter *filter);
static void headlist_add(/*@owned@*/struct area_data *entry);


struct area_data *area_iterator_start(const struct area_filter *filter)
{
    struct area_data *current = head_node.next;

    while (current != NULL && !passes(current, filter)) {
        current = current->next;
    }

    return current;
}

struct area_data *area_iterator(struct area_data *iterator, const struct area_filter *filter)
{
    struct area_data *current = iterator->next;
    while (current != NULL && !passes(current, filter)) {
        current = current->next;
    }

    return current;
}

struct area_data *area_getbyvnum(unsigned long vnum)
{
    struct area_data *subject;

    subject = head_node.next;
    while (subject != NULL) {
        if (subject->vnum == vnum) {
            return subject;
        }
        subject = subject->next;
    }
    return NULL;
}

struct area_data *area_getbycontainingvnum(unsigned long vnum)
{
    struct area_data *subject;

    subject = head_node.next;
    while (subject != NULL) {
        if (subject->min_vnum <= vnum && subject->max_vnum >= vnum) {
            return subject;
        }
        subject = subject->next;
    }
    return NULL;
}

struct area_data *area_new(unsigned long vnum)
{
    struct area_data *areadata;

    areadata = malloc(sizeof(struct area_data));
    assert(areadata != NULL);
    memset(areadata, 0, sizeof(struct area_data));

    areadata->vnum = vnum;
    /*@-mustfreeonly@*/
    {
        char filename[MAX_INPUT_LENGTH];
        (void)snprintf(filename, MAX_INPUT_LENGTH, "area%lu.are", areadata->vnum);
        areadata->file_name = strdup(filename);
    }
    areadata->name = strdup("New area");
    areadata->description = strdup("");
    /*@+mustfreeonly@*/
    areadata->area_flags = AREA_ADDED;
    areadata->security = 1;
    areadata->empty = true;

    headlist_add(areadata);

    return areadata;
}

struct array_list *area_serialize(const struct area_data *areadata)
{
    struct array_list *answer;
    struct reset_data *reset;

    answer = kvp_create_array(12);

    serialize_take_string(answer, "vnum", ulong_to_string(areadata->vnum));
    serialize_copy_string(answer, "name", areadata->name);
    if (areadata->description[0] != '\0') {
        serialize_copy_string(answer, "description", areadata->description);
    }
    serialize_take_string(answer, "flags", flag_to_string(areadata->area_flags));
    serialize_take_string(answer, "security", uint_to_string(areadata->security));

    serialize_take_string(answer, "min_vnum", ulong_to_string(areadata->min_vnum));
    serialize_take_string(answer, "max_vnum", ulong_to_string(areadata->max_vnum));
    serialize_take_string(answer, "llevel", uint_to_string(areadata->llevel));
    serialize_take_string(answer, "ulevel", uint_to_string(areadata->ulevel));

    for (reset = areadata->reset_first; reset != NULL; reset = reset->next) {
        struct array_list *serialized = resetdata_serialize(reset);
        serialize_take_string(answer, "reset", database_create_stream(serialized));
        kvp_free_array(serialized);
    }

    return answer;
}

struct area_data *area_deserialize(const struct array_list *data, const char *filename)
{
    struct area_data *areadata;
    const char *entry;
    const struct key_string_pair *entrynode;

    areadata = malloc(sizeof(struct area_data));
    assert(areadata != NULL);
    memset(areadata, 0, sizeof(struct area_data));

    /*@-mustfreeonly@*/
    areadata->file_name = strdup(filename);
    deserialize_assign_ulong(data, areadata->vnum, "vnum");
    deserialize_assign_string_default(data, areadata->name, "name", "no name");
    deserialize_assign_string_default(data, areadata->description, "description", "(no description)");
    deserialize_assign_flag(data, areadata->area_flags, "flags");
    deserialize_assign_uint(data, areadata->security, "security");
    deserialize_assign_ulong(data, areadata->min_vnum, "min_vnum");
    deserialize_assign_ulong(data, areadata->max_vnum, "max_vnum");
    deserialize_assign_uint(data, areadata->llevel, "llevel");
    deserialize_assign_uint(data, areadata->ulevel, "ulevel");
    /*@+mustfreeonly@*/

    {
        size_t index;
        struct array_list *serialized;
        array_list_each(data, index) {
            entrynode = array_list_node_at(data, struct key_string_pair, index);
            assert(entrynode != NULL);

            if (entrynode->key[0] == 'r' && strcmp(entrynode->key, "reset") == 0) {
                struct reset_data *element;
                serialized = database_parse_stream(entrynode->value);
                element = resetdata_deserialize(serialized);
                assert(element->next == NULL);
                if(areadata->reset_last == NULL)
                    areadata->reset_last = element;
                element->next = areadata->reset_first;
                areadata->reset_first = element;
                kvp_free_array(serialized);
            }

        }
    }

    headlist_add(areadata);
    return areadata;
}

void area_free(struct area_data *areadata)
{
    struct area_data *prev = areadata->prev;
    struct area_data *next = areadata->next;

    assert(areadata != &head_node);
    assert(prev != NULL); /** because only the head node has no previous. */

    prev->next = next;
    if (next != NULL)
        next->prev = prev;

    free(areadata->name);
    free(areadata->description);

    /** Clean up resets. */
    {
        /*@only@*/struct reset_data *reset = areadata->reset_first;
        /*@only@*/struct reset_data *reset_next = areadata->reset_first;
        while(reset != NULL) {
            reset_next = reset->next;
            resetdata_free(reset);
            reset = reset_next;
        }
        areadata->reset_last = NULL;
    }

    free(areadata);
}

void headlist_add(struct area_data *entry)
{
    struct area_data *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

bool passes(struct area_data *testee, const struct area_filter *filter)
{
    if (filter == NULL || filter->all) {
        return true;
    }
    return testee->vnum == filter->vnum;
}


void olc_area_getdescription(void *owner, char *target, size_t maxsize)
{
    struct area_data *area;
    area = (struct area_data *)owner;
    (void)strncpy(target, area->description, maxsize);
}

void olc_area_setdescription(void *owner, const char *text)
{
    struct area_data *area;
    area = (struct area_data *)owner;
    if (area->description != NULL)
        free (area->description);
    area->description = strdup(text);
    return;
}
