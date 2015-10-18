#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef OBJPROTO_MAX_KEY_HASH
// Consult http://www.planetmath.org/goodhashtableprimes for a nice writeup on numbers to use.
// Bear in mind that whatever value is used here translates to an array containing this number of elements.
#define OBJPROTO_MAX_KEY_HASH 3079 
#endif

#define HASH_KEY(vnum) (vnum) % OBJPROTO_MAX_KEY_HASH

/** exports */
const OBJECTPROTOTYPE_FILTER objectprototype_empty_filter;


/** imports */
extern void free_affect(AFFECT_DATA *af);
extern void free_extra_descr(EXTRA_DESCR_DATA *ed);


/** locals */
typedef struct hash_entry HASH_ENTRY;
struct hash_entry {
    /*@only@*/HASH_ENTRY *next;
    /*@dependent@*/struct objectprototype *entry;
};

static struct objectprototype head_node;
static bool passes(struct objectprototype *testee, const OBJECTPROTOTYPE_FILTER *filter);
static void headlist_add(/*@owned@*/struct objectprototype *entry);
/* an array of linked lists, with an empty head node. */
static HASH_ENTRY lookup[OBJPROTO_MAX_KEY_HASH];
static void lookup_add(long entrykey, /*@dependent@*/struct objectprototype *entry);
static void lookup_remove(long entrykey, /*@dependent@*/struct objectprototype *entry);
static size_t count_extras(const struct objectprototype *obj);
static size_t count_affects(const struct objectprototype *obj);

/*
 * Translates object virtual number to its obj index struct.
 * Hash table lookup.
 */
struct objectprototype *objectprototype_getbyvnum(long vnum)
{
    HASH_ENTRY *hashentry;
    long hashkey = HASH_KEY(vnum);

    for (hashentry = lookup[hashkey].next; 
         hashentry != NULL && hashentry->entry->vnum != vnum;
         hashentry = hashentry->next);

    return hashentry == NULL 
        ? NULL 
        : hashentry->entry;
}

struct objectprototype *objectprototype_new(long vnum)
{
    struct objectprototype *prototypedata;

    prototypedata = malloc(sizeof(struct objectprototype));
    assert(prototypedata != NULL);

    /** Default values */
    {
        memset(prototypedata, 0, sizeof(struct objectprototype));
        prototypedata->vnum = vnum;
        prototypedata->name = str_dup("no name");
        prototypedata->short_descr = str_dup("(no short description)");
        prototypedata->description = str_dup("(no description)");
        prototypedata->item_type = ITEM_TRASH;
        prototypedata->material = str_dup("unknown");             /* ROM */
        prototypedata->condition = 100;                           /* ROM */
    }

    /** Place on list. */
    headlist_add(prototypedata);

    /** Store in hash table. */
    lookup_add(vnum, prototypedata);

    return prototypedata;
}

struct objectprototype *objectprototype_deserialize(const KEYVALUEPAIR_ARRAY *data)
{
    struct objectprototype *prototypedata;
    long vnum;
    const char *vnumentry;

    prototypedata = malloc(sizeof(struct objectprototype));
    assert(prototypedata != NULL);
    memset(prototypedata, 0, sizeof(struct objectprototype));

    vnumentry = keyvaluepairarray_find(data, "vnum");
    vnum = (vnumentry != NULL) ? parse_long(vnumentry) : 0;

    /** Place on list. */
    headlist_add(prototypedata);

    /** Store in hash table. */
    lookup_add(vnum, prototypedata);

    return prototypedata;
}

#define SERIALIZED_NUMBER_SIZE 32
KEYVALUEPAIR_ARRAY *objectprototype_serialize(const struct objectprototype *obj)
{
    KEYVALUEPAIR_ARRAY *answer;
    size_t keys = 25;

    keys += count_extras(obj);
    keys += count_affects(obj);

    answer = keyvaluepairarray_create(keys);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "vnum", "%ld", obj->vnum);

    keyvaluepairarray_append(answer, "name", obj->name);
    if (obj->short_descr != NULL)
        keyvaluepairarray_append(answer, "short", obj->short_descr);
    if (obj->description != NULL)
        keyvaluepairarray_append(answer, "long", obj->description);
    if (obj->material != NULL)
        keyvaluepairarray_append(answer, "material", obj->material);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "extra2", "%ld", obj->extra2_flags);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "item_type", "%d", obj->item_type);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "extra", "%ld", obj->extra_flags);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "wear", "%ld", obj->wear_flags);

    //TODO - look at olc_save.save_object for more logic
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "value1", "%u", obj->value[0]);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "value2", "%u", obj->value[1]);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "value3", "%u", obj->value[2]);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "value4", "%u", obj->value[3]);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "value5", "%u", obj->value[4]);
    // ~TODO

    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "level", "%d", obj->level);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "weight", "%d", obj->weight);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "cost", "%u", obj->cost);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "inittimer", "%d", obj->init_timer);

    //TODO - look at olc_save.save_object for more logic
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "condition", "%d", obj->condition);
    //~TODO
    
    // count is a runtime value only used to track number of GAMEOBJECT *instances from this prototype.
    //keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "count", "%d", obj->count);
    keyvaluepairarray_appendf(answer, SERIALIZED_NUMBER_SIZE, "tnl", "%d", obj->xp_tolevel);

    /** append affects */
    {
        AFFECT_DATA *affect = obj->affected;
        while (affect != NULL) {
            keyvaluepairarray_appendf(answer, 256, "affect",
                                      "%d,%d,%d,%d,%d,%ld,%ld",
                                      affect->where,
                                      affect->type,
                                      affect->level,
                                      affect->duration,
                                      affect->location,
                                      affect->modifier,
                                      affect->bitvector);

            affect = affect->next;
        }
    }

    /** append extras */
    {
        static char keybuf[MAX_INPUT_LENGTH];
        EXTRA_DESCR_DATA *desc = obj->extra_descr;
        while (desc != NULL) {
            (void)snprintf(keybuf, MAX_INPUT_LENGTH, "extra-%s", desc->keyword);
            keyvaluepairarray_append(answer, keybuf, desc->description);
            desc = desc->next;
        }
    }


    return answer;
}

void objectprototype_free(struct objectprototype *prototypedata)
{
    assert(prototypedata != NULL);
    assert(prototypedata != &head_node);

    /** Remove from hash table. */
    lookup_remove(prototypedata->vnum, prototypedata);

    /** Extract from list. */
    {
        struct objectprototype *prev = prototypedata->prev;
        struct objectprototype *next = prototypedata->next;

        assert(prev != NULL); /** because only the head node has no previous. */
        prev->next = next;
        if (next != NULL) {
            next->prev = prev;
        }
    }

    /** Clean up affects */
    if (prototypedata->affected != NULL) {
        //TODO - affects management.
        /*@dependent@*/AFFECT_DATA *paf;
        /*@dependent@*/AFFECT_DATA *paf_next;
        for (paf = prototypedata->affected; paf != NULL; paf = paf_next) {
            paf_next = paf->next;
            free_affect(paf);
        }
    }

    /** Clean up extra descriptions */
    if (prototypedata->extra_descr != NULL) {
        //TODO - extras managment.
        /*@dependent@*/EXTRA_DESCR_DATA *ed;
        /*@dependent@*/EXTRA_DESCR_DATA *ed_next;
        for (ed = prototypedata->extra_descr; ed != NULL; ed = ed_next) {
            ed_next = ed->next;
            free_extra_descr(ed);
        }
    }

    /** Clean up strings */
    {
        if (prototypedata->name != NULL) free_string(prototypedata->name);
        if (prototypedata->description != NULL) free_string(prototypedata->description);
        if (prototypedata->short_descr != NULL) free_string(prototypedata->short_descr);
    }

    free(prototypedata);
}

int objectprototype_list_count()
{
    struct objectprototype *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
        counter++;
    return counter;
}

struct objectprototype *objectprototype_iterator_start(const OBJECTPROTOTYPE_FILTER *filter)
{
    return objectprototype_iterator(&head_node, filter);
}

struct objectprototype *objectprototype_iterator(struct objectprototype *current, const OBJECTPROTOTYPE_FILTER *filter)
{
    struct objectprototype *next;

    if (current == NULL) {
        return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
        next = next->next;
    }

    return next;
}


bool passes(struct objectprototype *testee, const OBJECTPROTOTYPE_FILTER *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->name != NULL && str_cmp(filter->name, testee->name)) {
        /** name filter specified but does not match current object. */
        return false;
    }

    return true;
}

void headlist_add(/*@owned@*/struct objectprototype *entry)
{
    struct objectprototype *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

void lookup_add(long entrykey, struct objectprototype *entry)
{
    HASH_ENTRY *node;
    long hashkey;

    hashkey = HASH_KEY(entrykey);
    node = malloc(sizeof(HASH_ENTRY));
    assert(node != NULL);
    node->entry = entry;
    node->next = lookup[hashkey].next;
    lookup[hashkey].next = node;
}

void lookup_remove(long entrykey, struct objectprototype *entry)
{
    HASH_ENTRY *head;
    HASH_ENTRY *prev;
    HASH_ENTRY *next;
    long hashkey;

    hashkey = HASH_KEY(entrykey);
    prev = &lookup[hashkey];
    head = prev->next;
    while (head != NULL && head->entry->vnum == entry->vnum) {
        prev = head;
        head = head->next;
    }

    assert(head != NULL);
    next = head->next;
    free(head);

    prev->next = next;
}

size_t count_extras(const struct objectprototype *obj)
{
    size_t keys = 0;
    EXTRA_DESCR_DATA *extra = obj->extra_descr;
    while (extra != NULL) {
        keys++;
        extra = extra->next;
    }
    return keys;
}

size_t count_affects(const struct objectprototype *obj)
{
    size_t keys = 0;
    AFFECT_DATA *affect = obj->affected;
    while (affect != NULL) {
        keys++;
        affect = affect->next;
    }
    return keys;
}

