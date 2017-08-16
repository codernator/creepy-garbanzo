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

    deserialize_assign_ulong(data, prototypedata->vnum, "vnum");
    /*@-mustfreeonly@*/
    deserialize_assign_string_default(data, prototypedata->name, "name", "no name");
    deserialize_assign_string_default(data, prototypedata->short_descr, "short", "(no short description)");
    deserialize_assign_string_default(data, prototypedata->description, "long", "(no description)");
    deserialize_assign_string_default(data, prototypedata->material, "material", "(unknown)");
    /*@+mustfreeonly@*/

    deserialize_assign_int(data, prototypedata->condition, "condition");
    deserialize_assign_flag(data, prototypedata->extra2_flags, "extra2");
    deserialize_assign_flag(data, prototypedata->extra_flags, "extra");
    deserialize_assign_flag(data, prototypedata->wear_flags, "wear");
    deserialize_assign_uint(data, prototypedata->item_type, "item_type");


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
    if (obj->material != NULL)
        serialize_copy_string(answer, "material", obj->material);

    serialize_take_string(answer, "extra", flag_to_string(obj->extra_flags));
    serialize_take_string(answer, "extra2", flag_to_string(obj->extra2_flags));
    serialize_take_string(answer, "wear", flag_to_string(obj->wear_flags));

    serialize_copy_string(answer, "type", item_name_by_type(obj->item_type));
    switch (obj->item_type) {
      default:
          serialize_take_string(answer, "value1", flag_to_string((unsigned long)obj->value[0]));
          serialize_take_string(answer, "value2", flag_to_string((unsigned long)obj->value[1]));
          serialize_take_string(answer, "value3", flag_to_string((unsigned long)obj->value[2]));
          serialize_take_string(answer, "value4", flag_to_string((unsigned long)obj->value[3]));
          serialize_take_string(answer, "value5", flag_to_string((unsigned long)obj->value[4]));
          break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
          serialize_take_string(answer, "value1", long_to_string(obj->value[0]));
          serialize_take_string(answer, "value2", long_to_string(obj->value[1]));
          serialize_copy_string(answer, "value3", liq_table[obj->value[2]].liq_name);
          serialize_take_string(answer, "value4", long_to_string(obj->value[3]));
          serialize_take_string(answer, "value5", long_to_string(obj->value[4]));
          break;

      case ITEM_CONTAINER:
          serialize_take_string(answer, "value1", long_to_string(obj->value[0]));
          serialize_take_string(answer, "value2", flag_to_string((unsigned long)obj->value[1]));
          serialize_take_string(answer, "value3", long_to_string(obj->value[2]));
          serialize_take_string(answer, "value4", long_to_string(obj->value[3]));
          serialize_take_string(answer, "value5", long_to_string(obj->value[4]));
          break;

      case ITEM_WEAPON:
          serialize_copy_string(answer, "value1", weapon_name((int)obj->value[2]));
          serialize_take_string(answer, "value2", long_to_string(obj->value[1]));
          serialize_take_string(answer, "value3", long_to_string(obj->value[2]));
          serialize_copy_string(answer, "value4", attack_table[obj->value[3]].name);
          serialize_take_string(answer, "value5", flag_to_string((unsigned long)obj->value[4]));
          break;

      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
          {
              struct dynamic_skill *skills[4];
              int idx;

              for (idx = 1; idx <= 4; idx++)
                  skills[idx - 1] = resolve_skill_sn((int)obj->value[idx]);

              serialize_take_string(answer, "value1", long_to_string(obj->value[0]));
              if (skills[0] != NULL)
                  serialize_copy_string(answer, "value2", skills[0]->name);
              if (skills[1] != NULL)
                  serialize_copy_string(answer, "value3", skills[1]->name);
              if (skills[2] != NULL)
                  serialize_copy_string(answer, "value4", skills[2]->name);
              if (skills[3] != NULL)
                  serialize_copy_string(answer, "value5", skills[3]->name);
              break;
          }

      case ITEM_STAFF:
      case ITEM_WAND:
          {
              struct dynamic_skill *skill;

              skill = resolve_skill_sn((int)obj->value[3]);
              serialize_take_string(answer, "value1", long_to_string(obj->value[0]));
              serialize_take_string(answer, "value2", long_to_string(obj->value[1]));
              serialize_take_string(answer, "value3", long_to_string(obj->value[2]));
              if (skill != NULL)
                  serialize_copy_string(answer, "value4", skill->name);
              serialize_take_string(answer, "value5", long_to_string(obj->value[4]));
              break;
          }
    }

    serialize_take_string(answer, "level", int_to_string(obj->level));
    serialize_take_string(answer, "weight", int_to_string(obj->weight));
    serialize_take_string(answer, "cost", uint_to_string(obj->cost));
    serialize_take_string(answer, "inittimer", int_to_string(obj->init_timer));
    serialize_take_string(answer, "condition", int_to_string(obj->condition));

    /** append affects */
    {
        struct affect_data *affect = obj->affected;

        affect = obj->affected;
        while (affect != NULL) {
            struct array_list *serialized_affect;
            array_list_create(serialized_affect, struct key_string_pair, 8);

            serialize_take_string(serialized_affect, "where", int_to_string(affect->where));
            serialize_take_string(serialized_affect, "type", int_to_string(affect->type));
            serialize_take_string(serialized_affect, "level", int_to_string(affect->level));
            serialize_take_string(serialized_affect, "duration", int_to_string(affect->duration));
            serialize_take_string(serialized_affect, "location", int_to_string(affect->location));
            serialize_take_string(serialized_affect, "modifier", long_to_string(affect->modifier));
            serialize_take_string(serialized_affect, "bitvector", long_to_string(affect->bitvector));

            serialize_take_string(answer, "affect", database_create_stream(serialized_affect));
            kvp_free_array(serialized_affect);

            affect = affect->next;
        }
    }

    /** append extras */
    {
        struct extra_descr_data *desc = obj->extra_descr;
        while (desc != NULL) {
            struct array_list *serialized_extra;
            array_list_create(serialized_extra, struct key_string_pair, 32);

            serialize_copy_string(serialized_extra, "keyword", desc->keyword);
            serialize_copy_string(serialized_extra, "description", desc->description);

            serialize_take_string(answer, "extra", database_create_stream(serialized_extra));
            kvp_free_array(serialized_extra);

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

