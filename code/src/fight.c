#include <stdio.h>
#include "merc.h"
#include "character.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"
#include "combat_roll.h"
#include "channels.h"


extern SKILL *gsp_web;
extern SKILL *gsp_banzai;
extern SKILL *gsp_second_attack;
extern SKILL *gsp_aggressive_parry;
extern SKILL *gsp_third_attack;
extern SKILL *gsp_fourth_attack;
extern SKILL *gsp_fifth_attack;
extern SKILL *gsp_enhanced_damage;
extern SKILL *gsp_aggressive_parry;
extern SKILL *gsp_flanking;
extern SKILL *gsp_poison;
extern SKILL *gsp_invisibility;
extern SKILL *gsp_mass_invisibility;
extern SKILL *gsp_darkness;
extern SKILL *gsp_haven;
extern SKILL *gsp_parry;
extern SKILL *gsp_evade;
extern SKILL *gsp_shield_block;
extern SKILL *gsp_dodge;
extern SKILL *gsp_sleep;


extern bool mp_percent_trigger(CHAR_DATA * mob, CHAR_DATA * ch, const void *arg1, const void *arg2, int type);
extern void mp_hprct_trigger(CHAR_DATA * mob, CHAR_DATA * ch);
extern int battlefield_count(void);
extern void battlefield_notify(char *buf);
extern int battlefield_participants(void);
extern bool in_battlefield(CHAR_DATA * ch);

#define MAX_DAMAGE_MESSAGE 58

bool is_safe(CHAR_DATA * ch, CHAR_DATA * victim);
bool one_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt, GAMEOBJECT * wield);
void set_fighting(CHAR_DATA * ch, CHAR_DATA * victim);
bool check_dispel(int dis_level, CHAR_DATA * victim, SKILL * skill);
void dam_message(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, bool immune);
void make_corpse(CHAR_DATA * ch);
bool check_shield_block(CHAR_DATA * ch, CHAR_DATA * victim);
void disarm(CHAR_DATA * ch, CHAR_DATA * victim);


/*
 * Local functions.
 */
static void check_assist(CHAR_DATA * ch, CHAR_DATA * victim);
static bool check_dodge(CHAR_DATA * ch, CHAR_DATA * victim);
static bool check_parry(CHAR_DATA * ch, CHAR_DATA * victim);
static bool check_evade(CHAR_DATA * ch, CHAR_DATA * victim);
static void death_cry(CHAR_DATA * ch, CHAR_DATA * killer);
static void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);
static int xp_compute(CHAR_DATA * gch, CHAR_DATA * victim, int total_levels);
static void use_magical_item(CHAR_DATA * ch);
static int max_damage(CHAR_DATA * ch, CHAR_DATA * victim, int dt, int amt);
static void mob_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt);
static void check_deathdrop(CHAR_DATA * ch);


/***************************************************************************
 *	violence_update
 *
 *	control the fights going on.
 *	called periodically by update_handler.
 ***************************************************************************/
void violence_update(void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;

    for (ch = char_list; ch != NULL; ch = ch_next) {
	ch_next = ch->next;

	if ((victim = ch->fighting) == NULL || ch->in_room == NULL) {
	    continue;
	}

	if (IS_AWAKE(ch) && ch->in_room == victim->in_room)
	    multi_hit(ch, victim, TYPE_UNDEFINED);
	else
	    stop_fighting(ch, false);

	if ((victim = ch->fighting) == NULL)
	    continue;

	check_assist(ch, victim);


	if (IS_NPC(ch)) {
	    if (HAS_TRIGGER(ch, TRIG_FIGHT))
		mp_percent_trigger(ch, victim, NULL, NULL, TRIG_FIGHT);
	    if (HAS_TRIGGER(ch, TRIG_HPCNT))
		mp_hprct_trigger(ch, victim);
	}
    }

    return;
}


/***************************************************************************
 *	check_assist
 *
 *	check for auto-assist
 ***************************************************************************/
void check_assist(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *rch, *rch_next;

    for (rch = ch->in_room->people; rch != NULL; rch = rch_next) {
	rch_next = rch->next_in_room;

	if (IS_AWAKE(rch) && rch->fighting == NULL) {
	    /* quick check for ASSIST_PLAYER */
	    if (!IS_NPC(ch) && IS_NPC(rch)
		    && IS_SET(rch->off_flags, ASSIST_PLAYERS)
		    && rch->level + 6 > victim->level) {
		broadcast_channel(rch, channels_find(CHANNEL_EMOTE), NULL, "```!screams and attacks!``");
		multi_hit(rch, victim, TYPE_UNDEFINED);
		continue;
	    }

	    /* PCs next */
	    if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
		if (((!IS_NPC(rch) && IS_SET(rch->act, PLR_AUTOASSIST))
			    || IS_AFFECTED(rch, AFF_CHARM))
			&& is_same_group(ch, rch)
			&& !is_safe(rch, victim)) {
		    if (IS_NPC(victim)) {
			multi_hit(rch, victim, TYPE_UNDEFINED);
			continue;
		    }

		    multi_hit(rch, victim, TYPE_UNDEFINED);
		}
		continue;
	    }

	    /* now check the NPC cases */

	    if (IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM)) {
		if ((IS_NPC(rch) && IS_SET(rch->off_flags, ASSIST_ALL))
			|| (IS_NPC(rch) && rch->group && rch->group == ch->group)
			|| (IS_NPC(rch) && rch->race == ch->race && IS_SET(rch->off_flags, ASSIST_RACE))
			|| (rch->mob_idx == ch->mob_idx && IS_SET(rch->off_flags, ASSIST_VNUM))) {
		    CHAR_DATA *vch;
		    CHAR_DATA *target;
		    int number;

		    if (number_bits(1) == 0)
			continue;

		    target = NULL;
		    number = 0;
		    for (vch = ch->in_room->people; vch; vch = vch->next) {
			if (can_see(rch, vch)
				&& is_same_group(vch, victim)
				&& number_range(0, number) == 0) {
			    target = vch;
			    number++;
			}
		    }

		    if (target != NULL) {
			broadcast_channel(rch, channels_find(CHANNEL_EMOTE), NULL, "```!screams and attacks!``");
			multi_hit(rch, target, TYPE_UNDEFINED);
		    }
		}
	    }
	}
    }
}


/***************************************************************************
 *	multi_hit
 *
 *	do one group of attacks
 ***************************************************************************/
void multi_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    GAMEOBJECT *weapon;
    GAMEOBJECT *off_weapon;
    SKILL *skill;
    int chance;

    weapon = get_eq_char(ch, WEAR_WIELD);

    /* decrement the wait */
    if (ch->desc == NULL)
	ch->wait = (int)UMAX(0, ch->wait - PULSE_VIOLENCE);

    /* no attacks for stunnies -- just a check */
    if (ch->position < POS_RESTING)
	return;

    if (is_affected(ch, gsp_web))
	return;

    if (IS_NPC(ch)) {
	mob_hit(ch, victim, dt);
	return;
    }


    if (is_affected(ch, gsp_banzai)) {
	int tmp_dam;
	int tmp_hit;

	tmp_dam = ch->damroll;
	tmp_hit = ch->hitroll;

	ch->damroll += (ch->level * 10);
	ch->hitroll += (ch->level * 10);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
	    vch_next = vch->next;

	    if (is_safe(ch, victim))
		continue;

	    if ((vch != victim && vch->fighting == ch))
		one_attack(ch, vch, dt, weapon);
	}

	ch->damroll = tmp_dam;
	ch->hitroll = tmp_hit;
    }

    one_attack(ch, victim, dt, weapon);

    if ((off_weapon = get_eq_char(ch, WEAR_SECONDARY)) != NULL) {
	one_attack(ch, victim, dt, off_weapon);
	if (ch->fighting != victim)
	    return;
    }

    if ((off_weapon = get_eq_char(ch, WEAR_THIRD)) != NULL) {
	one_attack(ch, victim, dt, off_weapon);
	if (ch->fighting != victim)
	    return;
    }

    if (ch->fighting != victim)
	return;

    if (IS_AFFECTED(ch, AFF_HASTE))
	one_attack(ch, victim, dt, weapon);

    if (ch->fighting != victim)
	return;


    if ((skill = gsp_second_attack) != NULL) {
	chance = get_learned_percent(ch, skill) / 2;
	if (IS_AFFECTED(ch, AFF_SLOW))
	    chance /= 2;

	if (number_percent() < chance) {
	    one_attack(ch, victim, dt, weapon);
	    check_improve(ch, skill, true, 5);

	    if (ch->fighting != victim)
		return;
	}
    }


    if ((skill = gsp_aggressive_parry) != NULL) {
	if ((chance = get_learned_percent(ch, skill)) >= 1) {
	    chance = (ch->level <= 200) ? ((ch->level + chance) / 4) : ((chance * 3) / 2);

	    if (IS_AFFECTED(ch, AFF_SLOW))
		chance /= 2;

	    if (number_percent() < chance) {
		check_improve(ch, skill, true, 5);
		one_attack(ch, victim, skill->sn, weapon);
		if (ch->fighting != victim)
		    return;
	    }
	}
    }

    if ((skill = gsp_third_attack) != NULL) {
	chance = get_learned_percent(ch, skill) / 3;
	if (IS_AFFECTED(ch, AFF_SLOW))
	    chance /= 2;

	if (number_percent() < chance) {
	    one_attack(ch, victim, dt, weapon);
	    check_improve(ch, skill, true, 6);
	    if (ch->fighting != victim)
		return;
	}
    }

    if ((skill = gsp_fourth_attack) != NULL) {
	chance = get_learned_percent(ch, skill) / 3;
	if (IS_AFFECTED(ch, AFF_SLOW))
	    chance = 0;

	if (number_percent() < chance) {
	    one_attack(ch, victim, dt, weapon);
	    check_improve(ch, skill, true, 6);
	    if (ch->fighting != victim)
		return;
	}
    }

    if ((skill = gsp_fifth_attack) != NULL) {
	chance = get_learned_percent(ch, skill) / 3;
	chance -= 5;
	if (IS_AFFECTED(ch, AFF_SLOW))
	    chance = 0;
	if (number_percent() < chance) {
	    one_attack(ch, victim, dt, weapon);
	    check_improve(ch, skill, true, 6);
	    if (ch->fighting != victim)
		return;
	}
    }
}


/* procedure for all mobile attacks */
void mob_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    GAMEOBJECT *weapon;
    int chance;
    int number;

    weapon = get_eq_char(ch, WEAR_WIELD);

    one_attack(ch, victim, dt, weapon);

    if (ch->fighting != victim)
	return;

    /* Area attack -- BALLS nasty! */
    if (IS_SET(ch->off_flags, OFF_AREA_ATTACK)) {
	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
	    vch_next = vch->next;
	    if ((vch != victim && vch->fighting == ch))
		one_attack(ch, vch, dt, weapon);
	}
    }

    if (IS_AFFECTED(ch, AFF_HASTE)
	    || (IS_SET(ch->off_flags, OFF_FAST) && !IS_AFFECTED(ch, AFF_SLOW)))
	one_attack(ch, victim, dt, weapon);

    chance = 50; 
    if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->off_flags, OFF_FAST))
	chance /= 2;

    if (number_percent() < chance) {
	one_attack(ch, victim, dt, weapon);
	if (ch->fighting != victim)
	    return;
    }

    chance = 30;
    if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->off_flags, OFF_FAST))
	chance = 0;

    if (number_percent() < chance) {
	one_attack(ch, victim, dt, weapon);
	if (ch->fighting != victim)
	    return;
    }

    if (IS_NPC(ch)
	    && number_percent() < UMIN(25, UMAX(10, ch->level))
	    && !IS_NPC(victim))
	use_magical_item(ch);

    if (ch->wait > 0)
	return;

    number = number_range(0, 8);

    switch (number) {
	case (0):
	    if (IS_SET(ch->off_flags, OFF_BASH))
	    do_bash(ch, "");
	    break;
    }
}


static void validate_attack_type(int *dt, CHAR_DATA *ch, GAMEOBJECT *wield)
{
    /*
     * Figure out the type of damage message.
     *(Uses second and third weapons if applicable.)
     */
    if (*dt == TYPE_UNDEFINED) {
	*dt = TYPE_HIT;
	if (wield != NULL && wield->item_type == ITEM_WEAPON)
	    *dt += wield->value[3];
	else
	    *dt += ch->dam_type;
    }
}

static int get_dam_type(CHAR_DATA *ch, int dt, GAMEOBJECT *wield)
{
    int dam_type = -1;

    if (dt < TYPE_HIT) {
	dam_type = wield != NULL
	    ? attack_table[wield->value[3]].damage
	    : attack_table[ch->dam_type].damage;
    } else {
	dam_type = attack_table[dt - TYPE_HIT].damage;
    }

    if (dam_type == -1)
	dam_type = DAM_BASH;

    return dam_type;
}

int lookup_class_table_index(CHAR_DATA *ch)
{
    if (IS_NPC(ch)) {
	if (IS_SET(ch->act, ACT_WARRIOR))
	    return 3;
	else if (IS_SET(ch->act, ACT_THIEF))
	    return 2;
	else if (IS_SET(ch->act, ACT_CLERIC))
	    return 1;
	else if (IS_SET(ch->act, ACT_MAGE))
	    return 0;
	else
	    return 2; // thief
    } else {
	return ch->class;
    }
}

static inline float get_combat_rating(int class_table_idx, int class_level, bool defense)
{
    float combat_rating, cr_improve;

    class_level = UMIN(MAX_LEVEL, class_level);

    combat_rating = defense
	? class_table[class_table_idx].defense_rating
	: class_table[class_table_idx].attack_rating;
    cr_improve = defense
	? class_table[class_table_idx].dr_improve
	: class_table[class_table_idx].ar_improve;

    return (combat_rating + (0.1f * class_level / cr_improve)) * class_level;
}

static inline int get_dex_bonus(CHAR_DATA *ch)
{
    return IS_AWAKE(ch)
	? get_curr_stat(ch, STAT_DEX) * (ch->level / 25)
	: 0;
}

static inline int get_victim_ac(CHAR_DATA *victim, int damage_type)
{
    long victim_ac = 0;

    switch (damage_type) {
	case (DAM_PIERCE):
	    victim_ac = victim->armor[AC_PIERCE];
	    break;
	case (DAM_BASH):
	    victim_ac = victim->armor[AC_BASH];
	    break;
	case (DAM_SLASH):
	    victim_ac = victim->armor[AC_SLASH];
	    break;
	default:
	    victim_ac = victim->armor[AC_EXOTIC];
	    break;
    }
    ;

    return victim_ac;
}

ONE_ATTACK_RESULT one_attack(CHAR_DATA *ch, CHAR_DATA *victim, int dt, GAMEOBJECT *attacker_wield)
{
    ONE_ATTACK_RESULT result = oar_error;

    /* just in case
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if (victim == ch || ch == NULL || victim == NULL
	    || victim->position == POS_DEAD
	    || ch->in_room != victim->in_room) {
	result = oar_error;
    } else if (IS_NPC(ch) && (ch->mob_idx == NULL)) {
	send_to_char("You can't attack. You're an NPC. Contact an IMM.\n\r", ch);
	result = oar_error;
    } else {
	COMBAT_ROLL_BOX attack_roll, defense_roll;
	float attack_resolution = 0;
	long victim_ac = 0;

	attack_roll.special_attack = NULL;

	attack_roll.combatant_level = ch->level;

	/* TODO : consider special attack in determining combat roll. */
	attack_roll.combat_rating = get_combat_rating(lookup_class_table_index(ch), ch->level, false) + get_dex_bonus(ch);
	attack_roll.weapon_sn = get_weapon_sn(ch, attacker_wield);
	attack_roll.weapon_skill = get_weapon_skill(ch, attack_roll.weapon_sn);
	fill_combat_roll(&attack_roll, false, 0);

	defense_roll.combatant_level = victim->level;
	defense_roll.combat_rating = get_combat_rating(lookup_class_table_index(victim), victim->level, true) + get_dex_bonus(victim);
	defense_roll.weapon_sn = get_weapon_sn(victim, NULL);
	defense_roll.weapon_skill = get_weapon_skill(victim, defense_roll.weapon_sn);
	fill_combat_roll(&defense_roll, true, 0);

	if (attack_roll.botch_count > 0) {
	    result = oar_fumble;
	} /* Note that the defender can't really botch. */
	else {
	    int damage_type;

	    /* The moment of truth. */
	    attack_resolution = (attack_roll.combat_rating + (float)attack_roll.total_roll) -
		(defense_roll.combat_rating + (float)defense_roll.total_roll);
	    validate_attack_type(&dt, ch, attacker_wield);
	    damage_type = get_dam_type(ch, dt, attacker_wield);

	    if (attack_resolution < 0) {
		//damage(ch, victim, 0, dt, damage_type, true);
		result = oar_miss;
	    } else {
		victim_ac = get_victim_ac(victim, damage_type);
	    }
	}


	{
	    char buf[MSL];

	    snprintf(buf, MSL, "%5d %5d %5d %5d %5d %5d %5d %5d %8f\n\r",
		    attack_roll.weapon_skill,
		    attack_roll.combatant_level,
		    attack_roll.combat_rating,
		    attack_roll.significant_dice_count,
		    attack_roll.dice_pool,
		    attack_roll.die_type,
		    attack_roll.botch_count,
		    attack_roll.total_roll,
		    attack_resolution);
	    printf("%s", buf);


	    snprintf(buf, MSL, "%5d %5d %5d %5d %5d %5d %5d %5d %8ld\n\r",
		    defense_roll.weapon_skill,
		    defense_roll.combatant_level,
		    defense_roll.combat_rating,
		    defense_roll.significant_dice_count,
		    defense_roll.dice_pool,
		    defense_roll.die_type,
		    defense_roll.botch_count,
		    defense_roll.total_roll,
		    victim_ac);
	    printf("%s", buf);
	}
    }

    tail_chain();
    return result;
}



/***************************************************************************
 *	one_hit
 *
 *	hit a character once with a weapon
 ***************************************************************************/
bool one_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt, GAMEOBJECT *wield)
{
    SKILL *attack;
    long victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam;
    int diceroll;
    int sn;
    int skill;
    int dam_type;
    bool result;

    sn = -1;

    /* just in case */
    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if (victim == ch || ch == NULL || victim == NULL
	    || victim->position == POS_DEAD
	    || ch->in_room != victim->in_room)
	return false;

    validate_attack_type(&dt, ch, wield);
    dam_type = get_dam_type(ch, dt, wield);

    /* get the weapon skill */
    sn = get_weapon_sn(ch, wield);
    skill = 20 + get_weapon_skill(ch, sn);

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if (IS_NPC(ch)) {
	thac0_00 = 20;
	thac0_32 = -4;  /* as good as a thief */

	if (IS_SET(ch->act, ACT_WARRIOR))
	    thac0_32 = -10;
	else if (IS_SET(ch->act, ACT_THIEF))
	    thac0_32 = -4;
	else if (IS_SET(ch->act, ACT_CLERIC))
	    thac0_32 = 2;
	else if (IS_SET(ch->act, ACT_MAGE))
	    thac0_32 = 6;
    } else {
	thac0_00 = class_table[ch->class].thac0_00;
	thac0_32 = class_table[ch->class].thac0_32;
    }
    thac0 = interpolate(ch->level, thac0_00, thac0_32);

    if (thac0 < 0)
	thac0 = thac0 / 2;

    if (thac0 < -5)
	thac0 = -5 + (thac0 + 5) / 2;

    thac0 -= GET_HITROLL(ch) * skill / 100;
    thac0 += 5 * (100 - skill) / 100;


    switch (dam_type) {
	case (DAM_PIERCE):
	    victim_ac = GET_AC(victim, AC_PIERCE) / 10;
	    break;
	case (DAM_BASH):
	    victim_ac = GET_AC(victim, AC_BASH) / 10;
	    break;
	case (DAM_SLASH):
	    victim_ac = GET_AC(victim, AC_SLASH) / 10;
	    break;
	default:
	    victim_ac = GET_AC(victim, AC_EXOTIC) / 10;
	    break;
    }
    ;

    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;

    if (!can_see(ch, victim))
	victim_ac -= 4;

    if (victim->position < POS_FIGHTING)
	victim_ac += 4;

    if (victim->position < POS_RESTING)
	victim_ac += 6;

    /*
     * The moment of excitement!
     */
    while ((diceroll = number_bits(5)) >= 20) {
    }

    /*
     * Because of the supremely insane AC on BT, we're going to remove the thac0 check to compensate for nonstop misses...
     * The problem is that the old thac0 values were kept on a wider range of levels and with a mucher higher ac
     * value range, so while ac values go up and up, the max thac0 value stays the same... here we change the thac0
     * check to something more reasonable, like a dexterity check
     */

    if (diceroll == 0 || diceroll == 19) {
	/* Miss. */
	damage(ch, victim, 0, dt, dam_type, true);
	tail_chain();
	return false;
    }

    /*
     * Hit.
     * Calc damage.
     */
    if (IS_NPC(ch) && (ch->mob_idx == NULL)) {
	send_to_char("You can't attack. You're an NPC. Contact an IMM.\n\r", ch);
	return false;
    }

    if (IS_NPC(ch) && wield == NULL) {
	dam = dice(ch->damage[DICE_NUMBER], ch->damage[DICE_TYPE]);
    } else {
	if (sn != -1)
	    check_improve(ch, resolve_skill_sn(sn), true, 5);
	if (wield != NULL) {
	    dam = dice((int)wield->value[1], (int)(wield->value[2]) * skill / 100);

	    if (get_eq_char(ch, WEAR_SHIELD) == NULL)        /* no shield = more */
		dam = dam * 11 / 10;

	    /* sharpness! */
	    if (IS_WEAPON_STAT(wield, WEAPON_SHARP)) {
		int percent;

		if ((percent = number_percent()) <= (skill / 6))
		    dam = 2 * dam + (dam * 2 * percent / 100);
	    }
	} else {
	    dam = number_range(1 + 4 * skill / 100, 2 * ch->level / 3 * skill / 100);
	}
    }

    /*
     * Bonuses.
     */
    if ((attack = gsp_enhanced_damage) != NULL && get_learned_percent(ch, attack) > 0) {
	diceroll = number_percent();
	if (diceroll <= get_learned_percent(ch, attack)) {
	    check_improve(ch, attack, true, 6);
	    dam += 2 * (dam * diceroll / 300);
	}
    }

    if (!IS_AWAKE(victim))
	dam *= 2;
    else if (victim->position < POS_FIGHTING)
	dam = dam * 3 / 2;

    if (wield != NULL) {
	if (dt == get_skill_number("displace")) {
	    if (wield->value[0] != 2)
		dam *= 2 + (ch->level / 60);
	    else
		dam *= 2 + (ch->level / 50);

	}

	if ((attack = gsp_aggressive_parry) != NULL && attack->sn == dt) {
	    if (wield->value[0] != 2)
		dam += (5 * (get_learned_percent(ch, attack))) / 3;
	    else
		dam += (ch->level + (get_learned_percent(ch, attack)));
	}
    }

    if ((attack = gsp_flanking) != NULL
	    && get_learned_percent(ch, attack) > 0
	    && get_learned_percent(victim, attack) <= 0) {
	if (number_range(1, 100) < get_learned_percent(ch, attack)) {
	    dam += 2 * (get_learned_percent(ch, attack) * 2 / 3);
	    check_improve(ch, attack, true, 2);
	}
    }

    dam += GET_DAMROLL(ch) * UMIN(100, skill) / 100;

    if (dam > 0) {
	/* add more damage for STR stat */
	dam += get_curr_stat(ch, STAT_STR) / 2;
	/* reduce damage for CON stat */
	dam -= get_curr_stat(ch, STAT_CON) / 2;
	/* make sure we didnt end up with negative damage */
	dam = UMAX(0, dam);
    }


    if (dam <= 0)
	dam = 1;
    result = (damage(ch, victim, dam, dt, dam_type, true) > 0);

    if (result && wield != NULL) {
	int dam;

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_POISON)) {
	    AFFECT_DATA *poison;
	    AFFECT_DATA af;
	    int level;

	    if ((poison = affect_find(wield->affected, gsp_poison)) == NULL)
		level = wield->level;
	    else
		level = poison->level;

	    if (!saves_spell(level / 2, victim, DAM_POISON)) {
		SKILL *skill_poison;

		if ((skill_poison = gsp_poison) != NULL) {
		    send_to_char("You feel poison coursing through your veins.", victim);
		    act("$n is poisoned by the venom on $p.", victim, wield, NULL, TO_ROOM);

		    af.where = TO_AFFECTS;
		    af.type = skill_poison->sn;
		    af.skill = skill_poison;
		    af.level = (int)(level * 3 / 4);
		    af.duration = (int)(level / 2);
		    af.location = APPLY_STR;
		    af.modifier = -1;
		    af.bitvector = AFF_POISON;
		    affect_join(victim, &af);
		}
	    }

	    /* weaken the poison if it's temporary */
	    if (poison != NULL) {
		poison->level = (int)UMAX(0, poison->level - 2);
		poison->duration = (int)UMAX(0, poison->duration - 1);

		if (poison->level == 0 || poison->duration == 0)
		    act("The poison on $p has worn off.", ch, wield, NULL, TO_CHAR);
	    }
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_ACIDIC)) {
	    dam = number_range(1, (wield->level / 50) * 5);

	    /*
	     * act("$p's coat of acid sears $n's flesh.", victim, wield, NULL, TO_ROOM);
	     * act("The coat of acid on $p burns your flesh.", victim, wield, NULL, TO_CHAR);
	     */

	    damage(ch, victim, dam, 0, DAM_ACID, false);
	    acid_effect(victim, ch->level, dam, TARGET_CHAR);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_VAMPIRIC)) {
	    dam = number_range(1, wield->level / 5 + 1);

	    /*
	     * act("$p draws life from $n.", victim, wield, NULL, TO_ROOM);
	     * act("You feel $p drawing your life away.", victim, wield, NULL, TO_CHAR);
	     */

	    damage(ch, victim, dam, 0, DAM_NEGATIVE, false);
	    ch->hit += dam / 2;
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_FLAMING)) {
	    dam = number_range(1, wield->level / 4 + 1);

	    /*
	     * act("$n is burned by $p.", victim, wield, NULL, TO_ROOM);
	     * act("$p sears your flesh.", victim, wield, NULL, TO_CHAR);
	     */

	    fire_effect((void *)victim, (int)(wield->level / 2), dam, TARGET_CHAR);
	    damage(ch, victim, dam, 0, DAM_FIRE, false);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_FROST)) {
	    dam = number_range(1, wield->level / 6 + 2);

	    /*
	     * act("$p freezes $n.", victim, wield, NULL, TO_ROOM);
	     * act("The cold touch of $p surrounds you with ice.", victim, wield, NULL, TO_CHAR);
	     */

	    cold_effect(victim, (int)(wield->level / 2), dam, TARGET_CHAR);
	    damage(ch, victim, dam, 0, DAM_COLD, false);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_SHOCKING)) {
	    dam = number_range(1, wield->level / 5 + 2);

	    /*
	     * act("$n is struck by lightning from $p.", victim, wield, NULL, TO_ROOM);
	     * act("You are shocked by $p.", victim, wield, NULL, TO_CHAR);
	     */

	    shock_effect(victim, (int)(wield->level / 2), dam, TARGET_CHAR);
	    damage(ch, victim, dam, 0, DAM_LIGHTNING, false);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_VORPAL)) {
	    if (vorpal_effect(ch, victim, wield))
		result = false;
	}
    }
    tail_chain();
    return result;
}


/***************************************************************************
 *	damage
 *
 *	damage a character -- this function is long and freakin ugly
 *	it needs to be tweaked so it is more managable
 ***************************************************************************/
int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type, bool show)
{
    GAMEOBJECT *corpse;
    bool immune;

    if (victim->position == POS_DEAD)
	return 0;

    furniture_check(ch);

    if (IS_AFFECTED(ch, AFF_CALLOUSED))
	dam = (dam * 11) / 10;

    if (victim != ch) {
	if (is_safe(ch, victim))
	    return 0;

	check_killer(ch, victim);
	if (victim->position > POS_STUNNED) {
	    if (victim->fighting == NULL) {
		set_fighting(victim, ch);
		if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_KILL))
		    mp_percent_trigger(victim, ch, NULL, NULL, TRIG_KILL);
	    }
	    if (victim->timer <= 4)
		victim->position = POS_FIGHTING;
	}

	if (victim->position > POS_STUNNED) {
	    if (ch->fighting == NULL)
		set_fighting(ch, victim);
	}

	/* more charm stuff */
	if (victim->master == ch)
	    stop_follower(victim);
    }

    /* strip certain affects */
    if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
	affect_strip(ch, gsp_invisibility);
	affect_strip(ch, gsp_mass_invisibility);
	REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
	act("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
    }

    if (is_affected(ch, gsp_darkness))
	affect_strip(ch, gsp_darkness);


    /* damage modifiers */

    if (dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY))
	dam /= 2;

    if (dam > 1 && !IS_NPC(victim) && IS_AFFECTED(victim, AFF_DRUID_CALL))
	dam -= dam / 4;

    if (dam > 1 && IS_AFFECTED(ch, AFF_CALLOUSED)) {
	int random;
	random = number_range(10, 200);

	dam += random;
    }

    immune = false;
    /* check for parry, dodge, and shield block */
    if (dt >= TYPE_HIT && ch != victim) {
	if (check_parry(ch, victim))
	    return 0;
	if (check_dodge(ch, victim))
	    return 0;
	if (check_evade(ch, victim))
	    return 0;
	if (check_shield_block(ch, victim))
	    return 0;
    }

    switch (check_immune(victim, dam_type)) {
	case (IS_IMMUNE):
	    immune = true;
	    dam = 0;
	    break;
	case (IS_RESISTANT):
	    dam -= dam / 3;
	    break;
	case (IS_VULNERABLE):
	    dam += dam / 2;
	    break;
    }

    dam = max_damage(ch, victim, dt, dam);


    if (show)
	dam_message(ch, victim, dam, dt, immune);

    if (dam == 0)
	return 0;

    /* hurt the victim - inform the victim of his new state */
    victim->hit -= dam;

    /* keep immortals from dying */
    if (!IS_NPC(victim)
	    && victim->level >= LEVEL_IMMORTAL
	    && victim->hit < 1)
	victim->hit = 1;

    update_pos(victim);

    switch (victim->position) {
	case POS_MORTAL:
	    act("$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM);
	    send_to_char("You are mortally wounded, and will die soon, if not aided.\n\r", victim);
	    break;

	case POS_INCAP:
	    act("$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM);
	    send_to_char("You are incapacitated and will slowly die, if not aided.\n\r", victim);
	    break;

	case POS_STUNNED:
	    act("$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM);
	    send_to_char("You are stunned, but will probably recover.\n\r", victim);
	    break;

	case POS_DEAD:
	    if (IS_NPC(victim)) {
		act("$n is ```!DEAD!!``", victim, 0, 0, TO_ROOM);
		send_to_char("You have been ```!K```1i```!LLED!!``\n\r\n\r", victim);
	    } else {
		act("$n falls to the ground, `8unconcious``, and `8fade`&s a`*way``.", victim, 0, 0, TO_ROOM);
		send_to_char("As the final swing descends, `8Blackness`` overcomes you....\n\r\n\r", victim);
	    }
	    break;

	default:
	    if (dam > victim->max_hit / 4)
		send_to_char("That really did ```1HURT!``\n\r", victim);
	    if (victim->hit < victim->max_hit / 4)
		send_to_char("You sure are ```!BLEEDING!``\n\r", victim);
	    break;
    }

    /* sleep spells and extremely wounded folks */
    if (!IS_AWAKE(victim))
	stop_fighting(victim, false);

    /* experience */
    if (victim->position == POS_DEAD) {
	group_gain(ch, victim);
	if (!IS_NPC(victim)) {
	    log_string("%s killed by %s at %ld", victim->name, (IS_NPC(ch) ? ch->short_descr : ch->name), ch->in_room->vnum);

	    if (victim->exp > exp_per_level(victim, victim->pcdata->points) * victim->level)
		gain_exp(victim, (2 * (exp_per_level(victim, victim->pcdata->points)
				* victim->level - victim->exp) / 2) + 50);
	}

	if (!IS_NPC(ch)) {
	    if ((ch != victim) && (!IS_TRUSTED(ch, IMPLEMENTOR))) {
		if (IS_NPC(victim)) {
		    ch->pcdata->mobkills++;
		} else {
		    ch->pcdata->pkills++;
		    victim->pcdata->pdeaths++;
		}
	    }
	} else {
	    if ((!IS_NPC(victim)) && (!IS_TRUSTED(victim, IMPLEMENTOR)))
		victim->pcdata->mobdeaths++;
	}

	{
	    static char wiznet_message[MIL];
	    sprintf(wiznet_message, "%s got toasted by %s at %s [room %ld]",
		    (IS_NPC(victim) ? victim->short_descr : victim->name),
		    (IS_NPC(ch) ? ch->short_descr : ch->name),
		    ch->in_room->name,
		    ch->in_room->vnum);

	    if (IS_NPC(victim))
		impnet(wiznet_message, NULL, NULL, (long)IMN_MOBDEATHS, 0, 0);
	    else
		wiznet(wiznet_message, NULL, NULL, WIZ_DEATHS, 0, 0);
	}

	check_killer(ch, victim);

	/* death trigger */
	if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_DEATH)) {
	    victim->position = POS_STANDING;
	    mp_percent_trigger(victim, ch, NULL, NULL, TRIG_DEATH);
	}

	raw_kill(victim, ch);
	if (ch != victim && !IS_NPC(ch)) {
	    if (IS_SET(victim->act, PLR_KILLER))
		REMOVE_BIT(victim->act, PLR_KILLER);
	    else
		REMOVE_BIT(victim->act, PLR_THIEF);
	}

	if (!IS_NPC(ch) && IS_NPC(victim)) {
	    GAMEOBJECT *coins;

	    corpse = get_obj_list(ch, "corpse", ch->in_room->contents);

	    if (IS_SET(ch->act, PLR_AUTOLOOT)
		    && corpse
		    && corpse->contains)
		do_get(ch, "all corpse");

	    if (IS_SET(ch->act, PLR_AUTOGOLD)
		    && corpse
		    && corpse->contains
		    && !IS_SET(ch->act, PLR_AUTOLOOT)) {
		if ((coins = get_obj_list(ch, "gcash", corpse->contains)) != NULL)
		    do_get(ch, "all.gcash corpse");
	    }

	    if (IS_SET(ch->act, PLR_AUTOSAC)) {
		if (IS_SET(ch->act, PLR_AUTOLOOT)
			&& corpse
			&& corpse->contains)
		    return dam;
		else
		    do_sacrifice(ch, "corpse");
	    }
	}
	return dam;
    }

    if (victim == ch)
	return dam;

    /* wimp out? */
    if (IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2) {
	if ((IS_SET(victim->act, ACT_WIMPY)
		    && number_bits(2) == 0
		    && victim->hit < victim->max_hit / 5)
		|| (IS_AFFECTED(victim, AFF_CHARM)
		    && victim->master != NULL
		    && victim->master->in_room != victim->in_room))
	    do_flee(victim, "");
    }

    if (!IS_NPC(victim)
	    && victim->hit > 0
	    && victim->hit <= victim->wimpy
	    && victim->wait < PULSE_VIOLENCE / 2)
	do_flee(victim, "");

    tail_chain();
    return dam;
}



/***************************************************************************
 *	is_safe
 *
 *	check to see if a character is safe from attackes
 ***************************************************************************/
bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (victim->in_room == NULL || ch->in_room == NULL)
	return true;

    if (victim->fighting == ch || victim == ch)
	return false;

    /* safe room? */
    if (IS_SET(victim->in_room->room_flags, ROOM_SAFE)
	    || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
	    || is_affected_room(victim->in_room, gsp_haven)
	    || is_affected_room(ch->in_room, gsp_haven)) {
	send_to_char("Not in this room.\n\r", ch);
	return true;
    }

    if (IS_NPC(victim)) {
	if (IS_SHOPKEEPER(victim)) {
	    send_to_char("The shopkeeper wouldn't like that.\n\r", ch);
	    return true;
	}

	/* no killing healers, trainers, etc */
	if (IS_TRAINER(victim)
		|| IS_GUILDMASTER(victim)
		|| IS_HEALER(victim)
		|| IS_CHANGER(victim)
		|| IS_EXCHANGER(victim)) {
	    send_to_char("I don't think Mota would approve.\n\r", ch);
	    return true;
	}

	if (!IS_NPC(ch)) {
	    /* no pets */
	    if (IS_SET(victim->act, ACT_PET)) {
		act("But $N looks so cute and cuddly...", ch, NULL, victim, TO_CHAR);
		return true;
	    }

	    /* no charmed creatures unless owner */
	    if (IS_AFFECTED(victim, AFF_CHARM) && ch != victim->master) {
		send_to_char("You don't own that monster.\n\r", ch);
		return true;
	    }
	}
    }
    /* killing players */
    else {
	/* NPC doing the killing */
	if (IS_NPC(ch)) {
	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
		    && ch->master->fighting != victim) {
		send_to_char("Players are your friends!\n\r", ch);
		return true;
	    }
	}
	/* player doing the killing */
	else {
	    if ((victim->level < LEVEL_NEWBIE) && (!IS_NPC(victim))) {
		send_to_char("Newbies are protected from `3s`2c`3u`2m`` like you!\n\r", ch);
		return true;
	    }

	    if ((ch->level < LEVEL_NEWBIE) && (!IS_NPC(victim))) {
		send_to_char("Stupid newbie, pkills are for real players.\n\r", ch);
		return true;
	    }

	    if (IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF))
		return false;

	    if (victim->level - ch->level >= 20 || ch->level - victim->level >= 20) {
		if ((victim->level > ch->level)) {
		    /*
		     * send_to_char("You charge into the fray!\n\r",ch);
		     * continue;
		     */
		} else {
		    send_to_char("They are out of your attack range!\n\r", ch);
		    return true;
		}
	    }
	}
    }

    return false;
}


bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area)
{
    if (IS_IMMORTAL(ch))
	return false;

    if (victim->in_room == NULL || ch->in_room == NULL)
	return true;

    if (victim == ch && area)
	return true;

    if (victim->fighting == ch || victim == ch)
	return false;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL && !area)
	return false;

    /* killing mobiles */
    if (IS_NPC(victim)) {
	/* safe room? */
	if (IS_SET(victim->in_room->room_flags, ROOM_SAFE)
		|| is_affected_room(victim->in_room, gsp_haven))
	    return true;

	/* no killing healers, trainers, etc */
	if (IS_SHOPKEEPER(victim)
		|| IS_TRAINER(victim)
		|| IS_GUILDMASTER(victim)
		|| IS_HEALER(victim)
		|| IS_CHANGER(victim)
		|| IS_EXCHANGER(victim))
	    return true;

	if (!IS_NPC(ch)) {
	    /* no pets */
	    if (IS_SET(victim->act, ACT_PET))
		return true;

	    /* no charmed creatures unless owner */
	    if (IS_AFFECTED(victim, AFF_CHARM) && (area || ch != victim->master))
		return true;

	    /* legal kill? -- cannot hit mob fighting non-group member */
	    if (victim->fighting != NULL && !is_same_group(ch, victim->fighting))
		return true;
	} else {
	    /* area effect spells do not hit other mobs */
	    if (area && !is_same_group(victim, ch->fighting))
		return true;
	}
    }
    /* killing players */
    else {
	if (area && IS_IMMORTAL(victim) && victim->level > LEVEL_IMMORTAL)
	    return true;

	if (area && is_same_group(victim, ch))
	    return true;

	/* NPC doing the killing */
	if (IS_NPC(ch)) {
	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
		    && ch->master->fighting != victim)
		return true;

	    /* safe room? */
	    if (IS_SET(victim->in_room->room_flags, ROOM_SAFE))
		return true;

	    /* legal kill? -- mobs only hit players grouped with opponent */
	    if (ch->fighting != NULL && !is_same_group(ch->fighting, victim))
		return true;
	}
	/* player doing the killing */
	else {
	    if (IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF))
		return false;

	    if (is_safe(ch, victim) && victim != ch)
		return true;
	}
    }
    return false;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char buf[MSL];

    /*fuck
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    /* Eo
     * while(IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL)
     * victim = victim->master; */
    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if (IS_NPC(victim) || IS_SET(victim->act, PLR_KILLER) || IS_SET(victim->act, PLR_THIEF))
	return;

    /*
     * Charm-o-rama.
     */
    if (IS_AFFECTED(ch, AFF_CHARM)) {
	if (ch->master == NULL) {
	    log_bug("Check_killer: %s bad AFF_CHARM", IS_NPC(ch) ? ch->short_descr : ch->name);
	    affect_strip(ch, skill_lookup("charm person"));
	    REMOVE_BIT(ch->affected_by, AFF_CHARM);
	    return;
	}

	send_to_char("One day your `1blood`7 may flow just as easily.\n\r", ch);
	log_string("%s is a killer!", ch->name);

	stop_follower(ch);
	return;
    }

    /*
     * NPC's are cool of course(as long as not charmed).
     * Hitting yourself is cool too(bleeding).
     * So is being immortal(Alander's idea).
     * And current killers stay as they are.
     */
    if (IS_NPC(ch) || ch == victim || ch->level >= LEVEL_IMMORTAL || IS_SET(ch->act, PLR_KILLER) || ch->fighting == victim)
	return;

    if (ch->in_room->sector_type != SECT_CITY)
	return;

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)) {
	send_to_char("```&*```O*```8* ``You are now a ```!K```1i```!LLER!! ```8*```O*```&*``\n\r", ch);
	SET_BIT(ch->act, PLR_KILLER);
    }

    ch->pcdata->killer_time = time(NULL);
    sprintf(buf, "$N is attempting to murder %s", victim->name);
    wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
    save_char_obj(ch);

    return;
}

/***************************************************************************
 * Check for parry.
 ***************************************************************************/
bool check_parry(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance;

    if (!IS_AWAKE(victim))
	return false;

    chance = get_learned_percent(victim, gsp_parry) / 2;

    if (get_eq_char(victim, WEAR_WIELD) == NULL) {
	if (IS_NPC(victim))
	    chance /= 2;
	else
	    return false;
    }

    if (!can_see(ch, victim))
	chance /= 2;


    if (number_percent() >= chance + victim->level - ch->level)
	return false;

    act("```^You parry $n's attack.``", ch, NULL, victim, TO_VICT);
    act("```6$N parries your attack.``", ch, NULL, victim, TO_CHAR);

    check_improve(victim, gsp_parry, true, 6);
    return true;
}

/***************************************************************************
 * Check for evade.
 ***************************************************************************/
bool check_evade(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance;

    if (!IS_AWAKE(victim))
	return false;

    if (IS_NPC(victim))
	return false;

    chance = get_learned_percent(victim, gsp_evade) / 2;

    if (get_eq_char(victim, WEAR_WIELD) == NULL) {
	if (IS_NPC(victim))
	    chance /= 2;
    }

    if (!can_see(ch, victim))
	chance /= 2;

    if (IS_NPC(ch))
	return false;

    if (number_percent() >= chance + victim->level - ch->level)
	return false;

    act("```^You evade $n's attack.``", ch, NULL, victim, TO_VICT);
    act("```6$N evades your attack.``", ch, NULL, victim, TO_CHAR);
    check_improve(victim, gsp_evade, true, 6);
    return true;
}

/***************************************************************************
 * Check for shield block.
 ***************************************************************************/
bool check_shield_block(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance;

    if (!IS_AWAKE(victim))
	return false;

    chance = get_learned_percent(victim, gsp_shield_block) / 2;

    if (get_eq_char(victim, WEAR_SHIELD) == NULL)
	return false;

    if (number_percent() >= chance + victim->level - ch->level)
	return false;

    act("```^You block $n's attack with your shield.``", ch, NULL, victim, TO_VICT);
    act("```6$N blocks your attack with a shield.``", ch, NULL, victim, TO_CHAR);
    check_improve(victim, gsp_shield_block, true, 6);
    return true;
}


/***************************************************************************
 * Check for dodge.
 ***************************************************************************/
bool check_dodge(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance;

    if (!IS_AWAKE(victim))
	return false;

    chance = get_learned_percent(victim, gsp_dodge) / 2;

    if (!can_see(victim, ch))
	chance /= 2;


    if (number_percent() >= chance + victim->level - ch->level)
	return false;

    act("```^You dodge $n's attack.``", ch, NULL, victim, TO_VICT);
    act("```6$N dodges your attack.``", ch, NULL, victim, TO_CHAR);
    check_improve(victim, gsp_dodge, true, 6);
    return true;
}



/*
 * Set position of a victim.
 */
void update_pos(CHAR_DATA *victim)
{
    if (victim->hit > 0) {
	if (victim->position <= POS_STUNNED)
	    victim->position = POS_STANDING;
	return;
    }

    if (IS_NPC(victim) && victim->hit < 1) {
	victim->position = POS_DEAD;
	return;
    }

    if (victim->hit <= -11) {
	victim->position = POS_DEAD;
	return;
    }

    if (victim->hit <= -6)
	victim->position = POS_MORTAL;
    else if (victim->hit <= -3)
	victim->position = POS_INCAP;
    else
	victim->position = POS_STUNNED;

    return;
}



/*
 * Start fights.
 */
void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (ch->fighting != NULL) {
	log_bug("Set_fighting: already fighting");
	return;
    }

    if (IS_AFFECTED(ch, AFF_SLEEP)) {
	affect_strip(ch, gsp_sleep);
    }

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    return;
}



/*
 * Stop fights.
 */
void stop_fighting(CHAR_DATA *ch, bool fBoth)
{
    CHAR_DATA *fch;

    for (fch = char_list; fch != NULL; fch = fch->next) {
	if (fch == ch || (fBoth && fch->fighting == ch)) {
	    fch->fighting = NULL;
	    fch->position = IS_NPC(fch) ? fch->default_pos : POS_STANDING;
	    update_pos(fch);
	}
    }

    return;
}



/*
 * Make a corpse out of a character.
 */
void make_corpse(CHAR_DATA *ch)
{
    char buf[MSL];
    GAMEOBJECT *corpse;
    GAMEOBJECT *obj;
    GAMEOBJECT *obj_next;
    char *name;


    if (IS_NPC(ch)) {
	name = ch->short_descr;
	corpse = create_object(objectprototype_getbyvnum(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer = number_range(3, 6);
	if (ch->gold > 0) {
	    obj_to_obj(create_money(ch->gold, ch->silver), corpse);
	    ch->gold = 0;
	    ch->silver = 0;
	}
	corpse->cost = 0;
    } else {
	name = ch->name;
	corpse = create_object(objectprototype_getbyvnum(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer = number_range(25, 40);
	corpse->owner = str_dup(ch->name);
	corpse->value[0] = 0;

	if (ch->level < LEVEL_IMMORTAL) {
	    if (ch->gold > 1 || ch->silver > 1) {
		ROOM_INDEX_DATA *gold_room;

		gold_room = get_random_room(ch, NULL);
		obj_to_room(create_money(ch->gold / 5, ch->silver / 5), gold_room);
		obj_to_obj(create_money(ch->gold / 5, ch->silver / 5), corpse);
		ch->gold -= 3 * ch->gold / 5;
		ch->silver -= 3 * ch->silver / 5;
	    }
	}

	corpse->cost = 0;
    }

    corpse->level = ch->level;

    sprintf(buf, corpse->short_descr, name);
    free_string(corpse->short_descr);
    corpse->short_descr = str_dup(buf);

    sprintf(buf, corpse->description, name);
    free_string(corpse->description);
    corpse->description = str_dup(buf);

    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
	bool floating = false;

	obj_next = obj->next_content;
	obj_from_char(obj);

	if (obj->item_type == ITEM_POTION)
	    obj->timer = number_range(500, 1000);

	if (obj->item_type == ITEM_SCROLL)
	    obj->timer = number_range(1000, 2500);

	if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH) && !floating) {
	    obj->timer = number_range(5, 10);
	    REMOVE_BIT(obj->extra_flags, ITEM_ROT_DEATH);
	}

	REMOVE_BIT(obj->extra_flags, ITEM_VIS_DEATH);

	if (IS_SET(obj->extra_flags, ITEM_INVENTORY)) {
	    extract_obj(obj);
	} else if ((obj->wear_loc == WEAR_FLOAT) && (!IS_NPC(ch))) {
	    obj_to_room(obj, ch->in_room);
	    return;
	} else {
	    obj_to_obj(obj, corpse);
	}
    }

    obj_to_room(corpse, ch->in_room);
    return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry(CHAR_DATA *ch, CHAR_DATA *killer)
{
    char *msg;
    long vnum;

    vnum = 0;
    msg = str_dup("");

    if (!IS_NPC(ch)) {
	if (!IS_NPC(killer)) {
	    msg = "$n's ear is severed from their head just before the final blow lands.";
	    vnum = OBJ_VNUM_SEVERED_HEAD;
	}
    } else if (!IS_NPC(ch)) {
    } else {
	switch (number_bits(4)) {
	    case 0:
		msg = "$n hits the ground ... ````D```8E```1A```8D``.";
		break;
	    case 1:
		if (ch->material == 0) {
		    msg = "$n splatters ```1blood`` on your armor.";
		    break;
		}
	    case 2:
		if (IS_SET(ch->parts, PART_GUTS)) {
		    msg = "$n spills $s ```2g```3u```2t```3s ``all over the floor.";
		    vnum = OBJ_VNUM_GUTS;
		}
		break;
	    case 3:
		if (IS_SET(ch->parts, PART_HEAD)) {
		    msg = "$n's ear is severed from their head just before the final blow lands.";
		    vnum = OBJ_VNUM_SEVERED_HEAD;
		}
		break;
	    case 4:
		if (IS_SET(ch->parts, PART_HEART)) {
		    msg = "$n's heart is torn from $s chest.";
		    vnum = OBJ_VNUM_TORN_HEART;
		}
		break;
	    case 5:
		if (IS_SET(ch->parts, PART_ARMS)) {
		    msg = "$n's arm is sliced from $s dead body.";
		    vnum = OBJ_VNUM_SLICED_ARM;
		}
		break;
	    case 6:
		if (IS_SET(ch->parts, PART_LEGS)) {
		    msg = "$n's leg is sliced from $s dead body.";
		    vnum = OBJ_VNUM_SLICED_LEG;
		}
		break;
	    case 7:
		if (IS_SET(ch->parts, PART_BRAINS)) {
		    msg = "$n's head is shattered, and $s ```Pb``r```Pa``i```Pn``s splash all over you.";
		    vnum = OBJ_VNUM_BRAINS;
		}
	}
    }

    if (msg[0] != '\0')
	act(msg, ch, NULL, NULL, TO_ROOM);

    if (vnum != 0) {
	char buf[MSL];
	GAMEOBJECT *obj;
	char *name;

	name = IS_NPC(ch) ? ch->short_descr : ch->name;
	obj = create_object(objectprototype_getbyvnum(vnum), 0);
	obj->timer = number_range(4, 7);

	sprintf(buf, obj->short_descr, name);
	free_string(obj->short_descr);
	obj->short_descr = str_dup(buf);

	sprintf(buf, obj->description, name);
	free_string(obj->description);
	obj->description = str_dup(buf);

	if (obj->item_type == ITEM_TREASURE && !IS_NPC(ch))
	    obj->timer = -1;

	if (obj->item_type == ITEM_FOOD) {
	    if (IS_SET(ch->form, FORM_POISON))
		obj->value[3] = 1;
	    else if (!IS_SET(ch->form, FORM_EDIBLE))
		obj->item_type = ITEM_TRASH;
	}

	obj_to_room(obj, ch->in_room);
    }

    return;
}


void raw_kill(CHAR_DATA *victim, CHAR_DATA *killer)
{
    stop_fighting(victim, true);

    if (!IS_NPC(victim)) {
	if (victim->pcdata->deathcry != NULL)
	    broadcast_channel(victim, channels_find(CHANNEL_SHOUT), NULL, victim->pcdata->deathcry);
    }

    if (victim->desc)
	do_die(victim, "");


    check_deathdrop(victim);

    if (victim != NULL && killer != NULL)
	death_cry(victim, killer);

    if (IS_NPC(victim))
	make_corpse(victim);


    if (IS_NPC(victim)) {
	victim->mob_idx->killed++;
	extract_char(victim, true);

	return;
    }

    extract_char(victim, false);

    victim->position = POS_SLEEPING;
    victim->hit = UMAX(1, victim->hit);
    victim->mana = UMAX(1, victim->mana);
    victim->move = UMAX(1, victim->move);

    strip_negative_affects(victim);
}



void group_gain(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char buf[MSL];
    CHAR_DATA *gch;
    int xp;
    int members;
    int group_levels;
    bool check;

    /*
     * Monsters don't get kill xp's.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if (victim == ch) {
	return;
    }

    members = 0;
    group_levels = 0;
    check = false;

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
	if (is_same_group(gch, ch)) {
	    members++;
	    group_levels += IS_NPC(gch) ? gch->level / 6 : gch->level;
	    check = check || (((gch->level - ch->level) > 20) || ((ch->level - gch->level) > 20));
	}
    }


    if (members == 0) {
	members = 1;
	group_levels = ch->level;
    }

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
	GAMEOBJECT *obj;
	GAMEOBJECT *obj_next;

	if (!is_same_group(gch, ch) || IS_NPC(gch))
	    continue;

	if (check == true || members >= 6)
	    send_to_char("You are out of range or this group is too large, your experience gain is reduced.\n\r", gch);
	else
	    group_levels = gch->level * 4 / 3;
	xp = xp_compute(gch, victim, group_levels);

	sprintf(buf, "You receive %d experience points.\n\r", xp);
	send_to_char(buf, gch);
	gain_exp(gch, xp);

	for (obj = ch->carrying; obj != NULL; obj = obj_next) {
	    obj_next = obj->next_content;
	    if (obj->wear_loc == WEAR_NONE)
		continue;

	    if (IS_OBJ_STAT2(obj, ITEM_RELIC) && obj->xp_tolevel > 0)
		gain_object_exp(ch, obj, xp);
	}
    }
    save_char_obj(ch);
    return;
}



/*
 * Compute xp for a kill.
 * Edit this function to change xp computations.
 */
int xp_compute(CHAR_DATA *gch, CHAR_DATA *victim, int total_levels)
{
    int xp, base_exp;
    int level_range;

    level_range = victim->level - gch->level;

    /* compute the base exp */
    switch (level_range) {
	default:
	    base_exp = 0;
	    break;
	case -9:
	    base_exp = 1;
	    break;
	case -8:
	    base_exp = 2;
	    break;
	case -7:
	    base_exp = 5;
	    break;
	case -6:
	    base_exp = 9;
	    break;
	case -5:
	    base_exp = 11;
	    break;
	case -4:
	    base_exp = 22;
	    break;
	case -3:
	    base_exp = 33;
	    break;
	case -2:
	    base_exp = 50;
	    break;
	case -1:
	    base_exp = 66;
	    break;
	case 0:
	    base_exp = 83;
	    break;
	case 1:
	    base_exp = 99;
	    break;
	case 2:
	    base_exp = 121;
	    break;
	case 3:
	    base_exp = 143;
	    break;
	case 4:
	    base_exp = 165;
	    break;
    }

    if (level_range > 4)
	base_exp = 160 + 20 * (level_range - 4);

    xp = base_exp;

    /* more exp at the low levels */
    if (gch->level < 6)
	xp = 10 * xp / (gch->level + 4);

    /* less at high */
    if (gch->level > 35)
	xp = 15 * xp / (gch->level - 25);

    /* reduce for playing time */

    /* randomize the rewards */
    xp = number_range(xp * 3 / 4, xp * 5 / 4);

    /* adjust for grouping */
    xp = xp * gch->level / (UMAX(1, total_levels - 1));

    return xp;
}


void dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, bool immune)
{
    const char *vs;
    const char *vp;
    const char *attack;
    char buf1[256];
    char buf2[256];
    char buf3[256];
    char punct;

    if (ch == NULL)
	return;

    if (dam == 0) {
	vs = "```1miss``";
	vp = "```1misses``";
    } else if (dam <= 4) {
	vs = "```1scratch``";
	vp = "```1scratches``";
    } else if (dam <= 8) {
	vs = "```1graze``";
	vp = "```1grazes``";
    } else if (dam <= 12) {
	vs = "```1hit``";
	vp = "```1hits``";
    } else if (dam <= 16) {
	vs = "```1injure``";
	vp = "```1injures``";
    } else if (dam <= 20) {
	vs = "```1wound``";
	vp = "```1wounds``";
    } else if (dam <= 24) {
	vs = "```1maul``";
	vp = "```1mauls``";
    } else if (dam <= 100) {
	vs = "```1decimate``";
	vp = "```1decimates``";
    } else if (dam <= 200) {
	vs = "```1devastate``";
	vp = "```1devastates``";
    } else if (dam <= 350) {
	vs = "```1maim``";
	vp = "```1maims``";
    } else if (dam <= 600) {
	vs = "```!MUTILATE``";
	vp = "```!MUTILATES``";
    } else if (dam <= 900) {
	vs = "```!DISEMBOWEL``";
	vp = "```!DISEMBOWELS``";
    } else if (dam <= 1500) {
	vs = "```!DISMEMBER``";
	vp = "```!DISMEMBERS``";
    } else if (dam <= 1800) {
	vs = "```!MASSACRE``";
	vp = "```!MASSACRES``";
    } else if (dam <= 2100) {
	vs = "```!MANGLE``";
	vp = "```!MANGLES``";
    } else if (dam <= 2500) {
	vs = "```1*** ```!DEMOLISH ```1***``";
	vp = "```1*** ```!DEMOLISHES ```1***``";
    } else if (dam <= 2900) {
	vs = "```1*** ```!DEVASTATE ```1***``";
	vp = "```1*** ```!DEVASTATES ```1***``";
    } else if (dam <= 3200) {
	vs = "```1=== ```!OBLITERATE ```1===``";
	vp = "```1=== ```!OBLITERATES ```1===``";
    } else if (dam <= 3300) {
	vs = "```1>>> ```!ANNIHILATE ```1<<<``";
	vp = "```1>>> ```!ANNIHILATES ```1<<<``";
    } else if (dam <= 3600) {
	vs = "```1<<< ```!ERADICATE ```1>>>``";
	vp = "```1<<< ```!ERADICATES ```1>>>``";
    } else if (dam <= 3900) {
	vs = "`1!!! S`!L`&A`1UGHT`&E`!R`1S !!!``";
	vp = "`1!!! S`!L`&A`1UGHT`&E`!R`1S !!!``";
    } else if (dam <= 4200) {
	vs = "`&-`#-`3-`!T``O`&as``t`!S`3-`#-`&-``";
	vp = "`&-`#-`3-`!T``O`&as``t`!S`3-`#-`&-``";
    } else if (dam <= 4500) {
	vs = "`1-`!(`&*`!)`1DESOLATES`!(`&*`!)`1-``";
	vp = "`1-`!(`&*`!)`1DESOLATES`!(`&*`!)`1-``";
    } else if (dam <= 4800) {
	vs = "`4STOMPS ``A `3MUDHOLE ``in";
	vp = "`4STOMPS ``A `3MUDHOLE ``in";
    } else if (dam <= 5100) {
	vs = "`^P`6A`8S`7T`&E`1S``";
	vp = "`^P`6A`8S`7T`&E`1S``";
    } else if (dam <= 5400) {
	vs = "`8-`!=`5]`1 MORTIFIES `5[`!=`8-``";
	vp = "`8-`!=`5]`1 MORTIFIES `5[`!=`8-``";
    } else if (dam <= 5600) {
	vs = "``)`PO`5(CIRCUMCISES)`PO``(``";
	vp = "``)`PO`5(CIRCUMCISES)`PO``(``";
    } else if (dam <= 5900) {
	vs = "`!ANALLY `#PROSECUTES``";
	vp = "`!ANALLY `#PROSECUTES``";
    } else if (dam <= 6200) {
	vs = "`1<`!*`1> `!DESTROYS ```1<`!*`1>``";
	vp = "`1<`!*`1> `!DESTROYS ```1<`!*`1>``";
    } else if (dam <= 6400) {
	vs = "`4### `@NEUTRALIZES ```4###``";
	vp = "`4### `@NEUTRALIZES ```4###``";
    } else if (dam <= 6600) {
	vs = "`&-*> ```#DISINTEGRATES ```&<*-``";
	vp = "`&-*> ```#DISINTEGRATES ```&<*-``";
    } else if (dam <= 6900) {
	vs = "`O<`P<`&+`6*`&+`P>`O> `2DISCORPERATES `O<`P<`&+`6*`&+`P>`O>``";
	vp = "`O<`P<`&+`6*`&+`P>`O> `2DISCORPERATES `O<`P<`&+`6*`&+`P>`O>``";
    } else if (dam <= 7200) {
	vs = "`6<`2<`&*`6%`&*`2>`6> `PEXCRUCIATES `6<`2<`&*`6%`&*`2>`6>``";
	vp = "`6<`2<`&*`6%`&*`2>`6> `PEXCRUCIATES `6<`2<`&*`6%`&*`2>`6>``";
    } else if (dam <= 7500) {
	vs = "`8<`6\\`2\\/`6/`8> `!ST`1O`!MPS `1A `!M`1U`!DH`1O`!LE `8<`6\\`2\\/`6/`8>``";
	vp = "`8<`6\\`2\\/`6/`8> `!ST`1O`!MPS `1A `!M`1U`!DH`1O`!LE `8<`6\\`2\\/`6/`8>``";
    } else {
	vs = "inflict ```#UNSPEAKABLE ```!PAIN ``to";
	vp = "inflicts ```#UNSPEAKABLE ```!PAIN ``to";
    }

    punct = (dam <= 24) ? '.' : '!';
    if (dt == TYPE_HIT) {
	if (victim == NULL) {
	    sprintf(buf1, "Something %s $n %c `8(```4%d```8)``", vp, punct, dam);
	    sprintf(buf2, "Something %s you%c `8(```4%d```8)``", vs, punct, dam);
	} else if (ch == victim) {
	    sprintf(buf1, "$n %s $melf %c `8(```4%d```8)``", vp, punct, dam);
	    sprintf(buf2, "You %s yourself%c `8(```4%d```8)``", vs, punct, dam);
	} else {
	    sprintf(buf1, "$n %s $N%c `8(```4%d```8)``", vp, punct, dam);
	    sprintf(buf2, "`aYou`` %s `a$N%c `8(```4%d```8)``", vs, punct, dam);
	    sprintf(buf3, "`A$n`` %s `Ayou%c `8(```4%d```8)``", vp, punct, dam);
	}
    } else {
	SKILL *skill;

	if ((skill = resolve_skill_sn(dt)) != NULL) {
	    attack = skill->dam_noun;
	} else if (dt >= TYPE_HIT
		&& dt <= TYPE_HIT + MAX_DAMAGE_MESSAGE) {
	    attack = attack_table[dt - TYPE_HIT].noun;
	} else {
	    log_bug("Dam_message: bad dt %d.", dt);
	    dt = TYPE_HIT;
	    attack = attack_table[0].name;
	}

	if (immune) {
	    if (victim == NULL) {
		sprintf(buf1, "$n is unaffected by the %s.", attack);
		sprintf(buf2, "You are immune to the %s.", attack);
	    } else if (ch == victim) {
		sprintf(buf1, "$n is unaffected by $s own %s.", attack);
		sprintf(buf2, "Luckily, you are immune to that.");
	    } else {
		sprintf(buf1, "$N is unaffected by $n's %s!", attack);
		sprintf(buf2, "$N is unaffected by your %s!", attack);
		sprintf(buf3, "$n's %s is powerless against you.", attack);
	    }
	} else {
	    if (victim == NULL) {
		sprintf(buf1, "The %s %s $m%c `8(```4%d```8)``", attack, vp, punct, dam);
		sprintf(buf2, "The %s %s you%c `8(```4%d```8)``", attack, vp, punct, dam);
	    } else if (ch == victim) {
		sprintf(buf1, "$n's %s %s $m%c `8(```4%d```8)``", attack, vp, punct, dam);
		sprintf(buf2, "Your %s %s you%c `8(```4%d```8)``", attack, vp, punct, dam);
	    } else {
		sprintf(buf1, "$n's %s %s $N%c `8(```4%d```8)``", attack, vp, punct, dam);
		sprintf(buf2, "`aYour %s`` %s `a$N%c `8(```4%d```8)``", attack, vp, punct, dam);
		sprintf(buf3, "`A$n's %s`` %s `Ayou%c `8(```4%d```8)``", attack, vp, punct, dam);
	    }
	}
    }

    if (victim == NULL) {
	act(buf1, ch, NULL, NULL, TO_ROOM);
	act(buf2, ch, NULL, NULL, TO_CHAR);
    } else if (ch == victim) {
	act(buf1, ch, NULL, NULL, TO_ROOM);
	act(buf2, ch, NULL, NULL, TO_CHAR);
    } else {
	act(buf1, ch, NULL, victim, TO_NOTVICT);
	act(buf2, ch, NULL, victim, TO_CHAR);
	act(buf3, ch, NULL, victim, TO_VICT);
    }

    return;
}





/***************************************************************************
 *	max_damage
 *
 *	set an upper limit on the amount of damage
 *	a pc-vs-pc attack can do
 ***************************************************************************/
int max_damage(CHAR_DATA *ch, CHAR_DATA *victim, int dt, int dam)
{
    int value = 0;
    int mod;
    int cap;
    int iter;


    /*
     * max_damage is usually level * 4 except for some exceptions
     * which are listed in the table below
     *
     * 0 means to ignore the max_damage rule
     */
    const struct modifier {
	SKILL * gsp;
	int	mod;
    } dam_modifier[] =
    {
	{ NULL, 0 }
    };


    if (!IS_NPC(ch) && !IS_NPC(victim)) {
	mod = 5;
	for (iter = 0; dam_modifier[iter].gsp != NULL; iter++) {
	    if (dam_modifier[iter].gsp->sn == dt) {
		mod = dam_modifier[iter].mod;
		break;
	    }
	}

	/* mod should be greater than 0 here if we should apply the rule */
	if (mod > 0) {
	    cap = ch->level * mod;
	    cap += get_curr_stat(ch, STAT_STR) / 2;
	    value = (cap >= dam) ? dam : cap;
	    if (value > 1) {
		if (!str_cmp(class_table[ch->class].name, "warrior")
			|| !str_cmp(class_table[ch->class].name, "thief"))
		    value += value / 4;

		if (!str_cmp(class_table[victim->class].name, "warrior")
			|| !str_cmp(class_table[victim->class].name, "thief"))
		    value = (value * 3) / 4;
	    }

	    return value;
	}
    }

    return dam;
}


/***************************************************************************
 *	disarm
 *	disarm a character - caller is responsible for success or failure
 ***************************************************************************/
void disarm(CHAR_DATA *ch, CHAR_DATA *victim)
{
    GAMEOBJECT *obj;

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == NULL)
	return;

    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
	act("$S weapon won't budge!", ch, NULL, victim, TO_CHAR);
	act("$n tries to disarm you, but your weapon won't budge!",
		ch, NULL, victim, TO_VICT);
	act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    act("$n DISARMS you and sends your weapon flying!", ch, NULL, victim, TO_VICT);
    act("You `PDISARMS`` $N!", ch, NULL, victim, TO_CHAR);
    act("$n `PDISARMS`` $N!", ch, NULL, victim, TO_NOTVICT);

    obj_from_char(obj);
    if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
	obj_to_char(obj, victim);
    } else {
	obj_to_room(obj, victim->in_room);
	if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, obj))
	    get_obj(victim, obj, NULL);
    }

    return;
}


/***************************************************************************
 *	check_deathdrop
 *
 *	check to see if the character is wearing anything
 *	that is flaged deathdrop - if they are, remove it
 *	from their inventory and put it in the room
 ***************************************************************************/
void check_deathdrop(CHAR_DATA *ch)
{
    GAMEOBJECT *obj;
    GAMEOBJECT *obj_next;

    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
	obj_next = obj->next_content;
	if (IS_SET(obj->extra_flags, ITEM_DEATH_DROP)) {
	    obj_from_char(obj);
	    obj_to_room(obj, ch->in_room);

	    act("$p flies out of $n's hands.", ch, obj, NULL, TO_ROOM);
	    act("$p flies from you hands.", ch, obj, NULL, TO_CHAR);
	}
    }
}



void use_magical_item(CHAR_DATA *ch)
{
    GAMEOBJECT *obj;
    GAMEOBJECT *cobj = NULL;
    int number = 0;
    char buf[MIL];

    for (obj = ch->carrying; obj; obj = obj->next_content) {
	if ((obj->item_type == ITEM_SCROLL
		    || obj->item_type == ITEM_WAND
		    || obj->item_type == ITEM_STAFF
		    || obj->item_type == ITEM_PILL)
		&& number_range(0, number) == 0) {
	    cobj = obj;
	    number++;
	}
    }

    if (!cobj)
	return;

    switch (cobj->item_type) {
	case ITEM_SCROLL:
	    do_recite(ch, "scroll");
	    break;
	case ITEM_WAND:
	    if (cobj->wear_loc == WEAR_HOLD)
		do_zap(ch, "");
	    break;
	case ITEM_STAFF:
	    if (cobj->wear_loc == WEAR_HOLD)
		do_brandish(ch, "");
	    break;
	case ITEM_POTION:
	    do_quaff(ch, "potion");
	    break;
	case ITEM_PILL:
	    sprintf(buf, "%s", cobj->name);
	    do_eat(ch, buf);
	    break;
    }
    return;
}
