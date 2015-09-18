#include "sysinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

#ifndef S_SPLINT_S
#include <ctype.h>
#else
#define isspace(ch) ((ch) == 20)
#endif

#define ABORT \
{ \
    if (raise(SIGABRT) != 0) { \
        _Exit(EXIT_FAILURE); \
    } \
}


static void write_value(FILE *fp, const char *value);
static size_t count_keys(FILE *fp, const char terminator);
static size_t count_key_length(FILE *fp);
static void read_data(FILE *fp, struct keyvaluepair_array *data, char terminator);
static /*@only@*/char *read_key(FILE *fp, size_t length);


void database_write(FILE *fp, const struct keyvaluepair_array *data)
{
    size_t i;

    for (i = 0; i < data->top; i++) {
        fprintf(fp, "%s\n", data->items[i].key);
        write_value(fp, data->items[i].value);
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



/**
 * Write a string to a file, block-indented by a tab.
 * Strip \n\r  \n\r  \r  \n from each line and write \n instead.
 * Preserve line-breaks and line lengths. (Empty lines will be written).
 * Follow the string with a new line \n.
 */
static void write_value(FILE *fp, const char *value)
{
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

