#ifndef __SYSINTERNALS_H__
#define __SYSINTERNALS_H__

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#ifdef S_SPLINT_S
#define _Exit exit
typedef int pid_t;
long long atoll(const char *nptr);
/*@only@*/char *strdup(/*@observer@*/const char *s);
#endif

// TODO - sysinternals should not have such specific data.
#define LOG_BUG_FILE "./db/log/bug.txt"  /* For 'bug' and bug() */
#define LOG_TYPO_FILE "./db/log/typo.txt" /* For 'typo' */
#define LOG_LAST_COMMANDS_FILE "./db/log/lastCMDs.txt"
#define LOG_ALWAYS_FILE "./db/log/logAlways.txt"
#define LOG_ALL_CMDS_FILE "./db/log/logALLCommands.txt"
#define LOG_PLAYER_FILE "./db/log/player/%s.txt"
#define LOG_SHUTDOWN_FILE    "./db/log/shutdown.txt"         /* For 'shutdown' */


#define LOWER(c)                 ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)                 ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define UMIN(a, b)               ((a) < (b) ? (a) : (b))
#define UABS(a)                  ((a) < 0 ? -(a) : (a))
#define UMAX(a, b)               ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define UCEILING(a, b)           ((a)/(b) + (((a)%(b)) != 0 ? 1 : 0))


struct array_list
{
    size_t size;
    size_t top;
    /*@only@*/void *items;
};

#define array_list_create(array, node_type, num_elements) \
    array = malloc(sizeof(struct array_list)); \
    assert(array != NULL); \
    array->size = num_elements; \
    array->top = 0; \
    array->items = calloc(sizeof(node_type), num_elements); \
    assert(array->items != NULL); \

#define array_list_grow(array, node_type, new_size) \
    if (new_size > array->size) { \
        node_type *new_items; \
        new_items = calloc(sizeof(node_type), new_size); \
        assert(new_items != NULL); \
        memcpy(new_items, array->items, sizeof(node_type)*new_size); \
        array->size = new_size; \
        free(array->items); \
        array->items = new_items; \
    }

#define array_list_any(array) ((array)->top > 0)

#define array_list_each(array, index) \
    for (index = 0; index < array->top; index++)

#define array_list_node_at(array, node_type, index) \
    &((node_type *)array->items)[index]

#define array_list_free(array, node_type, free_node) \
    if (array != NULL) { \
        free(array->items); \
        free(array); \
    }

#define array_list_append(out_node, array, node_type) \
    if (array->top == array->size) { \
        size_t new_size = array->size << 2; \
        array_list_grow(array, node_type, new_size); \
    } \
    out_node = &((node_type *)array->items)[array->top]; \
    array->top++;



typedef unsigned long long HASHVALUETYPE;
typedef unsigned int HASHBUCKETTYPE;
typedef unsigned char byte;



struct key_string_pair
{
    /*@owned@*/const char *key;
    /*@owned@*/const char *value;
};


typedef /*@observer@*/struct key_string_pair *key_string_pair_ptr;
struct keyvaluepairhashnode
{
    size_t size;
    size_t top;
    /*@dependent@*/key_string_pair_ptr *items;
};

struct keyvaluepairhash {
    int numhashbuckets;
    /*@only@*/struct keyvaluepairhashnode *lookup;
    /*@owned@*/key_string_pair_ptr *masterlist;
};


/** hashing.c */
HASHVALUETYPE calchashvalue(const char *key);
/** ~hashing.c */


/** key_string_pair.c */
/*@only@*/struct array_list *kvp_create_array(size_t numelements);
/*@observer@*//*@null@*/const char *kvp_array_find(const struct array_list *array, const char *key);
void kvp_free_array(/*@only@*//*@null@*/struct array_list *array);
/*@observer@*//*@null@*/const char *kvp_array_valueat(const struct array_list *array, const size_t idx); 

/*@only@*/struct keyvaluepairhash *keyvaluepairhash_create(/*@observer@*/struct array_list *array, size_t numelements, size_t numbuckets);
/*@observer@*//*@null@*/const char *keyvaluepairhash_get(struct keyvaluepairhash *hash, const char * const key);
void keyvaluepairhash_free(/*@only@*//*@null@*/struct keyvaluepairhash *hash);
/** ~key_string_pair.c */


/** librandom.c */
void init_mm(void);
int number_fuzzy(int number);
long number_fuzzy_long(long number);
int number_range(int from, int to);
long number_range_long(long from, long to);
int number_percent(void);
int number_bits(unsigned int width);
int dice(int number, int size);
/** ~librandom.c */


/** libutils.c */
void i_bubble_sort(int *iarray, int array_size);
/** ~libutils.c */


/** libstrings.c */

/**
 * copy an existing character buffer of size old_size into a new character buffer
 * of size new_size.
 * assert(new_size > old_size)
 */
/*@only@*/char *grow_buffer(/*@observer@*/const char *existing, size_t old_size, size_t new_size);

/*@only@*/char *replace_one(/*@observer@*/const char *original, /*@observer@*/const char *find, /*@observer@*/const char *replace, size_t max_length);
void string_lower(const char *source, /*@out@*/char *target, size_t maxLength);
void smash_tilde(char *str);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
void capitalize_into(const char *source, /*@out@*/ char *initialized_target, size_t string_length);
bool is_number(const char *test);

byte parse_byte(const char *string);
byte parse_byte2(const char *string, byte min, byte max);
int parse_int(const char *string);
long parse_long(const char *string);
unsigned int parse_unsigned_int(const char *string);
unsigned long parse_unsigned_long(const char *string);
unsigned long flag_from_string(const char *flagstring);

/*@only@*/char *int_to_string(int value);
/*@only@*/char *uint_to_string(unsigned int value);
/*@only@*/char *long_to_string(long value);
/*@only@*/char *ulong_to_string(unsigned long value);
/*@only@*/char *flag_to_string(unsigned long flag);
/** ~libstrings.c */

/** logging.c */
#define LOG_SINK_ALWAYS 1
#define LOG_SINK_ALL 2
#define LOG_SINK_LASTCMD 4
#define LOG_SINK_BUG 8
#define LOG_SINK_TYPO 16
#define LOG_SINK_SHUTDOWN 32

void log_to(int log, const char *fmt, ...);
void log_to_player(const char username[], const char *fmt, ...);
void log_bug(const char *fmt, ...);
void log_string(const char *fmt, ...);
/** ~logging.c */

/** database.c */
struct database_controller {
    /*@shared@*/FILE *_cfptr;
};

/*@only@*/struct array_list *database_parse_stream(/*@observer@*/const char *dbstream);
/*@only@*/char *database_create_stream(/*@observer@*/const struct array_list *data);
/*@only@*//*@null@*/struct database_controller *database_open(const char *const file_path, bool forreading);
/*@only@*/char *database_read_stream(/*@observer@*/const struct database_controller *db);
void database_write_stream(/*@observer@*/const struct database_controller *db, /*@observer@*/const char *data);
void database_close(/*@only@*/struct database_controller *db);
/** ~database.c */

#endif // __SYSINTERNALS_H__

