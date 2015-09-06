#include "sysinternals.h"

HASHVALUETYPE calchashvalue(const char *key)
{
    HASHVALUETYPE hash = 0;
    const char *c;
    while (*(c = key++) != '\0') {
	hash = (HASHVALUETYPE)(*c) + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

