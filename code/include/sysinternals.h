#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef S_SPLINT_S
#define _Exit exit
typedef int pid_t;
long long atoll(const char *nptr);
#endif


#define LOWER(c)                 ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)                 ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define UMIN(a, b)               ((a) < (b) ? (a) : (b))
#define UABS(a)                  ((a) < 0 ? -(a) : (a))
#define UMAX(a, b)               ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define UCEILING(a, b)           ((a)/(b) + (((a)%(b)) != 0 ? 1 : 0))
#define CALC_HASH_BUCKET(key, numhashbuckets) ((HASHBUCKETTYPE)(calchashvalue((key)) % (HASHVALUETYPE)(numhashbuckets)))


typedef unsigned long long HASHVALUETYPE;
typedef unsigned int HASHBUCKETTYPE;
typedef unsigned char byte;

typedef struct keyvaluepair KEYVALUEPAIR;
typedef struct keyvaluepair_array KEYVALUEPAIR_ARRAY;
typedef struct keyvaluepairhashnode KEYVALUEPAIR_HASHNODE;
typedef struct keyvaluepairhash KEYVALUEPAIR_HASH;
typedef struct keyvaluepairhashitem KEYVALUEPAIR_HASHITEM;

struct keyvaluepair
{
    /*@owned@*/const char *key;
    /*@owned@*/const char *value;
};

struct keyvaluepair_array
{
    size_t size;
    size_t top;
    /*@only@*/KEYVALUEPAIR *items;
};

typedef /*@observer@*/struct keyvaluepair *KEYVALUEPAIR_P;
struct keyvaluepairhashnode
{
    size_t size;
    size_t top;
    /*@dependent@*/KEYVALUEPAIR_P *items;
};

struct keyvaluepairhash {
    int numhashbuckets;
    /*@only@*/KEYVALUEPAIR_HASHNODE *lookup;
    /*@owned@*/KEYVALUEPAIR_P *masterlist;
};


/** hashing.c */
HASHVALUETYPE calchashvalue(const char *key);
/** ~hashing.c */


/** keyvaluepair.c */
/*@only@*/KEYVALUEPAIR_ARRAY *keyvaluepairarray_create(size_t numelements);
void keyvaluepairarray_append(KEYVALUEPAIR_ARRAY *array, const char *key, const char *value);
void keyvaluepairarray_appendf(KEYVALUEPAIR_ARRAY *array, size_t maxlength, const char *key, const char *valueformat, ...);
void keyvaluepairarray_grow(KEYVALUEPAIR_ARRAY *array, size_t newSize);
/*@observer@*//*@null@*/const char *keyvaluepairarray_find(const KEYVALUEPAIR_ARRAY *array, const char *key);
void keyvaluepairarray_free(/*@only@*//*@null@*/KEYVALUEPAIR_ARRAY *array);
/*@only@*/KEYVALUEPAIR_HASH *keyvaluepairhash_create(/*@observer@*/KEYVALUEPAIR_ARRAY *array, size_t numelements, size_t numbuckets);
/*@observer@*//*@null@*/const char *keyvaluepairhash_get(KEYVALUEPAIR_HASH *hash, const char * const key);
void keyvaluepairhash_free(/*@only@*//*@null@*/KEYVALUEPAIR_HASH *hash);
/** ~keyvaluepair.c */


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
void database_write(FILE *fp, /*@observer@*/const struct keyvaluepair_array *data);
/*@only@*/struct keyvaluepair_array *database_read(FILE *fp);
/** ~database.c */

