#include <stdlib.h>
#include "sysinternals.h"


extern void srandom(unsigned int);
extern int getpid();
extern time_t time(time_t *tloc);


static long number_mm(void);


void init_mm()
{
	srandom((unsigned int)time(NULL) ^ getpid());
}

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

int number_bits(unsigned int width)
{
	return (int)(number_mm() & ((1 << width) - 1));
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
