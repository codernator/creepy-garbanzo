#include "merc.h"
#include "object.h"
#include "magic.h"
#include "channels.h"
#include <stdio.h>
#include <string.h>


extern struct dynamic_skill *gsp_fast_healing;
extern struct dynamic_skill *gsp_meditation;
extern struct dynamic_skill *gsp_burning_flames;

extern DECLARE_DO_FUN(do_quit);
extern DECLARE_DO_FUN(do_echo);
extern DECLARE_DO_FUN(do_save);
extern DECLARE_DO_FUN(do_look);
extern DECLARE_DO_FUN(do_btrans);
extern DECLARE_DO_FUN(do_reboot);
extern DECLARE_DO_FUN(do_copyover);
extern bool mp_percent_trigger(struct char_data * mob, struct char_data * ch, const void *arg1, const void *arg2, int type);
extern void random_drop(void);
extern struct affect_data *new_affect(void);
extern void auction_update(void);


void gain_object_exp(struct char_data * ch, struct gameobject * obj, int gain);
void update_pktimer(struct char_data * ch);
char *who_string(struct char_data * ch);


/***************************************************************************
 *	auto save counter
 ***************************************************************************/
static int save_number = 0;

static void add_cache_affect(struct gameobject * obj, int bit, int sn, int level, int mod);
static void mobile_update(void);
static void weather_update(void);
static int hit_gain(struct char_data * ch);
static int mana_gain(struct char_data * ch);
static int move_gain(struct char_data * ch);
static void char_update(void);
static void advance_level_object(struct char_data * ch, struct gameobject * obj);
static void obj_update(void);
static void aggr_update(void);
static void room_update(void);
static void underwater_update(void);



void gain_object_exp(struct char_data *ch, struct gameobject *obj, int gain)
{
    int leftover = 0;

    if (IS_NPC(ch))
	return;

    if (obj->plevel >= 30) return;
    printf_to_char(ch, "%s has gained %d exp.\n\r", capitalize(OBJECT_SHORT(obj)), gain);
    obj->exp += gain;
    obj->xp_tolevel -= gain;

    if (obj->xp_tolevel <= 0) {
	obj->exp += obj->xp_tolevel;
	advance_level_object(ch, obj);
	leftover = (obj->xp_tolevel * 1);
	obj->plevel++;
	printf_to_char(ch, "%s has raised to level %d. To see your objects stats lore or identify it.\n\r", capitalize(OBJECT_SHORT(obj)), obj->plevel);
	obj->xp_tolevel = 1500 + (obj->plevel * 500);
	obj->xp_tolevel -= leftover;
	return;
    }
    return;
}

static void advance_level_object(struct char_data *ch, struct gameobject *obj)
{
    int pbonus = number_range(5, 10);
    int bonus = number_range(4, 8);

    pbonus = pbonus * 9 / 10;
    bonus = bonus * 8 / 10;

    pbonus = UMAX(6, pbonus);
    bonus = UMAX(1, bonus);

    add_cache_affect(obj, APPLY_DAMROLL, 0, obj->plevel * 5, number_range(1, 2));
    add_cache_affect(obj, APPLY_HITROLL, 0, obj->plevel * 5, number_range(1, 2));
    add_cache_affect(obj, APPLY_HIT, 0, obj->plevel * 5, number_range(1, 2));
    add_cache_affect(obj, APPLY_MANA, 0, obj->plevel * 5, number_range(1, 2));
    add_cache_affect(obj, APPLY_MOVE, 0, obj->plevel * 5, (number_range(10, 30)));

    if (obj->item_type == ITEM_WEAPON) {
	obj->value[1] += bonus / 4;
	obj->value[2] += bonus / 5;
    } else if (obj->item_type == ITEM_ARMOR) {
	obj->value[0] += UMAX(1, obj->plevel);
	obj->value[1] += UMAX(1, obj->plevel);
	obj->value[2] += UMAX(1, obj->plevel);
	obj->value[3] += (5 * UMAX(1, obj->plevel)) / 10;
    }

    return;
}

/***************************************************************************
 *	add_cache_affect
 *
 *	add a single affect for the style spell
 *	appends the affect to an existing affect if it already
 *	exists, otherwise it creates a new affect
 ***************************************************************************/
static void add_cache_affect(struct gameobject *obj, int bit, int sn, int level, int mod)
{
    struct affect_data *paf;
    bool found = false;

    for (paf = obj->affected; paf != NULL; paf = paf->next) {
	if (paf->location == bit) {
	    found = true;
	    paf->type = sn;
	    paf->modifier += mod;
	    paf->level = (int)UMAX(paf->level, (int)level);
	    if (paf->modifier > 4)
		SET_BIT(obj->extra_flags, ITEM_HUM);

	}
    }

    if (!found) {
	paf = new_affect();

	paf->where = TO_OBJECT;
	paf->type = sn;
	paf->level = (int)level;
	paf->duration = -1;
	paf->location = bit;
	paf->modifier = mod;
	paf->bitvector = 0;
	paf->next = obj->affected;
	obj->affected = paf;
    }
}


/***************************************************************************
 *	advance_level
 *
 *	advance (or demote) a character n levels
 ***************************************************************************/
void advance_level(struct char_data *ch, int level)
{
    int loop;
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;
    int add_train;
    int stat_con;
    int stat_int;
    int stat_wis;
    int stat_dex;
    int total_hp = 0;
    int total_mana = 0;
    int total_move = 0;
    int total_prac = 0;
    int total_train = 0;
    bool negative = false;

    if (IS_NPC(ch)) {
	send_to_char("Don't advance mobs. Use SET.\n\r", ch);
	return;
    }

    if (level < 0) {
	level *= -1;
	negative = true;
    }

    stat_int = get_curr_stat(ch, STAT_INT);
    stat_wis = get_curr_stat(ch, STAT_WIS);
    stat_dex = get_curr_stat(ch, STAT_DEX);
    stat_con = get_curr_stat(ch, STAT_CON);
    for (loop = 1; loop <= level; loop++) {
	/* generate a fuzzy hp gain */
	add_hp = number_range(class_table[ch->class].hp_min, (stat_con + 7) / 2);
	add_hp = UMIN(add_hp, class_table[ch->class].hp_max);
	add_hp = number_range(add_hp - 5, add_hp + 5);

	/* generate a fuzzy mana gain */
	add_mana = number_range(class_table[ch->class].mana_min, (stat_int + stat_wis + 13) / 4);
	add_mana = UMIN(add_mana, class_table[ch->class].mana_max);
	add_mana = number_range(add_mana - 5, add_mana + 5);


	add_move = number_range(1, ((stat_con + stat_dex) / 4));
	add_move = UMIN(add_move, 20);
	add_move = number_range(add_move - 5, add_move + 5);

	add_prac = URANGE(1, get_curr_stat(ch, STAT_WIS) / 5, 8);
	add_train = (get_curr_stat(ch, STAT_LUCK) >= 25) ? 3 : (get_curr_stat(ch, STAT_LUCK) >= 18) ? 2 : 1;

	add_hp = UMAX(4, add_hp);
	add_mana = UMAX(4, add_mana);
	add_move = UMAX(6, add_move);


	total_hp += add_hp;
	total_mana += add_mana;
	total_move += add_move;
	total_prac += add_prac;
	total_train += add_train;

	if (negative) {
	    ch->level -= 1;
	    ch->max_hit -= add_hp;
	    ch->max_mana -= add_mana;
	    ch->max_move -= add_move;

	    ch->pcdata->practice -= add_prac;
	    ch->pcdata->train -= add_train;
	    ch->pcdata->perm_hit -= add_hp;
	    ch->pcdata->perm_mana -= add_mana;
	    ch->pcdata->perm_move -= add_move;
	} else {
	    ch->level += 1;
	    ch->max_hit += add_hp;
	    ch->max_mana += add_mana;
	    ch->max_move += add_move;

	    ch->pcdata->practice += add_prac;
	    ch->pcdata->train += add_train;
	    ch->pcdata->perm_hit += add_hp;
	    ch->pcdata->perm_mana += add_mana;
	    ch->pcdata->perm_move += add_move;
	}
    }

    ch->pcdata->last_level = (ch->played + (int)(globalSystemState.current_time - ch->logon)) / 3600;

    printf_to_char(ch,
	    "Your %s is: %d/%d hp, %d/%d m, %d/%d mv %d/%d prac %d/%d trains.\n\r",
	    (negative) ? "loss" : "gain",
	    total_hp, ch->max_hit,
	    total_mana, ch->max_mana,
	    total_move, ch->max_move,
	    total_prac, ch->pcdata->practice,
	    total_train, ch->pcdata->train);

    return;
}

/***************************************************************************
 *	gain_exp
 *
 *	gain experience - advance level if attained
 ***************************************************************************/
void gain_exp(struct char_data *ch, int gain)
{
    static char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch) || ch->level >= (301 - 1)) {
	if (!IS_NPC(ch) && ch->level == 301 - 1)
	    ch->exp = exp_per_level(ch, ch->pcdata->points) * ch->level;
	if (!IS_NPC(ch)) ch->pcdata->extendedexp += (gain / 10);
	return;
    }

    /* max out the gain at exactly one level */
    if (gain > exp_per_level(ch, ch->pcdata->points))
	gain = exp_per_level(ch, ch->pcdata->points);
    ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);


    /* while the level < LEVEL_HERO and exp > exp_per_level - gain levels */
    while (ch->level < 300 && !IS_IMMORTAL(ch) && ch->exp >= exp_per_level(ch, ch->pcdata->points) * (ch->level + 1)) {
	send_to_char("You raise a level!!\n\r `1*`8-`7-`&-`8#`7#`&#`6<`@-`4G`$r`4a`$t`4s`@-`6>`&#`7#`8#`&-`7-`8-`1*``\n\r", ch);
	(void)snprintf(buf, MAX_INPUT_LENGTH, "$N has attained level %d!", ch->level + 1);
	wiznet(buf, ch, NULL, WIZ_LEVELS, 0, 0);

	advance_level(ch, 1);
	save_char_obj(ch);

	/* do the info */
	if (ch->level == 300) {
	    (void)snprintf(buf, MAX_INPUT_LENGTH, "%s has made it to 300!``\n\r", ch->name);
	    broadcast_channel(NULL, channels_find(CHANNEL_INFO), NULL, buf);
	    restore_char(ch);
	    send_to_char("`^You have been restored by the Immortal of Bad Trip for making it to level 300!``\n\r", ch);
	} else {
	    (void)snprintf(buf, MAX_INPUT_LENGTH, "%s has gained a level!``\n\r", ch->name);
	    broadcast_channel(NULL, channels_find(CHANNEL_INFO), NULL, buf);
	}
    }

    save_char_obj(ch);
}

/***************************************************************************
 *   extend                                                                 *
 *                                                                          *
 *   gain 1 extended level - immcommand                                     *
 ***************************************************************************/
void do_extend(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0') {
	send_to_char("`1Syntax: `!extend <`1char`!>``\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, arg1)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("You can only extend players.\n\r", ch);
	return;
    }

    if (victim->level < 300) {
	send_to_char("Player must be at 300 or over.\n\r", ch);
	return;
    }
    if (victim->level >= 600) {
	send_to_char("You can not use extend past level 600.\n\r", ch);
	return;
    }

    if (victim->pcdata->extendedexp < 100000) {
	send_to_char("Player does not have atleast 100,000 Extended Experience points.\n\r", ch);
	return;
    }

    victim->pcdata->extendedexp -= 100000;
    advance_level(victim, 1);
    victim->exp = exp_per_level(victim, victim->pcdata->points) * UMAX(1, victim->level);
    victim->trust = 0;
    save_char_obj(victim);
    send_to_char("100,000 Extended Experience points removed. Level granted.\n\r", ch);
    send_to_char("You have gained 1 extended level!\n\r", victim);
    return;
}

/***************************************************************************
 *	restore_char
 *
 *	restores a character
 ***************************************************************************/
void restore_char(struct char_data *ch)
{
    ch->hit = ch->max_hit;
    ch->mana = ch->max_mana;
    ch->move = ch->max_move;

    strip_negative_affects(ch);
    update_pos(ch);
}

/***************************************************************************
 *	strip_negative_affects
 *
 *	strip the negative affects from a character
 ***************************************************************************/
void strip_negative_affects(struct char_data *ch)
{
    struct dynamic_skill *skill_idx;

    for (skill_idx = skill_list; skill_idx != NULL; skill_idx = skill_idx->next)
	if (IS_SET(skill_idx->flags, SPELL_AFFSTRIP))
	    affect_strip(ch, skill_idx);

    reset_char(ch);
}


/***************************************************************************
 *	hit_gain
 *
 *	determine how many hp a character gets every tick
 ***************************************************************************/
static int hit_gain(struct char_data *ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL)
	return 0;

    if (IS_NPC(ch)) {
	gain = 5 + ch->level;
	switch (ch->position) {
	    default:
		gain /= 2;
		break;
	    case POS_SLEEPING:
		gain = 3 * gain / 2;
		break;
	    case POS_RESTING:
		break;
	    case POS_FIGHTING:
		gain /= 3;
		break;
	}
    } else {
	gain = UMAX(3, get_curr_stat(ch, STAT_CON) - 3 + ch->level / 2);
	gain += class_table[ch->class].hp_max - 10;
	number = number_percent();
	if (number < get_learned_percent(ch, gsp_fast_healing)) {
	    gain += number * gain / 100;
	    if (ch->hit < ch->max_hit)
		check_improve(ch, gsp_fast_healing, true, 8);
	}

	switch (ch->position) {
	    default:
		gain /= 4;
		break;
	    case POS_SLEEPING:
		break;
	    case POS_RESTING:
		gain /= 2;
		break;
	    case POS_FIGHTING:
		gain /= 6;
		break;
	}

	if (ch->pcdata->condition[COND_HUNGER] <= 0 && ch->pcdata->condition[COND_HUNGER] != -151)
	    gain /= 2;

	if (ch->pcdata->condition[COND_FEED] <= 0 && ch->pcdata->condition[COND_FEED] != -151)
	    gain /= 2;

	if (ch->pcdata->condition[COND_THIRST] <= 0 && ch->pcdata->condition[COND_THIRST] != -151)
	    gain /= 2;

    }

    gain = gain * ch->in_room->heal_rate / 100;
    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * (int)ch->on->value[3] / 100;

    if (IS_AFFECTED(ch, AFF_POISON))
	gain /= 4;

    if (IS_AFFECTED(ch, AFF_HASTE))
	gain /= 2;

    if (IS_AFFECTED(ch, AFF_SLOW))
	gain *= 2;

    return UMIN(gain, ch->max_hit - ch->hit);
}


/***************************************************************************
 *	mana_gain
 *
 *	determine the amount of a mana a character gains at each
 *	tick
 ***************************************************************************/
static int mana_gain(struct char_data *ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL)
	return 0;

    if (IS_NPC(ch)) {
	gain = 5 + ch->level;
	switch (ch->position) {
	    default:
		gain /= 2;
		break;
	    case POS_SLEEPING:
		gain = 3 * gain / 2;
		break;
	    case POS_RESTING:
		break;
	    case POS_FIGHTING:
		gain /= 3;
		break;
	}
    } else {
	gain = (get_curr_stat(ch, STAT_WIS)
		+ get_curr_stat(ch, STAT_INT) + ch->level) / 2;
	number = number_percent();
	if (number < get_learned_percent(ch, gsp_meditation)) {
	    gain += number * gain / 100;
	    if (ch->mana < ch->max_mana)
		check_improve(ch, gsp_meditation, true, 8);
	}
	if (!class_table[ch->class].fMana)
	    gain /= 2;

	switch (ch->position) {
	    default:
		gain /= 4;
		break;
	    case POS_SLEEPING:
		break;
	    case POS_RESTING:
		gain /= 2;
		break;
	    case POS_FIGHTING:
		gain /= 6;
		break;
	}

	if (ch->pcdata->condition[COND_HUNGER] <= 0 && ch->pcdata->condition[COND_HUNGER] != -151)
	    gain /= 2;

	if (ch->pcdata->condition[COND_FEED] <= 0 && ch->pcdata->condition[COND_FEED] != -151)
	    gain /= 2;

	if (ch->pcdata->condition[COND_THIRST] <= 0 && ch->pcdata->condition[COND_THIRST] != -151)
	    gain /= 2;
    }

    gain = gain * ch->in_room->mana_rate / 100;
    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * (int)ch->on->value[4] / 100;

    if (IS_AFFECTED(ch, AFF_POISON))
	gain /= 4;

    if (IS_AFFECTED(ch, AFF_HASTE))
	gain /= 2;

    if (IS_AFFECTED(ch, AFF_SLOW))
	gain *= 2;

    return UMIN(gain, ch->max_mana - ch->mana);
}


/***************************************************************************
 *	move_gain
 *
 *	determine the amount of movement a character gains at a tick
 ***************************************************************************/
static int move_gain(struct char_data *ch)
{
    int gain;

    if (ch->in_room == NULL)
	return 0;

    if (IS_NPC(ch)) {
	gain = ch->level;
    } else {
	gain = UMAX(15, ch->level);

	switch (ch->position) {
	    case POS_SLEEPING:
		gain += get_curr_stat(ch, STAT_DEX);
		break;
	    case POS_RESTING:
		gain += get_curr_stat(ch, STAT_DEX) / 2;
		break;
	}

	if (ch->pcdata->condition[COND_THIRST] <= 0 && ch->pcdata->condition[COND_THIRST] != -151)
	    gain /= 2;
    }

    gain = gain * ch->in_room->heal_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * (int)ch->on->value[3] / 100;

    if (IS_AFFECTED(ch, AFF_POISON))
	gain /= 4;

    if (IS_AFFECTED(ch, AFF_HASTE))
	gain /= 2;

    if (IS_AFFECTED(ch, AFF_SLOW))
	gain *= 2;

    return UMIN(gain, ch->max_move - ch->move);
}


/***************************************************************************
 *	gain_condition
 *
 *	does condition handling such as thirst, hunger, etc.
 ***************************************************************************/
void gain_condition(struct char_data *ch, int condition_idx, long value)
{
    long condition;

    if (value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL || condition_idx > COND_MAX)
	return;

    condition = ch->pcdata->condition[condition_idx];
    if (condition == -151)
	return;

    ch->pcdata->condition[condition_idx] = URANGE(-150l, condition + value, 48l);
    if (ch->pcdata->condition[condition_idx] <= 0) {
	switch (condition_idx) {
	    case COND_THIRST:
		send_to_char("`6D`^r`6i`^n`6k `^M`6e``.\n\r", ch);
		if (condition <= -70)
		    send_to_char("Perhaps you had better get something to drink...\n\r", ch);
		break;

	    case COND_HUNGER:
		send_to_char("`#E`3a`#t `3M`#e``.\n\r", ch);
		if (condition <= -70)
		    send_to_char("Perhaps you had better eat...\n\r", ch);
		break;
	}
    }

    return;
}


/***************************************************************************
 *	mobile_update
 *
 *	update the state of all mobiles....makes the mobiles appear
 *	to be alive kinda sorta maybe
 ***************************************************************************/
static void mobile_update(void)
{
    struct char_data *ch;
    struct char_data *ch_next;
    struct exit_data *pexit;
    int door;

    /* Examine all mobs. */
    for (ch = char_list; ch != NULL; ch = ch_next) {
	if (ch != NULL)
	    ch_next = ch->next;

	if (!IS_NPC(ch)
		|| ch->in_room == NULL
		|| IS_AFFECTED(ch, AFF_CHARM))
	    continue;

	if (ch->in_room->area->empty && !IS_SET(ch->act, ACT_UPDATE_ALWAYS))
	    continue;

	if (IS_SHOPKEEPER(ch)) {
	    if ((ch->gold * 100 + ch->silver) < ch->mob_idx->wealth) {
		ch->gold += ch->mob_idx->wealth * number_range(1, 20) / 5000000;
		ch->silver += ch->mob_idx->wealth * number_range(1, 20) / 50000;
	    }
	}

	/*
	 * Check triggers only if mobile still in default position
	 */
	if (ch->position == ch->mob_idx->default_pos) {
	    /* Delay */
	    if (HAS_TRIGGER(ch, TRIG_DELAY)
		    && ch->mprog_delay > 0) {
		if (--ch->mprog_delay <= 0) {
		    mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_DELAY);
		    continue;
		}
	    }

	    if (HAS_TRIGGER(ch, TRIG_RANDOM)) {
		if (mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_RANDOM))
		    continue;
	    }
	}



	/*
	 * Check triggers only if mobile still in default position
	 */
	if (ch->position == ch->mob_idx->default_pos) {
	    /* Delay */
	    if (HAS_TRIGGER(ch, TRIG_DELAY)
		    && ch->mprog_delay > 0) {
		if (--ch->mprog_delay <= 0) {
		    mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_DELAY);
		    continue;
		}
	    }
	    if (HAS_TRIGGER(ch, TRIG_RANDOM)) {
		if (mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_RANDOM))
		    continue;
	    }
	}


	/* That's all for sleeping / busy monster, and empty zones */
	if (ch->position != POS_STANDING)
	    continue;

	/* Scavenge */
	if (IS_SET(ch->act, ACT_SCAVENGER)
		&& ch->in_room->contents != NULL
		&& number_bits(6) == 0) {
	    struct gameobject *obj;
	    struct gameobject *obj_best;
	    unsigned int max;

	    max = 1;
	    obj_best = 0;
	    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
		if (CAN_WEAR(obj, ITEM_TAKE)
			&& can_loot(ch, obj)
			&& obj->cost > max
			&& obj->cost > 0
			&& count_users(obj) == 0) {
		    obj_best = obj;
		    max = obj->cost;
		}
	    }

	    if (obj_best) {
		obj_from_room(obj_best);
		obj_to_char(obj_best, ch);
		act("$n gets $p.", ch, obj_best, NULL, TO_ROOM);
	    }
	}

	/*Mob Memory by Urgo 7/28/96 */
	if (ch->mobmem != NULL && ch->in_room->people != NULL) {
	    struct char_data *pissed;
	    struct char_data *pissed_next;

	    for (pissed = ch->in_room->people; pissed; pissed = pissed_next) {
		pissed_next = pissed->next_in_room;

		if (ch->mobmem == pissed) {
		    act("$n glares at $N and says, '`!Came back for more, eh?!``'", ch, NULL, ch->mobmem, TO_NOTVICT);
		    act("$n glares at you and says, '`!Came back for more, eh?!``'", ch, NULL, ch->mobmem, TO_VICT);
		    act("You glare at $N and say, '`!Came back for more, eh?!``'", ch, NULL, ch->mobmem, TO_CHAR);

		    multi_hit(ch, pissed, TYPE_UNDEFINED);
		    ch->mob_wuss = NULL;
		    ch->mobmem = NULL;
		    continue;
		}
	    }
	}

	if (ch->mob_wuss != NULL && ch->in_room->people != NULL) {
	    struct room_index_data *was_in;
	    struct room_index_data *now_in;
	    struct char_data *laters;
	    struct char_data *laters_next;
	    int attempt;


	    for (laters = ch->in_room->people; laters; laters = laters_next) {
		laters_next = laters->next_in_room;
		if (ch->mob_wuss == laters) {
		    act("$n screams '`1Get away from me, $N!!``'",
			    ch, NULL, ch->mob_wuss, TO_ROOM);
		    act("You run from $n, screaming!",
			    NULL, NULL, ch->mob_wuss, TO_CHAR);
		    was_in = ch->in_room;
		    for (attempt = 0; attempt < 6; attempt++) {
			struct exit_data *pexit;
			int door;

			door = number_door();
			if ((pexit = was_in->exit[door]) == 0
				|| pexit->u1.to_room == NULL
				|| IS_SET(pexit->exit_info, EX_CLOSED)
				|| (IS_NPC(ch)
				    && IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)))
			    continue;

			move_char(ch, door, false);
			if ((now_in = ch->in_room) == was_in)
			    continue;

			ch->in_room = was_in;
			act("$n has fled!", ch, NULL, NULL, TO_ROOM);
			ch->in_room = now_in;
			break;
		    }
		}
	    }
	}


	/* Wander */
	if (!IS_SET(ch->act, ACT_SENTINEL)
		&& number_bits(3) == 0
		&& (door = number_bits(5)) <= 5
		&& (pexit = ch->in_room->exit[door]) != NULL
		&& pexit->u1.to_room != NULL
		&& !IS_SET(pexit->exit_info, EX_CLOSED)
		&& !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
		&& (!IS_SET(ch->act, ACT_STAY_AREA)
		    || pexit->u1.to_room->area == ch->in_room->area)
		&& (!IS_SET(ch->act, ACT_OUTDOORS)
		    || !IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS))
		&& (!IS_SET(ch->act, ACT_INDOORS)
		    || IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS)))
	    move_char(ch, door, false);
    }

    return;
}


/***************************************************************************
 *	weather_update
 *
 *	displays the status of the weather
 ***************************************************************************/
static void weather_update(void)
{
    struct descriptor_iterator_filter filter = { .must_playing = true };
    struct descriptor_data *d;
    char buf[MAX_STRING_LENGTH];
    int diff;

    buf[0] = '\0';

    switch (++globalGameState.gametime->hour) {
	case 5:
	    globalGameState.weather->sunlight = SUN_LIGHT;
	    strcat(buf, "A new day has begun.\n\r");
	    break;

	case 6:
	    globalGameState.weather->sunlight = SUN_RISE;
	    strcat(buf, "The `#sun`` rises in the east.\n\r");
	    break;

	case 19:
	    globalGameState.weather->sunlight = SUN_SET;
	    strcat(buf, "The `#sun`` slowly disappears in the west.\n\r");
	    break;

	case 20:
	    globalGameState.weather->sunlight = SUN_DARK;
	    strcat(buf, "The `8night`` has begun.\n\r");
	    break;

	case 24:
	    globalGameState.gametime->hour = 0;
	    globalGameState.gametime->day++;
	    break;
    }

    if (globalGameState.gametime->day >= 35) {
	globalGameState.gametime->day = 0;
	globalGameState.gametime->month++;
    }

    if (globalGameState.gametime->month >= 17) {
	globalGameState.gametime->month = 0;
	globalGameState.gametime->year++;
    }

    /*
     * Weather change.
     */
    if (globalGameState.gametime->month >= 9 && globalGameState.gametime->month <= 16)
	diff = globalGameState.weather->mmhg > 985 ? -2 : 2;
    else
	diff = globalGameState.weather->mmhg > 1015 ? -2 : 2;

    globalGameState.weather->change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    globalGameState.weather->change = UMAX(globalGameState.weather->change, -12);
    globalGameState.weather->change = UMIN(globalGameState.weather->change, 12);

    globalGameState.weather->mmhg += globalGameState.weather->change;
    globalGameState.weather->mmhg = UMAX(globalGameState.weather->mmhg, 960);
    globalGameState.weather->mmhg = UMIN(globalGameState.weather->mmhg, 1040);

    switch (globalGameState.weather->sky) {
	default:
	    log_bug("Weather_update: bad sky %d.", globalGameState.weather->sky);
	    globalGameState.weather->sky = SKY_CLOUDLESS;
	    break;

	case SKY_CLOUDLESS:
	    if (globalGameState.weather->mmhg < 990
		    || (globalGameState.weather->mmhg < 1010 && number_bits(2) == 0)) {
		strcat(buf, "The sky is getting cloudy.\n\r");
		globalGameState.weather->sky = SKY_CLOUDY;
	    }
	    break;

	case SKY_CLOUDY:
	    if (globalGameState.weather->mmhg < 970
		    || (globalGameState.weather->mmhg < 990 && number_bits(2) == 0)) {
		strcat(buf, "It starts to rain.\n\r");
		globalGameState.weather->sky = SKY_RAINING;
	    }

	    if (globalGameState.weather->mmhg > 1030 && number_bits(2) == 0) {
		strcat(buf, "The clouds disappear.\n\r");
		globalGameState.weather->sky = SKY_CLOUDLESS;
	    }
	    break;

	case SKY_RAINING:
	    if (globalGameState.weather->mmhg < 970 && number_bits(2) == 0) {
		strcat(buf, "Lightning flashes in the sky.\n\r");
		globalGameState.weather->sky = SKY_LIGHTNING;
	    }

	    if (globalGameState.weather->mmhg > 1030
		    || (globalGameState.weather->mmhg > 1010 && number_bits(2) == 0)) {
		strcat(buf, "The rain stopped.\n\r");
		globalGameState.weather->sky = SKY_CLOUDY;
	    }
	    break;

	case SKY_LIGHTNING:
	    if (globalGameState.weather->mmhg > 1010 || (globalGameState.weather->mmhg > 990 && number_bits(2) == 0)) {
		strcat(buf, "The lightning has stopped.\n\r");
		globalGameState.weather->sky = SKY_RAINING;
	    }
	    break;
    }

    if (buf[0] != '\0') {
	struct descriptor_data *dpending;

	dpending = descriptor_iterator_start(&filter);
	while ((d = dpending) != NULL) {
	    dpending = descriptor_iterator(d, &filter);

	    if (IS_OUTSIDE(d->character) && IS_AWAKE(d->character))
		send_to_char(buf, d->character);
	}
    }

    return;
}

/**
 * automatic restores set at the interval PULSE_RESTORE in merc.h
 */
static void auto_restore(void)
{
    struct descriptor_iterator_filter filter = { .must_playing = true };
    struct descriptor_data *d;
    struct descriptor_data *dpending;
    struct char_data *victim;

    dpending = descriptor_iterator_start(&filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &filter);

	victim = d->character;
	if (victim != NULL && !IS_NPC(victim))
	{
	    restore_char(victim);
	    save_char_obj(victim);
	    if (victim->in_room != NULL)
		send_to_char("`1A `!huge`` beam of `&light`` crosses the sky above you ..\n\r", victim);
	}
    }
}



/***************************************************************************
 *	room_update
 *
 *	update all of the rooms on the mud
 ***************************************************************************/
static void room_update(void)
{
    struct room_index_data *room;
    int index;
    static int decr;

    /* i want this function to be called twice each tick
     * but i want the timers on the affects to decrease
     * at a normal rate */
    if (++decr == 2)
	decr = 0;

    for (index = 0; index < MAX_KEY_HASH; index++) {
	for (room = room_index_hash[index]; room != NULL; room = room->next) {
	    if (room->affected != NULL) {
		struct affect_data *paf;
		struct affect_data *paf_next;
		struct dynamic_skill *skill;

		for (paf = room->affected; paf != NULL; paf = paf_next) {
		    paf_next = paf->next;

		    if ((skill = resolve_skill_affect(paf)) != NULL) {
			if (skill->affects != NULL) {
			    struct affect_list *affects;
			    for (affects = skill->affects; affects != NULL; affects = affects->next)
				if (affects->affect_fn != NULL)
				    (*affects->affect_fn)(skill, (void *)room, AFFECT_TYPE_ROOM, paf);
			}

			if (decr == 0) {
			    if (paf->duration < 0)
				continue;

			    if (--paf->duration == 0) {
				if (paf->type > 0
					&& skill->msg
					&& skill->msg[0] != '\0'
					&& room->people != NULL) {
				    act(skill->msg, room->people, NULL, NULL, TO_ROOM);
				    act(skill->msg, room->people, NULL, NULL, TO_CHAR);
				}
				affect_remove_room(room, paf);
			    }
			}
		    }
		}
	    }
	}
    }
}

/***************************************************************************
 *	char_update
 *
 *	update all of the rooms on the mud
 *	i really hate these big, long, ugly functions -- they end up
 *	getting very convoluted and not working the way you would
 *	expect -- like this one
 ***************************************************************************/
static void char_update(void)
{
    struct char_data *ch;
    struct char_data *ch_next;

    save_number++;
    if (save_number > 29)
	save_number = 0;

    for (ch = char_list; ch != NULL; ch = ch_next) {
	struct affect_data *paf;
	struct affect_data *paf_next;

	ch_next = ch->next;

	/* Timers on kill+thief flags */
	if (!IS_NPC(ch)) {
	    if (IS_SET(ch->act, PLR_KILLER)) {
		if ((ch->pcdata->killer_time == 0) ||
			(difftime(globalSystemState.current_time, ch->pcdata->killer_time) > 479)) {
		    send_to_char("You feel washed of your sins.\n\r", ch);
		    REMOVE_BIT(ch->act, PLR_KILLER);
		    ch->pcdata->killer_time = 0;
		}
	    }

	    if (IS_SET(ch->act, PLR_THIEF)) {
		if ((ch->pcdata->thief_time == 0) ||
			(difftime(globalSystemState.current_time, ch->pcdata->thief_time) > 479)) {
		    send_to_char("You feel washed of your sins.\n\r", ch);
		    REMOVE_BIT(ch->act, PLR_THIEF);
		    ch->pcdata->thief_time = 0;
		}
	    }
	}

	if (ch->position >= POS_STUNNED) {
	    /* check to see if we need to go home */
	    if (IS_NPC(ch)
		    && ch->zone != NULL
		    && ch->zone != ch->in_room->area
		    && ch->desc == NULL
		    && ch->fighting == NULL
		    && !IS_AFFECTED(ch, AFF_CHARM)
		    && !IS_SET(ch->act, ACT_UPDATE_ALWAYS)
		    && number_percent() < 10) {
		act("$n wanders on home.", ch, NULL, NULL, TO_ROOM);
		extract_char(ch, true);
		continue;
	    }

	    if (ch->hit < ch->max_hit)
		ch->hit += hit_gain(ch);
	    else
		ch->hit = ch->max_hit;

	    if (ch->mana < ch->max_mana)
		ch->mana += mana_gain(ch);
	    else
		ch->mana = ch->max_mana;

	    if (ch->move < ch->max_move)
		ch->move += move_gain(ch);
	    else
		ch->move = ch->max_move;
	}

	if (ch->position == POS_STUNNED)
	    update_pos(ch);

	if (!IS_NPC(ch) && ch->level < LEVEL_IMMORTAL) {
	    struct gameobject *obj;

	    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
		    && obj->item_type == ITEM_LIGHT
		    && obj->value[2] > 0) {
		if (--obj->value[2] == 0 && ch->in_room != NULL) {
		    --ch->in_room->light;
		    act("$p goes out.", ch, obj, NULL, TO_ROOM);
		    act("$p flickers and goes out.", ch, obj, NULL, TO_CHAR);
		    extract_obj(obj);
		} else if (obj->value[2] <= 5 && ch->in_room != NULL) {
		    act("$p flickers.", ch, obj, NULL, TO_CHAR);
		}
	    }

	    if (++ch->timer >= 6)
		SET_BIT(ch->comm, COMM_AFK);

	    if (ch->timer >= 15) {
		if (ch->was_in_room == NULL && ch->in_room != NULL) {
		    if (IS_IMMORTAL(ch)) {
			ch->timer = 0;
			return;
		    }

		    ch->was_in_room = ch->in_room;
		    if (ch->fighting != NULL)
			stop_fighting(ch, true);

		    furniture_check(ch);
		    act("$n disintegrates into a pile of `1molten `8ash``.", ch, NULL, NULL, TO_ROOM);
		    send_to_char("`1F`!i`#e`!r`1y flames`` engulf you.\n\r", ch);
		    if (ch->level > 1)
			save_char_obj(ch);

		    char_from_room(ch);
		    char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
		}
	    }

	    gain_condition(ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2);

	    if (ch->pcdata->condition[COND_THIRST] >= 0)
		gain_condition(ch, COND_THIRST, -1);
	    else
		gain_condition(ch, COND_THIRST, -25);

	    if (ch->pcdata->condition[COND_HUNGER] >= 0)
		gain_condition(ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);
	    else
		gain_condition(ch, COND_HUNGER, -25);

	    if (ch->pcdata->condition[COND_FEED] >= 0)
		gain_condition(ch, COND_FEED, -2);
	    else
		gain_condition(ch, COND_FEED, -25);
	}

	for (paf = ch->affected; paf != NULL; paf = paf_next) {
	    struct dynamic_skill *skill;

	    skill = NULL;
	    paf_next = paf->next;

	    /*
	     * do an affect to a character
	     * but make sure the character is still valid before
	     * doing the affect
	     */
	    if (ch == NULL)
		break;

	    if ((skill = resolve_skill_affect(paf)) != NULL) {
		if (skill->affects != NULL) {
		    struct affect_list *affects;

		    for (affects = skill->affects; affects != NULL; affects = affects->next)
			if (affects->affect_fn != NULL)
			    (*affects->affect_fn)(skill, (void *)ch, AFFECT_TYPE_CHAR, paf);
		}

		if (paf->duration > 0) {
		    paf->duration--;
		    /* fade the spell level */
		    if (number_range(0, 4) == 0 && paf->level > 0)
			paf->level--;
		} else if (paf->duration == 0) {
		    if (paf_next == NULL
			    || paf_next->type != paf->type
			    || paf_next->duration > 0) {
			if (paf->type > 0
				&& skill->msg
				&& skill->msg[0] != '\0') {
			    send_to_char(skill->msg, ch);
			    send_to_char("\n\r", ch);
			}
		    }

		    affect_remove(ch, paf);
		}
	    }
	}

	if (is_affected(ch, gsp_burning_flames) && ch != NULL) {
	    act("$n `8screams`` in agony as `!flames`` sear their flesh!", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You `8scream`` in agony as `!flames`` engulf you!\n\r", ch);
	    damage(ch, ch, number_range(50, 350), get_skill_number("burning flames"), DAM_FIRE, false);
	}

	if (ch->position == POS_INCAP && number_range(0, 1) == 0)
	    damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, false);
	else if (ch->position == POS_MORTAL)
	    damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, false);
    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for (ch = char_list; ch != NULL; ch = ch_next) {
	ch_next = ch->next;
	if ((ch->desc != NULL && (int)ch->desc->descriptor % 30 == save_number))
	    save_char_obj(ch);

	/*		if(!IS_NPC(ch) && ch->timer > 30)
	 *              {
	 *                      do_quit(ch, "");
	 *              }
	 */
	if ((!IS_NPC(ch)) && (IS_SET(ch->act, PLR_LINKDEAD)) && (!IS_IMMORTAL(ch))) {
	    impnet("`OAutomation`7: Killing [`8LINKDEAD`7] player $N", ch, NULL, IMN_AUTO, 0, 0);
	    if (IS_SET(ch->comm, COMM_AFK))
		REMOVE_BIT(ch->comm, COMM_AFK);
	    do_quit(ch, "");
	    return;
	}

	if (IS_SET(ch->comm, COMM_TICKS))
	    send_to_char("`7-- `#TICK!`7\n\r", ch);
    }
    return;
}


/***************************************************************************
 *	obj_update
 *
 *	update all of the objects in the world
 ***************************************************************************/
static void obj_update(void)
{
    struct gameobject *obj, *opending;
    struct affect_data *paf;
    struct affect_data *paf_next;

    opending = object_iterator_start(&object_empty_filter);
    while ((obj = opending) != NULL) {
	struct char_data *rch;
	char *message;

	opending = object_iterator(obj, &object_empty_filter);

	/* go through affects and decrement */
	for (paf = obj->affected; paf != NULL; paf = paf_next) {
	    paf_next = paf->next;
	    if (paf->duration > 0) {
		paf->duration--;
		if (number_range(0, 4) == 0
			&& paf->level > 0)
		    paf->level--;   /* spell strength fades with time */
	    } else if (paf->duration < 0) {
	    } else {
		struct dynamic_skill *skill;
		if (paf_next == NULL
			|| paf_next->type != paf->type
			|| paf_next->duration > 0) {
		    if (paf->type > 0
			    && (skill = resolve_skill_affect(paf)) != NULL
			    && skill->msg_obj != NULL
			    && skill->msg_obj[0] != '\0') {
			if (obj->carried_by != NULL) {
			    rch = obj->carried_by;
			    act(skill->msg_obj, rch, obj, NULL, TO_CHAR);
			}
			if (obj->in_room != NULL
				&& obj->in_room->people != NULL) {
			    rch = obj->in_room->people;
			    act(skill->msg_obj, rch, obj, NULL, TO_ALL);
			}
		    }
		}

		affect_remove_obj(obj, paf);
	    }
	}


	if (obj->timer <= 0 || --obj->timer > 0)
	    continue;

	switch (obj->item_type) {
	    case ITEM_FOUNTAIN:
		message = "$p dries up.";
		break;
	    case ITEM_CORPSE_NPC:
		message = "$p decays into dust.";
		break;
	    case ITEM_FOOD:
		message = "$p decomposes.";
		break;
	    case ITEM_POTION:
		message = "$p has evaporated from disuse.";
		break;
	    case ITEM_PORTAL:
		message = "$p winks out of existence.";
		break;
	    case ITEM_CONTAINER:
		if (CAN_WEAR(obj, ITEM_WEAR_FLOAT)) {
		    if (obj->contains)
			message = "$p flickers and vanishes, spilling its contents on the floor.";
		    else
			message = "$p flickers and vanishes.";
		} else {
		    message = "$p crumbles into dust.";
		}
		break;
	    default:
		message = "";
		break;
	}

	if (obj->carried_by != NULL) {
	    if (IS_SHOPKEEPER(obj->carried_by)) {
		obj->carried_by->silver += obj->cost / 5;
	    } else {
		act(message, obj->carried_by, obj, NULL, TO_CHAR);

		if (obj->wear_loc == WEAR_FLOAT)
		    act(message, obj->carried_by, obj, NULL, TO_ROOM);
	    }
	} else if (obj->in_room != NULL
		&& (rch = obj->in_room->people) != NULL) {
	    if (!(obj->in_obj && obj->in_obj->objprototype->vnum == OBJ_VNUM_PIT
			&& !CAN_WEAR(obj->in_obj, ITEM_TAKE))) {
		act(message, rch, obj, NULL, TO_ROOM);
		act(message, rch, obj, NULL, TO_CHAR);
	    }
	}

	if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT) && obj->contains != NULL) {
	    struct gameobject *t_obj, *next_obj;

	    for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj) {
		next_obj = t_obj->next_content;
		obj_from_obj(t_obj);

		if (obj->in_obj) { /* in another object */
		    obj_to_obj(t_obj, obj->in_obj);
		} else if (obj->carried_by) {   /* carried */
		    if (obj->wear_loc == WEAR_FLOAT) {
			if (obj->carried_by->in_room == NULL)
			    extract_obj(t_obj);
			else
			    obj_to_room(t_obj, obj->carried_by->in_room);
		    } else {
			obj_to_char(t_obj, obj->carried_by);
		    }
		} else if (obj->in_room == NULL) { /* destroy it */
		    extract_obj(t_obj);
		} else { /* to a room */
		    obj_to_room(t_obj, obj->in_room);
		}
	    }
	}
	extract_obj(obj);
    }
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
static void aggr_update(void)
{
    struct char_data *wch;
    struct char_data *wch_next;
    struct char_data *ch;
    struct char_data *ch_next;
    struct char_data *vch;
    struct char_data *vch_next;
    struct char_data *victim;

    for (wch = char_list; wch != NULL; wch = wch_next) {
	wch_next = wch->next;
	if (IS_NPC(wch)
		|| wch->level >= LEVEL_IMMORTAL
		|| wch->in_room == NULL
		|| wch->in_room->area->empty)
	    continue;

	for (ch = wch->in_room->people; ch != NULL; ch = ch_next) {
	    int count;

	    ch_next = ch->next_in_room;
	    if (!IS_NPC(ch)
		    || !IS_SET(ch->act, ACT_AGGRESSIVE)
		    || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
		    || IS_AFFECTED(ch, AFF_CALM)
		    || ch->fighting != NULL
		    || IS_AFFECTED(ch, AFF_CHARM)
		    || !IS_AWAKE(ch)
		    || (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch))
		    || !can_see(ch, wch)
		    || number_bits(1) == 0)
		continue;

	    /*
	     * Ok we have a 'wch' player character and a 'ch' npc aggressor.
	     * Now make the aggressor fight a RANDOM pc victim in the room,
	     *   giving each 'vch' an equal chance of selection.
	     */
	    count = 0;
	    victim = NULL;
	    for (vch = wch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (!IS_NPC(vch)
			&& vch->level < LEVEL_IMMORTAL
			&& ch->level >= vch->level - 5
			&& (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch))
			&& can_see(ch, vch)) {
		    if (number_range(0, count) == 0)
			victim = vch;
		    count++;
		}
	    }

	    if (victim == NULL)
		continue;

	    multi_hit(ch, victim, TYPE_UNDEFINED);
	}
    }

    return;
}


/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
/***************************************************************************
 *	update_handler
 *
 *	possibly one of the most important functions in the game --
 *	it is the nexus for updating anything automattically (as time passes)
 ***************************************************************************/
void update_handler(void)
{
    static int pulse_area;
    static int pulse_mobile;
    static int pulse_violence;
    static int pulse_point;
    static int pulse_rooms;
    static int pulse_restore;
    static int pulse_underwater;

    if (--pulse_rooms <= 0 || globalSystemState.tickset) {
	pulse_rooms = PULSE_ROOM;
	room_update();
	impnet("`!Update`7: Rooms have been updated.", NULL, NULL, IMN_UPDATES, 0, 0);
    }

    if (--pulse_restore <= 0) {
	pulse_restore = PULSE_RESTORE;
	auto_restore();
	impnet("`OAutomation`7: All players `Orestored`7..", NULL, NULL, IMN_AUTO, 0, 0);
    }

    if (--pulse_area <= 0) {
	pulse_area = PULSE_AREA;
	area_update();
	impnet("`!Update`7: Areas have been updated.", NULL, NULL, IMN_UPDATES, 0, 0);
    }

    if (--pulse_mobile <= 0) {
	pulse_mobile = PULSE_MOBILE;
	mobile_update();
	impnet("`!Update`7: Mobiles have been updated.", NULL, NULL, IMN_UPDATES, 0, 0);
    }

    if (--pulse_violence <= 0) {
	pulse_violence = PULSE_VIOLENCE;
	violence_update();
	impnet("`!Update`7: Violence has been updated.", NULL, NULL, IMN_UPDATES, 0, 0);
    }

    if (--pulse_underwater <= 0) {
	pulse_underwater = PULSE_UNDERWATER;
	underwater_update( );
    }

    if (globalSystemState.tickset)
	pulse_point = 0;

    if (--pulse_point <= 0) {
	char buf[100];

	wiznet("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
	impnet("`2TICK!`7", NULL, NULL, IMN_TICKS, 0, 0);
	pulse_point = PULSE_TICK;
	globalSystemState.tickset = false;
	if (globalSystemState.reboot_tick_counter > 0) {
	    globalSystemState.reboot_tick_counter--;
	    sprintf(buf, " `6R`6`^e`&b`^o`6ot`` in %d tick%s.", globalSystemState.reboot_tick_counter, globalSystemState.reboot_tick_counter == 1 ? "" : "s");
	    do_echo(NULL, buf);
	}

	if (globalSystemState.copyover_tick_counter > 0) {
	    globalSystemState.copyover_tick_counter--;
	    sprintf(buf, " `6C`6`^op`&yo`^ve`6r`` in %d tick%s.", globalSystemState.copyover_tick_counter, globalSystemState.copyover_tick_counter == 1 ? "" : "s");
	    do_echo(NULL, buf);
	}

	weather_update();
	char_update();
	obj_update();
	impnet("`!Update`7: Characters/Object/Weather/WhoOut have been updated.", NULL, NULL, IMN_UPDATES, 0, 0);

	if (!globalSystemState.reboot_tick_counter) {
	    do_reboot(NULL, "");
	}

	if (!globalSystemState.copyover_tick_counter) {
	    do_copyover(NULL, "");
	}
    }

    auction_update();
    aggr_update();
    tail_chain();
    return;
}


static void underwater_update(void)
{
    struct char_data *ch;
    struct char_data *ch_next;
    int dam;

    for (ch = char_list; ch != NULL; ch = ch_next) {
	ch_next = ch->next;

	if (!IS_NPC(ch)
		&& !IS_IMMORTAL(ch)
		&& (ch->in_room->sector_type == SECT_UNDERWATER)) {
	    if (IS_SET(ch->imm_flags, IMM_DROWNING))
		dam = 0;
	    else if (IS_SET(ch->res_flags, RES_DROWNING))
		dam = ch->hit / 4;
	    else if (IS_SET(ch->vuln_flags, VULN_DROWNING))
		dam = (ch->hit / 3) * 2;
	    else
		dam = ch->hit / 2;

	    if ((ch->hit > 200) && (dam > 0)) {
		ch->hit -= dam;
		send_to_char("You are `Od`6r`^o`Own`^i`6n`Og``!!!\n\r", ch);
		act("$n's `Plungs`` fill with `^w`6a`^t`6e`^r`` as $e slowly drowns.",
			ch, NULL, NULL, TO_ROOM);
		ch->position = POS_RESTING;
	    } else if (dam > 0) {
		ch->hit = 1;
		raw_kill(ch, NULL);
		send_to_char("You have `Od`6r`^o`Ow`^n`6e`Od``!!!\n\r", ch);
	    }
	}
    }
}
