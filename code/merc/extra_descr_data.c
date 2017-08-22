#include "merc.h"
#include "serialize.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct extra_descr_data *extradescrdata_new()
{
    struct extra_descr_data *target;

    target = malloc(sizeof(struct extra_descr_data));
    assert(target != NULL);
    memset(target, 0, sizeof(struct extra_descr_data));

    return target;
}

struct extra_descr_data *extradescrdata_clone(struct extra_descr_data *source)
{
    struct extra_descr_data *target;

    target = malloc(sizeof(struct extra_descr_data));
    assert(target != NULL);
    memset(target, 0, sizeof(struct extra_descr_data));

    /*@-mustfreeonly@*/
    target->keyword = strdup(source->keyword);
    target->description = strdup(source->description);
    /*@+mustfreeonly@*/

    return target;
}

struct extra_descr_data *extradescrdata_deserialize(const struct array_list *data)
{
    struct extra_descr_data *target;
    const char *entry; // used by deserialize_assign_* macro

    target = malloc(sizeof(struct extra_descr_data));
    assert(target != NULL);
    memset(target, 0, sizeof(struct extra_descr_data));

    /*@-mustfreeonly@*/
    deserialize_assign_string_default(data, target->keyword, "keyword", "no keyword");
    deserialize_assign_string_default(data, target->description, "description", "no description");
    /*@+mustfreeonly@*/

    return target;
}

void extradescrdata_free(struct extra_descr_data *data)
{
    assert(data != NULL);
    if (data->keyword)
        free(data->keyword);
    if (data->description)
        free(data->description);
    free(data);
}

struct array_list *extradescrdata_serialize(const struct extra_descr_data *extra)
{
    struct array_list *answer;

    answer = kvp_create_array(2);

    serialize_copy_string(answer, "keyword", extra->keyword);
    serialize_copy_string(answer, "description", extra->description);

    return answer;
}

struct extra_descr_data *extradescrdata_match(struct extra_descr_data *head, const char *partialkey)
{
    struct extra_descr_data *target;

    target = head->next;
    while (target != NULL) {
        if (is_name(partialkey, target->keyword))
            break;
        target = target->next;
    }

    return target;
}

void olc_extradescrdata_getdescription(void *owner, char *target, size_t maxsize)
{
    struct extra_descr_data *extra;
    extra = (struct extra_descr_data *)owner;
    (void)strncpy(target, extra->description, maxsize);
}

void olc_extradescrdata_setdescription(void *owner, const char *text)
{
    struct extra_descr_data *extra;
    extra = (struct extra_descr_data *)owner;
    if (extra->description != NULL)
        free (extra->description);
    extra->description = strdup(text);
    return;
}
