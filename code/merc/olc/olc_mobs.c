/***************************************************************************
 *  File: olc_medit.c                                                      *
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
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"
#ifndef S_SPLINT_S
#include <ctype.h>
#endif



/***************************************************************************
 * IMPORTS
 ***************************************************************************/
extern void mob_auto_hit_dice(MOB_INDEX_DATA *mix, enum medit_auto_config_type auto_config_type);
extern void string_append(CHAR_DATA * ch, char **string);



/*****************************************************************************
 *	do_medit
 *
 *	entry level function for mob editing
 *****************************************************************************/
void do_medit(CHAR_DATA *ch, const char *argument)
{
    MOB_INDEX_DATA *mob_idx;
    AREA_DATA *area;
    long value;
    char arg[MSL];

    DENY_NPC(ch);

    argument = one_argument(argument, arg);
    if (is_number(arg)) {
        value = parse_long(arg);

        if (!(mob_idx = get_mob_index(value))) {
            send_to_char("MEdit: That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, mob_idx->area)) {
            send_to_char("MEdit: Insufficient security to edit mobs in this area.\n\r", ch);
            return;
        }

        ch->desc->ed_data = (void *)mob_idx;
        ch->desc->editor = ED_MOBILE;
        return;
    } else {
        if (!str_cmp(arg, "create")) {
            value = parse_int(argument);
            if (arg[0] == '\0' || value == 0) {
                send_to_char("MEdit: Syntax: edit mobile create [vnum]\n\r", ch);
                return;
            }

            area = get_vnum_area(value);
            if (!area) {
                send_to_char("MEdit: That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, area)) {
                send_to_char("MEdit: Insufficient security to modify mobiles in this area.\n\r", ch);
                return;
            }

            if (medit_create(ch, argument)) {
                SET_BIT(area->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_MOBILE;
            }
            return;
        }

        if (!str_cmp(arg, "clone")) {
            one_argument(argument, arg);
            value = parse_int(arg);
            if (argument[0] == '\0' || arg[0] == '\0' || value == 0) {
                send_to_char("MEdit: Syntax: medit clone [new vnum] [existing vnum]\n\r", ch);
                return;
            }

            area = get_vnum_area(value);
            if (!area) {
                send_to_char("MEdit: That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, area)) {
                send_to_char("MEdit: Insufficient security to modify mobiles in this area.\n\r", ch);
                return;
            }

            if (medit_create(ch, argument)) {
                argument = one_argument(argument, arg);

                SET_BIT(area->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_MOBILE;
                medit_clone(ch, argument);
            }

            return;
        }
    }

    send_to_char("MEdit:  There is no default mobile to edit.\n\r", ch);
    return;
}


/*****************************************************************************
 *	medit_show
 *
 *	show the properties of a mobile
 *****************************************************************************/
EDIT(medit_show){
    MOB_INDEX_DATA *mob_idx;
    MPROG_LIST *list;

    EDIT_MOB(ch, mob_idx);
    printf_to_char(ch, "`&Name``:        [%s]\n\r`&Area``:        [%5d] %s\n\r",
                   mob_idx->player_name,
                   !mob_idx->area ? -1        : mob_idx->area->vnum,
                   !mob_idx->area ? "No Area" : mob_idx->area->name);
    printf_to_char(ch, "`&Act``:         [%s]\n\r", flag_string(act_flags, mob_idx->act));
    printf_to_char(ch, "`&Vnum``:        [%5d]   `&Sex``:   [%s]   `&Race``:    [%s]\n\r",
                   mob_idx->vnum,
                   mob_idx->sex == SEX_MALE    ? " male " :
                   mob_idx->sex == SEX_FEMALE  ? "female" :
                   mob_idx->sex == 3           ? "random" : "neuter",
                   race_table[mob_idx->race].name);

    printf_to_char(ch, "`&Level``:       [%5d]   `&Hitroll``: [%2d]    \n\r`&Dam Type``:    [%s]\n\r",
                   mob_idx->level,
                   mob_idx->hitroll,
                   attack_table[mob_idx->dam_type].name);

    if (mob_idx->group)
        printf_to_char(ch, "`&Group``:       [%5d]\n\r", mob_idx->group);

    printf_to_char(ch, "`&Hit dice``:    [%2d`Od``%-3d`4+``%4d] (%5d - %5d)\n\r",
                   mob_idx->hit[DICE_NUMBER],
                   mob_idx->hit[DICE_TYPE],
                   mob_idx->hit[DICE_BONUS],
                   (int)(mob_idx->hit[DICE_NUMBER] + mob_idx->hit[DICE_BONUS]),
                   (int)(mob_idx->hit[DICE_NUMBER] * mob_idx->hit[DICE_TYPE] + mob_idx->hit[DICE_BONUS]));

    printf_to_char(ch, "`&Damage dice``: [%2d`Od``%-3d`4+``%4d]\n\r",
                   mob_idx->damage[DICE_NUMBER],
                   mob_idx->damage[DICE_TYPE],
                   mob_idx->damage[DICE_BONUS]);

    printf_to_char(ch, "`&Mana dice``:   [%2d`Od``%-3d`4+``%4d]\n\r",
                   mob_idx->mana[DICE_NUMBER],
                   mob_idx->mana[DICE_TYPE],
                   mob_idx->mana[DICE_BONUS]);

    printf_to_char(ch, "`&Affected by``: [%s]\n\r",
                   flag_string(affect_flags, mob_idx->affected_by));

    printf_to_char(ch, "`&Armor``:       [`#pierce``: %d  `#bash``: %d  `#slash``: %d  `#magic``: %d]\n\r",
                   mob_idx->ac[AC_PIERCE],
                   mob_idx->ac[AC_BASH],
                   mob_idx->ac[AC_SLASH],
                   mob_idx->ac[AC_EXOTIC]);

    printf_to_char(ch, "`&Form``:        [%s]\n\r",
                   flag_string(form_flags, mob_idx->form));
    printf_to_char(ch, "`&Parts``:       [%s]\n\r",
                   flag_string(part_flags, mob_idx->parts));

    printf_to_char(ch, "`&Imm``:         [%s]\n\r",
                   flag_string(imm_flags, mob_idx->imm_flags));
    printf_to_char(ch, "`&Res``:         [%s]\n\r",
                   flag_string(res_flags, mob_idx->res_flags));
    printf_to_char(ch, "`&Vuln``:        [%s]\n\r",
                   flag_string(vuln_flags, mob_idx->vuln_flags));

    printf_to_char(ch, "`&Off``:         [%s]\n\r",
                   flag_string(off_flags, mob_idx->off_flags));

    printf_to_char(ch, "`&Size``:        [%s]\n\r",
                   flag_string(size_flags, mob_idx->size));
    printf_to_char(ch, "`&Material``:    [%s]\n\r", mob_idx->material);

    printf_to_char(ch, "`&Start pos.``   [%s]\n\r",
                   flag_string(position_flags, mob_idx->start_pos));

    printf_to_char(ch, "`&Default pos``  [%s]\n\r",
                   flag_string(position_flags, mob_idx->default_pos));

    printf_to_char(ch, "`&Wealth``:      [%5ld]\n\r", mob_idx->wealth);

    printf_to_char(ch, "`&Short descr``: %s\n\r`&Long descr``:\n\r%s",
                   mob_idx->short_descr,
                   mob_idx->long_descr);

    printf_to_char(ch, "`&Description``:\n\r%s", mob_idx->description);

    if (mob_idx->shop) {
        SHOP_DATA *shop;
        int iTrade;

        shop = mob_idx->shop;

        printf_to_char(ch,
                       "`&Shop data`` for [%5d]:\n\r"
                       "  Markup for purchaser: %d%%\n\r"
                       "  Markdown for seller:  %d%%\n\r",
                       shop->keeper,
                       shop->profit_buy,
                       shop->profit_sell);
        printf_to_char(ch, "  Hours: %d to %d.\n\r",
                       shop->open_hour,
                       shop->close_hour);

        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
            if (shop->buy_type[iTrade] != 0) {
                if (iTrade == 0) {
                    send_to_char("  Number Trades Type\n\r", ch);
                    send_to_char("  ------ -----------\n\r", ch);
                }
                printf_to_char(ch, "  [%4d] %s\n\r", iTrade,
                               flag_string(type_flags, shop->buy_type[iTrade]));
            }
        }
    }

    if (mob_idx->mprogs) {
        int cnt;

        printf_to_char(ch, "\n\r`1MO`1B`!Programs`` for [`#%5d``]:\n\r", mob_idx->vnum);

        for (cnt = 0, list = mob_idx->mprogs; list; list = list->next) {
            if (cnt == 0) {
                send_to_char("`&Number  Vnum Trigger Phrase``\n\r", ch);
                send_to_char("`1------- ---- ------- ------``\n\r", ch);
            }

            printf_to_char(ch, "[%5d] %4d %7s %s\n\r", cnt,
                           list->vnum, mprog_type_to_name(list->trig_type),
                           list->trig_phrase);
            cnt++;
        }
    }

    return false;
}



/*****************************************************************************
 *	medit_create
 *
 *	create a new mobile
 *****************************************************************************/
EDIT(medit_create){
    MOB_INDEX_DATA *mob_idx;
    AREA_DATA *area;
    long value;
    long hash_idx;

    value = parse_long(argument);
    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax:  medit create [vnum]\n\r", ch);
        return false;
    }

    area = get_vnum_area(value);
    if (!area) {
        send_to_char("MEdit:  That vnum is not assigned an area.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char("MEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return false;
    }

    if (get_mob_index(value)) {
        send_to_char("MEdit:  Mobile vnum already exists.\n\r", ch);
        return false;
    }

    mob_idx = new_mob_index();
    mob_idx->vnum = value;
    mob_idx->area = area;

    if (value > top_vnum_mob)
        top_vnum_mob = value;

    mob_idx->act = ACT_IS_NPC;
    hash_idx = value % MAX_KEY_HASH;
    mob_idx->next = mob_index_hash[hash_idx];
    mob_index_hash[hash_idx] = mob_idx;
    ch->desc->ed_data = (void *)mob_idx;

    send_to_char("Mobile Created.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	medit_clone
 *
 *	create a new mobile and clone its properties from another
 *****************************************************************************/
EDIT(medit_clone){
    MOB_INDEX_DATA *mob_idx;
    MOB_INDEX_DATA *pClone;
    int value;
    int iter;

    EDIT_MOB(ch, mob_idx);
    value = parse_int(argument);
    if (argument[0] == '\0'
        || value == 0) {
        send_to_char("Syntax:  clone [existing vnum]\n\r", ch);
        return false;
    }

    if ((pClone = get_mob_index(value)) == NULL) {
        send_to_char("MEdit:  Mobile to clone does not exist.\n\r", ch);
        return false;
    }

    mob_idx->new_format = pClone->new_format;

    free_string(mob_idx->player_name);
    free_string(mob_idx->short_descr);
    free_string(mob_idx->long_descr);
    free_string(mob_idx->description);

    mob_idx->player_name = str_dup(pClone->player_name);
    mob_idx->short_descr = str_dup(pClone->short_descr);
    mob_idx->long_descr = str_dup(pClone->long_descr);
    mob_idx->description = str_dup(pClone->description);

    mob_idx->act = pClone->act;
    mob_idx->affected_by = pClone->affected_by;
    mob_idx->level = pClone->level;
    mob_idx->hitroll = pClone->hitroll;

    for (iter = 0; iter < 3; iter++)
        mob_idx->hit[iter] = pClone->hit[iter];

    for (iter = 0; iter < 3; iter++)
        mob_idx->mana[iter] = pClone->mana[iter];

    for (iter = 0; iter < 3; iter++)
        mob_idx->damage[iter] = pClone->damage[iter];

    for (iter = 0; iter < 4; iter++)
        mob_idx->ac[iter] = pClone->ac[iter];

    mob_idx->dam_type = pClone->dam_type;
    mob_idx->off_flags = pClone->off_flags;
    mob_idx->res_flags = pClone->res_flags;
    mob_idx->vuln_flags = pClone->vuln_flags;
    mob_idx->start_pos = pClone->start_pos;
    mob_idx->default_pos = pClone->default_pos;
    mob_idx->sex = pClone->sex;
    mob_idx->race = pClone->race;
    mob_idx->wealth = pClone->wealth;
    mob_idx->form = pClone->form;
    mob_idx->parts = pClone->parts;
    mob_idx->size = pClone->size;

    free_string(mob_idx->material);
    mob_idx->material = str_dup(pClone->material);

    send_to_char("Mobile Cloned.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	set the default damage type of the mob
 *****************************************************************************/
EDIT(medit_damtype){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  damtype [damage message]\n\r", ch);
        send_to_char("Para ver una lista de tipos de mensajes, pon '? weapon'.\n\r", ch);
        return false;
    }

    mob_idx->dam_type = attack_lookup(argument);
    send_to_char("Damage type set.\n\r", ch);
    return true;
}

/*****************************************************************************
 *	set the level of the mobil
 *****************************************************************************/
EDIT(medit_level){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);

    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  level [number]\n\r", ch);
        return false;
    }

    mob_idx->level = parse_int(argument);
    send_to_char("Level set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	set the description of the mobile
 *****************************************************************************/
EDIT(medit_desc){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        string_append(ch, &mob_idx->description);
        return true;
    }

    send_to_char("Syntax:  desc    - line edit\n\r", ch);
    return false;
}


/*****************************************************************************
 *	medit_long
 *
 *	set the long description of a mobile
 *****************************************************************************/
EDIT(medit_long){
    MOB_INDEX_DATA *mob_idx;
    static char buf[MSL];

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  long [string]\n\r", ch);
        return false;
    }

    free_string(mob_idx->long_descr);

    (void)snprintf(buf, UMIN(strlen(argument), MSL), "%s\n\r", argument);
    smash_tilde(buf);
    buf[0] = UPPER(buf[0]);

    mob_idx->long_descr = str_dup(buf);
    send_to_char("Long description set.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	medit_short
 *
 *	edit the short description
 *****************************************************************************/
EDIT(medit_short){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  short [string]\n\r", ch);
        return false;
    }

    free_string(mob_idx->short_descr);
    mob_idx->short_descr = str_dup(argument);

    send_to_char("Short description set.\n\r", ch);
    return true;
}



/*****************************************************************************
 *	medit_name
 *
 *	set the name of the mobile
 *****************************************************************************/
EDIT(medit_name){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  name [string]\n\r", ch);
        return false;
    }

    free_string(mob_idx->player_name);
    mob_idx->player_name = str_dup(argument);
    send_to_char("Name set.\n\r", ch);
    return true;
}


/*****************************************************************************
 *	medit_shop
 *
 *	set the shop data for a mobile
 *****************************************************************************/
EDIT(medit_shop){
    MOB_INDEX_DATA *mob_idx;
    char command[MIL];
    char arg[MIL];

    EDIT_MOB(ch, mob_idx);
    argument = one_argument(argument, command);
    argument = one_argument(argument, arg);

    if (command[0] == '\0') {
        send_to_char("Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch);
        send_to_char("         shop profit [#xbuying%] [#xselling%]\n\r", ch);
        send_to_char("         shop type [#x0-4] [item type]\n\r", ch);
        send_to_char("         shop assign\n\r", ch);
        send_to_char("         shop remove\n\r", ch);
        return false;
    }


    if (!str_cmp(command, "hours")) {
        if (arg[0] == '\0'
            || !is_number(arg)
            || argument[0] == '\0'
            || !is_number(argument)) {
            send_to_char("Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch);
            return false;
        }

        if (!mob_idx->shop) {
            send_to_char("MEdit:  A shop must be assigned to this mobile first(shop assign).\n\r", ch);
            return false;
        }

        mob_idx->shop->open_hour = parse_int(arg);
        mob_idx->shop->close_hour = parse_int(argument);

        send_to_char("Shop hours set.\n\r", ch);
        return true;
    }


    if (!str_cmp(command, "profit")) {
        if (arg[0] == '\0'
            || !is_number(arg)
            || argument[0] == '\0'
            || !is_number(argument)) {
            send_to_char("Syntax:  shop profit [#xbuying%] [#xselling%]\n\r", ch);
            return false;
        }

        if (!mob_idx->shop) {
            send_to_char("MEdit:  A shop must be assigned to this mobile first(shop assign).\n\r", ch);
            return false;
        }

        mob_idx->shop->profit_buy = parse_int(arg);
        mob_idx->shop->profit_sell = parse_int(argument);
        send_to_char("Shop profit set.\n\r", ch);
        return true;
    }


    if (!str_cmp(command, "type")) {
        int value;

        if (arg[0] == '\0'
            || !is_number(arg)
            || argument[0] == '\0') {
            send_to_char("Syntax:  shop type [#x0-4] [item type]\n\r", ch);
            return false;
        }

        if (parse_int(arg) >= MAX_TRADE) {
            printf_to_char(ch, "MEdit:  May sell %d items max.\n\r", MAX_TRADE);
            return false;
        }

        if (!mob_idx->shop) {
            send_to_char("MEdit:  A shop must be assigned to this mobile first(shop assign).\n\r", ch);
            return false;
        }

        if ((value = flag_value(type_flags, argument)) == NO_FLAG) {
            send_to_char("MEdit:  That type of item is not known.\n\r", ch);
            return false;
        }

        mob_idx->shop->buy_type[parse_int(arg)] = value;
        send_to_char("Shop type set.\n\r", ch);
        return true;
    }

    /* shop assign && shop delete by Phoenix */
    if (!str_prefix(command, "assign")) {
        if (mob_idx->shop) {
            send_to_char("Mob already has a shop assigned to it.\n\r", ch);
            return false;
        }

        mob_idx->shop = new_shop();
        if (!shop_first)
            shop_first = mob_idx->shop;

        if (shop_last)
            shop_last->next = mob_idx->shop;
        shop_last = mob_idx->shop;

        mob_idx->shop->keeper = mob_idx->vnum;

        send_to_char("New shop assigned to mobile.\n\r", ch);
        return true;
    }

    if (!str_prefix(command, "remove")) {
        SHOP_DATA *shop;

        shop = mob_idx->shop;
        mob_idx->shop = NULL;

        if (shop == shop_first) {
            if (!shop->next) {
                shop_first = NULL;
                shop_last = NULL;
            } else {
                shop_first = shop->next;
            }
        } else {
            SHOP_DATA *ishop;

            for (ishop = shop_first; ishop; ishop = ishop->next) {
                if (ishop->next == shop) {
                    if (!shop->next) {
                        shop_last = ishop;
                        shop_last->next = NULL;
                    } else {
                        ishop->next = shop->next;
                    }
                }
            }
        }

        free_shop(shop);

        send_to_char("Mobile is no longer a shopkeeper.\n\r", ch);
        return true;
    }

    medit_shop(ch, "");
    return false;
}


/***************************************************************************
 *	medit_sex
 *
 *	set the sex of the mobile
 ***************************************************************************/
EDIT(medit_sex){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(sex_flags, argument)) != NO_FLAG) {
            mob_idx->sex = value;
            send_to_char("Sex set.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: sex [sex]\n\r"
                 "Type '? sex' for a list of flags.\n\r", ch);
    return false;
}

/***************************************************************************
 *	medit_act
 *
 *	set the act flags for a mobile
 ***************************************************************************/
EDIT(medit_act){
    MOB_INDEX_DATA *mob_idx;
    long value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(act_flags, argument)) != NO_FLAG) {
            mob_idx->act ^= value;
            SET_BIT(mob_idx->act, ACT_IS_NPC);
            send_to_char("Act flag toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: act [flag]\n\r"
                 "Type '? act' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_affect
 *
 *	set the affects for the mobile
 ***************************************************************************/
EDIT(medit_affect){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(affect_flags, argument)) != NO_FLAG) {
            mob_idx->affected_by ^= value;
            send_to_char("Affect flag toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: affect [flag]\n\r"
                 "Type '? affect' for a list of flags.\n\r", ch);
    return false;
}



/***************************************************************************
 *	medit_act
 *
 *	set the description of the room
 ***************************************************************************/
EDIT(medit_ac){
    MOB_INDEX_DATA *mob_idx;
    char arg[MIL];
    long pierce;
    long bash;
    long slash;
    long exotic;

    EDIT_MOB(ch, mob_idx);
    do {
        if (argument[0] == '\0')
            break;

        argument = one_argument(argument, arg);

        if (!is_number(arg))
            break;
        pierce = parse_long(arg);
        argument = one_argument(argument, arg);

        if (arg[0] != '\0') {
            if (!is_number(arg))
                break;
            bash = parse_long(arg);
            argument = one_argument(argument, arg);
        } else {
            bash = mob_idx->ac[AC_BASH];
        }

        if (arg[0] != '\0') {
            if (!is_number(arg))
                break;
            slash = parse_long(arg);
            argument = one_argument(argument, arg);
        } else {
            slash = mob_idx->ac[AC_SLASH];
        }

        if (arg[0] != '\0') {
            if (!is_number(arg))
                break;
            exotic = parse_long(arg);
        } else {
            exotic = mob_idx->ac[AC_EXOTIC];
        }

        mob_idx->ac[AC_PIERCE] = pierce;
        mob_idx->ac[AC_BASH] = bash;
        mob_idx->ac[AC_SLASH] = slash;
        mob_idx->ac[AC_EXOTIC] = exotic;

        send_to_char("Ac set.\n\r", ch);
        return true;
    } while (false);    /* Just do it once.. */

    send_to_char("Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\n\r"
                 "help MOB_AC  gives a list of reasonable ac-values.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_form
 *
 *	set the form flags for the mob
 ***************************************************************************/
EDIT(medit_form){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(form_flags, argument)) != NO_FLAG) {
            mob_idx->form ^= value;
            send_to_char("Form toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: form [flags]\n\r"
                 "Type '? form' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_part
 *
 *	set the parts for the mobile
 ***************************************************************************/
EDIT(medit_part){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(part_flags, argument)) != NO_FLAG) {
            mob_idx->parts ^= value;
            send_to_char("Parts toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: part [flags]\n\r"
                 "Type '? part' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_imm
 *
 *	set the immunites for the mobile
 ***************************************************************************/
EDIT(medit_imm){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(imm_flags, argument)) != NO_FLAG) {
            mob_idx->imm_flags ^= value;
            send_to_char("Immunity toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: imm [flags]\n\r"
                 "Type '? imm' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_res
 *
 *	set the resistances for the mobile
 ***************************************************************************/
EDIT(medit_res){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(res_flags, argument)) != NO_FLAG) {
            mob_idx->res_flags ^= value;
            send_to_char("Resistance toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: res [flags]\n\r"
                 "Type '? res' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_vuln
 *
 *	set the vulnerabilities for the mobile
 ***************************************************************************/
EDIT(medit_vuln){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(vuln_flags, argument)) != NO_FLAG) {
            mob_idx->vuln_flags ^= value;
            send_to_char("Vulnerability toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: vuln [flags]\n\r"
                 "Type '? vuln' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_material
 *
 *	set the material for the mobile
 ***************************************************************************/
EDIT(medit_material){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  material [string]\n\r", ch);
        return false;
    }

    free_string(mob_idx->material);
    mob_idx->material = str_dup(argument);
    send_to_char("Material set.\n\r", ch);
    return true;
}


/***************************************************************************
 *	medit_off
 *
 *	set the offense flags for the mobile
 ***************************************************************************/
EDIT(medit_off){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(off_flags, argument)) != NO_FLAG) {
            mob_idx->off_flags ^= value;
            send_to_char("Offensive behavior toggled.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: off [flags]\n\r"
                 "Type '? off' for a list of flags.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_size
 *
 *	set the size of the mobile
 ***************************************************************************/
EDIT(medit_size){
    MOB_INDEX_DATA *mob_idx;
    int value;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0') {
        if ((value = flag_value(size_flags, argument)) != NO_FLAG) {
            mob_idx->size = value;
            send_to_char("Size set.\n\r", ch);
            return true;
        }
    }

    send_to_char("Syntax: size [size]\n\r"
                 "Type '? size' for a list of sizes.\n\r", ch);
    return false;
}

static bool ShowMEditHitdiceSyntax(CHAR_DATA *ch)
{
    send_to_char("MEdit (hitdice): Syntax:  hitdice <number> d <type> + <bonus>\n\r", ch);
    send_to_char("MEdit (hitdice): -or- hitdice auto <easy|norm|hard|insane>\n\r", ch);
    return false;
}

/***************************************************************************
 *	medit_hitdice
 *
 *	set the hit dice for determining number of hp a mobile has
 ***************************************************************************/
EDIT(medit_hitdice){
    MOB_INDEX_DATA *mob_idx = NULL;

    EDIT_MOB(ch, mob_idx);

    if (argument[0] == '\0')
        return ShowMEditHitdiceSyntax(ch);

    if (isdigit((int)argument[0])) {
        static char buf[MIL];
        const char *num;
        const char *type;
        const char *bonus;
        char *cp;

        /* number of dice is the first argument */
        strncpy(buf, argument, UMIN(strlen(argument), MIL));
        num = cp = buf;
        while (isdigit((int)*cp))
            ++cp;

        /* put a null character between num and type */
        while (*cp != '\0' && !isdigit((int)*cp))
            *(cp++) = '\0';
        type = cp;

        while (isdigit((int)*cp))
            ++cp;

        /* put a null between type and bonus */
        while (*cp != '\0' && !isdigit((int)*cp))
            *(cp++) = '\0';
        bonus = cp;

        while (isdigit((int)*cp))
            ++cp;

        if (*cp != '\0')
            *cp = '\0';

        if ((!is_number(num) || parse_int(num) < 1) || (!is_number(type) || parse_int(type) < 1) || (!is_number(bonus) || parse_int(bonus) < 0))
            return ShowMEditHitdiceSyntax(ch);

        mob_idx->hit[DICE_NUMBER] = parse_int(num);
        mob_idx->hit[DICE_TYPE] = parse_int(type);
        mob_idx->hit[DICE_BONUS] = parse_int(bonus);

        send_to_char("Hitdice set.\n\r", ch);
        return true;
    } else {
        char arg[MIL];
        enum medit_auto_config_type auto_config_type = mact_easy;

        argument = one_argument(argument, arg);
        if (strncmp(arg, "auto", 5))
            return ShowMEditHitdiceSyntax(ch);

        if (mob_idx->level == 0) {
            send_to_char("MEdit (hitdice): Please set mob level before using auto function.\n\r", ch);
            return false;
        }

        argument = one_argument(argument, arg);
        if (!strncmp(arg, "easy", 5))
            auto_config_type = mact_easy;
        else if (!strncmp(arg, "norm", 5))
            auto_config_type = mact_normal;
        else if (!strncmp(arg, "hard", 5))
            auto_config_type = mact_hard;
        else if (!strncmp(arg, "insane", 5))
            auto_config_type = mact_insane;
        else
            return ShowMEditHitdiceSyntax(ch);

        mob_auto_hit_dice(mob_idx, auto_config_type);
        send_to_char("Hitdice auto configured.\n\r", ch);
        return true;
    }
}




/***************************************************************************
 *	medit_manadice
 *
 *	set the manadice for the mobile which determines the total amount
 *	of mana it gets when it loads
 ***************************************************************************/
EDIT(medit_manadice){
    MOB_INDEX_DATA *mob_idx;
    static char buf[MIL];
    const char *num;
    const char *type;
    const char *bonus;
    char *cp;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  manadice <number> d <type> + <bonus>\n\r", ch);
        return false;
    }

    /* num is the first argument */
    strncpy(buf, argument, UMIN(strlen(argument), MIL));
    num = cp = buf;
    while (isdigit((int)*cp))
        ++cp;

    /* separate num from type by nulling any spaces */
    while (*cp != '\0' && !isdigit((int)*cp))
        *(cp++) = '\0';
    type = cp;

    while (isdigit((int)*cp))
        ++cp;
    /* separate type from bonus by nulling any spaces */
    while (*cp != '\0' && !isdigit((int)*cp))
        *(cp++) = '\0';
    bonus = cp;

    while (isdigit((int)*cp))
        ++cp;
    /* null the rest of the string */
    if (*cp != '\0')
        *cp = '\0';

    if (!(is_number(num) && is_number(type) && is_number(bonus))) {
        send_to_char("Syntax:  manadice <number> d <type> + <bonus>\n\r", ch);
        return false;
    }

    if ((!is_number(num) || parse_int(num) < 1)
        || (!is_number(type) || parse_int(type) < 1)
        || (!is_number(bonus) || parse_int(bonus) < 0)) {
        send_to_char("Syntax:  manadice <number> d <type> + <bonus>\n\r", ch);
        return false;
    }

    mob_idx->mana[DICE_NUMBER] = parse_int(num);
    mob_idx->mana[DICE_TYPE] = parse_int(type);
    mob_idx->mana[DICE_BONUS] = parse_int(bonus);
    send_to_char("Manadice set.\n\r", ch);
    return true;
}


/***************************************************************************
 *	medit_damdice
 *
 *	set the damage dice for a mobile which determines how hard the
 *	loaded mobile will hit
 ***************************************************************************/
EDIT(medit_damdice){
    MOB_INDEX_DATA *mob_idx;
    static char buf[MIL];
    const char *num;
    const char *type;
    const char *bonus;
    char *cp;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax:  damdice <number> d <type> + <bonus>\n\r", ch);
        return false;
    }

    strncpy(buf, argument, UMIN(strlen(argument), MIL));
    /* num is the first argument */
    num = cp = buf;
    while (isdigit((int)*cp))
        ++cp;

    /* separate num from type by padding nulls for spaces */
    while (*cp != '\0' && !isdigit((int)*cp))
        *(cp++) = '\0';
    type = cp;

    while (isdigit((int)*cp))
        ++cp;

    /* separate type from bonus by padding the nulls for spaces */
    while (*cp != '\0' && !isdigit((int)*cp))
        *(cp++) = '\0';
    bonus = cp;

    while (isdigit((int)*cp))
        ++cp;

    /* pad the rest of the string with nulls */
    if (*cp != '\0')
        *cp = '\0';

    if (!(is_number(num) && is_number(type) && is_number(bonus))) {
        send_to_char("Syntax:  damdice <number> d <type> + <bonus>\n\r", ch);
        return false;
    }

    if ((!is_number(num) || parse_int(num) < 1) || (!is_number(type) || parse_int(type) < 1) || (!is_number(bonus) || parse_int(bonus) < 0)) {
        send_to_char("Syntax:  damdice <number> d <type> + <bonus>\n\r", ch);
        return false;
    }

    mob_idx->damage[DICE_NUMBER] = parse_int(num);
    mob_idx->damage[DICE_TYPE] = parse_int(type);
    mob_idx->damage[DICE_BONUS] = parse_int(bonus);
    send_to_char("Damdice set.\n\r", ch);
    return true;
}


/***************************************************************************
 *	medit_race
 *
 *	set the race of the mobile
 ***************************************************************************/
EDIT(medit_race){
    MOB_INDEX_DATA *mob_idx;
    int race;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] != '\0'
        && (race = race_lookup(argument)) != 0) {
        mob_idx->race = race;
        mob_idx->act |= race_table[race].act;
        mob_idx->affected_by |= race_table[race].aff;
        mob_idx->off_flags |= race_table[race].off;
        mob_idx->imm_flags |= race_table[race].imm;
        mob_idx->res_flags |= race_table[race].res;
        mob_idx->vuln_flags |= race_table[race].vuln;
        mob_idx->form |= race_table[race].form;
        mob_idx->parts |= race_table[race].parts;

        send_to_char("Race set.\n\r", ch);
        return true;
    }

    if (argument[0] == '?') {
        send_to_char("Available races are:", ch);
        for (race = 0; race_table[race].name != NULL; race++) {
            if ((race % 3) == 0)
                send_to_char("\n\r", ch);
            printf_to_char(ch, " %-15s", race_table[race].name);
        }
        send_to_char("\n\r", ch);
        return false;
    }

    send_to_char("Syntax:  race [race]\n\r"
                 "Type 'race ?' for a list of races.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_act
 *
 *	set the description of the room
 ***************************************************************************/
EDIT(medit_position){
    MOB_INDEX_DATA *mob_idx;
    char arg[MIL];
    int value;

    EDIT_MOB(ch, mob_idx);
    argument = one_argument(argument, arg);
    switch (arg[0]) {
      default:
          break;
      case 'S':
      case 's':
          if (str_prefix(arg, "start"))
              break;

          if ((value = flag_value(position_flags, argument)) == NO_FLAG)
              break;

          mob_idx->start_pos = value;
          send_to_char("Start position set.\n\r", ch);
          return true;

      case 'D':
      case 'd':
          if (str_prefix(arg, "default"))
              break;

          if ((value = flag_value(position_flags, argument)) == NO_FLAG)
              break;

          mob_idx->default_pos = value;
          send_to_char("Default position set.\n\r", ch);
          return true;
    }

    send_to_char("Syntax:  position [start/default] [position]\n\r"
                 "Type '? position' for a list of positions.\n\r", ch);
    return false;
}


/***************************************************************************
 *	medit_gold
 *
 *	set the amount of gold for a mobile
 ***************************************************************************/
EDIT(medit_gold){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  wealth [number]\n\r", ch);
        return false;
    }

    mob_idx->wealth = parse_unsigned_int(argument);
    send_to_char("Wealth set.\n\r", ch);
    return true;
}


/***************************************************************************
 *	medit_act
 *
 *	set the hitroll of the mobile
 ***************************************************************************/
EDIT(medit_hitroll){
    MOB_INDEX_DATA *mob_idx;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax:  hitroll [number]\n\r", ch);
        return false;
    }

    mob_idx->hitroll = parse_int(argument);
    send_to_char("Hitroll set.\n\r", ch);
    return true;
}


/***************************************************************************
 *	medit_group
 *
 *	set the group for the mobile
 ***************************************************************************/
EDIT(medit_group){
    MOB_INDEX_DATA *mob_idx;
    MOB_INDEX_DATA *pMTemp;
    BUFFER *buffer;
    char arg[MSL];
    char buf[MSL];
    int temp;
    bool found = false;

    EDIT_MOB(ch, mob_idx);
    if (argument[0] == '\0') {
        send_to_char("Syntax: group [number]\n\r", ch);
        send_to_char("        group show [number]\n\r", ch);
        return false;
    }

    if (is_number(argument)) {
        mob_idx->group = parse_int(argument);
        send_to_char("Group set.\n\r", ch);
        return true;
    }

    argument = one_argument(argument, arg);
    if (!strcmp(arg, "show") && is_number(argument)) {
        if (parse_int(argument) == 0) {
            send_to_char("Are you crazy?\n\r", ch);
            return false;
        }

        buffer = new_buf();

        for (temp = 0; temp < 65536; temp++) {
            pMTemp = get_mob_index(temp);
            if (pMTemp && (pMTemp->group == parse_int(argument))) {
                found = true;
                sprintf(buf, "[%7ld] %s\n\r", pMTemp->vnum, pMTemp->player_name);
                add_buf(buffer, buf);
            }
        }

        if (found)
            page_to_char(buf_string(buffer), ch);
        else
            send_to_char("No mobs in that group.\n\r", ch);

        free_buf(buffer);
        return false;
    }

    return false;
}


/***************************************************************************
 *	medit_addmpropg
 *
 *	add a mob program for a mobile
 ***************************************************************************/
EDIT(medit_addmprog){
    MOB_INDEX_DATA *mob_idx;
    MPROG_LIST *list;
    MPROG_CODE *code;
    char trigger[MSL];
    char phrase[MSL];
    char num[MSL];
    int value;

    EDIT_MOB(ch, mob_idx);
    argument = one_argument(argument, num);
    argument = one_argument(argument, trigger);
    argument = one_argument(argument, phrase);

    if (!is_number(num) || trigger[0] == '\0' || phrase[0] == '\0') {
        send_to_char("Syntax:   addmprog [vnum] [trigger] [phrase]\n\r", ch);
        return false;
    }

    if ((value = flag_value(mprog_flags, trigger)) == NO_FLAG) {
        send_to_char("Valid flags are:\n\r", ch);
        show_olc_help(ch, "mprog");
        return false;
    }

    if ((code = get_mprog_index(parse_int(num))) == NULL) {
        send_to_char("No such MOBProgram.\n\r", ch);
        return false;
    }

    list = new_mprog();
    list->vnum = parse_int(num);
    list->trig_type = value;
    list->trig_phrase = str_dup(phrase);
    list->code = code->code;

    SET_BIT(mob_idx->mprog_flags, value);
    list->next = mob_idx->mprogs;
    mob_idx->mprogs = list;

    send_to_char("Mprog Added.\n\r", ch);
    return true;
}


/***************************************************************************
 *	medit_delmprog
 *
 *	delete a mob program from a mobile
 ***************************************************************************/
EDIT(medit_delmprog){
    MOB_INDEX_DATA *mob_idx;
    MPROG_LIST *list;
    MPROG_LIST *list_next;
    char mprog[MSL];
    int value;
    int cnt = 0;

    EDIT_MOB(ch, mob_idx);
    one_argument(argument, mprog);
    if (!is_number(mprog) || mprog[0] == '\0') {
        send_to_char("Syntax:  delmprog [#mprog]\n\r", ch);
        return false;
    }

    value = parse_int(mprog);
    if (value < 0) {
        send_to_char("Only non-negative mprog-numbers allowed.\n\r", ch);
        return false;
    }

    if (!(list = mob_idx->mprogs)) {
        send_to_char("MEdit:  Non existant mprog.\n\r", ch);
        return false;
    }

    if (value == 0) {
        REMOVE_BIT(mob_idx->mprog_flags, mob_idx->mprogs->trig_type);

        list = mob_idx->mprogs;
        mob_idx->mprogs = list->next;

        free_mprog(list);
    } else {
        while ((list_next = list->next) && (++cnt < value))
            list = list_next;

        if (list_next) {
            REMOVE_BIT(mob_idx->mprog_flags, list_next->trig_type);

            list->next = list_next->next;
            free_mprog(list_next);
        } else {
            send_to_char("No such mprog.\n\r", ch);
            return false;
        }
    }

    /*
     *	this was a bug - removing the trig_type bit on the flags
     *	is bad when there is another mprog with the same trigger
     *
     *	just set it to 0 and re-set them all
     */
    mob_idx->mprog_flags = 0;
    for (list = mob_idx->mprogs; list != NULL; list = list->next)
        SET_BIT(mob_idx->mprog_flags, list->trig_type);

    send_to_char("Mprog removed.\n\r", ch);
    return true;
}
