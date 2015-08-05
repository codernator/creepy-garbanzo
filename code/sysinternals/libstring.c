#include "sysinternals.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>



/**
 * wrapper around isalnum for safe testing against character array.
 */
bool is_alnum(const char test)
{
	return isalnum((int)test);
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
		if (!is_digit(*arg))
			return false;

	return true;
}

/**
 * wrapper around isspace for safe testing against character array.
 */
bool is_space(const char test)
{
	return isspace((int)test);
}

/**
 * wrapper around isdigit for safe testing against character array.
 */
bool is_digit(const char test)
{
	return isdigit((int)test);
}

/**
 * wrapper around isalpha for safe testing against character array.
 */
bool is_alpha(const char test)
{
	return isalpha((int)test);
}

/**
 * wrapper around isupper for safe testing against character array.
 */
bool is_upper(const char test)
{
	return isupper((int)test);
}

/**
 * wrapper around tolower for safe testing against character array.
 */
char to_lower(const char test)
{
	return tolower((int)test);
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


byte parse_byte(char *string)
{
	int raw = atoi(string);

	return (byte)UMAX(UMIN(raw, 255), 0);
}

byte parse_byte2(char *string, byte min, byte max)
{
	int raw = atoi(string);

	return (byte)UMAX(UMIN(raw, (int)max), (int)min);
}

int parse_int(char *string)
{
	return atoi(string);
}

long parse_long(char *string)
{
	return atol(string);
}

unsigned int parse_unsigned_int(char *string)
{
	return (unsigned int)UMAX(0, atol(string));
}

unsigned long parse_unsigned_long(char *string)
{
	return UMAX(0, (unsigned long)atoll(string));
}

