#include <stdbool.h>
#include <stddef.h>

#define LOWER(c)                 ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)                 ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define UMIN(a, b)               ((a) < (b) ? (a) : (b))
#define UABS(a)                  ((a) < 0 ? -(a) : (a))
#define UMAX(a, b)               ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))

#ifdef S_SPLINT_S
typedef int pid_t;
long long atoll(const char *nptr);
#endif

typedef unsigned char byte;

typedef struct keyvaluepair KEYVALUEPAIR;
typedef struct keyvaluepair_array KEYVALUEPAIR_ARRAY;
typedef struct keyvaluepairhashnode KEYVALUEPAIR_HASHNODE;
typedef struct keyvaluepairhash KEYVALUEPAIR_HASH;
typedef struct keyvaluepairhashitem KEYVALUEPAIR_HASHITEM;

struct keyvaluepair
{
    const char *key;
    const char *value;
};

struct keyvaluepair_array
{
    size_t size;
    size_t top;
    KEYVALUEPAIR *items;
};

typedef /*@observer@*/struct keyvaluepair *KEYVALUEPAIR_P;
struct keyvaluepairhashnode
{
    size_t size;
    size_t top;
    KEYVALUEPAIR_P *items;
};

struct keyvaluepairhash {
    int hashkey;
    /*@only@*/KEYVALUEPAIR_HASHNODE *lookup;
};


/*@only@*/KEYVALUEPAIR_ARRAY *keyvaluepairarray_create(size_t numelements);
void keyvaluepairarray_append(KEYVALUEPAIR_ARRAY *array, const char *key, const char *value);
void keyvaluepairarray_appendf(KEYVALUEPAIR_ARRAY *array, size_t maxlength, const char *key, const char *valueformat, ...);
void keyvaluepairarray_free(/*@only@*//*@null@*/KEYVALUEPAIR_ARRAY *array);
/*@only@*/KEYVALUEPAIR_HASH *keyvaluepairhash_create(/*@observer@*/KEYVALUEPAIR_ARRAY *array, size_t numelements);
/*@observer@*//*@null@*/const char *keyvaluepairhash_get(KEYVALUEPAIR_HASH *hash, const char * const key);
void keyvaluepairhash_free(/*@only@*//*@null@*/KEYVALUEPAIR_HASH *hash);

void init_mm(void);

int number_fuzzy(int number);
long number_fuzzy_long(long number);
int number_range(int from, int to);
long number_range_long(long from, long to);
int number_percent(void);
int number_bits(unsigned int width);
int dice(int number, int size);


void i_bubble_sort(int *iarray, int array_size);

void smash_tilde(char *str);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
void capitalize_into(const char *source, /*@out@*/ char *initialized_target, size_t string_length);
bool is_space(const char test);
bool is_digit(const char test);
bool is_alpha(const char test);
bool is_upper(const char test);
bool is_alnum(const char test);
bool is_number(const char *test);
char to_lower(const char test);

byte parse_byte(const char *string);
byte parse_byte2(const char *string, byte min, byte max);
int parse_int(const char *string);
long parse_long(const char *string);
unsigned int parse_unsigned_int(const char *string);
unsigned long parse_unsigned_long(const char *string);

