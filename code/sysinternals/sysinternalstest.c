#include "sysinternals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


static void test_keyvaluepairarray();
static void test_keyvaluepairhash();

int main(/*@unused@*/int argc, /*@unused@*/char **argv)
{
    test_keyvaluepairarray();
    test_keyvaluepairhash();
    return EXIT_SUCCESS;
}


void test_keyvaluepairarray()
{
    KEYVALUEPAIR_ARRAY *subject;

    printf("%s\n", "KVP array test - assert no error.");
    subject = keyvaluepairarray_create(2);
    keyvaluepairarray_append(subject, "verb", "hello");
    keyvaluepairarray_appendf(subject, 10, "noun", "%s", "world");
    keyvaluepairarray_free(subject);
    printf("%s\n", "complete");
}

#define NUMELEMENTS 10
void test_keyvaluepairhash()
{
    KEYVALUEPAIR_HASH *subject;
    KEYVALUEPAIR_ARRAY *testdata;
    const KEYVALUEPAIR *answer;
    char keybuf[20];
    int idx;

    printf("%s\n", "KVP hash test - assert no error.");

    testdata = keyvaluepairarray_create(NUMELEMENTS);
    for (idx = 0; idx < NUMELEMENTS; idx++) {
	(void)snprintf(keybuf, 20, "key%d", idx);
	keyvaluepairarray_appendf(testdata, 20, keybuf, "value %d", idx);
    }

    subject = keyvaluepairhash_create(testdata, NUMELEMENTS);
    answer = keyvaluepairhash_get(subject, "key1");
    assert(answer != NULL);
    assert(strncmp(answer->key, "key1", 5) == 0);
    assert(strncmp(answer->value, "value 1", 8) == 0);

    answer = keyvaluepairhash_get(subject, "key7");
    assert(answer != NULL);
    assert(strncmp(answer->key, "key7", 5) == 0);
    assert(strncmp(answer->value, "value 7", 8) == 0);

    keyvaluepairhash_free(subject);
    keyvaluepairarray_free(testdata);
    printf("%s\n", "complete");
}
