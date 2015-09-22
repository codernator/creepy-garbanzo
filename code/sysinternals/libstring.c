#include "sysinternals.h"
#include <string.h>
#ifndef S_SPLINT_S
#include <ctype.h>
#endif
#include <stdlib.h>
#include <assert.h>



char *grow_buffer(const char *existing, size_t old_size, size_t new_size)
{
    char *new_buffer;

    assert(new_size > old_size);
    new_buffer = (char *)calloc(sizeof(char), new_size);
    assert(new_buffer != NULL);
    memcpy(new_buffer, existing, old_size);

    return new_buffer;
}

/**
 * return true if an argument is completely numeric.
 */
bool is_number(const char *arg)
{
    if (*arg == '\0')
        return false;

    if (*arg == '+' || *arg == '-')
        arg++;

    for (; *arg != '\0'; arg++)
        if (!isdigit((int)*arg))
            return false;

    return true;
}


/**
 *	replace all ~'s with -'s - used to verify that
 *	strings written to files to do contain ~'s
 */
void smash_tilde(char *str)
{
    for (; *str != '\0'; str++)
        if (*str == '~')
            *str = '-';

    return;
}

/**
 * determine whether astr is the same as bstr - case insensitive
 * returns true if they do not match....false if they do
 */
bool str_cmp(const char *astr, const char *bstr)
{
    if (astr == NULL) {
        return true;
    }

    if (bstr == NULL) {
        return true;
    }

    for (; *astr > '0' || *bstr > '0'; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr)) {
            return true;
        }
    }

    return false;
}

/**
 * determine if bstr begins with astr
 * returns true if they do not much....false if they do
 */
bool str_prefix(const char *astr, const char *bstr)
{
    if (astr == NULL) {
        return true;
    }

    if (bstr == NULL) {
        return true;
    }

    for (; *astr != '\0'; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr)) {
            return true;
        }
    }

    return false;
}

/**
 * determine if astr is part of bstr
 * returns true if they do not match....false if they do
 */
bool str_infix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ((c0 = LOWER(astr[0])) == '\0')
        return false;

    sstr1 = (int)strlen(astr);
    sstr2 = (int)strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
        if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar)) {
            return false;
        }
    }

    return true;
}

/**
 * determine if the suffix of a string matches another
 * returns true if they do not match....false if they do
 */
bool str_suffix(const char *astr, const char *bstr)
{
    size_t sstr1;
    size_t sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
        return false;

    return true;
}

/**
 * copies a string into a buffer with the initial string capped.
 */
void capitalize_into(const char *source, /*@out@*/ char *initialized_target, size_t string_length)
{
    int i;
    size_t counter;

    counter = 0;
    for (i = 0; counter < string_length && source[i] != '\0'; i++) {
        initialized_target[i] = LOWER(source[i]);
        counter++;
    }

    initialized_target[i] = '\0';
    initialized_target[0] = UPPER(initialized_target[0]);
}


byte parse_byte(const char *string)
{
    int raw = atoi(string);

    return (byte)UMAX(UMIN(raw, 255), 0);
}

byte parse_byte2(const char *string, byte min, byte max)
{
    int raw = atoi(string);

    return (byte)UMAX(UMIN(raw, (int)max), (int)min);
}

int parse_int(const char *string)
{
    return atoi(string);
}

long parse_long(const char *string)
{
    return atol(string);
}

unsigned int parse_unsigned_int(const char *string)
{
    return (unsigned int)UMAX(0, atol(string));
}

unsigned long parse_unsigned_long(const char *string)
{
    return UMAX(0, (unsigned long)atoll(string));
}

