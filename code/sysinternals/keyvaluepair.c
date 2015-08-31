#include "sysinternals.h"
#include <stdlib.h> //malloc, calloc, free, _Exit
#include <string.h> //memset, strlen
#include <assert.h> //assert
#include <signal.h> //raise

#ifdef S_SPLINT_S
#define _Exit exit
#endif

KEYVALUEPAIR_ARRAY *keyvaluepairarray_create(size_t numelements)
{
    KEYVALUEPAIR_ARRAY *head = malloc(sizeof(KEYVALUEPAIR_ARRAY));
    assert(head != NULL);

    head->size = numelements;
    head->top = 0;
    head->items = calloc(sizeof(KEYVALUEPAIR), numelements);
    assert(head->items != NULL);
    return head;
}

void keyvaluepairarray_append(KEYVALUEPAIR_ARRAY *array, const char *key, const char *value)
{
    size_t keylength = strlen(key);
    size_t vallength = strlen(value);
    char *akey;
    char *avalue;

    if (array->top == array->size)
    {
        if (raise(SIGABRT) != 0) {
            _Exit(EXIT_FAILURE);
        }
    }

    
    akey = calloc(keylength+1, sizeof(char));
    assert(akey != NULL);
    avalue = calloc(vallength+1, sizeof(char));
    assert(avalue != NULL);

    strncpy(akey, key, keylength);
    strncpy(avalue, value, vallength);

    array->items[array->top].key = akey;
    array->items[array->top].value = avalue;
    array->top += 1;
}

void keyvaluepairarray_free(KEYVALUEPAIR_ARRAY *array)
{
    size_t i;

    if (array == NULL)
        return;

    for (i = 0; i < array->top; i++) {
        /** These were allocated in this module, thus they are deallocated by this module. */
        free((char *)array->items[i].key);
        free((char *)array->items[i].value);
    }

    free(array->items);
    free(array);
}

