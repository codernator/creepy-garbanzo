/***************************************************************************
 *  File: olc_redit.c                                                      *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/


/***************************************************************************
 *	includes
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "object.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"
#include "interp.h"
#include "help.h"


/***************************************************************************
 *	external functions
 ***************************************************************************/
extern int wear_bit(int loc);


/***************************************************************************
 *	local functions
 ***************************************************************************/
static bool change_exit(struct char_data *ch, const char *argument, int door);
static void redit_extradesc_add(struct char_data *ch, const char *argument);
static void redit_extradesc_edit(struct char_data *ch, const char *argument);
static void redit_extradesc_delete(struct char_data *ch, const char *argument);
static void redit_extradesc_help(struct char_data *ch, const char *argument);
static const struct editor_cmd_type extra_cmd_table[] =
    {
        { "add",    redit_extradesc_add,    "add <key>    - add a new extra desc and enter string editor." },
        { "edit",   redit_extradesc_edit,   "edit <key>   - enter string editor for given key."},
        { "remove", redit_extradesc_delete, "remove <key> - remove extra desc by given key." },
        { "?",      redit_extradesc_help,   "?            - show this help."},
        { "", NULL, "" }
    };

static void redit_reset(struct char_data *ch, const char *argument);
static void redit_create(struct char_data *ch, const char *argument);
static void redit_clone(struct char_data *ch, const char *argument);
static void redit_edit(struct char_data *ch, const char *argument);
static void redit_delete(struct char_data *ch, const char *argument);
static void redit_help(struct char_data *ch, const char *argument);
static const struct editor_cmd_type redit_cmd_table[] =
    {
        { "create", redit_create,  "create <vnum>            - create a new room in the current area." },
        { "clone",  redit_clone,   "clone <source> <target>  - clone an existing room to a new room." },
        { "edit",   redit_edit,    "edit <vnum>              - enter edit mode for an existing room."},
        { "remove", redit_delete,  "remove <vnum>            - remove an existing room." },
        { "reset",  redit_reset,   "reset <vnum>             - reset a room." },
        { "?",      redit_help,    "?                        - show this help."},
        { "", NULL, "" }
    };



void do_redit(struct char_data *ch, const char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmdindex = 0;

    DENY_NPC(ch);

    argument = one_argument(argument, command);

    if (command[0] == '\0') {
        redit_help(ch, argument);
        return;
    }

    if (is_number(argument)) {
        // edit room <vnum> simpler than edit room edit <vnum>
        redit_edit(ch, argument);
    }

    for (cmdindex = 0; redit_cmd_table[cmdindex].do_fn != NULL; cmdindex++) {
        if (command[0] == redit_cmd_table[cmdindex].name[0]
            && !str_cmp(command, redit_cmd_table[cmdindex].name)) {
            (*redit_cmd_table[cmdindex].do_fn)(ch, argument);
            return;
        }
    }

    send_to_char("`@COMMAND NOT FOUND``\n\r", ch);
    return;
}



void redit_reset(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    unsigned long vnum;
    char arg[MAX_STRING_LENGTH];

    (void)one_argument(argument, arg);
    if (is_number(arg)){
        vnum = parse_unsigned_long(arg);
        room = roomtemplate_getbyvnum(vnum);
        if (room == NULL) {
            printf_to_char(ch, "Room %lu does not exist.\n\r", vnum);
            return;
        }
    } else {
        room = ch->in_room;
    }

    if (!IS_BUILDER(ch, room->area)) {
        send_to_char("Insufficient security to edit rooms.\n\r", ch);
        return;
    }

    reset_room(room);
    send_to_char("Room reset.\n\r", ch);
    return;
}

void redit_create(struct char_data *ch, const char *argument)
{
    struct area_data *area;
    struct roomtemplate *room;
    unsigned long vnum;

    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  edit room create [vnum]\n\r", ch);
        return;
    }

    area = area_getbycontainingvnum(vnum);
    if (!area) {
        printf_to_char(ch, "REdit: There is no area containing vnum %lu.\n\r", vnum);
        return;
    }

    if (!IS_BUILDER(ch, area)) {
        printf_to_char(ch, "REdit: You don't have access to build in the area contained by vnum %lu.\n\r", vnum);
        return;
    }

    if (get_room_index(vnum)) {
        printf_to_char(ch, "REdit: Room vnum %lu already exists.\n\r", vnum);
        return;
    }

    room = roomtemplate_new(vnum);
    room->area = area;

    if (value > top_vnum_room)
        top_vnum_room = value;

    send_to_char("Room created.\n\r", ch);

    char_from_room(ch);
    char_to_room(ch, room);
    SET_BIT(area->area_flags, AREA_CHANGED);
    ch->desc->ed_data = (void *)room;
    ch->desc->editor = ED_ROOM;
    return;
}

void redit_clone(struct char_data *ch, const char *argument)
{
    struct roomtemplate *source;
    struct roomtemplate *target;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    struct area_data *targetarea;
    unsigned long targetvnum;
    unsigned long sourcevnum;

    argument = one_argument(argument, arg1);
    (void)one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        send_to_char("REdit syntax: clone [target vnum] <source vnum>\n\r", ch);
        return;
    }

    targetvnum = parse_int(arg1);
    if (roomtemplate_getbyvnum(targetvnum) != NULL) {
        printf_to_char(ch, "REdit:  Target vnum %lu already exists.\n\r", targetvnum);
        return;
    }
    targetarea = area_getbycontainingvnum(targetvnum);
    if (!targetarea) {
        printf_to_char(ch, "REdit: There is no area containing vnum %lu.\n\r", targetvnum);
        return;
    }

    if (!IS_BUILDER(ch, targetarea)) {
        printf_to_char(ch, "REdit: You don't have access to build in the area contained by vnum %lu.\n\r", targetvnum);
        return;
    }

    if (arg2[0] != '\0')
    {
        if (!is_number(arg2))
        {
            send_to_char("REdit:  The source vnum must be a number.\n\r", ch);
            return;
        }
        sourcevnum = parse_int(arg2);
        source = roomtemplate_getbyvnum(sourcevnum);
        if (source == NULL)
        {
            printf_to_char(ch, "REdit:  Source vnum %lu does not yet exist.\n\r", sourcevnum);
            return;
        }
    }
    else
    {
        EDIT_ROOM(ch, source);
        sourcevnum = source->vnum;
    }

    target = roomtemplate_clone(source, targetvnum, targetarea);
    ch->desc->ed_data = (void *)target;
    if (targetvnum > top_vnum_room)
        top_vnum_room = targetvnum;

    char_from_room(ch);
    char_to_room(ch, target);
    SET_BIT(targetarea->area_flags, AREA_CHANGED);
    ch->desc->editor = ED_ROOM;
    printf_to_char(ch, "Room %lu cloned. You are now editing room %lu.\n\r", sourcevnum, targetvnum);
    return;
}

void redit_edit(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    unsigned long vnum;

    if (argument[0] == '\0') {
        EDIT_ROOM(ch, room);

        if (!IS_BUILDER(ch, room->area)) {
            send_to_char("REdit : insufficient security to edit rooms.\n\r", ch);
            return;
        }
        ch->desc->ed_data = (void *)room;
        ch->desc->editor = ED_ROOM;
        printf_to_char(ch, "You are now editing room %lu.\n\r", room->vnum);
        return;
    }

    if (!is_number(argument)) {
        send_to_char("REdit syntax: edit [target vnum]\n\r", ch);
        return;
    }

    vnum = parse_unsigned_long(argument);
    room = roomtemplate_getbyvnum(vnum);
    if (room == NULL) {
        printf_to_char(ch, "REdit:  Room vnum %lu does not yet exist.\n\r", vnum);
        return;
    }

    char_from_room(ch);
    char_to_room(ch, room);
    ch->desc->ed_data = (void *)room;
    ch->desc->editor = ED_ROOM;
    printf_to_char(ch, "You are now editing room %lu.\n\r", room->vnum);
    return;
}

void redit_rlist(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    struct buf_type *buf;
    char *unclr;
    char arg[MAX_INPUT_LENGTH];
    bool found;
    int col = 0;

    (void)one_argument(argument, arg);

    buf = new_buf();
    found = false;

    for (room = roomtemplate_iterator_start(&roomtemplate_empty_filter);
            room != NULL;
            room = roomtemplate_iterator(room, &roomtemplate_empty_filter)) {
        found = true;

        unclr = uncolor_str(capitalize(room->name));
        printf_buf(buf, "[`1%5d``] %-17.16s", room->vnum, unclr);
        free_string(unclr);

        if (++col % 3 == 0)
            add_buf(buf, "\n\r");
    }

    if (!found) {
        send_to_char("Room(s) not found in this area.\n\r", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf, "\n\r");

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
    return false;
}

void redit_show(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    struct gameobject *obj;
    struct char_data *rch;
    struct extra_descr_data *ed;
    struct affect_data *paf;
    char buf[MAX_STRING_LENGTH];
    int door;
    bool fcnt;

    EDIT_ROOM(ch, room);

    printf_to_char(ch, "`&Name``:        [%s]\n\r`&Area``:        [%5d] %s\n\r",
                   room->name, room->area->vnum, room->area->name);
    printf_to_char(ch, "`&Vnum``:        [%5d]\n\r`&Sector``:      [%s]\n\r",
                   room->vnum, flag_string(sector_flags, room->sector_type));
    printf_to_char(ch, "`&Room flags``:  [%s]\n\r",
                   flag_string(room_flags, room->room_flags));

    if (room->heal_rate != 100 || room->mana_rate != 100)
        printf_to_char(ch, "`&Health rate``: [%d]\n\r`&Mana rate``:   [%d]\n\r",
                       room->heal_rate, room->mana_rate);

    if (!IS_NULLSTR(room->owner))
        printf_to_char(ch, "`&Owner``:       [%s]\n\r", room->owner);

    printf_to_char(ch, "`&Description``:\n\r%s", room->description);

    ed = extradescrdata_iterator_startroom(room);
    if (ed != NULL) {
        struct extra_descr_data *ednext;

        printf_to_char(ch, "`&Desc Kwds``:   [");
        while (ed != NULL) {
            ednext = extradescrdata_iterator(ed);
            printf_to_char(ch, ed->keyword);
            if (ednext != NULL)
                printf_to_char(ch, " ");
            ed = ednext;
        }
        printf_to_char(ch, "]\n\r");
    }

    paf = affecttemplate_iterator_startroom(room);
    if (paf != NULL) {
        struct dynamic_skill *skill;
        int iter = 0;

        send_to_char("\n\r`&Number Level  Spell          ``\n\r", ch);
        send_to_char("`1------ ------ -------------------``\n\r", ch);
        while (paf != NULL) {
            if ((skill = resolve_skill_affect(paf)) != NULL) {
                printf_to_char(ch, "[%4d] %-6d %s\n\r",
                                iter++,
                                paf->level,
                                skill->name);
            }
            paf = affecttemplate_iterator(paf);
        }
    }

    printf_to_char(ch, "`&Characters``:  [");
    fcnt = false;
    for (rch = room->people; rch; rch = rch->next_in_room) {
        one_argument(rch->name, buf);
        if (rch->next_in_room)
            printf_to_char(ch, "%s ", buf);
        else
            printf_to_char(ch, "%s]\n\r", buf);
        fcnt = true;
    }

    if (!fcnt)
        send_to_char("none]\n\r", ch);

    printf_to_char(ch, "`&Objects``:     [");
    fcnt = false;
    for (obj = room->contents; obj; obj = obj->next_content) {
        one_argument(object_name_get(obj), buf);
        if (obj->next_content)
            printf_to_char(ch, "%s ", buf);
        else
            printf_to_char(ch, "%s]\n\r", buf);

        fcnt = true;
    }

    if (!fcnt)
        send_to_char("none]\n\r", ch);

    send_to_char("\n\r`&EXITS:\n\r", ch);
    for (door = 0; door < MAX_DIR; door++) {
        struct exit_data *pexit;

        if ((pexit = room->exit[door])) {
            char word[MAX_INPUT_LENGTH];
            char reset_state[MAX_STRING_LENGTH];
            const char *state;
            int iter;
            int length;

            printf_to_char(ch, "`#%s``:\n\r [%5d] Key: [%5d] ",
                           capitalize(dir_name[door]),
                           pexit->to_room ? pexit->to_room->vnum : 0,
                           pexit->key);
            /*
             * Format up the exit info.
             * Capitalize all flags that are not part of the reset info.
             */
            strcpy(reset_state, flag_string(exit_flags, pexit->rs_flags));
            state = flag_string(exit_flags, pexit->exit_info);

            fcnt = false;
            printf_to_char(ch, " Exit flags: [");
            for (;; ) {
                state = one_argument(state, word);

                if (word[0] != '\0') {
                    if (str_infix(word, reset_state)) {
                        length = (int)strlen(word);
                        for (iter = 0; iter < length; iter++)
                            word[iter] = UPPER(word[iter]);
                    }
                    if (!fcnt) {
                        printf_to_char(ch, "%s", word);
                        fcnt = true;
                    } else {
                        printf_to_char(ch, " %s", word);
                    }
                } else {
                    printf_to_char(ch, "]\n\r");
                    break;
                }
            }

            if (pexit->keyword && pexit->keyword[0] != '\0')
                printf_to_char(ch, " `&Keywords``: [%s]\n\r", pexit->keyword);

            if (pexit->description && pexit->description[0] != '\0')
                printf_to_char(ch, " %s", pexit->description);
        }
    }

    return false;
}

void redit_north(struct char_data *ch, const char *argument)
{
    return change_exit(ch, argument, DIR_NORTH);
}

void redit_south(struct char_data *ch, const char *argument)
{
    return change_exit(ch, argument, DIR_SOUTH);
}

void redit_east(struct char_data *ch, const char *argument)
{
    return change_exit(ch, argument, DIR_EAST);
}

void redit_west(struct char_data *ch, const char *argument)
{
    return change_exit(ch, argument, DIR_WEST);
}

void redit_up(struct char_data *ch, const char *argument)
{
    return change_exit(ch, argument, DIR_UP);
}

void redit_down(struct char_data *ch, const char *argument)
{
    return change_exit(ch, argument, DIR_DOWN);
}

void redit_ed(struct char_data *ch, const char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmdindex = 0;

    argument = one_argument(argument, command);

    if (command[0] == '\0') {
        redit_extradesc_help(ch, argument);
        return false;
    }

    for (cmdindex = 0; extra_cmd_table[cmdindex].do_fn != NULL; cmdindex++) {
        if (command[0] == extra_cmd_table[cmdindex].name[0]
            && !str_cmp(command, extra_cmd_table[cmdindex].name)) {
            (*extra_cmd_table[cmdindex].do_fn)(ch, argument);
            return true;
        }
    }

    send_to_char("``COMMAND NOT FOUND``\n\r", ch);
    return false;
}

void redit_name(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;

    EDIT_ROOM(ch, room);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  name [name]\n\r", ch);
        return false;
    }

    roomtemplate_setname(room, argument);
    send_to_char("Name set.\n\r", ch);
    return true;
}

void redit_desc(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;

    EDIT_ROOM(ch, room);
    if (argument[0] != '\0') {
        olc_roomtemplate_setdescription(room, argument);
        send_to_char("Room description set.", ch);
        return true;
    }

    olc_start_string_editor(ch, room, olc_roomtemplate_getdescription, olc_roomtemplate_setdescription);
    return true;
}

void redit_heal(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;

    EDIT_ROOM(ch, room);
    if (is_number(argument)) {
        room->heal_rate = parse_int(argument);
        send_to_char("Heal rate set.\n\r", ch);
        return true;
    }

    send_to_char("Syntax : heal <#xnumber>\n\r", ch);
    return false;
}

void redit_mana(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;

    EDIT_ROOM(ch, room);
    if (is_number(argument)) {
        room->mana_rate = parse_int(argument);
        send_to_char("Mana rate set.\n\r", ch);
        return true;
    }

    send_to_char("Syntax : mana <#xnumber>\n\r", ch);
    return false;
}

void redit_owner(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;

    EDIT_ROOM(ch, room);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  owner [owner]\n\r", ch);
        send_to_char("         owner none\n\r", ch);
        return false;
    }

    free_string(room->owner);
    if (!str_cmp(argument, "none"))
        room->owner = str_dup("");
    else
        room->owner = str_dup(argument);

    send_to_char("Owner set.\n\r", ch);
    return true;
}

void redit_room(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    int value;

    EDIT_ROOM(ch, room);

    if ((value = flag_value(room_flags, argument)) == NO_FLAG) {
        send_to_char("Syntax: room [flags]\n\r", ch);
        return false;
    }

    TOGGLE_BIT(room->room_flags, value);

    send_to_char("Room flags toggled.\n\r", ch);
    return true;
}

void redit_sector(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    int value;

    EDIT_ROOM(ch, room);

    if ((value = flag_value(sector_flags, argument)) == NO_FLAG) {
        send_to_char("Syntax: sector [type]\n\r", ch);
        return false;
    }

    room->sector_type = value;
    send_to_char("Sector type set.\n\r", ch);

    return true;
}

void redit_addaffect(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    struct affect_data af;
    struct dynamic_skill *skill;
    char arg[MAX_STRING_LENGTH];

    EDIT_ROOM(ch, room);
    argument = one_argument(argument, arg);
    skill = skill_lookup(arg);
    argument = one_argument(argument, arg);

    if (!is_number(arg)
        || skill == NULL
        || skill->target != TAR_ROOM) {
        send_to_char("Syntax: addaffect [spell] [level]\n\r", ch);
        return false;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = parse_int(arg);
    af.duration = -1;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_room(room, &af);
    send_to_char("Affect added.\n\r", ch);

    return true;
}

void redit_delaffect(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    struct affect_data *paf;
    struct affect_data *paf_next;
    char arg[MAX_STRING_LENGTH];
    int number;
    int count;

    EDIT_ROOM(ch, room);
    argument = one_argument(argument, arg);
    if (!is_number(arg)) {
        send_to_char("Syntax: delaffect [# affect]\n\r", ch);
        return false;
    }

    number = parse_int(arg);
    count = 0;
    for (paf = room->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;

        if (count++ == number) {
            affect_remove_room(room, paf);
            printf_to_char(ch, "Affect #%d removed.\n\r", number);
            return true;
        }
    }

    send_to_char("Affect number does not exist.\n\r", ch);
    return false;
}

void redit_mreset(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    struct mob_index_data *mob;
    struct char_data *newmob;
    struct reset_data *pReset;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    EDIT_ROOM(ch, room);

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char("Syntax:  mreset <vnum> <max #x> <mix #x>\n\r", ch);
        return false;
    }

    if (!(mob = get_mob_index(parse_int(arg)))) {
        send_to_char("REdit: No mobile has that vnum.\n\r", ch);
        return false;
    }

    if (mob->area != room->area) {
        send_to_char("REdit: No such mobile in this area.\n\r", ch);
        return false;
    }

    /*
     * Create the mobile reset.
     */
    pReset = new_reset_data();
    pReset->command = 'M';
    pReset->arg1 = mob->vnum;
    pReset->arg2 = is_number(arg2) ? parse_int(arg2) : MAX_MOB;
    pReset->arg3 = room->vnum;
    pReset->arg4 = is_number(argument) ? parse_int(argument) : 1;
    add_reset(room, pReset, 0);

    /*
     * Create the mobile.
     */
    newmob = create_mobile(mob);
    char_to_room(newmob, room);

    printf_to_char(ch, "%s(%d) has been loaded and added to resets.\n\rThere will be a maximum of %d loaded to this room.\n\r",
                   capitalize(mob->short_descr),
                   mob->vnum,
                   pReset->arg2);

    act("$n has created $N!", ch, NULL, newmob, TO_ROOM);
    return true;
}

void redit_oreset(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    struct objecttemplate *obj;
    struct gameobject *newobj;
    struct gameobject *to_obj;
    struct char_data *to_mob;
    struct reset_data *pReset;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int olevel = 0;

    EDIT_ROOM(ch, room);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        send_to_char("Syntax:  oreset <vnum> <args>\n\r", ch);
        send_to_char("        -no_args               = into room\n\r", ch);
        send_to_char("        -<obj_name>            = into obj\n\r", ch);
        send_to_char("        -<mob_name> <wear_loc> = into mob\n\r", ch);
        return false;
    }

    if (!(obj = objecttemplate_getbyvnum(parse_int(arg1)))) {
        send_to_char("REdit: No object has that vnum.\n\r", ch);
        return false;
    }

    if (obj->area != room->area) {
        send_to_char("REdit: No such object in this area.\n\r", ch);
        return false;
    }

    /*
     * Load into room.
     */
    if (arg2[0] == '\0') {
        pReset = new_reset_data();
        pReset->command = 'O';
        pReset->arg1 = obj->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = room->vnum;
        pReset->arg4 = 0;
        add_reset(room, pReset, 0 /* Last slot*/);

        newobj = create_object(obj);
        obj_to_room(newobj, room);

        printf_to_char(ch, "%s(%d) has been loaded and added to resets.\n\r",
                       capitalize(obj->short_descr),
                       obj->vnum);
        act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
        return true;
    }

    /*
     * Load into object's inventory.
     */
    if (argument[0] == '\0'
        && ((to_obj = get_obj_list(ch, arg2, room->contents)) != NULL)) {
        pReset = new_reset_data();
        pReset->command = 'P';
        pReset->arg1 = obj->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = to_obj->objtemplate->vnum;
        pReset->arg4 = 1;
        add_reset(room, pReset, 0);

        newobj = create_object(obj);
        newobj->cost = 0;
        obj_to_obj(newobj, to_obj);

        printf_to_char(ch, "%s(%d) has been loaded into "
                       "%s(%d) and added to resets.\n\r",
                       capitalize(OBJECT_SHORT(newobj)),
                       newobj->objtemplate->vnum,
                       OBJECT_SHORT(to_obj),
                       to_obj->objtemplate->vnum);
        act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
        return true;
    }

    if ((to_mob = get_char_room(ch, arg2)) != NULL) {
        int wear_loc;

        if ((wear_loc = flag_value(wear_loc_flags, argument)) == NO_FLAG) {
            send_to_char("REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch);
            return false;
        }

        /*
         * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
         */
        if (!IS_SET(obj->wear_flags, wear_bit(wear_loc))) {
            printf_to_char(ch, "%s(%d) has wear flags: [%s]\n\r",
                           capitalize(obj->short_descr),
                           obj->vnum,
                           flag_string(wear_flags, obj->wear_flags));
            return false;
        }

        /*
         * Can't load into same position.
         */
        if (get_eq_char(to_mob, wear_loc)) {
            send_to_char("REdit:  Object already equipped.\n\r", ch);
            return false;
        }

        pReset = new_reset_data();
        pReset->arg1 = obj->vnum;
        pReset->arg2 = wear_loc;

        if (pReset->arg2 == WEAR_NONE)
            pReset->command = 'G';
        else
            pReset->command = 'E';
        pReset->arg3 = wear_loc;
        add_reset(room, pReset, 0);

        olevel = URANGE(0, to_mob->level - 2, LEVEL_HERO);
        newobj = create_object(obj);

        if (IS_SHOPKEEPER(to_mob)) {
            switch (obj->item_type) {
              default:
                  olevel = 0;
                  break;
              case ITEM_PILL:
                  olevel = number_range(0, 10);
                  break;
              case ITEM_POTION:
                  olevel = number_range(0, 10);
                  break;
              case ITEM_SCROLL:
                  olevel = number_range(5, 15);
                  break;
              case ITEM_WAND:
                  olevel = number_range(10, 20);
                  break;
              case ITEM_STAFF:
                  olevel = number_range(15, 25);
                  break;
              case ITEM_ARMOR:
                  olevel = number_range(5, 15);
                  break;
              case ITEM_WEAPON:
                  if (pReset->command == 'G')
                      olevel = number_range(5, 15);
                  else
                      olevel = number_fuzzy(olevel);
                  break;
            }

            newobj = create_object(obj);
            if (pReset->arg2 == WEAR_NONE)
                SET_BIT(newobj->extra_flags, ITEM_INVENTORY);
        } else {
            newobj = create_object(obj);
        }

        obj_to_char(newobj, to_mob);
        if (pReset->command == 'E')
            equip_char(to_mob, newobj, (int)pReset->arg3);

        printf_to_char(ch, "%s(%d) has been loaded "
                       "%s of %s(%d) and added to resets.\n\r",
                       capitalize(obj->short_descr),
                       obj->vnum,
                       flag_string(wear_loc_strings, pReset->arg3),
                       to_mob->short_descr,
                       to_mob->mob_idx->vnum);
    }


    send_to_char("REdit:  That mobile isn't here.\n\r", ch);
    return false;
}

void redit_flagall(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;                          /* individual room to edit */
    struct area_data *area;                                /* area being edited */
    struct buf_type *buf;                                    /* text to return to ch */
    char *unclr;                                    /* uncolored room name */
    char rFlag[MAX_INPUT_LENGTH];                                /* name of room flag to set */
    int iFlag;                                      /* int value of room flag */
    char rType[MAX_INPUT_LENGTH];                                /* type of set (on, off, toggle) */
    bool found;                                     /* room exists? */
    long vnum;                                      /* vnum of room */
    int col = 0;                                    /* display columns */

    argument = one_argument(argument, rFlag);
    argument = one_argument(argument, rType);

    if ((iFlag = flag_value(room_flags, rFlag)) == NO_FLAG) {
        send_to_char("Syntax:  flagall [flag] <toggle>\n\r", ch);
        send_to_char("        `&Toggle can be `@on`&, `!off`&, or left out to toggle``\n\r", ch);
        send_to_char("        Type '`^? room``' to see a list of available room flags.\n\r", ch);
        return false;
    }

    area = ch->in_room->area;
    buf = new_buf();
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((room = get_room_index(vnum))) {
            found = true;

            if (!strcmp(rType, "on"))
                SET_BIT(room->room_flags, iFlag);
            else if (!strcmp(rType, "off"))
                REMOVE_BIT(room->room_flags, iFlag);
            else
                TOGGLE_BIT(room->room_flags, iFlag);

            unclr = uncolor_str(capitalize(room->name));
            printf_buf(buf, "[");

            if (IS_SET(room->room_flags, iFlag))
                printf_buf(buf, "`@");
            else
                printf_buf(buf, "`!");

            printf_buf(buf, "%5d``] %-17.16s", vnum, unclr);
            free_string(unclr);

            if (++col % 3 == 0)
                add_buf(buf, "\n\r");
        }
    }

    if (!found) {
        send_to_char("Room(s) not found in this area.\n\r", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf, "\n\r");

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
    return true;
}

void redit_showrooms(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;                          /* individual room to edit */
    struct area_data *area;                                /* area being edited */
    struct buf_type *buf;                                    /* text to return to ch */
    char *unclr;                                    /* uncolored room name */
    char rFlag[MAX_INPUT_LENGTH];                                /* name of room flag to see */
    int iFlag;                                      /* int value of room flag */
    bool found;                                     /* room exists? */
    long vnum;                                      /* vnum of room */
    int col = 0;                                    /* display columns */

    argument = one_argument(argument, rFlag);

    if ((iFlag = flag_value(room_flags, rFlag)) == NO_FLAG) {
        send_to_char("Syntax:  showrooms [flag]\n\r", ch);
        send_to_char("        Type '`^? room``' to see a list of available room flags.\n\r", ch);
        return false;
    }

    area = ch->in_room->area;
    buf = new_buf();
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((room = get_room_index(vnum))) {
            found = true;

            unclr = uncolor_str(capitalize(room->name));
            printf_buf(buf, "[");

            if (IS_SET(room->room_flags, iFlag))
                printf_buf(buf, "`@");
            else
                printf_buf(buf, "`!");

            printf_buf(buf, "%5d``] %-17.16s", vnum, unclr);
            free_string(unclr);

            if (++col % 3 == 0)
                add_buf(buf, "\n\r");
        }
    }

    if (!found) {
        send_to_char("Room(s) not found in this area.\n\r", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf, "\n\r");

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
    return false;
}


void redit_extradesc_add(struct char_data *ch, const char *argument)
{
    char keyword[MAX_INPUT_LENGTH];
    struct extra_descr_data *ed;
    struct roomtemplate *template;

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed add [keyword]\n\r", ch);
        return;
    }

    EDIT_ROOM(ch, template);
    ed = roomtemplate_addextra(template, keyword, "");
    olc_start_string_editor(ch, ed, olc_extradescrdata_getdescription, olc_extradescrdata_setdescription);
    return;
}

void redit_extradesc_edit(struct char_data *ch, const char *argument)
{
    struct extra_descr_data *ed;
    char keyword[MAX_INPUT_LENGTH];
    struct roomtemplate *template;
    EDIT_ROOM(ch, template);

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed edit [keyword]\n\r", ch);
        return;
    }

    ed = roomtemplate_findextra(template, keyword);
    if (ed == NULL) {
        send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
        return;
    }

    olc_start_string_editor(ch, ed, olc_extradescrdata_getdescription, olc_extradescrdata_setdescription);
    return;
}

void redit_extradesc_delete(struct char_data *ch, const char *argument)
{
    char keyword[MAX_INPUT_LENGTH];
    struct roomtemplate *template;
    EDIT_ROOM(ch, template);

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed delete [keyword]\n\r", ch);
        return;
    }

    if (!roomtemplate_deleteextra(template, keyword)) {
        send_to_char("REdit:  Extra description keyword not found.\n\r", ch);
        return;
    }

    send_to_char("Extra description deleted.\n\r", ch);
    return;
}

void redit_extradesc_help(struct char_data *ch, const char *argument)
{
    int cmdindex;
    send_to_char("`3-`#========`3- `@OLC Extra Description Editor `3-`#=========`3-``\n\r", ch);
    for (cmdindex = 0; extra_cmd_table[cmdindex].do_fn != NULL; cmdindex++) {
        printf_to_char(ch, "%s\n\r", extra_cmd_table[cmdindex].lexicon);
    }
    return;
}

bool change_exit(struct char_data *ch, const char *argument, int door)
{
    struct roomtemplate *room;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int value;

    EDIT_ROOM(ch, room);

    /* set the exit flags - needs full argument */
    if ((value = flag_value(exit_flags, argument)) != NO_FLAG) {
        struct roomtemplate *pToRoom;
        int rev;

        if (!room->exit[door]) {
            send_to_char("Exit does not exist.\n\r", ch);
            return false;
        }

        /* this room */
        TOGGLE_BIT(room->exit[door]->rs_flags, value);
        room->exit[door]->exit_info = room->exit[door]->rs_flags;

        /* connected room */
        pToRoom = room->exit[door]->to_room;
        rev = rev_dir[door];

        if (pToRoom->exit[rev] != NULL) {
            pToRoom->exit[rev]->rs_flags = room->exit[door]->rs_flags;
            pToRoom->exit[rev]->exit_info = room->exit[door]->exit_info;
        }

        send_to_char("Exit flag toggled.\n\r", ch);
        return true;
    }

    /* parse the arguments */
    argument = one_argument(argument, command);
    one_argument(argument, arg);

    if (command[0] == '\0' && argument[0] == '\0') {
        move_char(ch, door, true);
        return false;
    }

    if (command[0] == '?') {
        show_help(ch->desc, "OLC_EXIT", NULL);
        return false;
    }

    if (!str_cmp(command, "delete")) {
        struct roomtemplate *pToRoom;
        int rev;

        if (!room->exit[door]) {
            send_to_char("REdit:  Cannot delete a null exit.\n\r", ch);
            return false;
        }

        /* remove ToRoom exit */
        rev = rev_dir[door];
        pToRoom = room->exit[door]->to_room;

        if (pToRoom->exit[rev]) {
            free_exit(pToRoom->exit[rev]);
            pToRoom->exit[rev] = NULL;
        }

        /* remove this exit */
        free_exit(room->exit[door]);
        room->exit[door] = NULL;

        send_to_char("Exit unlinked.\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "link")) {
        struct exit_data *pExit;
        struct roomtemplate *toRoom;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  [direction] link [vnum]\n\r", ch);
            return false;
        }

        value = parse_int(arg);

        if (!(toRoom = get_room_index(value))) {
            send_to_char("REdit:  Cannot link to non-existant room.\n\r", ch);
            return false;
        }

        if (!IS_BUILDER(ch, toRoom->area)) {
            send_to_char("REdit:  Cannot link to that area.\n\r", ch);
            return false;
        }

        if (toRoom->exit[rev_dir[door]]) {
            send_to_char("REdit:  Remote side's exit already exists.\n\r", ch);
            return false;
        }

        if (!room->exit[door])
            room->exit[door] = new_exit();

        room->exit[door]->to_room = toRoom;
        room->exit[door]->orig_door = door;

        door = rev_dir[door];
        pExit = new_exit();
        pExit->to_room = room;
        pExit->orig_door = door;
        toRoom->exit[door] = pExit;

        send_to_char("Two-way link established.\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "dig")) {
        char buf[MAX_STRING_LENGTH];

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax: [direction] dig <vnum>\n\r", ch);
            return false;
        }

        redit_create(ch, arg);
        sprintf(buf, "link %s", arg);
        change_exit(ch, buf, door);
        return true;
    }

    if (!str_cmp(command, "room")) {
        struct roomtemplate *toRoom;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  [direction] room [vnum]\n\r", ch);
            return false;
        }
        value = parse_int(arg);

        if (!(toRoom = get_room_index(value))) {
            send_to_char("REdit:  Cannot link to non-existant room.\n\r", ch);
            return false;
        }

        if (!room->exit[door])
            room->exit[door] = new_exit();

        room->exit[door]->to_room = toRoom;
        room->exit[door]->orig_door = door;

        send_to_char("One-way link established.\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "key")) {
        struct objecttemplate *key;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  [direction] key [vnum]\n\r", ch);
            return false;
        }

        if (!room->exit[door]) {
            send_to_char("Exit does not exist.\n\r", ch);
            return false;
        }

        value = parse_int(arg);

        if (!(key = objecttemplate_getbyvnum(value))) {
            send_to_char("REdit:  Key doesn't exist.\n\r", ch);
            return false;
        }

        if (key->item_type != ITEM_KEY) {
            send_to_char("REdit:  Object is not a key.\n\r", ch);
            return false;
        }

        room->exit[door]->key = value;

        send_to_char("Exit key set.\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "name")) {
        if (arg[0] == '\0') {
            send_to_char("Syntax:  [direction] name [string]\n\r", ch);
            send_to_char("         [direction] name none\n\r", ch);
            return false;
        }

        if (!room->exit[door]) {
            send_to_char("Exit does not exist.\n\r", ch);
            return false;
        }

        free_string(room->exit[door]->keyword);

        if (str_cmp(arg, "none"))
            room->exit[door]->keyword = str_dup(arg);
        else
            room->exit[door]->keyword = str_dup("");

        send_to_char("Exit name set.\n\r", ch);
        return true;
    }

    if (!str_prefix(command, "description")) {
        if (arg[0] == '\0') {
            if (!room->exit[door]) {
                send_to_char("Exit does not exist.\n\r", ch);
                return false;
            }

            olc_start_string_editor(ch, &room->exit[door], olc_exittemplate_getdescription, olc_exittemplate_setdescription);
            return true;
        }

        send_to_char("Syntax:  [direction] desc\n\r", ch);
        return false;
    }

    return false;
}
