#include "merc.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/** exports */
const OBJECT_ITERATOR_FILTER object_empty_filter;


/** imports */
extern void free_affect(AFFECT_DATA *af);
extern void free_extra_descr(EXTRA_DESCR_DATA *ed);


/** locals */
static OBJ_DATA head_node;
static bool passes(OBJ_DATA *testee, const OBJECT_ITERATOR_FILTER *filter);




OBJ_DATA *new_object(OBJECTPROTOTYPE *prototypedata)
{
    OBJ_DATA *obj;

    obj = malloc(sizeof(OBJ_DATA));
    assert(obj != NULL);

    /** Default values */
    {
	memset(obj, 0, sizeof(OBJ_DATA));
	obj->objprototype = prototypedata;
    }

    /** Place on list. */
    {
	OBJ_DATA *headnext;
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

void free_object(OBJ_DATA *obj)
{
    assert(obj != NULL);
    assert(obj != &head_node);


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

    /** Clean up strings */
    {
	if (obj->name != NULL) free_string(obj->name);
	if (obj->description != NULL) free_string(obj->description);
	if (obj->short_descr != NULL) free_string(obj->short_descr);
	if (obj->owner != NULL) free_string(obj->owner);
    }

    free(obj);
}

int object_list_count()
{
    OBJ_DATA *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
	counter++;
    return counter;
}

OBJ_DATA *object_iterator_start(const OBJECT_ITERATOR_FILTER *filter)
{
    return object_iterator(&head_node, filter);
}

OBJ_DATA *object_iterator(OBJ_DATA *current, const OBJECT_ITERATOR_FILTER *filter)
{
    OBJ_DATA *next;

    if (current == NULL) {
	return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
	next = next->next;
    }

    return next;
}

inline bool is_situpon(OBJ_DATA *obj) {
    return (obj->item_type == ITEM_FURNITURE) && (IS_SET(obj->value[2], SIT_ON)
						    || IS_SET(obj->value[2], SIT_IN)
						    || IS_SET(obj->value[2], SIT_AT));
}

inline bool is_standupon(OBJ_DATA *obj) {
    return (obj->item_type == ITEM_FURNITURE && (IS_SET(obj->value[2], STAND_AT)
						    || IS_SET(obj->value[2], STAND_ON)
						    || IS_SET(obj->value[2], STAND_IN)));
}



bool passes(OBJ_DATA *testee, const OBJECT_ITERATOR_FILTER *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->name != NULL && str_cmp(filter->name, testee->name)) {
	/** name filter specified but does not match current object. */
	return false;
    }

    if (filter->object_template != NULL && filter->object_template != testee->objprototype) {
	return false;
    }

    return true;
}

