#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"


#ifndef OBJECTTEMPLATE_MAX_KEY_HASH
// Consult http://www.planetmath.org/goodhashtableprimes for a nice writeup on numbers to use.
// Bear in mind that whatever value is used here translates to an array containing this number of elements.
#define OBJECTTEMPLATE_MAX_KEY_HASH (unsigned long)3079 
#endif

#define HASH_KEY(vnum) (vnum) % OBJECTTEMPLATE_MAX_KEY_HASH

/** exports */
const struct objecttemplate_filter objecttemplate_empty_filter;


/** imports */
extern void free_affect(struct affect_data *af);
extern void free_extra_descr(struct extra_descr_data *ed);


/** locals */
struct hash_entry {
    /*@only@*/struct hash_entry *next;
    /*@dependent@*/struct objecttemplate *entry;
};

static struct objecttemplate head_node;
static bool passes(struct objecttemplate *testee, const struct objecttemplate_filter *filter);
static void headlist_add(/*@owned@*/struct objecttemplate *entry);
/* an array of linked lists, with an empty head node. */
static struct hash_entry lookup[OBJECTTEMPLATE_MAX_KEY_HASH];
static void lookup_add(unsigned long entrykey, /*@dependent@*/struct objecttemplate *entry);
static void lookup_remove(unsigned long entrykey, /*@dependent@*/struct objecttemplate *entry);

/*
 * Translates object virtual number to its obj index struct.
 * Hash table lookup.
 */
struct objecttemplate *objecttemplate_getbyvnum(unsigned long vnum)
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

struct objecttemplate *objecttemplate_new(unsigned long vnum)
{
    struct objecttemplate *templatedata;

    templatedata = malloc(sizeof(struct objecttemplate));
    assert(templatedata != NULL);

    /** Default values */
    {
        memset(templatedata, 0, sizeof(struct objecttemplate));
        templatedata->vnum = vnum;
        /*@-mustfreeonly@*/
        templatedata->name = strdup("no name");
        templatedata->short_descr = strdup("(no short description)");
        templatedata->description = strdup("(no description)");
        templatedata->affected = affectdata_new();
        templatedata->extra_descr = extradescrdata_new();
        /*@+mustfreeonly@*/
        templatedata->item_type = ITEM_TRASH;
        templatedata->condition = 100;

    }

    /** Place on list. */
    headlist_add(templatedata);

    /** Store in hash table. */
    lookup_add(vnum, templatedata);

    return templatedata;
}

struct objecttemplate *objecttemplate_clone(struct objecttemplate *source, unsigned long vnum, struct area_data *area)
{
    struct objecttemplate *clone;
    /*@observer@*/struct affect_data *affsource;
    struct affect_data *affclone;
    /*@observer@*/struct extra_descr_data *extrasource;
    struct extra_descr_data *extraclone;
    int i;

    clone = malloc(sizeof(struct objecttemplate));
    assert(clone != NULL);

    {
        memset(clone, 0, sizeof(struct objecttemplate));
        clone->vnum = vnum;
        clone->area = area;
        /*@-mustfreeonly@*/
        clone->name = strdup(source->name);
        clone->short_descr = strdup(source->short_descr);
        clone->description = strdup(source->description);
        clone->extra_descr = extradescrdata_new();
        clone->affected = affectdata_new();
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

        for (extrasource = source->extra_descr->next; extrasource != NULL; extrasource = extrasource->next) {
            extraclone = extradescrdata_clone(extrasource);
            assert(extraclone->next == NULL);
            extraclone->next = clone->extra_descr->next;
            clone->extra_descr->next = extraclone;
        }

        for (affsource = source->affected->next; affsource != NULL; affsource = affsource->next) {
            affclone = affectdata_clone(affsource);
            affclone->next = clone->affected->next;
            clone->affected->next = affclone;
        }
    }

    /** Place on list. */
    headlist_add(clone);

    /** Store in hash table. */
    lookup_add(vnum, clone);

    return clone;
}

struct objecttemplate *objecttemplate_deserialize(const struct array_list *data)
{
    struct objecttemplate *templatedata;
    const char *entry;
    const char *entrydata;
    struct array_list *serialized;
    struct extra_descr_data *extra;
    struct affect_data *affect;


    templatedata = malloc(sizeof(struct objecttemplate));
    assert(templatedata != NULL);
    memset(templatedata, 0, sizeof(struct objecttemplate));
    /*@-mustfreeonly@*/
    templatedata->affected = affectdata_new();
    templatedata->extra_descr = extradescrdata_new();
    /*@+mustfreeonly@*/


    deserialize_assign_ulong(data, templatedata->vnum, "vnum");
    /*@-mustfreeonly@*/
    deserialize_assign_string_default(data, templatedata->name, "name", "no name");
    deserialize_assign_string_default(data, templatedata->short_descr, "short", "(no short description)");
    deserialize_assign_string_default(data, templatedata->description, "long", "(no description)");
    /*@+mustfreeonly@*/

    deserialize_assign_int(data, templatedata->condition, "condition");
    deserialize_assign_flag(data, templatedata->extra2_flags, "extra2");
    deserialize_assign_flag(data, templatedata->extra_flags, "extra");
    deserialize_assign_flag(data, templatedata->wear_flags, "wear");
    deserialize_assign_uint(data, templatedata->item_type, "item_type");


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

            affect->next = templatedata->affected->next;
            templatedata->affected->next = affect;
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

            extra->next = templatedata->extra_descr->next;
            templatedata->extra_descr->next = extra;
        }

        kvp_free_array(extras);
    }

    /** Place on list. */
    headlist_add(templatedata);

    /** Store in hash table. */
    if (templatedata->vnum != 0) {
        if (objecttemplate_getbyvnum(templatedata->vnum) != NULL)
        {
            log_bug("Deserialize Template: vnum %d duplicated.", templatedata->vnum);
            RABORT(templatedata);
        }
        else
        {
            lookup_add(templatedata->vnum, templatedata);
        }
    }

    return templatedata;
}

struct array_list *objecttemplate_serialize(const struct objecttemplate *obj)
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
        struct affect_data *affect;
        // TODO consider counting the number of affects to tailor the size of this array.
        struct array_list *affects = kvp_create_array(256);

        affect = obj->affected->next;
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
        struct extra_descr_data *desc = obj->extra_descr->next;
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

void objecttemplate_free(struct objecttemplate *templatedata)
{
    assert(templatedata != NULL);
    assert(templatedata != &head_node);

    /** Remove from hash table. */
    lookup_remove(templatedata->vnum, templatedata);

    /** Extract from list. */
    {
        struct objecttemplate *prev = templatedata->prev;
        struct objecttemplate *next = templatedata->next;

        assert(prev != NULL); /** because only the head node has no previous. */
        prev->next = next;
        if (next != NULL) {
            next->prev = prev;
        }
    }

    /** Clean up affects */
    {
        /*@only@*/struct affect_data *paf;
        /*@only@*/struct affect_data *paf_next;
        paf_next = templatedata->affected;
        while (paf_next != NULL)
        {
            paf = paf_next;
            paf_next = paf_next->next;
            affectdata_free(paf);
        }
        paf = NULL; // trixie on splint
        templatedata->affected = NULL;
    }

    /** Clean up extras */
    {
        /*@only@*/struct extra_descr_data *extra;
        /*@only@*/struct extra_descr_data *extra_next;
        extra_next = templatedata->extra_descr;
        while (extra_next != NULL)
            {
                extra = extra_next;
                extra_next = extra_next->next;
                extradescrdata_free(extra);
            }
        extra = NULL; // trixie on splint
        templatedata->extra_descr = NULL;
    }


    /** Clean up strings */
    if (templatedata->name != NULL) free(templatedata->name);
    if (templatedata->description != NULL) free(templatedata->description);
    if (templatedata->short_descr != NULL) free(templatedata->short_descr);

    free(templatedata);
    return;
}

void objecttemplate_applyaffect(struct objecttemplate *template, struct affect_data *affect)
{
    // remember template->affected is a HEAD node that has no real data.
    affect->next = template->affected->next;
    template->affected->next = affect;
    return;
}


bool objecttemplate_deleteaffect(struct objecttemplate *template, int index)
{
    struct affect_data *prev = NULL;
    struct affect_data *curr = NULL;
    int cnt = 0;

    assert(index >= 0);
    if (template->affected->next == NULL)
        return false;

    prev = template->affected;
    curr = template->affected->next;
    for (cnt = 0; cnt < index; cnt++) {
        prev = curr;
        curr = curr->next;
        if (curr == NULL)
            return false;
    }

    prev->next = curr->next;

    // TODO figure out how to properly annotate or rewrite the contents of this method.
    /*@-dependenttrans@*/
    /*@-usereleased@*/
    curr->next = NULL;
    affectdata_free(curr);
    return true;
    /*@+compdef@*/
    /*@+usereleased@*/
}

void objecttemplate_setname(struct objecttemplate *template, const char *name)
{
    if (template->name != NULL)
        free(template->name);
    template->name = strdup(name);
    return;
}

void objecttemplate_setshort(struct objecttemplate *template, const char *short_descr)
{
    if (template->short_descr != NULL)
        free(template->short_descr);
    template->short_descr = strdup(short_descr);
    return;
}

void objecttemplate_setlong(struct objecttemplate *template, const char *description)
{
    if (template->description != NULL)
        free(template->description);
    template->description = strdup(description);
    return;
}

struct extra_descr_data *objecttemplate_addextra(struct objecttemplate *template, const char *keyword, const char *description)
{
    struct extra_descr_data *extra;

    extra = extradescrdata_new();
    assert(extra->next == NULL);
    assert(extra->keyword == NULL);
    assert(extra->description == NULL);
    extra->keyword = strdup(keyword);
    extra->description = strdup(description);
    extra->next = template->extra_descr->next;
    template->extra_descr->next = extra;
    return extra;
}

struct extra_descr_data *objecttemplate_findextra(struct objecttemplate *template, const char *keyword)
{
    struct extra_descr_data *extra;

    for (extra = template->extra_descr->next; extra != NULL; extra = extra->next)
        if (is_name(keyword, extra->keyword))
            return extra;

    return NULL;
}

bool objecttemplate_deleteextra(struct objecttemplate *template, const char *keyword)
{
    struct extra_descr_data *prev;
    struct extra_descr_data *curr;

    if (template->extra_descr->next == NULL)
        return false;

    prev = template->extra_descr;
    curr = prev->next;
    while (curr != NULL && !is_name(keyword, curr->keyword)) {
        prev = curr;
        curr = prev->next;
    }

    if (curr == NULL)
        return false;

    assert(prev->next == curr);
    prev->next = curr->next;

    // TODO figure out how to properly annotate or rewrite the contents of this method.
    /*@-dependenttrans@*/
    /*@-usereleased@*/
    /*@-compdef@*/
    curr->next = NULL;
    extradescrdata_free(curr);
    return true;
    /*@+compdef@*/
    /*@+usereleased@*/
    /*@+compdef@*/
}

int objecttemplate_list_count()
{
    struct objecttemplate *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
        counter++;
    return counter;
}

struct objecttemplate *objecttemplate_iterator_start(const struct objecttemplate_filter *filter)
{
    return objecttemplate_iterator(&head_node, filter);
}

struct objecttemplate *objecttemplate_iterator(struct objecttemplate *current, const struct objecttemplate_filter *filter)
{
    struct objecttemplate *next;

    if (current == NULL) {
        return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
        next = next->next;
    }

    return next;
}


bool passes(struct objecttemplate *testee, const struct objecttemplate_filter *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->name != NULL && str_cmp(filter->name, testee->name)) {
        /** name filter specified but does not match current object. */
        return false;
    }

    return true;
}

void headlist_add(/*@owned@*/struct objecttemplate *entry)
{
    struct objecttemplate *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

void lookup_add(unsigned long entrykey, struct objecttemplate *entry)
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

void lookup_remove(unsigned long entrykey, struct objecttemplate *entry)
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

void olc_objecttemplate_getdescription(void *owner, char *target, size_t maxlen)
{
    struct objecttemplate *template;
    template = (struct objecttemplate *)owner;
    (void)strncpy(target, template->description, maxlen);
    return;
}

void olc_objecttemplate_setdescription(void *owner, const char *text)
{
    struct objecttemplate *template;
    template = (struct objecttemplate *)owner;
    if (template->description != NULL)
        free(template->description);
    template->description = strdup(text);
    return;
}
