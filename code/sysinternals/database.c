#include "sysinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef S_SPLINT_S
#define _Exit exit
#endif

#define ABORT \
{ \
    if (raise(SIGABRT) != 0) { \
        _Exit(EXIT_FAILURE); \
    } \
}

static void write_value(FILE *fp, const char *value);
static int count_keys(FILE *fp, const char terminator);
static void read_data(FILE *fp, struct keyvaluepair_array *data, char terminator);


void database_write(FILE *fp, const struct keyvaluepair_array *data)
{
    size_t i;

    for (i = 0; i < data->top; i++) {
        fprintf(fp, "%s\n", data->items[i].key);
        write_value(fp, data->items[i].value);
    }
}

struct keyvaluepair_array *database_read(/*@unused@*/FILE *fp, const char terminator)
{
    fpos_t current_pos;
    int keys;
    struct keyvaluepair_array *data;

    if (fgetpos(fp, &current_pos) != 0) {
        perror("database_read");
        ABORT;
    }
    keys = count_keys(fp, terminator);
    if (fsetpos(fp, &current_pos) != 0) {
        perror("database_read");
        ABORT;
    }

    data = keyvaluepairarray_create(keys);
    read_data(fp, data, terminator);
    return data;
}


static void write_value(FILE *fp, const char *value)
{
    const char *p;
    char c;

    p = value;

    (int)putc('\t', fp);
    c = *p;
    while (c != '\0') {
        if (c == '\n' || c == '\r') {
            char t;
            (int)putc('\n', fp);
            (int)putc('\t', fp);
            t = *(p + 1);
            // skip past second part of \n\r or \r\n but not \r\r or \n\n.
            if ((t == '\n' || t == '\r') && t != c) {
                c = *(++p);
            }
        } else {
            (int)putc(c, fp);
        }

        c = *(++p);
    }
    (void)putc('\n', fp);
}

int count_keys(FILE *fp, const char terminator)
{
    int count = 0;
    char c;

    c = getc(fp);
    while (c != '\0' && c != terminator) {
        if (c == '\t' || c == '\n' || c == '\r') continue;
        count++;
    }
    return count;
}

void read_data(FILE *fp, struct keyvaluepair_array *data, char terminator)
{
    char c;

    c = getc(fp);
    while (c != '\0' && c != terminator) {
        if (isspace(c)) continue;

    }
}
