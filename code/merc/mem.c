/***************************************************************************
 *  File: mem.c                                                            *
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
#include "merc.h"


/** exports */
EXTRA_DESCR_DATA *extra_descr_free;
void free_extra_descr(EXTRA_DESCR_DATA * extra);
void free_affect(AFFECT_DATA * af);
void free_mprog(MPROG_LIST * mp);


/** imports */
extern char str_empty[1];
extern int top_reset;
extern long top_exit;
extern int top_ed;
extern long top_room;
extern long top_mob_index;
extern int top_mprog_index;


/** locals */
static EXIT_DATA *exit_free;
static ROOM_INDEX_DATA *room_index_free;
static SHOP_DATA *shop_free;
static MOB_INDEX_DATA *mob_index_free;
static RESET_DATA *reset_free;



RESET_DATA *new_reset_data(void)
{
    RESET_DATA *reset;

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



void free_reset_data(RESET_DATA *reset)
{
    reset->next = reset_free;
    reset_free = reset;
    return;
}





EXIT_DATA *new_exit(void)
{
    EXIT_DATA *exit;

    if (!exit_free) {
        exit = alloc_perm((unsigned int)sizeof(*exit));
        top_exit++;
    } else {
        exit = exit_free;
        exit_free = exit_free->next;
    }

    exit->u1.to_room = NULL;                  /* ROM OLC */
    exit->next = NULL;
    /*  exit->vnum         =   0;                        ROM OLC */
    exit->exit_info = 0;
    exit->key = 0;
    exit->keyword = &str_empty[0];
    exit->description = &str_empty[0];
    exit->rs_flags = 0;

    return exit;
}



void free_exit(EXIT_DATA *exit)
{
    free_string(exit->keyword);
    free_string(exit->description);

    exit->next = exit_free;
    exit_free = exit;
    return;
}


ROOM_INDEX_DATA *new_room_index(void)
{
    ROOM_INDEX_DATA *room;
    int door;

    if (!room_index_free) {
        room = alloc_perm((unsigned int)sizeof(*room));
        top_room++;
    } else {
        room = room_index_free;
        room_index_free = room_index_free->next;
    }

    room->next = NULL;
    room->people = NULL;
    room->contents = NULL;
    room->extra_descr = NULL;
    room->area = NULL;
    room->affected = NULL;

    for (door = 0; door < MAX_DIR; door++)
        room->exit[door] = NULL;

    room->name = &str_empty[0];
    room->description = &str_empty[0];
    room->owner = &str_empty[0];
    room->vnum = 0;
    room->room_flags = 0;
    room->light = 0;
    room->sector_type = 0;
    room->heal_rate = 100;
    room->mana_rate = 100;

    return room;
}



void free_room_index(ROOM_INDEX_DATA *room)
{
    EXTRA_DESCR_DATA *extra;
    RESET_DATA *reset;
    long door;

    free_string(room->name);
    free_string(room->description);
    free_string(room->owner);

    for (door = 0; door < MAX_DIR; door++)
        if (room->exit[door])
            free_exit(room->exit[door]);

    for (extra = room->extra_descr; extra; extra = extra->next)
        free_extra_descr(extra);

    for (reset = room->reset_first; reset; reset = reset->next)
        free_reset_data(reset);

    room->next = room_index_free;
    room_index_free = room;
    return;
}

extern AFFECT_DATA *affect_free;


SHOP_DATA *new_shop(void)
{
    SHOP_DATA *shop;
    int buy;

    if (!shop_free) {
        shop = alloc_perm((unsigned int)sizeof(*shop));
        top_shop++;
    } else {
        shop = shop_free;
        shop_free = shop_free->next;
    }

    shop->next = NULL;
    shop->keeper = 0;

    for (buy = 0; buy < MAX_TRADE; buy++)
        shop->buy_type[buy] = 0;

    shop->profit_buy = 100;
    shop->profit_sell = 100;
    shop->open_hour = 0;
    shop->close_hour = 23;

    return shop;
}



void free_shop(SHOP_DATA *shop)
{
    shop->next = shop_free;
    shop_free = shop;
    return;
}



MOB_INDEX_DATA *new_mob_index(void)
{
    MOB_INDEX_DATA *mob;

    if (!mob_index_free) {
        mob = alloc_perm((unsigned int)sizeof(*mob));
        top_mob_index++;
    } else {
        mob = mob_index_free;
        mob_index_free = mob_index_free->next;
    }

    mob->next = NULL;
    mob->shop = NULL;
    mob->area = NULL;
    mob->player_name = str_dup("no name");
    mob->short_descr = str_dup("(no short description)");
    mob->long_descr = str_dup("(no long description)\n\r");
    mob->description = &str_empty[0];
    mob->vnum = 0;
    mob->count = 0;
    mob->killed = 0;
    mob->sex = 0;
    mob->level = 0;
    mob->act = ACT_IS_NPC;
    mob->affected_by = 0;
    mob->hitroll = 0;
    mob->race = race_lookup("human");
    mob->form = 0;
    mob->parts = 0;
    mob->imm_flags = 0;
    mob->res_flags = 0;
    mob->vuln_flags = 0;
    mob->material = str_dup("unknown");
    mob->off_flags = 0;
    mob->size = SIZE_MEDIUM;
    mob->ac[AC_PIERCE] = 0;
    mob->ac[AC_BASH] = 0;
    mob->ac[AC_SLASH] = 0;
    mob->ac[AC_EXOTIC] = 0;
    mob->hit[DICE_NUMBER] = 0;
    mob->hit[DICE_TYPE] = 0;
    mob->hit[DICE_BONUS] = 0;
    mob->mana[DICE_NUMBER] = 0;
    mob->mana[DICE_TYPE] = 0;
    mob->mana[DICE_BONUS] = 0;
    mob->damage[DICE_NUMBER] = 0;
    mob->damage[DICE_TYPE] = 0;
    mob->damage[DICE_NUMBER] = 0;
    mob->start_pos = POS_STANDING;
    mob->default_pos = POS_STANDING;
    mob->wealth = 0;

    mob->new_format = true;         /* ROM */

    return mob;
}



void free_mob_index(MOB_INDEX_DATA *mob)
{
    free_string(mob->player_name);
    free_string(mob->short_descr);
    free_string(mob->long_descr);
    free_string(mob->description);
    free_mprog(mob->mprogs);

    free_shop(mob->shop);

    mob->next = mob_index_free;
    mob_index_free = mob;
    return;
}

static MPROG_CODE *mpcode_free;

MPROG_CODE *new_mpcode(void)
{
    MPROG_CODE *mpcode;

    if (!mpcode_free) {
        mpcode = alloc_perm((unsigned int)sizeof(*mpcode));
        top_mprog_index++;
    } else {
        mpcode = mpcode_free;
        mpcode_free = mpcode_free->next;
    }

    mpcode->vnum = 0;
    mpcode->comment = str_dup("");
    mpcode->code = str_dup("");
    mpcode->next = NULL;

    return mpcode;
}

void free_mpcode(MPROG_CODE *mpcode)
{
    free_string(mpcode->comment);
    free_string(mpcode->code);

    mpcode->next = mpcode_free;
    mpcode_free = mpcode;
    return;
}
