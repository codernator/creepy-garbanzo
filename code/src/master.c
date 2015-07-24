/**************************************************************************
 *  Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor(rtaylor@pacinfo.com)                               *
*           Gabrielle Taylor(gtaylor@pacinfo.com)                          *
*           Brian Moore(rom@rom.efn.org)                                   *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/
/***************************************************************************
*	guild/class specific skills, spells and other funkified shit
***************************************************************************/

/***************************************************************************
*	roll out:
*		this file
*		merc.h
*		db.c
*		interp.c
*		interp.h
*		magic.c
*		magic.h
*		fight.c
***************************************************************************/


/***************************************************************************
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <string.h>

#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "magic.h"

/***************************************************************************
*	declarations
***************************************************************************/
CHAR_DATA * get_char_scan       args((CHAR_DATA * ch, char *argument));

ROOM_INDEX_DATA *get_scan_room   args((ROOM_INDEX_DATA * room,
				       int direction,
				       int distance));



/***************************************************************************
*	carnage_affect
*
*	"Master" Warrior class skill - cannot be gained normally.
***************************************************************************/
void carnage_affect(CHAR_DATA *ch)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	AFFECT_DATA *paf;
	int mult;
	int chance;
	int skill;
	bool found;


	if (gsp_carnage == NULL
	    || (skill = get_learned_percent(ch, gsp_carnage)) <= 1)
		return;

	if ((vch = ch->fighting) == NULL)
		return;

	/* figure out how many people we are fighting */
	mult = 0;
	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;
		if (is_safe(ch, vch))
			continue;

		if (vch->fighting == ch)
			mult++;
	}

	found = FALSE;
	for (paf = ch->affected; paf != NULL; paf = paf->next) {
		if (paf->type == gsp_carnage->sn) {
			found = TRUE;
			switch (paf->location) {
			case APPLY_SAVING_SPELL:
				paf->modifier -= (mult * 2);
				ch->saving_throw -= (mult * 2);
				break;
			case APPLY_DAMROLL:
				paf->modifier += (mult * 3);
				ch->damroll += (mult * 3);
				break;
			case APPLY_HITROLL:
				paf->modifier += (mult * 3);
				ch->hitroll += (mult * 3);
				break;
			}
			paf->duration = 1;
		}
	}

	if (!found) {
		AFFECT_DATA af;

		af.where = TO_AFFECTS;
		af.type = gsp_carnage->sn;
		af.skill = gsp_carnage;
		af.level = ch->level;
		af.duration = 1;
		af.modifier = 10 + (mult * 2);
		af.bitvector = 0;

		/* add damroll affect */
		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);

		/* add hitroll affect */
		af.location = APPLY_HITROLL;
		affect_to_char(ch, &af);

		/* add saving affect */
		af.location = APPLY_SAVING_SPELL;
		af.modifier = (mult * -2);
		affect_to_char(ch, &af);
	}

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe(ch, vch))
			continue;

		if (vch->fighting == ch && ch != vch) {
			if ((chance = number_percent()) < skill) {
				damage(ch, vch, number_range(ch->level * (8 + mult), ch->level * (10 + mult)), gsp_carnage->sn, DAM_BASH, TRUE);
				check_improve(ch, gsp_carnage, TRUE, 1);

				if (vch != NULL) {
					damage(ch, vch, number_range(ch->level * (8 + mult), ch->level * (10 + mult)), gsp_carnage->sn, DAM_BASH,
					       TRUE);
					check_improve(ch, gsp_carnage, TRUE, 1);
				}

				check_killer(ch, vch);
			}
		}
	}
}

/***************************************************************************
*	spell_retribution
*
*	"Master" Cleric class skill - cannot be gained normally.
*
*	to make retribution work
*	in fight.c - damage(...) and damage(...)
*	before all the sanc/druid etc. checks
*
*       if(dam > 1
*       && !IS_NPC(victim)
*       && is_affected(victim, gsn_retribution)
*       && dt != gsn_retribution)
*       {
*               int	ret;
*
*               ret	= dam /3;
*               damage(vch, ch, ret, gsn_retribution, dam_type, TRUE);
*               dam	-= dam / 3;
*       }
***************************************************************************/
void spell_retribution(SKILL *skill, int level, CHAR_DATA *ch, void *vo, int target, char *argument)
{
	AFFECT_DATA af;

	if (is_affected(ch, skill)) {
		send_to_char("The spirits of vengence already protect you.\n\r", ch);
		return;
	}


	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = 20;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;

	affect_to_char(ch, &af);

	act("$n is surrounded by a `3a`1n`!g`1r`3y `!r`1e`!d ``aura.", ch, NULL, NULL, TO_ROOM);
	send_to_char("You are surrounded by a `3a`1n`!g`1r`3y `!r`1e`!d ``aura.\n\r", ch);

	return;
}




/***************************************************************************
*	spell_leech
*		Modifyed by Tylor -  Feb 22, 2007
*		Modifyed by Monrick - Jan 6, 2008
*	"Master" Witch class skill - cannot be gained normally.
***************************************************************************/
void spell_leech(SKILL *skill, int level, CHAR_DATA *ch, void *vo, int target, char *argument)
{
	CHAR_DATA *vch = (CHAR_DATA *)vo;
	int dam;
	int mana;
	int damdealt;

	dam = dice(level, 110);

	/* if they make their saving throw, halve the hp and mana damage */
	if (saves_spell(level, vch, DAM_NEGATIVE)) {
		dam /= 2;
		mana = UMIN(vch->mana, 300);
	} else {
		mana = UMIN(vch->mana, 600);
	}

	/* damage the victim */
	damdealt = damage(ch, vch, dam, skill->sn, DAM_NEGATIVE, TRUE);
	vch->mana -= mana;

	/* add 25% of the hp damage, and all of the mana damage
	 *          back to the caster */
	ch->hit = UMIN(ch->max_hit, ch->hit + (damdealt / 4));
	ch->mana = UMIN(ch->max_mana, ch->mana + mana);

	return;
}


/***************************************************************************
*	raze_damage
*
*	deal Raze damage for the entire room
***************************************************************************/
static void raze_damage(CHAR_DATA *ch, ROOM_INDEX_DATA *room, int level)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int hit;

	if (gsp_raze == NULL)
		return;

	act("The ground beneath your feet erupts in a shower of earth!!", room->people, NULL, NULL, TO_CHAR);
	act("The ground beneath your feet erupts in a shower of earth!!", room->people, NULL, NULL, TO_ROOM);

	hit = 0;
	for (vch = room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (vch->in_room == NULL
		    || ch->in_room == NULL
		    || vch->phased
		    || vch == ch)
			continue;

		/* safe room? */
		if (IS_SET(vch->in_room->room_flags, ROOM_SAFE)
		    || is_affected_room(vch->in_room, gsp_haven))
			continue;

		/* no killing healers, trainers, etc */
		if (IS_SHOPKEEPER(vch)
		    || IS_TRAINER(vch)
		    || IS_GUILDMASTER(vch)
		    || IS_HEALER(vch)
		    || IS_CHANGER(vch)
		    || IS_EXCHANGER(vch))
			continue;

		dam = dice(300, level);
		if (saves_spell(level, vch, DAM_BASH))
			dam /= 2;

		if (is_same_group(ch, vch))
			continue;

		damage(ch, vch, dam, gsp_raze->sn, DAM_BASH, TRUE);
		if (vch->fighting == ch
		    && vch->in_room != ch->in_room)
			stop_fighting(vch, FALSE);

		if (hit++ > 30)
			break;
	}
}


/***************************************************************************
*	raze_affect
*
*	deal Raze damage for the entire room
***************************************************************************/
void raze_affect(CHAR_DATA *ch)
{
	ROOM_INDEX_DATA *room;
	AFFECT_DATA *paf;
	int ticks;
	int strength;
	int mana;
	int dir;
	int depth;


	if (gsp_raze == NULL)
		return;

	if ((paf = affect_find(ch->affected, gsp_raze)) == NULL) {
		affect_strip(ch, gsp_raze);
		return;
	}


	ticks = paf->duration;
	strength = UMAX(30, (12 - ticks) * 13);
	mana = strength * 2;


	raze_damage(ch, ch->in_room, strength);

	for (depth = 1; depth <= 3; depth++) {
		strength = UMAX(10, strength - 10);
		for (dir = 0; dir < 6; dir++)
			if ((room = get_scan_room(ch->in_room, dir, depth)) != NULL)
				raze_damage(ch, room, strength);
	}

	if (paf->duration-- == 0)
		affect_strip(ch, gsp_raze);


	ch->mana = UMAX(0, ch->mana - mana);
	if (ch->mana == 0)
		affect_strip(ch, gsp_raze);
}


/***************************************************************************
*	spell_raze
*
*	"Master" Mage class skill - cannot be gained normally.
***************************************************************************/
void spell_raze(SKILL *skill, int level, CHAR_DATA *ch, void *vo, int target, char *argument)
{
	AFFECT_DATA af;

	if (is_affected(ch, skill)) {
		affect_strip(ch, skill);
	} else {
		af.where = TO_AFFECTS;
		af.type = skill->sn;
		af.skill = skill;
		af.level = level;
		af.duration = 12;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		act("$n is surrounded by a `3a`1n`!g`1r`3y `!r`1e`!d ``aura.", ch, NULL, NULL, TO_ROOM);
		send_to_char("You are surrounded by a `3a`1n`!g`1r`3y `!r`1e`!d ``aura.\n\r", ch);
	}

	return;
}




/***************************************************************************
*	do_displace
*
*	"Master" Thief class skill - cannot be gained normally.
***************************************************************************/
void do_displace(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch;
	OBJ_DATA *wpn;
	AFFECT_DATA af;
	int skill;

	if (gsp_displace == NULL
	    || (skill = get_learned_percent(ch, gsp_displace)) < 1) {
		send_to_char("You have no idea of what you are doing do you?.\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
		send_to_char("Who is your intented victim?\n\r", ch);
		return;
	}

	if (ch->fighting != NULL) {
		send_to_char("You are too busy fighting.\n\r", ch);
		return;
	} else if ((vch = get_char_scan(ch, argument)) == NULL) {
		send_to_char("They are nowhere in your line of sight.\n\r", ch);
		return;
	}

	if (vch == ch) {
		send_to_char("What are you talking about?!?\n\r", ch);
		return;
	}

	if (is_safe(ch, vch) || vch->in_room == NULL) {
		send_to_char("You failed to displace yourself to your victim.\n\r", ch);
		return;
	}

	if ((wpn = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		send_to_char("You need a weapon to displace.\n\r", ch);
		return;
	}


	if (skill > number_percent() || (skill >= 2 && !IS_AWAKE(vch))) {
		act("The `8Sh`7a`8d`7o`8ws`` come alive and envelop $n!", ch, NULL, NULL, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, vch->in_room);

		act("The `8Sh`7a`8d`7o`8ws`` in the room swirl furious and $n emerges with weapon in hand!!", ch, NULL, NULL, TO_ROOM);
		act("You summon the power of `8Sh`7a`8d`7o`8w`` to displace you to your victim.", ch, NULL, NULL, TO_CHAR);

		/* this call to is_safe should always return false, but never hurts to be safe. */
		if (!is_safe(ch, vch)) {
			check_improve(ch, gsp_displace, TRUE, 1);
			multi_hit(ch, vch, gsp_displace->sn);
			vorpal_effect(ch, vch, wpn);

			if (gsp_blindness != NULL
			    && !is_affected(ch, gsp_blindness)
			    && !saves_spell(ch->level + 5, vch, DAM_SLASH)) {
				af.where = TO_AFFECTS;
				af.type = gsp_blindness->sn;
				af.skill = gsp_blindness;
				af.level = ch->level;
				af.duration = (int)number_range(0, 4);
				af.location = 0;
				af.modifier = 0;
				af.bitvector = AFF_BLIND;
				affect_to_char(vch, &af);

				act("You have been blinded!", ch, NULL, vch, TO_VICT);
				act("$n blinds $N!", ch, NULL, vch, TO_ROOM);
				send_to_char("You blind your victim!!\n\r", ch);
			}

			if (gsp_fear != NULL
			    && !is_affected(ch, gsp_fear)
			    && !saves_spell(ch->level + 5, vch, DAM_SLASH)) {
				af.where = TO_AFFECTS;
				af.type = gsp_fear->sn;
				af.skill = gsp_fear;
				af.level = ch->level;
				af.duration = (int)number_range(0, 4);
				af.location = 0;
				af.modifier = 0;
				af.bitvector = 0;
				affect_to_char(vch, &af);

				act("$n strikes fear into your heart!", ch, NULL, vch, TO_VICT);
				act("$n scares the hell out of $N!", ch, NULL, vch, TO_ROOM);
				send_to_char("You stike fear into the heart of your victim!!\n\r", ch);
			}

			/* add other affects here */

			WAIT_STATE(ch, gsp_displace->wait);
			check_killer(ch, vch);
		} else {
			send_to_char("Something prevents you from attacking your prey.\n\r", ch);
			WAIT_STATE(ch, gsp_displace->wait / 2);
		}
	} else {
		send_to_char("You failed to displace yourself to your victim.\n\r", ch);
		WAIT_STATE(ch, gsp_displace->wait / 2);
	}


	return;
}
