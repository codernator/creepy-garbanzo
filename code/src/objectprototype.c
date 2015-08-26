#include "merc.h"
#include <assert.h>



/** exports */


/** imports */
extern void free_affect(AFFECT_DATA *af);
extern void free_extra_descr(EXTRA_DESCR_DATA *ed);


/** locals */
static OBJECTPROTOTYPE head_node;
static OBJECTPROTOTYPE recycle_node;
static bool passes(OBJECTPROTOTYPE *testee, const OBJECTPROTOTYPE_FILTER *filter);
static void allocate_more(void);


//static OBJECTPROTOTYPE *objectprototype_hash[MAX_KEY_HASH];


const OBJECTPROTOTYPE_FILTER objectprototype_empty_filter;

/*
 * Translates object virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJECTPROTOTYPE *objectprototype_getbyvnum(long vnum)
{
    //OBJECTPROTOTYPE *objprototype;

    //for (objprototype = obj_index_hash[vnum % MAX_KEY_HASH]; objprototype != NULL; objprototype = objprototype->next)
	//if (objprototype->vnum == vnum)
	    //return objprototype;

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

OBJECTPROTOTYPE *new_objectprototype(long vnum)
{
    OBJECTPROTOTYPE *prototypedata;

    allocate_more();

    prototypedata = recycle_node.next;
    assert(prototypedata != NULL);

    VALIDATE(prototypedata);
    /** Default values */
    {
	prototypedata->vnum = vnum;
	prototypedata->extra_descr = NULL;
	prototypedata->affected = NULL;
	prototypedata->area = NULL;
	prototypedata->name = str_dup("no name");
	prototypedata->short_descr = str_dup("(no short description)");
	prototypedata->description = str_dup("(no description)");
	prototypedata->item_type = ITEM_TRASH;
	prototypedata->extra_flags = 0;
	prototypedata->extra2_flags = 0;
	prototypedata->wear_flags = 0;
	prototypedata->count = 0;
	prototypedata->weight = 0;
	prototypedata->cost = 0;
	prototypedata->material = str_dup("unknown");             /* ROM */
	prototypedata->condition = 100;                           /* ROM */

	{
	    int value;
	    for (value = 0; value < 5; value++)             /* 5 - ROM */
		prototypedata->value[value] = 0;
	}
    }

    /** Swap lists. */
    {
	OBJECTPROTOTYPE *headnext;

	assert(recycle_node.next == prototypedata);
	recycle_node.next = prototypedata->next;

	prototypedata->prev = &head_node;
	headnext = head_node.next;
	if (headnext != NULL) {
	    assert(headnext->prev == &head_node);
	    headnext->prev = prototypedata;
	}

	prototypedata->next = headnext;
	head_node.next = prototypedata;
    }

    /** Store in hash table. */
    {
	//long hashkey;
	//hashkey = prototypedata->vnum % MAX_KEY_HASH;
	//objprototype->next = obj_index_hash[hash_idx];
	//objectprototype_hash[hash_idx] = objprototype;
    }

    return prototypedata;
}

OBJECTPROTOTYPE *free_objectprototype(OBJECTPROTOTYPE *prototypedata)
{
    assert(prototypedata != NULL);
    assert(prototypedata != &head_node);
    assert(IS_VALID(prototypedata));

    /** Move to the recycle list */
    { 
	OBJECTPROTOTYPE *prev;

	prev = prototypedata->prev;

	/** Assertion - only the head node has a NULL previous. */
	assert(prev != NULL);
	assert(prev->next == prototypedata);
	/*@-mustfreeonly@*///** due to assertion prev->next == prototypedata */
	prev->next = prototypedata->next;
	/*@+mustfreeonly@*/

	if (prototypedata->next != NULL) {
	    assert(prototypedata->next->prev == prototypedata);
	    prototypedata->next->prev = prev;
	}

	prototypedata->next = recycle_node.next;
	prototypedata->prev = NULL; /* recycle list is a stack structure (LIFO), so no need for prev. */
	prototypedata->next_hash = NULL;
	prototypedata->prev_hash = NULL;
	prototypedata->area = NULL;
	recycle_node.next = prototypedata;
	
	if (prototypedata->prev != NULL) {
	    prototypedata->prev->next = prototypedata->next;
	}
	if (prototypedata->next != NULL) {
	    prototypedata->next->prev = prototypedata->prev;
	}
	prototypedata->next = NULL;
	prototypedata->prev = NULL;
    }

    INVALIDATE(prototypedata);

    /** Clean up affects */
    if (prototypedata->affected != NULL) {
	//TODO - affects management.
	/*@dependent@*/AFFECT_DATA *paf;
	/*@dependent@*/AFFECT_DATA *paf_next;
	for (paf = prototypedata->affected; paf != NULL; paf = paf_next) {
	    paf_next = paf->next;
	    free_affect(paf);
	}
	prototypedata->affected = NULL;
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
	prototypedata->extra_descr = NULL;
    }

    /** Clean up strings */
    {
	if (prototypedata->name != NULL) free_string(prototypedata->name);
	if (prototypedata->description != NULL) free_string(prototypedata->description);
	if (prototypedata->short_descr != NULL) free_string(prototypedata->short_descr);
    }

    return NULL;
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

void allocate_more() 
{
    if (recycle_node.next == NULL) {
	recycle_node.next = alloc_perm((unsigned int)sizeof(recycle_node));
    }
}

