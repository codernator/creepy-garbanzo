#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "entityload.h"

#ifndef OBJPROTO_MAX_KEY_HASH
// Consult http://www.planetmath.org/goodhashtableprimes for a nice writeup on numbers to use.
// Bear in mind that whatever value is used here translates to an array containing this number of elements.
#define OBJPROTO_MAX_KEY_HASH (unsigned long)3079 
#endif

#define HASH_KEY(vnum) (vnum) % OBJPROTO_MAX_KEY_HASH

/** exports */
const OBJECTPROTOTYPE_FILTER objectprototype_empty_filter;


/** imports */
extern void free_affect(struct affect_data *af);
extern void free_extra_descr(struct extra_descr_data *ed);


/** locals */
struct hash_entry {
    /*@only@*/struct hash_entry *next;
    /*@dependent@*/struct objectprototype *entry;
};

static struct objectprototype head_node;
static bool passes(struct objectprototype *testee, const OBJECTPROTOTYPE_FILTER *filter);
static void headlist_add(/*@owned@*/struct objectprototype *entry);
/* an array of linked lists, with an empty head node. */
static struct hash_entry lookup[OBJPROTO_MAX_KEY_HASH];
static void lookup_add(unsigned long entrykey, /*@dependent@*/struct objectprototype *entry);
static void lookup_remove(unsigned long entrykey, /*@dependent@*/struct objectprototype *entry);
static size_t count_extras(const struct objectprototype *obj);
static size_t count_affects(const struct objectprototype *obj);

/*
 * Translates object virtual number to its obj index struct.
 * Hash table lookup.
 */
struct objectprototype *objectprototype_getbyvnum(unsigned long vnum)
{
    struct hash_entry *hashentry;
    unsigned long hashkey = HASH_KEY(vnum);

    for (hashentry = lookup[hashkey].next; 
         hashentry != NULL && hashentry->entry->vnum != vnum;
         hashentry = hashentry->next);

    return hashentry == NULL 
        ? NULL 
        : hashentry->entry;
}

struct objectprototype *objectprototype_new(unsigned long vnum)
{
    struct objectprototype *prototypedata;

    prototypedata = malloc(sizeof(struct objectprototype));
    assert(prototypedata != NULL);

    /** Default values */
    {
        memset(prototypedata, 0, sizeof(struct objectprototype));
        prototypedata->vnum = vnum;
        /*@-mustfreeonly@*/
        prototypedata->name = strdup("no name");
        prototypedata->short_descr = strdup("(no short description)");
        prototypedata->description = strdup("(no description)");
        prototypedata->material = strdup("unknown");
        /*@+mustfreeonly@*/
        prototypedata->item_type = ITEM_TRASH;
        prototypedata->condition = 100;
    }

    /** Place on list. */
    headlist_add(prototypedata);

    /** Store in hash table. */
    lookup_add(vnum, prototypedata);

    return prototypedata;
}

struct objectprototype *objectprototype_deserialize(const struct array_list *data)
{
    struct objectprototype *prototypedata;
    const char *entry;
    

    prototypedata = malloc(sizeof(struct objectprototype));
    assert(prototypedata != NULL);
    memset(prototypedata, 0, sizeof(struct objectprototype));

    ASSIGN_ULONG_KEY(data, prototypedata->vnum, "vnum");
    /*@-mustfreeonly@*/
    ASSIGN_STRING_KEY_DEFAULT(data, prototypedata->name, "name", "no name");
    ASSIGN_STRING_KEY_DEFAULT(data, prototypedata->short_descr, "short", "(no short description)");
    ASSIGN_STRING_KEY_DEFAULT(data, prototypedata->description, "long", "(no description)");
    ASSIGN_STRING_KEY_DEFAULT(data, prototypedata->material, "material", "(unknown)");
    /*@+mustfreeonly@*/

    ASSIGN_INT_KEY(data, prototypedata->condition, "condition");
    ASSIGN_FLAG_KEY(data, prototypedata->extra2_flags, "extra2");
    ASSIGN_FLAG_KEY(data, prototypedata->extra_flags, "extra");
    ASSIGN_FLAG_KEY(data, prototypedata->wear_flags, "wear");
    ASSIGN_UINT_KEY(data, prototypedata->item_type, "item_type");


    /** Place on list. */
    headlist_add(prototypedata);

    /** Store in hash table. */
    if (prototypedata->vnum != 0) {
        lookup_add(prototypedata->vnum, prototypedata);
    }

    return prototypedata;
}

struct array_list *objectprototype_serialize(const struct objectprototype *obj)
{
    struct array_list *answer;
    size_t keys = 25;

    keys += count_extras(obj);
    keys += count_affects(obj);

    answer = kvp_create_array(keys);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "vnum", "%ld", obj->vnum);

    kvp_array_append_copy(answer, "name", obj->name);
    if (obj->short_descr != NULL)
        kvp_array_append_copy(answer, "short", obj->short_descr);
    if (obj->description != NULL)
        kvp_array_append_copy(answer, "long", obj->description);
    if (obj->material != NULL)
        kvp_array_append_copy(answer, "material", obj->material);

    SERIALIZE_FLAGS(obj->extra2_flags, "extra2", answer);
    SERIALIZE_FLAGS(obj->extra_flags, "extra", answer);
    SERIALIZE_FLAGS(obj->wear_flags, "wear", answer);

    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "type", "%u", obj->item_type);

    //TODO - look at olc_save.save_object for more logic
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "value1", "%ld", obj->value[0]);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "value2", "%ld", obj->value[1]);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "value3", "%ld", obj->value[2]);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "value4", "%ld", obj->value[3]);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "value5", "%ld", obj->value[4]);
    // ~TODO

    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "level", "%d", obj->level);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "weight", "%d", obj->weight);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "cost", "%u", obj->cost);
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "inittimer", "%d", obj->init_timer);

    //TODO - look at olc_save.save_object for more logic
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "condition", "%d", obj->condition);
    //~TODO
    
    kvp_array_append_copyf(answer, SERIALIZED_NUMBER_SIZE, "tnl", "%d", obj->xp_tolevel);

    /** append affects */
    {
        struct affect_data *affect = obj->affected;
        while (affect != NULL) {
            kvp_array_append_copyf(answer, 256, "affect",
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
        struct extra_descr_data *desc = obj->extra_descr;
        while (desc != NULL) {
            (void)snprintf(keybuf, MAX_INPUT_LENGTH, "extra-%s", desc->keyword);
            kvp_array_append_copy(answer, keybuf, desc->description);
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
        /*@dependent@*/struct affect_data *paf;
        /*@dependent@*/struct affect_data *paf_next;
        for (paf = prototypedata->affected; paf != NULL; paf = paf_next) {
            paf_next = paf->next;
            free_affect(paf);
        }
    }

    /** Clean up extra descriptions */
    if (prototypedata->extra_descr != NULL) {
        //TODO - extras managment.
        /*@dependent@*/struct extra_descr_data *ed;
        /*@dependent@*/struct extra_descr_data *ed_next;
        for (ed = prototypedata->extra_descr; ed != NULL; ed = ed_next) {
            ed_next = ed->next;
            free_extra_descr(ed);
        }
    }

    /** Clean up strings */
    if (prototypedata->name != NULL) free(prototypedata->name);
    if (prototypedata->description != NULL) free(prototypedata->description);
    if (prototypedata->short_descr != NULL) free(prototypedata->short_descr);
    if (prototypedata->material != NULL) free(prototypedata->material);

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

void lookup_add(unsigned long entrykey, struct objectprototype *entry)
{
    struct hash_entry *node;
    unsigned long hashkey;

    hashkey = HASH_KEY(entrykey);
    node = malloc(sizeof(struct hash_entry));
    assert(node != NULL);
    node->entry = entry;
    node->next = lookup[hashkey].next;
    lookup[hashkey].next = node;
}

void lookup_remove(unsigned long entrykey, struct objectprototype *entry)
{
    struct hash_entry *head;
    struct hash_entry *prev;
    struct hash_entry *next;
    unsigned long hashkey;

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
    struct extra_descr_data *extra = obj->extra_descr;
    while (extra != NULL) {
        keys++;
        extra = extra->next;
    }
    return keys;
}

size_t count_affects(const struct objectprototype *obj)
{
    size_t keys = 0;
    struct affect_data *affect = obj->affected;
    while (affect != NULL) {
        keys++;
        affect = affect->next;
    }
    return keys;
}

