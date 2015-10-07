#include "sysinternals.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

#ifndef S_SPLINT_S
#include <ctype.h>
#else
extern bool isspace(const char ch);
#endif

#ifndef DATABASE_RECORD_TERMINATOR
#define DATABASE_RECORD_TERMINATOR '#'
#endif

#ifndef DATABASE_INIT_KEYCOUNT
#define DATABASE_INIT_KEYCOUNT 32
#endif

#ifndef DATABASE_INIT_KEYLEN
#define DATABASE_INIT_KEYLEN 32
#endif

#ifndef DATABASE_INIT_VALUELEN
#define DATABASE_INIT_VALUELEN 1024
#endif

#define ABORT \
{ \
    if (raise(SIGABRT) != 0) { \
        _Exit(EXIT_FAILURE); \
    } \
}


struct database_controller *database_open(const char const *filepath)
{
    struct database_controller *answer;

    answer = malloc(sizeof(struct database_controller));
    assert(answer != NULL);

    answer->_cfptr = fopen(filepath, "a+b");
    if (answer->_cfptr == NULL) {
        free(answer);
        return NULL;
    }

    return answer;
}

void database_close(struct database_controller *db)
{
    if (fclose(db->_cfptr) == EOF) {
        ABORT;
    }
    db->_cfptr = NULL;
    free(db);
}


/** 
 * write a collection of key-value (string,string) pairs to a file in the format:
 * - the key is written to its own line unmodified with no indent.
 * - the value is written on lines following the key with a single-tab block indent.
 *
 * following the final key/value pair, write the record terminator.
 *
 * Exampli Gratis:
 * - given { .key = "message", .value = "Hello, World!\nWhat's up?\n(... besides the sky.)" }
 * - given DATABASE_RECORD_TERMINATOR #
 * - output
 * message\n
 * \tHello, World!\n
 * \tWhat's up?\n
 * \t(... besides the sky.)\n
 * #
 *
 * Notes:
 * - Items with no key will not be written.
 * - Items with no value _will be_ written.
 * - Any white-space at the start of a key will be skipped, because keys must be identified by a
 *   non-whitespace character.
 * - White-space in values will be preserved.
 */
void database_write(const struct database_controller *db, const struct keyvaluepair_array *data)
{
    size_t i;
    FILE *fp = db->_cfptr;

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
        }
    }

    (void)fputc(DATABASE_RECORD_TERMINATOR, fp);
    (void)fputc('\n', fp);
}

struct keyvaluepair_array *database_read(const struct database_controller *db)
{
    const int EOL = (int)'\n';
    const int TAB = (int)'\t';
    const int TERM = (int)DATABASE_RECORD_TERMINATOR;
    size_t numkeys = DATABASE_INIT_KEYCOUNT;
    size_t keylen = DATABASE_INIT_KEYLEN;
    size_t valuelen = DATABASE_INIT_VALUELEN;
    struct keyvaluepair_array *data;
    char *keybuf;
    char *valuebuf;
    FILE *fp = db->_cfptr;

    keybuf = (char *)calloc(sizeof(char), keylen);
    assert(keybuf != NULL);
    valuebuf = (char *)calloc(sizeof(char), valuelen);
    assert(valuebuf != NULL);

    data = keyvaluepairarray_create(numkeys);

    /** TODO - this is unbrushed hair. */
    while (true) {
        memset(keybuf, 0, sizeof(char) * keylen);
        memset(valuebuf, 0, sizeof(char) * keylen);

        /** input a key. */
        {
            int c;
            size_t i;

            i = 0;
            c = getc(fp);
            while (c != EOF && c != EOL) {
                keybuf[i++] = (char)c;
                if (i == keylen) {
                    size_t newkeylen = keylen << 2;
                    char *newkeybuf;
                    /* time to grow the buffer */
                    newkeybuf = grow_buffer(keybuf, keylen, newkeylen);
                    free(keybuf);
                    keybuf = newkeybuf;
                    keylen = newkeylen;
                }
                c = getc(fp);
            }
            keybuf[i] = '\0';

            /** this was a blank line. moving along. */
            if (keybuf[0] == '\0') {
                if (c == EOF) { // it was blank because we are done!
                    break;
                } else {
                    continue;
                }
            }

            /* The record terminator alone is not a valid key, but a valid
             * key could start with the record terminator.
             */
            if (keybuf[0] == (char)TERM && keybuf[1] == '\0') {
                break;
            }

            /** valid key, but EOF anyway? seems like a premature file closure. */
            if (c == EOF) {
                fprintf(stderr, "Premature end of file reading key %s.", keybuf);
                ABORT;
                break;
            }
        }

        /** input a value */
        {
            int c;
            int pc;
            size_t i;

            i = 0;
            pc = EOL;
            c = getc(fp);
            while (c != EOF) {
                if (pc == EOL) {
                    if (c != TAB) {
                        /* Anything following end of line that isn't a tab is the end of
                         * this value. It could be end-of-record or the start of a new key,
                         * so, put it back!
                         */
                        int ung = ungetc(c, fp);
                        assert(ung == c);
                        /* The previous EOL was appended but is not part of the value. */
                        valuebuf[i-1] = '\0';
                        break;
                    }
                    /* this is the block indent for the value. */
                    pc = c;
                    c = getc(fp);
                    continue;
                }
                valuebuf[i++] = (char)c;
                if (i == valuelen) {
                    size_t newvaluelen = valuelen << 2;
                    char *newvaluebuf = grow_buffer(valuebuf, valuelen, newvaluelen);
                    /* time to grow the buffer */
                    free(valuebuf);
                    valuebuf = newvaluebuf;
                    valuelen = newvaluelen;
                }
                pc = c;
                c = getc(fp);
            }
            valuebuf[i] = '\0';

            keyvaluepairarray_append(data, keybuf, valuebuf);
        }
    }

    free(keybuf);
    free(valuebuf);
    return data;
}



char *read_key(FILE *fp, size_t length)
{
    char *key;
    char *p;
    char c;

    key = calloc(sizeof(char), length+1);
    assert(key != NULL);
    p = key;
    c = (char)getc(fp);
    while (c != '\n' && c != '\r' && c != '\0') {
        *p = c;
        p++;
        c = (char)getc(fp);
    }
    *p = '\0';

    // If there is a \n\r or a \r\n, slide past the second half of it.
    if (c == '\n' || c == '\r') {
        char t = (char)getc(fp);
        if (((t != '\r') && (t != '\n')) || (t == c)) {
            (void)ungetc(t, fp);
        }
    }

    return key;
}

