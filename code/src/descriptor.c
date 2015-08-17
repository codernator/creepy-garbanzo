#include "merc.h"

/* exportable */
const DESCRIPTOR_ITERATOR_FILTER descriptor_empty_filter = { .all = false, .must_playing = false, .descriptor = 0, .skip_character = NULL };


static DESCRIPTOR_DATA head_node;
static DESCRIPTOR_DATA recycle_head_node;
static bool passes(DESCRIPTOR_DATA *testee, const DESCRIPTOR_ITERATOR_FILTER *filter);


void descriptor_list_add(DESCRIPTOR_DATA *d) 
{
    d->prev = &head_node;
    if (head_node.next != NULL)
        head_node.next->prev = d;

    d->next = head_node.next;
    head_node.next = d;
}

void descriptor_list_remove(DESCRIPTOR_DATA *d) 
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


int descriptor_recycle_count()
{
    DESCRIPTOR_DATA *d;
    int counter = 0;
	for (d = recycle_head_node.next; d != NULL; d = d->next)
		counter++;
    return counter;
}


/**
 * create a new descriptor
 */
DESCRIPTOR_DATA *new_descriptor(void)
{
	DESCRIPTOR_DATA *d;

	if (recycle_head_node.next == NULL) {
		d = alloc_perm((unsigned int)sizeof(*d));
	} else {
		d = recycle_head_node.next;
		recycle_head_node.next= d->next;
	}

	VALIDATE(d);

	d->connected = CON_GET_ANSI;
	d->outsize = 2000;
	d->outbuf = alloc_mem((unsigned int)d->outsize);

	return d;
}

/**
 * free a descriptor
 */
void free_descriptor(DESCRIPTOR_DATA *d)
{
	if (!IS_VALID(d))
		return;

	free_string(d->host);
	free_mem(d->outbuf, (unsigned int)d->outsize);
	INVALIDATE(d);
	d->next = recycle_head_node.next;
	recycle_head_node.next = d;
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

