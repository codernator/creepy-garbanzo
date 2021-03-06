#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"



extern struct dynamic_skill *gsp_deft;
extern struct dynamic_skill *gsp_dash;
extern struct dynamic_skill *gsp_mana_vortex;


/***************************************************************************
 *	local functions
 ***************************************************************************/
static void say_spell(struct char_data * ch, struct dynamic_skill * skill);


/***************************************************************************
 *	imported functions
 ***************************************************************************/
extern bool remove_obj(struct char_data * ch, int iWear, bool fReplace);
extern void wear_obj(struct char_data * ch, struct gameobject * obj, bool fReplace);
extern void dam_message(struct char_data * ch, struct char_data * victim, int dam, int dt, bool immune);



/***************************************************************************
 *	find_spell
 *
 *	finds a a spell that a character can cast if possible
 ***************************************************************************/
int find_spell(struct char_data *ch, const char *name)
{
    struct dynamic_skill *skill;
    struct learned_info *learned;
    struct level_info *level;
    int found = -1;


    skill = skill_lookup((char *)name);
    if (IS_NPC(ch))
	return skill->sn;

    learned = get_learned_skill(ch, skill);
    level = get_skill_level(ch, skill);
    if (learned != NULL && level == NULL && level->level >= ch->level)
	return skill->sn;

    return found;
}


/***************************************************************************
 *	spell_lookup
 *
 *	lookup a dynamic skill
 ***************************************************************************/
static struct dynamic_skill *spell_lookup(struct char_data *ch, char *name)
{
    struct dynamic_skill *skill;
    struct dynamic_skill *skill_tmp;
    int percent;

    skill_tmp = NULL;
    for (skill = skill_list; skill != NULL; skill = skill->next) {
	/* short-circuit the loop if we dont have a spell
	 * or the first letters dont match */
	if (skill->spells == NULL
		|| skill->name[0] != name[0])
	    continue;

	if ((percent = get_learned_percent(ch, skill)) <= 0)
	    continue;

	if (!str_cmp(name, skill->name))
	    return skill;

	if (skill_tmp == NULL && !str_prefix(name, skill->name))
	    skill_tmp = skill;

    }

    return skill_tmp;
}


/***************************************************************************
 *	say_spell
 *
 *	say gibberish incantation of a spell
 ***************************************************************************/
static void say_spell(struct char_data *ch, struct dynamic_skill *skill)
{
    struct char_data *rch;
    char buf[MAX_STRING_LENGTH];
    char buf_nonclass[MAX_STRING_LENGTH];
    char buf_inclass[MAX_STRING_LENGTH];
    char *pName;
    int iSyl;
    int length;

    struct syl_type {
	char *	old;
	char *	new;
    };

    static const struct syl_type syl_table[] =
    {
	{ " ",	   " "	    },
	{ "ar",	   "ooga"   },
	{ "au",	   "kada"   },
	{ "bless", "fido"   },
	{ "blind", "nose"   },
	{ "bur",   "mosa"   },
	{ "cu",	   "mofu"   },
	{ "de",	   "ocumbo" },
	{ "en",	   "unba"   },
	{ "light", "zinx"   },
	{ "lo",	   "hi"	    },
	{ "mor",   "zak"    },
	{ "move",  "sido"   },
	{ "ness",  "necri"  },
	{ "ning",  "illa"   },
	{ "per",   "duda"   },
	{ "ra",	   "gru"    },
	{ "fresh", "ikki"   },
	{ "re",	   "carkus" },
	{ "son",   "surfu"  },
	{ "tect",  "infra"  },
	{ "tri",   "cula"   },
	{ "ven",   "nofo"   },
	{ "a",	   "a"	    },
	{ "b",	   "b"	    },
	{ "c",	   "q"	    },
	{ "d",	   "e"	    },
	{ "e",	   "z"	    },
	{ "f",	   "y"	    },
	{ "g",	   "o"	    },
	{ "h",	   "p"	    },
	{ "i",	   "u"	    },
	{ "j",	   "y"	    },
	{ "k",	   "t"	    },
	{ "l",	   "r"	    },
	{ "m",	   "w"	    },
	{ "n",	   "i"	    },
	{ "o",	   "a"	    },
	{ "p",	   "s"	    },
	{ "q",	   "d"	    },
	{ "r",	   "f"	    },
	{ "s",	   "g"	    },
	{ "t",	   "h"	    },
	{ "u",	   "j"	    },
	{ "v",	   "z"	    },
	{ "w",	   "x"	    },
	{ "x",	   "n"	    },
	{ "y",	   "l"	    },
	{ "z",	   "k"	    },
	{ "",	   ""	    }
    };


    buf[0] = '\0';

    for (pName = skill->name; *pName != '\0'; pName += length) {
	for (iSyl = 0; (length = (int)strlen(syl_table[iSyl].old)) != 0; iSyl++) {
	    if (!str_prefix(syl_table[iSyl].old, pName)) {
		strcat(buf, syl_table[iSyl].new);
		break;
	    }
	}

	if (length == 0)
	    length = 1;
    }

    sprintf(buf_nonclass, "$n chants the words, '```5%s``'.", buf);
    sprintf(buf_inclass, "$n chants the words, '```5%s``'.", skill->name);

    for (rch = ch->in_room->people; rch; rch = rch->next_in_room)
	if (rch != ch)
	    act(ch->class == rch->class ? buf_inclass : buf_nonclass, ch, NULL, rch, TO_VICT);

    return;
}


/***************************************************************************
 *	saves_spell
 *
 *	compute a saving throw
 *	negative applies make the saving throw easier
 ***************************************************************************/
bool saves_spell(int level, struct char_data *victim, int dam_type)
{
    int vch_lvl;
    int save;


    /*
     * original formula
     * save = (100 + (vch_lvl - level)) * 5 - victim->saving_throw;
     */

    /* effective victim level = level + ((WIS + INT) / 2) / 3 */
    vch_lvl = victim->level + ((get_curr_stat(victim, STAT_INT) + get_curr_stat(victim, STAT_WIS)) / 6);
    save = 40 + (vch_lvl - level) - (victim->saving_throw / 5);

    /* berserked victims get some extra funk */
    if (IS_AFFECTED(victim, AFF_BERSERK))
	save += vch_lvl / 20;

    switch (check_immune(victim, dam_type)) {
	case IS_IMMUNE:
	    return true;
	case IS_RESISTANT:
	    save += 20;
	    break;
	case IS_VULNERABLE:
	    save -= 20;
	    break;
    }


    /* spell casters are more succeptible */
    if (!IS_NPC(victim)
	    && class_table[victim->class].fMana)
	save = 9 * save / 10;

    save = URANGE(2, save, 98);
    return number_percent() < save;
}


/***************************************************************************
 *	saves_dispell
 *
 *	save vs. dispel
 ***************************************************************************/
bool saves_dispel(int dis_level, int spell_level, int duration)
{
    int save;

    if (duration == -1)
	spell_level += 8;

    save = 50 + (spell_level - dis_level) * 5;
    save = URANGE(2, save, 98);

    return number_percent() < save;
}

/***************************************************************************
 *	check_dispell
 *
 *	co-routine for dispel magic and cancellation
 ***************************************************************************/
bool check_dispel(int dis_level, struct char_data *victim, struct dynamic_skill *skill)
{
    struct affect_data *af;

    if (skill == NULL)
	return false;

    if (is_affected(victim, skill)) {
	for (af = victim->affected; af != NULL; af = af->next) {
	    if (af->type == skill->sn) {
		if (!saves_dispel(dis_level, af->level, af->duration)) {
		    affect_strip(victim, skill);
		    if (skill->msg != NULL) {
			send_to_char(skill->msg, victim);
			send_to_char("\n\r", victim);
		    }

		    if (skill->msg_others != NULL
			    && skill->msg_others[0] != '\0')
			act(skill->msg_others, victim, NULL, NULL, TO_ROOM);
		    return true;
		} else {
		    af->level--;
		}
	    }
	}
    }

    return false;
}


/***************************************************************************
 *	remove_all_affects
 *
 *	removes all of the affects on a victim
 ***************************************************************************/
void remove_all_affects(struct char_data *victim)
{
    struct affect_data *af;
    struct dynamic_skill *skill;

    while ((af = victim->affected) != NULL) {
	if ((skill = resolve_skill_affect(af)) != NULL) {
	    if (skill->msg) {
		send_to_char(skill->msg, victim);
		send_to_char("\n\r", victim);
	    }
	    affect_strip(victim, skill);
	} else {
	    break;
	}
    }
}


/***************************************************************************
 *	do_cast
 *
 *	cast a spell
 ***************************************************************************/
void do_cast(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj;
    struct learned_info *learned;
    struct level_info *level_info;
    struct dynamic_skill *skill;
    char spell[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    void *vo;
    int mana;
    int target;
    int wait;
    int chance;
    int level;
    int level_mod;

    /*
     * Switched NPC's can cast spells, but others can't.
     */
    argument = one_argument(argument, spell);
    one_argument(argument, arg);

    if (spell[0] == '\0') {
	send_to_char("Cast which what where?\n\r", ch);
	return;
    }

    skill = spell_lookup(ch, spell);
    if (skill == NULL || skill->spells == NULL) {
	send_to_char("You don`8'``t know that `1s`!p`&e`!l`1l``!\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)) {
	learned = get_learned_skill(ch, skill);
	level_info = get_skill_level(ch, skill);

	if (learned == NULL || level_info == NULL || level_info->level > ch->level) {
	    send_to_char("You don`8'``t know that `1s`!p`&e`!l`1l``!\n\r", ch);
	    return;
	}
    }


    if (ch->position < skill->min_pos) {
	send_to_char("You're too distracted.\n\r", ch);
	return;
    }

    /*
     * Locate targets.
     */
    victim = NULL;
    obj = NULL;
    vo = NULL;
    target = TARGET_NONE;

    mana = skill->min_mana;
    wait = skill->wait;
    level_mod = (get_curr_stat(ch, STAT_INT) + get_curr_stat(ch, STAT_WIS)) / 6;
    level = (IS_NPC(ch) || class_table[ch->class].fMana) ? ch->level : (int)((ch->level * 7) / 8);

    switch (skill->target) {
	default:
	    log_bug("Do_cast: bad target for sn %d.", skill->sn);
	    return;

	case TAR_IGNORE:
	case TAR_ROOM:
	    break;

	case TAR_CHAR_OFFENSIVE:
	    level_mod = get_curr_stat(ch, STAT_INT) / 5;
	    if (arg[0] == '\0') {
		if ((victim = ch->fighting) == NULL) {
		    send_to_char("Cast the `1s`!p`&e`!l`1l`` on whom?\n\r", ch);
		    return;
		}
	    } else {
		if ((victim = get_char_room(ch, argument)) == NULL) {
		    if ((obj = get_eq_char(ch, WEAR_HOLD)) != NULL
			    && OBJECT_TYPE(obj) == ITEM_DOLL
			    && obj->target != NULL
			    && is_name(argument, obj->target->name)) {
			victim = obj->target;
		    } else {
			send_to_char("``They aren`8'``t here.\n\r", ch);
			return;
		    }
		}
	    }

	    if ((victim->level <= 10)
		    && (!IS_NPC(victim))) {
		send_to_char("Newbies are protected from `3s`2c`3u`2m`` like you!\n\r", ch);
		return;
	    }

	    if ((ch->level <= 10)
		    && (!IS_NPC(victim))) {
		send_to_char("Silly newbie .. try leveling a bit first\n\r", ch);
		return;
	    }

	    if ((!IS_NPC(victim))
		    && (IS_SET(victim->act, PLR_LINKDEAD)))
		send_to_char("That player is currently [`8LINKDEAD`7] ..\n\r", ch);

	    if (!IS_NPC(ch)) {
		if (victim != ch && is_safe(ch, victim)) {
		    send_to_char("Not on that target.\n\r", ch);
		    return;
		}
		check_killer(ch, victim);
	    }

	    if (IS_AFFECTED(ch, AFF_CHARM)
		    && ch->master == victim) {
		send_to_char("``You can`8'``t do that on your own follower.\n\r", ch);
		return;
	    }

	    vo = (void *)victim;
	    target = TARGET_CHAR;
	    break;

	case TAR_CHAR_DEFENSIVE:
	    level_mod = get_curr_stat(ch, STAT_WIS) / 3;
	    if (arg[0] == '\0') {
		victim = ch;
	    } else {
		if ((victim = get_char_room(ch, argument)) == NULL) {
		    if ((obj = get_eq_char(ch, WEAR_HOLD)) != NULL
			    && OBJECT_TYPE(obj) == ITEM_DOLL
			    && obj->target != NULL
			    && is_name(argument, obj->target->name)) {
			victim = obj->target;
		    } else {
			send_to_char("They aren't here.\n\r", ch);
			return;
		    }
		}
	    }

	    vo = (void *)victim;
	    target = TARGET_CHAR;
	    break;

	case TAR_CHAR_SELF:
	    level_mod = get_curr_stat(ch, STAT_WIS) / 3;
	    if (arg[0] != '\0' && !is_name(argument, ch->name)) {
		send_to_char("``You cannot cast this `1s`!p`&e`!l`1l`` on another.\n\r", ch);
		return;
	    }

	    vo = (void *)ch;
	    target = TARGET_CHAR;
	    break;

	case TAR_OBJ_INV:
	    if (arg[0] == '\0') {
		send_to_char("``What should the `1s`!p`&e`!l`1l`` be cast upon?\n\r", ch);
		return;
	    }

	    if ((obj = get_obj_carry(ch, argument)) == NULL) {
		send_to_char("You are not carrying that.\n\r", ch);
		return;
	    }

	    vo = (void *)obj;
	    target = TARGET_OBJ;
	    break;

	case TAR_OBJ_CHAR_OFF:
	    level_mod = get_curr_stat(ch, STAT_INT) / 5;
	    if (arg[0] == '\0') {
		if ((victim = ch->fighting) == NULL) {
		    send_to_char("``Cast the `1s`!p`&e`!l`1l`` on whom or what?\n\r", ch);
		    return;
		}

		target = TARGET_CHAR;
	    } else if ((victim = get_char_room(ch, argument)) != NULL) {
		target = TARGET_CHAR;
	    } else if ((obj = get_eq_char(ch, WEAR_HOLD)) != NULL
		    && OBJECT_TYPE(obj) == ITEM_DOLL
		    && obj->target != NULL
		    && is_name(argument, obj->target->name)) {
		victim = obj->target;
		target = TARGET_CHAR;
	    }

	    if (target == TARGET_CHAR) { /* check the sanity of the attack */
		if (is_safe_spell(ch, victim, false) && victim != ch) {
		    send_to_char("Not on that target.\n\r", ch);
		    return;
		}

		if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		    send_to_char("``You can`8'``t do that on your own follower.\n\r", ch);
		    return;
		}

		if (!IS_NPC(ch))
		    check_killer(ch, victim);


		vo = (void *)victim;
	    } else if ((obj = get_obj_here(ch, argument)) != NULL) {
		vo = (void *)obj;
		target = TARGET_OBJ;
	    } else {
		send_to_char("``You don`8'``t see that here.\n\r", ch);
		return;
	    }
	    break;

	case TAR_OBJ_CHAR_DEF:
	    level_mod = get_curr_stat(ch, STAT_WIS) / 3;
	    if (arg[0] == '\0') {
		victim = ch;

		vo = (void *)ch;
		target = TARGET_CHAR;
	    } else if ((victim = get_char_room(ch, argument)) != NULL) {
		vo = (void *)victim;
		target = TARGET_CHAR;
	    } else if ((obj = get_obj_carry(ch, argument)) != NULL) {
		vo = (void *)obj;
		target = TARGET_OBJ;
	    } else {
		send_to_char("``You don`8'``t see that here.\n\r", ch);
		return;
	    }
	    break;
    }

    if (ch->mana < mana) {
	if (!IS_NPC(ch))
	    send_to_char("``You don`8'``t have enough mana.\n\r", ch);
	return;
    }

    if (str_cmp(skill->name, "ventriloquate"))
	say_spell(ch, skill);

    if (ch->level < LEVEL_IMMORTAL) {
	if (is_affected(ch, gsp_deft))
	    wait /= 2;

	if (is_affected(ch, gsp_dash))
	    wait /= 2;

	WAIT_STATE(ch, wait);
    }


    /*chance =(is_affected(ch, gsn_web)) ? get_skill(ch, sn) - 20 : get_skill(ch, sn);*/
    chance = get_learned_percent(ch, skill);
    if (number_percent() > chance) {
	send_to_char("You lost your concentration.\n\r", ch);
	check_improve(ch, skill, false, 1);
	ch->mana -= mana / 2;
    } else {
	if (!IS_IMMORTAL(ch)) {
	    if (gsp_mana_vortex != NULL
		    && is_affected_room(ch->in_room, gsp_mana_vortex)) {
		int dam;

		act("The room draws on your `Pm`5y`^s`6ti`^c`5a`Pl e`5n`^e`6rg`^i`5e`Ps`` and feeds them back at you!!", ch, NULL, NULL, TO_CHAR);
		act("The room draws on $n's `Pm`5y`^s`6ti`^c`5a`Pl e`5n`^e`6rg`^i`5e`Ps`` and feeds them back at $m!!", ch, NULL, NULL, TO_ROOM);
		if ((dam = damage(ch, ch, mana * 100, gsp_mana_vortex->sn, DAM_NEGATIVE, false)) > 0) {
		    dam_message(ch, NULL, dam, gsp_mana_vortex->sn, false);
		    ch->mana -= dam;
		}
		return;
	    } else {
		ch->mana -= mana;
	    }
	}

	if (IS_AFFECTED(ch, AFF_BERSERK))
	    level -= level / 30;

	/* add the level modifier for INT and/or WIS */
	if (level_mod > 0)
	    level += (level_mod - 1);

	cast_spell(ch, skill, level, vo, target, argument);
	check_improve(ch, skill, true, 1);
    }

    if ((skill->target == TAR_CHAR_OFFENSIVE || (skill->target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))) {
	if (victim != ch && victim->master != ch) {
	    struct char_data *vch;
	    struct char_data *vch_next;

	    for (vch = ch->in_room->people; vch; vch = vch_next) {
		vch_next = vch->next_in_room;
		if (victim == vch && victim->fighting == NULL) {
		    check_killer(victim, ch);
		    multi_hit(victim, ch, TYPE_UNDEFINED);
		    break;
		}
	    }
	}
    }

    return;
}





/***************************************************************************
 *	obj_cast_spell
 *
 *	cast a spell using a magical object
 ***************************************************************************/
void obj_cast_spell(int	sn, int	level, struct char_data * ch, struct char_data * victim, struct gameobject *	obj)
{
    struct dynamic_skill *skill;
    void *vo;
    int target = TARGET_NONE;


    if (sn <= 0)
	return;

    skill = resolve_skill_sn(sn);
    if (skill == NULL || skill->spells == NULL) {
	log_bug("Obj_cast_spell: bad sn %d.", sn);
	return;
    }

    switch (skill->target) {
	default:
	    log_bug("Obj_cast_spell: bad target for sn %d.", sn);
	    return;

	case TAR_IGNORE:
	case TAR_ROOM:
	    vo = NULL;
	    break;

	case TAR_CHAR_OFFENSIVE:
	    if (victim == NULL)
		victim = ch->fighting;

	    if (victim == NULL) {
		send_to_char("You can't do that.\n\r", ch);
		return;
	    }

	    if (is_safe(ch, victim) && ch != victim) {
		send_to_char("Something isn't right...\n\r", ch);
		return;
	    }

	    vo = (void *)victim;
	    target = TARGET_CHAR;
	    break;

	case TAR_CHAR_DEFENSIVE:
	case TAR_CHAR_SELF:
	    if (victim == NULL)
		victim = ch;

	    vo = (void *)victim;
	    target = TARGET_CHAR;
	    break;

	case TAR_OBJ_INV:
	    if (obj == NULL) {
		send_to_char("You can't do that.\n\r", ch);
		return;
	    }
	    vo = (void *)obj;
	    target = TARGET_OBJ;
	    break;

	case TAR_OBJ_CHAR_OFF:
	    if (victim == NULL && obj == NULL) {
		if (ch->fighting != NULL) {
		    victim = ch->fighting;
		} else {
		    send_to_char("You can't do that.\n\r", ch);
		    return;
		}
	    }

	    if (victim != NULL) {
		if (is_safe_spell(ch, victim, false) && ch != victim) {
		    send_to_char("Something isn't right...\n\r", ch);
		    return;
		}

		vo = (void *)victim;
		target = TARGET_CHAR;
	    } else {
		vo = (void *)obj;
		target = TARGET_OBJ;
	    }
	    break;

	case TAR_OBJ_CHAR_DEF:
	    if (victim == NULL && obj == NULL) {
		vo = (void *)ch;
		target = TARGET_CHAR;
	    } else if (victim != NULL) {
		vo = (void *)victim;
		target = TARGET_CHAR;
	    } else {
		vo = (void *)obj;
		target = TARGET_OBJ;
	    }

	    break;
    }

    cast_spell(ch, skill, level, vo, target, "");

    if ((skill->target == TAR_CHAR_OFFENSIVE
		|| (skill->target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))) {
	if (victim != ch
		&& victim->master != ch) {
	    struct char_data *vch;
	    struct char_data *vch_next;

	    for (vch = ch->in_room->people; vch; vch = vch_next) {
		vch_next = vch->next_in_room;
		if (victim == vch && victim->fighting == NULL) {
		    check_killer(victim, ch);
		    multi_hit(victim, ch, TYPE_UNDEFINED);
		    break;
		}
	    }
	}
    }

    return;
}




/***************************************************************************
 *	do_quaff
 ***************************************************************************/
void do_quaff(struct char_data *ch, const char *argument)
{
    struct gameobject *obj;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Quaff what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You do not have that potion.\n\r", ch);
        return;
    }

    if (OBJECT_TYPE(obj) != ITEM_POTION) {
        send_to_char("You can quaff only potions.\n\r", ch);
        return;
    }

    act("$n quaffs $p.", ch, obj, NULL, TO_ROOM);
    act("You quaff $p.", ch, obj, NULL, TO_CHAR);

    obj_cast_spell((int)obj->value[1], (int)obj->value[0], ch, ch, NULL);
    obj_cast_spell((int)obj->value[2], (int)obj->value[0], ch, ch, NULL);
    obj_cast_spell((int)obj->value[3], (int)obj->value[0], ch, ch, NULL);
    obj_cast_spell((int)obj->value[4], (int)obj->value[0], ch, ch, NULL);

    extract_obj(obj);
    return;
}


/***************************************************************************
 *	do_recite
 ***************************************************************************/
void do_recite(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    struct gameobject *scroll;
    struct gameobject *obj;
    struct dynamic_skill *skill;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if ((skill = skill_lookup("scrolls")) == NULL) {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if ((scroll = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have that scroll.\n\r", ch);
        return;
    }

    if (OBJECT_TYPE(scroll) != ITEM_SCROLL) {
        send_to_char("You can recite only scrolls.\n\r", ch);
        return;
    }

    obj = NULL;
    if (arg2[0] == '\0') {
        victim = ch;
    } else {
        if ((victim = get_char_room(ch, arg2)) == NULL && (obj = get_obj_here(ch, arg2)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    act("$n recites $p.", ch, scroll, NULL, TO_ROOM);
    act("You recite $p.", ch, scroll, NULL, TO_CHAR);

    if (number_percent() >= 20 + get_learned_percent(ch, skill) * 4 / 5) {
        send_to_char("You mispronounce a syllable.\n\r", ch);
        check_improve(ch, skill, false, 2);
    } else {
        obj_cast_spell((int)scroll->value[1], (int)scroll->value[0], ch, victim, obj);
        obj_cast_spell((int)scroll->value[2], (int)scroll->value[0], ch, victim, obj);
        obj_cast_spell((int)scroll->value[3], (int)scroll->value[0], ch, victim, obj);
        obj_cast_spell((int)scroll->value[4], (int)scroll->value[0], ch, victim, obj);
        check_improve(ch, skill, true, 2);
    }

    extract_obj(scroll);
    return;
}



/***************************************************************************
 *	do_brandish
 ***************************************************************************/
void do_brandish(struct char_data *ch, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;
    struct gameobject *staff;
    struct dynamic_skill *skill;
    struct dynamic_skill *cast;

    if ((skill = skill_lookup("staves")) == NULL) {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (OBJECT_TYPE(staff) != ITEM_STAFF) {
        send_to_char("You can brandish only with a staff.\n\r", ch);
        return;
    }

    if ((cast = resolve_skill_sn((int)staff->value[3])) == NULL || cast->spells == NULL) {
        log_bug("Do_brandish: bad sn %d.", staff->value[3]);
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (staff->value[2] > 0) {
        act("$n brandishes $p.", ch, staff, NULL, TO_ROOM);
        act("You brandish $p.", ch, staff, NULL, TO_CHAR);
        for (vch = ch->in_room->people; vch; vch = vch_next) {
            vch_next = vch->next_in_room;

            switch (cast->target) {
                default:
                log_bug("Do_brandish: bad target for sn %d.", staff->value[3]);
                return;

                case TAR_IGNORE:
                    if (vch != ch)
                        continue;
                    break;
                        case TAR_CHAR_OFFENSIVE:
                    if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch))
                        continue;
                    break;

                case TAR_CHAR_DEFENSIVE:
                    if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))
                        continue;
                    break;

                case TAR_CHAR_SELF:
                    if (vch != ch)
                        continue;
                    break;
            }

            obj_cast_spell((int)staff->value[3], (int)staff->value[0], ch, vch, NULL);
            check_improve(ch, skill, true, 2);
        }
    }

    if (--staff->value[2] <= 0) {
        act("Your $p seems to be powerless!", ch, staff, NULL, TO_CHAR);
        staff->value[2] = 0;
    }

    return;
}


/***************************************************************************
 *	do_zap
 ***************************************************************************/
void do_zap(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    struct gameobject *wand;
    struct gameobject *obj;
    struct dynamic_skill *skill;
    char arg[MAX_INPUT_LENGTH];

    if ((skill = skill_lookup("wands")) == NULL) {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0' && ch->fighting == NULL) {
        send_to_char("Zap whom or what?\n\r", ch);
        return;
    }

    if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (OBJECT_TYPE(wand) != ITEM_WAND) {
        send_to_char("You can zap only with a wand.\n\r", ch);
        return;
    }

    obj = NULL;
    if (arg[0] == '\0') {
        if (ch->fighting != NULL) {
            victim = ch->fighting;
        } else {
            send_to_char("Zap whom or what?\n\r", ch);
            return;
        }
    } else {
        if ((victim = get_char_room(ch, arg)) == NULL && (obj = get_obj_here(ch, arg)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (wand->value[2] > 0) {
        if (victim != NULL) {
            act("$n zaps $N with $p.", ch, wand, victim, TO_ROOM);
            act("You zap $N with $p.", ch, wand, victim, TO_CHAR);
        } else {
            act("$n zaps $P with $p.", ch, wand, obj, TO_ROOM);
            act("You zap $P with $p.", ch, wand, obj, TO_CHAR);
        }

        obj_cast_spell((int)wand->value[3], (int)wand->value[0], ch, victim, obj);
        check_improve(ch, skill, true, 2);
    }

    if (--wand->value[2] <= 0)
        act("Your $p seems powerless.", ch, wand, NULL, TO_CHAR);

    return;
}


/***************************************************************************
 *	cast_spell
 *
 *	cast a spell - should be able to be tweaked for spell-chaining
 ***************************************************************************/
void cast_spell(struct char_data *ch, struct dynamic_skill *skill, int level, void *vo, int target, const char *argument)
{
    struct spell_list *spells;

    if (skill != NULL && skill->spells != NULL) {
	for (spells = skill->spells; spells != NULL; spells = spells->next)
	    (*spells->spell_fn)(skill, level, ch, vo, target, argument);
    }
}


/***************************************************************************
 *	do_deft
 ***************************************************************************/
void do_deft(struct char_data *ch, const char *argument)
{
    struct affect_data af;
    struct dynamic_skill *skill;
    int percent;

    if ((skill = gsp_deft) == NULL) {
	send_to_char("`!WHAT``\n\r`PTHE``\n\r`OFUCK?``\n\r", ch);
	return;
    }

    if ((percent = get_learned_percent(ch, skill)) < 1) {
	send_to_char("`!WHAT``\n\r`PTHE``\n\r`OFUCK?``\n\r", ch);
	return;
    }

    if (is_affected(ch, skill)) {
	send_to_char("You are already deft enough.\n\r", ch);
	return;
    }

    if ((number_percent() < percent)) {
	send_to_char("You feel your mental capacity double.\n\r", ch);
	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = ch->level;
	af.duration = -1;
	af.modifier = 0;
	af.bitvector = 0;
	af.location = APPLY_NONE;

	affect_to_char(ch, &af);
	check_improve(ch, skill, true, 3);
    } else {
	send_to_char("You try to concentrate on your magic, but fail miserably..\n\r", ch);
	check_improve(ch, skill, false, 3);
    }

    return;
}

/***************************************************************************
 *	do_dash
 ***************************************************************************/
void do_dash(struct char_data *ch, const char *argument)
{
    struct affect_data af;
    struct dynamic_skill *skill;
    int percent;

    if ((skill = gsp_dash) == NULL) {
	send_to_char("`!WHAT``\n\r`PTHE``\n\r`OFUCK?``\n\r", ch);
	return;
    }

    if ((percent = get_learned_percent(ch, skill)) < 1) {
	send_to_char("`!WHAT``\n\r`PTHE``\n\r`OFUCK?``\n\r", ch);
	return;
    }


    if (is_affected(ch, skill)) {
	send_to_char("Your little brain cannot race any faster.\n\r", ch);
	return;
    }

    if ((number_percent() < percent)) {
	send_to_char("Your heart races and your mind quickens.\n\r", ch);
	af.where = TO_AFFECTS;
	af.type = skill->sn;
	af.skill = skill;
	af.level = ch->level;
	af.duration = -1;
	af.modifier = 0;
	af.bitvector = 0;
	af.location = APPLY_NONE;

	affect_to_char(ch, &af);
	check_improve(ch, skill, true, 3);
    } else {
	send_to_char("You try to steel your mental prowess, but you are too stupid..\n\r", ch);
	check_improve(ch, skill, false, 3);
    }

    return;
}
