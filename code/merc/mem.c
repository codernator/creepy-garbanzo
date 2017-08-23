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
struct extra_descr_data *extra_descr_free;
void free_extra_descr(struct extra_descr_data * extra);
void free_affect(struct affect_data * af);
void free_mprog(struct mprog_list * mp);


/** imports */
extern char str_empty[1];
extern int top_ed;
extern long top_mob_index;
extern int top_mprog_index;


/** locals */
static struct shop_data *shop_free;
static struct mob_index_data *mob_index_free;
extern struct affect_data *affect_free;


struct shop_data *new_shop(void)
{
    struct shop_data *shop;
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



void free_shop(struct shop_data *shop)
{
    shop->next = shop_free;
    shop_free = shop;
    return;
}



struct mob_index_data *new_mob_index(void)
{
    struct mob_index_data *mob;

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



void free_mob_index(struct mob_index_data *mob)
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

static struct mprog_code *mpcode_free;

struct mprog_code *new_mpcode(void)
{
    struct mprog_code *mpcode;

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

void free_mpcode(struct mprog_code *mpcode)
{
    free_string(mpcode->comment);
    free_string(mpcode->code);

    mpcode->next = mpcode_free;
    mpcode_free = mpcode;
    return;
}
