#include <stdio.h>
#include "merc.h"
#include "character.h"
#include "mob_cmds.h"
#include "channels.h"

extern void make_corpse(CHAR_DATA *ch);
extern bool check_shield_block(CHAR_DATA *ch, CHAR_DATA *victim);
extern void disarm(CHAR_DATA *ch, CHAR_DATA *victim);
extern void spell_charm_person(SKILL *skill, int level, CHAR_DATA *ch, void *vo, int target, char *argument);
extern void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim);
extern int battlefield_count(void);

/*
 * Generate a random door.
 */
int number_door(void)
{
    return number_range(0, 5);
}

void do_bash(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	char arg[MIL];
	int chance;
	int evade;
	int total;
	int percent;

	if ((skill = skill_lookup("bash")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if ((chance = get_learned_percent(ch, skill)) == 0
	    || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BASH))) {
		send_to_char("Bashing? What's that?\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			send_to_char("But you aren't fighting anyone!\n\r", ch);
			return;
		}
	} else if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (is_affected(victim, skill)) {
		act("$N is already off balance!.", ch, NULL, victim, TO_CHAR);
		return;
	}

	evade = get_learned_percent(victim, gsp_evade);

	if (evade > 0) {
		evade = URANGE(20, evade, 50);
		if (number_percent() <= evade) {
			act("$n charges you and tries to knock you down, but you evade them!", ch, NULL, victim, TO_VICT);
			act("You try to bash $N, but they evade you!", ch, NULL, victim, TO_CHAR);
			WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
			return;
		}
	}

	if (victim->position < POS_FIGHTING) {
		act("You'll have to let $M get back up first.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim == ch) {
		send_to_char("You try to bash your brains out, but fail.\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;


	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
		return;
	}

/* modifiers */

/* size  and weight */
	chance += ch->carry_weight / 250;
	chance -= victim->carry_weight / 200;

	if (ch->size < victim->size)
		chance += (ch->size - victim->size) * 15;
	else
		chance += (ch->size - victim->size) * 10;

/* stats */
	chance += get_curr_stat(ch, STAT_STR);
	chance -= (get_curr_stat(victim, STAT_DEX) * 1) / 1;

	{
		long ac = GET_AC(victim, AC_BASH);
		if (ac >= 101)
			chance -= ac / 15;
	}
/* speed */
	if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
		chance -= 30;

/* level */
	chance += (ch->level - victim->level);

	if (!IS_NPC(victim)
	    && chance < (percent = get_learned_percent(victim, gsp_dodge)))
		chance -= 3 * (percent - chance);

/* now the attack */
	if (number_percent() < chance) {
		int modifier;

		if (check_shield_block(ch, victim)) {
			act("$n slams into you, but your shield absorbs the brunt of the impact!", ch, NULL, victim, TO_VICT);
			act("You slam into $N, but $S shield absorbs the impact!", ch, NULL, victim, TO_CHAR);
			act("$n slams into $N's shield.", ch, NULL, victim, TO_NOTVICT);
			check_improve(ch, skill, true, 1);
			WAIT_STATE(victim, PULSE_VIOLENCE);
			WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
			victim->position = POS_RESTING;
			modifier = 2;
		} else {
			act("$n sends you sprawling with a powerful bash!", ch, NULL, victim, TO_VICT);
			act("You slam into $N, and send $M flying!", ch, NULL, victim, TO_CHAR);
			act("$n sends $N sprawling with a powerful bash.", ch, NULL, victim, TO_NOTVICT);
			check_improve(ch, skill, true, 1);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
			victim->position = POS_RESTING;
			modifier = 1;
		}

		if (number_percent() < 10) {
			act("$n shatters one of your bones!", ch, NULL, victim, TO_VICT);
			act("You shatter one of $N's bones with your bash!", ch, NULL, victim, TO_CHAR);
			act("$n breaks one of $N's bones with the bash!", ch, NULL, victim, TO_NOTVICT);
			total = number_range(ch->level, 25 + 4 * ch->size + chance / 10) * 6;
		} else {
			total = number_range(ch->level, 25 + 4 * ch->size + chance / 10) * 3;
		}

		total = (total > 0) ? total / modifier : total;

		(void)damage(ch, victim, total, skill->sn, DAM_BASH, false);
	} else {
		(void)damage(ch, victim, 0, skill->sn, DAM_BASH, false);

		act("You fall flat on your face!", ch, NULL, victim, TO_CHAR);
		act("$n falls flat on $s face.", ch, NULL, victim, TO_NOTVICT);
		act("You evade $n's bash, causing $m to fall flat on $s face.", ch, NULL, victim, TO_VICT);

		check_improve(ch, skill, false, 1);
		ch->position = POS_RESTING;
		WAIT_STATE(ch, skill->wait);
	}
}

void do_kill(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	CHAR_DATA *victim;

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Kill whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}


	if ((!IS_NPC(victim)) && (IS_SET(victim->act, PLR_LINKDEAD))) {
		send_to_char("That player is currently `7[`8LINKDEAD`7] ..\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("You hit yourself.  Ouch!\n\r", ch);
		multi_hit(ch, ch, TYPE_UNDEFINED);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		send_to_char("You do the best you can!\n\r", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		log_string("%s is attempting to kill %s", ch->name, victim->name);
	}

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	check_killer(ch, victim);
	multi_hit(ch, victim, TYPE_UNDEFINED);
}

void do_murde(CHAR_DATA *ch, /*@unused@*/char *argument)
{
	send_to_char("If you want to MURDER, spell it out.\n\r", ch);
}

void do_murder(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char buf[MSL];
	char arg[MIL];

	(void)one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Murder whom?\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act, ACT_PET)))
		return;

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if ((!IS_NPC(victim)) && (IS_SET(victim->act, PLR_LINKDEAD)))
		send_to_char("That player is currently `7[`8LINKDEAD`7] ..\n\r", ch);


	if (victim == ch) {
		send_to_char("Suicide is a mortal sin.\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;


	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		send_to_char("You do the best you can!\n\r", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		log_string("%s is attempting to murder %s", ch->name, victim->name);
	}

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
	if (IS_NPC(ch))
		(void)snprintf(buf, MSL, "Help! I am being attacked by %s!", ch->short_descr);
	else
		(void)snprintf(buf, MSL, "Help! I am being attacked by %s!", ch->name);

	broadcast_channel(victim, channels_find(CHANNEL_SHOUT), NULL, buf);
	check_killer(ch, victim);
	multi_hit(ch, victim, TYPE_UNDEFINED);
}

void do_flee(/*@dependent@*/CHAR_DATA *ch, /*@unused@*/char *argument)
{
	ROOM_INDEX_DATA *was_in;
	ROOM_INDEX_DATA *now_in;
	CHAR_DATA *victim;
	int attempt;

	if ((victim = ch->fighting) == NULL) {
		if (ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;
		send_to_char("You aren't fighting anyone.\n\r", ch);
		return;
	}

	was_in = ch->in_room;
	for (attempt = 0; attempt < 6; attempt++) {
		EXIT_DATA *pexit;
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

		if (!IS_NPC(ch)) {
			send_to_char("You flee from combat!\n\r", ch);

			if ((ch->class == class_lookup("thief"))
			    && (number_percent() < 3 * (ch->level / 2))) {
				send_to_char("You snuck away safely.\n\r", ch);
			} else {
				send_to_char("You lose ```!10 ``exp.\n\r", ch);
				gain_exp(ch, -10);
			}
		}

		stop_fighting(ch, true);

		if (!IS_NPC(ch))
			return;

		if (IS_NPC(victim) && (victim->max_hit <= (victim->max_hit / 2))) {
			send_to_char("Perhaps you should avoid that opponent..\n\r", ch);
			victim->mobmem = ch;
			return;
		}

		if ((IS_NPC(ch)) && (!IS_NPC(victim))) {
			ch->mob_wuss = victim;
			return;
		}
	}

	send_to_char("```#PANIC``! You couldn't escape!\n\r", ch);
}

void do_rescue(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	CHAR_DATA *fch;
	SKILL *skill;
	char arg[MIL];
	int heal;

	if ((skill = skill_lookup("rescue")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Rescue whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}


	if (victim == ch) {
		send_to_char("What about fleeing instead?\n\r", ch);
		return;
	}

	if (!IS_NPC(ch) && IS_NPC(victim)) {
		send_to_char("Doesn't need your help!\n\r", ch);
		return;
	}

	if (ch->fighting == victim) {
		send_to_char("Too late.\n\r", ch);
		return;
	}

	if ((fch = victim->fighting) == NULL) {
		send_to_char("That person is not fighting right now.\n\r", ch);
		return;
	}


	WAIT_STATE(ch, skill->wait);
	if (number_percent() > get_learned_percent(ch, skill)) {
		send_to_char("You fail the rescue.\n\r", ch);

		check_improve(ch, skill, false, 1);
		return;
	}

	act("```&You rescue $N!``", ch, NULL, victim, TO_CHAR);
	act("```&$n rescues you!``", ch, NULL, victim, TO_VICT);
	act("```&$n rescues $N!``", ch, NULL, victim, TO_NOTVICT);
	check_improve(ch, skill, true, 1);

	heal = 1000;
	victim->hit = UMIN(victim->hit + heal, victim->max_hit);
	update_pos(victim);

	send_to_char("You feel `#b`@e`#t`@t`#e`@r``!\n\r", victim);

	/* stop the old fight */
	stop_fighting(fch, false);
	stop_fighting(victim, false);

	check_killer(ch, fch);

	/* start the new one */
	set_fighting(ch, fch);
	set_fighting(fch, ch);
	return;
}

void do_disarm(CHAR_DATA *ch, /*@unused@*/char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	SKILL *skill;
	int chance;
	int hth;
	int ch_weapon;
	int vict_weapon;
	int ch_vict_weapon;

	if ((skill = skill_lookup("disarm")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	hth = 0;
	if ((chance = get_learned_percent(ch, skill)) == 0) {
		send_to_char("You hum the tune to 'Disarm', since you can't really disarm someone.\n\r", ch);
		return;
	}

	if (get_eq_char(ch, WEAR_WIELD) == NULL
	    && ((hth = get_learned_percent(ch, gsp_hand_to_hand)) == 0
		|| (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_DISARM)))) {
		send_to_char("You must wield a weapon to disarm.\n\r", ch);
		return;
	}

	if ((victim = ch->fighting) == NULL) {
		send_to_char("You aren't fighting anyone.\n\r", ch);
		return;
	}

	if ((obj = get_eq_char(victim, WEAR_WIELD)) == NULL) {
		send_to_char("Your opponent is not wielding a weapon.\n\r", ch);
		return;
	}

	/* find weapon skills */
	ch_weapon = get_weapon_skill(ch, get_weapon_sn(ch, NULL));
	vict_weapon = get_weapon_skill(victim, get_weapon_sn(victim, NULL));
	ch_vict_weapon = get_weapon_skill(ch, get_weapon_sn(victim, NULL));

	/* modifiers */

	/* skill */
	if (get_eq_char(ch, WEAR_WIELD) == NULL)
		chance = chance * hth / 150;
	else
		chance = chance * ch_weapon / 100;

	chance += (ch_vict_weapon / 2 - vict_weapon) / 2;

/* dex vs. strength */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= get_curr_stat(victim, STAT_STR);

/* level */
	chance += (ch->level - victim->level) * 2;

/* and now the attack */
	if (number_percent() < chance) {
		disarm(ch, victim);
		WAIT_STATE(ch, skill->wait);
		check_improve(ch, skill, true, 1);
	} else {
		WAIT_STATE(ch, skill->wait);

		act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR);
		act("$n tries to disarm you, but fails.", ch, NULL, victim, TO_VICT);
		act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);

		check_improve(ch, skill, false, 1);
	}

	check_killer(ch, victim);
}

void do_surrender(CHAR_DATA *ch, /*@unused@*/char *argument)
{
	CHAR_DATA *mob;

	if ((mob = ch->fighting) == NULL) {
		send_to_char("But you're not fighting!\n\r", ch);
		return;
	}
	act("You surrender to $N!", ch, NULL, mob, TO_CHAR);
	act("$n surrenders to you!", ch, NULL, mob, TO_VICT);
	act("$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT);
	stop_fighting(ch, true);

	if (!IS_NPC(ch) && IS_NPC(mob)
	    && (!HAS_TRIGGER(mob, TRIG_SURR) || !mp_percent_trigger(mob, ch, NULL, NULL, TRIG_SURR))) {
		act("$N seems to ignore your cowardly act!", ch, NULL, mob, TO_CHAR);
		multi_hit(mob, ch, TYPE_UNDEFINED);
	}
}

void do_sla(CHAR_DATA *ch, /*@unused@*/char *argument)
{
	send_to_char("If you want to SLAY, spell it out.\n\r", ch);
}

void do_slay(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];

	(void)one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Slay whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch == victim) {
		send_to_char("Suicide is a mortal sin.\n\r", ch);
		return;
	}

	if (!IS_NPC(victim) && victim->level >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	act("You ```!slay ``$M in cold ```1blood``!", ch, NULL, victim, TO_CHAR);
	act("$n ```!slays ``you in cold ```1blood``!", ch, NULL, victim, TO_VICT);
	act("$n ```!slays ``$N in cold ```1blood``!", ch, NULL, victim, TO_NOTVICT);

	make_corpse(victim);
	raw_kill(victim, ch);
}

void do_intimidate(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	char buf[MSL];
	char arg[MIL];
	int chance;

	if ((skill = skill_lookup("intimidate")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if ((chance = get_learned_percent(ch, skill)) == 0) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		send_to_char("Who do you want to intimidate?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They're not here for you to intimidate.\n\r", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;


	if (victim == ch) {
		send_to_char("You're real afraid of yourself now.\n\r", ch);
		return;
	}

	if (ch->move < 80 || ch->mana < 100) {
		send_to_char("You don't have enough energy right now.\n\r", ch);
		return;
	}

	if (ch->size < victim->size)
		chance += (ch->size - victim->size) * 10;

	chance += (ch->level - victim->level);
	chance += 50;
	if (chance <= 5)
		chance = 5;

	if (number_percent() < chance) {
		send_to_char("You try your ability at intimidation.\n\r", ch);
		ch->move -= 150;
		ch->move = UMAX(ch->move, 1);

		spell_charm_person(skill_lookup("charm person"), (int)(ch->level * 2 / 3), ch, victim, 1, "");
	} else {
		ch->move -= 80;
		send_to_char("You are unable to muster up enough influence.\n\r", ch);
		(void)snprintf(buf, MSL, "%s tries to bully you and look intimidating.\n\r", ch->name);
		send_to_char(buf, victim);
		if IS_NPC(victim){
			if (number_percent() < 50) {
				printf_to_char(ch, "%s looks PISSED.\n\r", victim->name);
				multi_hit(victim, ch, (int)TRIG_FIGHT);
			}
		}
	}

	ch->mana -= 50;
}

/* ********************************************************************
 * Raise dead, based off the find familiar code
 * April 4 1998 by Gothar
 *
 * This skill allows your players to have a companion like those loveable pets in the pet shops.
 *  gothar@magma.ca
 * mcco0055@algonquinc.on.ca
 *
 * Implemented for BT by Monrick, March 2008
 * ******************************************************************** */
void do_familiar(/*@dependent@*/CHAR_DATA *ch, /*@unused@*/char *argument)
{
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *mount;
	SKILL *skill;
	int i, chance;

	if ((skill = skill_lookup("raise dead")) == NULL) {
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if ((chance = get_learned_percent(ch, skill)) <= 0) {
		send_to_char("You don't know where to start.\n\r", ch);
		return;
	}

	if (chance <= number_percent()) {
		send_to_char("You can't seem to concentrate.\n\r", ch);
		return;
	}

	if (ch->pet != NULL) {
		send_to_char("You all ready have a summoned creature.\n\r", ch);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		send_to_char("You can't study the ritual while in combat!\n\r", ch);
		return;
	}

	if ((pMobIndex = get_mob_index(MOB_VNUM_FAMILIAR)) == NULL) {
		send_to_char("The powers to summon the undead are too weak here to control.\n\r", ch);
		return;
	}
	/* can't cast the spell in these sectors */
	if (ch->in_room->sector_type == SECT_INSIDE
	    || ch->in_room->sector_type == SECT_WATER_SWIM
	    || ch->in_room->sector_type == SECT_WATER_NOSWIM
	    || ch->in_room->sector_type == SECT_UNDERWATER
	    || ch->in_room->sector_type == SECT_AIR) {
		send_to_char("The gods forbid you from raising the dead here.\n\r", ch);
		return;
	}

	mount = create_mobile(pMobIndex);

	mount->level = (int)(number_fuzzy(5) * number_fuzzy(273));
	mount->mana = mount->max_mana = 5339;
	mount->hit = mount->max_hit = (number_fuzzy(2194) * number_fuzzy(3) * number_fuzzy(3) * number_fuzzy(3) + 3728);
	for (i = 0; i < 4; i++)
		mount->armor[i] = -(number_fuzzy(13294) * (number_fuzzy(8)));
	mount->hitroll = (int)(number_fuzzy(434) * (number_fuzzy(8)));
	mount->damroll = (int)(number_fuzzy(163) * (number_fuzzy(8) + number_fuzzy(15)));
/*   mount->int = 25; */

	/* free up the old mob names */
	free_string(mount->description);
	free_string(mount->name);
	free_string(mount->short_descr);
	free_string(mount->long_descr);

	/* terrain*/
	switch (ch->in_room->sector_type) {
	case (SECT_CITY):
	case (SECT_FIELD):
/* Your basic zombie */
		mount->description =
			str_dup("The decaying corpse of a once might warrior stands before you.\n\r"
				"You can feel the dirt and disease crawling off this animated corpse.\n\r");
		mount->short_descr = str_dup("zombie warrior");
		mount->long_descr = str_dup("The body of a rotting animated body stands here.\n\r");
		mount->name = str_dup("summoned zombie");
		mount->dam_type = 14; /* scratch */
		break;

/* Your basic skeleton */
	case (SECT_FOREST):  /* skeleton */
	case (SECT_HILLS):
		mount->description =
			str_dup("You see a skeleton of a decomped half giant. A blue aura radiates from around him.\n\r"
				"Though this animated creature is devoid of flesh you can feel the power radiate from him\n\r");
		mount->short_descr = str_dup("skeleton half giant");
		mount->long_descr = str_dup("A animated skeleton stands here defending its master.\n\r");
		mount->name = str_dup("summoned skeleton");
		mount->dam_type = 46; /* claw */
		break;
	case (SECT_MOUNTAIN):
/*Specter */
		mount->description =
			str_dup("The spectral remains of what appears to have once been an elf floats before you with a eerie blue glow. Its face is locked in a never ending scream of pain and suffering.\n\r");
		mount->short_descr = str_dup("Specter");
		mount->long_descr = str_dup("A translucent glowing form of a elf floats here..\n\r");
		mount->name = str_dup("familiar mountain lion");
		mount->dam_type = 39;   /* bite */
		break;
	case (SECT_DESERT):             /* Mummy*/
		mount->description =
			str_dup("The dried out husk of a tribal warrior with a never ending thirst stands before you.\n\r"
				"The mummified remains glow with a faint blue aura of dark magic.\n\r");

		mount->short_descr = str_dup("mummy");
		mount->long_descr = str_dup("A mummified tribal warrior stands here\n\r");
		mount->name = str_dup("mummy warrior");
		mount->dam_type = 33; /* suction */
		break;
	}
	/* player seen stuff here */
	sit(ch, NULL);
	char_to_room(mount, ch->in_room);
	act("You kneel to the ground and begin to carve dark symbols into the ground. You prepare the spell to call a $N!.", ch, NULL, mount, TO_CHAR);
	act("$n kneles to the ground and begins carving dark symbols into the ground. $n calls to a $N!", ch, NULL, mount, TO_ROOM);     WAIT_STATE(ch, 2 * PULSE_MOBILE);
	add_follower(mount, ch);
	mount->leader = ch;
	ch->pet = mount;
	stand(ch, NULL);

	SET_BIT(mount->act, ACT_PET);
	SET_BIT(mount->affected_by, AFF_CHARM);
	ch->move /= 2; /* physically draining lose of move */
	check_improve(ch, skill, true, 6);
}

