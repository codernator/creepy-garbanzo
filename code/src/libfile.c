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


extern bool is_space(const char test);
extern bool is_digit(const char test);


/***************************************************************************
* PUBLIC INTERFACE
***************************************************************************/
char fread_letter(FILE *fp);
long fread_long(FILE *fp);
unsigned int fread_uint(FILE *fp);
long fread_flag(FILE *fp);
int fread_number(FILE *fp);
void fread_to_eol(FILE *fp);
char *fread_word(FILE *fp);


/***************************************************************************
* IMPLEMENTATION
***************************************************************************/
/*
 * Read a letter from a file.
 */
char fread_letter(FILE *fp)
{
	char c;

	do
		c = (char)getc(fp);
	while (is_space(c));

	return c;
}


/*
 * Read a number from a file.
 */
long fread_long(FILE *fp)
{
	long number = 0;
	bool sign = FALSE;
	char c;

	do
		c = (char)getc(fp);
	while (is_space(c));

	if (c == '+') {
		c = (char)getc(fp);
	} else if (c == '-') {
		sign = TRUE;
		c = (char)getc(fp);
	}

	if (!is_digit(c)) {
		bug("Fread_long: bad format.", 0);
		exit(1);
	}

	while (is_digit(c)) {
		number = number * 10 + (long)c - (long)'0';
		c = (char)getc(fp);
	}

	if (sign)
		number = 0 - number;

	if (c == '|')
		number += fread_long(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}


/*
 * Read a number from a file.
 */
unsigned int fread_uint(FILE *fp)
{
	unsigned int number = 0;
	bool sign = FALSE;
	char c;

	do
		c = (char)getc(fp);
	while (is_space(c));

	if (c == '+') {
		c = (char)getc(fp);
	} else if (c == '-') {
		sign = TRUE;
		c = (char)getc(fp);
	}

	if (!is_digit(c)) {
		bug("Fread_uint: bad format.", 0);
		exit(1);
	}

	while (is_digit(c)) {
		number = number * 10 + (unsigned int)c - (unsigned int)'0';
		c = (char)getc(fp);
	}

	if (sign)
		number = 65535u - number;

	if (c == '|')
		number += fread_uint(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}



/*
 * Read a number from a file.
 */
int fread_number(FILE *fp)
{
	int number;
	bool sign;
	char c;

	do
		c = (char)getc(fp);
	while (is_space(c));

	number = 0;

	sign = FALSE;
	if (c == '+') {
		c = (char)getc(fp);
	} else if (c == '-') {
		sign = TRUE;
		c = (char)getc(fp);
	}

	if (!is_digit(c)) {
		bug("Fread_number: bad format.", 0);
		exit(1);
	}

	while (is_digit(c)) {
		number = number * 10 + (int)c - (int)'0';
		c = (char)getc(fp);
	}

	if (sign)
		number = 0 - number;

	if (c == '|')
		number += fread_number(fp);
	else if (c != ' ')
		ungetc(c, fp);

	return number;
}


long fread_flag(FILE *fp)
{
	int number;
	char c;
	bool negative = FALSE;

	do
		c = (char)getc(fp);
	while (is_space(c));

	if (c == '-') {
		negative = TRUE;
		c = (char)getc(fp);
	}

	number = 0;

	if (!is_digit(c)) {
		while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
			number += flag_convert(c);
			c = (char)getc(fp);
		}
	}

	while (is_digit(c)) {
		number = number * 10 + (int)c - (int)'0';
		c = (char)getc(fp);
	}

	if (c == '|')
		number += fread_flag(fp);
	else if (c != ' ')
		ungetc(c, fp);

	if (negative)
		return -1 * number;

	return number;
}



/*
 * Read to end of line(for comments).
 */
void fread_to_eol(FILE *fp)
{
	char c;

	do
		c = (char)getc(fp);
	while (c != '\n' && c != '\r');

	do
		c = (char)getc(fp);
	while (c == '\n' || c == '\r');

	ungetc(c, fp);
	return;
}



/*
 * Read one word(into static buffer).
 */
char *fread_word(FILE *fp)
{
	static char word[MAX_INPUT_LENGTH];
	char *pword;
	char cEnd;

	do
		cEnd = (char)getc(fp);
	while (is_space(cEnd));

	if (cEnd == '\'' || cEnd == '"') {
		pword = word;
	} else {
		word[0] = cEnd;
		pword = word + 1;
		cEnd = ' ';
	}

	for (; pword < word + MAX_INPUT_LENGTH; pword++) {
		*pword = (char)getc(fp);
		if (cEnd == ' ' ? is_space(*pword) : *pword == cEnd) {
			if (cEnd == ' ')
				ungetc(*pword, fp);
			*pword = '\0';
			return word;
		}
	}

	bug("Fread_word: word too long.", 0);
	exit(1);
}
