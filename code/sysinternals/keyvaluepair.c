#include "sysinternals.h"
#include <stdlib.h> //malloc, calloc, free, _Exit
#include <string.h> //memset, strlen
#include <assert.h> //assert
#include <signal.h> //raise
#include <stdarg.h>
#include <stdio.h> //vsprintf

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

void keyvaluepairarray_grow(KEYVALUEPAIR_ARRAY *array, size_t newSize)
{
    KEYVALUEPAIR *newitems;

    assert(newSize > array->size);
    newitems = calloc(sizeof(KEYVALUEPAIR), newSize);
    assert(newitems != NULL);
    memcpy(newitems, array->items, sizeof(KEYVALUEPAIR)*array->size);
    array->size = newSize;
    free(array->items);
    array->items = newitems;
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

void keyvaluepairarray_appendf(KEYVALUEPAIR_ARRAY *array, size_t maxlength, const char *key, const char *valueformat, ...)
{
    char buf[maxlength];
    va_list args;

    va_start(args, valueformat);
    (void)vsnprintf(buf, maxlength, valueformat, args);
    va_end(args);

    keyvaluepairarray_append(array, key, buf);
}

const char *keyvaluepairarray_find(const KEYVALUEPAIR_ARRAY *array, const char *key)
{
    size_t idx;
    size_t keylen = strlen(key);

    for(idx = 0; idx < array->top; idx++) {
	if (strncmp(key, array->items[idx].key, keylen+1) == 0)
	    return array->items[idx].value;
    }

    return NULL;
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


#define DEFAULT_NUMHASHBUCKETS 53
KEYVALUEPAIR_HASH *keyvaluepairhash_create(KEYVALUEPAIR_ARRAY *array, size_t numelements, size_t numbuckets)
{
    int idx;
    KEYVALUEPAIR_HASH *hash;
    size_t bucketsize = (size_t)UCEILING((unsigned int)numelements, (unsigned int)numbuckets) + UCEILING((unsigned int)numelements, 10);

    hash = malloc(sizeof(KEYVALUEPAIR_HASH));
    assert(hash != NULL);
    hash->numhashbuckets = (numbuckets == 0 ? DEFAULT_NUMHASHBUCKETS : numbuckets);
    hash->lookup = calloc(sizeof(KEYVALUEPAIR_HASHNODE), (size_t)hash->numhashbuckets);
    assert(hash->lookup != NULL);
    hash->masterlist = calloc(sizeof(KEYVALUEPAIR_P), bucketsize * hash->numhashbuckets);
    assert(hash->masterlist != NULL);

    for (idx = 0; idx < hash->numhashbuckets; idx++) {
	KEYVALUEPAIR_HASHNODE *hashnode = &hash->lookup[idx];
	// perfect world means numelements/numbuckets per bucket, but allow for margin of error.
	hashnode->size = bucketsize;
	hashnode->top = 0;
	hashnode->items = &hash->masterlist[idx * numelements];
	assert(hashnode->items != NULL);
    }

    for(idx = 0; idx < (int)numelements; idx++) {
	HASHBUCKETTYPE hashbucket = CALC_HASH_BUCKET(array->items[idx].key, hash->numhashbuckets);
	KEYVALUEPAIR_HASHNODE *hashnode = &hash->lookup[hashbucket];
	if (hashnode->top == hashnode->size)
	{
	    if (raise(SIGABRT) != 0) {
		_Exit(EXIT_FAILURE);
	    }
	}

	hashnode->items[hashnode->top] = &array->items[idx];
	hashnode->top++;
    }

    return hash;
}

const char *keyvaluepairhash_get(KEYVALUEPAIR_HASH *hash, const char * const key)
{
    HASHBUCKETTYPE hashbucket = CALC_HASH_BUCKET(key, hash->numhashbuckets);
    size_t keylen = strlen(key);
    KEYVALUEPAIR_HASHNODE *hashnode = &hash->lookup[hashbucket];
    size_t idx;

    for (idx = 0; idx < hashnode->top; idx++) {
	if (strncmp(key, hashnode->items[idx]->key, keylen+1) == 0) {
	    return hashnode->items[idx]->value;
	}
    }

    return NULL;
}

void keyvaluepairhash_free(KEYVALUEPAIR_HASH *hash)
{
    if (hash == NULL) return;

    free(hash->masterlist);
    free(hash->lookup);
    free(hash);
}



