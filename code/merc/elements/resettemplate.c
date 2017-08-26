#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"

/*
        switch (reset->command) {
          case 'M':
          case 'O':
              rVnum = reset->arg3;
              break;

          case 'P':
          case 'G':
          case 'E':
              break;

          case 'D':
              room_idx = get_room_index((rVnum = reset->arg1));
              if (reset->arg2 < 0
                  || reset->arg2 >= MAX_DIR || !room_idx
                  || ((pexit = room_idx->exit[reset->arg2]) == NULL)
                  || !IS_SET(pexit->rs_flags, EX_ISDOOR)) {
                  bug(fp, "Load_resets: 'D': exit %d, room %d not door.", reset->arg2, reset->arg1);
                  ABORT;
              }

              switch (reset->arg3) {
                default: 
                    bug(fp, "Load_resets: 'D': bad 'locks': %d.", reset->arg3); 
                    break;
                case 0: 
                    break;
                case 1: 
                    SET_BIT(pexit->rs_flags, EX_CLOSED);
                    SET_BIT(pexit->exit_info, EX_CLOSED); 
                    break;
                case 2: 
                    SET_BIT(pexit->rs_flags, EX_CLOSED | EX_LOCKED);
                    SET_BIT(pexit->exit_info, EX_CLOSED | EX_LOCKED); 
                    break;
              }
              break;

          case 'R':
              rVnum = reset->arg1;
              break;
        }

        if (rVnum == -1) {
        bug(fp, "load_resets : rVnum == -1");
        ABORT;
        }

        if (reset->command != 'D')
        new_reset(get_room_index(rVnum), reset);
        else
        free_reset_data(reset);

        struct reset_data *new_reset_data(void)
        {
        struct reset_data *reset;

        if (!reset_free) {
        reset = alloc_perm((unsigned int)sizeof(*reset));
        top_reset++;
        } else {
        reset = reset_free;
        reset_free = reset_free->next;
        }

        reset->next = NULL;
        reset->command = 'X';
        reset->arg1 = 0;
        reset->arg2 = 0;
        reset->arg3 = 0;
        reset->arg4 = 0;

        return reset;
        }



        void free_reset_data(struct reset_data *reset)
        {
        reset->next = reset_free;
        reset_free = reset;
        return;
        }
*/

struct reset_data *resetdata_new()
{
    struct reset_data *templatedata;

    templatedata = malloc(sizeof(struct reset_data));
    assert(templatedata != NULL);
    memset(templatedata, 0, sizeof(struct reset_data));
    return templatedata;
}

struct reset_data *resetdata_clone(struct reset_data *source)
{
    struct reset_data *clone;

    clone = malloc(sizeof(struct reset_data));
    assert(clone != NULL);
    memset(clone, 0, sizeof(struct reset_data));

    clone->command = source->command;
    clone->arg1 = source->arg1;
    clone->arg2 = source->arg2;
    clone->arg3 = source->arg3;
    clone->arg4 = source->arg4;

    return clone;
}

struct reset_data *resetdata_deserialize(const struct array_list *data)
{
    struct reset_data *templatedata;
    const char *entry;

    templatedata = malloc(sizeof(struct reset_data));
    assert(templatedata != NULL);
    memset(templatedata, 0, sizeof(struct reset_data));

    deserialize_assign_char(data, templatedata->command, "command");
    deserialize_assign_long(data, templatedata->arg1, "arg1");
    deserialize_assign_int(data, templatedata->arg2, "arg2");
    deserialize_assign_long(data, templatedata->arg3, "arg3");
    deserialize_assign_int(data, templatedata->arg4, "arg4");

    return templatedata;
}

struct array_list *resetdata_serialize(const struct reset_data *obj)
{
    struct array_list *answer;

    answer = kvp_create_array(5);

    serialize_take_string(answer, "command", int_to_string((int)obj->command));
    serialize_take_string(answer, "arg1", long_to_string(obj->arg1));
    serialize_take_string(answer, "arg2", int_to_string(obj->arg2));
    serialize_take_string(answer, "arg3", long_to_string(obj->arg3));
    serialize_take_string(answer, "arg4", int_to_string(obj->arg4));

    return answer;
}

void resetdata_free(struct reset_data *templatedata)
{
    assert(templatedata != NULL);
    free(templatedata);
    return;
}
