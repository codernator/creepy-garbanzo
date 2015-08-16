#include "merc.h"

/* exportable */
const DESCRIPTOR_ITERATOR_FILTER descriptor_empty_filter = { .all = false };


static const DESCRIPTOR_DATA d_zero;
static DESCRIPTOR_DATA d_head;
static DESCRIPTOR_DATA *list_head = &d_head;
static bool passes(DESCRIPTOR_DATA *testee, const DESCRIPTOR_ITERATOR_FILTER *filter);


void descriptor_list_add(DESCRIPTOR_DATA *d) 
{
    d->next = list_head->next;
    list_head->next = d;
}

void descriptor_list_remove(DESCRIPTOR_DATA *d) 
{
    DESCRIPTOR_DATA *prev;
    prev = list_head;
    while (prev->next != NULL && prev->next != d)
        prev = prev->next;
    if (prev) {
        prev->next = d->next;
        d->next = NULL;
    }
}

DESCRIPTOR_DATA *descriptor_iterator_start(const DESCRIPTOR_ITERATOR_FILTER *filter)
{
    return descriptor_iterator(list_head, filter);
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
    int count = 0;

    for (d = list_head->next; d != NULL; d = d->next)
        count++;
    return count;
}


static DESCRIPTOR_DATA *descriptor_free;

int descriptor_recycle_count()
{
    DESCRIPTOR_DATA *d;
    int counter = 0;

	for (d = descriptor_free; d != NULL; d = d->next)
		counter++;

    return counter;
}


/**
 * create a new descriptor
 */
DESCRIPTOR_DATA *new_descriptor(void)
{
	DESCRIPTOR_DATA *d;

	if (descriptor_free == NULL) {
		d = alloc_perm((unsigned int)sizeof(*d));
	} else {
		d = descriptor_free;

		descriptor_free = descriptor_free->next;
	}

	*d = d_zero;
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
	d->next = descriptor_free;
	descriptor_free = d;
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

