#define ASSIGN_ULONG_KEY(data, field, name) \
    entry = keyvaluepairarray_find((data), (name)); \
    (field) = (entry != NULL) ? parse_unsigned_long(entry) : 0;

#define ASSIGN_UINT_KEY(data, field, name) \
    entry = keyvaluepairarray_find((data), (name)); \
    (field) = (entry != NULL) ? parse_unsigned_int(entry) : 0;

#define ASSIGN_INT_KEY(data, field, name) \
    entry = keyvaluepairarray_find((data), (name)); \
    (field) = (entry != NULL) ? parse_int(entry) : 0;

#define ASSIGN_STRING_KEY(data, field, name, defaultValue) \
    entry = keyvaluepairarray_find((data), (name)); \
    (field) = (entry != NULL) ? string_copy(entry) : ((defaultValue) == NULL ? NULL : string_copy(defaultValue));

#define ASSIGN_FLAG_KEY(data, field, name) \
    entry = keyvaluepairarray_find((data), (name)); \
    (field) = (entry != NULL) ? flag_from_string(entry) : 0;

#define ASSIGN_BOOL_KEY(data, field, name) \
    entry = keyvaluepairarray_find((data), (name)); \
    (field) = (entry != NULL) ? parse_int(entry) == 1 : false;

