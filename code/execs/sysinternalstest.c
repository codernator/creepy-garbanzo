#include "sysinternals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


static void test_keyvaluepairarray();
static void test_keyvaluepairhash();
static void test_database_write();
static void test_database_read();
static void test_flags();

static void dump_kvp(const KEYVALUEPAIR_ARRAY *subject);

int main(/*@unused@*/int argc, /*@unused@*/char **argv)
{
    test_keyvaluepairarray();
    test_keyvaluepairhash();
    test_database_write();
    test_database_read();
    test_flags();
    return EXIT_SUCCESS;
}


void test_database_write()
{
    KEYVALUEPAIR_ARRAY *subject;

    subject = keyvaluepairarray_create(3);
    keyvaluepairarray_append(subject, "verb", "hello");
    keyvaluepairarray_appendf(subject, 20, "noun", "%s\n%s?\n", "world", "how goes");
    keyvaluepairarray_appendf(subject, 10, "number", "%d", 123);

    dump_kvp(subject);
    keyvaluepairarray_free(subject);
    printf("%s\n", "complete");
}

#define TEST_DB_FILE "database_read_test.db"
#define TEST_HELP_FILE "../db/helps.txt"

void test_database_read()
{
    KEYVALUEPAIR_ARRAY *subject;
    struct database_controller *db;

    printf("%s.\n", "Creating test data");
    subject = keyvaluepairarray_create(3);
    keyvaluepairarray_append(subject, "verb", "hello");
    keyvaluepairarray_appendf(subject, 20, "noun", "%s\n%s?\n", "world", "how goes");
    keyvaluepairarray_appendf(subject, 10, "number", "%d", 123);

    db = database_open(TEST_DB_FILE);
    assert(db != NULL);
    database_write(db, subject);
    database_close(db);
    keyvaluepairarray_free(subject);
    printf("%s.\n", "Test data created");


    printf("%s.\n", "Reading data");
    db = database_open(TEST_DB_FILE);
    assert(db != NULL);
    subject = database_read(db);
    database_close(db);
    printf("%s.\n", "Printing data");
    dump_kvp(subject);
    printf("%s.\n", "Clean up");
    keyvaluepairarray_free(subject);


    printf("%s.\n", "Try extra large key and value.");
    {
        const char *key;
        const char *val;
        char *keybuf;
        char *valbuf;
        /* Create KVP */
        keybuf = calloc(sizeof(char), 200);
        assert(keybuf != NULL);
        valbuf = calloc(sizeof(char), 3000);
        assert(valbuf != NULL);
        subject = keyvaluepairarray_create(3);
        memset(keybuf, (int)'k', 200*sizeof(char));
        keybuf[199] = '\0';
        memset(valbuf, (int)'v', 3000*sizeof(char));
        valbuf[2999] = '\0';
        keyvaluepairarray_append(subject, keybuf, valbuf);
        free(keybuf);
        free(valbuf);
        /* Write to test file. */
        db = database_open(TEST_DB_FILE);
        assert(db != NULL);
        database_write(db, subject);
        database_close(db);
        keyvaluepairarray_free(subject);
        /* Read from test file. */
        db = database_open(TEST_DB_FILE);
        assert(db != NULL);
        subject = database_read(db);
        database_close(db);
        key= subject->items[0].key;
        val= subject->items[0].value;
        printf("%d (%c %c), %d (%c %c)", (int)strlen(key), key[0], key[198], (int)strlen(val), val[0], val[2998]);
        keyvaluepairarray_free(subject);
    }

    printf("%s.\n", "Try multi-record");
    {
        db = database_open(TEST_DB_FILE);
        assert(db != NULL);
        subject = keyvaluepairarray_create(3);
        keyvaluepairarray_appendf(subject, 50, "key1", "%s", "value1-1\nvalue1-1a");
        keyvaluepairarray_appendf(subject, 50, "key2", "%s", "value1-2");
        database_write(db, subject);
        keyvaluepairarray_free(subject);
        subject = keyvaluepairarray_create(3);
        keyvaluepairarray_appendf(subject, 50, "key1", "%s", "value2-1\n\nvalue2-1a\nvalue2-1b");
        keyvaluepairarray_appendf(subject, 50, "key2", "%s", "value2-2");
        database_write(db, subject);
        keyvaluepairarray_free(subject);
        database_close(db);

        db = database_open(TEST_DB_FILE);
        assert(db != NULL);
        subject = database_read(db);
        dump_kvp(subject);
        keyvaluepairarray_free(subject);
        subject = database_read(db);
        dump_kvp(subject);
        keyvaluepairarray_free(subject);
        database_close(db);
    }

    printf("%s.\n", "Try help");
    {
        db = database_open(TEST_HELP_FILE);
        assert(db != NULL);
        while (true) {
            subject = database_read(db);
            if (!keyvaluepairarray_any(subject)) {
                keyvaluepairarray_free(subject);
                printf("%s.\n", "No more data.");
                break;
            }
            dump_kvp(subject);
            keyvaluepairarray_free(subject);
        }
        database_close(db);
    }
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

void dump_kvp(const KEYVALUEPAIR_ARRAY *data)
{
    size_t i;
    for (i = 0; i < data->top; i++) {
        printf("%s\n", data->items[i].key);
        printf("%s\n", data->items[i].value);
    }
}

void test_flags()
{
    unsigned long flag = 0xFFFFFFFF;
    char *flags;

    flags = flag_to_string(flag);
    printf("Flags: %s\n", flags);
    free(flags);


    flag = 0xFFFFFFFF;
    flags = flag_to_string(flag);
    printf("Flags: %lu %s\n", flag, flags);
    flag = flag_from_string(flags);
    printf("Flags: %lu %s\n", flag, flags);
    free(flags);
}
