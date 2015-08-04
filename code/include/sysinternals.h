#include <stdbool.h>
#include <stddef.h>

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
bool to_lower(const char test);
