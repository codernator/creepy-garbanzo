#include "sysinternals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void kvp_copy_string(struct array_list *kvp_array, const char *const key, const char *value)
static void kvp_take_string(struct array_list *kvp_array, const char *const key, char *value)

static void test_keyvaluepairarray(void);
static void test_keyvaluepairhash(void);
static void test_database_write(void);
static void test_database_read(void);
static void test_flags(void);
static void test_database_create_stream(void);

static void dump_kvp(const struct array_list *subject);

int main(/*@unused@*/int argc, /*@unused@*/char **argv)
{
    test_keyvaluepairarray();
    test_keyvaluepairhash();
    test_database_write();
    test_database_read();
    test_flags();
    test_database_create_stream();
    return EXIT_SUCCESS;
}

#define TEST_DB_FILE "database_read_test.db"

void test_database_create_stream()
{
    struct array_list *subject;
    char *dbstream;

    printf("%s\n", "test_database_create_stream");
    subject = kvp_create_array(3);
    kvp_copy_string(subject, "verb", "hello");
    kvp_copy_string(subject, "noun", "world\nhow goes?\n");
    kvp_take_string(subject, "number",int_to_string(123));

    dbstream = database_create_stream(subject);
    kvp_free_array(subject);

    subject = kvp_create_array(2);
    kvp_copy_string(subject, "keyword", "hello");
    kvp_copy_string(subject, "text", dbstream);

    free(dbstream);

    dump_kvp(subject);
    kvp_free_array(subject);
    printf("%s\n", "complete");
}


void test_database_write()
{
    struct array_list *subject;
    struct database_controller *db;
    char *dbstream;

    subject = kvp_create_array(3);
    kvp_copy_string(subject, "verb", "hello");
    kvp_copy_string(subject, "noun", "world\nhow goes?\n");
    kvp_take_string(subject, "number", int_to_string(123));
    dbstream = database_create_stream(subject);
    kvp_free_array(subject);

    db = database_open(TEST_DB_FILE, false);
    assert(db != NULL);
    database_write_stream(db, dbstream);
    database_close(db);
    free(dbstream);
    
    printf("%s\n", "complete");
}

void test_database_read()
{
    struct array_list *subject;
    struct database_controller *db;
    char *dbstream;

    printf("%s.\n", "Creating test data");
    subject = kvp_create_array(3);
    kvp_copy_string(subject, "verb", "hello");
    kvp_copy_string(subject, "noun", "world\nhow goes?\n");
    kvp_take_string(subject, "number", int_to_string(123));
    dbstream = database_create_stream(subject);
    kvp_free_array(subject);

    db = database_open(TEST_DB_FILE, false);
    assert(db != NULL);
    database_write_stream(db, dbstream);
    database_close(db);
    free(dbstream);
    printf("%s.\n", "Test data created");


    db = database_open(TEST_DB_FILE, true);
    assert(db != NULL);
    printf("%s.\n", "Reading data");
    dbstream = database_read_stream(db);
    printf("%s.\n", "Parsing data");
    subject = database_parse_stream(dbstream);
    free(dbstream);
    database_close(db);
    printf("%s.\n", "Printing data");
    dump_kvp(subject);
    printf("%s.\n", "Clean up");
    kvp_free_array(subject);


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
        subject = kvp_create_array(3);
        memset(keybuf, (int)'k', 200*sizeof(char));
        keybuf[199] = '\0';
        memset(valbuf, (int)'v', 3000*sizeof(char));
        valbuf[2999] = '\0';
        kvp_take_string(subject, keybuf, valbuf);
        free(keybuf);
        dbstream = database_create_stream(subject);
        kvp_free_array(subject);

        /* Write to test file. */
        db = database_open(TEST_DB_FILE, false);
        assert(db != NULL);
        database_write_stream(db, dbstream);
        free(dbstream);
        database_close(db);
        /* Read from test file. */
        db = database_open(TEST_DB_FILE, true);
        assert(db != NULL);
        printf("%s.\n", "Reading data");
        dbstream = database_read_stream(db);
        printf("%s.\n", "Parsing data");
        subject = database_parse_stream(dbstream);
        free(dbstream);
        database_close(db);
        printf("%s\n", "First item?");
        key = ((struct key_string_pair *)subject->items)[0].key;
        val = ((struct key_string_pair *)subject->items)[0].value;
        printf("%d (%c %c), %d (%c %c)", (int)strlen(key), key[0], key[198], (int)strlen(val), val[0], val[2998]);
        kvp_free_array(subject);
    }

    printf("%s.\n", "Try multi-record");
    {
        db = database_open(TEST_DB_FILE, false);
        assert(db != NULL);
        subject = kvp_create_array(3);
        kvp_copy_string(subject, "key1", "value1-1\nvalue1-1a");
        kvp_copy_string(subject, "key2", "value1-2");
        dbstream = database_create_stream(subject);
        kvp_free_array(subject);
        database_write_stream(db, dbstream);
        free(dbstream);
        subject = kvp_create_array(3);
        kvp_copy_string(subject, "key1", "value2-1\n\nvalue2-1a\nvalue2-1b");
        kvp_copy_string(subject, "key2", "value2-2");
        dbstream = database_create_stream(subject);
        kvp_free_array(subject);
        database_write_stream(db, dbstream);
        free(dbstream);
        database_close(db);

        db = database_open(TEST_DB_FILE, true);
        assert(db != NULL);
        printf("%s.\n", "Reading data");
        dbstream = database_read_stream(db);
        printf("%s.\n", "Parsing data");
        subject = database_parse_stream(dbstream);
        free(dbstream);
        dump_kvp(subject);
        kvp_free_array(subject);
        printf("%s.\n", "Reading data");
        dbstream = database_read_stream(db);
        printf("%s.\n", "Parsing data");
        subject = database_parse_stream(dbstream);
        free(dbstream);
        dump_kvp(subject);
        kvp_free_array(subject);
        database_close(db);
    }
    printf("%s.\n", "Fin");
}

void test_keyvaluepairarray()
{
    struct array_list *subject;

    printf("%s\n", "KVP array test - assert no error.");
    subject = kvp_create_array(2);
    kvp_copy_string(subject, "verb", "hello");
    kvp_copy_string(subject, "noun", "world");
    kvp_free_array(subject);
    printf("%s\n", "complete");
}

#define NUMELEMENTS 537
void test_keyvaluepairhash()
{
    struct keyvaluepairhash *subject;
    struct array_list *testdata;
    const char *answer;
    char keybuf[20];
    char valuebuf[20];
    int idx;

    printf("%s\n", "KVP hash test - assert no error.");

    testdata = kvp_create_array(NUMELEMENTS);
    for (idx = 0; idx < NUMELEMENTS; idx++) {
        (void)snprintf(keybuf, 20, "key%d", idx+1);
        (void)snprintf(valuebuf, 20, "value%d", idx+1);
        kvp_copy_string(testdata, keybuf, valuebuf);
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
            struct keyvaluepairhashnode *node = &subject->lookup[idx];
            if (node->top > 0) {
                if (node->top > max) max = node->top;
                printf("%d, %d, %d\n", idx, (int)node->size, (int)node->top);
            }
        }
        printf("Max occurrences %d, Max elements %d\n", (int)max, (int)subject->lookup[0].size);
    }

    keyvaluepairhash_free(subject);
    kvp_free_array(testdata);
    printf("%s\n", "complete");
}

void dump_kvp(const struct array_list *data)
{
    size_t i;
    for (i = 0; i < data->top; i++) {
        printf("%s\n", ((struct key_string_pair *)data->items)[i].key);
        printf("%s\n", ((struct key_string_pair *)data->items)[i].value);
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



void kvp_copy_string(struct array_list *kvp_array, const char *const key, const char *value)
{
    struct key_string_pair *kvp; 
    array_list_append(kvp, kvp_array, struct key_string_pair); 
    kvp->key = strdup(key); 
    kvp->value = strdup(value); 
}

void kvp_take_string(struct array_list *kvp_array, const char *const key, char *value)
{
    struct key_string_pair *kvp; 
    array_list_append(kvp, kvp_array, struct key_string_pair); 
    kvp->key = strdup(key); 
    kvp->value = value; 
}
