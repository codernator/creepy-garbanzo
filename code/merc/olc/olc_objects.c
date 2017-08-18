#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"
#include "interp.h"
#include "help.h"



extern char *format_string(char *oldstring /*, bool fSpace */);
extern void string_append(struct char_data * ch, char **string);

static bool set_value(struct char_data *ch, struct objectprototype *prototype, const char *argument, int value);
static void show_obj_values(struct char_data *ch, struct objectprototype *obj);
static bool set_obj_values(struct char_data *ch, struct objectprototype *prototype, int value_num, const char *argument);

static void oedit_create(struct char_data *ch, const char *argument);
static void oedit_clone(struct char_data *ch, const char *argument);


static void oedit_extradesc_add(struct char_data *ch, const char *argument);
static void oedit_extradesc_edit(struct char_data *ch, const char *argument);
static void oedit_extradesc_delete(struct char_data *ch, const char *argument);
static void oedit_extradesc_format(struct char_data *ch, const char *argument);
static void oedit_extradesc_help(struct char_data *ch, const char *argument);
static const struct cmd_type extra_cmd_table[] =
    {
        { "add", oedit_extradesc_add, POS_DEAD, 0, 0, 0 },
        { "edit", oedit_extradesc_edit, POS_DEAD, 0, 0, 0 },
        { "remove", oedit_extradesc_delete, POS_DEAD, 0, 0, 0 },
        { "format", oedit_extradesc_format, POS_DEAD, 0, 0, 0 },
        { "?", oedit_extradesc_help, POS_DEAD, 0, 0, 0 },
        { "", NULL, POS_DEAD, 0, 0, 0 }
    };


void do_oedit(struct char_data *ch, const char *argument)
{
    struct objectprototype *prototype;
    char arg[MAX_STRING_LENGTH];
    int value;

    DENY_NPC(ch);

    argument = one_argument(argument, arg);
    if (is_number(arg)) {
        value = parse_int(arg);
        if (!(prototype = objectprototype_getbyvnum(value))) {
            send_to_char("OEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, prototype->area)) {
            send_to_char("Insufficient security to edit objects.\n\r", ch);
            return;
        }

        ch->desc->ed_data = (void *)prototype;
        ch->desc->editor = ED_OBJECT;
        return;
    } else {
        if (!str_cmp(arg, "create")) {
            oedit_create(ch, argument);
            return;
        }

        if (!str_cmp(arg, "clone")) {
            oedit_clone(ch, argument);
            return;
        }
    }

    send_to_char("OEdit:  There is no default object to edit.\n\r", ch);
    return;
}

EDIT(oedit_show)
{
    struct objectprototype *prototype;
    struct affect_data *paf;
    int cnt;

    EDIT_OBJ(ch, prototype);

    printf_to_char(ch, "`&Name``:         [%s]\n\r`&Area``:        [%5d] %s\n\r", prototype->name, (!prototype->area) ? -1 : prototype->area->vnum, (!prototype->area) ? "No Area" : prototype->area->name);

    printf_to_char(ch, "`&Vnum``:         [%5d]\n\r`&Type``:        [%s]\n\r", prototype->vnum, flag_string(type_flags, prototype->item_type));

    printf_to_char(ch, "`&Wear flags``:   [%s]\n\r", flag_string(wear_flags, prototype->wear_flags));
    printf_to_char(ch, "`&Extra flags``:  [%s]\n\r", flag_string(extra_flags, prototype->extra_flags));
    printf_to_char(ch, "`&Extra2 flags``: [%s]\n\r", flag_string(extra2_flags, prototype->extra2_flags));
    printf_to_char(ch, "`&Condition``:    [%5d]\n\r", prototype->condition);
    printf_to_char(ch, "`&Timer``:        [%5d]\n\r", prototype->init_timer);
    printf_to_char(ch, "`&Weight``:       [%5d]\n\r`&Cost``:        [%5d]\n\r", prototype->weight, prototype->cost);

    if (prototype->extra_descr) {
        struct extra_descr_data *ed;

        send_to_char("`&Ex desc kwd``: [", ch);
        for (ed = prototype->extra_descr->next; ed; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char(" ", ch);
        }
        send_to_char("]\n\r", ch);
    }

    printf_to_char(ch, "`&Short desc``:  %s\n\r`&Long desc``:\n\r     %s\n\r", prototype->short_descr, prototype->description);

    for (cnt = 0, paf = prototype->affected->next; paf; paf = paf->next) {
        if (cnt == 0) {
            send_to_char("`&Number Modifier Type    Affects``\n\r", ch);
            send_to_char("`1------ -------- ------- -------``\n\r", ch);
        }
        switch (paf->where) {
          case TO_AFFECTS:
              printf_to_char(ch, "[%4d] %-8d affect  %s\n\r", cnt, paf->modifier,
                             flag_string(affect_flags, paf->bitvector));
              break;
          case TO_IMMUNE:
              printf_to_char(ch, "[%4d] %-8d immune  %s\n\r", cnt, paf->modifier,
                             flag_string(imm_flags, paf->bitvector));
              break;
          case TO_RESIST:
              printf_to_char(ch, "[%4d] %-8d resist  %s\n\r", cnt, paf->modifier,
                             flag_string(res_flags, paf->bitvector));
              break;
          case TO_VULN:
              printf_to_char(ch, "[%4d] %-8d vuln   %s\n\r", cnt, paf->modifier,
                             flag_string(vuln_flags, paf->bitvector));
              break;
          default:
              printf_to_char(ch, "[%4d] %-8d apply   %s\n\r", cnt, paf->modifier,
                             flag_string(apply_flags, paf->location));
        }
        cnt++;
    }

    send_to_char("\n\r", ch);
    show_obj_values(ch, prototype);
    return false;
}

EDIT(oedit_addaffect)
{
    struct objectprototype *prototype;
    struct affect_data *affect;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    int value;

    EDIT_OBJ(ch, prototype);
    argument = one_argument(argument, loc);
    one_argument(argument, mod);
    if (loc[0] == '\0' || mod[0] == '\0' || !is_number(mod)) {
        send_to_char("Syntax:  addaffect [location] [#xmod]\n\r", ch);
        return false;
    }

    if ((value = flag_value(apply_flags, loc)) == NO_FLAG) { /* Hugin */
        send_to_char("Valid affects are:\n\r", ch);
        show_olc_help(ch, "apply");
        return false;
    }

    affect = affectdata_new();
    affect->location = value;
    affect->modifier = parse_int(mod);
    affect->where = TO_OBJECT;
    affect->type = -1;
    affect->duration = -1;
    affect->bitvector = 0;

    objectprototype_applyaffect(prototype, affect);

    send_to_char("Affect added.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_addapply
 *
 *	adds an affect to the object
 *	valid applys are:
 *		affects		- spell affects
 *		object		- same as addaffect
 *		immune		- immunities
 *		resist		- resistances
 *		vuln		- vulnerabilities
 *****************************************************************************/
EDIT(oedit_addapply)
{
    struct objectprototype *prototype;
    struct affect_data *affect;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    char type[MAX_STRING_LENGTH];
    char bvector[MAX_STRING_LENGTH];
    int value;
    int bitvector;
    int typ;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '?' || argument[0] == '\0') {
        send_to_char("Syntax:  addapply [type] [where] [modifier] [bitvector]\n\r", ch);
        return false;
    }

    argument = one_argument(argument, type);
    argument = one_argument(argument, loc);
    argument = one_argument(argument, mod);
    one_argument(argument, bvector);

    if (type[0] == '\0'
        || (typ = flag_value(apply_types, type)) == NO_FLAG) {
        send_to_char("Invalid apply type.\n\rValid apply types are:\n\r", ch);
        show_olc_help(ch, "apptype");
        return false;
    }

    if (loc[0] == '\0'
        || (value = flag_value(apply_flags, loc)) == NO_FLAG) {
        send_to_char("Valid applys are:\n\r", ch);
        show_olc_help(ch, "apply");
        return false;
    }

    if (bvector[0] == '\0'
        || (bitvector = flag_value(bitvector_type[typ].table, bvector)) == NO_FLAG) {
        send_to_char("Invalid bitvector type.\n\r", ch);
        send_to_char("Valid bitvector types are:\n\r", ch);
        show_olc_help(ch, bitvector_type[typ].help);
        return false;
    }

    if (mod[0] == '\0' || !is_number(mod)) {
        send_to_char("Syntax:  addapply [type] [location] [#xmod] [bitvector]\n\r", ch);
        return false;
    }

    affect = affectdata_new();
    affect->location = value;
    affect->modifier = parse_int(mod);
    affect->where = (int)apply_types[typ].bit;
    affect->type = -1;
    affect->duration = -1;
    affect->bitvector = bitvector;

    objectprototype_applyaffect(prototype, affect);

    send_to_char("Apply added.\n\r", ch);
    return true;
}

EDIT(oedit_delaffect)
{
    struct objectprototype *prototype;
    char arg1[MAX_STRING_LENGTH];
    int index;

    EDIT_OBJ(ch, prototype);
    one_argument(argument, arg1);
    if (!is_number(arg1) || arg1[0] == '\0') {
        send_to_char("Syntax:  delaffect [#xaffect]\n\r", ch);
        return false;
    }

    index = parse_int(arg1);
    if (index < 0) {
        send_to_char("Only non-negative affect-numbers allowed.\n\r", ch);
        return false;
    }

    if (!objectprototype_deleteaffect(prototype, index)) {
        send_to_char("No such affect.\n\r", ch);
        return false;
    }

    send_to_char("Affect removed.\n\r", ch);
    return true;
}

EDIT(oedit_name)
{
    struct objectprototype *prototype;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  name [string]\n\r", ch);
        return false;
    }

    objectprototype_setname(prototype, argument);
    send_to_char("Name set.\n\r", ch);
    return true;
}

EDIT(oedit_short)
{
    struct objectprototype *prototype;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  short [string]\n\r", ch);
        return false;
    }

    objectprototype_setshort(prototype, argument);
    send_to_char("Short description set.\n\r", ch);
    return true;
}

EDIT(oedit_long)
{
    struct objectprototype *prototype;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  long [string]\n\r", ch);
        return false;
    }

    objectprototype_setlong(prototype, argument);
    send_to_char("Long description set.\n\r", ch);
    return true;
}




/*****************************************************************************
 *	oedit_value0
 *
 *	edit v0
 *****************************************************************************/
EDIT(oedit_value0){
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);
    return set_value(ch, prototype, argument, 0);
}

/*****************************************************************************
 *	oedit_value1
 *
 *	edit v1
 *****************************************************************************/
EDIT(oedit_value1){
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);
    return set_value(ch, prototype, argument, 1);
}

/*****************************************************************************
 *	oedit_value2
 *
 *	edit v2
 *****************************************************************************/
EDIT(oedit_value2){
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);
    return set_value(ch, prototype, argument, 2);
}

/*****************************************************************************
 *	oedit_value3
 *
 *	edit v3
 *****************************************************************************/
EDIT(oedit_value3){
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);
    return set_value(ch, prototype, argument, 3);
}

/*****************************************************************************
 *	oedit_value4
 *
 *	edit v4
 *****************************************************************************/
EDIT(oedit_value4){
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);
    return set_value(ch, prototype, argument, 4);
}



/*****************************************************************************
 *	oedit_weight
 *
 *	edit the weight property of the object
 *****************************************************************************/
EDIT(oedit_weight){
    struct objectprototype *prototype;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  weight [number]\n\r", ch);
        return false;
    }

    prototype->weight = parse_int(argument);
    send_to_char("Weight set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_cost
 *
 *	edit the cost property of the object
 *****************************************************************************/
EDIT(oedit_cost){
    struct objectprototype *prototype;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  cost [number]\n\r", ch);
        return false;
    }

    prototype->cost = parse_unsigned_int(argument);
    send_to_char("Cost set.\n\r", ch);
    return true;
}




/*****************************************************************************
 *	oedit_ed
 *
 *	edit the extra description of an object
 *****************************************************************************/
EDIT(oedit_ed){
    char command[MAX_INPUT_LENGTH];
    int cmdindex = 0;
    int trust = 0;

    argument = one_argument(argument, command);

    if (command[0] == '\0') {
        oedit_extradesc_help(ch, argument);
        return false;
    }

    trust = get_trust(ch);
    for (cmdindex = 0; extra_cmd_table[cmdindex].name[0] != '\0'; cmdindex++) {
        if (command[0] == extra_cmd_table[cmdindex].name[0]
                && !str_cmp(command, extra_cmd_table[cmdindex].name)
                && extra_cmd_table[cmdindex].level <= trust) {
            (*cmd_table[cmdindex].do_fun)(ch, argument);
            return true;
        }
    }

    send_to_char("``COMMAND NOT FOUND``\n\r", ch);
    return false;
}


/*****************************************************************************
 *	oedit_extra
 *
 *	edit the extra flags for an object
 *****************************************************************************/
EDIT(oedit_extra){
    struct objectprototype *prototype;
    int value;

    EDIT_OBJ(ch, prototype);
    if (argument[0] != '\0') {
        if ((value = flag_value(extra_flags, argument)) != NO_FLAG) {
            TOGGLE_BIT(prototype->extra_flags, value);
            send_to_char("Extra flag toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax:  extra [flag]\n\r"
                 "Type '? extra' for a list of flags.\n\r", ch);
    return false;
}

/*****************************************************************************
 *       oedit_extra2
 *
 *       edit the extra2 flags for an object
 *****************************************************************************/
EDIT(oedit_extra2){
    struct objectprototype *prototype;
    int value;

    EDIT_OBJ(ch, prototype);
    if (argument[0] != '\0') {
        if ((value = flag_value(extra2_flags, argument)) != NO_FLAG) {
            TOGGLE_BIT(prototype->extra2_flags, value);
            send_to_char("Extra flag toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax:  extra2 [flag]\n\r"
                 "Type '? extra2' for a list of flags.\n\r", ch);
    return false;
}


/*****************************************************************************
 *	oedit_wear
 *
 *	edit the wear locations of an object
 *****************************************************************************/
EDIT(oedit_wear){
    struct objectprototype *prototype;
    int value;

    EDIT_OBJ(ch, prototype);
    if (argument[0] != '\0') {
        if ((value = flag_value(wear_flags, argument)) != NO_FLAG) {
            TOGGLE_BIT(prototype->wear_flags, value);
            send_to_char("Wear flag toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax:  wear [flag]\n\r"
                 "Type '? wear' for a list of flags.\n\r", ch);
    return false;
}


/*****************************************************************************
 *	oedit_type
 *
 *	set the object type
 *****************************************************************************/
EDIT(oedit_type){
    struct objectprototype *prototype;
    int value;

    EDIT_OBJ(ch, prototype);
    if (argument[0] != '\0') {
        if ((value = flag_value(type_flags, argument)) != NO_FLAG) {
            prototype->item_type = value;
            send_to_char("Type set.\n\r", ch);

            /* clear the values */
            prototype->value[0] = 0;
            prototype->value[1] = 0;
            prototype->value[2] = 0;
            prototype->value[3] = 0;
            prototype->value[4] = 0;
            return true;
        }
    }

    send_to_char("Syntax:  type [flag]\n\r"
                 "Type '? type' for a list of flags.\n\r", ch);
    return false;
}


/** edit the timer property of an object (auto destroy)	Added by Monrick, 5/2008 */
EDIT(oedit_timer){
    struct objectprototype *prototype;

    EDIT_OBJ(ch, prototype);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  timer [# of ticks]  (0 for infinite)\n\r", ch);
        return false;
    }

    prototype->init_timer = parse_int(argument);
    send_to_char("Timer set.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	oedit_condition
 *
 *	edit the condition of the object
 *****************************************************************************/
EDIT(oedit_condition){
    struct objectprototype *prototype;
    int value;

    EDIT_OBJ(ch, prototype);
    if (argument[0] != '\0'
        && (value = parse_int(argument)) >= 0
        && (value <= 100)) {
        prototype->condition = value;
        send_to_char("Condition set.\n\r", ch);
        return true;
    }

    send_to_char("Syntax:  condition [number]\n\r"
                 "Where number can range from 0(ruined) to 100(perfect).\n\r", ch);
    return false;
}




/*****************************************************************************
 *	set_value
 *
 *	set one of the 4 value properties on the object
 *****************************************************************************/
bool set_value(struct char_data *ch, struct objectprototype *prototype, const char *argument, int value)
{
    if (argument[0] == '\0') {
        set_obj_values(ch, prototype, -1, "");
        return false;
    }

    if (set_obj_values(ch, prototype, value, argument))
        return true;

    return false;
}

static void show_obj_values(struct char_data *ch, struct objectprototype *obj)
{
    struct dynamic_skill *skill;
    int idx;

    switch (obj->item_type) {
      default:
          break;

      case ITEM_LIGHT:
          if (obj->value[2] == -1 || obj->value[2] == 999)
              printf_to_char(ch, "`1[`!v2`1]`& Light``:  Infinite[-1]\n\r");
          else
              printf_to_char(ch, "`1[`!v2`1]`& Light``:  [%d]\n\r", obj->value[2]);
          break;

      case ITEM_WAND:
      case ITEM_STAFF:
          skill = resolve_skill_sn((int)obj->value[3]);

          printf_to_char(ch, "`1[`!v0`1]`& Level``:          [%d]\n\r"
                         "`1[`!v1`1]`& Charges Total``:  [%d]\n\r"
                         "`1[`!v2`1]`& Charges Left``:   [%d]\n\r"
                         "`1[`!v3`1]`& Spell``:          %s\n\r",
                         obj->value[0],
                         obj->value[1],
                         obj->value[2],
                         (skill != NULL) ? skill->name : "none");
          break;

      case ITEM_PORTAL:
          printf_to_char(ch, "`1[`!v0`1]`& Charges``:        [%d]\n\r"
                         "`1[`!v1`1]`& Exit Flags``:     %s\n\r"
                         "`1[`!v2`1]`& Portal Flags``:   %s\n\r"
                         "`1[`!v3`1]`& Goes to(vnum)``:  [%d]\n\r",
                         obj->value[0],
                         flag_string(exit_flags, obj->value[1]),
                         flag_string(portal_flags, obj->value[2]),
                         obj->value[3]);
          break;
      case ITEM_DICE:
          printf_to_char(ch, "`1[`!v0`1]`& Number of Dice``:  [%d]\n\r"
                         "`1[`!v1`!]`& Type of Dice``:    [%d]\n\r",
                         obj->value[0],
                         obj->value[1]);
          break;
      case ITEM_FURNITURE:
          printf_to_char(ch, "`1[`!v0`1]`& Max people``:      [%d]\n\r"
                         "`1[`!v1`1]`& Max weight``:      [%d]\n\r"
                         "`1[`!v2`1]`& Furniture Flags``: %s\n\r"
                         "`1[`!v3`1]`& Heal bonus``:      [%d]\n\r"
                         "`1[`!v4`1]`& Mana bonus``:      [%d]\n\r",
                         obj->value[0],
                         obj->value[1],
                         flag_string(furniture_flags, obj->value[2]),
                         obj->value[3],
                         obj->value[4]);
          break;

      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
          printf_to_char(ch, "`1[`!v0`1]`& Level``:  [%d]\n\r", obj->value[0]);

          for (idx = 1; idx <= 4; idx++) {
              if ((skill = resolve_skill_sn((int)obj->value[idx])) != NULL)
                  printf_to_char(ch, "`1[`!v%d`1]`& Spell``:  %s\n\r", idx, skill->name);
              else
                  printf_to_char(ch, "`1[`!v%d`1]`& Spell``:  none\n\r", idx);
          }
          break;

      case ITEM_ARMOR:
          printf_to_char(ch, "`1[`!v0`1]`& Ac pierce``:       [%d]\n\r"
                         "`1[`!v1`1]`& Ac bash``:         [%d]\n\r"
                         "`1[`!v2`1]`& Ac slash``:        [%d]\n\r"
                         "`1[`!v3`1]`& Ac exotic``:       [%d]\n\r",
                         obj->value[0],
                         obj->value[1],
                         obj->value[2],
                         obj->value[3]);
          break;

      case ITEM_WEAPON:
          printf_to_char(ch, "`1[`!v0`1]`& Weapon class``:   %s\n\r",
                         flag_string(weapon_class, obj->value[0]));
          printf_to_char(ch, "`1[`!v1`1]`& Number of dice``: [%d]\n\r", obj->value[1]);
          printf_to_char(ch, "`1[`!v2`1]`& Type of dice``:   [%d]\n\r", obj->value[2]);
          printf_to_char(ch, "`1[`!v3`1]`& Type``:           %s\n\r",
                         attack_table[obj->value[3]].name);
          printf_to_char(ch, "`1[`!v4`1]`& Special type``:   %s\n\r",
                         flag_string(weapon_flag_type, obj->value[4]));
          break;

      case ITEM_CONTAINER:
          printf_to_char(ch, "`1[`!v0`1]`& Weight``:      [%d kg]\n\r"
                         "`1[`!v1`1]`& Flags``:       [%s]\n\r"
                         "`1[`!v2`1]`& Key``:         [%d] %s\n\r"
                         "`1[`!v3`1]`& Capacity``:    [%d]\n\r"
                         "`1[`!v4`1]`& Weight Mult``: [%d]\n\r",
                         obj->value[0],
                         flag_string(container_flags, obj->value[1]),
                         obj->value[2],
                         objectprototype_getbyvnum(obj->value[2]) ?
                         objectprototype_getbyvnum(obj->value[2])->short_descr : "none",
                         obj->value[3],
                         obj->value[4]);
          break;

      case ITEM_DRINK_CON:
          printf_to_char(ch, "`1[`!v0`1]`& Liquid Total``: [%d]\n\r"
                         "`1[`!v1`1]`& Liquid Left``:  [%d]\n\r"
                         "`1[`!v2`1]`& Liquid``:       %s\n\r"
                         "`1[`!v3`1]`& Poisoned``:     %s\n\r",
                         obj->value[0],
                         obj->value[1],
                         liq_table[obj->value[2]].liq_name,
                         obj->value[3] != 0 ? "Yes" : "No");
          break;

      case ITEM_FOUNTAIN:
          printf_to_char(ch, "`1[`!v0`1]`& Liquid Total``: [%d]\n\r"
                         "`1[`!v1`1]`& Liquid Left``:  [%d]\n\r"
                         "`1[`!v2`1]`& Liquid``:	    %s\n\r",
                         obj->value[0],
                         obj->value[1],
                         liq_table[obj->value[2]].liq_name);
          break;

      case ITEM_FOOD:
          printf_to_char(ch, "`1[`!v0`1]`& Food hours``: [%d]\n\r"
                         "`1[`!v1`1]`& Full hours``: [%d]\n\r"
                         "`1[`!v3`1]`& Poisoned``:   %s\n\r",
                         obj->value[0],
                         obj->value[1],
                         obj->value[3] != 0 ? "Yes" : "No");
          break;

      case ITEM_MONEY:
          printf_to_char(ch, "`1[`!v0`1]`& Gold``:   [%d]\n\r", obj->value[1]);
          printf_to_char(ch, "`1[`!v1`1]`& Silver``: [%d]\n\r", obj->value[0]);
          break;
    }
    return;
}

/*****************************************************************************
 *	set_obj_values
 *
 *	set the value properties of an item based on it's type
 *****************************************************************************/
bool set_obj_values(struct char_data *ch, struct objectprototype *prototype, int value_num, const char *argument)
{
    struct dynamic_skill *skill;
    int value;

    switch (prototype->item_type) {
      default:
          break;

      case ITEM_LIGHT:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_LIGHT", NULL);
                return false;
            case 2:
                send_to_char("Hours of Light set.\n\r\n\r", ch);
                prototype->value[2] = parse_int(argument);
                break;
          }
          break;

      case ITEM_WAND:
      case ITEM_STAFF:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_STAFF_WAND", NULL);
                return false;
            case 0:
                send_to_char("Spell level set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Total number of charges set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Current number of charges set.\n\r\n\r", ch);
                prototype->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("Spell type set.\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    prototype->value[3] = skill->sn;
                break;
          }
          break;

      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_SCROLL_POTION_PILL", NULL);
                return false;
            case 0:
                send_to_char("Spell level set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Spell type 1 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    prototype->value[1] = skill->sn;
                break;
            case 2:
                send_to_char("Spell type 2 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    prototype->value[2] = skill->sn;
                break;
            case 3:
                send_to_char("Spell type 3 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    prototype->value[3] = skill->sn;
                break;
            case 4:
                send_to_char("Spell type 4 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    prototype->value[4] = skill->sn;
                break;
          }
          break;

      case ITEM_ARMOR:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_ARMOR", NULL);
                return false;
            case 0:
                send_to_char("AC pierce set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("AC bash set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("AC slash set.\n\r\n\r", ch);
                prototype->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("AC exotic set.\n\r\n\r", ch);
                prototype->value[3] = parse_int(argument);
                break;
          }
          break;

      case ITEM_WEAPON:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_WEAPON", NULL);
                return false;
            case 0:
                send_to_char("Weapon class set.\n\r\n\r", ch);
                ALT_FLAGVALUE_SET(prototype->value[0], weapon_class, argument);
                break;
            case 1:
                send_to_char("Number of dice set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Type of dice set.\n\r\n\r", ch);
                prototype->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("Weapon type set.\n\r\n\r", ch);
                prototype->value[3] = attack_lookup(argument);
                break;
            case 4:
                send_to_char("Special weapon type toggled.\n\r\n\r", ch);
                ALT_FLAGVALUE_TOGGLE(prototype->value[4], weapon_flag_type, argument);
                break;
          }
          break;

      case ITEM_PORTAL:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_PORTAL", NULL);
                return false;
            case 0:
                send_to_char("Charges set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Exit flags set.\n\r\n\r", ch);
                ALT_FLAGVALUE_SET(prototype->value[1], exit_flags, argument);
                break;
            case 2:
                send_to_char("Portal flags set.\n\r\n\r", ch);
                ALT_FLAGVALUE_SET(prototype->value[2], portal_flags, argument);
                break;
            case 3:
                send_to_char("Exit vnum set.\n\r\n\r", ch);
                prototype->value[3] = parse_int(argument);
                break;
          }
          break;

      case ITEM_FURNITURE:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_FURNITURE", NULL);
                return false;
            case 0:
                send_to_char("Number of people set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Max weight set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Furniture flags toggled.\n\r\n\r", ch);
                ALT_FLAGVALUE_TOGGLE(prototype->value[2], furniture_flags, argument);
                break;
            case 3:
                send_to_char("Heal bonus set.\n\r\n\r", ch);
                prototype->value[3] = parse_int(argument);
                break;
            case 4:
                send_to_char("Mana bonus set.\n\r\n\r", ch);
                prototype->value[4] = parse_int(argument);
                break;
          }
          break;

      case ITEM_DICE:
          switch (value_num) {
            case 0:
                prototype->value[0] = parse_int(argument);
                send_to_char("Number of dice set.\n\r\n\r", ch);
                break;
            case 1:
                prototype->value[1] = parse_int(argument);
                send_to_char("Number of sides set.\n\r\n\r", ch);
                break;
            default:
                show_help(ch->desc, "ITEM_DICE", NULL);
                break;
          }
          break;
      case ITEM_CONTAINER:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_CONTAINER", NULL);
                return false;
            case 0:
                send_to_char("Weight capacity set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                if ((value = flag_value(container_flags, argument)) != NO_FLAG) {
                    TOGGLE_BIT(prototype->value[1], value);
                } else {
                    show_help(ch->desc, "ITEM_CONTAINER", NULL);
                    return false;
                }
                send_to_char("Container type set.\n\r\n\r", ch);
                break;
            case 2:
                if (parse_int(argument) != 0) {
                    if (!objectprototype_getbyvnum(parse_int(argument))) {
                        send_to_char("There is no such item.\n\r\n\r", ch);
                        return false;
                    }

                    if (objectprototype_getbyvnum(parse_int(argument))->item_type != ITEM_KEY) {
                        send_to_char("That item is not a key.\n\r\n\r", ch);
                        return false;
                    }
                }
                send_to_char("Container key set.\n\r\n\r", ch);
                prototype->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("Container max weight set.\n\r", ch);
                prototype->value[3] = parse_int(argument);
                break;
            case 4:
                send_to_char("Weight multiplier set.\n\r\n\r", ch);
                prototype->value[4] = parse_int(argument);
                break;
          }
          break;

      case ITEM_DRINK_CON:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_DRINK", NULL);
                return false;
            case 0:
                send_to_char("Maximum amount of liquid hours set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Current amount of liquid hours set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Liquid type set.\n\r\n\r", ch);
                prototype->value[2] = (liq_lookup(argument) != -1 ? liq_lookup(argument) : 0);
                break;
            case 3:
                send_to_char("Poison value toggled.\n\r\n\r", ch);
                prototype->value[3] = (prototype->value[3] == 0) ? 1 : 0;
                break;
          }
          break;

      case ITEM_FOUNTAIN:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_FOUNTAIN", NULL);
                return false;
            case 0:
                send_to_char("Maximum amount of liquid hours set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Current amount of liquid hours set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Liquid type set.\n\r\n\r", ch);
                prototype->value[2] = (liq_lookup(argument) != -1 ? liq_lookup(argument) : 0);
                break;
          }
          break;

      case ITEM_FOOD:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_FOOD", NULL);
                return false;
            case 0:
                send_to_char("Hours of food set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Hours of full set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
            case 3:
                send_to_char("Poison value toggled.\n\r\n\r", ch);
                prototype->value[3] = (prototype->value[3] == 0) ? 1 : 0;
                break;
          }
          break;

      case ITEM_MONEY:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_MONEY", NULL);
                return false;
            case 0:
                send_to_char("Gold amount set.\n\r\n\r", ch);
                prototype->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Silver amount set.\n\r\n\r", ch);
                prototype->value[1] = parse_int(argument);
                break;
          }
          break;
    }
    show_obj_values(ch, prototype);
    return true;
}


void oedit_create(struct char_data *ch, const char *argument)
{
    struct objectprototype *prototype;
    struct area_data *area;
    long vnum;

    if (!is_number(argument)) {
        send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
        return;
    }

    vnum = parse_long(argument);
    if (argument[0] == '\0' || vnum == 0) {
        send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
        return;
    }

    area = area_getbycontainingvnum(vnum);
    if (!area) {
        send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
        return;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return;
    }

    if (objectprototype_getbyvnum(vnum)) {
        send_to_char("OEdit:  Object vnum already exists.\n\r", ch);
        return;
    }

    prototype = objectprototype_new(vnum);
    prototype->area = area;

    if (vnum > top_vnum_obj)
        top_vnum_obj = vnum;

    SET_BIT(area->area_flags, AREA_CHANGED);
    ch->desc->editor = ED_OBJECT;
    ch->desc->ed_data = (void *)prototype;

    send_to_char("Object Created.\n\r", ch);
    return;
}

void oedit_clone(struct char_data *ch, const char *argument)
{
    struct objectprototype *source;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    struct area_data *targetarea;
    unsigned long targetvnum;
    unsigned long sourcevnum;

    argument = one_argument(argument, arg1);
    (void)one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        send_to_char("OEdit syntax:  clone [target vnum] <source vnum>\n\r", ch);
        return;
    }
    targetvnum = parse_int(arg1);
    if (objectprototype_getbyvnum(targetvnum) != NULL) {
        printf_to_char(ch, "OEdit:  Target vnum %lu already exists.\n\r", targetvnum);
        return;
    }
    targetarea = area_getbycontainingvnum(targetvnum);
    if (!targetarea) {
        printf_to_char(ch, "OEdit: Target vnum %lu is not assigned an area.\n\r", targetvnum);
        return;
    }

    if (!IS_BUILDER(ch, targetarea)) {
        printf_to_char(ch, "OEdit: Target vnum %lu is in an area you cannot build in.\n\r", targetvnum);
        return;
    }

    if (arg2[0] != '\0')
    {
        if (!is_number(arg2))
        {
            send_to_char("OEdit:  The source vnum must be a number.\n\r", ch);
            return;
        }
        sourcevnum = parse_int(arg2);
        source = objectprototype_getbyvnum(sourcevnum);
        if (source == NULL)
        {
            printf_to_char(ch, "OEdit:  Source vnum %lu does not yet exist.\n\r", sourcevnum);
            return;
        }
    }
    else
    {
        EDIT_OBJ(ch, source);
        sourcevnum = source->vnum;
    }

    ch->desc->ed_data = objectprototype_clone(source, targetvnum, targetarea);
    if (targetvnum > top_vnum_obj)
        top_vnum_obj = targetvnum;

    SET_BIT(targetarea->area_flags, AREA_CHANGED);
    ch->desc->editor = ED_OBJECT;
    printf_to_char(ch, "Object %lu cloned. You are now editing object %lu.\n\r", sourcevnum, targetvnum);
    return;
}



void oedit_extradesc_add(struct char_data *ch, const char *argument)
{
    char keyword[MAX_INPUT_LENGTH];
    struct extra_descr_data *ed;
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed add [keyword]\n\r", ch);
        return;
    }

    ed = objectprototype_addextra(prototype, keyword, "");

    // set description editor.
    string_append(ch, &ed->description);
    return;
}

void oedit_extradesc_edit(struct char_data *ch, const char *argument)
{
    struct extra_descr_data *ed;
    char keyword[MAX_INPUT_LENGTH];
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed edit [keyword]\n\r", ch);
        return;
    }

    ed = objectprototype_findextra(prototype, keyword);
    if (ed == NULL) {
        send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
        return;
    }

    string_append(ch, &ed->description);
    return;
}

void oedit_extradesc_delete(struct char_data *ch, const char *argument)
{
    char keyword[MAX_INPUT_LENGTH];
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed delete [keyword]\n\r", ch);
        return;
    }

    if (!objectprototype_deleteextra(prototype, keyword)) {
        send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
        return;
    }

    send_to_char("Extra description deleted.\n\r", ch);
    return;
}

void oedit_extradesc_format(struct char_data *ch, const char *argument)
{
    struct extra_descr_data *ed;
    char keyword[MAX_INPUT_LENGTH];
    struct objectprototype *prototype;
    EDIT_OBJ(ch, prototype);

    one_argument(argument, keyword);
    if (keyword[0] == '\0') {
        send_to_char("Syntax:  ed format [keyword]\n\r", ch);
        return;
    }

    ed = objectprototype_findextra(prototype, keyword);

    if (ed == NULL) {
        send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
        return;
    }

    ed->description = format_string(ed->description);
    send_to_char("Extra description formatted.\n\r", ch);
    return;
}

void oedit_extradesc_help(struct char_data *ch, const char *argument) {
    send_to_char("Syntax:  ed add [keyword]\n\r", ch);
    send_to_char("         ed delete [keyword]\n\r", ch);
    send_to_char("         ed edit [keyword]\n\r", ch);
    send_to_char("         ed format [keyword]\n\r", ch);
    return;
}

