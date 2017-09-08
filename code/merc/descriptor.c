#include "merc.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/** exports */
const DESCRIPTOR_ITERATOR_FILTER descriptor_empty_filter = { .all = false, .must_playing = false, .descriptor = 0, .skip_character = NULL };


/** imports */


/** locals */
static struct descriptor_data head_node;
static bool passes(struct descriptor_data *testee, const DESCRIPTOR_ITERATOR_FILTER *filter);


/** create a new descriptor */
struct descriptor_data *descriptor_new(SOCKET descriptor)
{
    struct descriptor_data *d;

    d = malloc(sizeof(struct descriptor_data));
    assert(d != NULL);

    /** Default values */
    {
        memset(d, 0, sizeof(struct descriptor_data));
        d->descriptor = descriptor;
        d->connected = CON_GET_ANSI;
        d->outsize = 2000;
        /*@-mustfreeonly@*/ /** obviously */
        d->outbuf = calloc(d->outsize, sizeof(char));
        /*@+mustfreeonly@*/
    }


    /** Place on list. */
    {
        struct descriptor_data *headnext;

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

void descriptor_host_set(struct descriptor_data *d, const char *value) 
{
    if (d->host != NULL)
        free(d->host);
    d->host = strdup(value);
}

/** free a descriptor */
void descriptor_free(struct descriptor_data *d)
{
    assert(d != NULL);
    assert(d != &head_node);

    /** Extract from list. */
    {
        struct descriptor_data *prev = d->prev;
        struct descriptor_data *next = d->next;

        assert(prev != NULL); /** because only the head node has no previous. */
        prev->next = next;
        if (next != NULL) {
            next->prev = prev;
        }
    }

    /** Clean up strings. */
    {
        if (d->host != NULL) free(d->host);
        if (d->outbuf != NULL) free(d->outbuf);
    }

    free(d);
}

void descriptor_push_interpreter(struct descriptor *desc, INTERPRETER_FUN *fun)
{
    desc->interpreter_stack.fun[++desc->interpreter_stack.index] = fun;
}

void descriptor_pop_interpreter(struct descriptor *desc)
{
    desc->interpreter_stack.fun[desc->interpreter_stack.index--] = NULL;
}

INTERPRETER_FUN *descriptor_interpreter(struct descriptor *desc)
{
    return desc->interpreter_stack.fun[desc->interpreter_stack.index] = fun;
}


struct descriptor_data *descriptor_iterator_start(const DESCRIPTOR_ITERATOR_FILTER *filter)
{
    return descriptor_iterator(&head_node, filter);
}

struct descriptor_data *descriptor_iterator(struct descriptor_data *current, const DESCRIPTOR_ITERATOR_FILTER *filter)
{
    struct descriptor_data *next;

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
    struct descriptor_data *d;
    int counter = 0;
    for (d = head_node.next; d != NULL; d = d->next)
        counter++;
    return counter;
}



bool passes(struct descriptor_data *testee, const DESCRIPTOR_ITERATOR_FILTER *filter)
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

