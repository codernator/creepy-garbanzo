#include "merc.h"
#include "object.h"
#include "recycle.h" // for new_extra_descr in clone.
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/** exports */
const OBJECT_ITERATOR_FILTER object_empty_filter;


/** imports */
extern void free_affect(AFFECT_DATA *af);
extern void free_extra_descr(EXTRA_DESCR_DATA *ed);


/** locals */
static GAMEOBJECT head_node;
static bool passes(GAMEOBJECT *testee, const OBJECT_ITERATOR_FILTER *filter);




GAMEOBJECT *object_new(struct objectprototype *prototypedata)
{
    GAMEOBJECT *obj;

    obj = malloc(sizeof(GAMEOBJECT));
    assert(obj != NULL);

    /** Default values */
    {
	memset(obj, 0, sizeof(GAMEOBJECT));
	obj->objprototype = prototypedata;
    }

    /** Place on list. */
    {
	GAMEOBJECT *headnext;
	obj->prev = &head_node;
	headnext = head_node.next;
	if (headnext != NULL) {
	    assert(headnext->prev == &head_node);
	    headnext->prev = obj;
	}

	obj->next = headnext;
	head_node.next = obj;
    }

    return obj;
}

GAMEOBJECT *object_clone(GAMEOBJECT *parent)
{
    GAMEOBJECT *clone;

    assert(parent != NULL);
    clone = malloc(sizeof(GAMEOBJECT));
    assert(clone != NULL);


    /** Default values */
    {
	memset(clone, 0, sizeof(GAMEOBJECT));
	clone->objprototype = parent->objprototype;
    }

    /** Place on list. */
    {
	GAMEOBJECT *headnext;
	clone->prev = &head_node;
	headnext = head_node.next;
	if (headnext != NULL) {
	    assert(headnext->prev == &head_node);
	    headnext->prev = clone;
	}

	clone->next = headnext;
	head_node.next = clone;
    }

    /* start fixing the object */
    if (parent->override_name != NULL) clone->override_name = str_dup(parent->override_name);
    if (parent->short_descr != NULL) clone->short_descr = str_dup(parent->short_descr);
    if (parent->description != NULL) clone->description = str_dup(parent->description);
    if (parent->material != NULL) clone->material = str_dup(parent->material);
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->timer = parent->timer;

    {
	int i;
	for (i = 0; i < 5; i++) {
	    clone->value[i] = parent->value[i];
	}
    }

    /* affects */
    {
	AFFECT_DATA *paf;
	clone->enchanted = parent->enchanted;
	for (paf = parent->affected; paf != NULL; paf = paf->next) {
	    affect_to_obj(clone, paf);
	}
    }

    /* extended desc */
    {
	EXTRA_DESCR_DATA *ed, *ed_new;
	for (ed = parent->extra_descr; ed != NULL; ed = ed->next) {
	    ed_new = new_extra_descr();
	    /*@-mustfreeonly@*/
	    ed_new->keyword = str_dup(ed->keyword);
	    ed_new->description = str_dup(ed->description);
	    ed_new->next = clone->extra_descr;
	    clone->extra_descr = ed_new;
	    /*@+mustfreeonly@*/
	}
    }

    return clone;
}

void object_free(GAMEOBJECT *obj)
{
    assert(obj != NULL);
    assert(obj != &head_node);

    /** Extract from list. */
    {
	GAMEOBJECT *prev = obj->prev;
	GAMEOBJECT *next = obj->next;

	assert(prev != NULL); /** because only the head node has no previous. */
	prev->next = next;
	if (next != NULL) {
	    next->prev = prev;
	}
    }

    /** Clean up strings */
    {
	if (obj->override_name != NULL) free_string(obj->override_name);
	if (obj->description != NULL) free_string(obj->description);
	if (obj->short_descr != NULL) free_string(obj->short_descr);
	if (obj->owner_name != NULL) free_string(obj->owner_name);
	if (obj->material != NULL) free_string(obj->material);
    }

    /** Clean up affects. */
    if (obj->affected != NULL) {
	//TODO - affect management.
	/*@dependent@*/AFFECT_DATA *paf;
	/*@dependent@*/AFFECT_DATA *paf_next;
	for (paf = obj->affected; paf != NULL; paf = paf_next) {
	    paf_next = paf->next;
	    free_affect(paf);
	}
    }

    /** Clean up extra descriptions */
    if (obj->extra_descr != NULL) {
	//TODO - extras managment.
	/*@dependent@*/EXTRA_DESCR_DATA *ed;
	/*@dependent@*/EXTRA_DESCR_DATA *ed_next;
	for (ed = obj->extra_descr; ed != NULL; ed = ed_next) {
	    ed_next = ed->next;
	    free_extra_descr(ed);
	}
    }

    free(obj);
}

int object_list_count()
{
    GAMEOBJECT *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
	counter++;
    return counter;
}

GAMEOBJECT *object_iterator_start(const OBJECT_ITERATOR_FILTER *filter)
{
    return object_iterator(&head_node, filter);
}

GAMEOBJECT *object_iterator(GAMEOBJECT *current, const OBJECT_ITERATOR_FILTER *filter)
{
    GAMEOBJECT *next;

    if (current == NULL) {
	return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
	next = next->next;
    }

    return next;
}

inline const char *object_ownername_get(const GAMEOBJECT *object)
{
    return object->owner_name;
}

void object_ownername_set(GAMEOBJECT *object, const CHAR_DATA *owner)
{
    if (object->owner_name != NULL)
	free_string(object->owner_name);

    if (owner != NULL)
	object->owner_name = NULL;
    else
	object->owner_name = str_dup(owner->name);
}

inline const char *object_name_get(const GAMEOBJECT *object)
{
    return object->override_name == NULL ? object->objprototype->name : object->override_name;
}

void object_name_set(GAMEOBJECT *object, const char *name)
{
    if (object->override_name != NULL)
	free_string(object->override_name);

    if (name == NULL) {
	object->override_name = NULL;
    } else {
	object->override_name = str_dup(name);
    }
}


inline bool is_situpon(GAMEOBJECT *obj) {
    return (obj->item_type == ITEM_FURNITURE) && (IS_SET(obj->value[2], SIT_ON) || IS_SET(obj->value[2], SIT_IN) || IS_SET(obj->value[2], SIT_AT));
}

inline bool is_standupon(GAMEOBJECT *obj) {
    return (obj->item_type == ITEM_FURNITURE && (IS_SET(obj->value[2], STAND_AT) || IS_SET(obj->value[2], STAND_ON) || IS_SET(obj->value[2], STAND_IN)));
}



bool passes(GAMEOBJECT *testee, const OBJECT_ITERATOR_FILTER *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->override_name != NULL && str_cmp(filter->name, testee->override_name)) {
	/** name filter specified but does not match current object. */
	return false;
    }

    if (filter->object_template != NULL && filter->object_template != testee->objprototype) {
	return false;
    }

    return true;
}

