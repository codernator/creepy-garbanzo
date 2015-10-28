#define SERIALIZED_NUMBER_SIZE 32


#define SERIALIZE_FLAGS(flags, key, kvpa) \
{ \
    char *flagstring = flag_to_string(flags); \
    kvp_array_append_copy((kvpa), (key), flagstring); \
    free(flagstring); \
}

#define ASSIGN_ULONG_KEY(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_unsigned_long(entry) : 0;

#define ASSIGN_UINT_KEY(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_unsigned_int(entry) : 0;

#define ASSIGN_INT_KEY(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_int(entry) : 0;

#define ASSIGN_STRING_KEY(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? strdup(entry) : NULL;

#define ASSIGN_STRING_KEY_DEFAULT(data, field, name, defaultValue) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? strdup(entry) : strdup(defaultValue);

#define ASSIGN_FLAG_KEY(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? flag_from_string(entry) : 0;

#define ASSIGN_BOOL_KEY(data, field, name) \
    entry = kvp_array_find((data), (name)); \
    (field) = (entry != NULL) ? parse_int(entry) == 1 : false;

