#include "merc.h"

extern void free_affect(AFFECT_DATA *af);
extern void free_extra_descr(EXTRA_DESCR_DATA *ed);


static OBJECTPROTOTYPE head_node;
static OBJECTPROTOTYPE recycle_node;
static bool passes(OBJECTPROTOTYPE *testee, const OBJECTPROTOTYPE_FILTER *filter);

//static OBJECTPROTOTYPE *objectprototype_hash[MAX_KEY_HASH];


const OBJECTPROTOTYPE_FILTER objectprototype_empty_filter;

/*
 * Translates object virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJECTPROTOTYPE *objectprototype_getbyvnum(long vnum)
{
    //OBJECTPROTOTYPE *obj_idx;

    //for (obj_idx = obj_index_hash[vnum % MAX_KEY_HASH]; obj_idx != NULL; obj_idx = obj_idx->next)
	//if (obj_idx->vnum == vnum)
	    //return obj_idx;

    OBJECTPROTOTYPE *current;
    OBJECTPROTOTYPE *pending;

    pending = objectprototype_iterator_start(&objectprototype_empty_filter);
    while ((current = pending) != NULL) {
	pending = objectprototype_iterator(current, &objectprototype_empty_filter);
	if (current->vnum == vnum)
	    return current;
    }
    return NULL;
}

void objectprototype_list_add(OBJECTPROTOTYPE *prototypedata)
{
    //long hashkey;

    prototypedata->prev = &head_node;
    if (head_node.next != NULL)
	head_node.next->prev = prototypedata;

    prototypedata->next = head_node.next;
    head_node.next = prototypedata;

    //hashkey = prototypedata->vnum % MAX_KEY_HASH;
    //obj_idx->next = obj_index_hash[hash_idx];
    //objectprototype_hash[hash_idx] = obj_idx;
}

void objectprototype_list_remove(OBJECTPROTOTYPE *prototypedata)
{
    if (prototypedata->prev != NULL) {
	prototypedata->prev->next = prototypedata->next;
    }
    if (prototypedata->next != NULL) {
	prototypedata->next->prev = prototypedata->prev;
    }
    prototypedata->next = NULL;
    prototypedata->prev = NULL;
}

OBJECTPROTOTYPE *new_objectprototype(void)
{
    /*@shared@*/OBJECTPROTOTYPE *objectprototype;

    if (recycle_node.next == NULL) {
	objectprototype = alloc_perm((unsigned int)sizeof(*objectprototype));
    } else {
	objectprototype = recycle_node.next;
	recycle_node.next = objectprototype->next;
    }
    VALIDATE(objectprototype);

    objectprototype->next = NULL;
    objectprototype->extra_descr = NULL;
    objectprototype->affected = NULL;
    objectprototype->area = NULL;
    objectprototype->name = str_dup("no name");
    objectprototype->short_descr = str_dup("(no short description)");
    objectprototype->description = str_dup("(no description)");
    objectprototype->vnum = 0;
    objectprototype->item_type = ITEM_TRASH;
    objectprototype->extra_flags = 0;
    objectprototype->extra2_flags = 0;
    objectprototype->wear_flags = 0;
    objectprototype->count = 0;
    objectprototype->weight = 0;
    objectprototype->cost = 0;
    objectprototype->material = str_dup("unknown");             /* ROM */
    objectprototype->condition = 100;                           /* ROM */

    {
	int value;
	for (value = 0; value < 5; value++)             /* 5 - ROM */
	    objectprototype->value[value] = 0;
    }

    return objectprototype;
}

void free_objectprototype(OBJECTPROTOTYPE *objectprototype)
{
    /*@shared@*/AFFECT_DATA *paf;
    /*@shared@*/AFFECT_DATA *paf_next;
    /*@shared@*/EXTRA_DESCR_DATA *ed;
    /*@shared@*/EXTRA_DESCR_DATA *ed_next;

    if (!IS_VALID(objectprototype))
	return;

    //TODO - affects management.
    for (paf = objectprototype->affected; paf != NULL; paf = paf_next) {
	paf_next = paf->next;
	free_affect(paf);
    }

    //TODO - extras managment.
    for (ed = objectprototype->extra_descr; ed != NULL; ed = ed_next) {
	ed_next = ed->next;
	free_extra_descr(ed);
    }
    objectprototype->extra_descr = NULL;

    free_string(objectprototype->name);
    free_string(objectprototype->description);
    free_string(objectprototype->short_descr);
    INVALIDATE(objectprototype);

    objectprototype->next = recycle_node.next;
    recycle_node.next = objectprototype;
}

int objectprototype_list_count()
{
    OBJECTPROTOTYPE *o;
    int counter = 0;
    for (o = head_node.next; o != NULL; o = o->next)
	counter++;
    return counter;
}

int objectprototype_recycle_count()
{
    OBJECTPROTOTYPE *o;
    int counter = 0;
    for (o = recycle_node.next; o != NULL; o = o->next)
	counter++;
    return counter;
}

OBJECTPROTOTYPE *objectprototype_iterator_start(const OBJECTPROTOTYPE_FILTER *filter)
{
    return objectprototype_iterator(&head_node, filter);
}

OBJECTPROTOTYPE *objectprototype_iterator(OBJECTPROTOTYPE *current, const OBJECTPROTOTYPE_FILTER *filter)
{
    OBJECTPROTOTYPE *next;

    if (current == NULL) {
	return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
	next = next->next;
    }

    return next;
}


bool passes(OBJECTPROTOTYPE *testee, const OBJECTPROTOTYPE_FILTER *filter)
{
    if (filter->name != NULL && filter->name[0] != '\0' && testee->name != NULL && str_cmp(filter->name, testee->name)) {
	/** name filter specified but does not match current object. */
	return false;
    }

    return true;
}
