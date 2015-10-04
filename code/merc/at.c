#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"


/** imports */
extern bool expand_cmd(CHAR_DATA * vch, const char *arg, char *buf, char find);



CHAR_DATA *get_random_npc(CHAR_DATA *ch, AREA_DATA *area)
{
    CHAR_DATA *vch = NULL;
    CHAR_DATA *wch;
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

CHAR_DATA *get_random_pc(CHAR_DATA *ch, AREA_DATA *area)
{
    CHAR_DATA *vch = NULL;
    CHAR_DATA *wch;
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

void do_at(CHAR_DATA *ch, const char *argument)
{
    ROOM_INDEX_DATA *location = NULL;
    ROOM_INDEX_DATA *original;
    CHAR_DATA *wch = NULL;
    CHAR_DATA *vch = NULL;
    GAMEOBJECT *on;
    char arg[MIL];
    char cmd[MSL];

    DENY_NPC(ch)

        argument = one_argument(argument, arg);
    if (arg[0] == '\0' || argument[0] == '\0'
        || arg[0] == '?' || !str_cmp(arg, "help")) {
        show_help(ch->desc, "at_imm_command", NULL);
        return;
    }

    if (!str_prefix(arg, "random")) {
        AREA_DATA *area;
        char check[MIL];

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
