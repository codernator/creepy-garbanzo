#include "sysinternals.h"


inline void i_bubble_sort(int *iarray, int array_size)
{
	int holder, x, y;

	for (x = 0; x < array_size; x++)
		for (y = 0; y < array_size - 1; y++)
			if (iarray[y] > iarray[y + 1]) {
				holder = iarray[y + 1];
				iarray[y + 1] = iarray[y];
				iarray[y] = holder;
			}
}
