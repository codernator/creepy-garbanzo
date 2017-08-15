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

#define GROW_BUFFER(buf, buflen) \
{ \
    size_t newbuflen; \
    char *newbuf; \
    newbuflen = buflen << 2; \
    newbuf = grow_buffer(buf, buflen, newbuflen); \
    assert(newbuf != NULL); \
    free(buf); \
    buf = newbuf; \
    buflen = newbuflen; \
}



struct database_controller *database_open(const char const *filepath, bool forreading)
{
    struct database_controller *answer;

    answer = malloc(sizeof(struct database_controller));
    assert(answer != NULL);

    answer->_cfptr = fopen(filepath, forreading ? "a+b" : "w+b");
    if (answer->_cfptr == NULL) {
        free(answer);
        return NULL;
    }

    return answer;
}

void database_close(struct database_controller *db)
{
    int closeresult;

    closeresult = fclose(db->_cfptr);
    db->_cfptr = NULL;
    free(db);
    if (closeresult == EOF) {
        perror("database close.");
        ABORT;
    }
}

#define STRPTR_PUTC(dbstream, dbsindex, streamsize, chr) \
    dbstream[dbsindex] = chr; \
    dbsindex++; \
    if (dbsindex == streamsize) { \
        GROW_BUFFER(dbstream, streamsize); \
    }


/** 
 * write a collection of key-value (string,string) pairs to a string buffer in the format:
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
char *database_create_stream(const struct array_list *data)
{
    char *dbstream;
    size_t dbsindex = 0;
    size_t streamsize = DATABASE_INIT_VALUELEN;
    size_t i;

    dbstream = calloc(sizeof(char), streamsize);
    assert(dbstream != NULL);

    /** for each key-value pair... */
    for (i = 0; i < data->top; i++) {
        /**
         * Write the key part of a key-value pair to a file, beginning on a new line,
         * and terminated by a new line. Preceding spaces in a key will simply be skipped.
         */
        {
            const char *key = ((struct key_string_pair *)data->items)[i].key;
            size_t len;

            // no key = no write.
            if (key == NULL || key[0] == '\0') continue;

            // can't abide preceding spaces in key.
            while (isspace(key[0]) && key[0] != '\0') { key++; }

            // blank key = no write.
            if (key[0] == '\0') continue;

            len = strlen(key);
            if (dbsindex + len >= streamsize) { 
                GROW_BUFFER(dbstream, streamsize);
            }
            memcpy(dbstream + dbsindex, key, len);
            dbsindex += len;
            STRPTR_PUTC(dbstream, dbsindex, streamsize, '\n');
        }

        /**
         * Write a string value to a file, block-indented by a tab.
         * Strip \n\r  \n\r  \r  \n from each line and write \n instead.
         * Preserve line-breaks and line lengths. (Empty lines will be written).
         * Follow the string with a new line \n, the terminator character, and
         * another new line \n.
         */
        {
            const char *value = ((struct key_string_pair *)data->items)[i].value;
            const char *p;
            char c;

            p = value;

            STRPTR_PUTC(dbstream, dbsindex, streamsize, '\t');
            c = *p;
            while (c != '\0') {
                if (c == '\n' || c == '\r') {
                    char t;
                    STRPTR_PUTC(dbstream, dbsindex, streamsize, '\n');
                    STRPTR_PUTC(dbstream, dbsindex, streamsize, '\t');
                    // skip past second part of \n\r or \r\n but not \r\r or \n\n.
                    t = *(p + 1);
                    if ((t == '\n' || t == '\r') && t != c) {
                        c = *(++p);
                    }
                } else {
                    STRPTR_PUTC(dbstream, dbsindex, streamsize, c);
                }

                c = *(++p);
            }
            STRPTR_PUTC(dbstream, dbsindex, streamsize, '\n');
        }
    }

    STRPTR_PUTC(dbstream, dbsindex, streamsize, DATABASE_RECORD_TERMINATOR);
    STRPTR_PUTC(dbstream, dbsindex, streamsize, '\n');
    dbstream[dbsindex] = '\0';

    return dbstream;
}

void database_write_stream(const struct database_controller *db, const char *data)
{
    size_t none;
    size_t len;
    FILE *fp = db->_cfptr;

    len = strlen(data);
    none = fwrite(data, sizeof(char), len, fp);
    if (none < len) {
        perror("Write failed.");
    }
}

char *database_read_stream(const struct database_controller *db)
{
    const int EOL = (int)'\n';
    const int TERM = (int)DATABASE_RECORD_TERMINATOR;
    char *dbstream;
    size_t dbsindex = 0;
    size_t streamsize = DATABASE_INIT_VALUELEN;
    FILE *fp = db->_cfptr;
    int c;
    int pc;

    dbstream = calloc(sizeof(char), streamsize);
    assert(dbstream != NULL);

    // TODO - consider reading buffer in blocks. Will need to reset file pointer 
    // based on where record end is found so next record read starts from the correct location.
    pc = 0;
    while (true) {
        c = fgetc(fp);
        if (feof(fp) == 1)
            break;
        if (c == 0) {
            dbstream[dbsindex] = '\0';
            fprintf(stderr, "%s", "Premature end of file reading record.");
            break;
        }

        STRPTR_PUTC(dbstream, dbsindex, streamsize, (char)c);

        if (pc == EOL && c == TERM) {
            dbstream[dbsindex] = '\0';
            break;
        }
        pc = c;
    }

    return dbstream;
}


struct array_list *database_parse_stream(const char *dbstream)
{
    const char EOL = '\n';
    const char TAB = '\t';
    const char TERM = DATABASE_RECORD_TERMINATOR;
    const char EOFC = (char)EOF;
    size_t numkeys = DATABASE_INIT_KEYCOUNT;
    struct array_list *data;
    size_t dbsindex;


    data = kvp_create_array(numkeys);

    /** TODO - this is unbrushed hair. */
    dbsindex = 0;
    while (true) {
        char *keybuf;
        char *valuebuf;
        struct key_string_pair *kvp; 
        size_t keylen = DATABASE_INIT_KEYLEN;
        size_t valuelen = DATABASE_INIT_VALUELEN;

        keybuf = (char *)calloc(sizeof(char), keylen);
        assert(keybuf != NULL);
        valuebuf = (char *)calloc(sizeof(char), valuelen);
        assert(valuebuf != NULL);

        /** input a key. */
        {
            char c;
            size_t i;

            i = 0;
            c = dbstream[dbsindex++];
            if (c == DATABASE_RECORD_TERMINATOR) {
                break;
            }
            while (c != EOFC && c != EOL) {
                keybuf[i++] = (char)c;
                if (i == keylen) {
                    /* time to grow the buffer */
                    GROW_BUFFER(keybuf, keylen);
                }
                c = dbstream[dbsindex++];
            }
            if (i >= keylen) {
                ABORT;
            }
            keybuf[i] = '\0';

            /** this was a blank line. moving along. */
            if (keybuf[0] == '\0') {
                if (c == EOFC) { // it was blank because we are done!
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
            if (c == EOFC) {
                ABORT;
                break;
            }
        }

        /** input a value */
        {
            char c;
            char pc;
            size_t i;

            i = 0;
            pc = EOL;
            c = dbstream[dbsindex++];
            while (c != EOFC) {
                if (pc == EOL) {
                    if (c != TAB) {
                        /* Anything following end of line that isn't a tab is the end of
                         * this value. It could be end-of-record or the start of a new key,
                         * so, put it back!
                         */
                        dbsindex--;
                        c = pc;
                        /* The previous EOL was appended but is not part of the value. */
                        valuebuf[i-1] = '\0';
                        break;
                    }
                    /* this is the block indent for the value. */
                    pc = c;
                    c = dbstream[dbsindex++];
                    continue;
                }
                valuebuf[i++] = (char)c;
                if (i == valuelen) {
                    /* time to grow the buffer */
                    GROW_BUFFER(valuebuf, valuelen);
                }
                pc = c;
                c = dbstream[dbsindex++];
            }
            valuebuf[i] = '\0';

            array_list_append(kvp, data, struct key_string_pair); 
            // assert every entry is fresh.
            /*@-mustfreeonly@*/
            kvp->key = keybuf; 
            kvp->value = valuebuf; 
            /*@+mustfreeonly@*/
        }
    }

    return data;
}

