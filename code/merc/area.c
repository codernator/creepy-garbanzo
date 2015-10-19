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

struct area_iterator *area_iterator_start(const struct area_filter *filter)
{
    struct area_iterator *iterator;
    struct area_data *current = head_node.next;

    while (current != NULL && !passes(current, filter)) {
        current = current->next;
    }

    if (current == NULL) {
        return NULL;
    }

    iterator = malloc(sizeof(struct area_iterator));
    assert(iterator != NULL);
    iterator->current = current;

    return iterator;
}

struct area_iterator *area_iterator(struct area_iterator *iterator, const struct area_filter *filter)
{
    struct area_data *current = iterator->current->next;
    while (current != NULL && !passes(current, filter)) {
        current = current->next;
    }

    if (current == NULL) {
        free(iterator);
        return NULL;
    }

    iterator->current = current;
    return iterator;
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
        areadata->name = string_copy("New area");
        areadata->description = string_copy("");
        areadata->area_flags = AREA_ADDED;
        areadata->security = 1;
        areadata->builders = string_copy("");
        areadata->credits = string_copy("");
        areadata->empty = true;
        
        {
            char filename[MAX_INPUT_LENGTH];
            (void)snprintf(filename, MAX_INPUT_LENGTH, "area%lu.are", areadata->vnum);
            areadata->file_name = string_copy(filename);
        }
        /*@+mustfreeonly@*/
    }

    headlist_add(areadata);

    return areadata;
}

KEYVALUEPAIR_ARRAY *area_serialize(const struct area_data *areadata)
{
    KEYVALUEPAIR_ARRAY *answer;

    answer = keyvaluepairarray_create(20);

    keyvaluepairarray_appendf(answer, 64, "vnum", "%lu", areadata->vnum);
    keyvaluepairarray_append(answer, "name", areadata->name);
    if (areadata->description[0] != '\0') {
        keyvaluepairarray_append(answer, "description", areadata->description);
    }
    {
        char *flags = flag_to_string(areadata->area_flags);
        keyvaluepairarray_append(answer, "flags", flags);
        free(flags);
    }
    keyvaluepairarray_appendf(answer, 64, "security", "%u", areadata->security);
    if (areadata->builders[0] != '\0') {
        keyvaluepairarray_append(answer, "builders", areadata->builders);
    }
    if (areadata->credits[0] != '\0') {
        keyvaluepairarray_append(answer, "credits", areadata->credits);
    }
    keyvaluepairarray_appendf(answer, 64, "min_vnum", "%lu", areadata->min_vnum);
    keyvaluepairarray_appendf(answer, 64, "max_vnum", "%lu", areadata->max_vnum);
    keyvaluepairarray_appendf(answer, 64, "llevel", "%u", areadata->llevel);
    keyvaluepairarray_appendf(answer, 64, "ulevel", "%u", areadata->ulevel);

    return answer;
}



struct area_data *area_deserialize(const KEYVALUEPAIR_ARRAY *data, const char *filename)
{
    struct area_data *areadata;
    const char *entry;

    areadata = malloc(sizeof(struct area_data));
    assert(areadata != NULL);
    memset(areadata, 0, sizeof(struct area_data));

    /*@-mustfreeonly@*/
    ASSIGN_ULONG_KEY(data, areadata->vnum, "vnum");
    areadata->file_name = string_copy(filename);
    ASSIGN_STRING_KEY(data, areadata->name, "name");
    ASSIGN_STRING_KEY(data, areadata->description, "description");
    ASSIGN_STRING_KEY(data, areadata->credits, "credits");
    ASSIGN_STRING_KEY(data, areadata->builders, "builders");
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

