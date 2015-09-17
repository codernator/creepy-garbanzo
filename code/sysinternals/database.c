#include "sysinternals.h"
#include <stdio.h>


static void write_value(FILE *fp, const char *value);


void database_write(FILE *fp, struct keyvaluepair_array *data)
{
    size_t i;

    for (i = 0; i < data->top; i++) {
        fprintf(fp, "%s:\n", data->items[i].key);
        write_value(fp, data->items[i].value);
    }
}

struct keyvaluepair_array *database_read(/*@unused@*/FILE *fp)
{
    return keyvaluepairarray_create(10);
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

