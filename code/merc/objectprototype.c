#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"


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

struct objectprototype *objectprototype_clone(struct objectprototype *source, unsigned long vnum, struct area_data *area)
{
    struct objectprototype *clone;
    /*@observer@*/struct affect_data *affsource;
    struct affect_data *affclone;
    /*@observer@*/struct extra_descr_data *extrasource;
    struct extra_descr_data *extraclone;
    int i;

    clone = malloc(sizeof(struct objectprototype));
    assert(clone != NULL);

    {
        memset(clone, 0, sizeof(struct objectprototype));
        clone->vnum = vnum;
        clone->area = area;
        /*@-mustfreeonly@*/
        clone->name = strdup(source->name);
        clone->short_descr = strdup(source->short_descr);
        clone->description = strdup(source->description);
        /*@+mustfreeonly@*/
        clone->item_type = source->item_type;
        clone->extra_flags = source->extra_flags;
        clone->extra2_flags = source->extra2_flags;
        clone->wear_flags = source->wear_flags;
        clone->init_timer = source->init_timer;
        clone->condition = source->condition;
        clone->weight = source->weight;
        clone->cost = source->cost;

        for (i = 0; i < 5; i++)
            clone->value[i] = source->value[i];

        for (extrasource = source->extra_descr; extrasource != NULL; extrasource = extrasource->next) {
            extraclone = extradescrdata_clone(extrasource);
            assert(extraclone->next == NULL);
            extraclone->next = clone->extra_descr;
            clone->extra_descr = extraclone;
        }

        for (affsource = source->affected; affsource != NULL; affsource = affsource->next) {
            affclone = affectdata_clone(affsource);
            affclone->next = clone->affected;
            clone->affected = affclone;
        }
    }

    /** Place on list. */
    headlist_add(clone);

    /** Store in hash table. */
    lookup_add(vnum, clone);

    return clone;
}

struct objectprototype *objectprototype_deserialize(const struct array_list *data)
{
    struct objectprototype *prototypedata;
    const char *entry;
    const char *entrydata;
    struct array_list *serialized;
    struct extra_descr_data *extra;
    struct affect_data *affect;


    prototypedata = malloc(sizeof(struct objectprototype));
    assert(prototypedata != NULL);
    memset(prototypedata, 0, sizeof(struct objectprototype));

    deserialize_assign_ulong(data, prototypedata->vnum, "vnum");
    /*@-mustfreeonly@*/
    deserialize_assign_string_default(data, prototypedata->name, "name", "no name");
    deserialize_assign_string_default(data, prototypedata->short_descr, "short", "(no short description)");
    deserialize_assign_string_default(data, prototypedata->description, "long", "(no description)");
    /*@+mustfreeonly@*/

    deserialize_assign_int(data, prototypedata->condition, "condition");
    deserialize_assign_flag(data, prototypedata->extra2_flags, "extra2");
    deserialize_assign_flag(data, prototypedata->extra_flags, "extra");
    deserialize_assign_flag(data, prototypedata->wear_flags, "wear");
    deserialize_assign_uint(data, prototypedata->item_type, "item_type");


    entry = kvp_array_find(data, "affects");
    if (entry != NULL) {
        struct array_list *affects = database_parse_stream(entry);
        size_t i;

        for (i = 0; i < affects->top; i++) {

            entrydata = kvp_array_valueat(affects, i);
            assert(entrydata != NULL);
            serialized = database_parse_stream(entrydata);
            affect = affectdata_deserialize(serialized);
            kvp_free_array(serialized);
            assert(affect->next == NULL);

            affect->next = prototypedata->affected;
            prototypedata->affected = affect;
        }

        kvp_free_array(affects);
    }

    entry = kvp_array_find(data, "extras");
    if (entry != NULL) {
        struct array_list *extras = database_parse_stream(entry);
        size_t i;

        for (i = 0; i < extras->top; i++) {
            entrydata = kvp_array_valueat(extras, i);
            assert(entrydata != NULL);
            serialized = database_parse_stream(entrydata);
            extra = extradescrdata_deserialize(serialized);
            kvp_free_array(serialized);
            assert(extra->next == NULL);

            extra->next = prototypedata->extra_descr;
            prototypedata->extra_descr = extra;
        }

        kvp_free_array(extras);
    }

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

    answer = kvp_create_array(20);

    serialize_take_string(answer, "vnum", ulong_to_string(obj->vnum));
    serialize_copy_string(answer, "name", obj->name);

    if (obj->short_descr != NULL)
        serialize_copy_string(answer, "short", obj->short_descr);
    if (obj->description != NULL)
        serialize_copy_string(answer, "long", obj->description);

    serialize_take_string(answer, "extra", flag_to_string(obj->extra_flags));
    serialize_take_string(answer, "extra2", flag_to_string(obj->extra2_flags));
    serialize_take_string(answer, "wear", flag_to_string(obj->wear_flags));

    serialize_copy_string(answer, "type", item_name_by_type(obj->item_type));
    serialize_take_string(answer, "value1", flag_to_string((unsigned long)obj->value[0]));
    serialize_take_string(answer, "value2", flag_to_string((unsigned long)obj->value[1]));
    serialize_take_string(answer, "value3", flag_to_string((unsigned long)obj->value[2]));
    serialize_take_string(answer, "value4", flag_to_string((unsigned long)obj->value[3]));
    serialize_take_string(answer, "value5", flag_to_string((unsigned long)obj->value[4]));
    serialize_take_string(answer, "weight", int_to_string(obj->weight));
    serialize_take_string(answer, "cost", uint_to_string(obj->cost));
    serialize_take_string(answer, "inittimer", int_to_string(obj->init_timer));
    serialize_take_string(answer, "condition", int_to_string(obj->condition));

    {
        struct affect_data *affect = obj->affected;
        // TODO consider counting the number of affects to tailor the size of this array.
        struct array_list *affects = kvp_create_array(256);

        affect = obj->affected;
        while (affect != NULL) {
            struct array_list *serialized_affect;
            serialized_affect = affectdata_serialize(affect);
            serialize_take_string(affects, "affect", database_create_stream(serialized_affect));
            kvp_free_array(serialized_affect);

            affect = affect->next;
        }

        serialize_take_string(answer, "affects", database_create_stream(affects));
        kvp_free_array(affects);
    }

    {
        struct extra_descr_data *desc = obj->extra_descr;
        struct array_list *extras = kvp_create_array(256);
        while (desc != NULL) {
            struct array_list *serialized_extra;
            serialized_extra = extradescrdata_serialize(desc);
            serialize_take_string(extras, "extra", database_create_stream(serialized_extra));
            kvp_free_array(serialized_extra);

            desc = desc->next;
        }

        serialize_take_string(answer, "extras", database_create_stream(extras));
        kvp_free_array(extras);
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
        /*@only@*/struct affect_data *paf;
        /*@only@*/struct affect_data *paf_next;
        paf_next = prototypedata->affected;
        while (paf_next != NULL)
        {
            paf = paf_next;
            paf_next = paf_next->next;
            affectdata_free(paf);
        }
        paf = NULL; // trixie on splint
        prototypedata->affected = NULL;
    }

    /** Clean up extras */
    if (prototypedata->affected != NULL) {
        /*@only@*/struct extra_descr_data *extra;
        /*@only@*/struct extra_descr_data *extra_next;
        extra_next = prototypedata->extra_descr;
        while (extra_next != NULL)
            {
                extra = extra_next;
                extra_next = extra_next->next;
                extradescrdata_free(extra);
            }
        extra = NULL; // trixie on splint
        prototypedata->extra_descr = NULL;
    }


    /** Clean up strings */
    if (prototypedata->name != NULL) free(prototypedata->name);
    if (prototypedata->description != NULL) free(prototypedata->description);
    if (prototypedata->short_descr != NULL) free(prototypedata->short_descr);

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

