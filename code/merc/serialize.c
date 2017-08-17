#include "serialize.h"
#include <string.h>


void serialize_copy_string(struct array_list *kvp_array, const char *const key, const char *value)
{
    struct key_string_pair *kvp;
    array_list_append(kvp, kvp_array, struct key_string_pair);
    kvp->key = strdup(key);
    kvp->value = strdup(value);
}

void serialize_take_string(struct array_list *kvp_array, const char *const key, char *value)
{
    struct key_string_pair *kvp;
    array_list_append(kvp, kvp_array, struct key_string_pair);
    kvp->key = strdup(key);
    kvp->value = value;
}


