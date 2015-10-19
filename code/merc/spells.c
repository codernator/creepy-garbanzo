#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "object.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"


extern SKILL *gsp_bless;
extern SKILL *gsp_blindness;
extern SKILL *gsp_burning_flames;
extern SKILL *gsp_curse;
extern SKILL *gsp_faerie_fog;
extern SKILL *gsp_frenzy;
extern SKILL *gsp_gate;
extern SKILL *gsp_haste;
extern SKILL *gsp_invisibility;
extern SKILL *gsp_mass_invisibility;
extern SKILL *gsp_nexus;
extern SKILL *gsp_poison;
extern SKILL *gsp_portal;
extern SKILL *gsp_sneak;



void spell_acid_blast(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;

    dam = dice(level * 2, 5);
    if (saves_spell(level, victim, DAM_ACID))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_ACID, true);
    return;
}

void spell_armor(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("``You are already `&(``armored`&)``.\n\r", ch);
	else
	    act("$N ``is already `&(``armored`&)``.", ch, NULL, victim, TO_CHAR);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = 24;
    af.modifier = (level * -2) / 3;
    af.location = APPLY_AC;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    send_to_char("``A `@m`2a`7g`&i`7c`2a`@l`` aura surrounds you.\n\r", victim);

    if (ch != victim)
	act("$N is protected by your `@m`2a`&g`2i`@c``.", ch, NULL, victim, TO_CHAR);
    return;
}

void spell_blindness(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_BLIND)
	    || saves_spell(level, victim, DAM_OTHER)
	    || is_affected(victim, skill)) {
	send_to_char("They are unaffected by your attempt to blind them.\n\r", ch);
	return;
    }


    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.location = APPLY_HITROLL;
    af.modifier = -1 * level;
    af.duration = 1 + level;
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, &af);

    send_to_char("``You are `8bli`7n`8ded``!  `1O`!h `1S`!h`1i`!t``!\n\r", victim);
    act("$n is `8bli`7n`8ded``.", victim, NULL, NULL, TO_ROOM);
    return;
}



void spell_burning_hands(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    static const int dam_each[] =
    {
	0,
	0, 0,	0,  0,	14, 17, 20, 23, 26, 29,
	29,29,	30, 30, 31, 31, 32, 32, 33, 33,
	34,34,	35, 35, 36, 36, 37, 37, 38, 38,
	39,39,	40, 40, 41, 41, 42, 42, 43, 43,
	44,44,	45, 45, 46, 46, 47, 47, 48, 48
    };

    int dam;

    level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);

    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);
    if (saves_spell(level, victim, DAM_FIRE))
	dam /= 2;
    damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
    return;
}



void spell_call_lightning(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;
    int dam;


    if (!IS_OUTSIDE(ch)) {
	send_to_char("``You must be `3o`@ut`2do`@or`3s``.\n\r", ch);
	return;
    }

    if (globalGameState.weather->sky < SKY_RAINING) {
	send_to_char("``You need bad `Ow`^e`6ath`^e`Or``.\n\r", ch);
	return;
    }

    dam = dice(level / 2, 8);

    send_to_char("`@M`2o`@t`2a``'s lightning strikes your foes!\n\r", ch);
    act("$n calls `@M`2o`@t`2a``'s lightning to strike $s foes! `1BLAM``", ch, NULL, NULL, TO_ROOM);

    for (vch = char_list; vch != NULL; vch = vch_next) {
	vch_next = vch->next;
	if (vch->in_room == NULL)
	    continue;
	if (vch->in_room == ch->in_room) {
	    if (vch != ch && (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))) {
		damage(ch, vch, saves_spell(level, vch, DAM_LIGHTNING) ? dam / 2 : dam, skill->sn, DAM_LIGHTNING, true);
	    }
	    continue;
	}

	if (vch->in_room->area == ch->in_room->area && IS_OUTSIDE(vch) && IS_AWAKE(vch))
	    send_to_char("`&Lightning ```#f`&l`#a`&s`#h`&e`#s`` in the sky.\n\r", vch);
    }
}

/* RT calm spell stops all fighting in the room */
void spell_calm(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *vch;
    AFFECT_DATA af;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;
    int chance;

    /* get sum of all mobile levels in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
	if (vch->position == POS_FIGHTING) {
	    count++;
	    if (IS_NPC(vch))
		mlevel += vch->level;
	    else
		mlevel += vch->level / 2;
	    high_level = UMAX(high_level, vch->level);
	}
    }

    /* compute chance of stopping combat */
    chance = 4 * level - high_level + 2 * count;

    if (IS_IMMORTAL(ch)) /* always works */
	mlevel = 0;

    if (number_range(0, chance) >= mlevel) { /* hard to stop large fights */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
	    if (IS_NPC(vch) && (IS_SET(vch->imm_flags, IMM_MAGIC)
			|| IS_SET(vch->act, ACT_UNDEAD)))
		return;

	    if (IS_AFFECTED(vch, AFF_CALM) || IS_AFFECTED(vch, AFF_BERSERK)
		    || is_affected(vch, gsp_frenzy))
		return;

	    send_to_char("``A wave of `^ca`6lm`` passes over you.\n\r", vch);
	    if (vch->fighting || vch->position == POS_FIGHTING)
		stop_fighting(vch, false);

	    af.where = TO_AFFECTS;
	    af.type = skill->sn;
	    af.skill = skill;
	    af.level = level;
	    af.duration = level / 4;
	    af.location = APPLY_HITROLL;
	    if (!IS_NPC(vch))
		af.modifier = -5;
	    else
		af.modifier = -2;

	    af.bitvector = AFF_CALM;
	    affect_to_char(vch, &af);

	    af.location = APPLY_DAMROLL;
	    affect_to_char(vch, &af);
	}
    }
}


void spell_cause_light(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    damage(ch, (struct char_data *)vo, dice(1, 8) * level / 30, skill->sn, DAM_HARM, true);
    return;
}



void spell_cause_critical(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    damage(ch, (struct char_data *)vo, dice(30, 3) * level / 10, skill->sn, DAM_HARM, true);
    return;
}



void spell_cause_serious(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    damage(ch, (struct char_data *)vo, dice(2, 25) * level / 20, skill->sn, DAM_HARM, true);
    return;
}

void spell_meteor_storm(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *tmp_vict;
    struct char_data *last_vict;
    struct char_data *next_vict;
    bool found;
    int dam;

    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE)) {
	send_to_char("`8Not in this room``. <`PSLAPS HAND``>\n\r", ch);
	return;
    }

    act("$n summons a `8m`7e`&t`7e`8o`7r `&s`#t`!o`1rm`` to strike $N.", ch, NULL, victim, TO_ROOM);
    act("$n summons a `8m`7e`&t`7e`8o`7r 1&s`#t`!o`1rm`` which strikes you!", ch, NULL, victim, TO_VICT);

    dam = dice(level, 30);
    if (saves_spell(level, victim, DAM_ACID))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_ACID, true);
    last_vict = victim;
    level -= 30;    /* decrement damage */

    /* new targets */
    while (level > 0) {
	found = false;
	for (tmp_vict = ch->in_room->people;
		tmp_vict != NULL;
		tmp_vict = next_vict) {
	    next_vict = tmp_vict->next_in_room;

	    if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict) {
		if (is_same_group(tmp_vict, ch))
		    continue;

		found = true;
		last_vict = tmp_vict;
		act("A meteor `!strikes`` $n!", tmp_vict, NULL, NULL, TO_ROOM);
		act("A meteor `!hits`` you!", tmp_vict, NULL, NULL, TO_CHAR);
		dam = dice(level, 20);

		if (saves_spell(level, tmp_vict, DAM_ACID))
		    dam /= 3;

		damage(ch, tmp_vict, dam, skill->sn, DAM_ACID, true);
		level -= 30;    /* decrement damage */
	    }
	} /* end target searching loop */

	if (!found) { /* no target found, hit the caster */
	    if (ch == NULL)
		return;

	    if (last_vict == ch) {   /* no double hits */
		act("The `!storm`` subsides.", ch, NULL, NULL, TO_ROOM);
		act("The `!storm`` subsides.", ch, NULL, NULL, TO_CHAR);
		return;
	    }

	    last_vict = ch;
	    act("A meteor `!strikes`` $n!", ch, NULL, NULL, TO_ROOM);
	    send_to_char("A meteor `!strikes`` you!\n\r", ch);
	    dam = dice(level * 3, 15);

	    if (saves_spell(level, ch, DAM_ACID))
		dam /= 3;
	    damage(ch, ch, dam, skill->sn, DAM_ACID, true);
	    level -= 30;    /* decrement damage */
	    if (ch == NULL)
		return;
	}
	/* now go back and find more targets */
    }
}

void spell_chain_lightning(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *tmp_vict;
    struct char_data *last_vict;
    struct char_data *next_vict;
    bool found;
    int dam;


    act("A `Olightning`` bolt leaps from $n's hand and arcs to $N.", ch, NULL, victim, TO_ROOM);
    act("A `&lightning`` bolt leaps from your hand and arcs to $N.", ch, NULL, victim, TO_CHAR);
    act("A `^lightning`` bolt leaps from $n's hand and hits you!", ch, NULL, victim, TO_VICT);

    dam = dice(level, 6);
    if (saves_spell(level, victim, DAM_LIGHTNING))
	dam /= 3;

    damage(ch, victim, dam, skill->sn, DAM_LIGHTNING, true);
    last_vict = victim;
    level -= 20;    /* decrement damage */

    /* new targets */
    while (level > 0) {
	found = false;
	for (tmp_vict = ch->in_room->people;
		tmp_vict != NULL;
		tmp_vict = next_vict) {
	    next_vict = tmp_vict->next_in_room;
	    if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict) {
		found = true;
		last_vict = tmp_vict;
		if (!is_same_group(tmp_vict, ch)) {
		    act("The bolt `!arcs`` to $n!", tmp_vict, NULL, NULL, TO_ROOM);
		    act("The bolt `!hits`` you!", tmp_vict, NULL, NULL, TO_CHAR);
		    dam = dice(level, 6);
		    if (saves_spell(level, tmp_vict, DAM_LIGHTNING))
			dam /= 3;
		    damage(ch, tmp_vict, dam, skill->sn, DAM_LIGHTNING, true);
		}

		level -= 20;    /* decrement damage */
	    }
	} /* end target searching loop */

	if (!found) {   /* no target found, hit the caster */
	    if (ch == NULL)
		return;

	    if (last_vict == ch) {   /* no double hits */
		act("The bolt seems to have `!f`1i`&z`7z`8led`` out.", ch, NULL, NULL, TO_ROOM);
		act("The bolt `3g`&r`#oun`&d`3s`` out through your body.", ch, NULL, NULL, TO_CHAR);
		return;
	    }

	    last_vict = ch;
	    act("The bolt `!arcs`` to $n...`1SHIT``!", ch, NULL, NULL, TO_ROOM);
	    send_to_char("``You are `!struck`` by your own lightning!\n\r", ch);
	    dam = dice(level, 6);
	    if (saves_spell(level, ch, DAM_LIGHTNING))
		dam /= 3;
	    damage(ch, ch, dam, skill->sn, DAM_LIGHTNING, true);
	    level -= 21;    /* decrement damage */
	    if (ch == NULL)
		return;
	}
    }
}

void spell_charm_person(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_safe(ch, victim))
	return;

    if (victim->level < ch->level - 10
	    && !IS_NPC(victim)) {
	send_to_char("Pick on someone your own size .. coward..\n\r", ch);
	return;
    }

    if (victim == ch) {
	send_to_char("You like yourself even better!  You feel compelled to strip naked in front of everyone and molest yourself, but you don't.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM)
	    || IS_AFFECTED(ch, AFF_CHARM)
	    || IS_SET(victim->imm_flags, IMM_CHARM)
	    || saves_spell(level, victim, DAM_CHARM)) {
	send_to_char("Your efforts have been to no avail.\n\r", ch);
	return;
    }

    if (victim->master)
	stop_follower(victim);

    add_follower(victim, ch);
    victim->leader = ch;

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("``Isn't $n just so `pnice``?`1 :)``", ch, NULL, victim, TO_VICT);
    if (ch != victim)
	act("$N looks at you with absolute adoration swarming $s face.", ch, NULL, victim, TO_CHAR);

    log_string("%s has charmed %s", ch->name, victim->name);
    return;
}



void spell_chill_touch(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    int dam;
    static const int dam_each[] =
    {
	0,  0,	0,  6,	7,  8,	9,  12, 13, 13, 13,
	14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
	17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
	20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
	24, 24, 24, 25, 25, 25, 26, 26, 26, 27
    };


    level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] * 15, dam_each[level] * 35);


    if (!saves_spell(level, victim, DAM_COLD)) {
	act("$n turns `^b`&lu`Oe`` and begins to solidify.", victim, NULL, NULL, TO_ROOM);
	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = 6;
	af.location = APPLY_STR;
	af.modifier = -1;
	af.bitvector = 0;
	affect_join(victim, &af);
    } else {
	dam /= 2;
    }

    damage(ch, victim, dam, skill->sn, DAM_COLD, true);
    return;
}



void spell_colour_spray(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    static const int dam_each[] =
    {
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,
	30, 35, 40, 45, 50, 55, 55, 55, 56, 57,
	58, 58, 59, 60, 61, 61, 62, 63, 64, 64,
	65, 66, 67, 67, 68, 69, 70, 70, 71, 72,
	73, 73, 74, 75, 76, 76, 77, 78, 79, 79
    };

    int dam;


    level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] * 10, dam_each[level] * 25);
    if (saves_spell(level, victim, DAM_LIGHT)) {
	dam /= 2;
    } else {
	if (gsp_blindness != NULL)
	    cast_spell(ch, gsp_blindness, level, (void *)victim, TARGET_CHAR, "");
    }

    damage(ch, victim, dam, skill->sn, DAM_LIGHT, true);
    return;
}


void spell_control_weather(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    if (!str_cmp(argument, "better"))
	globalGameState.weather->change += dice(level / 3, 4);
    else if (!str_cmp(argument, "worse"))
	globalGameState.weather->change -= dice(level / 3, 4);
    else
	send_to_char("Do you want it to get `!better ``or `8worse``?\n\r", ch);

    send_to_char("`1O`!k``.\n\r", ch);
    return;
}

void spell_create_water(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;
    long water;


    if (obj->wear_loc != -1) {
	send_to_char("The item `1*`!must```1*`` be in your inventory.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_DRINK_CON) {
	send_to_char("It is unable to hold `Ow`^a`&t`^e`Or``.\n\r", ch);
	return;
    }

    if (obj->value[2] != LIQ_WATER && obj->value[1] != 0) {
	send_to_char("It contains some other `!l`@i`#q`Pu`Oi`&d``.\n\r", ch);
	return;
    }

    water = (long)(level * (globalGameState.weather->sky >= SKY_RAINING ? 4 : 2));
    water = UMIN(water, obj->value[0] - obj->value[1]);

    if (water > 0) {
	obj->value[2] = LIQ_WATER;
	obj->value[1] += water;
	act("$p is `&f`^i`6l`^l`Oe`4d``.", ch, obj, NULL, TO_CHAR);
    }
}


/***************************************************************************
 *	spell_cure_blindness
 ***************************************************************************/
void spell_cure_blindness(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;

    if (gsp_blindness == NULL)
	return;

    if (!is_affected(victim, gsp_blindness)) {
	if (victim == ch)
	    send_to_char("You aren't `8b`7lin`8d``.\n\r", ch);
	else
	    act("$N doesn't appear to be `8b`7linde`8d``.", ch, NULL, victim, TO_CHAR);
	return;
    }


    if (check_dispel(level, victim, gsp_blindness)) {
	send_to_char("Your `&v`Oi`^s`6i`4o`8n`` returns!\n\r", victim);
	act("$n is no longer `8bli`&n`8ded``.", victim, NULL, NULL, TO_ROOM);
	do_look(victim, "auto");
    } else {
	send_to_char("`1S`!pell `!f`1ailed``.\n\r", ch);
    }
}


/***************************************************************************
 *	spell_cure_critical
 ***************************************************************************/
void spell_cure_critical(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int heal;

    heal = dice(3, 8) + level - 6;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("You feel `#b`@e`#t`@t`#e`@r``!\n\r", victim);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);
    return;
}

void spell_cure_light(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int heal;

    heal = dice(1, 8) + level / 3;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("``You feel `#b`@e`#t`2t`#e`@r``!\n\r", victim);

    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    return;
}



void spell_cure_poison(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    SKILL *skill_poison;

    skill_poison = gsp_poison;
    if (!is_affected(victim, skill_poison)) {
	if (victim == ch)
	    send_to_char("You aren't `Pp`5ois`Pon`5ed``.\n\r", ch);
	else
	    act("$N doesn't appear to be `Pp`5ois`Pon`5ed``.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (check_dispel(level, victim, skill_poison)) {
	send_to_char("A `!w`1ar`!m`` feeling runs through your body.\n\r", victim);
	act("$n looks much `!better``.", victim, NULL, NULL, TO_ROOM);
    } else {
	send_to_char("`!S`!pell `1f`!ailed``.\n\r", ch);
    }
}

void spell_cure_serious(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int heal;

    heal = dice(2, 8) + level / 2;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);

    send_to_char("You feel `@b`#e`2t`#t`2e`#r``!\n\r", victim);

    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    return;
}

void spell_darkness(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    AFFECT_DATA af;

    if (ch->class != class_lookup("witch")) {
	send_to_char("Yeah right.  Get a life dipshit.\n\r", ch);
	return;
    }

    if (is_affected(ch, skill)) {
	send_to_char("You cannot get any `8D`7a`8rk`7e`8r``.\n\r", ch);
	return;
    }

    act("$n `8f`7a`&d`7e`8s`` into the `8sh`7a`8d`7o`8ws``.", ch, NULL, NULL, TO_ROOM);
    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level + 12;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You `8f`7a`&de`` into the `8sh`7a`8d`7o`8ws``.\n\r", ch);
    return;
}


/* RT replacement demonfire spell */
void spell_demonfire(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;

    if (victim != ch) {
	act("$n calls forth the `1d`!e`1m`!o`1n`!s`` of `8H`&e`1l`!l`` upon $N!",
		ch, NULL, victim, TO_ROOM);
	act("$n has assailed you with the `1d`!e`1m`!o`1n`!s`` of `8H`&e`1l`!l``!",
		ch, NULL, victim, TO_VICT);
	send_to_char("You conjure forth the `1d`!e`1m`!o`1n`!s`` of `8H`&el`1l`!!``\n\r", ch);
    }

    if (gsp_curse != NULL && gsp_curse->spells != NULL)
	cast_spell(ch, gsp_curse, (3 * level / 4), victim, TARGET_CHAR, argument);

    dam = dice(level, 20);
    if (saves_spell(level, victim, DAM_NEGATIVE))
	dam /= 2;
    damage(ch, victim, dam, skill->sn, DAM_NEGATIVE, true);
}

void spell_detect_invis(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_INVIS)) {
	if (victim == ch)
	    send_to_char("You can already see `8invisibl`&e``.\n\r", ch);
	else
	    act("$N can already see `8invisibl`&e`` things.", ch, NULL, victim, TO_CHAR);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char(victim, &af);

    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    else
	send_to_char("Your eyes `8tingle``.\n\r", victim);
    return;
}



void spell_detect_magic(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_MAGIC)) {
	if (victim == ch)
	    send_to_char("You can already sense magical `!auras``.\n\r", ch);
	else
	    act("$N can already detect `!magic``.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char(victim, &af);

    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    else
	send_to_char("Your eyes `#tingle``.\n\r", victim);

    return;
}



void spell_detect_poison(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;

    if (obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD) {
	if (obj->value[3] != 0)
	    send_to_char("You smell poisonous fumes.\n\r", ch);
	else
	    send_to_char("It looks delicious.\n\r", ch);
    } else {
	send_to_char("It doesn't look poisoned.\n\r", ch);
    }

    return;
}

void spell_dispel_magic(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    SKILL *skill_idx;
    bool found = false;

    level += level / 10;
    if (number_range(1, 10) == 1)
	level += level / 10;

    if (saves_spell(level, victim, DAM_OTHER)) {
	send_to_char("You feel a brief `8t`7i`&ngli`7n`8g`` sensation.\n\r", victim);
	send_to_char("You `1failed``.\n\r", ch);
	return;
    }

    for (skill_idx = skill_list; skill_idx != NULL; skill_idx = skill_idx->next) {
	if (IS_SET(skill_idx->flags, SPELL_DISPELLABLE)) {
	    if (check_dispel(level, victim, skill_idx))
		found = true;
	}
    }

    if (found)
	send_to_char("`1O`!k``.\n\r", ch);
    else
	send_to_char("`1S`!pell `1f`!ailed``.\n\r", ch);

    return;
}


/***************************************************************************
 *	spell_cancellation
 *
 *	deffensive affect strip of negative affects
 ***************************************************************************/
void spell_cancellation(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    SKILL *skill_idx;
    bool found = false;

    level += level / 10;
    if (number_range(1, 10) == 1)
	level += level / 10;

    if ((IS_NPC(ch) && !IS_NPC(victim))) {
	send_to_char("You `8failed``, try dispel magic.\n\r", ch);
	return;
    }


    for (skill_idx = skill_list; skill_idx != NULL; skill_idx = skill_idx->next) {
	if (IS_SET(skill_idx->flags, SPELL_CANCELABLE)) {
	    if (check_dispel(level, victim, skill_idx))
		found = true;
	}
    }

    if (found)
	send_to_char("`1O`!k``.\n\r", ch);
    else
	send_to_char("`!Spell `!failed``.\n\r", ch);
}



/***************************************************************************
 *	spell_earthquake
 ***************************************************************************/
void spell_earthquake(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;


    send_to_char("The `3e`7a`8r`7t`3h`` trembles beneath your feet!\n\r", ch);
    act("$n makes the `3e`7a`8r`7t`3h`` tremble and shiver.", ch, NULL, NULL, TO_ROOM);

    for (vch = char_list; vch != NULL; vch = vch_next) {
	vch_next = vch->next;
	if (vch->in_room == NULL)
	    continue;

	if (vch->in_room == ch->in_room) {
	    if (vch != ch && !is_safe_spell(ch, vch, true)) {
		if (IS_AFFECTED(vch, AFF_FLYING)) {
		    continue;
		} else {
		    if (is_same_group(vch, ch))
			continue;
		    damage(ch, vch, level + dice(10, 50), skill->sn, DAM_BASH, true);
		}
	    }
	    continue;
	}

	if (vch->in_room->area == ch->in_room->area)
	    send_to_char("The `3e`7a`8r`7t`3h`` trembles and shivers.\n\r", vch);
    }

    return;
}

void spell_enchant_armor(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;
    AFFECT_DATA *paf;
    int result;
    int fail;
    int ac_bonus;
    int added;
    bool ac_found = false;

    if (obj->item_type != ITEM_ARMOR) {
	send_to_char("That isn't an `1a`!r`1m`!o`1r``.\n\r", ch);
	return;
    }

    if (obj->wear_loc != -1) {
	send_to_char("The item `1*`!must`1* be carried to be enchanted.\n\r", ch);
	return;
    }

    /* this means they have no bonus */
    ac_bonus = 0;
    fail = 10;              /* base 10% chance of failure */

    /* find the bonuses */
    if (!obj->enchanted) {
	for (paf = obj->objprototype->affected; paf != NULL; paf = paf->next) {
	    if (paf->location == APPLY_AC) {
		ac_bonus = (int)paf->modifier;
		ac_found = true;
		if (ac_bonus != 0)
		    fail += (ac_bonus > 0) ? (ac_bonus * 3) : (ac_bonus * -3);
		/* 5 *(ac_bonus * ac_bonus); */
	    } else { /* things get a little harder */
		fail += 7;
	    }
	}
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next) {
	if (paf->location == APPLY_AC) {
	    ac_bonus = (int)paf->modifier;
	    ac_found = true;
	    if (ac_bonus != 0)
		fail += (ac_bonus > 0) ? (ac_bonus * 3) : (ac_bonus * -3);
	    /* 5 *(ac_bonus * ac_bonus); */
	} else { /* things get a little harder */
	    fail += 7;
	}
    }

    /* apply other modifiers */
    fail -= level;

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
	fail -= 35;

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
	fail -= 15;

    fail = URANGE(5, fail, 85);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5)) { /* item destroyed */
	act("$p flares `1bli`#ndin`1gly``... and evaporates!", ch, obj, NULL, TO_CHAR);
	act("$p flares `1bli`#ndin`1gly``... and evaporates!", ch, obj, NULL, TO_ROOM);
	extract_obj(obj);
	return;
    }

    if (result < (fail / 3)) { /* item disenchanted */
	AFFECT_DATA *paf_next;

	act("$p glows `1bri`#ght`1ly``, then fades...`1FUCK``!.", ch, obj, NULL, TO_CHAR);
	act("$p glows `1bri`#ght`1ly``, then fades.", ch, obj, NULL, TO_ROOM);
	obj->enchanted = true;

	/* remove all affects */
	for (paf = obj->affected; paf != NULL; paf = paf_next) {
	    paf_next = paf->next;
	    free_affect(paf);
	}
	obj->affected = NULL;

	/* clear all flags */
	obj->extra_flags = 0;
	return;
    }

    if (result <= fail) {   /* failed, no bad result */
	send_to_char("Nothing seemed to `3happen``.\n\r", ch);
	return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted) {
	AFFECT_DATA *af_new;

	obj->enchanted = true;

	for (paf = obj->objprototype->affected; paf != NULL; paf = paf->next) {
	    af_new = new_affect();

	    af_new->next = obj->affected;
	    obj->affected = af_new;

	    af_new->where = paf->where;
	    af_new->type = UMAX(0, paf->type);
	    af_new->level = paf->level;
	    af_new->duration = paf->duration;
	    af_new->location = paf->location;
	    af_new->modifier = paf->modifier;
	    af_new->bitvector = paf->bitvector;
	}
    }

    if (result <= (90 - level / 5)) { /* success! */
	act("$p shimmers with a gold aura.", ch, obj, NULL, TO_CHAR);
	act("$p shimmers with a gold aura.", ch, obj, NULL, TO_ROOM);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	added = -10;
    } else {
	/* exceptional enchant */
	act("$p glows a brillant gold!", ch, obj, NULL, TO_CHAR);
	act("$p glows a brillant gold!", ch, obj, NULL, TO_ROOM);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	SET_BIT(obj->extra_flags, ITEM_GLOW);
	added = -50;
    }

    /* now add the enchantments */
    if (obj->level < LEVEL_HERO)
	obj->level = UMIN(LEVEL_HERO - 1, obj->level + 1);

    if (ac_found) {
	for (paf = obj->affected; paf != NULL; paf = paf->next) {
	    if (paf->location == APPLY_AC) {
		paf->type = skill->sn;
		paf->modifier += added;
		paf->level = UMAX(paf->level, level);
	    }
	}
    } else {
	paf = new_affect();
	paf->where = TO_OBJECT;
	paf->type = skill->sn;
	paf->level = level;
	paf->duration = -1;
	paf->location = APPLY_AC;
	paf->modifier = added;
	paf->bitvector = 0;

	paf->next = obj->affected;
	obj->affected = paf;
    }
}




void spell_enchant_weapon(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;
    AFFECT_DATA *paf;
    int result;
    int fail;
    int hit_bonus;
    int dam_bonus;
    int added;
    bool hit_found = false;
    bool dam_found = false;

    if (obj->item_type != ITEM_WEAPON) {
	send_to_char("That isn't a weapon, `1TARD``!\n\r", ch);
	return;
    }

    if (obj->wear_loc != -1) {
	send_to_char("The item `1*`!must`1* ``be carried to be enchanted.\n\r", ch);
	return;
    }

    /* this means they have no bonus */
    hit_bonus = 0;
    dam_bonus = 0;
    fail = 25; /* base 25% chance of failure */

    /* find the bonuses */
    if (!obj->enchanted) {
	for (paf = obj->objprototype->affected; paf != NULL; paf = paf->next) {
	    if (paf->location == APPLY_HITROLL) {
		hit_bonus = (int)paf->modifier;
		hit_found = true;
		fail += 2 * (hit_bonus * hit_bonus);
	    } else if (paf->location == APPLY_DAMROLL) {
		dam_bonus = (int)paf->modifier;
		dam_found = true;
		fail += 2 * (dam_bonus * dam_bonus);
	    } else { /* things get a little harder */
		fail += 25;
	    }
	}
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next) {
	if (paf->location == APPLY_HITROLL) {
	    hit_bonus = (int)paf->modifier;
	    hit_found = true;
	    fail += 2 * (hit_bonus * hit_bonus);
	} else if (paf->location == APPLY_DAMROLL) {
	    dam_bonus = (int)paf->modifier;
	    dam_found = true;
	    fail += 2 * (dam_bonus * dam_bonus);
	} else { /* things get a little harder */
	    fail += 25;
	}
    }

    /* apply other modifiers */
    fail -= 3 * level / 2;

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
	fail -= 15;

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
	fail -= 5;

    fail = URANGE(5, fail, 95);
    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5)) { /* item destroyed */
	act("$p shivers `8vio`!l`1e`!n`8tly`` and explodes!", ch, obj, NULL, TO_CHAR);
	act("$p shivers `8vio`!l`!e`!n`8tly`` and explodeds!", ch, obj, NULL, TO_ROOM);
	extract_obj(obj);
	return;
    }

    if (result < (fail / 2)) { /* item disenchanted */
	AFFECT_DATA *paf_next;

	act("$p glows `1bri`#ght`1ly``, then fades...`1FUCK``!.", ch, obj, NULL, TO_CHAR);
	act("$p glows `1bri`#ght`1ly``, then fades.", ch, obj, NULL, TO_ROOM);
	obj->enchanted = true;

	/* remove all affects */
	for (paf = obj->affected; paf != NULL; paf = paf_next) {
	    paf_next = paf->next;
	    free_affect(paf);
	}
	obj->affected = NULL;

	/* clear all flags */
	obj->extra_flags = 0;
	return;
    }

    if (result <= fail) {   /* failed, no bad result */
	send_to_char("Nothing seemed to `3happen``.\n\r", ch);
	return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted) {
	AFFECT_DATA *af_new;

	obj->enchanted = true;

	for (paf = obj->objprototype->affected; paf != NULL; paf = paf->next) {
	    af_new = new_affect();

	    af_new->next = obj->affected;
	    obj->affected = af_new;

	    af_new->where = paf->where;
	    af_new->type = UMAX(0, paf->type);
	    af_new->level = paf->level;
	    af_new->duration = paf->duration;
	    af_new->location = paf->location;
	    af_new->modifier = paf->modifier;
	    af_new->bitvector = paf->bitvector;
	}
    }

    if (result <= (100 - level / 5)) { /* success! */
	act("$p glows blue.", ch, obj, NULL, TO_CHAR);
	act("$p glows blue.", ch, obj, NULL, TO_ROOM);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	added = 2;
    } else {
	act("$p glows a brillant blue!", ch, obj, NULL, TO_CHAR);
	act("$p glows a brillant blue!", ch, obj, NULL, TO_ROOM);
	SET_BIT(obj->extra_flags, ITEM_MAGIC);
	SET_BIT(obj->extra_flags, ITEM_GLOW);
	added = 4;
    }

    /* now add the enchantments */
    if (obj->level < LEVEL_HERO - 1)
	obj->level = UMIN(LEVEL_HERO - 1, obj->level + 1);

    if (dam_found) {
	for (paf = obj->affected; paf != NULL; paf = paf->next) {
	    if (paf->location == APPLY_DAMROLL) {
		paf->type = skill->sn;
		paf->modifier += added;

		paf->level = UMAX(paf->level, level);
		if (paf->modifier > 4)
		    SET_BIT(obj->extra_flags, ITEM_HUM);
	    }
	}
    } else {
	paf = new_affect();
	paf->where = TO_OBJECT;
	paf->type = skill->sn;
	paf->level = level;
	paf->duration = -1;
	paf->location = APPLY_DAMROLL;
	paf->modifier = added;
	paf->bitvector = 0;
	paf->next = obj->affected;
	obj->affected = paf;
    }

    if (hit_found) {
	for (paf = obj->affected; paf != NULL; paf = paf->next) {
	    if (paf->location == APPLY_HITROLL) {
		paf->type = skill->sn;
		paf->modifier += added;
		paf->level = UMAX(paf->level, level);
		if (paf->modifier > 4)
		    SET_BIT(obj->extra_flags, ITEM_HUM);
	    }
	}
    } else {
	paf = new_affect();
	paf->type = skill->sn;
	paf->level = level;
	paf->duration = -1;
	paf->location = APPLY_HITROLL;
	paf->modifier = added;
	paf->bitvector = 0;
	paf->next = obj->affected;
	obj->affected = paf;
    }
}



/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;

    if (saves_spell(level, victim, DAM_NEGATIVE)) {
	send_to_char("You feel a momentary `Oc`^h`6i`^l`Ol``.\n\r", victim);
	send_to_char("Your opponent `Os`^h`Oi`^v`Oe`^r`Os`` for a moment, but shakes it off.\n\r", ch);
	return;
    }

    if (victim->level <= 2) {
	dam = ch->hit + 1;
    } else {
	if (!IS_NPC(victim))
	    gain_exp(victim, 0 - number_range(level / 2, 3 * level / 2));
	victim->mana /= 2;
	victim->move /= 2;
	dam = dice(1, level);
	ch->hit += dam;
    }

    send_to_char("You feel your `!life`` `&s`7li`8pping`` away!\n\r", victim);
    send_to_char("`1W`!o`1w`8....`1what `!a `1rush``!\n\r", ch);
    damage(ch, victim, dam, skill->sn, DAM_NEGATIVE, true);

    return;
}

void spell_fireball(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    int type;


    type = dice(1, 4);

    switch (type) {
	case 1:
	    {
		static const int dam_each[] =
		{
		    0,
		    0,  0,	  0,   0,   0,	 0,   0,   0,	0,   0,
		    0,  0,	  0,   0,   30,	 35,  40,  45,	50,  55,
		    60, 65,	  70,  75,  80,	 82,  84,  86,	88,  90,
		    92, 94,	  96,  98,  100, 102, 104, 106, 108, 110,
		    112,114,  116, 118, 120, 122, 124, 126, 128, 130
		};

		int dam;


		level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
		level = UMAX(0, level);
		dam = number_range(dam_each[level] * 2, dam_each[level] * 6);

		if (saves_spell(level, victim, DAM_FIRE))
		    dam /= 2;


		damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
		return;
	    }

	case 2:
	    {
		if (gsp_burning_flames != NULL) {
		    af.type = gsp_burning_flames->sn;
		    af.skill = gsp_burning_flames;
		    af.level = ch->level;
		    af.duration = ch->level;

		    af.location = APPLY_STR;
		    af.modifier = -number_range(1, 25);
		    af.bitvector = (long)AFF_BURNING;
		    affect_join(victim, &af);

		    send_to_char("Your fireball explodes catching your target on fire!!\n\r", ch);
		    act("$n screams in agony as they burst into flames!", victim, NULL, NULL, TO_ROOM);
		    break;
		}
	    }

	case 3:
	    {
		static const int dam_each[] =
		{
		    0,
		    0,  0,	  0,   0,   0,	 0,   0,   0,	0,   0,
		    0,  0,	  0,   0,   30,	 35,  40,  45,	50,  55,
		    60, 65,	  70,  75,  80,	 82,  84,  86,	88,  90,
		    92, 94,	  96,  98,  100, 102, 104, 106, 108, 110,
		    112,114,  116, 118, 120, 122, 124, 126, 128, 130
		};

		int dam;


		level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
		level = UMAX(0, level);
		dam = number_range(dam_each[level] * 3, dam_each[level] * 9);
		if (saves_spell(level, victim, DAM_FIRE))
		    dam /= 2;

		damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
		return;
	    }
	case 4:
	    {
		static const int dam_each[] =
		{
		    0,
		    0,  0,	  0,   0,   0,	 0,   0,   0,	0,   0,
		    0,  0,	  0,   0,   30,	 35,  40,  45,	50,  55,
		    60, 65,	  70,  75,  80,	 82,  84,  86,	88,  90,
		    92, 94,	  96,  98,  100, 102, 104, 106, 108, 110,
		    112,114,  116, 118, 120, 122, 124, 126, 128, 130
		};

		int dam;


		level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
		level = UMAX(0, level);
		dam = number_range(dam_each[level] * 4, dam_each[level] * 10);
		if (saves_spell(level, victim, DAM_FIRE))
		    dam /= 2;

		damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
		return;
	    }
	default:
	    {
		static const int dam_each[] =
		{
		    0,
		    0,  0,	  0,   0,   0,	 0,   0,   0,	0,   0,
		    0,  0,	  0,   0,   30,	 35,  40,  45,	50,  55,
		    60, 65,	  70,  75,  80,	 82,  84,  86,	88,  90,
		    92, 94,	  96,  98,  100, 102, 104, 106, 108, 110,
		    112,114,  116, 118, 120, 122, 124, 126, 128, 130
		};

		int dam;


		level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
		level = UMAX(0, level);
		dam = number_range(dam_each[level], dam_each[level] * 2);
		if (saves_spell(level, victim, DAM_FIRE))
		    dam /= 2;

		damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
		return;
	    }
    }
}




void spell_fireproof(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj;
    AFFECT_DATA af;

    /* deal with the object case first */
    if (target == TARGET_OBJ) {
	obj = (struct gameobject *)vo;
	if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
	    act("$p is already protected from `1b`!u`1r`!n`1i`!n`1g``.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	af.where = TO_OBJECT;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = number_fuzzy(level / 4);
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = ITEM_BURN_PROOF;

	affect_to_obj(obj, &af);

	act("You protect $p from `!f`1i`#r`!e``.", ch, obj, NULL, TO_CHAR);
	act("$p is surrounded by a `2pro`@t`7ec`@t`2ive`` aura.", ch, obj, NULL, TO_ROOM);
    } else {
	victim = (struct char_data *)vo;

	affect_strip(victim, skill);
	af.where = TO_IMMUNE;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = level / 2;
	af.location = 0;
	af.modifier = 0;
	af.bitvector = IMM_FIRE;
	affect_to_char(victim, &af);

	send_to_char("You are now fireproofed.\n\r", victim);
	if (ch != victim)
	    act("You have now fireproofed $N.", ch, NULL, victim, TO_CHAR);

	return;
    }
}



void spell_flamestrike(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;


    dam = dice((3 * level) / 3, 20);
    if (saves_spell(level, victim, DAM_FIRE))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
    return;
}



void spell_faerie_fire(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	AFFECT_DATA *paf;

	for (paf = victim->affected; paf != NULL; paf = paf->next) {
	    if (paf->type == skill->sn
		    && paf->modifier != 100
		    && paf->location != APPLY_AC) {
		affect_strip(victim, skill);
		af.where = TO_AFFECTS;
		af.type = skill->sn;
		af.skill = skill;
		af.level = ch->level;
		af.duration = level / 10;
		af.modifier = 100;
		af.bitvector = 0;
		af.location = APPLY_SAVING_SPELL;
		affect_to_char(victim, &af);

		af.location = APPLY_AC;
		af.modifier = 4 * level;
		af.bitvector = AFF_FAERIE_FIRE;
		affect_to_char(victim, &af);

		send_to_char("Your `Ppink`` outline glows `&brighter``!\n\r", victim);
		act("$n's `Ppink`` outline glows `&brighter``!", victim, NULL, NULL, TO_ROOM);
	    } else if (paf->type == skill->sn
		    && paf->modifier == 100
		    && paf->location != APPLY_AC) {
		act("There is absolutely no room left in $n's aura for any more `Ppinkness``!\n\r", victim, NULL, NULL, TO_ROOM);
		send_to_char("There is no room left in your aura for more `Ppinkness``!\n\r", victim);
	    }
	}
    } else if (!IS_AFFECTED(victim, AFF_FAERIE_FIRE)) {
	send_to_char("You are surrounded by a `Ppink`` outline.\n\r", victim);
	act("$n is surrounded by a `Ppink`` outline.", victim, NULL, NULL, TO_ROOM);

	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = level / 10;
	af.location = APPLY_AC;
	af.modifier = 2 * level;
	af.bitvector = AFF_FAERIE_FIRE;
	affect_to_char(victim, &af);

	af.modifier = 50;
	af.bitvector = 0;

	af.location = APPLY_SAVING_SPELL;
	affect_to_char(victim, &af);
    } else {
	act("There is absolutely no room left in $n's aura for any more `Ppinkness``!\n\r", victim, NULL, NULL, TO_ROOM);
	send_to_char("There is no room left in your aura for more `Ppinkness``!\n\r", victim);
    }

    return;
}



void spell_faerie_fog(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct room_index_data *room;
    struct char_data *ich;
    AFFECT_DATA af;

    if (!(room = ch->in_room))
	return;

    act("$n conjures a c`8l`&o`8u``d of p`5urpl``e smoke.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You conjure a c`8l`&o`8u``d of p`5urp``le smoke.\n\r", ch);

    if (!is_affected_room(room, skill)) {
	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = number_range(6, 10);
	af.location = 0;
	af.modifier = 2 * level;
	af.bitvector = 0;
	affect_to_room(room, &af);
    }

    for (ich = room->people; ich != NULL; ich = ich->next_in_room) {
	if (ich->invis_level > 0)
	    continue;

	if (ich == ch || saves_spell(level, ich, DAM_OTHER))
	    continue;

	affect_strip(ich, gsp_invisibility);
	affect_strip(ich, gsp_mass_invisibility);
	affect_strip(ich, gsp_sneak);
	REMOVE_BIT(ich->affected_by, AFF_HIDE);
	REMOVE_BIT(ich->affected_by, AFF_INVISIBLE);
	REMOVE_BIT(ich->affected_by, AFF_SNEAK);
	act("$n is `!revealed``!", ich, NULL, NULL, TO_ROOM);
	send_to_char("You are `1revealed``!\n\r", ich);
    }

    return;
}

void spell_floating_disc(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *disc, *floating;

    floating = get_eq_char(ch, WEAR_FLOAT);
    if (floating != NULL && IS_OBJ_STAT(floating, ITEM_NOREMOVE)) {
	act("You can't remove $p.", ch, floating, NULL, TO_CHAR);
	return;
    }

    disc = create_object(objectprototype_getbyvnum(OBJ_VNUM_DISC), 0);
    disc->value[0] = ch->level * 10;        /* 10 pounds per level capacity */
    disc->value[3] = ch->level * 5;         /* 5 pounds per level max per item */
    disc->timer = ch->level * 2 - number_range(0, level / 2);

    act("$n has created a floating `8black`` `1disc``.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You create a floating `1disc``.\n\r", ch);
    obj_to_char(disc, ch);
    wear_obj(ch, disc, true);
    return;
}


void spell_fly(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_FLYING)) {
	if (victim == ch)
	    send_to_char("You are already `&_-^`#airborne`&^-_``.\n\r", ch);
	else
	    act("$N doesn't need your help to `#fly``.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level + 3;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char(victim, &af);

    send_to_char("Your feet `&_-``rise`&-_`` off the ground.\n\r", victim);
    act("$n's feet `&_-``rise`&-_`` off the ground.", victim, NULL, NULL, TO_ROOM);

    return;
}

/* RT clerical berserking spell */
void spell_frenzy(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill) || IS_AFFECTED(victim, AFF_BERSERK)) {
	if (victim == ch)
	    send_to_char("You are already in a `1fr`!e`#nzy``.\n\r", ch);
	else
	    act("$N is already in a `1fr`!e`#nzy``.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (is_affected(victim, skill_lookup("calm"))) {
	if (victim == ch)
	    send_to_char("Why don't you just `^c`Ohil`^l`` for a while?\n\r", ch);
	else
	    act("$N doesn't look like $e wants to `1fight`` anymore.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 3;
    af.modifier = level / 6;
    af.bitvector = 0;

    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);

    af.modifier = 10 * (level / 12);
    af.location = APPLY_AC;
    affect_to_char(victim, &af);

    send_to_char("You are filled with `#holy `&wrath``!\n\r", victim);
    act("$n gets a `1w`!i`1l`!d`` look in $s eyes!", victim, NULL, NULL, TO_ROOM);
}

/* RT ROM-style gate */
void spell_gate(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    bool gate_pet;
    int result;
    int fail;


    if ((victim = get_char_world(ch, argument)) == NULL
	    || (!can_trans_room(ch, victim, skill->sn))) {
	send_to_char("You failed.\n\r", ch);
	return;
    }


    if (ch->position == POS_FIGHTING) {
	fail = 65;
	result = number_percent();
	if (result >= fail) {
	    send_to_char("You failed!\n\r", ch);
	    return;
	}
    }

    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
	gate_pet = true;
    else
	gate_pet = false;

    act("$n steps through a gate and `8v`7a`&n`7i`8s`7h`&e`7s``.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You step through a gate and `8v`7a`&ni`7s`8h``.\n\r", ch);
    char_from_room(ch);
    char_to_room(ch, victim->in_room);

    act("$n has arrived through a `8g`7a`&t`7e``.", ch, NULL, NULL, TO_ROOM);
    do_look(ch, "auto");

    if (gate_pet) {
	act("$n steps through a gate and `8v`7a`&n`7i`8s`7h`&e`6s``.", ch->pet, NULL, NULL, TO_ROOM);
	send_to_char("You step through a gate and `8v`7a`&n`7i`8s`7h``.\n\r", ch->pet);
	char_from_room(ch->pet);
	char_to_room(ch->pet, victim->in_room);
	act("$n has arrived through a `8g`7a`&t`8e``.", ch->pet, NULL, NULL, TO_ROOM);
	do_look(ch->pet, "auto");
    }
}



void spell_giant_strength(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You are already as `1s`!t`#ro`!n`1g`` as you can get!\n\r", ch);
	else
	    act("$N can't get any `1s`!t`#rong`!e`1r``.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = (level / 15);
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your muscles surge with `Phe`5ighten`Ped `1p`!o`#w`!e`1r``!\n\r", victim);
    act("$n's muscles surge with `Phe`5ighten`Ped `1p`!o`#w`!e`1r``.", victim, NULL, NULL, TO_ROOM);
    return;
}



void spell_harm(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;


    dam = UMAX(200, victim->hit - dice(1, 4));
    if (saves_spell(level, victim, DAM_HARM))
	dam = UMIN(50, dam / 2);

    dam = 3 * (UMIN(100, dam));
    damage(ch, victim, dam, skill->sn, DAM_HARM, true);
    return;
}

/* RT haste spell */
void spell_haste(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)
	    || IS_AFFECTED(victim, AFF_HASTE)
	    || IS_SET(victim->off_flags, OFF_FAST)) {
	if (victim == ch)
	    send_to_char("You are already one `!FAST`` Motherfucker!\n\r", ch);
	else
	    act("$N is already a `@fast`` motherfucker.", ch, NULL, victim, TO_CHAR);

	return;
    }

    if (IS_AFFECTED(victim, AFF_SLOW)) {
	if (!check_dispel(level, victim, skill_lookup("slow"))) {
	    if (victim != ch)
		send_to_char("`1S`!pell `1f`!ailed``.\n\r", ch);

	    send_to_char("You feel momentarily `@faster``.\n\r", victim);
	    return;
	}
	act("$n is moving less s l  o   w    l    y.", victim, NULL, NULL, TO_ROOM);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    if (victim == ch)
	af.duration = level / 2;
    else
	af.duration = level / 4;

    af.location = APPLY_DEX;
    af.modifier = 1 + (int)(level >= 18) + (int)(level >= 25) + (int)(level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char(victim, &af);

    send_to_char("You feel yourself moving more `8quickly``.\n\r", victim);
    act("$n is moving more `8quickly``.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    return;
}



void spell_heal(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;

    victim->hit = UMIN(victim->hit + 400, victim->max_hit);
    update_pos(victim);

    send_to_char("A `1w`!a`#r`!m`` feeling fills your body.\n\r", victim);
    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    return;
}

void spell_heal_mana(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;

    victim->mana += dice(2, 8) + level;
    victim->mana = UMIN(victim->mana, victim->max_mana);
    send_to_char("A warm glow passes through you.\n\r", victim);

    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);
    return;
}

void spell_identify(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;

    identify_item(ch, obj);
    return;
}

void spell_infravision(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_INFRARED)) {
	if (victim == ch)
	    send_to_char("You can already `#see ``in the `8dark``.\n\r", ch);
	else
	    act("$N already has `1in`!f`#rav`!isi`1on``.\n\r", ch, NULL, victim, TO_CHAR);
	return;
    }
    act("$n's eyes glow `1r`!e`1d``.\n\r", ch, NULL, NULL, TO_ROOM);

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char(victim, &af);

    send_to_char("Your eyes glow `1red``.\n\r", victim);
    return;
}



void spell_invis(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    AFFECT_DATA af;

    /* object invisibility */
    if (target == TARGET_OBJ) {
	struct gameobject *obj = (struct gameobject *)vo;

	if (IS_OBJ_STAT(obj, ITEM_INVIS)) {
	    act("$p is already `8invisible``.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	af.where = TO_OBJECT;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = level + 12;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = ITEM_INVIS;
	affect_to_obj(obj, &af);

	act("$p `8f`7a`&d`7e`8s`` out of sight.", ch, obj, NULL, TO_ALL);
	return;
    }

    /* character invisibility */
    victim = (struct char_data *)vo;

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
	return;

    act("$n `8f`7a`&d`7e`8s`` out of existence.", victim, NULL, NULL, TO_ROOM);

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level + 12;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(victim, &af);
    send_to_char("You `8f`7a`&de`` out of existence.\n\r", victim);
    return;
}

void spell_lightning_bolt(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    static const int dam_each[] =
    {
	0,
	0, 0,	0,  0,	0,  0,	0,  0,	25, 28,
	31,34,	37, 40, 40, 41, 42, 42, 43, 44,
	44,45,	46, 46, 47, 48, 48, 49, 50, 50,
	51,52,	52, 53, 54, 54, 55, 56, 56, 57,
	58,58,	59, 60, 60, 61, 62, 62, 63, 64
    };

    int dam;


    level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] * 2, dam_each[level] * 6);
    if (saves_spell(level, victim, DAM_LIGHTNING))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_LIGHTNING, true);
    return;
}

void spell_locate_object(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct buf_type *buffer;
    struct gameobject *obj, *opending;
    struct gameobject *in_obj;
    bool found;
    int number = 0;
    int max_found;


    found = false;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

    buffer = new_buf();

    opending = object_iterator_start(&object_empty_filter);
    while ((obj = opending) != NULL) {
	opending = object_iterator(obj, &object_empty_filter);
	if (!can_see_obj(ch, obj) || !is_name(argument, object_name_get(obj)) || IS_OBJ_STAT(obj, ITEM_NOLOCATE) || number_percent() > 2 * level || ch->level < obj->level)
	    continue;

	found = true;
	number++;

	for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

	if (in_obj->carried_by != NULL && can_see(ch, in_obj->carried_by)) {
	    if (can_see(in_obj->carried_by, ch))
		printf_buf(buffer, "One is carried by %s\n\r", PERS(in_obj->carried_by, ch));
	} else {
	    if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
		printf_buf(buffer, "One is in %s [Room %d]\n\r", in_obj->in_room->name, in_obj->in_room->vnum);
	    else if (in_obj->in_room != NULL)
		printf_buf(buffer, "One is in %s\n\r", in_obj->in_room->name);
	}


	if (number >= max_found)
	    break;
    }

    if (!found)
	send_to_char("Nothing like that in heaven or earth.\n\r", ch);
    else
	page_to_char(buf_string(buffer), ch);

    free_buf(buffer);

    return;
}

/* New Magic Missile code by Tetsuo */
void spell_magic_missile(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *tmp_vict, *last_vict, *next_vict;
    bool found;
    int dam;
    int chance;
    int num_missile;



    /* first strike */
    num_missile = level / 50;
    /*
     *      act("A magic missile erupts from $n's hand and hits $N in the chest.",
     *           ch, NULL, victim, TO_NOTVICT);
     *      act("A magic missile flies from your hand and hits $N in the chest.",
     *           ch, NULL, victim, TO_CHAR);
     *      act("A magic missile flies from $n's hand and hits you in the chest!",
     *           ch, NULL, victim, TO_VICT);*/

    dam = dice(level, 55);
    chance = number_percent();
    if (chance > 20)
	dam = dam / 3 * 2;

    if (saves_spell(level, victim, DAM_ENERGY))
	dam /= 3;

    damage(ch, victim, dam, skill->sn, DAM_ENERGY, true);
    last_vict = victim;
    num_missile -= 1;       /* decrement number of missiles */

    /* new targets */
    while (num_missile > 0) {
	found = false;
	for (tmp_vict = ch->in_room->people;
		tmp_vict != NULL;
		tmp_vict = next_vict) {
	    next_vict = tmp_vict->next_in_room;

	    if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict && num_missile > 0) {
		last_vict = tmp_vict;
		if (is_same_group(tmp_vict, ch))
		    continue;

		found = true;

		/*				act("A magic missile erupts from $n's hand and hits $N in the chest.",
		 *                                       ch, NULL, tmp_vict, TO_NOTVICT);
		 *                              act("A magic missile flies from your hand and hits $N in the chest.",
		 *                                       ch, NULL, tmp_vict, TO_CHAR);
		 *                              act("A magic missile flies from $n's hand and hits you in the chest!",
		 *                                       ch, NULL, tmp_vict, TO_VICT);*/
		dam = dice(level / 1, 2);
		if (saves_spell(level, tmp_vict, DAM_ENERGY))
		    dam /= 2;

		damage(ch, tmp_vict, dam, skill->sn, DAM_ENERGY, true);
		num_missile -= 1;       /* decrement number of missiles */
	    }
	} /* end target searching loop */

	if (!found) {   /* no target found, hit the caster */
	    if (ch == NULL)
		return;

	    if (last_vict == ch)     /* no double hits */
		return;

	    last_vict = ch;
	    num_missile -= 1;       /* decrement damage */
	    if (ch == NULL)
		return;
	}
	/* now go back and find more targets */
    }
}


void spell_mass_healing(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *gch;
    SKILL *skill_heal;
    SKILL *skill_refresh;

    skill_heal = skill_lookup("heal");
    skill_refresh = skill_lookup("refresh");

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
	if ((IS_NPC(ch) && IS_NPC(gch)) || (!IS_NPC(ch) && !IS_NPC(gch))) {
	    cast_spell(ch, skill_heal, level, gch, TARGET_CHAR, argument);
	    cast_spell(ch, skill_refresh, level, gch, TARGET_CHAR, argument);
	}
    }
}


void spell_mass_invis(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    AFFECT_DATA af;
    struct char_data *gch;

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
	if (!is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_INVISIBLE))
	    continue;

	act("$n slowly `8f`7a`&d`7e`8s`` out of existence.", gch, NULL, NULL, TO_ROOM);
	send_to_char("You slowly `8f`7ad`&e`` out of existence.\n\r", gch);

	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level / 2;
	af.duration = 24;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_INVISIBLE;
	affect_to_char(gch, &af);
    }
    send_to_char("`1O`!k``.\n\r", ch);

    return;
}

void spell_null(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    send_to_char("That's not a `1s`!p`&e`!l`1l``!\n\r", ch);
    return;
}



void spell_pass_door(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_PASS_DOOR)) {
	if (victim == ch)
	    send_to_char("You are already out of `1p`!h`#a`!s`1e``.\n\r", ch);
	else
	    act("$N is already shifted out of `1p`!h`#a`!s`1e``.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char(victim, &af);

    act("$n turns `1t`!r`&an`7slu`&ce`!n`1t``.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You turn `1t`!r`&an`7slu`&ce`!n`1t``.\n\r", victim);
    return;
}

void spell_extinguish_flames(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;

    if (gsp_burning_flames == NULL
	    || !is_affected(victim, gsp_burning_flames)) {
	if (victim == ch)
	    send_to_char("You aren't on fire!.\n\r", ch);
	else
	    act("$N isn't on fire.", ch, NULL, victim, TO_CHAR);

	return;
    }

    if (check_dispel(level, victim, gsp_burning_flames)) {
	send_to_char("A wave of cold rushes through your body as the flames are extinguished!\n\r", ch);
	act("The burning flames on $n's body are extinguished.", victim, NULL, NULL, TO_ROOM);
    } else {
	send_to_char("Spell failed.\n\r", ch);
    }
}



void spell_burning_flames(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (saves_spell(level, victim, DAM_FIRE)) {
	act("$n resists the flames.", victim, NULL, NULL, TO_ROOM);
	send_to_char("You resist the flames.\n\r", victim);
	return;
    }

    if (IS_SET(victim->imm_flags, IMM_FIRE))
	return;

    af.type = skill->sn;
    af.level = level;
    af.skill = skill;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = -2;
    af.bitvector = (long)AFF_BURNING;
    affect_join(victim, &af);

    send_to_char("You burst into flames!.\n\r", victim);
    act("$n bursts into flames!.", victim, NULL, NULL, TO_ROOM);

    return;
}


void spell_poison(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj;
    AFFECT_DATA af;


    if (target == TARGET_OBJ) {
	obj = (struct gameobject *)vo;

	if (obj->item_type == ITEM_FOOD
		|| obj->item_type == ITEM_DRINK_CON) {
	    if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
		act("Your spell fails to `1co`!rru`1pt`` $p.", ch, obj, NULL, TO_CHAR);
		return;
	    }

	    obj->value[3] = 1;
	    act("$p is infused with `!poi`#son`!ous`` vapors.", ch, obj, NULL, TO_ALL);
	    return;
	}

	if (obj->item_type == ITEM_WEAPON) {
	    if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
		    || IS_WEAPON_STAT(obj, WEAPON_FROST)
		    || IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
		    || IS_WEAPON_STAT(obj, WEAPON_SHARP)
		    || IS_WEAPON_STAT(obj, WEAPON_VORPAL)
		    || IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
		    || IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
		act("You can't seem to `!env`1e`!nom`` $p.", ch, obj, NULL, TO_CHAR);
		return;
	    }

	    if (IS_WEAPON_STAT(obj, WEAPON_POISON)) {
		act("$p is already `1env`!eno`1med``.", ch, obj, NULL, TO_CHAR);
		return;
	    }

	    af.where = TO_WEAPON;
	    af.type = skill->sn;
	    af.skill = skill;
	    af.level = level / 2;
	    af.duration = level / 8;
	    af.location = 0;
	    af.modifier = 0;
	    af.bitvector = WEAPON_POISON;
	    affect_to_obj(obj, &af);

	    act("$p is coated with `8d`1e`!a`7d`&ly`` venom.", ch, obj, NULL, TO_ALL);
	    return;
	}

	act("You can't `Ppo`!is`Pon`` $p.", ch, obj, NULL, TO_CHAR);
	return;
    }

    victim = (struct char_data *)vo;

    if (saves_spell(level, victim, DAM_POISON)) {
	act("$n turns slightly `@g`2r`@e`2e`@n``, but it passes.", victim, NULL, NULL, TO_ROOM);
	send_to_char("You feel momentarily `2i`@l`2l``, but it passes.\n\r", victim);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = -2;
    af.bitvector = AFF_POISON;
    affect_join(victim, &af);

    send_to_char("You feel very `2s`@i`2c`@k``.\n\r", victim);
    act("$n looks very `2i`@l`2l``.", victim, NULL, NULL, TO_ROOM);

    return;
}

void spell_recharge(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;
    int percnt;
    int charges;
    int partial;
    int fail;

    if (obj->wear_loc != -1) {
	send_to_char("The item `1*`!must`1*`` be in your inventory.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_STAFF && obj->item_type != ITEM_WAND) {
	send_to_char("You can only `Orecharge `1wands ``and `!staves``.\n\r", ch);
	return;
    }

    if (obj->value[3] >= 3 * level / 2) {
	send_to_char("Your skill`&z`` are not great enough for that.\n\r", ch);
	return;
    }

    fail = number_range(1, 8);      /* Give a 25% change just doesn't work */

    if ((fail == 1) || (fail == 2)) {
	send_to_char("Nothing seemed to `^happen``.\n\r", ch);
	return;
    }

    if (obj->value[2] > 0) {
	act("$p flares `&blind`#ing`8ly``... and evaporates!", ch, obj, NULL, TO_CHAR);
	act("$p flares `&blind`#ing`8ly``... and evaporates!", ch, obj, NULL, TO_ROOM);
	extract_obj(obj);
	return;
    }
    percnt = number_range(1, 100);

    if (percnt < 26) { /*  Fully recharge */
	obj->value[2] = obj->value[1];
	act("$p `@glows `!bri`1g`&h`1t`!ly!``", ch, obj, NULL, TO_CHAR);
	return;
    } else if (percnt < 71) { /* Partial recharge */
	partial = (int)(obj->value[1] - 1);
	charges = number_range(1, partial);
	obj->value[2] = charges;
	act("$p`@ glows `^so`6ft`^ly``.", ch, obj, NULL, TO_CHAR);
	return;
    } else {
	/* blows up */
	act("$p flares `1blin`!di`1ngly``... and evaporates!", ch, obj, NULL, TO_CHAR);
	act("$p flares `1blin`!di`1ngly``... and evaporates!", ch, obj, NULL, TO_ROOM);
	extract_obj(obj);
	return;
    }
}


void spell_refresh(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;

    victim->move = UMIN(victim->move + level, victim->max_move);
    if (victim->max_move == victim->move)
	send_to_char("You feel fully `Orefreshed``!\n\r", victim);
    else
	send_to_char("You feel less `Otired``.\n\r", victim);


    if (ch != victim)
	send_to_char("`1O`!k``.\n\r", ch);

    return;
}

void spell_remove_curse(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj;

    /* do object cases first */
    if (target == TARGET_OBJ) {
	obj = (struct gameobject *)vo;

	if (IS_OBJ_STAT(obj, ITEM_NODROP)
		|| IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
	    if (!IS_OBJ_STAT(obj, ITEM_NOUNCURSE)
		    && !saves_dispel(level + 2, obj->level, 0)) {
		REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
		REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
		act("$p `@glows `^b`6l`4u`Oe``.", ch, obj, NULL, TO_ALL);
		return;
	    }

	    act("The `8curse`` on $p is beyond your po`#w``er.", ch, obj, NULL, TO_CHAR);
	    return;
	}

	act("There doesn't seem to be a `8curse`` on $p.", ch, obj, NULL, TO_CHAR);
	return;
    }

    /* characters */
    victim = (struct char_data *)vo;

    if (check_dispel(level, victim, gsp_curse)) {
	send_to_char("You feel `#b`@e`#t`2t`#e`@r``.\n\r", victim);
	act("$n looks more `Pre`5lax`Ped``.", victim, NULL, NULL, TO_ROOM);
    }

    for (obj = victim->carrying; obj != NULL; obj = obj->next_content) {
	if ((IS_OBJ_STAT(obj, ITEM_NODROP)
		    || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
		&& !IS_OBJ_STAT(obj, ITEM_NOUNCURSE)) {
	    if (!saves_dispel(level, obj->level, 0)) {
		REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
		REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);

		act("Your $p `@glows `6b`^l`Ou`4e``.", victim, obj, NULL, TO_CHAR);
		act("$n's $p `@glows `6b`^l`Ou`4e``.", victim, obj, NULL, TO_ROOM);

		break;
	    }
	}
    }
}

/*
 * spell_revive:  re-worked by Monrick, 3/2008
 */
void spell_revive(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    SKILL *skill_strip;
    int modifier;
    int diff;
    int xMana;
    int maxGain;
    int idx;
    char *strip_affs[] = { "poison", "blindness", "sleep", "" };

    diff = 0;
    if (ch->mana <= 0) {
	send_to_char("Your spell sputters and fizzles!!\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) &&
	    (ch->class != class_lookup("cleric"))) {
	modifier = (ch->class == class_lookup("witch")) ? 5 : 1;

	diff = ch->max_hit - ch->hit;
	maxGain = (ch->mana * modifier);

	if (diff >= maxGain) {
	    send_to_char("You channel everything you have into life!\n\r", ch);
	    ch->hit += maxGain;
	    ch->mana = 0;
	    xMana = 0;
	} else {
	    send_to_char("You channel your mana into life!\n\r", ch);
	    ch->hit = ch->max_hit;
	    xMana = (diff / modifier);
	}
    } else {
	if (ch->mana < 1000) {
	    send_to_char("``You don`8'``t have enough mana!\n\r", ch);
	    return;
	}

	send_to_char("`8T`7h`&e `8g`7o`&ds `8f`7a`&vor `8y`7o`&u``!\n\r", ch);
	ch->hit = ch->max_hit;
	xMana = 500;

	for (idx = 0; strip_affs[idx][0] != '\0'; idx++)
	    if ((skill_strip = skill_lookup(strip_affs[idx])) != NULL)
		affect_strip(ch, skill_strip);
    }

    ch->mana -= xMana;
    update_pos(ch);
    return;
}

/***************************************************************************
 *	spell_sanctuary
 ***************************************************************************/
void spell_sanctuary(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_SANCTUARY)) {
	if (victim == ch)
	    send_to_char("You are already in `&sanctuary``.\n\r", ch);
	else
	    act("$N is already in `&sanctuary``.", ch, NULL, victim, TO_CHAR);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 6;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char(victim, &af);

    act("$n is surrounded by a `&white ``aura.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You are surrounded by a `&white ``aura.\n\r", victim);
    return;
}


void spell_druid_call(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DRUID_CALL)) {
	if (victim == ch)
	    send_to_char("The `@d`2r`#u`2i`@d`8s`` have already aided you for battle.\n\r", ch);
	else
	    act("$N is already `1pro`!t`&e`!c`1ted`` by the `@d`2r`#u`2i`@d`8s``.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 6;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_DRUID_CALL;
    affect_to_char(victim, &af);

    act("$n is surrounded by a `8grey ``aura.", victim, NULL, NULL, TO_ROOM);
    send_to_char("The `2d`@r`#u`@i`2d`8s`` `1pro`&t`1ect`` you.\n\r", victim);
    return;
}

void spell_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You are already `2sh`@i`&e`@l`2ded`` from harm.\n\r", ch);
	else
	    act("$N is already `1pro`!t`&e`!c`1ted`` by a `2s`@h`&ie`@l`2d``.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = 8 + level;
    af.location = APPLY_AC;
    af.modifier = -100;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("$n is surrounded by a force `2s`@h`&ie`@l`2d``.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You are surrounded by a force `2s`@h`&ie`@l`2d``.\n\r", victim);
    return;
}

void spell_shocking_grasp(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    static const int dam_each[] =
    {
	0,
	0, 0,	0,  0,	0,  0,	20, 25, 29, 33,
	36,39,	39, 39, 40, 40, 41, 41, 42, 42,
	43,43,	44, 44, 45, 45, 46, 46, 47, 47,
	48,48,	49, 49, 50, 50, 51, 51, 52, 52,
	53,53,	54, 54, 55, 55, 56, 56, 57, 57
    };

    int dam;


    level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level], dam_each[level] * 3);
    if (saves_spell(level, victim, DAM_LIGHTNING))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_LIGHTNING, true);
    return;
}



void spell_sleep(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_SLEEP)
	    || (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD))
	    || (level + 2) < victim->level
	    || saves_spell(level - 4, victim, DAM_CHARM)) {
	send_to_char("Your sleep spell does nothing.\n\r", ch);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = 5;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, &af);

    if (IS_AWAKE(victim)) {
	send_to_char("You feel very sleepy `8..... `1z`!z`1z`!z`1z`!z``.\n\r", victim);
	send_to_char("It worked!  They fall fast asleep.....\n\r", ch);
	act("$n goes to `@sleep``.", victim, NULL, NULL, TO_ROOM);
	victim->position = POS_SLEEPING;
    }


    return;
}

void spell_slow(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill) || IS_AFFECTED(victim, AFF_SLOW)) {
	if (victim == ch)
	    send_to_char("You can't move any s l  o   w    e     r!\n\r", ch);
	else
	    act("$N can't get any s l  o   w    e     r than that.", ch, NULL, victim, TO_CHAR);

	return;
    }

    if (saves_spell(level, victim, DAM_OTHER)
	    || IS_SET(victim->imm_flags, IMM_MAGIC)) {
	if (victim != ch)
	    send_to_char("Nothing seemed to `@happen``.\n\r", ch);

	send_to_char("You feel momentarily `@let`2h`Oa`2r`@gic.``\n\r", victim);
	return;
    }

    if (IS_AFFECTED(victim, AFF_HASTE)) {
	if (!check_dispel(level, victim, gsp_haste)) {
	    if (victim != ch)
		send_to_char("`1S`!pell `1f`!ailed``.\n\r", ch);

	    send_to_char("You feel momentarily s l  o   w    e     r.\n\r", victim);
	    return;
	}

	act("$n is moving less `8qu`&ick`8ly``.", victim, NULL, NULL, TO_ROOM);
	return;
    }


    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_DEX;
    af.modifier = -1 - (int)(level >= 18) - (int)(level >= 25) - (int)(level >= 32);
    af.bitvector = AFF_SLOW;
    affect_to_char(victim, &af);

    send_to_char("You feel yourself s l  o   w    i    n      g d o w n...\n\r", victim);
    act("$n starts to move in s l  o   w motion.", victim, NULL, NULL, TO_ROOM);
    return;
}




void spell_stone_skin(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("Your skin is already as `Phard`` as a `8r`3o`8c`3k``.\n\r", ch);
	else
	    act("$N is already as `Phard`` as can be.", ch, NULL, victim, TO_CHAR);

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level;
    af.location = APPLY_AC;
    af.modifier = level * -4 / 3;
    af.bitvector = 0;    /*AFF_STONE_SKIN;*/
    affect_to_char(victim, &af);

    act("$n's skin turns to `3s``t`8o``n`3e``.", victim, NULL, NULL, TO_ROOM);
    send_to_char("Your skin turns to `3s``t`8o``n`3e``.\n\r", victim);
    return;
}



void spell_ventriloquate(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];

    struct char_data *vch;

    argument = one_argument(argument, speaker);

    sprintf(buf1, "%s says '%s'.\n\r", speaker, argument);
    sprintf(buf2, "Someone makes %s say '%s'.\n\r", speaker, argument);
    buf1[0] = UPPER(buf1[0]);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	if (!is_name(speaker, vch->name))
	    send_to_char(saves_spell(level, vch, DAM_OTHER) ? buf2 : buf1, vch);

    return;
}



void spell_weaken(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	send_to_char("They already look weak.\n\r", ch);
	return;
    }
    if (saves_spell(level, victim, DAM_OTHER)) {
	act("$n starts to slump over, but fights off the affects.\n\r", victim, NULL, NULL, TO_ROOM);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_STR;
    af.modifier = -1 * (level / 5);
    af.bitvector = AFF_WEAKEN;
    affect_to_char(victim, &af);

    send_to_char("You feel your strength `8s`7li`8p`& a`7w`8a`7y``.\n\r", victim);
    act("$n looks `@tired ``and `2weak``.", victim, NULL, NULL, TO_ROOM);

    return;
}

/***************************************************************************
 *	spell_timewarp
 ***************************************************************************/
void spell_timewarp(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    act("``The world `Pph`5a`^s`6e`&s`` as $n warps time.", ch, NULL, NULL, TO_ROOM);
    send_to_char("The world `Pph`5a`^s`6e`&s`` as you warp time.\n\r", ch);
    do_tick(ch, "");
    return;
}


/*
 * NPC spells.
 */
void spell_acid_breath(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam, hp_dam, dice_dam, hpch;


    act("$n spits `@a`2c`1i`!d`` at $N.", ch, NULL, victim, TO_NOTVICT);
    act("$n spits a stream of corrosive `@a`2c`1i`!d`` at you.", ch, NULL, victim, TO_VICT);
    act("You spit `@a`2c`1i`!d`` at $N.", ch, NULL, victim, TO_CHAR);

    hpch = UMAX(30, ch->hit);
    hpch = UMIN(hpch, 20000);             /* cap this - dam was ridiculous for players with high hp */
    hp_dam = number_range(hpch / 7 + 1, hpch / 4);
    dice_dam = dice(level, 20);
    dam = UMIN(hp_dam + (dice_dam / 2), dice_dam + (hp_dam / 2));

    if (saves_spell(level, victim, DAM_ACID)) {
	acid_effect(victim, level / 2, dam / 4, TARGET_CHAR);
	damage(ch, victim, dam / 2, skill->sn, DAM_ACID, true);
    } else {
	acid_effect(victim, level, dam, TARGET_CHAR);
	damage(ch, victim, dam, skill->sn, DAM_ACID, true);
    }
}



void spell_fire_breath(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *vch;
    struct char_data *vch_next;
    int dam;
    int hp_dam;
    int dice_dam;
    int hpch;


    act("$n breathes forth a cone of `1f`!i`#r`&e``.", ch, NULL, victim, TO_NOTVICT);
    act("$n breathes a cone of hot `1f`!i`#r`&e`` over you!", ch, NULL, victim, TO_VICT);
    act("You breath forth a cone of `1f`!i`#r`&e``.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(30, ch->hit);
    hpch = UMIN(hpch, 20000);             /* cap this - dam was ridiculous for players with high hp */
    hp_dam = number_range(hpch / 9 + 1, hpch / 5);
    dice_dam = dice(level, 20);
    dam = UMIN(hp_dam + (dice_dam / 2), dice_dam + (hp_dam / 2));

    fire_effect(victim->in_room, level, dam / 2, TARGET_ROOM);

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
	vch_next = vch->next_in_room;

	if (is_safe_spell(ch, vch, true)
		|| (IS_NPC(vch) && IS_NPC(ch)
		    && (ch->fighting != vch || vch->fighting != ch)))
	    continue;

	if (vch == victim) {     /* full damage */
	    if (saves_spell(level, vch, DAM_FIRE)) {
		fire_effect(vch, level / 2, dam / 4, TARGET_CHAR);
		damage(ch, vch, dam / 2, skill->sn, DAM_FIRE, true);
	    } else {
		fire_effect(vch, level, dam, TARGET_CHAR);
		damage(ch, vch, dam, skill->sn, DAM_FIRE, true);
	    }
	} else {
	    /* partial damage */
	    if (saves_spell(level - 2, vch, DAM_FIRE)) {
		fire_effect(vch, level / 4, dam / 8, TARGET_CHAR);
		damage(ch, vch, dam / 4, skill->sn, DAM_FIRE, true);
	    } else {
		fire_effect(vch, level / 2, dam / 4, TARGET_CHAR);
		damage(ch, vch, dam / 2, skill->sn, DAM_FIRE, true);
	    }
	}
    }
}

void spell_frost_breath(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *vch;
    struct char_data *vch_next;
    int dam;
    int hp_dam;
    int dice_dam;
    int hpch;


    act("$n breathes out a freezing cone of `6f`^r`&o`Os`4t`!", ch, NULL, victim, TO_NOTVICT);
    act("$n breathes a freezing cone of `6f`^r`&o`Os`4t`` over you!", ch, NULL, victim, TO_VICT);
    act("You breath out a cone of `6f`^r`&o`Os`4t``.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(30, ch->hit);
    hpch = UMIN(hpch, 20000);             /* cap this - dam was ridiculous for players with high hp */
    hp_dam = number_range(hpch / 9 + 1, hpch / 6);
    dice_dam = dice(level, 20);
    dam = UMIN(hp_dam + (dice_dam / 2), dice_dam + (hp_dam / 2));

    cold_effect(victim->in_room, level, dam / 2, TARGET_ROOM);

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
	vch_next = vch->next_in_room;

	if (is_safe_spell(ch, vch, true)
		|| (IS_NPC(vch) && IS_NPC(ch)
		    && (ch->fighting != vch || vch->fighting != ch)))
	    continue;

	if (vch == victim) {     /* full damage */
	    if (saves_spell(level, vch, DAM_COLD)) {
		cold_effect(vch, level / 2, dam / 4, TARGET_CHAR);
		damage(ch, vch, dam / 2, skill->sn, DAM_COLD, true);
	    } else {
		cold_effect(vch, level, dam, TARGET_CHAR);
		damage(ch, vch, dam, skill->sn, DAM_COLD, true);
	    }
	} else {
	    if (saves_spell(level - 2, vch, DAM_COLD)) {
		cold_effect(vch, level / 4, dam / 8, TARGET_CHAR);
		damage(ch, vch, dam / 4, skill->sn, DAM_COLD, true);
	    } else {
		cold_effect(vch, level / 2, dam / 4, TARGET_CHAR);
		damage(ch, vch, dam / 2, skill->sn, DAM_COLD, true);
	    }
	}
    }
}


void spell_gas_breath(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;
    int dam;
    int hp_dam;
    int dice_dam;
    int hpch;


    act("$n breathes out a cloud of poisonous `Pg`1a`!s``!", ch, NULL, NULL, TO_ROOM);
    act("You breath out a cloud of poisonous `Pg`1a`!s``.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(30, ch->hit);
    hpch = UMIN(hpch, 20000);             /* cap this - dam was ridiculous for players with high hp */
    hp_dam = number_range(hpch / 9 + 1, 8);
    dice_dam = dice(level, 20);
    dam = UMIN(hp_dam + (dice_dam / 2), dice_dam + (hp_dam / 2));

    poison_effect(ch->in_room, level, dam, TARGET_ROOM);
    for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
	vch_next = vch->next_in_room;

	if (is_safe_spell(ch, vch, true)
		|| (IS_NPC(ch) && IS_NPC(vch)
		    && (ch->fighting == vch || vch->fighting == ch)))
	    continue;

	if (saves_spell(level, vch, DAM_POISON)) {
	    poison_effect(vch, level / 2, dam / 4, TARGET_CHAR);
	    damage(ch, vch, dam / 2, skill->sn, DAM_POISON, true);
	} else {
	    poison_effect(vch, level, dam, TARGET_CHAR);
	    damage(ch, vch, dam, skill->sn, DAM_POISON, true);
	}
    }
}

void spell_lightning_breath(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;
    int hp_dam;
    int dice_dam;
    int hpch;


    act("$n breathes a `&bolt ``of `Olightning`` at $N.", ch, NULL, victim, TO_NOTVICT);
    act("$n breathes a `&bolt ``of `Olightning ``at you!", ch, NULL, victim, TO_VICT);
    act("You breathe a `&bolt ``of `Olightning ``at $N.", ch, NULL, victim, TO_CHAR);

    hpch = UMAX(30, ch->hit);
    hpch = UMIN(hpch, 20000);             /* cap this - dam was ridiculous for players with high hp */
    hp_dam = number_range(hpch / 9 + 1, hpch / 5);
    dice_dam = dice(level, 20);
    dam = UMIN(hp_dam + (dice_dam / 2), dice_dam + (hp_dam / 2));

    if (saves_spell(level, victim, DAM_LIGHTNING)) {
	shock_effect(victim, level / 2, dam / 4, TARGET_CHAR);
	damage(ch, victim, dam / 2, skill->sn, DAM_LIGHTNING, true);
    } else {
	shock_effect(victim, level, dam, TARGET_CHAR);
	damage(ch, victim, dam, skill->sn, DAM_LIGHTNING, true);
    }
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;

    dam = number_range(250, 1000);
    if (saves_spell(level, victim, DAM_PIERCE))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_PIERCE, true);
    return;
}

void spell_high_explosive(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;


    dam = number_range(300, 1200);
    if (saves_spell(level, victim, DAM_PIERCE))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_PIERCE, true);
    return;
}

void spell_equipment_invis(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;

    if (obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_ARMOR) {
	if (!IS_OBJ_STAT(obj, ITEM_INVIS)) {
	    SET_BIT(obj->extra_flags, ITEM_INVIS);
	    act("$p `8f`7a`&d`7e`8s`` out of existence.", ch, obj, NULL, TO_ALL);
	    return;
	} else {
	    send_to_char("That item is already `8invisible``.\n\r", ch);
	    return;
	}
    } else {
	send_to_char("That item is not a `1wea`!pon`` or piece of `^arm`6or``.\n\r", ch);
	return;
    }
}

void spell_noremove(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *obj = (struct gameobject *)vo;

    if (obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_ARMOR) {
	if (!IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
	    SET_BIT(obj->extra_flags, ITEM_NOREMOVE);
	    act("$p glows `1red``.", ch, obj, NULL, TO_ALL);
	    return;
	} else if (!IS_OBJ_STAT(obj, ITEM_NODROP)) {
	    SET_BIT(obj->extra_flags, ITEM_NODROP);
	    act("$p glows `!bright `1red``.", ch, obj, NULL, TO_ALL);
	    return;
	} else {
	    send_to_char("That item is already `1noremove``/`1nodrop``.\n\r", ch);
	    return;
	}
    } else {
	send_to_char("That item is not a `1weapon ``or piece of `!armor``.\n\r", ch);
	return;
    }
}

void spell_shatter_curse(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj;
    bool found = false;

    /* do object cases first */
    if (target == TARGET_OBJ) {
	obj = (struct gameobject *)vo;

	if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
	    if (!IS_OBJ_STAT(obj, ITEM_NOUNCURSE) && !saves_dispel(level + 2, obj->level, 0)) {
		act("You convulse as you toss $p to the ground, destroying it.", ch, obj, NULL, TO_CHAR);
		extract_obj(obj);
		return;
	    }

	    act("You failed.", ch, NULL, NULL, TO_CHAR);
	    return;
	}
	act("$p isn't cursed.", ch, obj, NULL, TO_CHAR);
	return;
    }

    victim = (struct char_data *)vo;

    for (obj = victim->carrying; (obj != NULL && !found); obj = obj->next_content) {
	if ((IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
		&& !IS_OBJ_STAT(obj, ITEM_NOUNCURSE)) {
	    if (!saves_dispel(level, obj->level, 0)) {
		found = true;
		act("You convulse as you toss $p to the ground, destroying it.", victim, obj, NULL, TO_CHAR);
		act("$n convulses as $e tosses $p to the ground, destroying it.", victim, obj, NULL, TO_ROOM);
		extract_obj(obj);
	    }
	}
    }
}

void spell_winds(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct room_index_data *room;
    SKILL *skill_fog;

    skill_fog = gsp_faerie_fog;

    if ((room = ch->in_room) == NULL)
	return;

    act("$n conjures a powerful wind from the north.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You conjure powerful winds.\n\r", ch);

    if (skill_fog != NULL && is_affected_room(room, skill_fog)) {
	act("$n's winds dissipates the `Ppurple haze``.", ch, NULL, NULL, TO_ROOM);
	send_to_char("Your winds cause the `Ppurple haze`` to dissipate.\n\r", ch);
	affect_strip_room(room, skill_fog->sn);
    }

    return;
}


/***************************************************************************
 *	spell_web
 *
 *	to do:
 *		add - merc.h
 *			extern int	gsn_web;
 *		add - db.c
 *			int			gsn_web;
 *		add - const.c
 *			{
 *				"web",
 *				{ 120, 302, 302, 302, 302 },
 *				{ 2, 2, 4, 4, 2 },
 *				spell_web,		TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
 *				NULL,			SLOT(569),	20,	12,
 *				"spell",		"The sticky goo trapping you dissipates.",	""
 *			},
 *		add - magic.h
 *			DECLARE_SPELL_FUN(	spell_web	);
 *		add - fight.c
 *			bool	check_dispel(int dis_level, struct char_data *victim, int sn);
 *
 *		add - in multi-hit
 *			if(is_affected(ch, gsn_web))
 *			{
 *				int disp_lvl;
 *
 *				disp_lvl =(IS_NPC(ch)) ? ch->level * 3 / 4 : ch->level;
 *				disp_lvl =(ch->size > SIZE_HUGE) ? ch->level * 7 / 8: ch->level / 2;
 *				if(!check_dispel(disp_lvl, ch, gsn_web))
 *				{
 *					return;
 *				}
 *			}
 *		add - act_move.c
 *			if(is_affected(ch, gsn_web) )
 *			{
 *				send_to_char("You are rooted to the ground with a sticky web.\n\r", ch );
 *				return;
 *			}
 *
 ***************************************************************************/
void spell_web(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	act("$N is already webbed.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (saves_spell(level, victim, DAM_ENERGY)) {
	act("Your webs are not strong enough to contain $N.", ch, NULL, victim, TO_CHAR);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 30;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;

    affect_to_char(victim, &af);

    act("Strands of energy shoot from $N's hands encasing $n in a gooey web.", victim, NULL, ch, TO_ROOM);
    act("Strands of energy spring from $N's hands encasing you in a sticky web.", victim, NULL, ch, TO_CHAR);
    return;
}







/***************************************************************************
 *	spell_displacement
 *
 *	add the displacement affect the room
 ***************************************************************************/
void spell_displacement(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct room_index_data *room;
    AFFECT_DATA af;

    if ((room = ch->in_room) == NULL)
	return;

    if (is_affected_room(room, skill)) {
	send_to_char("This room is already `4d`Oi`6s`Op`4l`Oa`6c`Oe`6d``.\n\r", ch);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_range(10, 25);
    af.location = 0;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_room(room, &af);

    send_to_char("You `4d`Oi`6s`Op`4l`Oa`6c`Oe`` the room.\n\r", ch);
    return;
}




/***************************************************************************
 *	spell_haven
 *
 *	set the haven affect
 ***************************************************************************/
void spell_haven(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct room_index_data *room;
    AFFECT_DATA af;

    if ((room = ch->in_room) == NULL)
	return;

    if (is_affected_room(room, skill)) {
	send_to_char("The room is already `3p`7e`&a`7c`&e`7f`&u`3l``.\n\r", ch);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_range(3, 5);
    af.location = 0;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_room(room, &af);

    send_to_char("A wave of `#p`7e`&a`7c`&e`` washes over the room.\n\r", ch);
    return;
}


/***************************************************************************
 *	spell_mana_vortex
 *
 *	add the mana vortex affect the room
 ***************************************************************************/
void spell_mana_vortex(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct room_index_data *room;
    AFFECT_DATA af;

    if ((room = ch->in_room) == NULL)
	return;

    if (is_affected_room(room, skill)) {
	send_to_char("This room already `1p`!u`1ls`!e`1s`` with a hunger for `1m`!a`1n`!a``.\n\r", ch);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_range(10, 25);
    af.location = 0;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_room(room, &af);

    send_to_char("The room comes alive with a `1th`!i`1rst`` for `1m`!a`1n`!a``.\n\r", ch);
    return;
}


/***************************************************************************
 *	spell_parasitic_cloud
 *
 *	add the displacement affect the room
 ***************************************************************************/
void spell_parasitic_cloud(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct room_index_data *room;
    AFFECT_DATA af;

    if ((room = ch->in_room) == NULL)
	return;

    if (is_affected_room(room, skill)) {
	send_to_char("A `2p`@a`2r`@a`2s`@i`2t`@i`2c`` cloud already fills the room.\n\r", ch);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_range(10, 25);
    af.location = 0;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_room(room, &af);

    send_to_char("You conjure a `2m`@a`2l`@i`2c`@i`8o`@u`2s`` cloud.\n\r", ch);
    return;
}





/***************************************************************************
 *	spell_granduer
 ***************************************************************************/
void spell_grandeur(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    AFFECT_DATA af;
    int hp_mod;
    int mv_mod;
    int mana_cost;

    if (is_affected(ch, skill)) {
	send_to_char("You are already as `3b`#ee`3f`#y`` as you can appear.\n\r", ch);
	return;
    }

    hp_mod = ch->max_hit;
    mv_mod = ch->max_move;

    mana_cost = hp_mod / 8;

    ch->mana = UMAX(ch->mana - mana_cost, 0);

    if (ch->hit <= ch->max_hit)
	ch->hit += hp_mod;

    if (ch->move <= ch->max_move)
	ch->move += mv_mod;

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 30;
    af.location = APPLY_HIT;
    af.modifier = hp_mod;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.location = APPLY_MOVE;
    af.modifier = mv_mod;
    affect_to_char(ch, &af);

    act("$n `1sw`!e`1lls`` to `@i`2mm`@e`2ns`@e`` proportions!", ch, NULL, NULL, TO_ROOM);
    send_to_char("`OI`4ll`Ou`4s`Oio`4n`` engulfs you and you appear much `1l`!a`1rg`!e`1r`` than `3l`#i`3f`#e``!\n\r", ch);
    return;
}


/***************************************************************************
 *	spell_farsight
 ***************************************************************************/
void spell_farsight(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    AFFECT_DATA af;

    if (IS_AFFECTED(ch, AFF_BLIND)) {
	send_to_char("Maybe it would help if you could `&s`#e`&e``?\n\r", ch);
	return;
    }

    if (!is_affected(ch, skill)) {
	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = number_range(3, 8);
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	send_to_char("Your eyes `#tingle`` and `@glow``.\n\r", ch);
    } else {
	send_to_char("You cannot see any further.\n\r", ch);
    }
}



/***************************************************************************
 *	spell_portal
 ***************************************************************************/
void spell_portal(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    struct gameobject *portal;

    if ((victim = get_char_world(ch, argument)) == NULL
	    || !can_trans_room(ch, victim, skill->sn)) {
	send_to_char("`1You`! failed``.\n\r", ch);
	return;
    }


    portal = create_object(objectprototype_getbyvnum(OBJ_VNUM_PORTAL), 0);
    portal->timer = 2 + level / 25;
    portal->value[0] = 1;
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal, ch->in_room);

    act("$p appears.", ch, portal, NULL, TO_ROOM);
    act("$p appears.", ch, portal, NULL, TO_CHAR);
}


/***************************************************************************
 *	spell_nexus
 ***************************************************************************/
void spell_nexus(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim;
    struct gameobject *portal;
    struct room_index_data *to_room;
    struct room_index_data *from_room;


    if ((victim = get_char_world(ch, argument)) == NULL
	    || !can_trans_room(ch, victim, skill->sn)) {
	send_to_char("`1You `!failed``.\n\r", ch);
	return;
    }


    from_room = ch->in_room;
    to_room = victim->in_room;

    /* portal one */
    portal = create_object(objectprototype_getbyvnum(OBJ_VNUM_PORTAL), 0);
    portal->timer = 1 + level / 10;
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal, from_room);

    act("$p `8s`&w`7ir`&l`8s`` into `@existence``.", ch, portal, NULL, TO_ROOM);
    act("$p `8s`&w`7ir`&l`8s`` into `@existence``.", ch, portal, NULL, TO_CHAR);

    /* portal two */
    portal = create_object(objectprototype_getbyvnum(OBJ_VNUM_PORTAL), 0);
    portal->timer = 1 + level / 10;
    portal->value[3] = ch->in_room->vnum;

    obj_to_room(portal, to_room);

    if (victim->in_room->people != NULL) {
	act("$p `8s`&w`7ir`&l`8s`` into `@existence``.", victim->in_room->people, portal, NULL, TO_ROOM);
	act("$p `8s`&w`7ir`&l`8s`` into `@existence``.", victim->in_room->people, portal, NULL, TO_CHAR);
    }
}


/***************************************************************************
 *	spell_pollenburst
 ***************************************************************************/
void spell_pollenburst(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, skill)) {
	send_to_char("Your victim is already blinded by `#pollen``.\n\r", ch);
	return;
    }

    if (saves_spell(level, victim, DAM_OTHER)) {
	send_to_char("Your victim dodged your `#pollen``!\n\r", ch);
	return;
    }


    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = number_range(0, 2);
    af.location = APPLY_DEX;
    af.modifier = -5;
    af.bitvector = AFF_POLLEN;
    affect_to_char(victim, &af);

    af.location = APPLY_HITROLL;
    af.modifier = -2;
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, &af);

    send_to_char("You release a burst of `#pollen``!\n\r", ch);
    act("$n releases a burst of `#pollen``!\n\r", ch, NULL, NULL, TO_ROOM);
}


/***************************************************************************
 *	spell_thorns
 ***************************************************************************/
void spell_thorns(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;

    dam = dice(level, 15);
    if (saves_spell(level, victim, DAM_WOOD))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_WOOD, true);
    return;
}


/***************************************************************************
 *	spell_monsoon
 ***************************************************************************/
void spell_monsoon(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *tmp_vict, *last_vict, *next_vict;
    bool found;
    int dam;
    int chance;
    int num_hit = 4;

    act("$n conjures up a m`4o`^n`6s`^o`4o``n to drown $N.", ch, NULL, victim, TO_ROOM);
    act("$n conjures up a m`4o`^n`6s`^o`4o``n to drown you!", ch, NULL, victim, TO_VICT);

    dam = dice(level, 55);
    chance = number_percent();
    if (chance > 20)
	dam = dam / 3 * 2;

    if (saves_spell(level, victim, DAM_DROWNING))
	dam /= 3;

    damage(ch, victim, dam, skill->sn, DAM_DROWNING, true);
    last_vict = victim;
    num_hit -= 1;


    while (num_hit > 0) {
	found = false;
	for (tmp_vict = ch->in_room->people;
		tmp_vict != NULL;
		tmp_vict = next_vict) {
	    next_vict = tmp_vict->next_in_room;

	    if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict && num_hit > 0) {
		last_vict = tmp_vict;
		if (is_same_group(tmp_vict, ch))
		    continue;

		found = true;

		act("A tidal wave `^strikes`` $n!", victim, NULL, NULL, TO_ROOM);
		act("A tidal wave `Ocrashes`` into you!", victim, NULL, NULL, TO_CHAR);

		dam = (dam - 100) + number_range(1, 200);


		damage(ch, tmp_vict, dam, skill->sn, DAM_DROWNING, true);
		num_hit -= 1;
	    }
	}

	if (!found) {   /* no target found, hit the caster */
	    if (ch == NULL)
		return;

	    if (last_vict == ch)     /* no double hits */
		return;

	    last_vict = ch;
	    num_hit -= 1;       /* decrement damage */
	    if (ch == NULL)
		return;
	}
    }
}

/***************************************************************************
 *	spell_fireblade
 ***************************************************************************/
void spell_fireblade(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct gameobject *old_weapon;
    struct gameobject *weapon;

    if ((old_weapon = get_eq_char(ch, WEAR_WIELD)) != NULL) {
	if (old_weapon->objprototype != NULL
		&& old_weapon->objprototype->vnum == OBJ_VNUM_FIREBLADE) {
	    unequip_char(ch, old_weapon);
	    extract_obj(old_weapon);
	    act("The light in the room dims.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You release the weave of your weapon.\n\r", ch);
	    return;
	}
	unequip_char(ch, old_weapon);
    }
    weapon = create_object(objectprototype_getbyvnum(OBJ_VNUM_FIREBLADE), 0);

    weapon->level = ch->level;
    weapon->timer = ch->level / 4;

    obj_to_char(weapon, ch);
    equip_char(ch, weapon, WEAR_WIELD);
    act("A blinding light shines from $n.", ch, NULL, NULL, TO_ROOM);
    send_to_char("A blinding light stretches from your hand.\n\r", ch);
    return;
}

/***************************************************************************
 *	spell_ring_of_fire
 ***************************************************************************/
void spell_ring_of_fire(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    struct char_data *tmp_vict;
    struct char_data *last_vict;
    struct char_data *next_vict;
    struct exit_data *pexit;
    struct room_index_data *curr_room;
    int dam;
    int door;

    act("A ring of fire appears from the sky incinerating anything in its way.", ch, NULL, victim, TO_ROOM);
    act("A ring of fire rages toward you.", ch, NULL, victim, TO_VICT);
    act("You release the fury of fire!", ch, NULL, NULL, TO_CHAR);

    dam = dice(level, 12);
    if (saves_spell(level, victim, DAM_FIRE))
	dam /= 4;

    damage(ch, victim, dam, skill->sn, DAM_FIRE, true);
    last_vict = victim;

    if (ch->in_room == NULL)
	return;

    /* lets hit everyone in the room except for minions */
    for (tmp_vict = ch->in_room->people;
	    tmp_vict != NULL;
	    tmp_vict = next_vict) {
	next_vict = tmp_vict->next_in_room;
	if (is_same_group(tmp_vict, ch))
	    continue;

	if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict) {
	    last_vict = tmp_vict;
	    act("A ring of fire rages toward $n!", tmp_vict, NULL, NULL, TO_ROOM);
	    act("You are covered head to toe in flames!", tmp_vict, NULL, NULL, TO_CHAR);

	    dam = dice(level, 10);
	    if (saves_spell(level, tmp_vict, DAM_FIRE))
		dam /= 4;
	    damage(ch, tmp_vict, dam, skill->sn, DAM_FIRE, true);
	} /* end if(!is_safe_spell . . . */
    }       /* end for-loop */
    /* Now its time to go looking for other adjoining rooms */

    curr_room = ch->in_room;
    for (door = 0; door <= 5; door++) {
	if ((pexit = curr_room->exit[door]) != NULL
		&& (!IS_SET(pexit->exit_info, EX_CLOSED))) {
	    if ((tmp_vict = pexit->u1.to_room->people) != NULL) {
		switch (door) {
		    case DIR_NORTH:
			act("Fire rages north!", ch, NULL, NULL, TO_ROOM);
			send_to_char("Fire rages north!\n\r", ch);
			act("Flames enter from the south!", tmp_vict, NULL, NULL, TO_ROOM);
			send_to_char("Flames enter from the south!\n\r", tmp_vict);
			break;
		    case DIR_SOUTH:
			act("Fire rages south!", ch, NULL, NULL, TO_ROOM);
			send_to_char("Fire rages south!\n\r", ch);
			act("Flames enter from the north!", tmp_vict, NULL, NULL, TO_ROOM);
			send_to_char("Flames enter from the north!\n\r", tmp_vict);
			break;
		    case DIR_WEST:
			act("Fire rages west!", ch, NULL, NULL, TO_ROOM);
			send_to_char("Fire rages west!\n\r", ch);
			act("Flames enter from the east!", tmp_vict, NULL, NULL, TO_ROOM);
			send_to_char("Flames enter from the east!\n\r", tmp_vict);
			break;
		    case DIR_EAST:
			act("Fire rages east!", ch, NULL, NULL, TO_ROOM);
			send_to_char("Fire rages east!\n\r", ch);
			act("Flames enter from the west!", tmp_vict, NULL, NULL, TO_ROOM);
			send_to_char("Flames enter from the west!\n\r", tmp_vict);
			break;
		    case DIR_UP:
			act("Fire rages upward!", ch, NULL, NULL, TO_ROOM);
			send_to_char("Fire rages upward!\n\r", ch);
			act("Flames enter from below!", tmp_vict, NULL, NULL, TO_ROOM);
			send_to_char("Flames enter from below!\n\r", tmp_vict);
			break;
		    case DIR_DOWN:
			act("Fire rages downward!", ch, NULL, NULL, TO_ROOM);
			send_to_char("Fire rages downward!\n\r", ch);
			act("Flames enter from above!", tmp_vict, NULL, NULL, TO_ROOM);
			send_to_char("Flames enter from above!\n\r", tmp_vict);
			break;
		}

		for (tmp_vict = pexit->u1.to_room->people;
			tmp_vict != NULL;
			tmp_vict = next_vict) {
		    next_vict = tmp_vict->next_in_room;
		    last_vict = tmp_vict;

		    act("Fire rages your body!", ch, NULL, tmp_vict, TO_VICT);
		    act("Fire engulfs $n!", tmp_vict, NULL, NULL, TO_ROOM);

		    dam = dice(level, 10);
		    if (saves_spell(level, tmp_vict, DAM_FIRE))
			dam /= 4;
		    damage(ch, tmp_vict, dam, skill->sn, DAM_FIRE, true);
		}
	    }
	}
    }
}

/***************************************************************************
 *	spell_ice_shield
 ***************************************************************************/
void spell_ice_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    SKILL *skill_dispel;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You already have a shield of `6i`7c`^e`7.\n\r", ch);
	else
	    act("$N already has a shield of `6i`7c`^e`7.", ch, NULL, victim, TO_CHAR);
	return;
    }

    skill_dispel = skill_lookup("fire shield");
    if (is_affected(victim, skill_dispel)) {
	if (!check_dispel(level, victim, skill_dispel)) {
	    if (victim != ch)
		send_to_char("`!S`1pell `!f`1ailed`7.\n\r", ch);
	} else {
	    send_to_char("Your `!fire`7 shield is destroyed!\n\r", victim);
	    act("$n's `!fire`7 shield is snuffed out.", victim, NULL, NULL, TO_ROOM);
	}

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = (victim == ch) ? level / 3 : level / 2;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.bitvector = RES_FIRE;
    affect_to_char(victim, &af);

    af.where = TO_VULN;
    af.bitvector = VULN_COLD;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded by a shield of `6i`7c`^e`7.\n\r", victim);
    act("$n is surrounded by a shield of `6i`7c`^e`7.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return;
}

/***************************************************************************
 *	spell_fire_shield
 ***************************************************************************/
void spell_fire_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    SKILL *skill_dispel;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You already have a shield of `!fire`7.\n\r", ch);
	else
	    act("$N already has a shield of `!fire`7.", ch, NULL, victim, TO_CHAR);
	return;
    }

    skill_dispel = skill_lookup("ice_shield");
    if (is_affected(victim, skill_dispel)) {
	if (!check_dispel(level, victim, skill_dispel)) {
	    if (victim != ch)
		send_to_char("`!S`1pell `!f`1ailed`7.\n\r", ch);
	} else {
	    send_to_char("Your shield of `6i`7c`^e`` is destroyed!\n\r", victim);
	    act("$n's `6i`7c`^e`` shield is snuffed out.", victim, NULL, NULL, TO_ROOM);
	}
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = (victim == ch) ? level / 5 : level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.bitvector = RES_COLD;
    affect_to_char(victim, &af);

    af.where = TO_VULN;
    af.bitvector = VULN_FIRE;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded by a shield of `!fire`7.\n\r", victim);
    act("$n is surrounded by a shield of `!fire.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return;
}

/***************************************************************************
 *	spell_acidic_shield
 ***************************************************************************/
void spell_acidic_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    SKILL *skill_dispel;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You already have an `@a`2cidic`7 shield.\n\r", ch);

	else
	    act("$N already has a shield of `@a`2cid`7.", ch, NULL, victim, TO_CHAR);

	return;
    }

    skill_dispel = skill_lookup("water shield");
    if (is_affected(victim, skill_dispel)) {
	if (!check_dispel(level, victim, skill_dispel)) {
	    if (victim != ch)
		send_to_char("`!S`1pell `!f`1ailed`7.\n\r", ch);
	} else {
	    send_to_char("Your shield of `Owater`7 washes away!\n\r", victim);
	    act("$n's `Owater`` shield is snuffed out.", victim, NULL, NULL, TO_ROOM);
	}

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = (victim == ch) ? level / 5 : level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.bitvector = RES_DROWNING;
    affect_to_char(victim, &af);

    af.where = TO_VULN;
    af.bitvector = VULN_ACID;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded by a shield of `@a`2cid`7.\n\r", victim);
    act("$n is surrounded by a shield of `@a`2cid`7.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return;
}


/***************************************************************************
 *	spell_water_shield
 ***************************************************************************/
void spell_water_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    SKILL *skill_dispel;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You already have a shield of `Owater`7.\n\r", ch);
	else
	    act("$N already has a shield of `Owater`7.", ch, NULL, victim, TO_CHAR);

	return;
    }

    skill_dispel = skill_lookup("acidic shield");
    if (is_affected(victim, skill_dispel)) {
	if (!check_dispel(level, victim, skill_dispel)) {
	    if (victim != ch)
		send_to_char("`!S`1pell `!f`1ailed`7.\n\r", ch);
	} else {
	    send_to_char("Your shield of `@a`2cid`7 washes away!\n\r", victim);
	    act("$n's `@a`2cid`` shield is snuffed out.", victim, NULL, NULL, TO_ROOM);
	}

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = (victim == ch) ? level / 5 : level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.bitvector = RES_ACID;
    affect_to_char(victim, &af);

    af.where = TO_VULN;
    af.bitvector = VULN_DROWNING;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded by a shield of `Owater`7.\n\r", victim);
    act("$n is surrounded by a wall of `Owater`7.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return;
}

/***************************************************************************
 *	spell_holy_shield
 ***************************************************************************/
void spell_holy_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    SKILL *skill_dispel;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You already have a shield of `&holy`7 energy.\n\r", ch);
	else
	    act("$N already has a shield of `&holy`7 energy.", ch, NULL, victim, TO_CHAR);

	return;
    }

    skill_dispel = skill_lookup("negative shield");
    if (is_affected(victim, skill_dispel)) {
	if (!check_dispel(level, victim, skill_dispel)) {
	    if (victim != ch)
		send_to_char("`!S`1pell `!f`1ailed`7.\n\r", ch);
	} else {
	    send_to_char("Your shield of `8negative`7 energy is destroyed!\n\r", victim);
	    act("$n's `8negative`` shield is snuffed out.", victim, NULL, NULL, TO_ROOM);
	}
	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = (victim == ch) ? level / 5 : level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.bitvector = RES_NEGATIVE;
    affect_to_char(victim, &af);

    af.where = TO_VULN;
    af.bitvector = VULN_HOLY;
    affect_to_char(victim, &af);
    send_to_char("You are surrounded by a shield of `&holy`7 energy.\n\r", victim);
    act("$n is surrounded by a shield of `&holy`7 energy.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return;
}


/***************************************************************************
 *	spell_negative_shield
 ***************************************************************************/
void spell_negative_shield(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;
    SKILL *skill_dispel;

    if (is_affected(victim, skill)) {
	if (victim == ch)
	    send_to_char("You already have a shield of `8negative`7 energy.\n\r", ch);
	else
	    act("$N already has a shield of `8negative`7 energy.", ch, NULL, victim, TO_CHAR);
	return;
    }

    skill_dispel = skill_lookup("holy shield");
    if (is_affected(victim, skill_dispel)) {
	if (!check_dispel(level, victim, skill_dispel)) {
	    if (victim != ch)
		send_to_char("`!S`1pell `!f`1ailed`7.\n\r", ch);
	} else {
	    send_to_char("Your shield of `&holy`7 energy is destroyed!\n\r", victim);
	    act("$n's `&holy`` shield is snuffed out.", victim, NULL, NULL, TO_ROOM);
	}

	return;
    }

    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = (victim == ch) ? level / 5 : level / 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.where = TO_RESIST;
    af.bitvector = RES_HOLY;
    affect_to_char(victim, &af);

    af.where = TO_VULN;
    af.bitvector = VULN_NEGATIVE;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded by a shield of `8negative`7 energy.\n\r", victim);
    act("$n is surrounded by a shield of `8negative`7 energy.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
	send_to_char("Ok.\n\r", ch);

    return;
}




/***************************************************************************
 *	spell_freeze_bolt
 ***************************************************************************/
void spell_freeze_bolt(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;

    dam = dice(level, 10);
    if (saves_spell(level, victim, DAM_COLD))
	dam /= 2;

    damage(ch, victim, dam, skill->sn, DAM_COLD, true);
    return;
}


/***************************************************************************
 *	spell_freeze_storm
 ***************************************************************************/
void spell_freeze_storm(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;
    int dam;
    int counter;

    counter = 1;

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
	if (counter <= 20) {
	    vch_next = vch->next_in_room;
	    if (!is_safe_spell(ch, vch, true)) {
		if (is_same_group(vch, ch))
		    continue;

		dam = dice(level, 8);
		if (saves_spell(level, vch, DAM_COLD))
		    dam /= 2;
		damage(ch, vch, dam, skill->sn, DAM_COLD, true);
	    }
	    counter = counter + 1;
	} else {
	    return;
	}
    }
    return;
}


/***************************************************************************
 *	spell_frost_hands
 ***************************************************************************/
void spell_frost_hands(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    int dam;
    static const int dam_each[] =
    {
	0,
	0, 0,	0,  0,	14, 17, 20, 23, 26, 29,
	29,29,	30, 30, 31, 31, 32, 32, 33, 33,
	34,34,	35, 35, 36, 36, 37, 37, 38, 38,
	39,39,	40, 40, 41, 41, 42, 42, 43, 43,
	44,44,	45, 45, 46, 46, 47, 47, 48, 48
    };

    level = UMIN(level, (int)sizeof(dam_each) / (int)sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] * 2, dam_each[level] * 4);

    if (saves_spell(level, victim, DAM_COLD))
	dam /= 2;
    damage(ch, victim, dam, skill->sn, DAM_COLD, true);
    return;
}


/***************************************************************************
 *	spell_acidic_rain
 ***************************************************************************/
void spell_acidic_rain(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;
    int dam;


    send_to_char("You call `!A`&c`!i`&d`!i`&c rain from the heavens!\n\r", ch);

    for (vch = char_list; vch != NULL; vch = vch_next) {
	vch_next = vch->next;

	if (vch->in_room == NULL)
	    continue;

	if (vch->in_room->area == ch->in_room->area) {
	    send_to_char("`!A`&c`!i`&d`!i`&c rain pours down from above!\n\r", vch);

	    if (vch != ch && !is_safe_spell(ch, vch, true)) {
		if (is_same_group(vch, ch))
		    continue;

		if (IS_NPC(vch))
		    continue;

		dam = dice(level, 35);
		if (saves_spell(level, vch, DAM_ACID))
		    dam /= 2;

		damage(ch, vch, dam, skill->sn, DAM_ACID, true);
		acid_effect(vch, level / 2, dam / 3, TARGET_CHAR);
	    }
	}
    }
}


/***************************************************************************
 *       spell_super_speed
 ***************************************************************************/
void spell_super_speed(SKILL *skill, int level, struct char_data *ch, void *vo, int target, const char *argument)
{
    struct char_data *victim = (struct char_data *)vo;
    AFFECT_DATA af;

    if (is_affected(ch, skill)) {
	send_to_char("You are already moving as fast as you can!\n\r", victim);
	return;
    }


    af.where = TO_AFFECTS;
    af.type = skill->sn;
    af.skill = skill;
    af.level = level;
    af.duration = level / 5;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    act("$n begins to move MUCH faster!", ch, NULL, NULL, TO_ROOM);
    send_to_char("Your body speeds up to unnatural levels!\n\rThe parries of your opponent look like child's work.\n\r", ch);
    return;
}

/***************************************************************************
 *	can_trans_room
 *
 *	check to see if a character can get into a room via magical
 *	means
 *
 *	Updated by Monrick, 3/2008 - no more hard-coded rooms
 ***************************************************************************/
bool can_trans_room(struct char_data *ch, struct char_data *victim, int sn)
{
    if (victim == NULL || victim == ch || victim->in_room == NULL) return false;

    if (!can_see_room(ch, victim->in_room)) return false;
    if (room_is_private(victim->in_room)) return false;
    if (gsp_gate != NULL && sn == gsp_gate->sn)
	if (IS_SET(victim->in_room->room_flags, ROOM_NOGATE)) return false;
    if (gsp_portal != NULL && sn == gsp_portal->sn)
	if (IS_SET(victim->in_room->room_flags, ROOM_NOPORTAL)) return false;
    if (gsp_nexus != NULL && sn == gsp_nexus->sn)
	if (IS_SET(victim->in_room->room_flags, ROOM_NOPORTAL)) return false;
    if (IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)) return false;
    if (IS_SET(victim->in_room->room_flags, ROOM_NOTRANSPORT)) return false;
    if (IS_SET(ch->in_room->room_flags, ROOM_NOTRANSPORT)) return false;
    if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim))
	if (get_trust(ch) < get_trust(victim)) return false;
    if (!IS_IMMORTAL(ch) && IS_SET(victim->in_room->room_flags, ROOM_GODS_ONLY)) return false;
    if (get_trust(ch) < MAX_LEVEL && IS_SET(victim->in_room->room_flags, ROOM_IMP_ONLY)) return false;

    /* for some rooms, only portal and nexus can get you in and out */
    if ((gsp_portal != NULL && sn != gsp_portal->sn) && (gsp_nexus != NULL && sn != gsp_nexus->sn)) {
	if (IS_SET(ch->in_room->room_flags, ROOM_PORTALONLY)) return false;
	else if (IS_SET(victim->in_room->room_flags, ROOM_PORTALONLY)) return false;
    }
    return true;
}
