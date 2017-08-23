#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"


#ifndef ROOMTEMPLATE_MAX_KEY_HASH
// Consult http://www.planetmath.org/goodhashtableprimes for a nice writeup on numbers to use.
// Bear in mind that whatever value is used here translates to an array containing this number of elements.
#define ROOMTEMPLATE_MAX_KEY_HASH (unsigned long)3079 
#endif

#define HASH_KEY(vnum) (vnum) % ROOMTEMPLATE_MAX_KEY_HASH

/** exports */
const struct roomtemplate_filter roomtemplate_empty_filter;


/** imports */
extern void free_affect(struct affect_data *af);
extern void free_extra_descr(struct extra_descr_data *ed);


/** locals */
struct hash_entry {
    /*@only@*/struct hash_entry *next;
    /*@dependent@*/struct roomtemplate *entry;
};

static struct roomtemplate head_node;
static bool passes(struct roomtemplate *testee, const struct roomtemplate_filter *filter);
static void headlist_add(/*@owned@*/struct roomtemplate *entry);
/* an array of linked lists, with an empty head node. */
static struct hash_entry lookup[ROOMTEMPLATE_MAX_KEY_HASH];
static void lookup_add(unsigned long entrykey, /*@dependent@*/struct roomtemplate *entry);
static void lookup_remove(unsigned long entrykey, /*@dependent@*/struct roomtemplate *entry);

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
struct roomtemplate *roomtemplate_getbyvnum(unsigned long vnum)
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

struct roomtemplate *roomtemplate_new(unsigned long vnum)
{
    struct roomtemplate *templatedata;

    templatedata = malloc(sizeof(struct roomtemplate));
    assert(templatedata != NULL);

    /** Default values */
    {
        memset(templatedata, 0, sizeof(struct roomtemplate));
        templatedata->vnum = vnum;
        /*@-mustfreeonly@*/
        templatedata->name = strdup("no name");
        templatedata->description = strdup("(no description)");
        templatedata->affected = affectdata_new();
        templatedata->extra_descr = extradescrdata_new();
        /*@+mustfreeonly@*/
    }

    /** Place on list. */
    headlist_add(templatedata);

    /** Store in hash table. */
    lookup_add(vnum, templatedata);

    return templatedata;
}

struct roomtemplate *roomtemplate_clone(struct roomtemplate *source, unsigned long vnum, struct area_data *area)
{
    struct roomtemplate *clone;
    /*@observer@*/struct affect_data *affsource;
    struct affect_data *affclone;
    /*@observer@*/struct extra_descr_data *extrasource;
    struct extra_descr_data *extraclone;
    /*@observer@*/struct reset_data *resetsource;
    struct reset_data *resetclone;
    int i;

    clone = malloc(sizeof(struct roomtemplate));
    assert(clone != NULL);

    {
        memset(clone, 0, sizeof(struct roomtemplate));
        clone->vnum = vnum;
        clone->area = area;
        /*@-mustfreeonly@*/
        clone->name = strdup(source->name);
        clone->description = strdup(source->description);
        clone->extra_descr = extradescrdata_new();
        clone->affected = affectdata_new();
        /*@+mustfreeonly@*/
        clone->room_flags = source->room_flags;
        clone->light = source->light;
        clone->sector_type = source->sector_type;
        clone->heal_rate = source->heal_rate;
        clone->mana_rate = source->mana_rate;
        clone->timer = source->timer;

        for (i = 0; i < MAX_EXITS; i++) {
            if (source->exit[i] != NULL)
                clone->exit[i] = exittemplate_clone(source->exit[i], source->exit[i]->direction);
        }

        for (resetsource = source->reset_first; resetsource != NULL; resetsource = resetsource->next) {
            resetclone = resetdata_clone(resetsource);
            assert(resetclone->next == NULL);
            if (clone->reset_last == NULL)
                clone->reset_last = resetclone;
            resetclone->next = clone->reset_first;
            clone->reset_first = resetclone;
        }

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

struct roomtemplate *roomtemplate_deserialize(const struct array_list *data)
{
    struct roomtemplate *templatedata;
    const char *entry;


    templatedata = malloc(sizeof(struct roomtemplate));
    assert(templatedata != NULL);
    memset(templatedata, 0, sizeof(struct roomtemplate));
    /*@-mustfreeonly@*/
    templatedata->affected = affectdata_new();
    templatedata->extra_descr = extradescrdata_new();
    /*@+mustfreeonly@*/


    deserialize_assign_ulong(data, templatedata->vnum, "vnum");
    /*@-mustfreeonly@*/
    deserialize_assign_string_default(data, templatedata->name, "name", "(no name)");
    deserialize_assign_string_default(data, templatedata->description, "long", "(no description)");
    /*@+mustfreeonly@*/

    deserialize_assign_flag(data, templatedata->room_flags, "room_flags");
    deserialize_assign_int(data, templatedata->light, "light");
    deserialize_assign_int(data, templatedata->sector_type, "sector_type");
    deserialize_assign_int(data, templatedata->heal_rate, "heal_rate");
    deserialize_assign_int(data, templatedata->mana_rate, "mana_rate");
    deserialize_assign_int(data, templatedata->timer, "timer");


    {
        size_t index;
        struct array_list *serialized;
        struct key_string_pair *entrydata;
        array_list_each(data, index) {
            entrydata = array_list_node_at(data, struct key_string_pair, index);
            assert(entrydata != NULL);

            if (entrydata->key[0] == 'e' && !strcmp(entrydata->key, "exit")) {
                struct exit_data *element;
                serialized = database_parse_stream(entrydata->value);
                element = exittemplate_deserialize(serialized);
                if (element->direction < 0 || element->direction >= MAX_EXITS)
                    log_bug("bad exit dir: (%lu)-(%lu) x %d", templatedata->vnum, element->vnum, element->direction);
                else
                    templatedata->exit[element->direction] = element;
            }
            else if (entrydata->key[0] == 'r' && !strcmp(entrydata->key, "reset")) {
                struct reset_data *element;
                serialized = database_parse_stream(entrydata->value);
                element = resetdata_deserialize(serialized);
                assert(element->next == NULL);
                if(templatedata->reset_last == NULL)
                    templatedata->reset_last = element;
                element->next = templatedata->reset_first;
                templatedata->reset_first = element;
            } else if (entrydata->key[0] == 'a' && !strcmp(entrydata->key, "affect")) {
                struct affect_data *element;
                serialized = database_parse_stream(entrydata->value);
                element = affectdata_deserialize(serialized);
                assert(element->next == NULL);
                element->next = templatedata->affected;
                templatedata->affected = element;

                //struct affect_data af;
                //struct dynamic_skill *skill;

                //if ((skill = skill_lookup(fread_word(fp))) != NULL) {
                //   af.where = TO_AFFECTS;
                //   af.type = skill->sn;
                //   af.skill = skill;
                //   af.level = (int)fread_number(fp);
                //   af.duration = -1;
                //   af.location = 0;
                //   af.modifier = 0;
                //   af.bitvector = 0;
                //   affect_to_templatedata(room_idx, &af);
                //}
            } else if (entrydata->key[0] == 'e' && !strcmp(entrydata->key, "extradesc")) {
                struct extra_descr_data *element;
                serialized = database_parse_stream(entrydata->value);
                element = extradescrdata_deserialize(serialized);
                assert(element->next == NULL);
                element->next = templatedata->extra_descr->next;
                templatedata->extra_descr->next = element;
            }

            kvp_free_array(serialized);
        }
    }


    /** Place on list. */
    headlist_add(templatedata);

    /** Store in hash table. */
    if (templatedata->vnum != 0) {
        if (roomtemplate_getbyvnum(templatedata->vnum) != NULL)
        {
            log_bug("Deserialize Template: vnum %d duplicated.", templatedata->vnum);
        }
        else
        {
            lookup_add(templatedata->vnum, templatedata);
        }
    }

    return templatedata;
}

struct array_list *roomtemplate_serialize(const struct roomtemplate *template)
{
    struct array_list *answer;
    struct reset_data *reset;
    struct affect_data *affect;
    struct extra_descr_data *desc;
    int i;

    // TODO consider tailoring the size of this array to the data.
    answer = kvp_create_array(256);

    serialize_take_string(answer, "vnum", ulong_to_string(template->vnum));
    serialize_copy_string(answer, "name", template->name);

    if (template->description != NULL)
        serialize_copy_string(answer, "long", template->description);

    serialize_take_string(answer, "room_flags", flag_to_string(template->room_flags));

    serialize_take_string(answer, "light", int_to_string(template->light));
    serialize_take_string(answer, "sector_type", int_to_string(template->sector_type));
    serialize_take_string(answer, "heal_rate", int_to_string(template->heal_rate));
    serialize_take_string(answer, "mana_rate", int_to_string(template->mana_rate));
    serialize_take_string(answer, "timer", int_to_string(template->timer));

    for (i = 0; i < MAX_EXITS; i++) {
        struct array_list *serialized;
        if (template->exit[i] == NULL)
            continue;

        serialized = exittemplate_serialize(template->exit[i]);
        serialize_take_string(answer, "exit", database_create_stream(serialized));
        kvp_free_array(serialized);
    }

    for (reset = template->reset_first; reset != NULL; reset = reset->next) {
        struct array_list *serialized = resetdata_serialize(reset);
        serialize_take_string(answer, "reset", database_create_stream(serialized));
        kvp_free_array(serialized);
    }

    for (affect = template->affected->next; affect != NULL; affect = affect->next) {
        struct array_list *serialized = affectdata_serialize(affect);
        serialize_take_string(answer, "affect", database_create_stream(serialized));
        kvp_free_array(serialized);
    }

    for (desc = template->extra_descr->next; desc != NULL; desc = desc->next) {
        struct array_list *serialized = extradescrdata_serialize(desc);
        serialize_take_string(answer, "extra", database_create_stream(serialized));
        kvp_free_array(serialized);
    }

    return answer;
}

void roomtemplate_free(struct roomtemplate *templatedata)
{
    assert(templatedata != NULL);
    assert(templatedata != &head_node);

    /** Remove from hash table. */
    lookup_remove(templatedata->vnum, templatedata);

    /** Extract from list. */
    {
        struct roomtemplate *prev = templatedata->prev;
        struct roomtemplate *next = templatedata->next;

        assert(prev != NULL); /** because only the head node has no previous. */
        prev->next = next;
        if (next != NULL) {
            next->prev = prev;
        }
    }

    {
        int i;
        for (i = 0; i < MAX_EXITS; i++) {
            if (templatedata->exit[i] == NULL)
                continue;
            exittemplate_free(templatedata->exit[i]);
        }
    }

    /** Clean up resets. */
    {
        /*@only@*/struct reset_data *reset = templatedata->reset_first;
        /*@only@*/struct reset_data *reset_next = templatedata->reset_first;
        while(reset != NULL) {
            reset_next = reset->next;
            resetdata_free(reset);
            reset = reset_next;
        }
        templatedata->reset_last = NULL;
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

    free(templatedata);
    return;
}

void roomtemplate_applyaffect(struct roomtemplate *template, struct affect_data *affect)
{
    // remember template->affected is a HEAD node that has no real data.
    affect->next = template->affected->next;
    template->affected->next = affect;
    return;
}


bool roomtemplate_deleteaffect(struct roomtemplate *template, int index)
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

/*@dependent@*/struct extra_descr_data *roomtemplate_addextra(struct roomtemplate *template, /*@observer@*/const char *keyword, /*@observer@*/const char *description)
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

/*@dependent@*//*@null@*/struct extra_descr_data *roomtemplate_findextra(struct roomtemplate *template, /*@observer@*/const char *keyword)
{
    struct extra_descr_data *extra;

    for (extra = template->extra_descr->next; extra != NULL; extra = extra->next)
        if (is_name(keyword, extra->keyword))
            return extra;

    return NULL;
}

bool roomtemplate_deleteextra(struct roomtemplate *template, /*@observer@*/const char *keyword)
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

void roomtemplate_setname(struct roomtemplate *template, const char *name)
{
    if (template->name != NULL)
        free(template->name);
    template->name = strdup(name);
    return;
}

void roomtemplate_setlong(struct roomtemplate *template, const char *description)
{
    if (template->description != NULL)
        free(template->description);
    template->description = strdup(description);
    return;
}

int roomtemplate_list_count()
{
    struct roomtemplate *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
        counter++;
    return counter;
}

struct roomtemplate *roomtemplate_iterator_start(const struct roomtemplate_filter *filter)
{
    return roomtemplate_iterator(&head_node, filter);
}

struct roomtemplate *roomtemplate_iterator(struct roomtemplate *current, const struct roomtemplate_filter *filter)
{
    struct roomtemplate *next;

    if (current == NULL) {
        return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
        next = next->next;
    }

    return next;
}


bool passes(struct roomtemplate *testee, const struct roomtemplate_filter *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->name != NULL && str_cmp(filter->name, testee->name)) {
        /** name filter specified but does not match current room. */
        return false;
    }

    return true;
}

void headlist_add(/*@owned@*/struct roomtemplate *entry)
{
    struct roomtemplate *headnext;

    entry->prev = &head_node;
    headnext = head_node.next;
    if (headnext != NULL) {
        assert(headnext->prev == &head_node);
        headnext->prev = entry;
    }

    entry->next = headnext;
    head_node.next = entry;
}

void lookup_add(unsigned long entrykey, struct roomtemplate *entry)
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

void lookup_remove(unsigned long entrykey, struct roomtemplate *entry)
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

void olc_roomtemplate_getdescription(void *owner, char *target, size_t maxlen)
{
    struct roomtemplate *roomtemplate;
    roomtemplate = (struct roomtemplate *)owner;
    (void)strncpy(target, roomtemplate->description, maxlen);
    return;
}

void olc_roomtemplate_setdescription(void *owner, const char *text)
{
    struct roomtemplate *roomtemplate;
    roomtemplate = (struct roomtemplate *)owner;
    free_string(roomtemplate->description);
    roomtemplate->description = str_dup(text);
    return;
 }
