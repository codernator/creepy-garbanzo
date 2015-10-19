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


/*****************************************************************************
 *	display the value properties of an item based on it's type
 *****************************************************************************/
static void show_obj_values(struct char_data *ch, struct objectprototype *obj)
{
    SKILL *skill;
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

      case ITEM_SOCKETS:
          printf_to_char(ch, "[v0] Type:           %s\n\r",
                         flag_string(socket_flags, obj->value[0]));
          printf_to_char(ch, "[v1] Value:          %s\n\r",
                         flag_string(socket_values, obj->value[1]));
          break;
    }
    return;
}



/*****************************************************************************
 *	set_obj_values
 *
 *	set the value properties of an item based on it's type
 *****************************************************************************/
static bool set_obj_values(struct char_data *ch, struct objectprototype *pObj, int value_num, const char *argument)
{
    SKILL *skill;
    int value;

    switch (pObj->item_type) {
      default:
          break;

      case ITEM_LIGHT:
          switch (value_num) {
            default:
                show_help(ch->desc, "ITEM_LIGHT", NULL);
                return false;
            case 2:
                send_to_char("Hours of Light set.\n\r\n\r", ch);
                pObj->value[2] = parse_int(argument);
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Total number of charges set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Current number of charges set.\n\r\n\r", ch);
                pObj->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("Spell type set.\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    pObj->value[3] = skill->sn;
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Spell type 1 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    pObj->value[1] = skill->sn;
                break;
            case 2:
                send_to_char("Spell type 2 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    pObj->value[2] = skill->sn;
                break;
            case 3:
                send_to_char("Spell type 3 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    pObj->value[3] = skill->sn;
                break;
            case 4:
                send_to_char("Spell type 4 set.\n\r\n\r", ch);
                if ((skill = skill_lookup(argument)) != NULL)
                    pObj->value[4] = skill->sn;
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("AC bash set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("AC slash set.\n\r\n\r", ch);
                pObj->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("AC exotic set.\n\r\n\r", ch);
                pObj->value[3] = parse_int(argument);
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
                ALT_FLAGVALUE_SET(pObj->value[0], weapon_class, argument);
                break;
            case 1:
                send_to_char("Number of dice set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Type of dice set.\n\r\n\r", ch);
                pObj->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("Weapon type set.\n\r\n\r", ch);
                pObj->value[3] = attack_lookup(argument);
                break;
            case 4:
                send_to_char("Special weapon type toggled.\n\r\n\r", ch);
                ALT_FLAGVALUE_TOGGLE(pObj->value[4], weapon_flag_type, argument);
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Exit flags set.\n\r\n\r", ch);
                ALT_FLAGVALUE_SET(pObj->value[1], exit_flags, argument);
                break;
            case 2:
                send_to_char("Portal flags set.\n\r\n\r", ch);
                ALT_FLAGVALUE_SET(pObj->value[2], portal_flags, argument);
                break;
            case 3:
                send_to_char("Exit vnum set.\n\r\n\r", ch);
                pObj->value[3] = parse_int(argument);
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Max weight set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Furniture flags toggled.\n\r\n\r", ch);
                ALT_FLAGVALUE_TOGGLE(pObj->value[2], furniture_flags, argument);
                break;
            case 3:
                send_to_char("Heal bonus set.\n\r\n\r", ch);
                pObj->value[3] = parse_int(argument);
                break;
            case 4:
                send_to_char("Mana bonus set.\n\r\n\r", ch);
                pObj->value[4] = parse_int(argument);
                break;
          }
          break;

      case ITEM_DICE:
          switch (value_num) {
            case 0:
                pObj->value[0] = parse_int(argument);
                send_to_char("Number of dice set.\n\r\n\r", ch);
                break;
            case 1:
                pObj->value[1] = parse_int(argument);
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                if ((value = flag_value(container_flags, argument)) != NO_FLAG) {
                    TOGGLE_BIT(pObj->value[1], value);
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
                pObj->value[2] = parse_int(argument);
                break;
            case 3:
                send_to_char("Container max weight set.\n\r", ch);
                pObj->value[3] = parse_int(argument);
                break;
            case 4:
                send_to_char("Weight multiplier set.\n\r\n\r", ch);
                pObj->value[4] = parse_int(argument);
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Current amount of liquid hours set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Liquid type set.\n\r\n\r", ch);
                pObj->value[2] = (liq_lookup(argument) != -1 ? liq_lookup(argument) : 0);
                break;
            case 3:
                send_to_char("Poison value toggled.\n\r\n\r", ch);
                pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Current amount of liquid hours set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 2:
                send_to_char("Liquid type set.\n\r\n\r", ch);
                pObj->value[2] = (liq_lookup(argument) != -1 ? liq_lookup(argument) : 0);
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Hours of full set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
            case 3:
                send_to_char("Poison value toggled.\n\r\n\r", ch);
                pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
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
                pObj->value[0] = parse_int(argument);
                break;
            case 1:
                send_to_char("Silver amount set.\n\r\n\r", ch);
                pObj->value[1] = parse_int(argument);
                break;
          }
          break;

      case ITEM_SOCKETS:
          switch (value_num) {
            default:
                show_help(ch->desc, "SOCKET_LIST", NULL);
                return false;
            case 0:
                if ((value = flag_value(socket_flags, argument)) != NO_FLAG) {
                    pObj->value[0] = value;
                    send_to_char("Inlay type set.\n\r\n\r", ch);
                } else {
                    show_help(ch->desc, "SOCKET_LIST", NULL);
                    return false;
                }
                break;
            case 1:
                if ((value = flag_value(socket_values, argument)) != NO_FLAG) {
                    pObj->value[1] = value;
                    send_to_char("Gem value set.\n\r\n\r", ch);
                } else {
                    show_help(ch->desc, "SOCKET_LIST", NULL);
                    return false;
                }
                break;
          }
          break;
    }
    show_obj_values(ch, pObj);
    return true;
}


/*****************************************************************************
 *	set_value
 *
 *	set one of the 4 value properties on the object
 *****************************************************************************/
static bool set_value(struct char_data *ch, struct objectprototype *pObj, const char *argument, int value)
{
    if (argument[0] == '\0') {
        set_obj_values(ch, pObj, -1, "");
        return false;
    }

    if (set_obj_values(ch, pObj, value, argument))
        return true;

    return false;
}



/*****************************************************************************
 *	do_oedit
 *****************************************************************************/
void do_oedit(struct char_data *ch, const char *argument)
{
    struct objectprototype *pObj;
    struct area_data *pArea;
    char arg[MAX_STRING_LENGTH];
    int value;

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg);
    if (is_number(arg)) {
        value = parse_int(arg);
        if (!(pObj = objectprototype_getbyvnum(value))) {
            send_to_char("OEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, pObj->area)) {
            send_to_char("Insufficient security to edit objects.\n\r", ch);
            return;
        }

        ch->desc->ed_data = (void *)pObj;
        ch->desc->editor = ED_OBJECT;
        return;
    } else {
        if (!str_cmp(arg, "create")) {
            value = parse_int(argument);
            if (argument[0] == '\0' || value == 0) {
                send_to_char("Syntax:  edit object create [vnum]\n\r", ch);
                return;
            }

            pArea = area_getbycontainingvnum(value);
            if (!pArea) {
                send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, pArea)) {
                send_to_char("Insufficient security to edit objects.\n\r", ch);
                return;
            }

            if (oedit_create(ch, argument)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_OBJECT;
            }

            return;
        }

        if (!str_cmp(arg, "clone")) {
            value = parse_int(argument);
            if (argument[0] == '\0' || value == 0) {
                send_to_char("Syntax:  edit object create [vnum]\n\r", ch);
                return;
            }

            pArea = area_getbycontainingvnum(value);
            if (!pArea) {
                send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, pArea)) {
                send_to_char("Insufficient security to edit objects.\n\r", ch);
                return;
            }

            if (oedit_create(ch, argument)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_OBJECT;
                argument = one_argument(argument, arg);
                oedit_clone(ch, argument);
            }

            return;
        }
    }

    send_to_char("OEdit:  There is no default object to edit.\n\r", ch);
    return;
}



/*****************************************************************************
 *	oedit_show
 *
 *	show the properties of an object
 *****************************************************************************/
EDIT(oedit_show){
    struct objectprototype *pObj;
    AFFECT_DATA *paf;
    int cnt;

    EDIT_OBJ(ch, pObj);

    printf_to_char(ch, "`&Name``:         [%s]\n\r`&Area``:        [%5d] %s\n\r", pObj->name, (!pObj->area) ? -1 : pObj->area->vnum, (!pObj->area) ? "No Area" : pObj->area->name);

    printf_to_char(ch, "`&Vnum``:         [%5d]\n\r`&Type``:        [%s]\n\r", pObj->vnum, flag_string(type_flags, pObj->item_type));

    printf_to_char(ch, "`&Level``:        [%5d]\n\r", pObj->level);
    printf_to_char(ch, "`&Wear flags``:   [%s]\n\r", flag_string(wear_flags, pObj->wear_flags));
    printf_to_char(ch, "`&Extra flags``:  [%s]\n\r", flag_string(extra_flags, pObj->extra_flags));
    printf_to_char(ch, "`&Extra2 flags``: [%s]\n\r", flag_string(extra2_flags, pObj->extra2_flags));
    printf_to_char(ch, "`&Material``:     [%s]\n\r", pObj->material);
    printf_to_char(ch, "`&Condition``:    [%5d]\n\r", pObj->condition);
    printf_to_char(ch, "`&Timer``:        [%5d]\n\r", pObj->init_timer);
    printf_to_char(ch, "`&Weight``:       [%5d]\n\r`&Cost``:        [%5d]\n\r", pObj->weight, pObj->cost);

    printf_to_char(ch, "`&Xp to level``:  [%5d]\n\r",            /* ROM */ pObj->xp_tolevel);

    if (pObj->extra_descr) {
        struct extra_descr_data *ed;

        send_to_char("`&Ex desc kwd``: [", ch);

        for (ed = pObj->extra_descr; ed; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char(" ", ch);
        }
        send_to_char("]\n\r", ch);
    }

    printf_to_char(ch, "`&Short desc``:  %s\n\r`&Long desc``:\n\r     %s\n\r",
                   pObj->short_descr,
                   pObj->description);


    for (cnt = 0, paf = pObj->affected; paf; paf = paf->next) {
        if (cnt == 0) {
            send_to_char("`&Number Modifier Type    Affects``\n\r", ch);
            send_to_char("`1------ -------- ------- -------``\n\r", ch);
        }
        switch (paf->where) {
          case TO_AFFECTS:
              printf_to_char(ch, "[%4d] %-8d affect  %s\n\r", cnt,
                             paf->modifier,
                             flag_string(affect_flags, paf->bitvector));
              break;
          case TO_IMMUNE:
              printf_to_char(ch, "[%4d] %-8d immune  %s\n\r", cnt,
                             paf->modifier,
                             flag_string(imm_flags, paf->bitvector));
              break;
          case TO_RESIST:
              printf_to_char(ch, "[%4d] %-8d resist  %s\n\r", cnt,
                             paf->modifier,
                             flag_string(res_flags, paf->bitvector));
              break;
          case TO_VULN:
              printf_to_char(ch, "[%4d] %-8d vuln   %s\n\r", cnt,
                             paf->modifier,
                             flag_string(vuln_flags, paf->bitvector));
              break;
          default:
              printf_to_char(ch, "[%4d] %-8d apply   %s\n\r", cnt,
                             paf->modifier,
                             flag_string(apply_flags, paf->location));
        }
        cnt++;
    }

    send_to_char("\n\r", ch);
    show_obj_values(ch, pObj);
    return false;
}



/*****************************************************************************
 *	oedit_create
 *
 *	create a new object
 *****************************************************************************/
EDIT(oedit_create){
    struct objectprototype *pObj;
    struct area_data *pArea;
    long value;

    value = parse_int(argument);
    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
        return false;
    }

    pArea = area_getbycontainingvnum(value);
    if (!pArea) {
        send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return false;
    }

    if (objectprototype_getbyvnum(value)) {
        send_to_char("OEdit:  Object vnum already exists.\n\r", ch);
        return false;
    }

    pObj = objectprototype_new(value);
    pObj->area = pArea;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    ch->desc->ed_data = (void *)pObj;

    send_to_char("Object Created.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_clone
 *
 *	clone one objects properties to another
 *****************************************************************************/
EDIT(oedit_clone){
    struct objectprototype *pObj;
    struct objectprototype *pClone;
    AFFECT_DATA *pAff;
    AFFECT_DATA *pNew;
    int value;
    int iter;

    EDIT_OBJ(ch, pObj);
    value = parse_int(argument);
    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax:  clone [existing vnum]\n\r", ch);
        return false;
    }

    if ((pClone = objectprototype_getbyvnum(value)) == NULL) {
        send_to_char("OEdit:  The source object does not exist.\n\r", ch);
        return false;
    }

    free_string(pObj->name);
    free_string(pObj->short_descr);
    free_string(pObj->description);
    free_string(pObj->material);

    pObj->name = str_dup(pClone->name);
    pObj->short_descr = str_dup(pClone->short_descr);
    pObj->description = str_dup(pClone->description);
    pObj->material = str_dup(pClone->material);
    pObj->level = pClone->level;
    pObj->item_type = pClone->item_type;
    pObj->extra_flags = pClone->extra_flags;
    pObj->extra2_flags = pClone->extra2_flags;
    pObj->wear_flags = pClone->wear_flags;
    pObj->condition = pClone->condition;
    pObj->weight = pClone->weight;

    for (iter = 0; iter < 5; iter++)
        pObj->value[iter] = pClone->value[iter];

    for (pAff = pClone->affected; pAff != NULL; pAff = pAff->next) {
        pNew = new_affect();
        pNew->location = pAff->location;
        pNew->modifier = pAff->modifier;
        pNew->where = pAff->where;
        pNew->type = pAff->type;
        pNew->duration = pAff->duration;
        pNew->bitvector = pAff->bitvector;
        pNew->level = pAff->level;
        pNew->next = pObj->affected;
        pObj->affected = pNew;
    }

    send_to_char("Object Cloned.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	oedit_addaffect
 *
 *	adds an affect to the object
 *****************************************************************************/
EDIT(oedit_addaffect){
    struct objectprototype *pObj;
    AFFECT_DATA *pAff;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    int value;

    EDIT_OBJ(ch, pObj);
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

    pAff = new_affect();
    pAff->location = value;
    pAff->modifier = parse_int(mod);
    pAff->where = TO_OBJECT;
    pAff->type = -1;
    pAff->duration = -1;
    pAff->bitvector = 0;
    pAff->level = pObj->level;
    pAff->next = pObj->affected;
    pObj->affected = pAff;

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
EDIT(oedit_addapply){
    struct objectprototype *pObj;
    AFFECT_DATA *pAff;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    char type[MAX_STRING_LENGTH];
    char bvector[MAX_STRING_LENGTH];
    int value;
    int bitvector;
    int typ;

    EDIT_OBJ(ch, pObj);
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

    pAff = new_affect();
    pAff->location = value;
    pAff->modifier = parse_int(mod);
    pAff->where = (int)apply_types[typ].bit;
    pAff->type = -1;
    pAff->duration = -1;
    pAff->bitvector = bitvector;
    pAff->level = pObj->level;
    pAff->next = pObj->affected;
    pObj->affected = pAff;

    send_to_char("Apply added.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	oedit_delaffect
 *
 *	deletes an affect from the object
 *****************************************************************************/
EDIT(oedit_delaffect){
    struct objectprototype *pObj;
    AFFECT_DATA *pAff;
    AFFECT_DATA *pAff_next;
    char affect[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_OBJ(ch, pObj);
    one_argument(argument, affect);
    if (!is_number(affect) || affect[0] == '\0') {
        send_to_char("Syntax:  delaffect [#xaffect]\n\r", ch);
        return false;
    }

    value = parse_int(affect);
    if (value < 0) {
        send_to_char("Only non-negative affect-numbers allowed.\n\r", ch);
        return false;
    }

    if (!(pAff = pObj->affected)) {
        send_to_char("OEdit:  Non-existant affect.\n\r", ch);
        return false;
    }

    if (value == 0) {
        /* remove the first affect */
        pAff = pObj->affected;
        pObj->affected = pAff->next;
        free_affect(pAff);
    } else {
        /* find the affect */
        while ((pAff_next = pAff->next) && (++cnt < value))
            pAff = pAff_next;

        if (pAff_next) {
            pAff->next = pAff_next->next;
            free_affect(pAff_next);
        } else {
            send_to_char("No such affect.\n\r", ch);
            return false;
        }
    }

    send_to_char("Affect removed.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	oedit_name
 *
 *	edits the name property of the object
 *****************************************************************************/
EDIT(oedit_name){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  name [string]\n\r", ch);
        return false;
    }

    free_string(pObj->name);
    pObj->name = str_dup(argument);
    send_to_char("Name set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_short
 *
 *	edits the short description of the object
 *****************************************************************************/
EDIT(oedit_short){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  short [string]\n\r", ch);
        return false;
    }

    free_string(pObj->short_descr);
    pObj->short_descr = str_dup(argument);
    send_to_char("Short description set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_long
 *
 *	edits the long description of the object
 *****************************************************************************/
EDIT(oedit_long){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  long [string]\n\r", ch);
        return false;
    }

    free_string(pObj->description);
    pObj->description = str_dup(argument);
    send_to_char("Long description set.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	oedit_values
 *
 *	edit the five values on an object
 *	single point of entry/failure for the oedit_valueN functions
 *****************************************************************************/
static bool oedit_values(struct char_data *ch, const char *argument, int value)
{
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);

    return set_value(ch, pObj, argument, value);
}


/*****************************************************************************
 *	oedit_value0
 *
 *	edit v0
 *****************************************************************************/
EDIT(oedit_value0){
    return oedit_values(ch, argument, 0);
}

/*****************************************************************************
 *	oedit_value1
 *
 *	edit v1
 *****************************************************************************/
EDIT(oedit_value1){
    return oedit_values(ch, argument, 1);
}

/*****************************************************************************
 *	oedit_value2
 *
 *	edit v2
 *****************************************************************************/
EDIT(oedit_value2){
    return oedit_values(ch, argument, 2);
}

/*****************************************************************************
 *	oedit_value3
 *
 *	edit v3
 *****************************************************************************/
EDIT(oedit_value3){
    return oedit_values(ch, argument, 3);
}

/*****************************************************************************
 *	oedit_value4
 *
 *	edit v4
 *****************************************************************************/
EDIT(oedit_value4){
    return oedit_values(ch, argument, 4);
}



/*****************************************************************************
 *	oedit_weight
 *
 *	edit the weight property of the object
 *****************************************************************************/
EDIT(oedit_weight){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  weight [number]\n\r", ch);
        return false;
    }

    pObj->weight = parse_int(argument);
    send_to_char("Weight set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_cost
 *
 *	edit the cost property of the object
 *****************************************************************************/
EDIT(oedit_cost){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  cost [number]\n\r", ch);
        return false;
    }

    pObj->cost = parse_unsigned_int(argument);
    send_to_char("Cost set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_ed
 *
 *	edit the extra description of an object
 *****************************************************************************/
EDIT(oedit_ed){
    struct objectprototype *pObj;
    struct extra_descr_data *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_OBJ(ch, pObj);
    argument = one_argument(argument, command);
    one_argument(argument, keyword);

    if (command[0] == '\0') {
        send_to_char("Syntax:  ed add [keyword]\n\r", ch);
        send_to_char("         ed delete [keyword]\n\r", ch);
        send_to_char("         ed edit [keyword]\n\r", ch);
        send_to_char("         ed format [keyword]\n\r", ch);
        return false;
    }

    if (!str_cmp(command, "add")) {
        if (keyword[0] == '\0') {
            send_to_char("Syntax:  ed add [keyword]\n\r", ch);
            return false;
        }

        ed = new_extra_descr();
        ed->keyword = str_dup(keyword);
        ed->next = pObj->extra_descr;
        pObj->extra_descr = ed;

        string_append(ch, &ed->description);
        return true;
    }

    if (!str_cmp(command, "edit")) {
        if (keyword[0] == '\0') {
            send_to_char("Syntax:  ed edit [keyword]\n\r", ch);
            return false;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
            if (is_name(keyword, ed->keyword))
                break;

        if (!ed) {
            send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
            return false;
        }

        string_append(ch, &ed->description);
        return true;
    }

    if (!str_cmp(command, "delete")) {
        struct extra_descr_data *ped = NULL;

        if (keyword[0] == '\0') {
            send_to_char("Syntax:  ed delete [keyword]\n\r", ch);
            return false;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next) {
            if (is_name(keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed) {
            send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
            return false;
        }

        if (!ped)
            pObj->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr(ed);
        send_to_char("Extra description deleted.\n\r", ch);
        return true;
    }


    if (!str_cmp(command, "format")) {
        if (keyword[0] == '\0') {
            send_to_char("Syntax:  ed format [keyword]\n\r", ch);
            return false;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next) {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed) {
            send_to_char("OEdit:  Extra description keyword not found.\n\r", ch);
            return false;
        }

        ed->description = format_string(ed->description);
        send_to_char("Extra description formatted.\n\r", ch);
        return true;
    }

    oedit_ed(ch, "");
    return false;
}





/*****************************************************************************
 *	oedit_extra
 *
 *	edit the extra flags for an object
 *****************************************************************************/
EDIT(oedit_extra){
    struct objectprototype *pObj;
    int value;

    EDIT_OBJ(ch, pObj);
    if (argument[0] != '\0') {
        if ((value = flag_value(extra_flags, argument)) != NO_FLAG) {
            TOGGLE_BIT(pObj->extra_flags, value);
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
    struct objectprototype *pObj;
    int value;

    EDIT_OBJ(ch, pObj);
    if (argument[0] != '\0') {
        if ((value = flag_value(extra2_flags, argument)) != NO_FLAG) {
            TOGGLE_BIT(pObj->extra2_flags, value);
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
    struct objectprototype *pObj;
    int value;

    EDIT_OBJ(ch, pObj);
    if (argument[0] != '\0') {
        if ((value = flag_value(wear_flags, argument)) != NO_FLAG) {
            TOGGLE_BIT(pObj->wear_flags, value);
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
    struct objectprototype *pObj;
    int value;

    EDIT_OBJ(ch, pObj);
    if (argument[0] != '\0') {
        if ((value = flag_value(type_flags, argument)) != NO_FLAG) {
            pObj->item_type = value;
            send_to_char("Type set.\n\r", ch);

            /* clear the values */
            pObj->value[0] = 0;
            pObj->value[1] = 0;
            pObj->value[2] = 0;
            pObj->value[3] = 0;
            pObj->value[4] = 0;
            return true;
        }
    }

    send_to_char("Syntax:  type [flag]\n\r"
                 "Type '? type' for a list of flags.\n\r", ch);
    return false;
}

/*****************************************************************************
 *	oedit_material
 *
 *	edit the material property of an object
 *****************************************************************************/
EDIT(oedit_material){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  material [string]\n\r", ch);
        return false;
    }

    free_string(pObj->material);
    pObj->material = str_dup(argument);
    send_to_char("Material set.\n\r", ch);
    return true;
}

/** edit the timer property of an object (auto destroy)	Added by Monrick, 5/2008 */
EDIT(oedit_timer){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  timer [# of ticks]  (0 for infinite)\n\r", ch);
        return false;
    }

    pObj->init_timer = parse_int(argument);
    send_to_char("Timer set.\n\r", ch);
    return true;
}

/*****************************************************************************
 *	oedit_level
 *
 *	edit the level of the object
 *****************************************************************************/
EDIT(oedit_level){
    struct objectprototype *pObj;

    EDIT_OBJ(ch, pObj);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  level [number]\n\r", ch);
        return false;
    }

    pObj->level = parse_int(argument);
    send_to_char("Level set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	oedit_condition
 *
 *	edit the condition of the object
 *****************************************************************************/
EDIT(oedit_condition){
    struct objectprototype *pObj;
    int value;

    EDIT_OBJ(ch, pObj);
    if (argument[0] != '\0'
        && (value = parse_int(argument)) >= 0
        && (value <= 100)) {
        pObj->condition = value;
        send_to_char("Condition set.\n\r", ch);
        return true;
    }

    send_to_char("Syntax:  condition [number]\n\r"
                 "Where number can range from 0(ruined) to 100(perfect).\n\r", ch);
    return false;
}


EDIT(oedit_xptolevel){
    struct objectprototype *pObj;
    int amount;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  xptolevel [number]\n\r", ch);
        return false;
    }

    amount = parse_int(argument);

    if (amount < 0 || amount > 5000) {
        send_to_char("Please choose an amount between 0 and 5000\n\r", ch);
        return false;
    }

    pObj->xp_tolevel = parse_int(argument);

    send_to_char("Exp to level set.\n\r", ch);
    return true;
}
