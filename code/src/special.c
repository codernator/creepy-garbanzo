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
#include <stdio.h>
#include <string.h>
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
extern DECLARE_DO_FUN(do_transfer);
extern DECLARE_DO_FUN(do_peace);
extern DECLARE_DO_FUN(do_drop);
extern DECLARE_DO_FUN(do_sacrifice);
extern DECLARE_DO_FUN(do_eat);

/*
 * The following special functions are available for mobiles.
 */
static DECLARE_SPEC_FUN(spec_breath_any);
static DECLARE_SPEC_FUN(spec_breath_acid);
static DECLARE_SPEC_FUN(spec_breath_fire);
static DECLARE_SPEC_FUN(spec_breath_frost);
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

extern bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace);
extern void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
extern int check_dir(CHAR_DATA * ch, int dir);

/* the function table */
const struct spec_type spec_table[] =
{
	{ "spec_breath_any",	   spec_breath_any	 },
	{ "spec_breath_acid",	   spec_breath_acid	 },
	{ "spec_breath_fire",	   spec_breath_fire	 },
	{ "spec_breath_frost",	   spec_breath_frost	 },
	{ "spec_breath_gas",	   spec_breath_gas	 },
	{ "spec_breath_lightning", spec_breath_lightning },
	{ "spec_cast_adept",	   spec_cast_adept	 },
	{ "spec_cast_cleric",	   spec_cast_cleric	 },
	{ "spec_cast_judge",	   spec_cast_judge	 },
	{ "spec_cast_mage",	   spec_cast_mage	 },
	{ "spec_cast_undead",	   spec_cast_undead	 },
	{ "spec_executioner",	   spec_executioner	 },
	{ "spec_fido",		   spec_fido		 },
	{ "spec_guard",		   spec_guard		 },
	{ "spec_janitor",	   spec_janitor		 },
	{ "spec_mayor",		   spec_mayor		 },
	{ "spec_rmove",		   spec_rmove		 },
	{ "spec_poison",	   spec_poison		 },
	{ "spec_thief",		   spec_thief		 },
	{ "spec_nasty",		   spec_nasty		 },
	{ "spec_troll_member",	   spec_troll_member	 },
	{ "spec_ogre_member",	   spec_ogre_member	 },
	{ "spec_patrolman",	   spec_patrolman	 },
	{ "spec_teleport",	   spec_teleport	 },
	{ "spec_buddha",	   spec_buddha		 },
	{ "spec_teacher",	   spec_teacher		 },
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

