#include "merc.h"
#include "tables.h"
#include "interp.h"
#include "lookup.h"
#include "olc.h"
#include "help.h"

extern char *flag_string(const struct flag_type *flag_table, long bits);

/***************************************************************************
 *	declarations
 ***************************************************************************/
typedef void FLAG_CMD (struct char_data *ch, void *target, const char *argument);

/* flag command map */
struct flag_cmd_map {
    const char *flag;
    const struct flag_type *table;
    FLAG_CMD *flag_fn;
};

/* utility functions */
static void flag_list(struct char_data * ch, const struct flag_cmd_map *flag_table);
static void flag_list_value(struct char_data * ch, const struct flag_type *flag_table);

static void flag_set(struct char_data * ch, void *target, char *title, const struct flag_cmd_map *table, const char *argument);

/* top-level functions */
static void flag_set_char(struct char_data * ch, const char *argument);
static void flag_set_room(struct char_data * ch, const char *argument);
static void flag_set_obj(struct char_data * ch, const char *argument);


/* mob specific flag functions */
static void flag_char_act(struct char_data * ch, void *target, const char *argument);
static void flag_char_off(struct char_data * ch, void *target, const char *argument);
static void flag_char_form(struct char_data * ch, void *target, const char *argument);
static void flag_char_part(struct char_data * ch, void *target, const char *argument);

/* shared between mob and char */
static void flag_char_aff(struct char_data * ch, void *target, const char *argument);
static void flag_char_imm(struct char_data * ch, void *target, const char *argument);
static void flag_char_res(struct char_data * ch, void *target, const char *argument);
static void flag_char_vuln(struct char_data * ch, void *target, const char *argument);

/* char specific flag functions */
static void flag_char_plr(struct char_data * ch, void *target, const char *argument);
static void flag_char_comm(struct char_data * ch, void *target, const char *argument);
static void flag_char_deathroom(struct char_data * ch, void *target, const char *argument);

/* room flag functions */
static void flag_room_room(struct char_data * ch, void *target, const char *argument);

/* object flag functions */
static void flag_obj_extra(struct char_data * ch, void *target, const char *argument);
static void flag_obj_extra2(struct char_data * ch, void *target, const char *argument);
static void flag_obj_wpn_class(struct char_data * ch, void *target, const char *argument);
static void flag_obj_wpn_flags(struct char_data * ch, void *target, const char *argument);
static void flag_obj_wpn_damage(struct char_data * ch, void *target, const char *argument);


static const struct flag_cmd_map mob_set_flags[] =
{
    { "act",  act_flags,	flag_char_act  },
    { "off",  off_flags,	flag_char_off, },
    { "form", form_flags,	flag_char_form },
    { "part", part_flags,	flag_char_part },
    { "aff",  affect_flags, flag_char_aff  },
    { "imm",  imm_flags,	flag_char_imm  },
    { "res",  res_flags,	flag_char_res  },
    { "vuln", vuln_flags,	flag_char_vuln },
    { "",	  NULL,		NULL	       }
};

static const struct flag_cmd_map char_set_flags[] =
{
    { "plr",       plr_flags,    flag_char_plr	 },
    { "comm",      comm_flags,   flag_char_comm	 },
    { "deathroom", NULL,	     flag_char_deathroom },
    { "aff",       affect_flags, flag_char_aff	 },
    { "imm",       imm_flags,    flag_char_imm	 },
    { "res",       res_flags,    flag_char_res	 },
    { "vuln",      vuln_flags,   flag_char_vuln	 },
    { "",	       NULL,	     NULL		 }
};

static const struct flag_cmd_map room_set_flags[] =
{
    { "room", room_flags, flag_room_room },
    { "",	  NULL,	      NULL	     }
};


static const struct flag_cmd_map obj_set_flags[] =
{
    { "extra",	   extra_flags,	     flag_obj_extra	 },
    { "extra2",	   extra2_flags,     flag_obj_extra2	 },
    { "weapon_type",   weapon_class,     flag_obj_wpn_class	 },
    { "weapon_flags",  weapon_flag_type, flag_obj_wpn_flags	 },
    { "weapon_damage", damage_flags,     flag_obj_wpn_damage },
    { "",		   NULL,	     NULL		 }
};

/*	additionaly possible flags - socket_type, socket_flags,
 *      token_flags, furniture_flags */

void do_flag(struct char_data *ch, const char *argument)
{
    char type[MAX_INPUT_LENGTH];

    DENY_NPC(ch);

    argument = one_argument(argument, type);
    if (type[0] == '\0' || type[0] == '?' || !str_prefix(type, "help")) {
        show_help(ch->desc, "flag_syntax", NULL);
        return;
    }

    if (!str_prefix(type, "mob") || !str_prefix(type, "char")) {
        flag_set_char(ch, argument);
        return;
    }

    if (!str_prefix(type, "obj")) {
        flag_set_obj(ch, argument);
        return;
    }


    if (!str_prefix(type, "room")) {
        flag_set_room(ch, argument);
        return;
    }

    send_to_char("Invalid flag type.\n\r\n\r", ch);

    /* print the help msg */
    do_flag(ch, "");
    return;
}


void flag_set_char(struct char_data *ch, const char *argument)
{
    struct char_data *vch;
    char name[MAX_INPUT_LENGTH];

    argument = one_argument(argument, name);
    if ((vch = get_char_world(ch, name)) == NULL) {
        send_to_char("That character does not exist.", ch);
        return;
    }

    if (IS_NPC(vch))
        /* we have a mob...attempt to set the flag */
        flag_set(ch, vch, "Mob", mob_set_flags, argument);
    else
        flag_set(ch, vch, "Character", char_set_flags, argument);
    return;
}

void flag_set_obj(struct char_data *ch, const char *argument)
{
    struct gameobject *obj;
    char name[MAX_INPUT_LENGTH];

    argument = one_argument(argument, name);
    if ((obj = get_obj_here(ch, name)) == NULL) {
        send_to_char("That object is not here.", ch);
        return;
    }

    flag_set(ch, obj, "Object", obj_set_flags, argument);
    return;
}

void flag_set_room(struct char_data *ch, const char *argument)
{
    struct room_index_data *room;
    char index[MAX_INPUT_LENGTH];
    long vnum;

    one_argument(argument, index);

    if (is_number(index)
        && ((vnum = parse_long(index)) > 0)
        && ((room = get_room_index(vnum)) != NULL))
        argument = one_argument(argument, index);
    else
        room = ch->in_room;

    if (room == NULL) {
        send_to_char("That room does not exist.\n\r", ch);
        return;
    }

    flag_set(ch, room, "Room", room_set_flags, argument);
    return;
}

void flag_char_aff(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if ((value = flag_value(affect_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->affected_by, value);
            send_to_char("Affect flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->affected_by, value);
            send_to_char("Affect flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->affected_by, value);
            send_to_char("Affect flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_vuln(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if ((value = flag_value(vuln_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->vuln_flags, value);
            send_to_char("Vulnerability flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->vuln_flags, value);
            send_to_char("Vulnerability flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->vuln_flags, value);
            send_to_char("Vulnerability flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_res(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if ((value = flag_value(res_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->res_flags, value);
            send_to_char("Resistance flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->res_flags, value);
            send_to_char("Resistance flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->res_flags, value);
            send_to_char("Resistance flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_imm(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if ((value = flag_value(imm_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->imm_flags, value);
            send_to_char("Immunity flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->imm_flags, value);
            send_to_char("Immunity flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->imm_flags, value);
            send_to_char("Immunity flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_part(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if (!IS_NPC(vch)) {
        send_to_char("You can only set Part flags on a mob.\n\r", ch);
        return;
    }

    if ((value = flag_value(part_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->parts, value);
            send_to_char("Part flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->parts, value);
            send_to_char("Part flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->parts, value);
            send_to_char("Part flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_form(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if (!IS_NPC(vch)) {
        send_to_char("You can only set Form flags on a mob.\n\r", ch);
        return;
    }

    if ((value = flag_value(form_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->form, value);
            send_to_char("Form flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->form, value);
            send_to_char("Form flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->form, value);
            send_to_char("Form flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_off(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if (!IS_NPC(vch)) {
        send_to_char("You can only set Offense flags on a mob.\n\r", ch);
        return;
    }

    if ((value = flag_value(off_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->off_flags, value);
            send_to_char("Offense flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->off_flags, value);
            send_to_char("Offense flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->off_flags, value);
            send_to_char("Offense flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_act(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if (!IS_NPC(vch)) {
        send_to_char("You can only set Act flags on a mob.\n\r", ch);
        return;
    }

    if ((value = flag_value(act_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->act, value);
            send_to_char("Act flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->act, value);
            send_to_char("Act flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->act, value);
            send_to_char("Act flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_deathroom(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    struct room_index_data *room;
    long value;

    if (IS_NPC(vch)) {
        send_to_char("You can only set Death Room on a player.\n\r", ch);
        return;
    }


    value = parse_long(argument);
    if (is_number(argument) && value > 0) {
        if ((room = get_room_index(value)) != NULL) {
            vch->deathroom = value;
            send_to_char("Death Room set.\n\r", ch);
        } else {
            send_to_char("That room number does not exist.\n\r", ch);
        }
    } else {
        if (vch->deathroom > 0) {
            vch->deathroom = 0;
            send_to_char("Death Room reset to 0.\n\r", ch);
            return;
        }

        send_to_char("Must supply a room number to set the Death Room.\n\r", ch);
    }

    return;
}

void flag_char_comm(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if (IS_NPC(vch)) {
        send_to_char("You can only set Communication flags on a player.\n\r", ch);
        return;
    }


    if ((value = flag_value(comm_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->comm, value);
            send_to_char("Communication flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->comm, value);
            send_to_char("Communication flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->comm, value);
            send_to_char("Communication flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_char_plr(struct char_data *ch, void *target, const char *argument)
{
    struct char_data *vch = (struct char_data *)target;
    long value;

    if (IS_NPC(vch)) {
        send_to_char("You can only set Player flags on a player.\n\r", ch);
        return;
    }


    if ((value = flag_value(plr_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(vch->act, value);
            send_to_char("Player flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(vch->act, value);
            send_to_char("Player flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(vch->act, value);
            send_to_char("Player flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_room_room(struct char_data *ch, void *target, const char *argument)
{
    struct room_index_data *room = (struct room_index_data *)target;
    long value;

    if ((value = flag_value(room_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(room->room_flags, value);
            send_to_char("Room flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(room->room_flags, value);
            send_to_char("Room flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(room->room_flags, value);
            send_to_char("Room flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_obj_extra(struct char_data *ch, void *target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)target;
    long value;

    if ((value = flag_value(extra_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(obj->extra_flags, value);
            send_to_char("Extra flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(obj->extra_flags, value);
            send_to_char("Extra flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(obj->extra_flags, value);
            send_to_char("Extra flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_obj_extra2(struct char_data *ch, void *target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)target;
    long value;

    if ((value = flag_value(extra2_flags, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(obj->extra2_flags, value);
            send_to_char("Extra2 flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(obj->extra2_flags, value);
            send_to_char("Extra2 flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(obj->extra2_flags, value);
            send_to_char("Extra2 flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_obj_wpn_class(struct char_data *ch, void *target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)target;
    long value;


    if (OBJECT_TYPE(obj) != ITEM_WEAPON) {
        send_to_char("You can only set weapon type on weapons.\n\r", ch);
        return;
    }

    if ((value = flag_value(weapon_class, argument)) != NO_FLAG) {
        obj->value[0] = value;
        send_to_char("Weapon class set.\n\r", ch);
        return;
    }

    send_to_char("That weapon class does not exists or is not settable.\n\r", ch);
    return;
}

void flag_obj_wpn_flags(struct char_data *ch, void *target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)target;
    long value;


    if (OBJECT_TYPE(obj) != ITEM_WEAPON) {
        send_to_char("You can only set weapon flags on weapons.\n\r", ch);
        return;
    }

    if ((value = flag_value(weapon_flag_type, argument)) != NO_FLAG) {
        if (*argument == '+') {
            SET_BIT(obj->value[4], value);
            send_to_char("Weapon flags added.\n\r", ch);
        } else if (*argument == '-') {
            REMOVE_BIT(obj->value[4], value);
            send_to_char("Weapon flags removed.\n\r", ch);
        } else {
            TOGGLE_BIT(obj->value[4], value);
            send_to_char("Weapon flags toggled.\n\r", ch);
        }

        return;
    }

    send_to_char("Those flags do not exists or are not settable.\n\r", ch);
    return;
}

void flag_obj_wpn_damage(struct char_data *ch, void *target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)target;
    long value;


    if (OBJECT_TYPE(obj) != ITEM_WEAPON) {
        send_to_char("You can only set damage type on weapons.\n\r", ch);
        return;
    }

    if ((value = flag_value(damage_flags, argument)) != NO_FLAG) {
        obj->value[3] = value;
        send_to_char("Damage type set.\n\r", ch);
        return;
    }

    send_to_char("That damage type does not exists or is not settable.\n\r", ch);
    return;
}

void flag_set(struct char_data *ch, void *target, char *title, const struct flag_cmd_map *table, const char *argument)
{
    char flag[MAX_INPUT_LENGTH];
    int idx;

    argument = one_argument(argument, flag);

    /* if we are asking for help at this level, then we need to list the
     * available flags for the type */
    if (flag[0] != '\0'
        && flag[0] != '?'
        && str_prefix(flag, "help")) {
        for (idx = 0; table[idx].flag[0] != '\0'; idx++) {
            if (!str_prefix(flag, table[idx].flag)) {
                /* we found the proper flag - do some stuff */

                if (argument[0] == '\0' || argument[0] == '?' || !str_prefix(argument, "help")) {
                    /* print usage for that flag */
                    if (table[idx].table != NULL) {
                        send_to_char("Available flags:", ch);
                        flag_list_value(ch, table[idx].table);
                    } else {
                        send_to_char("There are no flags for that stat.  You must provide a numeric value.\n\r", ch);
                    }

                    return;
                }

                /* call the function */
                (*table[idx].flag_fn)(ch, target, argument);
                return;
            }
        }

        /* if we havent returned by now, then we must have not found a match
         * so print a list of flags for the type */
    }

    printf_to_char(ch, "Available %s Flags:", title);

    flag_list(ch, table);
    return;
}

void flag_list(struct char_data *ch, const struct flag_cmd_map *flag_table)
{
    int idx;

    for (idx = 0; flag_table[idx].flag[0] != '\0'; idx++) {
        if ((idx % 6) == 0)
            send_to_char("\n\r  ", ch);
        printf_to_char(ch, "%-8.10s  ", flag_table[idx].flag);
    }

    send_to_char("\n\r", ch);
    return;
}

void flag_list_value(struct char_data *ch, const struct flag_type *flag_table)
{
    int idx;

    for (idx = 0; flag_table[idx].name != NULL; idx++) {
        if ((idx % 4) == 0)
            send_to_char("\n\r  ", ch);

        printf_to_char(ch, "%-13.15s  ", flag_table[idx].name);
    }

    send_to_char("\n\r", ch);
    return;
}
