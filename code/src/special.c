/**************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,        *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael          *
 *   Chastain, Michael Quan, and Mitchell Tse.                              *
 *	                                                                       *
 *   In order to use any part of this Merc Diku Mud, you must comply with   *
 *   both the original Diku license in 'license.doc' as well the Merc	   *
 *   license in 'license.txt'.  In particular, you may not remove either of *
 *   these copyright notices.                                               *
 *                                                                             *
 *   Much time and thought has gone into this software and you are          *
 *   benefitting.  We hope that you share your changes too.  What goes      *
 *   around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor(rtaylor@hypercube.org)                                *
*       Gabrielle Taylor(gtaylor@hypercube.org)                           *
*       Brian Moore(zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"

/* command procedures needed */
extern DECLARE_DO_FUN(do_yell);
extern DECLARE_DO_FUN(do_open);
extern DECLARE_DO_FUN(do_close);
extern DECLARE_DO_FUN(do_say);
extern DECLARE_DO_FUN(do_flee);
extern DECLARE_DO_FUN(do_order);
extern DECLARE_DO_FUN(do_shout);
extern DECLARE_DO_FUN(do_murder);
extern DECLARE_DO_FUN(do_sit);
extern DECLARE_DO_FUN(do_cast);
extern DECLARE_DO_FUN(do_emote);
extern DECLARE_DO_FUN(do_push);
extern DECLARE_DO_FUN(do_banzai);
extern DECLARE_DO_FUN(do_transfer);
extern DECLARE_DO_FUN(do_peace);
extern DECLARE_DO_FUN(do_drop);
extern DECLARE_DO_FUN(do_sacrifice);
extern DECLARE_DO_FUN(do_eat);

/*
 * The following special functions are available for mobiles.
 */
static DECLARE_SPEC_FUN(spec_snake_charm);
static DECLARE_SPEC_FUN(spec_breath_any);
static DECLARE_SPEC_FUN(spec_breath_acid);
static DECLARE_SPEC_FUN(spec_breath_fire);
static DECLARE_SPEC_FUN(spec_breath_frost);
static DECLARE_SPEC_FUN(spec_italian_guy);
static DECLARE_SPEC_FUN(spec_breath_gas);
static DECLARE_SPEC_FUN(spec_breath_lightning);
static DECLARE_SPEC_FUN(spec_cast_adept);
static DECLARE_SPEC_FUN(spec_cast_cleric);
static DECLARE_SPEC_FUN(spec_cast_judge);
static DECLARE_SPEC_FUN(spec_cast_mage);
static DECLARE_SPEC_FUN(spec_cast_undead);
static DECLARE_SPEC_FUN(spec_executioner);
static DECLARE_SPEC_FUN(spec_fido);
static DECLARE_SPEC_FUN(spec_guard);
static DECLARE_SPEC_FUN(spec_janitor);
static DECLARE_SPEC_FUN(spec_mayor);
static DECLARE_SPEC_FUN(spec_killa);
static DECLARE_SPEC_FUN(spec_rmove);
static DECLARE_SPEC_FUN(spec_poison);
static DECLARE_SPEC_FUN(spec_thief);
static DECLARE_SPEC_FUN(spec_nasty);
static DECLARE_SPEC_FUN(spec_troll_member);
static DECLARE_SPEC_FUN(spec_ogre_member);
static DECLARE_SPEC_FUN(spec_patrolman);
static DECLARE_SPEC_FUN(spec_teleport);
static DECLARE_SPEC_FUN(spec_buddha);
static DECLARE_SPEC_FUN(spec_teacher);
static DECLARE_SPEC_FUN(spec_guard_white);
static DECLARE_SPEC_FUN(spec_kungfu_poison);
static DECLARE_SPEC_FUN(spec_taxidermist);
static DECLARE_SPEC_FUN(spec_grapez);
static DECLARE_SPEC_FUN(spec_huge_guard);
static DECLARE_SPEC_FUN(spec_dealer_smurf);
static DECLARE_SPEC_FUN(spec_intelligent);

extern bool remove_obj  args((CHAR_DATA * ch, int iWear, bool fReplace));
extern void wear_obj            args((CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace));
extern int check_dir           args((CHAR_DATA * ch, int dir));

/* the function table */
const struct spec_type spec_table[] =
{
	{ "spec_breath_any",	   spec_breath_any	 },
	{ "spec_breath_acid",	   spec_breath_acid	 },
	{ "spec_breath_fire",	   spec_breath_fire	 },
	{ "spec_breath_frost",	   spec_breath_frost	 },
	{ "spec_breath_gas",	   spec_breath_gas	 },
	{ "spec_breath_lightning", spec_breath_lightning },
	{ "spec_snake_charm",	   spec_snake_charm	 },
	{ "spec_cast_adept",	   spec_cast_adept	 },
	{ "spec_cast_cleric",	   spec_cast_cleric	 },
	{ "spec_cast_judge",	   spec_cast_judge	 },
	{ "spec_cast_mage",	   spec_cast_mage	 },
	{ "spec_cast_undead",	   spec_cast_undead	 },
	{ "spec_executioner",	   spec_executioner	 },
	{ "spec_fido",		   spec_fido		 },
	{ "spec_italian_guy",	   spec_italian_guy	 },
	{ "spec_guard",		   spec_guard		 },
	{ "spec_janitor",	   spec_janitor		 },
	{ "spec_mayor",		   spec_mayor		 },
	{ "spec_rmove",		   spec_rmove		 },
	{ "spec_killa",		   spec_killa		 },
	{ "spec_poison",	   spec_poison		 },
	{ "spec_thief",		   spec_thief		 },
	{ "spec_nasty",		   spec_nasty		 },
	{ "spec_troll_member",	   spec_troll_member	 },
	{ "spec_ogre_member",	   spec_ogre_member	 },
	{ "spec_patrolman",	   spec_patrolman	 },
	{ "spec_teleport",	   spec_teleport	 },
	{ "spec_buddha",	   spec_buddha		 },
	{ "spec_teacher",	   spec_teacher		 },
	{ "spec_guard_white",	   spec_guard_white	 },
	{ "spec_kungfu_poison",	   spec_kungfu_poison	 },
	{ "spec_taxidermist",	   spec_taxidermist	 },
	{ "spec_grapez",	   spec_grapez		 },
	{ "spec_huge_guard",	   spec_huge_guard	 },
	{ "spec_dealer_smurf",	   spec_dealer_smurf	 },
	{ "spec_intelligent",	   spec_intelligent	 },
	{ NULL,			   NULL			 }
};

/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN *spec_lookup(const char *name)
{
	int idx;

	for (idx = 0; spec_table[idx].name != NULL; idx++) {
		if (LOWER(name[0]) == LOWER(spec_table[idx].name[0])
		    && !str_prefix(name, spec_table[idx].name))
			return spec_table[idx].function;
	}

	return 0;
}

char *spec_name(SPEC_FUN *function)
{
	int idx;

	for (idx = 0; spec_table[idx].function != NULL; idx++)
		if (function == spec_table[idx].function)
			return spec_table[idx].name;

	return NULL;
}

static bool util_mob_cast(CHAR_DATA *ch, char *spell, void *vo, int target, char *argument)
{
	SKILL *skill;

	if ((skill = skill_lookup(spell)) != NULL) {
		cast_spell(ch, skill, ch->level, vo, target, argument);
		return TRUE;
	}

	return FALSE;
}



/*
 * Core procedure for dragons.
 */
static bool util_dragon_breath(CHAR_DATA *ch, char *spell_name)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;

	if (ch->position != POS_FIGHTING)
		return FALSE;


	for (victim = ch->in_room->people; victim != NULL; victim = v_next) {
		v_next = victim->next_in_room;
		if (victim->fighting == ch && number_bits(3) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	return util_mob_cast(ch, spell_name, victim, TARGET_CHAR, "");
}


static bool spec_troll_member(CHAR_DATA *ch)
{
	CHAR_DATA *vch, *victim = NULL;
	int count = 0;
	char *message;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
	    || IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

/* find an ogre to beat up */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (!IS_NPC(vch) || ch == vch)
			continue;

		if (vch->mob_idx->vnum == MOB_VNUM_PATROLMAN)
			return FALSE;

		if (vch->mob_idx->group == GROUP_VNUM_OGRES
		    && ch->level > vch->level - 2 && !is_safe(ch, vch)) {
			if (number_range(0, count) == 0)
				victim = vch;

			count++;
		}
	}

	if (victim == NULL)
		return FALSE;

/* say something, then raise hell */
	switch (number_range(0, 6)) {
	default:
		message = NULL;
		break;
	case 0:
		message = "$n yells 'I've been looking for you, punk!'";
		break;
	case 1:
		message = "With a scream of rage, $n attacks $N.";
		break;
	case 2:
		message =
			"$n says 'What's slimy Ogre trash like you doing around here?'";
		break;
	case 3:
		message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
		break;
	case 4:
		message = "$n says 'There's no cops to save you this time!'";
		break;
	case 5:
		message = "$n says 'Time to join your brother, spud.'";
		break;
	case 6:
		message = "$n says 'Let's rock.'";
		break;
	}

	if (message != NULL)
		act(message, ch, NULL, victim, TO_ALL);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
}

static bool spec_ogre_member(CHAR_DATA *ch)
{
	CHAR_DATA *vch, *victim = NULL;
	int count = 0;
	char *message;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
	    || IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

/* find an troll to beat up */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (!IS_NPC(vch) || ch == vch)
			continue;

		if (vch->mob_idx->vnum == MOB_VNUM_PATROLMAN)
			return FALSE;

		if (vch->mob_idx->group == GROUP_VNUM_TROLLS
		    && ch->level > vch->level - 2 && !is_safe(ch, vch)) {
			if (number_range(0, count) == 0)
				victim = vch;

			count++;
		}
	}

	if (victim == NULL)
		return FALSE;

/* say something, then raise hell */
	switch (number_range(0, 6)) {
	default:
		message = NULL;
		break;
	case 0:
		message = "$n yells 'I've been looking for you, punk!'";
		break;
	case 1:
		message = "With a scream of rage, $n attacks $N.'";
		break;
	case 2:
		message =
			"$n says 'What's Troll filth like you doing around here?'";
		break;
	case 3:
		message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
		break;
	case 4:
		message = "$n says 'There's no cops to save you this time!'";
		break;
	case 5:
		message = "$n says 'Time to join your brother, spud.'";
		break;
	case 6:
		message = "$n says 'Let's rock.'";
		break;
	}

	if (message != NULL)
		act(message, ch, NULL, victim, TO_ALL);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
}

static bool spec_patrolman(CHAR_DATA *ch)
{
	CHAR_DATA *vch, *victim = NULL;
	OBJ_DATA *obj;
	char *message;
	int count = 0;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch, AFF_CALM) || ch->in_room == NULL
	    || IS_AFFECTED(ch, AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

/* look for a fight in the room */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (vch == ch)
			continue;

		if (vch->fighting != NULL) { /* break it up! */
			if (number_range(0, count) == 0)
				victim = (vch->level > vch->fighting->level)
					 ? vch : vch->fighting;
			count++;
		}
	}

	if (victim == NULL || (IS_NPC(victim) && victim->spec_fun == ch->spec_fun))
		return FALSE;

	if (((obj = get_eq_char(ch, WEAR_NECK_1)) != NULL
	     && obj->obj_idx->vnum == OBJ_VNUM_WHISTLE)
	    || ((obj = get_eq_char(ch, WEAR_NECK_2)) != NULL
		&& obj->obj_idx->vnum == OBJ_VNUM_WHISTLE)) {
		act("You blow down hard on $p.", ch, obj, NULL, TO_CHAR);
		act("$n blows on $p, ***WHEEEEEEEEEEEET***", ch, obj, NULL, TO_ROOM);

		for (vch = char_list; vch != NULL; vch = vch->next) {
			if (vch->in_room == NULL)
				continue;

			if (vch->in_room != ch->in_room
			    && vch->in_room->area == ch->in_room->area)
				send_to_char("You hear a shrill whistling sound.\n\r", vch);
		}
	}

	switch (number_range(0, 6)) {
	default:
		message = NULL;
		break;
	case 0:
		message = "$n yells 'All roit! All roit! break it up!'";
		break;
	case 1:
		message =
			"$n says 'Society's to blame, but what's a bloke to do?'";
		break;
	case 2:
		message =
			"$n mumbles 'bloody kids will be the death of us all.'";
		break;
	case 3:
		message = "$n shouts 'Stop that! Stop that!' and attacks.";
		break;
	case 4:
		message = "$n pulls out his billy and goes to work.";
		break;
	case 5:
		message =
			"$n sighs in resignation and proceeds to break up the fight.";
		break;
	case 6:
		message = "$n says 'Settle down, you hooligans!'";
		break;
	}

	if (message != NULL)
		act(message, ch, NULL, NULL, TO_ALL);

	multi_hit(ch, victim, TYPE_UNDEFINED);

	return TRUE;
}


static bool spec_nasty(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	unsigned int gold;

	if (!IS_AWAKE(ch))
		return FALSE;

	if (ch->position != POS_FIGHTING) {
		for (victim = ch->in_room->people; victim != NULL; victim = v_next) {
			v_next = victim->next_in_room;
			if (!IS_NPC(victim)
			    && (victim->level > ch->level)
			    && (victim->level < ch->level + 10)) {
				if (ch->position != POS_FIGHTING)
					do_murder(ch, victim->name);

				return TRUE;
			}
		}
		return FALSE;   /*  No one to attack */
	}

/* okay, we must be fighting.... steal some coins and flee */
	if ((victim = ch->fighting) == NULL)
		return FALSE;   /* let's be paranoid.... */

	switch (number_bits(2)) {
	case 0:
		act("$n rips apart your coin purse, spilling your gold!", ch, NULL, victim, TO_VICT);
		act("You slash apart $N's coin purse and gather his gold.", ch, NULL, victim, TO_CHAR);
		act("$N's coin purse is ripped apart!", ch, NULL, victim, TO_NOTVICT);
		gold = victim->gold / 10;                       /* steal 10% of his gold */
		victim->gold -= gold;
		ch->gold += gold;
		return TRUE;
	case 1:
		do_flee(ch, "");
		return TRUE;
	default:
		return FALSE;
	}
}


static bool spec_teleport(CHAR_DATA *ch)
{
	switch (number_bits(5)) {
	case 3:
		return util_mob_cast(ch, "teleport", ch, TARGET_CHAR, "");
	default:
		return FALSE;
	}
}



/*
 * Special procedures for mobiles.
 */
static bool spec_breath_any(CHAR_DATA *ch)
{
	if (ch->position != POS_FIGHTING)
		return FALSE;

	switch (number_bits(3)) {
	case 0:
		return spec_breath_fire(ch);
	case 1:
	case 2:
		return spec_breath_lightning(ch);
	case 3:
		return spec_breath_gas(ch);
	case 4:
		return spec_breath_acid(ch);
	case 5:
	case 6:
	case 7:
		return spec_breath_frost(ch);
	}

	return FALSE;
}



static bool spec_breath_acid(CHAR_DATA *ch)
{
	return util_dragon_breath(ch, "acid breath");
}



static bool spec_breath_fire(CHAR_DATA *ch)
{
	return util_dragon_breath(ch, "fire breath");
}



static bool spec_breath_frost(CHAR_DATA *ch)
{
	return util_dragon_breath(ch, "frost breath");
}



static bool spec_breath_gas(CHAR_DATA *ch)
{
	return util_dragon_breath(ch, "gas breath");
}



static bool spec_breath_lightning(CHAR_DATA *ch)
{
	return util_dragon_breath(ch, "lightning breath");
}



static bool spec_cast_adept(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	char *spell;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (victim != ch
		    && can_see(ch, victim)
		    && number_bits(1) == 0
		    && !IS_NPC(victim) && victim->level < 11)
			break;
	}

	if (victim == NULL)
		return FALSE;

	if (is_affected(victim, gsp_anti_magic_aura))
		return FALSE;

	switch (number_bits(4)) {
	case 0:
		act("$n utters the word 'abrazak'.", ch, NULL, NULL, TO_ROOM);
		spell = "armor";
		break;
	case 1:
		act("$n utters the word 'fido'.", ch, NULL, NULL, TO_ROOM);
		spell = "bless";
		break;
	case 2:
		act("$n utters the words 'judicandus noselacri'.", ch, NULL, NULL, TO_ROOM);
		spell = "cure blindness";
		break;
	case 3:
		act("$n utters the words 'judicandus dies'.", ch, NULL, NULL, TO_ROOM);
		spell = "cure light";
		break;
	case 4:
		act("$n utters the words 'judicandus sausabru'.", ch, NULL, NULL, TO_ROOM);
		spell = "cure poison";
		break;
	case 5:
		act("$n utters the word 'candusima'.", ch, NULL, NULL, TO_ROOM);
		spell = "refresh";
		break;
	case 6:
		act("$n utters the words 'judicandus eugzagz'.", ch, NULL, NULL, TO_ROOM);
		spell = "cure disease";
		break;
	default:
		return FALSE;
	}

	return util_mob_cast(ch, spell, victim, TARGET_CHAR, "");
}



static bool spec_cast_cleric(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	char *spell;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (victim->fighting == ch
		    && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	if (is_affected(victim, gsp_anti_magic_aura))
		return FALSE;

	for (;; ) {
		int min_level;

		switch (number_bits(4)) {
		case 0:
			min_level = 0;
			spell = "blindness";
			break;
		case 1:
			min_level = 3;
			spell = "plague";
			break;
		case 2:
			min_level = 7;
			spell = "blindness";
			break;
		case 3:
			min_level = 9;
			spell = "ring of fire";
			break;
		case 4:
			min_level = 10;
			spell = "black plague";
			break;
		case 5:
			min_level = 12;
			spell = "curse";
			break;
		case 6:
			min_level = 12;
			spell = "change sex";
			break;
		case 7:
			min_level = 13;
			spell = "fireball";
			break;
		case 8:
		case 9:
		case 10:
			min_level = 15;
			spell = "psychic headbutt";
			break;
		case 11:
			min_level = 15;
			spell = "plague";
			break;
		default:
			min_level = 16;
			spell = "dispel magic";
			break;
		}

		if (ch->level >= min_level)
			break;
	}

	return util_mob_cast(ch, spell, victim, TARGET_CHAR, "");
}


/***************************************************************************
*	spec_cast_judge
***************************************************************************/
static bool spec_cast_judge(CHAR_DATA *ch)
{
	CHAR_DATA *victim;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (victim->fighting == ch
		    && number_bits(2) == 0)
			break;
	}

	if (victim == NULL)
		return FALSE;

	if (is_affected(victim, gsp_anti_magic_aura))
		return FALSE;

	return util_mob_cast(ch, "high explosive", victim, TARGET_CHAR, "");
}


/***************************************************************************
*	spec_cast_mage
***************************************************************************/
static bool spec_cast_mage(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	char *spell;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
		if (victim->fighting == ch && number_bits(2) == 0)
			break;

	if (victim == NULL)
		return FALSE;

	if (is_affected(victim, gsp_anti_magic_aura))
		return FALSE;

	for (;; ) {
		int min_level;

		switch (number_bits(4)) {
		case 0:
			min_level = 0;
			spell = "blindness";
			break;
		case 1:
			min_level = 3;
			spell = "chill touch";
			break;
		case 2:
			min_level = 7;
			spell = "weaken";
			break;
		case 3:
			min_level = 8;
			spell = "teleport";
			break;
		case 4:
			min_level = 11;
			spell = "colour spray";
			break;
		case 5:
			min_level = 12;
			spell = "change sex";
			break;
		case 6:
			min_level = 13;
			spell = "energy drain";
			break;
		case 7:
		case 8:
		case 9:
			min_level = 15;
			spell = "fireball";
			break;
		case 10:
			min_level = 20;
			spell = "plague";
			break;
		default:
			min_level = 20;
			spell = "acid blast";
			break;
		}

		if (ch->level >= min_level)
			break;
	}

	return util_mob_cast(ch, spell, victim, TARGET_CHAR, "");
}


/***************************************************************************
*	spec_cast_undead
***************************************************************************/
static bool spec_cast_undead(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	char *spell;

	if (ch->position != POS_FIGHTING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
		if (victim->fighting == ch && number_bits(2) == 0)
			break;

	if (victim == NULL)
		return FALSE;

	if (is_affected(victim, gsp_anti_magic_aura))
		return FALSE;


	for (;; ) {
		int min_level;

		switch (number_bits(4)) {
		case 0:
			min_level = 0;
			spell = "curse";
			break;
		case 1:
			min_level = 3;
			spell = "weaken";
			break;
		case 2:
			min_level = 6;
			spell = "chill touch";
			break;
		case 3:
			min_level = 9;
			spell = "blindness";
			break;
		case 4:
			min_level = 12;
			spell = "poison";
			break;
		case 5:
			min_level = 15;
			spell = "energy drain";
			break;
		case 6:
			min_level = 18;
			spell = "harm";
			break;
		case 7:
			min_level = 21;
			spell = "teleport";
			break;
		case 8:
			min_level = 20;
			spell = "plague";
			break;
		default:
			min_level = 18;
			spell = "harm";
			break;
		}

		if (ch->level >= min_level)
			break;
	}

	return util_mob_cast(ch, spell, victim, TARGET_CHAR, "");
}

/***************************************************************************
*	spec_executioner
***************************************************************************/
static bool spec_executioner(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	char buf[MSL];
	char *crime;

	if (!IS_AWAKE(ch) || ch->fighting != NULL)
		return FALSE;

	crime = "";
	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (!IS_NPC(victim)
		    && IS_SET(victim->act, PLR_KILLER)
		    && can_see(ch, victim)) {
			crime = "KILLER";
			break;
		}

		if (!IS_NPC(victim)
		    && IS_SET(victim->act, PLR_THIEF)
		    && can_see(ch, victim)) {
			crime = "THIEF";
			break;
		}
	}

	if (victim == NULL)
		return FALSE;

	sprintf(buf, "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!", victim->name, crime);
	REMOVE_BIT(ch->comm, COMM_NOSHOUT);
	do_yell(ch, buf);
	multi_hit(ch, victim, TYPE_UNDEFINED);
	return TRUE;
}


/***************************************************************************
*	spec_fido
***************************************************************************/
static bool spec_fido(CHAR_DATA *ch)
{
	OBJ_DATA *corpse;
	OBJ_DATA *c_next;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (corpse = ch->in_room->contents; corpse != NULL; corpse = c_next) {
		c_next = corpse->next_content;
		if (corpse->item_type != ITEM_CORPSE_NPC)
			continue;

		act("$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM);
		for (obj = corpse->contains; obj; obj = obj_next) {
			obj_next = obj->next_content;
			obj_from_obj(obj);
			obj_to_room(obj, ch->in_room);
		}
		extract_obj(corpse);
		return TRUE;
	}

	return FALSE;
}



/***************************************************************************
*	spec_guard
***************************************************************************/
static bool spec_guard(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *ech;
	char buf[MSL];
	char *crime;
	int max_evil;

	if (!IS_AWAKE(ch) || ch->fighting != NULL)
		return FALSE;

	max_evil = 300;
	ech = NULL;
	crime = "";

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (!IS_NPC(victim)
		    && IS_SET(victim->act, PLR_KILLER)
		    && can_see(ch, victim)) {
			crime = "KILLER";
			break;
		}

		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)
		    && can_see(ch, victim)) {
			crime = "THIEF";
			break;
		}

		if (victim->fighting != NULL
		    && victim->fighting != ch
		    && victim->alignment < max_evil) {
			max_evil = victim->alignment;
			ech = victim;
		}
	}

	if (victim != NULL) {
		sprintf(buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!", victim->name, crime);
		REMOVE_BIT(ch->comm, COMM_NOSHOUT);
		do_yell(ch, buf);
		multi_hit(ch, victim, TYPE_UNDEFINED);
		return TRUE;
	}

	if (ech != NULL) {
		act("$n screams 'PROTECT THE INNOCENT!!  BANZAI!!", ch, NULL, NULL, TO_ROOM);
		multi_hit(ch, ech, TYPE_UNDEFINED);
		return TRUE;
	}

	return FALSE;
}



/***************************************************************************
*	spec_janitor
***************************************************************************/
static bool spec_janitor(CHAR_DATA *ch)
{
	OBJ_DATA *trash;
	OBJ_DATA *trash_next;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (trash = ch->in_room->contents; trash != NULL; trash = trash_next) {
		trash_next = trash->next_content;

		if (!IS_SET(trash->wear_flags, ITEM_TAKE)
		    || !can_loot(ch, trash)
		    || (count_users(trash) > 0))
			continue;

		if (trash->item_type == ITEM_DRINK_CON
		    || trash->item_type == ITEM_TRASH
		    || trash->cost < 10) {
			act("$n picks up some trash.", ch, NULL, NULL, TO_ROOM);
			obj_from_room(trash);
			obj_to_char(trash, ch);
			return TRUE;
		}
	}

	return FALSE;
}

static bool spec_rmove(CHAR_DATA *ch)
{
	static int rnum;

	rnum = number_range(1, 6);

	if (rnum == 1) move_char(ch, check_dir(ch, DIR_NORTH), FALSE);
	if (rnum == 2) move_char(ch, check_dir(ch, DIR_SOUTH), FALSE);
	if (rnum == 3) move_char(ch, check_dir(ch, DIR_EAST), FALSE);
	if (rnum == 4) move_char(ch, check_dir(ch, DIR_WEST), FALSE);
	if (rnum == 5) move_char(ch, check_dir(ch, DIR_UP), FALSE);
	if (rnum == 6) move_char(ch, check_dir(ch, DIR_DOWN), FALSE);

	return FALSE;
}

/***************************************************************************
*	spec_mayor
***************************************************************************/
static bool spec_mayor(CHAR_DATA *ch)
{
	static const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
	static const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";
	static const char *path;
	static int pos;
	static bool move;

	if (!move) {
		if (time_info.hour == 6) {
			path = open_path;
			move = TRUE;
			pos = 0;
		}

		if (time_info.hour == 20) {
			path = close_path;
			move = TRUE;
			pos = 0;
		}
	}

	if (ch->fighting != NULL)
		return spec_cast_mage(ch);

	if (!move || ch->position < POS_SLEEPING)
		return FALSE;

	switch (path[pos]) {
	case '0':
	case '1':
	case '2':
	case '3':
		move_char(ch, (int)(path[pos] - '0'), FALSE);
		break;
	case 'W':
		ch->position = POS_STANDING;
		act("$n awakens and groans loudly.(`1Hangover Aura``)", ch, NULL, NULL, TO_ROOM);
		break;
	case 'S':
		ch->position = POS_SLEEPING;
		act("$n lies down and falls asleep.(`8D`2u`8S`@t`8E`2d `PAura``)", ch, NULL, NULL, TO_ROOM);
		break;
	case 'a':
		act("$n says 'Hello Bia-chiz-na-cheeze-wiz-cheeto-chester-tha-cheetah-cheech-and-chong-swatch-watch-crotch-tea-with-ligity-lime-chach!'", ch, NULL, NULL, TO_ROOM);
		break;
	case 'b':
		act("$n says 'Someone just took a big dump five south from the healer, go look!!'", ch, NULL, NULL, TO_ROOM);
		break;
	case 'c':
		act("$n says 'All of these annoying messages are brought to you by CRAVEN MOOREHEAD, BEN DOVER, and PHIL McCREVIS.'", ch, NULL, NULL, TO_ROOM);
		break;
	case 'd':
		act("$n says 'Ohhhhhhhhhhh what a hangover, this SUCKS.'", ch, NULL, NULL, TO_ROOM);
		break;
	case 'e':
		act("$n says 'I fucking despise you, you piece of shit!!'", ch, NULL, NULL, TO_ROOM);
		break;
	case 'E':
		act("$n says 'God damn this town smells like crap!!'", ch, NULL, NULL, TO_ROOM);
		break;
	case 'O':
		do_open(ch, "gate");
		break;
	case 'C':
		do_close(ch, "gate");
		break;
	case '.':
		move = FALSE;
		break;
	}

	pos++;
	return FALSE;
}

/***************************************************************************
*	spec_killa
***************************************************************************/
static bool spec_killa(CHAR_DATA *ch)
{
	CHAR_DATA *vch;
	char *spell_name;
	int rand;
	bool success;

	if (ch->fighting != NULL) {
		vch = ch->fighting;
		success = TRUE;
		spell_name = NULL;
		rand = number_range(1, 10);
		switch (rand) {
		case (1):
			spell_name = "ring of fire";
			break;
		case (2):
			spell_name = "cloud of death";
			break;
		case (3):
			spell_name = "blood boil";
			break;
		case (4):
			spell_name = "gas breath";
			break;
		case (5):
			spell_name = "acid breath";
			break;
		case (6):
			spell_name = "lightning breath";
			break;
		case (7):
			spell_name = "psychic headbutt";
			break;
		case (8):
			spell_name = "teleport";
			break;
		case (9):
			spell_name = "revive";
			break;
		case (10):
			spell_name = "acid blast";
			break;
		}

		if (spell_name != NULL)
			util_mob_cast(ch, spell_name, vch, TARGET_CHAR, "");
	} else {
		success = FALSE;
	}

	return success;
}

/***************************************************************************
*	spec_poison
***************************************************************************/
static bool spec_poison(CHAR_DATA *ch)
{
	CHAR_DATA *victim;

	if (ch->position != POS_FIGHTING
	    || (victim = ch->fighting) == NULL
	    || number_percent() > 2 * ch->level)
		return FALSE;


	act("You bite $N!", ch, NULL, victim, TO_CHAR);
	act("$n bites $N!", ch, NULL, victim, TO_NOTVICT);
	act("$n bites you!", ch, NULL, victim, TO_VICT);

	util_mob_cast(ch, "poison", victim, TARGET_CHAR, "");
	return TRUE;
}


/***************************************************************************
*	spec_thief
***************************************************************************/
static bool spec_thief(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	unsigned int gold;
	unsigned int silver;

	if (ch->position != POS_STANDING)
		return FALSE;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (IS_NPC(victim)
		    || victim->level >= LEVEL_IMMORTAL
		    || number_bits(5) != 0
		    || !can_see(ch, victim))
			continue;

		if (IS_AWAKE(victim) && number_range(0, ch->level) == 0) {
			act("You discover $n's hands in your wallet!  What a fucker!", ch, NULL, victim, TO_VICT);
			act("$N discovers $n's hands in $S wallet!  What a fucker!", ch, NULL, victim, TO_NOTVICT);
			return TRUE;
		} else {
			gold = victim->gold * UMIN(number_range(1, 20), ch->level / 2) / 100;
			gold = UMIN(gold, (unsigned int)(ch->level * ch->level * 10));
			ch->gold += gold;
			victim->gold -= gold;

			silver = victim->silver * UMIN((unsigned int)number_range(1, 20), (unsigned int)(ch->level / 2)) / 100;
			silver = UMIN(silver, (unsigned int)(ch->level * ch->level * 25));
			ch->silver += silver;
			victim->silver -= silver;

			return TRUE;
		}
	}

	return FALSE;
}


/***************************************************************************
*	spec_buddha
***************************************************************************/
static bool spec_buddha(CHAR_DATA *ch)
{
	if (ch->position != POS_FIGHTING)
		return FALSE;

	switch (number_bits(3)) {
	case 0:
		return spec_breath_fire(ch);
	case 1:
	case 2:
		return spec_breath_lightning(ch);
	case 3:
		return spec_breath_gas(ch);
	case 4:
		return spec_breath_acid(ch);
	case 5:
		return spec_cast_cleric(ch);
	case 6:
	case 7:
		return spec_breath_frost(ch);
	}

	return FALSE;
}


/***************************************************************************
*	spec_teacher
***************************************************************************/
static bool spec_teacher(CHAR_DATA *ch)
{
	OBJ_DATA *corpse;
	OBJ_DATA *c_next;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if (!IS_AWAKE(ch))
		return FALSE;

	for (corpse = ch->in_room->contents; corpse != NULL; corpse = c_next) {
		c_next = corpse->next_content;
		if (corpse->item_type != ITEM_CORPSE_NPC)
			continue;

		act("$n sacrifices the corpse to Buddha!", ch, NULL, NULL, TO_ROOM);
		for (obj = corpse->contains; obj; obj = obj_next) {
			obj_next = obj->next_content;
			obj_from_obj(obj);
			obj_to_room(obj, ch->in_room);
		}
		extract_obj(corpse);
		return TRUE;
	}

	return FALSE;
}


/***************************************************************************
*	spec_guard_white
***************************************************************************/
static bool spec_guard_white(CHAR_DATA *ch)
{
	char buf[MSL];
	CHAR_DATA *victim;
	CHAR_DATA *ech;
	char *crime;
	int max_evil;

	if (!IS_AWAKE(ch) || ch->fighting != NULL)
		return FALSE;

	max_evil = 300;
	ech = NULL;
	crime = "";

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER)) {
			crime = "KILLER";
			break;
		}

		if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)) {
			crime = "THIEF";
			break;
		}

		if (victim->fighting != NULL
		    && victim->fighting != ch
		    && victim->alignment < max_evil) {
			max_evil = victim->alignment;
			ech = victim;
		}
	}

	if (victim != NULL) {
		sprintf(buf, "%s is a %s!  How DARE you come to the Temple!!!!", victim->name, crime);
		do_shout(ch, buf);
		multi_hit(ch, victim, TYPE_UNDEFINED);
		return TRUE;
	}

	if (ech != NULL) {
		act("$n screams '`!Now you DIE you Bastard!!!!``'", ch, NULL, NULL, TO_ROOM);
		multi_hit(ch, ech, TYPE_UNDEFINED);
		return TRUE;
	}

	return FALSE;
}


/***************************************************************************
*	spec_kungfu_poison
***************************************************************************/
static bool spec_kungfu_poison(CHAR_DATA *ch)
{
	CHAR_DATA *victim;

	if (ch->position != POS_FIGHTING
	    || (victim = ch->fighting) == NULL
	    || number_percent() > 2 * ch->level)
		return FALSE;

	act("You hit $N with a poison palm technique!", ch, NULL, victim, TO_CHAR);
	act("$n hits $N with the poison palm technique!", ch, NULL, victim, TO_NOTVICT);
	act("$n hits you with the poison palm technique!", ch, NULL, victim, TO_VICT);

	return util_mob_cast(ch, "poison", victim, 0, "");
}


/***************************************************************************
*	spec_taxidermist
***************************************************************************/
static bool spec_taxidermist(CHAR_DATA *ch)
{
	OBJ_DATA *inv;

	if (ch->position != POS_STANDING)
		return FALSE;

	if (ch->mob_idx->shop == NULL
	    || time_info.hour < ch->mob_idx->shop->open_hour
	    || time_info.hour > ch->mob_idx->shop->close_hour)
		return FALSE;

	for (inv = ch->carrying; inv != NULL; inv = inv->next_content) {
		if (inv->item_type == ITEM_CORPSE_NPC) {
			util_mob_cast(ch, "make bag", inv, TARGET_CHAR, "");
			return TRUE;
		} else if (inv->wear_loc == WEAR_NONE && number_percent() < 5) {
			act("$n suggests you buy $p.", ch, inv, NULL, TO_ROOM);
			return TRUE;
		}
	}

	return FALSE;
}

/***************************************************************************
*	spec_grapez
***************************************************************************/
static bool spec_grapez(CHAR_DATA *ch)
{
	static const char open_path[] = "a32bcdd.";
	static const char close_path[] = "defg01h.";
	static const char *path;
	static int pos;
	static bool move;

	if (!move) {
		if (time_info.hour == 10) {
			path = open_path;
			move = TRUE;
			pos = 0;
		}

		if (time_info.hour == 15) {
			path = close_path;
			move = TRUE;
			pos = 0;
		}
	}

	if (ch->fighting != NULL)
		return spec_cast_mage(ch);

	if (!move || ch->position < POS_SLEEPING)
		return FALSE;

	switch (path[pos]) {
	case '0':
	case '1':
	case '2':
	case '3':
		move_char(ch, (int)(path[pos] - '0'), FALSE);
		break;
	case 'a':
		ch->position = POS_STANDING;
		act("$n stands up and says '`PI gotta drop a log!!``'\n\r", ch, NULL, NULL, TO_ROOM);
		break;
	case 'b':
		do_close(ch, "north");
		break;
	case 'c':
		do_sit(ch, "toilet");
		break;
	case 'd':
		act("$n grimaces as he pinches a big loaf.\n\r", ch, NULL, NULL, TO_ROOM);
		break;
	case 'e':
		ch->position = POS_STANDING;
		act("$n stands up, flushes, and washes his hands.\n\r", ch, NULL, NULL, TO_ROOM);

	case 'f':
		do_open(ch, "north");
		break;

	case 'g':
		act("$n says '`Pman, I feel 10 pounds lIghter!``'\n\r", ch, NULL, NULL, TO_ROOM);
		break;
	case 'h':
		act("$n stoned.\n\r", ch, NULL, NULL, TO_ROOM);
		break;
	case '.':
		move = FALSE;
		break;
	}

	pos++;
	return FALSE;
}



/***************************************************************************
*	spec_huge_guard
***************************************************************************/
static bool spec_huge_guard(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int counter = 0;
	int temp = 0;
	char insult[20];
	char buf[MSL];

	ch->perm_stat[STAT_STR] = 25;
	counter = number_range(1, 11);

	if (!IS_SET(ch->act, ACT_SENTINEL))
		SET_BIT(ch->act, ACT_SENTINEL);

	if (ch->in_room != get_room_index(ROOM_VNUM_ALTAR))
		do_transfer(ch, "self 3054");

	for (victim = ch->in_room->people; victim != NULL; victim = v_next) {
		v_next = victim->next_in_room;
		if (!IS_NPC(victim)) {
			if ((IS_SET(victim->act, PLR_IDIOT))
			    && (can_see(ch, victim))) {
				victim->position = POS_STANDING;
				temp = victim->perm_stat[STAT_STR];
				victim->perm_stat[STAT_STR] = 1;

				switch (counter) {
				case 1:
					sprintf(insult, "fucker");
					break;
				case 2:
					sprintf(insult, "assmunch");
					break;
				case 3:
					sprintf(insult, "bunghole");
					break;
				case 4:
					sprintf(insult, "you twit");
					break;
				case 5:
					sprintf(insult, "weasel");
					break;
				case 6:
					sprintf(insult, "frognogger");
					break;
				case 7:
					sprintf(insult, "stupid");
					break;
				case 8:
					sprintf(insult, "buffoon");
					break;
				case 9:
					sprintf(insult, "bonehead");
					break;
				case 10:
					sprintf(insult, "dork");
					break;
				default:
					sprintf(insult, "yogurt-head");
					break;
				}

				sprintf(buf, "growls '`PGet out of here, %s!! We don't want `#idiots`P here!``'\n\r", insult);
				do_emote(ch, buf);
				sprintf(buf, "%s south", victim->name);
				do_push(ch, buf);

				victim->perm_stat[STAT_STR] = temp;
				return TRUE;
			}
		}
	}

	return FALSE;
}

/***************************************************************************
*	spec_dealer_smurf
***************************************************************************/
static bool spec_dealer_smurf(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	int message;

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) {
		if (!IS_NPC(victim)) {
			message = number_range(0, 21);

			if (message == 4) {
				printf_to_char(victim, "`@%s tells you '`1PSSST! Wanna buy some smurfy buds?`@'``\n\r", ch->short_descr);
				return TRUE;
			}

			if (message == 8) {
				printf_to_char(victim, "`@%s tells you '`1I got the killer shit, mang!`@'``\n\r", ch->short_descr);
				return TRUE;
			}

			if (message == 12) {
				printf_to_char(victim, "`@%s tells you '`1Wow, you should try this killer Smurfbud, man!`@'``\n\r", ch->short_descr);
				return TRUE;
			}

			if (message == 20) {
				printf_to_char(victim, "`@%s tells you '`1This shit'll really SMURF you up!`@'``\n\r", ch->short_descr);
				return TRUE;
			}
		}
	}
	return FALSE;
}


/***************************************************************************
*	spec_italian_guy
***************************************************************************/
static bool spec_italian_guy(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int message;
	char buf[MSL];

	for (victim = ch->in_room->people; victim != NULL; victim = v_next) {
		v_next = victim->next_in_room;

		if (!IS_NPC(victim)) {
			message = number_range(0, 21);

			if (message == 4) {
				sprintf(buf, "%s says '`PYou like-a slice of lasagna?``'\n\r", ch->short_descr);
				send_to_char(buf, victim);
				return TRUE;
			}

			if (message == 8) {
				sprintf(buf, "%s says '`PMy calzone is better than your mama's!``'\n\r", ch->short_descr);
				send_to_char(buf, victim);
				return TRUE;
			}

			if (message == 12) {
				sprintf(buf, "%s says '`PEat-a my pizza or I break-a you face!``'\n\r", ch->short_descr);
				send_to_char(buf, victim);
				return TRUE;
			}

			if (message == 20) {
				sprintf(buf, "%s hums a tune and tosses a pizza into the air.\n\r", ch->short_descr);
				send_to_char(buf, victim);
				return TRUE;
			}
		}
	}
	return FALSE;
}


/***************************************************************************
*	spec_snake_charm
***************************************************************************/
static bool spec_snake_charm(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	char buf[MSL];

	if (ch->position != POS_FIGHTING) {
		switch (number_bits(3)) {
		case 0:
			sprintf(buf, "all sing %s", ch->name);
			do_order(ch, buf);      /* a chance to get free here */
			break;
		case 1:
			sprintf(buf, "all goss %s is pretty cool. I'm getting a lot of experience really fast!", ch->short_descr);
			do_order(ch, buf);
			break;
		case 2:
			sprintf(buf, "all goss 'YES!  I just got 1327xp for killing %s!", ch->short_descr);
			do_order(ch, buf);
			break;
		case 3:
			sprintf(buf, "all give dagger %s", ch->name);
			do_order(ch, "all remove dagger");
			do_order(ch, buf);
			break;
		case 4:
			sprintf(buf, "all give sword %s", ch->name);
			do_order(ch, "all remove sword");
			do_order(ch, buf);
			break;
		case 5:
			sprintf(buf, "all give mace %s", ch->name);
			do_order(ch, "all remove mace");
			do_order(ch, buf);
			break;
		case 6:
			do_order(ch, "all drop all");
			break;
		case 7:
			sprintf(buf, "all cast 'cure serious' %s", ch->name);
			do_order(ch, buf);
			break;
		}

		return TRUE;
	}

	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
		if (victim->fighting == ch && number_bits(2) == 0)
			break;

	if (victim == NULL)
		return FALSE;

	if (is_affected(victim, gsp_anti_magic_aura))
		return FALSE;

	act("$n begins playing a new, beautiful song.", ch, NULL, NULL, TO_ROOM);
	util_mob_cast(ch, "charm person", victim, TAR_CHAR_OFFENSIVE, "");


	if (IS_AFFECTED(victim, AFF_CHARM))
		do_peace(ch, NULL);

	return TRUE;
}


/***************************************************************************
*	spec_intelligent
***************************************************************************/
static bool spec_intelligent(CHAR_DATA *ch)
{
	OBJ_DATA *object;
	OBJ_DATA *obj2;
	OBJ_DATA *object_next;
	char buf[MSL];

	if (!IS_AWAKE(ch))
		return FALSE;

	for (object = ch->in_room->contents; object; object = object_next) {
		object_next = object->next_content;
		if (object == NULL)
			continue;

		if (!IS_SET(object->wear_flags, ITEM_TAKE))
			continue;

		if (object->item_type == ITEM_CORPSE_NPC)
			continue;

		if (count_users(object) > 0)
			continue;

		if ((object->item_type != ITEM_DRINK_CON
		     && object->item_type != ITEM_TRASH)
		    && !((IS_OBJ_STAT(object, ITEM_ANTI_EVIL) && IS_EVIL(ch))
			 || (IS_OBJ_STAT(object, ITEM_ANTI_GOOD) && IS_GOOD(ch))
			 || (IS_OBJ_STAT(object, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))) {
			act("$n picks up $p and examines it carefully.", ch, object, NULL, TO_ROOM);
			obj_from_room(object);
			obj_to_char(object, ch);

			/*Now compare it to what we already have */
			for (obj2 = ch->carrying; obj2; obj2 = obj2->next_content) {
				if (obj2->wear_loc != WEAR_NONE
				    && can_see_obj(ch, obj2)
				    && object->item_type == obj2->item_type
				    && (object->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
					break;
			}

			if (!obj2) {
				switch (object->item_type) {
				default:
					strcpy(buf, "Hey, what a find!");
					do_say(ch, buf);
					break;
				case ITEM_CORPSE_PC:
					strcpy(buf, "Finally!  Some revenge!");
					do_get(ch, "all corpse");
					do_drop(ch, "corpse");
					do_wear(ch, "all");
					break;
				case ITEM_FOOD:
					strcpy(buf, "This looks like a tasty morsel!");
					do_say(ch, buf);
					do_eat(ch, object->name);
					break;
				case ITEM_WAND:
					strcpy(buf, "Wow, a magic wand!");
					do_say(ch, buf);
					wear_obj(ch, object, FALSE);
					break;
				case ITEM_STAFF:
					strcpy(buf, "Kewl, a magic staff!");
					do_say(ch, buf);
					wear_obj(ch, object, FALSE);
					break;
				case ITEM_WEAPON:
					strcpy(buf, "Hey, this looks like a nifty weapon!");
					do_say(ch, buf);
					wear_obj(ch, object, FALSE);
					break;
				case ITEM_ARMOR:
					strcpy(buf, "Oooh...a nice piece of armor!");
					do_say(ch, buf);
					wear_obj(ch, object, FALSE);
					break;
				}

				return TRUE;
			}

			if ((object->level > obj2->level)) {
				strcpy(buf, "Now THIS looks like an improvement!");
				do_say(ch, buf);
				remove_obj(ch, obj2->wear_loc, TRUE);
				wear_obj(ch, object, FALSE);
			} else {
				strcpy(buf, "I don't want this piece of junk!");
				do_say(ch, buf);
				act("You don't like the look of $p.", ch, object, NULL, TO_CHAR);
				do_drop(ch, object->name);
				do_sacrifice(ch, object->name);
			}
			return TRUE;
		}
	}

	return FALSE;
}
