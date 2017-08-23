#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"



struct exit_data *exittemplate_new(unsigned long vnum)
{
    struct exit_data *templatedata;

    templatedata = malloc(sizeof(struct exit_data));
    assert(templatedata != NULL);

    /** Default values */
    {
        memset(templatedata, 0, sizeof(struct exit_data));
        templatedata->vnum = vnum;
        /*@-mustfreeonly@*/
        templatedata->keyword = strdup("(no keyword)");
        templatedata->description = strdup("(no description)");
        /*@+mustfreeonly@*/
    }

    return templatedata;
}

struct exit_data *exittemplate_clone(struct exit_data *source, int direction)
{
    struct exit_data *clone;

    clone = malloc(sizeof(struct exit_data));
    assert(clone != NULL);

    {
        memset(clone, 0, sizeof(struct exit_data));
        clone->vnum = source->vnum;
        clone->to_room = source->to_room;
        clone->direction = direction;
        /*@-mustfreeonly@*/
        clone->keyword = strdup(source->keyword);
        clone->description = strdup(source->description);
        /*@+mustfreeonly@*/

        clone->exit_info = source->exit_info;
        clone->rs_flags = source->rs_flags;
        clone->key = source->key;
        clone->orig_door = source->orig_door;
    }

    return clone;
}

struct exit_data *exittemplate_deserialize(const struct array_list *data)
{
    struct exit_data *templatedata;
    const char *entry;

    templatedata = malloc(sizeof(struct exit_data));
    assert(templatedata != NULL);
    memset(templatedata, 0, sizeof(struct exit_data));

    deserialize_assign_ulong(data, templatedata->vnum, "vnum");
    /*@-mustfreeonly@*/
    deserialize_assign_string_default(data, templatedata->keyword, "keyword", "(no keyword)");
    deserialize_assign_string_default(data, templatedata->description, "long", "(no description)");
    /*@+mustfreeonly@*/

    deserialize_assign_int(data, templatedata->direction, "direction");
    deserialize_assign_flag(data, templatedata->exit_info, "exit_info");
    deserialize_assign_flag(data, templatedata->rs_flags, "rs_flags");
    deserialize_assign_long(data, templatedata->key, "key");
    deserialize_assign_int(data, templatedata->orig_door, "orig_door");

    return templatedata;
}

struct array_list *exittemplate_serialize(const struct exit_data *obj)
{
    struct array_list *answer;

    answer = kvp_create_array(20);

    serialize_take_string(answer, "vnum", ulong_to_string(obj->vnum));
    if (obj->keyword != NULL)
        serialize_copy_string(answer, "keyword", obj->keyword);

    if (obj->description != NULL)
        serialize_copy_string(answer, "long", obj->description);

    serialize_take_string(answer, "direction", int_to_string(obj->direction));
    serialize_take_string(answer, "exit_info", flag_to_string(obj->exit_info));
    serialize_take_string(answer, "rs_flags", flag_to_string(obj->rs_flags));

    serialize_take_string(answer, "key", long_to_string(obj->key));
    serialize_take_string(answer, "orig_door", int_to_string(obj->orig_door));

    return answer;
}

void exittemplate_free(struct exit_data *templatedata)
{
    assert(templatedata != NULL);

    if (templatedata->keyword != NULL) free(templatedata->keyword);
    if (templatedata->description != NULL) free(templatedata->description);
    free(templatedata);
    return;
}


void exittemplate_setkeyword(struct exit_data *template, const char *keyword)
{
    if (template->keyword != NULL)
        free(template->keyword);
    template->keyword = strdup(keyword);
    return;
}

void exittemplate_setlong(struct exit_data *template, const char *description)
{
    if (template->description != NULL)
        free(template->description);
    template->description = strdup(description);
    return;
}


void olc_exittemplate_getdescription(void *owner, char *target, size_t maxlen)
{
    struct exit_data *exit_data;
    exit_data = (struct exit_data *)owner;
    (void)strncpy(target, exit_data->description, maxlen);
    return;
}

void olc_exittemplate_setdescription(void *owner, const char *text)
{
    struct exit_data *exit_data;
    exit_data = (struct exit_data *)owner;
    free_string(exit_data->description);
    exit_data->description = str_dup(text);
    return;
 }
