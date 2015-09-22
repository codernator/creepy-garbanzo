#include "sysinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

#ifndef S_SPLINT_S
#include <ctype.h>
#else
extern bool isspace(const char ch);
#endif

#define VALUE_TERMINATOR '~'

#define ABORT \
{ \
    if (raise(SIGABRT) != 0) { \
        _Exit(EXIT_FAILURE); \
    } \
}


static size_t count_keys(FILE *fp, const char terminator);
static size_t count_key_length(FILE *fp);
static void read_data(FILE *fp, struct keyvaluepair_array *data, char terminator);
static /*@only@*/char *read_key(FILE *fp, size_t length);


/** 
 * write a collection of key-value (string,string) pairs to a file in the format:
 * - the key is written to its own line unmodified with no indent.
 * - the value is written on lines following the key with a single-tab block indent.
 *
 * Exampli Gratis:
 * - given { .key = "message", .value = "Hello, World!\nWhat's up?\n(... besides the sky.)" }
 * - output
 * message\n
 * \tHello, World!\n
 * \tWhat's up?\n
 * \t(... besides the sky.)\n
 *
 * Notes:
 * - Items with no key will not be written.
 * - Items with no value _will be_ written.
 * - Any white-space at the start of a key will be skipped, because keys must be identified by a
 *   non-whitespace character.
 * - White-space in values will be preserved.
 */
void database_write(FILE *fp, const struct keyvaluepair_array *data)
{
    size_t i;

    /** for each key-value pair... */
    for (i = 0; i < data->top; i++) {
        /**
         * Write the key part of a key-value pair to a file, beginning on a new linei,
         * and terminated by a new line. Preceding spaces in a key will simply be skipped.
         */
        {
            const char *key = data->items[i].key;

            // no key = no write.
            if (key == NULL || key[0] == '\0') continue;

            // can't abide preceding spaces in key.
            while (isspace(key[0]) && key[0] != '\0') { key++; }

            // blank key = no write.
            if (key[0] == '\0') continue;

            fprintf(fp, "%s\n", key);
        }

        /**
         * Write a string value to a file, block-indented by a tab.
         * Strip \n\r  \n\r  \r  \n from each line and write \n instead.
         * Preserve line-breaks and line lengths. (Empty lines will be written).
         * Follow the string with a new line \n, the terminator character, and
         * another new line \n.
         */
        {
            const char *value = data->items[i].value;
            const char *p;
            char c;

            p = value;

            (void)fputc('\t', fp);
            c = *p;
            while (c != '\0') {
                if (c == '\n' || c == '\r') {
                    char t;
                    (void)fputc('\n', fp);
                    (void)fputc('\t', fp);
                    // skip past second part of \n\r or \r\n but not \r\r or \n\n.
                    t = *(p + 1);
                    if ((t == '\n' || t == '\r') && t != c) {
                        c = *(++p);
                    }
                } else {
                    (void)fputc(c, fp);
                }

                c = *(++p);
            }
            (void)fputc('\n', fp);
            (void)fputc(VALUE_TERMINATOR, fp);
            (void)fputc('\n', fp);
        }
    }
}

struct keyvaluepair_array *database_read(FILE *fp, const char terminator)
{
    size_t numkeys;
    struct keyvaluepair_array *data;

    numkeys = count_keys(fp, terminator);
    data = keyvaluepairarray_create(numkeys);
    read_data(fp, data, terminator);

    return data;
}


size_t count_keys(FILE *fp, const char terminator)
{
    size_t count = 0;
    char c;
    fpos_t current_pos;

    if (fgetpos(fp, &current_pos) != 0) {
        perror("count_keys");
        ABORT;
    }

    c = (char)fgetc(fp);
    while (c != '\0' && c != terminator) {
        if (!(c == '\t' || c == '\n' || c == '\r'))
            count++;
        c = (char)fgetc(fp);
    }

    if (fsetpos(fp, &current_pos) != 0) {
        perror("count_keys");
        ABORT;
    }

    return count;
}

size_t count_key_length(FILE *fp)
{
    fpos_t current_pos;
    size_t count = 0;
    char c;

    if (fgetpos(fp, &current_pos) != 0) {
        perror("count_keys");
        ABORT;
    }

    c = (char)fgetc(fp);
    while (c != '\0' && c != '\n' && c != '\r') {
        count++;
        c = (char)fgetc(fp);
    }

    if (fsetpos(fp, &current_pos) != 0) {
        perror("count_keys");
        ABORT;
    }

    return count;
}

void read_data(FILE *fp, struct keyvaluepair_array *data, char terminator)
{
    char c;

    c = (char)fgetc(fp);
    while (c != '\0' && c != terminator) {
        if (!isspace(c)) {
            size_t key_length;
            char *key;

            (void)ungetc(c, fp);
            key_length = count_key_length(fp);
            key = read_key(fp, key_length);

            keyvaluepairarray_append(data, key, "");
            free(key);
        }
        c = (char)fgetc(fp);
    }
}

char *read_key(FILE *fp, size_t length)
{
    char *key;
    char *p;
    char c;
    
    key = calloc(sizeof(char), length+1);
    assert(key != NULL);
    p = key;
    c = (char)fgetc(fp);
    while (c != '\n' && c != '\r' && c != '\0') {
        *p = c;
        p++;
        c = (char)fgetc(fp);
    }
    *p = '\0';

    // If there is a \n\r or a \r\n, slide past the second half of it.
    if (c == '\n' || c == '\r') {
        char t = (char)fgetc(fp);
        if (((t != '\r') && (t != '\n')) || (t == c)) {
            (void)ungetc(t, fp);
        }
    }

    return key;
}

