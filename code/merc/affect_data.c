#include "merc.h"
#include "serialize.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct affect_data *affectdata_new()
{
    struct affect_data *target;

    target = malloc(sizeof(struct affect_data));
    assert(target != NULL);
    memset(target, 0, sizeof(struct affect_data));

    return target;
}

struct affect_data *affectdata_clone(struct affect_data *source)
{
    struct affect_data *target;

    target = malloc(sizeof(struct affect_data));
    assert(target != NULL);
    memset(target, 0, sizeof(struct affect_data));

    target->location = source->location;
    target->modifier = source->modifier;
    target->where = source->where;
    target->type = source->type;
    target->duration = source->duration;
    target->bitvector = source->bitvector;
    target->level = source->level;

    return target;
}

struct affect_data *affectdata_deserialize(const struct array_list *data)
{
    struct affect_data *target;
    const char *entry; // used by deserialize_assign_* macro

    target = malloc(sizeof(struct affect_data));
    assert(target != NULL);
    memset(target, 0, sizeof(struct affect_data));

    deserialize_assign_long(data, target->bitvector, "bitvector");
    deserialize_assign_int(data, target->duration, "duration");
    deserialize_assign_int(data, target->level, "level");
    deserialize_assign_int(data, target->location, "location");
    deserialize_assign_long(data, target->modifier, "modifier");
    deserialize_assign_int(data, target->type, "type");
    deserialize_assign_int(data, target->where, "where");

    return target;
}

void affectdata_free(struct affect_data *data)
{
    assert(data != NULL);
    free(data);
}

struct array_list *affectdata_serialize(const struct affect_data *affect)
{
    struct array_list *answer;

    answer = kvp_create_array(8);

    serialize_take_string(answer, "bitvector", long_to_string(affect->bitvector));
    serialize_take_string(answer, "duration", int_to_string(affect->duration));
    serialize_take_string(answer, "level", int_to_string(affect->level));
    serialize_take_string(answer, "location", int_to_string(affect->location));
    serialize_take_string(answer, "modifier", long_to_string(affect->modifier));
    serialize_take_string(answer, "type", int_to_string(affect->type));
    serialize_take_string(answer, "where", int_to_string(affect->where));

    return answer;
}
