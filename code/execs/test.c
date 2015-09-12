#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PULSE_PER_SECOND 4
#define LOOP_TIME_SLICE 
int main(/*@unused@*/int argc, /*@unused@*/char **argv)
{
    static const double loop_time_slice = ((double)CLOCKS_PER_SEC/PULSE_PER_SECOND);
    clock_t last;
    clock_t current;
    long wait;


    current = clock();
    last = current - 56875;

    wait = (long)(1000000 * (loop_time_slice - (double)(current - last)) / CLOCKS_PER_SEC);

    printf("\nlast: %ld, current: %ld, cps: %ld, yo: %ld\n\n", (long)last, (long)current, CLOCKS_PER_SEC, wait);
    return EXIT_SUCCESS;
}

