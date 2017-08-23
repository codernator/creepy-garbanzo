#include "sysinternals.h"

void serialize_copy_string(struct array_list *kvp_array, const char *const key, /*@observer@*/const char *value);
void serialize_take_string(struct array_list *kvp_array, const char *const key, /*@only@*/char *value);


#define deserialize_assign_long(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_long(entry) : 0;

#define deserialize_assign_ulong(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_unsigned_long(entry) : 0;

#define deserialize_assign_uint(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_unsigned_int(entry) : 0;

#define deserialize_assign_int(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_int(entry) : 0;

#define deserialize_assign_char(data, field, name)     \
    entry = kvp_array_find((data), (name));           \
    (field) = (entry != NULL) ? (char)parse_int(entry) : 0;

#define deserialize_assign_string(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? strdup(entry) : NULL;

#define deserialize_assign_string_default(data, field, name, defaultValue) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? strdup(entry) : strdup(defaultValue);

#define deserialize_assign_flag(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? flag_from_string(entry) : 0;

#define deserialize_assign_bool(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_int(entry) == 1 : false;

