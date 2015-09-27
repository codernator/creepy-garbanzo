#include "sysinternals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


static void test_keyvaluepairarray();
static void test_keyvaluepairhash();
static void test_database_write();
static void test_database_read();


int main(/*@unused@*/int argc, /*@unused@*/char **argv)
{
    test_keyvaluepairarray();
    test_keyvaluepairhash();
    test_database_write();
    test_database_read();
    return EXIT_SUCCESS;
}


void test_database_write()
{
    KEYVALUEPAIR_ARRAY *subject;

    subject = keyvaluepairarray_create(3);
    keyvaluepairarray_append(subject, "verb", "hello");
    keyvaluepairarray_appendf(subject, 20, "noun", "%s\n%s?\n", "world", "how goes");
    keyvaluepairarray_appendf(subject, 10, "number", "%d", 123);

    database_write(stdout, subject);
    keyvaluepairarray_free(subject);
    printf("%s\n", "complete");
}

#define TEST_DB_FILE "database_read_test.db"
void test_database_read()
{
    KEYVALUEPAIR_ARRAY *subject;
    FILE *db;

    printf("%s.\n", "Creating test data");
    subject = keyvaluepairarray_create(3);
    keyvaluepairarray_append(subject, "verb", "hello");
    keyvaluepairarray_appendf(subject, 20, "noun", "%s\n%s?\n", "world", "how goes");
    keyvaluepairarray_appendf(subject, 10, "number", "%d", 123);

    db = fopen(TEST_DB_FILE, "w");
    assert(db != NULL);
    database_write(db, subject);
    (void)fclose(db);
    keyvaluepairarray_free(subject);
    printf("%s.\n", "Test data created");


    printf("%s.\n", "Reading data");
    db = fopen(TEST_DB_FILE, "r");
    assert(db != NULL);
    subject = database_read(db);
    (void)fclose(db);
    printf("%s.\n", "Printing data");
    database_write(stdout, subject);
    printf("%s.\n", "Clean up");
    keyvaluepairarray_free(subject);
    printf("%s.\n", "Fin");
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

#define NUMELEMENTS 537
void test_keyvaluepairhash()
{
    KEYVALUEPAIR_HASH *subject;
    KEYVALUEPAIR_ARRAY *testdata;
    const char *answer;
    char keybuf[20];
    int idx;

    printf("%s\n", "KVP hash test - assert no error.");

    testdata = keyvaluepairarray_create(NUMELEMENTS);
    for (idx = 0; idx < NUMELEMENTS; idx++) {
	(void)snprintf(keybuf, 20, "key%d", idx+1);
	keyvaluepairarray_appendf(testdata, 20, keybuf, "value %d", idx+1);
    }

    printf("%s\n", "Create");
    subject = keyvaluepairhash_create(testdata, NUMELEMENTS, 13);
    printf("%s\n", "Get 1");
    answer = keyvaluepairhash_get(subject, "key1");
    assert(answer != NULL);
    assert(strncmp(answer, "value 1", 8) == 0);

    printf("%s %d\n", "Get", NUMELEMENTS);
    (void)snprintf(keybuf, 20, "key%d", NUMELEMENTS);
    answer = keyvaluepairhash_get(subject, keybuf);
    assert(answer != NULL);
    {
	char valuebuf[20];
	(void)snprintf(valuebuf, 20, "value %d", NUMELEMENTS);
	assert(strncmp(answer, valuebuf, 20) == 0);
    }

    printf("%s\n", "Dump");
    {
	size_t max = 0;
	for (idx = 0; idx < subject->numhashbuckets; idx++) {
	    KEYVALUEPAIR_HASHNODE *node = &subject->lookup[idx];
	    if (node->top > 0) {
		if (node->top > max) max = node->top;
		printf("%d, %d, %d\n", idx, (int)node->size, (int)node->top);
	    }
	}
	printf("Max occurrences %d, Max elements %d\n", (int)max, (int)subject->lookup[0].size);
    }

    keyvaluepairhash_free(subject);
    keyvaluepairarray_free(testdata);
    printf("%s\n", "complete");
}
