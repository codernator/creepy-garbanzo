#include "merc.h"



DESCRIPTOR_DATA *descriptor_iterator(DESCRIPTOR_DATA *current)
{
    DESCRIPTOR_DATA *next;
    if (current == NULL) {
        return NULL;
    }

    next = current->next;
    while (next != NULL && !next->pending_delete) {
        next = next->next;
    }

    return next;
}

DESCRIPTOR_DATA *descriptor_connected_iterator(DESCRIPTOR_DATA *current)
{
    DESCRIPTOR_DATA *next;
    if (current == NULL) {
        return NULL;
    }

    next = current->next;
    while (next != NULL && (!next->pending_delete || !next->connected)) {
        next = next->next;
    }

    return next;
}

