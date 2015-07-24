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
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

#include "merc.h"
#include "tables.h"
#include "magic.h"
#include "lookup.h"
#include "interp.h"
#include "recycle.h"

/***************************************************************************
*	local function declarations
***************************************************************************/
extern bool remove_obj         args((CHAR_DATA * ch, int iWear, bool fReplace));
extern void set_fighting       args((CHAR_DATA * ch, CHAR_DATA * victim));
extern bool one_attack                                   args((CHAR_DATA * ch, CHAR_DATA * victim, int dt, OBJ_DATA * wield));
bool check_shield_block args((CHAR_DATA * ch, CHAR_DATA * victim));



/***************************************************************************
*	general utility functions
***************************************************************************/
/***************************************************************************
*	do_raceinfo
***************************************************************************/
void do_raceinfo(CHAR_DATA *ch, char *argument)
{
	BUFFER *buf;
	int race_idx;
	int skill_idx;
	int race = 0;
	bool first;
	bool all;

	all = (argument[0] == '\0' || !str_prefix(argument, "all"));
	if (!all) {
		race = race_lookup(argument);
		if (race <= 0) {
			send_to_char("That is an invalid race.\n\r", ch);
			return;
		}
	}
	buf = new_buf();
	for (race_idx = 1; pc_race_table[race_idx].name[0] != '\0'; race_idx++) {
		if (all || race == race_idx) {
			add_buf(buf, "\n\r`2-----------------------------------------------------------------------``\n\r");
			printf_buf(buf, "`#Race`3:``             %s\n\r", capitalize(pc_race_table[race_idx].name));

			/* display skills */
			first = FALSE;
			for (skill_idx = 0; skill_idx < 5; skill_idx++) {
				if (pc_race_table[race_idx].skills[skill_idx] == NULL)
					break;

				if (!first) {
					add_buf(buf, "`#Default skills`3:``   ");
					first = TRUE;
				} else if (skill_idx % 3 == 0) {
					add_buf(buf, "\n\r                  ");

/*printf_buf(buf, "                 %s\n\r", capitalize(pc_race_table[race_idx].skills[skill_idx]));*/
				}
				printf_buf(buf, "%-18.18s", capitalize(pc_race_table[race_idx].skills[skill_idx]));
			}

			if (skill_idx % 3 == 0)
				add_buf(buf, "\n\r");

			/* display the stat header */
			add_buf(buf, "\n\r\n\r`@                  STR     INT     WIS     DEX     CON     LUCK\n\r");
			add_buf(buf, "`2-----------------------------------------------------------------------``\n\r");
			printf_buf(buf, "`#Starting stats`3:``   %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``   %3d\n\r",
				   pc_race_table[race_idx].stats[STAT_STR],
				   pc_race_table[race_idx].stats[STAT_INT],
				   pc_race_table[race_idx].stats[STAT_WIS],
				   pc_race_table[race_idx].stats[STAT_DEX],
				   pc_race_table[race_idx].stats[STAT_CON],
				   pc_race_table[race_idx].stats[STAT_LUCK]);

			printf_buf(buf, "`#Trainable stats`3:``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``   %3d\n\r",
				   pc_race_table[race_idx].max_train_stats[STAT_STR],
				   pc_race_table[race_idx].max_train_stats[STAT_INT],
				   pc_race_table[race_idx].max_train_stats[STAT_WIS],
				   pc_race_table[race_idx].max_train_stats[STAT_DEX],
				   pc_race_table[race_idx].max_train_stats[STAT_CON],
				   pc_race_table[race_idx].max_train_stats[STAT_LUCK]);

			printf_buf(buf, "`#Maximum stats`3:``    %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``   %3d\n\r",
				   (pc_race_table[race_idx].max_stats[STAT_STR] > 0) ? pc_race_table[race_idx].max_stats[STAT_STR] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_INT] > 0) ? pc_race_table[race_idx].max_stats[STAT_INT] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_WIS] > 0) ? pc_race_table[race_idx].max_stats[STAT_WIS] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_DEX] > 0) ? pc_race_table[race_idx].max_stats[STAT_DEX] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_CON] > 0) ? pc_race_table[race_idx].max_stats[STAT_CON] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_LUCK] > 0) ? pc_race_table[race_idx].max_stats[STAT_LUCK] : 0);

			add_buf(buf, "`2-----------------------------------------------------------------------``\n\r");
			add_buf(buf, "\n\r");
		}
	}

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
}


/***************************************************************************
*	mutants
***************************************************************************/
#define MIN_REGEN               5

/***************************************************************************
*	do_grow
***************************************************************************/
void do_grow(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (ch->race != race_lookup("mutant")) {
		send_to_char("You should try eating your wheaties..\n\r", ch);
		return;
	}

	if (IS_SET(ch->comm2, COMM2_THIRDARM)) {
		send_to_char("You try hard, but fail to grow another arm.\n\r", ch);
		return;
	}

	SET_BIT(ch->comm2, COMM2_THIRDARM);
	ch->move = 10;
	act("$n screams in agony as a third arm sprouts from $s shoulderblades.", ch, NULL, NULL, TO_ROOM);
	act("You scream in agony as a third arm sprouts from your shoulderblades.", ch, NULL, NULL, TO_CHAR);
}


/***************************************************************************
*	do_third
***************************************************************************/
void do_third(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;

	if (ch->race != race_lookup("mutant")) {
		send_to_char("`!WHAT\n\r`PTHE\n\r`OFUCK?``\n\r", ch);
		return;
	}

	if (!IS_SET(ch->comm2, COMM2_THIRDARM)) {
		send_to_char("Grow a third arm before you try that.\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
		send_to_char("Wear which weapon in your third hand?\n\r", ch);
		return;
	}

	obj = get_obj_carry(ch, argument);
	if (obj == NULL) {
		send_to_char("You have no such thing.\n\r", ch);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		send_to_char("You can only wield a WEAPON .. Bonehead.\n\r", ch);
		return;
	}

	if (get_eq_char(ch, WEAR_SHIELD) != NULL) {
		send_to_char("You cannot use second and third weapons while using a shield.\n\r", ch);
		return;
	}

	if (ch->level < obj->level) {
		printf_to_char(ch,
			       "You must be level %d to use this object.\n\r",
			       obj->level);
		act("$n tries to use $p, but is too inexperienced.",
		    ch, obj, NULL, TO_ROOM);
		return;
	}

	if (get_eq_char(ch, WEAR_WIELD) == NULL) {
		send_to_char("You need to wield a primary weapon, before using a third one!\n\r", ch);
		return;
	}

	if ((get_obj_weight(obj) * 2) > get_obj_weight(get_eq_char(ch, WEAR_WIELD))) {
		send_to_char("Your third weapon has to be considerably lighter than the primary one.\n\r", ch);
		return;
	}

	if (!remove_obj(ch, WEAR_THIRD, TRUE))
		return;

	act("$n wields $p in $s third hand.", ch, obj, NULL, TO_ROOM);
	act("You wield $p in your third hand.", ch, obj, NULL, TO_CHAR);
	equip_char(ch, obj, WEAR_THIRD);
}


/***************************************************************************
*	do_regenerate
***************************************************************************/
void do_regenerate(CHAR_DATA *ch, char *argument)
{
	int regen = 0;
	int regenmana = 0;
	int regenmove = 0;

	int cost;

	if (IS_NPC(ch))
		return;

	if (ch->race != race_lookup("mutant")) {
		send_to_char("You aren't a mutant, dumbass.\n\r", ch);
		return;
	}
/*
 *      if(ch->hit <= (ch->max_hit / MIN_REGEN)
 || ch->mana <= (ch->max_mana / MIN_REGEN)
 || ch->move <= (ch->max_move / MIN_REGEN))
 || {
 ||             send_to_char("You don't have enough stamina to regenerate.\n\r", ch);
 ||             return;
 || }
 */
	if (ch->position != POS_SLEEPING) {
		send_to_char("You must sleep to regenerate.\n\r", ch);
		return;
	}

	if (is_affected(ch, gsp_black_mantle)) {
		send_to_char("You are black mantled.  You cannot regenerate.\n\r", ch);
		return;
	}

	if (ch->ticks_since_last_fight < 1) {
		send_to_char("You are too nervous to regenerate right now.\n\r", ch);
		return;
	}
	cost = (ch->level * 1000);

	if (ch->pcdata->gold_in_bank <= (unsigned int)(cost + 1)) {
		send_to_char("You do not have enough gold in the bank!  Go deposit some.\n\r", ch);
		return;
	}
	if (ch->hit == ch->max_hit && ch->mana == ch->max_mana && ch->move == ch->max_move) {
		send_to_char("You are all healed up already!\n\r", ch);
		return;
	}
	ch->pcdata->gold_in_bank -= cost;
	send_to_char("You feel warm and tingly as your body regenerates.\n\r", ch);

	act("$n looks a little better.\n\r", ch, NULL, NULL, TO_ROOM);
	regen = number_range((ch->max_hit / 26), ch->level);
	regenmana = number_range((ch->max_mana / 29), ch->level);
	regenmove = number_range((ch->max_move / 18), ch->level);


/*    regen = number_range((ch->level*2)/3,ch->level);*/

	ch->hit += regen;
	ch->move += regenmove;
	ch->mana += regenmana;
	WAIT_STATE(ch, 23);

	ch->hit = UMIN(ch->hit, ch->max_hit);
	ch->move = UMIN(ch->move, ch->max_move);
	ch->mana = UMIN(ch->mana, ch->max_mana);
}


/***************************************************************************
*	dragons
***************************************************************************/
/***************************************************************************
*	do_breathe
***************************************************************************/
void do_breathe(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vo;
	char arg[MIL];
	int type;

	if (IS_NPC(ch)) {
		send_to_char("You are not funny.\n\r", ch);
		return;
	}

	if (ch->race != race_lookup("dragon")) {
		send_to_char("Unless you never brush, that's not going to do much good.\n\r", ch);
		return;
	}
	one_argument(argument, arg);

	if (arg[0] == '\0') {
		vo = ch->fighting;
		if (vo == NULL) {
			send_to_char("Breathe on who?\n\r", ch);
			return;
		}
	} else if ((vo = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_SAFE)) {
		send_to_char("You can't do that here!\n\r", ch);
		return;
	}

	if (ch->mana < 75) {
		send_to_char("You haven't the energy!\n\r", ch);
		return;
	}

	if (is_safe(ch, vo))
		return;

	ch->mana -= 50;
	WAIT_STATE(ch, 6);

	if (IS_SET(ch->act, PLR_BATTLE)) {
		type = number_range(1, 4);
		switch (type) {
		case 1:
			spell_fire_breath(skill_lookup("fire breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		case 2:
			spell_frost_breath(skill_lookup("frost breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		case 3:
			spell_gas_breath(skill_lookup("gas breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		default:
			spell_lightning_breath(skill_lookup("lightning breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		}
	} else {
		type = number_range(1, 5);
		switch (type) {
		case 1:
			spell_acid_breath(skill_lookup("acid breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		case 2:
			spell_fire_breath(skill_lookup("fire breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		case 3:
			spell_frost_breath(skill_lookup("frost breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		case 4:
			spell_gas_breath(skill_lookup("gas breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		default:
			spell_lightning_breath(skill_lookup("lightning breath"), ch->level, ch, vo, TAR_CHAR_OFFENSIVE, "");
			break;
		}
	}
}

/***************************************************************************
*	do_shriek
***************************************************************************/
void do_shriek(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	AFFECT_DATA *paf;
	CHAR_DATA *victim;
	SKILL *skill;
	char arg[MIL];
	long counter = 0;


	if (ch->race != race_lookup("dragon")) {
		send_to_char("Unless your name is Courtney Love don't even bother!\n\r", ch);
		return;
	}


	if ((skill = skill_lookup("shriek")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}


	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Yeah, we know you have a big mouth but who you gonna use it on?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("Even if you could why would you?\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (victim->fighting == NULL)
		set_fighting(victim, ch);


	WAIT_STATE(ch, 6);
	check_improve(ch, skill, TRUE, 3);

	if (is_affected(victim, skill)) {
		for (paf = victim->affected; paf != NULL; paf = paf->next) {
			if (paf->type == skill->sn && paf->location != APPLY_SAVING_SPELL) {
				counter = paf->modifier;
				counter -= 25;
			}
		}
	} else {
		counter = -25;
	}

	if (counter > -525) {
		affect_strip(victim, skill);

		af.where = TO_AFFECTS;
		af.type = skill->sn;
		af.skill = skill;
		af.level = ch->level;
		af.duration = (ch->level / 15);
		af.modifier = counter;
		af.bitvector = 0;

		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);

		af.modifier = (UABS(counter)) / 5;

		af.location = APPLY_SAVING_SPELL;
		affect_to_char(victim, &af);

		send_to_char("You blow out their eardrums!\n\r", ch);
		act("$n opens their mouth and your eardrums begin to hemmorage!\n\r", ch, NULL, victim, TO_VICT);
		act("$n opens their mouth and dazes $N.", ch, NULL, victim, TO_ROOM);
	} else {
		send_to_char("You can not affect them any further with your voice.\n\r", ch);
	}


	return;
}


/***************************************************************************
*	giants
***************************************************************************/
/***************************************************************************
*	do_crush
***************************************************************************/
void do_crush(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	char buf[100];
	char arg[MIL];
	int chance;

	if (ch->race != race_lookup("giant")) {
		send_to_char("You're not nearly large enough to crush someone!\n\r", ch);
		return;
	}


	if ((skill = skill_lookup("crush")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Who did you want to stomp on?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("You would much rather do that to someone else.\n\r", ch);
		return;
	}

	if (ch->move <= 0 || ch->mana <= 0) {
		send_to_char("You are too tired to grab them.\n\r", ch);
		return;
	}



	if (is_safe(ch, victim))
		return;

	if (gsp_evade != NULL) {
		chance = get_learned_percent(victim, gsp_evade) / 2;
		chance += chance / 2;

		if (number_percent() <= chance) {
			act("$n brings their fist down towards you, but you evade it!", ch, NULL, victim, TO_VICT);
			act("You bring your fist down towards $N, but they evade your attempt!", victim, NULL, ch, TO_CHAR);
			ch->move -= 25;

			WAIT_STATE(ch, number_range(2, 4) * PULSE_VIOLENCE);
			return;
		}
	}

	if (!IS_NPC(victim) && (ch->position != POS_FIGHTING)) {
		sprintf(buf,
			"ATTACK: %s(%d) stomped on %s(%d) in room [%ld]!",
			ch->name,
			ch->level,
			victim->name,
			victim->level,
			victim->in_room->vnum);

		log_string(buf);
		wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
	}

	if (!IS_NPC(ch)
	    && (victim->race != race_lookup("giant"))) {
		damage(ch, victim, (ch->level * 3 + 45), skill->sn, DAM_BASH, TRUE);
		ch->mana = ch->mana - 25;
		ch->move = ch->move - 25;

		WAIT_STATE(ch, number_range(2, 4) * PULSE_VIOLENCE);
		WAIT_STATE(victim, number_range(1, 3) * PULSE_VIOLENCE);

		act("You bring your fist down upon $N, crushing their body!", ch, NULL, victim, TO_CHAR);
		act("$n brings their fist down upon you, crushing you!", ch, NULL, NULL, TO_VICT);
		act("$n brings their fist down upon $N and crushes them!", ch, NULL, victim, TO_ROOM);

		multi_hit(ch, victim, TYPE_UNDEFINED);
	} else {
		send_to_char("You missed!\n\r", ch);
	}
}


/***************************************************************************
*	do_crush
***************************************************************************/
void do_throw(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	int percent;


	if (str_cmp(race_table[ch->race].name, "giant")) {
		send_to_char("You aren't strong enough to throw.\n\r", ch);
		return;
	}


	if ((skill = skill_lookup("throw")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (IS_NPC(ch)) {
		percent = UMIN(number_fuzzy(75), (ch->level * 2));
	} else {
		percent = get_learned_percent(ch, skill);
		if (percent <= 0) {
			send_to_char("You are much too puny to throw anyone, runt!\n\r", ch);
			return;
		}
	}

	/* fighting to throw */
	if (ch->fighting == NULL) {
		send_to_char("You must be fighting to throw.\n\r", ch);
		return;
	}

	victim = (ch->fighting);

	if (percent > number_percent()) {
		ROOM_INDEX_DATA *to_room;
		ROOM_INDEX_DATA *tmp_room;
		EXIT_DATA *exit = NULL;
		EXIT_DATA *tmp_exit;
		int ex;
		int count;
		int dam;


		/* do not divide 0 by 5 -- evil */
		if (victim->hit > 0)
			dam = UMAX(number_range(ch->level * 3, ch->level * 8), (victim->hit / 6));
		else
			dam = number_range(ch->level * 3, ch->level * 8);

		/* get a random exit */
		dam += ch->level;
		count = 0;
		to_room = NULL;
		for (ex = 0; ex < 6; ex++) {
			if (((tmp_exit = ch->in_room->exit[ex]) != NULL)
			    && ((tmp_room = tmp_exit->u1.to_room) != NULL)) {
				if (!is_room_owner(victim, tmp_room) && room_is_private(tmp_room))
					continue;

				if (number_range(0, count) == 0) {
					exit = tmp_exit;
					to_room = tmp_room;
					count++;
				}
			}
		}

		if (NULL != to_room && NULL != exit) {
			/* the exit is a door -- character does not have passdoor */
			if ((IS_SET(exit->exit_info, EX_CLOSED))
			    && (!IS_AFFECTED(victim, AFF_PASS_DOOR) || IS_SET(exit->exit_info, EX_NOPASS))) {
				/* 75% chance of smashing the door down */
				if (number_percent() <= 25) {
					REMOVE_BIT(exit->exit_info, EX_CLOSED);
					REMOVE_BIT(exit->exit_info, EX_LOCKED);

					act("\n\rARRRGGGHHH... $n goes flying out of the room. SMASH!! ...and smashes down a door!", victim, NULL, NULL, TO_NOTVICT);

					char_from_room(victim);
					char_to_room(victim, to_room);

					act("$n comes flying into the room! THUD!!!", victim, NULL, NULL, TO_NOTVICT);
					act("$n THROWS your punk ass out of the room. SMASH!!...and smashes down a door!", ch, NULL, victim, TO_VICT);
					damage(ch, victim, dam * 2, skill->sn, DAM_BASH, TRUE);

					stop_fighting(ch, TRUE);
					stop_fighting(victim, TRUE);
					update_pos(victim);

					if (victim->position > POS_RESTING)
						victim->position = POS_RESTING;

					do_look(victim, "");
				} else {
					act("$n sends you flying across the room...and SPLAT!!...into a door!\n\r", ch, NULL, victim, TO_VICT);
					act("\n\rARRRGGGHHH... $n goes flying across the room!!...SPLAT!!...right into a door!\n\r", victim, NULL, NULL, TO_NOTVICT);
					damage(ch, victim, dam, skill->sn, DAM_BASH, TRUE);
				}
			} else {
				/* character goes flying */
				act("\n\rARRRGGGHHH... $n goes flying out of the room!!", victim, NULL, NULL, TO_NOTVICT);

				char_from_room(victim);
				char_to_room(victim, to_room);

				act("$n comes flying into the room! THUD!!!\n\r", victim, NULL, NULL, TO_NOTVICT);
				act("$n throws  you out of the room!!\n\r", ch, NULL, victim, TO_VICT);
				damage(ch, victim, dam, skill->sn, DAM_BASH, TRUE);

				stop_fighting(ch, TRUE);
				stop_fighting(victim, TRUE);

				update_pos(victim);

				if (victim->position > POS_RESTING)
					victim->position = POS_RESTING;

				do_look(victim, "");
			}
		}

		WAIT_STATE(ch, skill->wait);
		WAIT_STATE(victim, (skill->wait * 2 / 3));

		check_improve(ch, skill, TRUE, 1);
	} else {
		/* skill failed */
		act("You cannot get a decent grip on $m.\n\r", ch, NULL, victim, TO_CHAR);
		act("$n tries to pick you up but slips!.\n\r", ch, NULL, victim, TO_VICT);

		WAIT_STATE(ch, skill->wait);
		check_improve(ch, skill, FALSE, 1);
	}

	check_killer(ch, victim);
	return;
}


/***************************************************************************
*	felines
***************************************************************************/
/***************************************************************************
*	do_rake
***************************************************************************/
void do_rake(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	CHAR_DATA *victim;
	SKILL *skill;
	char arg[MIL];
	int mod;

	mod = ch->damroll / 5;

	if (ch->move < 5) {
		send_to_char("You are too exhausted.\n\r", ch);
		return;
	}

	if (ch->race != race_lookup("feline")) {
		send_to_char("Hey .. you don't have claws!\n\r", ch);
		return;
	}

	if ((skill = skill_lookup("rake")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			send_to_char("Whose blood would you like to spill?\n\r", ch);
			return;
		}
	} else if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, skill->wait);
	if (ch->level > number_percent()) {
		int iter, count;

		count = number_range(1, 4);

		for (iter = 0; iter < count; iter++) {
			if (number_percent() < ((ch->level + 5 - victim->level) * 7)) {
				act("$n's claws rake across your face!", ch, NULL, victim, TO_VICT);
				act("Your claws rake across $n's face!", victim, NULL, ch, TO_VICT);
				damage(ch, victim, number_range(ch->level * 4, ch->level * 10) + mod, skill->sn, DAM_SLASH, TRUE);
				ch->move -= 25;

				if (!is_affected(ch, skill)
				    && !saves_spell(ch->level + 5, victim, DAM_SLASH)) {
					af.where = TO_AFFECTS;
					af.type = skill->sn;
					af.skill = skill;
					af.level = ch->level;
					af.duration = number_range(0, 4);
					af.location = 0;
					af.modifier = 0;
					af.bitvector = AFF_BLIND;
					affect_to_char(victim, &af);
				}

				check_improve(ch, skill, TRUE, 1);
			} else {
				act("$n's claws rake across your chest!", ch, NULL, victim, TO_VICT);
				act("Your claws rake across $n's chest!", victim, NULL, ch, TO_VICT);
				damage(ch, victim, number_range(ch->level * 6, ch->level * 12) + mod, skill->sn, DAM_SLASH, TRUE);
				ch->move -= 5;
				check_improve(ch, skill, TRUE, 1);
			}
		}
	} else {
		damage(ch, victim, 0, skill->sn, DAM_BASH, TRUE);
		check_improve(ch, skill, FALSE, 1);
	}
	return;
}


/***************************************************************************
*	do_hiss
***************************************************************************/
void do_hiss(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	char arg[MIL];

	if (!IS_NPC(ch) && (ch->race != race_lookup("feline"))) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if ((skill = skill_lookup("hiss")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Hiss at whom?\n\r", ch);
		return;
	}


	if ((victim = ch->fighting) == NULL) {
		if ((victim = get_char_room(ch, arg)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		if (victim == ch) {
			send_to_char("That's silly.\n\r", ch);
			return;
		}
	}

	if (ch->fighting != NULL) {
		send_to_char("You're too busy fighting, bitch.\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("You're going to have a tough time doing that.\n\r", ch);
		return;
	}


	if (is_safe(ch, victim))
		return;


	if (victim == ch) {
		send_to_char("You're going to have a tough time doing that.\n\r", ch);
		return;
	}

	send_to_char("You hiss at your enemy!\n\r", ch);
	send_to_char("Spittle flies in your face!\n\r", victim);
	check_killer(ch, victim);
	damage(ch, victim, number_range(ch->level * 4, ch->level * 6), skill->sn, DAM_PIERCE, TRUE);

	check_improve(ch, skill, TRUE, 1);

	WAIT_STATE(ch, PULSE_VIOLENCE);
	WAIT_STATE(victim, PULSE_VIOLENCE * 2);



	if (gsp_fear != NULL
	    && !is_affected(ch, gsp_fear))
		cast_spell(ch, gsp_fear, (ch->level + 15), victim, TARGET_CHAR, "");


	return;
}




/***************************************************************************
*	do_esp
***************************************************************************/
void do_esp(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[MSL];
	CHAR_DATA *victim;
	int race;

	one_argument(argument, arg);

	if (ch->race != (race = race_lookup("human"))) {
		send_to_char("`!WHAT``\n\r`PTHE``\n\r`OFUCK?``\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			send_to_char("Use ESP on who?\n\r", ch);
			return;
		}

		ch->linked = victim;
	} else if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch && ch->linked != NULL) {
		if (!IS_NPC(victim)) {
			sprintf(buf, "You are no longer psychically linked to %s.\n\r", ch->linked->name);
			send_to_char(buf, ch);
		}

		ch->linked = NULL;
		return;
	} else if (victim == ch && ch->target == NULL) {
		send_to_char("You read your own thoughts.. how impressive.\n\r", ch);
		return;
	} else if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
		return;
	} else {
		if (!IS_NPC(victim)) {
			sprintf(buf, "You are now linked to %s's mind.\n\r", victim->name);
			send_to_char(buf, ch);
		} else {
			send_to_char("You can only link your mind to other players.\n\r", ch);
			return;
		}

		ch->linked = victim;
		SET_BIT(victim->act, PLR_ESP);
		return;
	}
}

/***************************************************************************
*       do_hyper
***************************************************************************/
void do_hyper(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	SKILL *skill;
	int level;


	if (ch->race != race_lookup("sprite")
	    || (skill = skill_lookup("hyper metabolism")) == NULL) {
		send_to_char("`!WHAT``\n\r`PTHE``\n\r`OFUCK?``\n\r", ch);
		return;
	}
	level = ch->level;

	if (is_affected(ch, skill)) {
		send_to_char("You are already hyperactive!\n\r", ch);
		return;
	}

	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = level;
	af.duration = (level / 10) + number_range(3, 5);
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	af.location = APPLY_SAVES;
	af.modifier = 100;
	affect_to_char(ch, &af);

	send_to_char("You feel your pulse speed up, and strangely you feel hungry...\n\r", ch);
	return;
}



/***************************************************************************
*	elves
***************************************************************************/


/***************************************************************************
*	dwarves
***************************************************************************/
/***************************************************************************
*	do_kneecap
***************************************************************************/
void do_kneecap(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	char arg[MIL];
	int race;
	int chance;

	if (ch->move < 50) {
		send_to_char("You are too exhausted.\n\r", ch);
		return;
	}

	if (ch->race != (race = race_lookup("dwarf"))) {
		send_to_char("Pick a target more along your eye level.\n\r", ch);
		return;
	}

	if ((skill = skill_lookup("kneecap")) == NULL) {
		send_to_char("Huh?", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		if ((victim = ch->fighting) == NULL) {
			send_to_char("Break whos kneecaps?\n\r", ch);
			return;
		}
	} else if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("You want to break your own kneecap?\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (gsp_evade != NULL) {
		chance = get_learned_percent(victim, gsp_evade) / 2;
		chance += chance / 2;

		if (number_percent() <= chance) {
			act("$n charges you and dives for your knee, but you evade them!", ch, NULL, victim, TO_VICT);
			act("You try to slam into $n's knee, but they evade you!", victim, NULL, ch, TO_CHAR);
			ch->move -= 25;
			WAIT_STATE(ch, skill->wait);
			return;
		}
	}

	WAIT_STATE(ch, skill->wait);
	if (ch->level > number_percent()) {
		act("$n slams into your knee!", ch, NULL, victim, TO_VICT);
		act("You slam into $n's knee!", victim, NULL, ch, TO_VICT);
		damage(ch, victim, number_range(ch->level * 6, ch->level * 10), skill->sn, DAM_BASH, TRUE);

		ch->move -= 50;
		if (victim->move > 0)
			victim->move /= 2;
		victim->move -= (ch->level / 3);

		WAIT_STATE(victim, skill->wait / 2);
		check_improve(ch, skill, TRUE, 1);
	} else {
		damage(ch, victim, 0, skill->sn, DAM_BASH, TRUE);
		check_improve(ch, skill, FALSE, 1);
		ch->move -= 25;
	}

	return;
}

/***************************************************************************
*	aiel
***************************************************************************/
/***************************************************************************
*	do_veil
***************************************************************************/
void do_veil(CHAR_DATA *ch, char *argument)
{
	AFFECT_DATA af;
	SKILL *skill;
	int race;

	if (!IS_NPC(ch)
	    && (ch->race != (race = race_lookup("Aiel")))) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if ((skill = gsp_veil) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (ch->fighting) {
		send_to_char("You can't veil yourself now that the dance has begun.\n\r", ch);
		return;
	}

	if (is_affected(ch, skill)) {
		send_to_char("You're already veiled.\n\r", ch);
		return;
	}

	if (ch->mana >= 50) {
		ch->mana -= 50;
	} else {
		send_to_char("You don your veil.\n\r", ch);
		return;
	}

	if (number_percent() < 80) {
		af.type = skill->sn;
		af.skill = skill;
		af.level = ch->level;
		af.duration = 2;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		send_to_char("You don your veil.\n\r"
			     "You feel yourself begin to move with uncanny speed.\n\r"
			     "You feel yourself begin to move with deadly grace.\n\r", ch);
		act("$n dons a black veil.", ch, NULL, NULL, TO_ROOM);
		check_improve(ch, skill, TRUE, 2);

		return;
	} else {
		check_improve(ch, gsp_veil, FALSE, 2);
		send_to_char("You don your veil.\n\r", ch);
	}
}


/***************************************************************************
*	do_dream
***************************************************************************/
void do_dream(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	one_argument(argument, arg);
	if (ch->race != race_lookup("aiel")) {
		send_to_char("You are not a dreamer.\n\r", ch);
		return;
	}

	if (ch->position != POS_SLEEPING) {
		send_to_char("You must sleep to dream.\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		send_to_char("Dream of whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL
	    || victim == ch
	    || victim->in_room == NULL
	    || !can_see_room(ch, victim->in_room)
	    || IS_SET(ch->in_room->room_flags, ROOM_NODREAM)
	    || IS_SET(victim->in_room->room_flags, ROOM_NODREAM)) {
		send_to_char("Your dreams are a jumble, and you cannot visualize your target.\n\r", ch);
		return;
	}

	sprintf(buf, "Your thoughts coalesce into an image of %s.", victim->name);
	send_to_char(buf, ch);

	ch->dream = victim;
}



/***************************************************************************
*	vampires
***************************************************************************/
/***************************************************************************
*	do_feed
***************************************************************************/
void do_feed(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill_poison;
	SKILL *skill_sleep;
	char arg[MIL];
	char buf[MSL];
	int race;
	int foo;

	one_argument(argument, arg);

	if (ch->race != (race = race_lookup("vampire"))) {
		send_to_char("You find the thought of feeding on others repulsive!\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		send_to_char("Feed on whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("How can you feed off yourself?\n\r", ch);
		return;
	}

	if (is_safe(ch, victim)) {
		send_to_char("I don't think so.  Not here.  Not now.\n\r", ch);
		return;
	}

	if (!IS_IMMORTAL(ch)) {
		if (victim->race == ch->race) {
			send_to_char("You couldn't find what you need in them.\n\r", ch);
			return;
		}

		if (!IS_NPC(ch)) {
			if (ch->pcdata->condition[COND_FEED] == 100) {
				send_to_char("You do not need to feed.\n\r", ch);
				return;
			}
		}

		if (IS_NPC(ch)) {
			send_to_char("You do not need to feed.\n\r", ch);
			return;
		}

		if (!IS_NPC(victim) && (victim->hit <= (victim->max_hit / 5))) {
			send_to_char("Your victim looks quite pale.\n\rPerhaps you should go find another?\n\r", ch);
			return;
		}

		if (!IS_NPC(victim) && victim->drained < 0) {
			send_to_char("Your victim looks quite pale.\n\rPerhaps you should go find another?\n\r", ch);
			return;
		}
	}

	if (!IS_NPC(victim)) {
		sprintf(buf, "You are filled with warmth as %s's blood rushes through your veins.\n\r", victim->name);
		send_to_char(buf, ch);
	} else {
		sprintf(buf, "You are filled with warmth as %s's blood rushes through your veins.\n\r", victim->short_descr);
		send_to_char(buf, ch);
	}

	if (!IS_NPC(ch)) {
		if (ch->pcdata->condition[COND_FEED] <= 0 && ch->pcdata->condition[COND_FEED] > -151) {
			ch->pcdata->condition[COND_FEED] = 0;
			gain_condition(ch, COND_FULL, (victim->level / 2));
			gain_condition(ch, COND_FEED, (victim->level / 2));
			send_to_char("Your `1bloodlust`` is slaked. For now.\n\r", ch);
		}


		printf_to_char(victim, "Out of nowhere, %s `!bites`` you on the neck!\n\rThat really did `!HURT``!!\n\r", ch->name);

		act("$n `!drains`` $N's `1blood``!", ch, NULL, victim, TO_NOTVICT);
		if (!IS_NPC(victim) && !IS_IMMORTAL(victim)) {
			if (victim->hit > victim->max_hit / 5) {
				victim->hit -= ch->level * 4;
				ch->hit += ch->level * 2;
			}

			if (victim->mana > victim->max_mana / 5) {
				victim->mana -= ch->level * 4;
				ch->mana += ch->level * 2;
			}

			if (victim->move > victim->max_move / 5) {
				victim->move -= ch->level * 4;
				ch->move += ch->level * 2;
			}

			if (victim->hit <= victim->max_hit / 5) {
				send_to_char("Your victim has had enough.\n\r", ch);
				return;
			}

			if (victim->hit < 0 && victim->hit > -11)
				victim->position = POS_STUNNED;

			if (victim->hit < -10) {
				send_to_char("You have `!KILLED`` your victim!\n\r", ch);
				check_killer(ch, victim);
				raw_kill(victim, NULL);
			}

			victim->drained -= (ch->level / 24);
		} else {
			victim->drained -= (ch->level / 24);
		}
	}

	skill_sleep = skill_lookup("sleep");

	if (victim->position < POS_RESTING) {
		if ((skill_sleep != NULL)
		    && is_affected(victim, skill_sleep)) {
			check_dispel(victim->level, victim, skill_sleep);
			if (!is_affected(victim, skill_sleep)) {
				victim->position = POS_SLEEPING;
				update_pos(victim);
			}
		}
	}

	if ((victim->level >= ch->level - 5) && (skill_sleep != NULL)
	    && !is_affected(victim, skill_sleep)) {
		foo = (number_range(0, 5));

		if (foo == 3 || foo == 2)
			multi_hit(victim, ch, TYPE_UNDEFINED);
	}

	if ((skill_poison = gsp_poison) != NULL
	    && is_affected(victim, skill_poison)) {
		/*  poison the feeder */
		AFFECT_DATA af;

		act("$n chokes as rotten blood rushes through $s veins.", ch, 0, 0, TO_ROOM);
		send_to_char("You choke as rotten blood rushes through your veils.\n\r", ch);

		af.where = TO_AFFECTS;
		af.type = skill_poison->sn;
		af.skill = skill_poison;
		af.level = number_fuzzy(victim->level);
		af.duration = 2 * victim->level;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_POISON;

		affect_join(ch, &af);
		ch->hit -= victim->level;
	}

	if (victim->level > ch->level)
		multi_hit(victim, ch, TYPE_UNDEFINED);

	return;
}


/***************************************************************************
*	sprites
***************************************************************************/
/***************************************************************************
*	do_invisible
***************************************************************************/
void do_invisible(CHAR_DATA *ch, char *argument)
{
	int race;

	if (ch->race != (race = race_lookup("sprite"))) {
		send_to_char("You close your eyes and hope no one can see you\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
		act("You are already invisible.", ch, NULL, NULL, TO_CHAR);
		return;
	}

	SET_BIT(ch->affected_by, AFF_INVISIBLE);
	act("You fade out of existence.", ch, NULL, NULL, TO_CHAR);
	return;
}


/***************************************************************************
*	do_dust
***************************************************************************/
void do_dust(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	SKILL *lookup;
	char *spells[13] = { "cure critical",
			     "cure critical",
			     "cure critical",
			     "refresh",
			     "refresh",
			     "druid call",
			     "sanctuary",
			     "haste",
			     "frenzy",
			     "bless",
			     "air armor",
			     "bark skin",
			     "stone skin" };
	int sn[4];
	int idx;
	int rand;
	int check;
	int race;

	if (ch->race != (race = race_lookup("sprite"))) {
		send_to_char("Only sprites can make faerie dust!\n\r", ch);
		return;
	}

	idx = 0;
	for (check = 0; check < 4; check++)
		sn[check] = 0;

	while (idx < 4) {
		rand = number_range(0, 12);
		lookup = skill_lookup(spells[rand]);

		for (check = 0; check < 4; check++) {
			if (sn[check] == 0) {
				sn[check] = lookup->sn;
				idx++;
				break;
			}
			if (sn[check] == lookup->sn)
				break;
		}
	}

	obj = create_object(get_obj_index(OBJ_VNUM_BLANK_PILL), 0);
	obj->value[0] = number_range(ch->level, ch->level * 3);
	obj->value[1] = sn[0];
	obj->value[2] = sn[1];
	obj->value[3] = sn[2];
	obj->value[4] = sn[3];

	send_to_char("You create a little pile of dust!\n\r", ch);
	obj->timer = 5;
	obj_to_char(obj, ch);
	return;
}





/***************************************************************************
*	do_bite
***************************************************************************/
void do_bite(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	SKILL *spell;
	void *vo;
	char arg[MIL];
	int level;
	int level_mod;
	int count;
	int target;

	if (IS_NPC(ch))
		return;

	if (ch->race != race_lookup("werebeast") && ch->race != race_lookup("vampire")) {
		send_to_char("Only werebeasts and vampires can bite!\n\r", ch);
		return;
	}

	if ((skill = skill_lookup("bite")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Bite whom?\n\r", ch);
		return;
	}

	if ((victim = ch->fighting) == NULL) {
		if ((victim = get_char_room(ch, arg)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		if (victim == ch) {
			send_to_char("That's silly.\n\r", ch);
			return;
		}
	} else {
		send_to_char("You're a bit busy already...\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	count = 0;
	if (ch->level > victim->level)
		count += number_range(1, 3) + 1;
	else
		count += number_range(1, 2);

	if (IS_AFFECTED(ch, AFF_HASTE))
		count += 1;

	/* set up spell arguments */

	spell = skill_lookup("black plague");
	vo = (void *)victim;
	target = TARGET_CHAR;
	level = (IS_NPC(ch) || class_table[ch->class].fMana) ? ch->level : (ch->level * 7) / 8;
	level_mod = get_curr_stat(ch, STAT_INT) / 5;

	if (level_mod > 0)
		level += (level_mod - 1);


	/**************************/

	WAIT_STATE(ch, skill->wait);

	if (get_learned_percent(ch, skill) > number_percent()) {
		int counter;

		send_to_char("You leap out and sink your teeth into your victim!\n\r", ch);
		send_to_char("You feel quite drained all of the sudden.\n\r", victim);
		for (counter = 0; counter < count; counter++) {
			one_attack(ch, victim, skill->sn, NULL);
			if (victim->mana > 0) {
				victim->mana -= number_range(MAX_LEVEL / 12, MAX_LEVEL / 6);
				victim->mana = UMAX(0, victim->mana);
			}
			if ((spell != NULL) && (number_percent() > 3))
				cast_spell(ch, spell, level, vo, target, "");
		}

		WAIT_STATE(victim, skill->wait / 2);
		check_improve(ch, skill, TRUE, 2);
	} else {
		damage(ch, victim, 0, skill->sn, DAM_NONE, TRUE);
		send_to_char("You miss your victim's neck!\n\r", ch);
		check_improve(ch, skill, FALSE, 1);
	}

	check_killer(ch, victim);
	return;
}
