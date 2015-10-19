#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"
#include "help.h"

/** imports */
extern bool expand_cmd(struct char_data * vch, const char *arg, char *buf, char find);



struct char_data *get_random_npc(struct char_data *ch, struct area_data *area)
{
    struct char_data *vch = NULL;
    struct char_data *wch;
    int count = 0;

    for (wch = char_list; wch != NULL; wch = wch->next) {
        if (IS_NPC(wch)
            && can_see(ch, wch)
            && wch->in_room != NULL
            && !IS_SET(wch->in_room->room_flags, ROOM_NORANDOM)
            && (area == NULL || wch->in_room->area == area)
            && number_range(0, count) == 0) {
            vch = wch;
            count++;
        }
    }

    return vch;
}

struct char_data *get_random_pc(struct char_data *ch, struct area_data *area)
{
    struct char_data *vch = NULL;
    struct char_data *wch;
    int count = 0;

    for (wch = char_list; wch != NULL; wch = wch->next) {
        if (!IS_NPC(wch)
            && can_see(ch, wch)
            && wch->in_room != NULL
            && !IS_SET(wch->in_room->room_flags, ROOM_NORANDOM)
            && (area == NULL || wch->in_room->area == area)
            && wch != ch
            && get_trust(wch) < get_trust(ch)
            && number_range(0, count) == 0) {
            vch = wch;
            count++;
        }
    }

    return vch;
}

void do_at(struct char_data *ch, const char *argument)
{
    struct room_index_data *location = NULL;
    struct room_index_data *original;
    struct char_data *wch = NULL;
    struct char_data *vch = NULL;
    struct gameobject *on;
    char arg[MAX_INPUT_LENGTH];
    char cmd[MAX_STRING_LENGTH];

    DENY_NPC(ch)

        argument = one_argument(argument, arg);
    if (arg[0] == '\0' || argument[0] == '\0'
        || arg[0] == '?' || !str_cmp(arg, "help")) {
        show_help(ch->desc, "at_imm_command", NULL);
        return;
    }

    if (!str_prefix(arg, "random")) {
        struct area_data *area;
        char check[MAX_INPUT_LENGTH];

        argument = one_argument(argument, arg);
        area = NULL;
        one_argument(argument, check);
        if (!str_prefix(check, "area")) {
            argument = one_argument(argument, check);
            area = ch->in_room->area;
        }

        if (!str_prefix(arg, "room")) {
            location = get_random_room(ch, area);
        } else if (!str_prefix(arg, "mob")) {
            vch = get_random_npc(ch, area);
            location = vch->in_room;
        } else if (!str_prefix(arg, "char") || !str_prefix(arg, "player")) {
            vch = get_random_pc(ch, area);
            location = vch->in_room;
        }
    } else {
        location = find_location(ch, arg);
    }

    if (location == NULL) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, location) && room_is_private(location) && get_trust(ch) < LEVEL_IMMORTAL) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    original = ch->in_room;
    on = ch->on;

    char_from_room(ch);
    char_to_room(ch, location);
    if (vch != NULL) {
        if (expand_cmd(vch, argument, cmd, '@'))
            interpret(ch, cmd);
    } else {
        interpret(ch, argument);
    }
    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (wch = char_list; wch != NULL; wch = wch->next) {
        if (wch == ch) {
            char_from_room(ch);
            char_to_room(ch, original);
            ch->on = on;
            break;
        }
    }

    return;
}
