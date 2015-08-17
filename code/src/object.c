#include "merc.h"

extern void free_affect(AFFECT_DATA *af);
extern void free_extra_descr(EXTRA_DESCR_DATA *ed);


static OBJ_DATA head_node;
static OBJ_DATA recycle_node;
static bool passes(OBJ_DATA *testee, const OBJECT_ITERATOR_FILTER *filter);

const OBJECT_ITERATOR_FILTER object_empty_filter;

void object_list_add(OBJ_DATA *d)
{
    d->prev = &head_node;
    if (head_node.next != NULL)
        head_node.next->prev = d;

    d->next = head_node.next;
    head_node.next = d;
}

void object_list_remove(OBJ_DATA *d)
{
    if (d->prev != NULL) {
        d->prev->next = d->next;
    }
    if (d->next != NULL) {
        d->next->prev = d->prev;
    }
    d->next = NULL;
    d->prev = NULL;
}

OBJ_DATA *new_object(void)
{
	/*@shared@*/OBJ_DATA *obj;

	if (recycle_node.next == NULL) {
		obj = alloc_perm((unsigned int)sizeof(*obj));
	} else {
		obj = recycle_node.next;
		recycle_node.next = obj->next;
	}
	VALIDATE(obj);

	return obj;
}

void free_object(OBJ_DATA *obj)
{
	/*@shared@*/AFFECT_DATA *paf;
	/*@shared@*/AFFECT_DATA *paf_next;
	/*@shared@*/EXTRA_DESCR_DATA *ed;
	/*@shared@*/EXTRA_DESCR_DATA *ed_next;

	if (!IS_VALID(obj))
		return;

    //TODO - affect management.
	for (paf = obj->affected; paf != NULL; paf = paf_next) {
		paf_next = paf->next;
		free_affect(paf);
	}
	obj->affected = NULL;

    //TODO - extras managment.
	for (ed = obj->extra_descr; ed != NULL; ed = ed_next) {
		ed_next = ed->next;
		free_extra_descr(ed);
	}
	obj->extra_descr = NULL;

	free_string(obj->name);
	free_string(obj->description);
	free_string(obj->short_descr);
	free_string(obj->owner);
	INVALIDATE(obj);

	obj->next = recycle_node.next;
	recycle_node.next = obj;
}

int object_list_count()
{
    OBJ_DATA *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
        counter++;
    return counter;
}

int object_recycle_count()
{
    OBJ_DATA *o;
    int counter = 0;
    for (o = recycle_node.next; o != NULL; o = o->next)
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


bool passes(OBJ_DATA *testee, const OBJECT_ITERATOR_FILTER *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->name != NULL && str_cmp(filter->name, testee->name)) {
        /** name filter specified but does not match current object. */
        return false;
    }

    if (filter->object_template != NULL && filter->object_template != testee->obj_idx) {
        return false;
    }

    return true;
}
