#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "entityload.h"

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

    {
        memset(areadata, 0, sizeof(struct area_data));
        /*@-mustfreeonly@*/
        areadata->vnum = vnum;
        areadata->name = strdup("New area");
        areadata->description = strdup("");
        areadata->area_flags = AREA_ADDED;
        areadata->security = 1;
        areadata->builders = strdup("");
        areadata->credits = strdup("");
        areadata->empty = true;
        
        {
            char filename[MAX_INPUT_LENGTH];
            (void)snprintf(filename, MAX_INPUT_LENGTH, "area%lu.are", areadata->vnum);
            areadata->file_name = strdup(filename);
        }
        /*@+mustfreeonly@*/
    }

    headlist_add(areadata);

    return areadata;
}

struct array_list *area_serialize(const struct area_data *areadata)
{
    struct array_list *answer;

    answer = kvp_create_array(20);

    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "vnum", "%lu", areadata->vnum);
    kvp_array_append_copy(answer, "name", areadata->name);
    if (areadata->description[0] != '\0') {
        kvp_array_append_copy(answer, "description", areadata->description);
    }
    {
        char *flags = flag_to_string(areadata->area_flags);
        kvp_array_append_copy(answer, "flags", flags);
        free(flags);
    }
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "security", "%u", areadata->security);
    if (areadata->builders[0] != '\0') {
        kvp_array_append_copy(answer, "builders", areadata->builders);
    }
    if (areadata->credits[0] != '\0') {
        kvp_array_append_copy(answer, "credits", areadata->credits);
    }
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "min_vnum", "%lu", areadata->min_vnum);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "max_vnum", "%lu", areadata->max_vnum);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "llevel", "%u", areadata->llevel);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "ulevel", "%u", areadata->ulevel);

    return answer;
}



struct area_data *area_deserialize(const struct array_list *data, const char *filename)
{
    struct area_data *areadata;
    const char *entry;

    areadata = malloc(sizeof(struct area_data));
    assert(areadata != NULL);
    memset(areadata, 0, sizeof(struct area_data));

    /*@-mustfreeonly@*/
    ASSIGN_ULONG_KEY(data, areadata->vnum, "vnum");
    areadata->file_name = strdup(filename);
    ASSIGN_STRING_KEY_DEFAULT(data, areadata->name, "name", "no name");
    ASSIGN_STRING_KEY_DEFAULT(data, areadata->description, "description", "(no description)");
    ASSIGN_STRING_KEY_DEFAULT(data, areadata->credits, "credits", "");
    ASSIGN_STRING_KEY_DEFAULT(data, areadata->builders, "builders", "");
    ASSIGN_ULONG_KEY(data, areadata->min_vnum, "min_vnum");
    ASSIGN_ULONG_KEY(data, areadata->max_vnum, "max_vnum");
    ASSIGN_FLAG_KEY(data, areadata->area_flags, "flags");
    ASSIGN_UINT_KEY(data, areadata->security, "security");
    ASSIGN_UINT_KEY(data, areadata->llevel, "llevel");
    ASSIGN_UINT_KEY(data, areadata->ulevel, "ulevel");
    /*@+mustfreeonly@*/

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
    free(areadata->credits);
    free(areadata->builders);

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

