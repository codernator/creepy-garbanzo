#include "merc.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/** exports */
const DESCRIPTOR_ITERATOR_FILTER descriptor_empty_filter = { .all = false, .must_playing = false, .descriptor = 0, .skip_character = NULL };


/** imports */


/** locals */
static DESCRIPTOR_DATA head_node;
static bool passes(DESCRIPTOR_DATA *testee, const DESCRIPTOR_ITERATOR_FILTER *filter);


/** create a new descriptor */
DESCRIPTOR_DATA *new_descriptor(SOCKET descriptor)
{
    DESCRIPTOR_DATA *d;

    d = malloc(sizeof(DESCRIPTOR_DATA));
    assert(d != NULL);

    /** Default values */
    {
	memset(d, 0, sizeof(DESCRIPTOR_DATA));
	d->descriptor = descriptor;
	d->connected = CON_GET_ANSI;
	d->outsize = 2000;
	/*@-mustfreeonly@*/ /** obviously */
	d->outbuf = calloc(d->outsize, sizeof(char));
	/*@+mustfreeonly@*/
    }


    /** Place on list. */
    {
	DESCRIPTOR_DATA *headnext;

	d->prev = &head_node;
	headnext = head_node.next;
	if (headnext != NULL) {
	    assert(headnext->prev == &head_node);
	    headnext->prev = d;
	}

	d->next = headnext;
	head_node.next = d;
    }

    return d;
}

/** free a descriptor */
void free_descriptor(DESCRIPTOR_DATA *d)
{
    assert(d != NULL);
    assert(d != &head_node);

    /** Extract from list. */
    {
	DESCRIPTOR_DATA *prev = d->prev;
	DESCRIPTOR_DATA *next = d->next;

	assert(prev != NULL); /** because only the head node has no previous. */
	prev->next = next;
	if (next != NULL) {
	    next->prev = prev;
	}
    }

    /** Clean up strings. */
    {
	if (d->host != NULL) free_string(d->host);
	if (d->outbuf != NULL) free(d->outbuf);
    }

    free(d);
}

DESCRIPTOR_DATA *descriptor_iterator_start(const DESCRIPTOR_ITERATOR_FILTER *filter)
{
    return descriptor_iterator(&head_node, filter);
}

DESCRIPTOR_DATA *descriptor_iterator(DESCRIPTOR_DATA *current, const DESCRIPTOR_ITERATOR_FILTER *filter)
{
    DESCRIPTOR_DATA *next;

    if (current == NULL) {
	return NULL;
    }

    next = current->next;
    while (next != NULL && !passes(next, filter)) {
	next = next->next;
    }

    return next;
}

int descriptor_list_count()
{
    DESCRIPTOR_DATA *d;
    int counter = 0;
    for (d = head_node.next; d != NULL; d = d->next)
	counter++;
    return counter;
}



bool passes(DESCRIPTOR_DATA *testee, const DESCRIPTOR_ITERATOR_FILTER *filter)
{
    if (filter->all) 
	return true;

    /** any item flagged for deletion is invalid for any filter option other than "all" */
    if (testee->pending_delete)
	return false;

    if (filter->descriptor > 0 && testee->descriptor != filter->descriptor)
	return false;

    if (filter->must_playing && (testee->character == NULL || testee->connected != CON_PLAYING))
	return false;

    if (filter->skip_character != NULL && filter->skip_character == testee->character)
	return false;

    return true;
}

