#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const AREA_FILTER area_empty_filter;

static AREA_DATA head_node;
static bool passes(AREA_DATA *testee, const AREA_FILTER *filter);
static void headlist_add(/*@owned@*/AREA_DATA *entry);


AREA_DATA *area_getbyvnum(unsigned long vnum)
{
    AREA_DATA *subject;
    AREA_FILTER vnum_filter;

    vnum_filter.vnum = vnum;

    subject = head_node.next;
    while (subject != NULL) {
        if (passes(subject, &vnum_filter)) {
            return subject;
        }
        subject = subject->next;
    }
    return NULL;
}



AREA_DATA *area_new(unsigned long vnum)
{
    AREA_DATA *areadata;

    areadata = malloc(sizeof(AREA_DATA));
    assert(areadata != NULL);

    {
        memset(areadata, 0, sizeof(AREA_DATA));
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
            char filename[MIL];
            (void)snprintf(filename, MIL, "area%lu.are", areadata->vnum);
            areadata->file_name = string_copy(filename);
        }
        /*@+mustfreeonly@*/
    }

    headlist_add(areadata);

    return areadata;
}

KEYVALUEPAIR_ARRAY *area_serialize(const AREA_DATA *areadata)
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


#define ASSIGN_ULONG_KEY(field, name) \
    entry = keyvaluepairarray_find(data, (name)); \
    (field) = (entry != NULL) ? parse_unsigned_long(entry) : 0;

#define ASSIGN_UINT_KEY(field, name) \
    entry = keyvaluepairarray_find(data, (name)); \
    (field) = (entry != NULL) ? parse_unsigned_int(entry) : 0;

#define ASSIGN_STRING_KEY(field, name) \
    entry = keyvaluepairarray_find(data, (name)); \
    (field) = (entry != NULL) ? string_copy(entry) : 0;

#define ASSIGN_FLAG_KEY(field, name) \
    entry = keyvaluepairarray_find(data, (name)); \
    (field) = (entry != NULL) ? flag_from_string(entry) : 0;

#define ASSIGN_BOOL_KEY(field, name) \
    entry = keyvaluepairarray_find(data, (name)); \
    (field) = (entry != NULL) ? parse_int(entry) == 1 : false;

AREA_DATA *area_deserialize(const KEYVALUEPAIR_ARRAY *data, const char *filename)
{
    AREA_DATA *areadata;
    const char *entry;

    areadata = malloc(sizeof(AREA_DATA));
    assert(areadata != NULL);
    memset(areadata, 0, sizeof(AREA_DATA));

    /*@-mustfreeonly@*/
    ASSIGN_ULONG_KEY(areadata->vnum, "vnum");
    areadata->file_name = string_copy(filename);
    ASSIGN_STRING_KEY(areadata->name, "name");
    ASSIGN_STRING_KEY(areadata->description, "description");
    ASSIGN_STRING_KEY(areadata->credits, "credits");
    ASSIGN_STRING_KEY(areadata->builders, "builders");
    ASSIGN_ULONG_KEY(areadata->min_vnum, "min_vnum");
    ASSIGN_ULONG_KEY(areadata->max_vnum, "max_vnum");
    ASSIGN_FLAG_KEY(areadata->area_flags, "flags");
    ASSIGN_UINT_KEY(areadata->security, "security");
    ASSIGN_UINT_KEY(areadata->llevel, "llevel");
    ASSIGN_UINT_KEY(areadata->ulevel, "ulevel");
    /*@+mustfreeonly@*/

    headlist_add(areadata);
    return areadata;
}

void area_free(AREA_DATA *areadata)
{
    AREA_DATA *prev = areadata->prev;
    AREA_DATA *next = areadata->next;

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

void headlist_add(AREA_DATA *entry)
{
    AREA_DATA *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

bool passes(AREA_DATA *testee, const AREA_FILTER *filter)
{
    return testee->vnum == filter->vnum;
}

