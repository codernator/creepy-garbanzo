/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                         *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	     *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *   ROM has been brought to you by the ROM consortium                      *
 *       Russ Taylor(rtaylor@hypercube.org)                                 *
 *       Gabrielle Taylor(gtaylor@hypercube.org)                            *
 *       Brian Moore(zump@rom.org)                                          *
 *   By using this code, you have agreed to follow the terms of the         *
 *   ROM license, in the file Rom24/doc/rom.license                         *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/resource.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "libstring.h"


static bool to_lower(const char test);

// TODO - BROKE
char *lower(char *s)
{
	static char c[1000];
	int i = 0;

	strcpy(c, s);

	while (c[i] != '\0') {
		c[i] = to_lower(c[i]);
		i++;
	}
	return c;
}


/***************************************************************************
* is_alnum
* wrapper around isalnum for safe testing against character array.
***************************************************************************/
bool is_alnum(const char test)
{
	return isalnum((int)test);
}


/***************************************************************************
 * return TRUE if an argument is completely numeric.
***************************************************************************/
bool is_number(const char *arg)
{
	if (*arg == '\0')
		return FALSE;

	if (*arg == '+' || *arg == '-')
		arg++;

	for (; *arg != '\0'; arg++)
		if (!is_digit(*arg))
			return FALSE;

	return TRUE;
}

/***************************************************************************
* is_space
* wrapper around isspace for safe testing against character array.
***************************************************************************/
bool is_space(const char test)
{
	return isspace((int)test);
}

/***************************************************************************
* is_digit
* wrapper around isdigit for safe testing against character array.
***************************************************************************/
bool is_digit(const char test)
{
	return isdigit((int)test);
}

/***************************************************************************
* is_alpha
* wrapper around isalpha for safe testing against character array.
***************************************************************************/
bool is_alpha(const char test)
{
	return isalpha((int)test);
}

/***************************************************************************
* is_upper
* wrapper around isupper for safe testing against character array.
***************************************************************************/
bool is_upper(const char test)
{
	return isupper((int)test);
}


/***************************************************************************
* to_lower
* wrapper around tolower for safe testing against character array.
***************************************************************************/
bool to_lower(const char test)
{
	return tolower((int)test);
}


/***************************************************************************
*	smash_tilde
*
*	replace all ~'s with -'s - used to verify that
*	strings written to files to do contain ~'s
***************************************************************************/
void smash_tilde(char *str)
{
	for (; *str != '\0'; str++)
		if (*str == '~')
			*str = '-';

	return;
}



/***************************************************************************
*	str_cmp
*
*	check to see if astr is the same as bstr - case insensitive
*	returns TRUE if they do not much....FALSE if they do
***************************************************************************/
bool str_cmp(const char *astr, const char *bstr)
{
	if (astr == NULL) {
		bug("Str_cmp: null astr.", 0);
		return TRUE;
	}

	if (bstr == NULL) {
		bug("Str_cmp: null bstr.", 0);
		return TRUE;
	}

	for (; *astr || *bstr; astr++, bstr++)
		if (LOWER(*astr) != LOWER(*bstr))
			return TRUE;

	return FALSE;
}



/***************************************************************************
*	str_prefix
*
*	check to see if bstr begins with astr
*	returns TRUE if they do not much....FALSE if they do
***************************************************************************/
bool str_prefix(const char *astr, const char *bstr)
{
	if (astr == NULL) {
		bug("str_prefix: null astr.", 0);
		return TRUE;
	}

	if (bstr == NULL) {
		bug("str_prefix: null bstr.", 0);
		return TRUE;
	}

	for (; *astr != '\0'; astr++, bstr++)
		if (LOWER(*astr) != LOWER(*bstr))
			return TRUE;

	return FALSE;
}



/***************************************************************************
*	str_infix
*
*	check to see if astr is part of bstr
*	returns TRUE if they do not much....FALSE if they do
***************************************************************************/
bool str_infix(const char *astr, const char *bstr)
{
	int sstr1;
	int sstr2;
	int ichar;
	char c0;

	if ((c0 = LOWER(astr[0])) == '\0')
		return FALSE;

	sstr1 = (int)strlen(astr);
	sstr2 = (int)strlen(bstr);

	for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
		if (c0 == LOWER(bstr[ichar])
		    && !str_prefix(astr, bstr + ichar))
			return FALSE;
	}

	return TRUE;
}




/***************************************************************************
*	str_suffix
*
*	check to see if the suffix of a string matches another
*	returns TRUE if they do not much....FALSE if they do
***************************************************************************/
bool str_suffix(const char *astr, const char *bstr)
{
	size_t sstr1;
	size_t sstr2;

	sstr1 = strlen(astr);
	sstr2 = strlen(bstr);

	if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
		return FALSE;

	return TRUE;
}



/***************************************************************************
*	str_replace
*
*	replace one string for another
***************************************************************************/
char *str_replace(char *orig, char *find, char *replace)
{
	static char buf[MSL];
	char *orig_idx;
	size_t len;
	char *match;

	buf[0] = '\0';
	strcpy(buf, orig);
	orig_idx = orig;

	match = strstr(buf, find);
	while (match != NULL) {
		len = strlen(buf) - strlen(strstr(orig_idx, find));
		buf[len] = '\0';
		orig_idx += len + strlen(find);

		strcat(buf, replace);
		strcat(buf, orig_idx);

		match = strstr(buf, find);
	}

	return buf;
}



/*
 * Returns an initial-capped string.
 */
char *capitalize(const char *str)
{
	static char strcap[MAX_STRING_LENGTH];
	int i;

	for (i = 0; str[i] != '\0'; i++)
		strcap[i] = LOWER(str[i]);
	strcap[i] = '\0';
	strcap[0] = UPPER(strcap[0]);
	return strcap;
}

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
