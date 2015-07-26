/***************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
 *   Chastain, Michael Quan, and Mitchell Tse.                              *
 *	                                                                         *
 *   In order to use any part of this Merc Diku Mud, you must comply with   *
 *   both the original Diku license in 'license.doc' as well the Merc	     *
 *   license in 'license.txt'.  In particular, you may not remove either of *
 *   these copyright notices.                                               *
 *                                                                             *
 *   Much time and thought has gone into this software and you are          *
 *   benefitting.  We hope that you share your changes too.  What goes      *
 *   around, comes around.                                                  *
 ****************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *   ROM has been brought to you by the ROM consortium                      *
 *       Russ Taylor(rtaylor@hypercube.org)                                 *
 *       Gabrielle Taylor(gtaylor@hypercube.org)                            *
 *       Brian Moore(zump@rom.org)                                          *
 *   By using this code, you have agreed to follow the terms of the         *
 *   ROM license, in the file Rom24/doc/rom.license                         *
 ****************************************************************************/

#include <stdlib.h>
#include "merc.h"


/***************************************************************************
* IMPORTS
***************************************************************************/
extern void srandom(unsigned int);
extern int getpid();
extern time_t time(time_t *tloc);


/***************************************************************************
* PUBLIC INTERFACE
***************************************************************************/
int number_fuzzy(int number);
long number_fuzzy_long(long number);
long number_range_long(long from, long to);
int number_range(int from, int to);
int number_percent(void);
int number_door(void);
int number_bits(unsigned int width);
void init_mm(void);
long number_mm(void);
int dice(int number, int size);


/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy(int number)
{
	switch (number_bits(2)) {
	case 0:
		number -= 1;
		break;
	case 3:
		number += 1;
		break;
	}

	return UMAX(1, number);
}

long number_fuzzy_long(long number)
{
	switch (number_bits(2)) {
	case 0:
		number -= 1;
		break;
	case 3:
		number += 1;
		break;
	}

	return UMAX(1, number);
}





/*
 * Generate a random number.
 */
long number_range_long(long from, long to)
{
	unsigned int power;
	long number;
	long bit = 0;

	if (from == 0 && to == 0)
		return 0;

	if ((to = to - from + 1) <= 1)
		return from;

	if (to < 0) {
		power = 2;
	} else {
		unsigned int too = (unsigned int)to;
		for (power = 2; power < too; power <<= 1) {
		}
	}

	bit = (long)power - 1l;
	while ((number = (long)number_mm() & bit) >= to) {
	}

	return from + number;
}



/*
 * Generate a random number.
 */
int number_range(int from, int to)
{
	unsigned int power;
	int number;
	int bit = 0;

	if (from == 0 && to == 0)
		return 0;

	if ((to = to - from + 1) <= 1)
		return from;

	if (to < 0) {
		power = 2;
	} else {
		unsigned int too = (unsigned int)to;
		for (power = 2; power < too; power <<= 1) {
		}
	}

	bit = (int)power - 1;
	while ((number = (int)number_mm() & bit) >= to) {
	}

	return from + number;
}




/*
 * Generate a percentile roll.
 */
int number_percent(void)
{
	long percent;
	const long bit = 127;

	while ((percent = number_mm() & bit) > 99) {
	}

	return 1 + (int)percent;
}



/*
 * Generate a random door.
 */
int number_door(void)
{
	long door;
	const long bit = 7;

	while ((door = number_mm() & bit) > 5) {
	}

	return (int)door;
}


int number_bits(unsigned int width)
{
	return (int)(number_mm() & ((1 << width) - 1));
}



void init_mm()
{
	srandom((unsigned int)time(NULL) ^ getpid());
}



long number_mm(void)
{
	return random() >> 6;
}


/*
 * Roll some dice.
 */
int dice(int number, int size)
{
	int idice;
	int sum;

	switch (size) {
	case 0:
		return 0;
	case 1:
		return number;
	}

	for (idice = 0, sum = 0; idice < number; idice++)
		sum += number_range(1, size);

	return sum;
}
