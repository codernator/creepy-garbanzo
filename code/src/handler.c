
#include <stdio.h>
#include "merc.h"
#include "character.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "olc.h"
#include "interp.h"


extern void bug_long(const char *str, long param);
extern char *flag_string(const struct flag_type *flag_table, long bits);
extern void affect_modify(CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd);
extern void blood_rage(CHAR_DATA * ch);
extern long parse_long(char *test);

/* external functions */
extern void affect_join_obj(OBJ_DATA * obj, AFFECT_DATA * paf);


/***************************************************************************
*	local functions
***************************************************************************/

/***************************************************************************
*	is_help
*
*	checks to see if a character string is a cry for help
***************************************************************************/
bool is_help(char *argument)
{
	if (argument[0] == '\0'
	    || argument[0] == '?'
	    || !str_prefix(argument, "help"))
		return TRUE;

	return FALSE;
}


/***************************************************************************
*	lookup functions
***************************************************************************/
/***************************************************************************
*	material_lookup
***************************************************************************/
int material_lookup(const char *name)
{
	return 0;
}

/***************************************************************************
*	weapon_lookup
***************************************************************************/
int weapon_lookup(const char *name)
{
	int type;

	for (type = 0; weapon_table[type].name != NULL; type++) {
		if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
		    && !str_prefix(name, weapon_table[type].name))
			return type;
	}

	return -1;
}


/***************************************************************************
*	weapon_type
***************************************************************************/
int weapon_type(const char *name)
{
	int type;

	type = weapon_lookup(name);
	if (type >= 0 && weapon_table[type].name != NULL)
		return weapon_table[type].type;

	return WEAPON_EXOTIC;
}


/***************************************************************************
*	item_name
***************************************************************************/
char *item_name(int item_type)
{
	int type;

	for (type = 0; item_table[type].name != NULL; type++)
		if (item_type == item_table[type].type)
			return item_table[type].name;

	return "none";
}

/***************************************************************************
*	weapon_name
***************************************************************************/
char *weapon_name(int weapon_type)
{
	int type;

	for (type = 0; weapon_table[type].name != NULL; type++)
		if (weapon_type == weapon_table[type].type)
			return weapon_table[type].name;

	return "exotic";
}

/***************************************************************************
*	attack_lookup
***************************************************************************/
int attack_lookup(const char *name)
{
	int att;

	for (att = 0; attack_table[att].noun != NULL; att++) {
		if (LOWER(name[0]) == LOWER(attack_table[att].noun[0])
		    && !str_prefix(name, attack_table[att].noun))
			return att;
	}

	return 0;
}

/***************************************************************************
*	wiznet_lookup
***************************************************************************/
long wiznet_lookup(const char *name)
{
	int flag;

	for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
		if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
		    && !str_prefix(name, wiznet_table[flag].name))
			return flag;
	}

	return -1;
}


/***************************************************************************
*	impnet_lookup
***************************************************************************/
long impnet_lookup(const char *name)
{
	int flag;

	for (flag = 0; impnet_table[flag].name != NULL; flag++) {
		if (LOWER(name[0]) == LOWER(impnet_table[flag].name[0])
		    && !str_prefix(name, impnet_table[flag].name))
			return flag;
	}

	return -1;
}


/***************************************************************************
*	class_lookup
***************************************************************************/
int class_lookup(const char *name)
{
	int class;

	for (class = 0; class < MAX_CLASS; class++) {
		if (LOWER(name[0]) == LOWER(class_table[class].name[0])
		    && !str_prefix(name, class_table[class].name))
			return class;
	}

	return -1;
}


/***************************************************************************
*	is_friend
***************************************************************************/
bool is_friend(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (is_same_group(ch, victim))
		return TRUE;

	if (!IS_NPC(ch))
		return FALSE;

	if (!IS_NPC(victim)) {
		if (IS_SET(ch->off_flags, ASSIST_PLAYERS))
			return TRUE;
		else
			return FALSE;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))
		return FALSE;

	if (IS_SET(ch->off_flags, ASSIST_ALL))
		return TRUE;

	if (ch->group && ch->group == victim->group)
		return TRUE;


	if (IS_SET(ch->off_flags, ASSIST_VNUM)
	    && ch->mob_idx == victim->mob_idx)
		return TRUE;

	if (IS_SET(ch->off_flags, ASSIST_RACE) && ch->race == victim->race)
		return TRUE;

	if (IS_SET(ch->off_flags, ASSIST_ALIGN)
	    && !IS_SET(ch->act, ACT_NOALIGN) && !IS_SET(victim->act, ACT_NOALIGN)
	    && ((IS_GOOD(ch) && IS_GOOD(victim))
		|| (IS_EVIL(ch) && IS_EVIL(victim))
		|| (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
		return TRUE;

	return FALSE;
}

/***************************************************************************
*	count_users
*
*	counts the number of people 'on' an object
***************************************************************************/
int count_users(OBJ_DATA *obj)
{
	CHAR_DATA *fch;
	int count = 0;

	if (obj == NULL || obj->in_room == NULL)
		return 0;

	for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
		if (fch->on == obj)
			count++;

	return count;
}



/***************************************************************************
*	check_immune
*
*	for immunity, vulnerabiltiy, and resistant
*	the 'globals'(magic and weapons) may be overriden
*	three other cases -- wood, silver, and iron --
*	are checked in fight.c
***************************************************************************/
int check_immune(CHAR_DATA *ch, int dam_type)
{
	int immune;
	int def;
	long bit;

	immune = -1;
	def = IS_NORMAL;

	if (dam_type == DAM_NONE)
		return immune;

	if (dam_type <= 3) {
		if (IS_SET(ch->imm_flags, IMM_WEAPON))
			def = IS_IMMUNE;
		else if (IS_SET(ch->res_flags, RES_WEAPON))
			def = IS_RESISTANT;
		else if (IS_SET(ch->vuln_flags, VULN_WEAPON))
			def = IS_VULNERABLE;
	} else {
		if (IS_SET(ch->imm_flags, IMM_MAGIC))
			def = IS_IMMUNE;
		else if (IS_SET(ch->res_flags, RES_MAGIC))
			def = IS_RESISTANT;
		else if (IS_SET(ch->vuln_flags, VULN_MAGIC))
			def = IS_VULNERABLE;
	}

	switch (dam_type) {
	case (DAM_BASH):
		bit = IMM_BASH;
		break;
	case (DAM_PIERCE):
		bit = IMM_PIERCE;
		break;
	case (DAM_SLASH):
		bit = IMM_SLASH;
		break;
	case (DAM_FIRE):
		bit = IMM_FIRE;
		break;
	case (DAM_COLD):
		bit = IMM_COLD;
		break;
	case (DAM_LIGHTNING):
		bit = IMM_LIGHTNING;
		break;
	case (DAM_ACID):
		bit = IMM_ACID;
		break;
	case (DAM_POISON):
		bit = IMM_POISON;
		break;
	case (DAM_NEGATIVE):
		bit = IMM_NEGATIVE;
		break;
	case (DAM_HOLY):
		bit = IMM_HOLY;
		break;
	case (DAM_ENERGY):
		bit = IMM_ENERGY;
		break;
	case (DAM_MENTAL):
		bit = IMM_MENTAL;
		break;
	case (DAM_DISEASE):
		bit = IMM_DISEASE;
		break;
	case (DAM_DROWNING):
		bit = IMM_DROWNING;
		break;
	case (DAM_LIGHT):
		bit = IMM_LIGHT;
		break;
	case (DAM_CHARM):
		bit = IMM_CHARM;
		break;
	case (DAM_SOUND):
		bit = IMM_SOUND;
		break;
	case (DAM_ILLUSION):
		bit = IMM_ILLUSION;
		break;
	case (DAM_WOOD):
		bit = IMM_WOOD;
		break;
	default:
		return def;
	}

	if (IS_SET(ch->imm_flags, bit)) {
		immune = IS_IMMUNE;
	} else if (IS_SET(ch->res_flags, bit) && immune != IS_IMMUNE) {
		immune = IS_RESISTANT;
	} else if (IS_SET(ch->vuln_flags, bit)) {
		if (immune == IS_IMMUNE)
			immune = IS_RESISTANT;
		else if (immune == IS_RESISTANT)
			immune = IS_NORMAL;
		else
			immune = IS_VULNERABLE;
	}

	if (immune == -1)
		return def;
	else
		return immune;
}


/***************************************************************************
*	find_location
***************************************************************************/
ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	if (is_number(arg))
		return get_room_index(parse_long(arg));

	if ((victim = get_char_world(ch, arg)) != NULL)
		return victim->in_room;

	if ((obj = get_obj_world(ch, arg)) != NULL)
		return obj->in_room;

	return NULL;
}



/***************************************************************************
* get_weapon_sn
***************************************************************************/
int get_weapon_sn(CHAR_DATA *ch, OBJ_DATA *wield)
{
	SKILL *skill;

	// If no wield passed in, assume we want to use the character's primary weapon.
	if (wield == NULL) {
		wield = get_eq_char(ch, WEAR_WIELD);
    }

    skill = NULL;
	if (wield != NULL && wield->item_type == ITEM_WEAPON) {
		switch (wield->value[0]) {
		default:
			skill = NULL;
			break;
		case (WEAPON_SWORD):
			skill = gsp_sword;
			break;
		case (WEAPON_DAGGER):
			skill = gsp_dagger;
			break;
		case (WEAPON_SPEAR):
			skill = gsp_spear;
			break;
		case (WEAPON_MACE):
			skill = gsp_mace;
			break;
		case (WEAPON_AXE):
			skill = gsp_axe;
			break;
		case (WEAPON_FLAIL):
			skill = gsp_flail;
			break;
		case (WEAPON_WHIP):
			skill = gsp_whip;
			break;
		case (WEAPON_POLEARM):
			skill = gsp_polearm;
			break;
		}
	}

	return (skill != NULL) ? skill->sn : -1;
}


/***************************************************************************
*	get_weapon_skill
***************************************************************************/
int get_weapon_skill(CHAR_DATA *ch, int sn)
{
	int skill;

	if (IS_NPC(ch)) {
		if (sn == -1)
			skill = 3 * ch->level;
		else if (gsp_hand_to_hand != NULL && sn == gsp_hand_to_hand->sn)
			skill = 40 + 2 * ch->level;
		else
			skill = 40 + 5 * ch->level / 2;
	} else {
		if (sn == -1)
			skill = 3 * ch->level;
		else
			skill = get_learned_percent(ch, resolve_skill_sn(sn));
			/*skill = ch->pcdata->learned[sn];*/
	}

	return URANGE(0, skill, 100);
}


/***************************************************************************
*	reset_char
*
*	used to reset default on a character that are bestowed
*	by race abilities or equipment.
***************************************************************************/
void reset_char(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	AFFECT_DATA *af;
	int loc;
	long mod;
	int stat;
	int i;

	if (IS_NPC(ch))
		return;

	if (ch->pcdata->perm_hit == 0
	    || ch->pcdata->perm_mana == 0
	    || ch->pcdata->perm_move == 0
	    || ch->pcdata->last_level == 0) {
		/* do a FULL reset */
		for (loc = 0; loc < MAX_WEAR; loc++) {
			obj = get_eq_char(ch, loc);
			if (obj == NULL)
				continue;

			if (!obj->enchanted) {
				for (af = obj->obj_idx->affected; af != NULL; af = af->next) {
					mod = af->modifier;
					switch (af->location) {
					case APPLY_SEX:
						ch->sex -= mod;
						if (ch->sex < 0 || ch->sex > 2)
							ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
						break;
					case APPLY_MANA:
						ch->max_mana -= mod;
						break;
					case APPLY_HIT:
						ch->max_hit -= mod;
						break;
					case APPLY_MOVE:
						ch->max_move -= mod;
						break;
					}
				}
			}

			for (af = obj->affected; af != NULL; af = af->next) {
				mod = af->modifier;
				switch (af->location) {
				case APPLY_SEX:
					ch->sex -= mod;
					break;
				case APPLY_MANA:
					ch->max_mana -= mod;
					break;
				case APPLY_HIT:
					ch->max_hit -= mod;
					break;
				case APPLY_MOVE:
					ch->max_move -= mod;
					break;
				}
			}
		}
		/* now reset the permanent stats */
		ch->pcdata->perm_hit = ch->max_hit;
		ch->pcdata->perm_mana = ch->max_mana;
		ch->pcdata->perm_move = ch->max_move;
		ch->pcdata->last_level = ch->played / 3600;
	}

	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2) {
		if (ch->sex > 0 && ch->sex < 3)
			ch->pcdata->true_sex = ch->sex;
		else
			ch->pcdata->true_sex = 0;
	}


/* now restore the character to his/her true condition */
	for (stat = 0; stat < MAX_STATS; stat++)
		ch->mod_stat[stat] = 0;

	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
		ch->pcdata->true_sex = 0;

	ch->sex = ch->pcdata->true_sex;
	ch->max_hit = ch->pcdata->perm_hit;
	ch->max_mana = ch->pcdata->perm_mana;
	ch->max_move = ch->pcdata->perm_move;

	for (i = 0; i < 4; i++)
		ch->armor[i] = 100;

	ch->hitroll = 0;
	ch->damroll = 0;
	ch->saving_throw = 0;
	/*	because humans are so average
	 *	instead of rooted somewhere in magic,
	 *  their saving throw vs. magical attacks
	 *	is stronger -- -1/5 of the level
	 *
	 * if( ch->race == race_lookup("human"))
	 * {
	 *      ch->saving_throw -= ch->level / 5;
	 * }
	 */
/* now start adding back the effects */
	for (loc = 0; loc < MAX_WEAR; loc++) {
		obj = get_eq_char(ch, loc);
		if (obj == NULL)
			continue;

		for (i = 0; i < 4; i++)
			ch->armor[i] -= apply_ac(obj, loc, i);

		if (!obj->enchanted) {
			for (af = obj->obj_idx->affected; af != NULL; af = af->next) {
				mod = af->modifier;
				switch (af->location) {
				case APPLY_STR:
					ch->mod_stat[STAT_STR] += mod;
					break;
				case APPLY_DEX:
					ch->mod_stat[STAT_DEX] += mod;
					break;
				case APPLY_INT:
					ch->mod_stat[STAT_INT] += mod;
					break;
				case APPLY_WIS:
					ch->mod_stat[STAT_WIS] += mod;
					break;
				case APPLY_CON:
					ch->mod_stat[STAT_CON] += mod;
					break;
				case APPLY_LUCK:
					ch->mod_stat[STAT_LUCK] += mod;
					break;
				case APPLY_SEX:
					ch->sex += mod;
					break;
				case APPLY_MANA:
					ch->max_mana += mod;
					break;
				case APPLY_HIT:
					ch->max_hit += mod;
					break;
				case APPLY_MOVE:
					ch->max_move += mod;
					break;
				case APPLY_AC:
					for (i = 0; i < 4; i++)
						ch->armor[i] += mod;
					break;
				case APPLY_HITROLL:
					ch->hitroll += mod;
					break;
				case APPLY_DAMROLL:
					ch->damroll += mod;
					break;
				case APPLY_SAVES:
					ch->saving_throw += mod;
					break;
				case APPLY_SAVING_ROD:
					ch->saving_throw += mod;
					break;
				case APPLY_SAVING_PETRI:
					ch->saving_throw += mod;
					break;
				case APPLY_SAVING_BREATH:
					ch->saving_throw += mod;
					break;
				case APPLY_SAVING_SPELL:
					ch->saving_throw += mod;
					break;
				}
			}
		}

		for (af = obj->affected; af != NULL; af = af->next) {
			mod = af->modifier;
			switch (af->location) {
			case APPLY_STR:
				ch->mod_stat[STAT_STR] += mod;
				break;
			case APPLY_DEX:
				ch->mod_stat[STAT_DEX] += mod;
				break;
			case APPLY_INT:
				ch->mod_stat[STAT_INT] += mod;
				break;
			case APPLY_WIS:
				ch->mod_stat[STAT_WIS] += mod;
				break;
			case APPLY_CON:
				ch->mod_stat[STAT_CON] += mod;
				break;
			case APPLY_LUCK:
				ch->mod_stat[STAT_LUCK] += mod;
				break;
			case APPLY_SEX:
				ch->sex += mod;
				break;
			case APPLY_MANA:
				ch->max_mana += mod;
				break;
			case APPLY_HIT:
				ch->max_hit += mod;
				break;
			case APPLY_MOVE:
				ch->max_move += mod;
				break;
			case APPLY_AC:
				for (i = 0; i < 4; i++)
					ch->armor[i] += mod;
				break;
			case APPLY_HITROLL:
				ch->hitroll += mod;
				break;
			case APPLY_DAMROLL:
				ch->damroll += mod;
				break;
			case APPLY_SAVES:
				ch->saving_throw += mod;
				break;
			case APPLY_SAVING_ROD:
				ch->saving_throw += mod;
				break;
			case APPLY_SAVING_PETRI:
				ch->saving_throw += mod;
				break;
			case APPLY_SAVING_BREATH:
				ch->saving_throw += mod;
				break;
			case APPLY_SAVING_SPELL:
				ch->saving_throw += mod;
				break;
			}
		}
	}

/* now add back spell effects */
	for (af = ch->affected; af != NULL; af = af->next) {
		mod = af->modifier;
		switch (af->location) {
		case APPLY_STR:
			ch->mod_stat[STAT_STR] += mod;
			break;
		case APPLY_DEX:
			ch->mod_stat[STAT_DEX] += mod;
			break;
		case APPLY_INT:
			ch->mod_stat[STAT_INT] += mod;
			break;
		case APPLY_WIS:
			ch->mod_stat[STAT_WIS] += mod;
			break;
		case APPLY_CON:
			ch->mod_stat[STAT_CON] += mod;
			break;
		case APPLY_LUCK:
			ch->mod_stat[STAT_LUCK] += mod;
			break;
		case APPLY_SEX:
			ch->sex += mod;
			break;
		case APPLY_MANA:
			ch->max_mana += mod;
			break;
		case APPLY_HIT:
			ch->max_hit += mod;
			break;
		case APPLY_MOVE:
			ch->max_move += mod;
			break;
		case APPLY_AC:
			for (i = 0; i < 4; i++)
				ch->armor[i] += mod;
			break;
		case APPLY_HITROLL:
			ch->hitroll += mod;
			break;
		case APPLY_DAMROLL:
			ch->damroll += mod;
			break;
		case APPLY_SAVES:
			ch->saving_throw += mod;
			break;
		case APPLY_SAVING_ROD:
			ch->saving_throw += mod;
			break;
		case APPLY_SAVING_PETRI:
			ch->saving_throw += mod;
			break;
		case APPLY_SAVING_BREATH:
			ch->saving_throw += mod;
			break;
		case APPLY_SAVING_SPELL:
			ch->saving_throw += mod;
			break;
		}
	}

	/* reset werebest blood rage */
	blood_rage(ch);

/* make sure sex is RIGHT!!!! */
	if (ch->sex < 0 || ch->sex > 2)
		ch->sex = ch->pcdata->true_sex;
}


/***************************************************************************
*	get_trust
*
*	gets the trust level of a user
*		if null is passed it is lvl 311
*		if an NPC is passed it is 0
*		if ch->trust > 0 then return ch->trust
*		otherwise return ch->level
***************************************************************************/
int get_trust(CHAR_DATA *ch)
{
	if (!ch)
		return MAX_LEVEL + 1;

	if (ch->desc != NULL && ch->desc->original != NULL)
		ch = ch->desc->original;

	if (IS_NPC(ch))
		return (int)0;

	return (ch->trust != 0) ? ch->trust : ch->level;
}


/***************************************************************************
*	get_age
*
*	gets the age, in mud years, for a player
***************************************************************************/
int get_age(CHAR_DATA *ch)
{
	return 17 + (ch->played + (int)(globalSystemState.current_time - ch->logon)) / 72000;
}


/***************************************************************************
*	get_current_stat
*
*	gets a the specified current stat by combining
*	perm_stat with mod_stat and putting that number
*	in the number range 3 - <max stat value>
***************************************************************************/
int get_curr_stat(CHAR_DATA *ch, int stat)
{
	int val;
	int max;

	if (IS_NPC(ch) || ch->race < 0 || ch->race > MAX_PC_RACE) {
		val = ch->perm_stat[stat] + ch->mod_stat[stat];
	} else if (ch->level > LEVEL_IMMORTAL) {
		val = 1000;
	} else {
		val = ch->perm_stat[stat] + ch->mod_stat[stat];
		if (pc_race_table[ch->race].max_stats[stat] > 0) {
			max = pc_race_table[ch->race].max_stats[stat];
			val = UMIN(val, max);
		}
	}

	return UMAX(3, val);
}


/***************************************************************************
*	get_max_train
*
*	gets the highest value a stat can be trained for a
*	given character
***************************************************************************/
int get_max_train(CHAR_DATA *ch, int stat)
{
	int max;

	if (IS_NPC(ch) || ch->race < 0 || ch->race > MAX_PC_RACE) {
		max = 25;
	} else {
		max = pc_race_table[ch->race].max_train_stats[stat];
		if (class_table[ch->class].attr_prime == stat) {
			if (ch->race == race_lookup("human"))
				max += 3;
			else
				max += 2;
		}
	}

	return UMAX(3, max);
}


/***************************************************************************
*	get_carry_n
*
*	get the number of items a character can carry
***************************************************************************/
int can_carry_n(CHAR_DATA *ch)
{
	if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
		return 1000;

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
		return 0;

	return MAX_WEAR + 2 * get_curr_stat(ch, STAT_DEX) + ch->level;
}



/***************************************************************************
*	get_carry_w
*
*	get the maximum amount of weight a character can carry
***************************************************************************/
int can_carry_w(CHAR_DATA *ch)
{
	if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
		return 10000000;

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
		return 0;

	return ((get_curr_stat(ch, STAT_STR) - 2) * 200) + (ch->level * 25);
}

/***************************************************************************
*	get_wield_weight
*
*	get the maximum wieght of a wielded item
***************************************************************************/
int get_wield_weight(CHAR_DATA *ch)
{
	return UMIN((get_curr_stat(ch, STAT_STR) * 5 / 2), 75) * 10;
}



/***************************************************************************
*	is_name
*
*	is a given name in a name list
***************************************************************************/
bool is_name(char *str, char *namelist)
{
	char name[MIL], part[MIL];
	char *list, *string;


	string = str;
/* we need ALL parts of string to match part of namelist */
	for (;; ) { /* start parsing string */
		str = one_argument(str, part);

		if (part[0] == '\0')
			return TRUE;

		/* check to see if this is part of namelist */
		list = namelist;
		for (;; ) { /* start parsing namelist */
			list = one_argument(list, name);
			if (name[0] == '\0') /* this name was not found */
				return FALSE;

			if (!str_prefix(string, name))
				return TRUE; /* full pattern match */

			if (!str_prefix(part, name))
				break;
		}
	}
}





/*
 * Move a char out of a room.
 */
void char_from_room(CHAR_DATA *ch)
{
	OBJ_DATA *obj;

	if (ch->in_room == NULL) {
		bug("Char_from_room: NULL.", 0);
		return;
	}

	if (!IS_NPC(ch))
		--ch->in_room->area->nplayer;

	if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
	    && obj->item_type == ITEM_LIGHT
	    && obj->value[2] != 0
	    && ch->in_room->light > 0)
		--ch->in_room->light;

	if (ch == ch->in_room->people) {
		ch->in_room->people = ch->next_in_room;
	} else {
		CHAR_DATA *prev;

		for (prev = ch->in_room->people; prev; prev = prev->next_in_room) {
			if (prev->next_in_room == ch) {
				prev->next_in_room = ch->next_in_room;
				break;
			}
		}

		if (prev == NULL)
			bug("Char_from_room: ch not found.", 0);
	}

	ch->in_room = NULL;
	ch->next_in_room = NULL;
	ch->on = NULL;  /* sanity check! */
	return;
}



void char_to_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex)
{
	OBJ_DATA *obj;

	if (pRoomIndex == NULL) {
		ROOM_INDEX_DATA *room;

		bug("Char_to_room: NULL.", 0);

		if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
			char_to_room(ch, room);

		return;
	}

	ch->in_room = pRoomIndex;
	ch->next_in_room = pRoomIndex->people;
	pRoomIndex->people = ch;

	if (!IS_NPC(ch)) {
		if (ch->in_room->area->empty) {
			ch->in_room->area->empty = FALSE;
			ch->in_room->area->age = 0;
		}
		++ch->in_room->area->nplayer;
	}

	if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
	    && obj->item_type == ITEM_LIGHT
	    && obj->value[2] != 0)
		++ch->in_room->light;

	if (IS_AFFECTED(ch, AFF_PLAGUE)) {
		AFFECT_DATA *paf;
		AFFECT_DATA af;
		CHAR_DATA *vch;

		paf = affect_find(ch->affected, gsp_plague);

		if (paf == NULL) {
			REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
			return;
		}

		if (paf->level == 1)
			return;

		if (gsp_plague != NULL) {
			af.where = TO_AFFECTS;
			af.type = gsp_plague->sn;
			af.skill = gsp_plague;
			af.level = (int)(paf->level - 1);
			af.duration = (int)number_range(1, 2 * af.level);
			af.location = APPLY_STR;
			af.modifier = -5;
			af.bitvector = AFF_PLAGUE;

			for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
				if (!saves_spell(af.level - 2, vch, DAM_DISEASE)
				    && !IS_IMMORTAL(vch)
				    && !IS_AFFECTED(vch, AFF_PLAGUE)
				    && number_bits(6) == 0) {
					send_to_char("You feel hot and feverish.\n\r", vch);
					act("$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM);
					affect_join(vch, &af);
				}
			}
		}
	}

	return;
}



/*
 * Give an obj to a char.
 */
void obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch)
{
	obj->next_content = ch->carrying;
	ch->carrying = obj;
	obj->carried_by = ch;
	obj->in_room = NULL;
	obj->in_obj = NULL;
	ch->carry_number += get_obj_number(obj);
	ch->carry_weight += get_obj_weight(obj);
}


/*
 * Take an obj from its character.
 */
void obj_from_char(OBJ_DATA *obj)
{
	CHAR_DATA *ch;

	if ((ch = obj->carried_by) == NULL) {
		bug("Obj_from_char: null ch.", 0);
		return;
	}

	if (obj->wear_loc != WEAR_NONE)
		unequip_char(ch, obj);

	if (ch->carrying == obj) {
		ch->carrying = obj->next_content;
	} else {
		OBJ_DATA *prev;

		for (prev = ch->carrying; prev != NULL; prev = prev->next_content) {
			if (prev->next_content == obj) {
				prev->next_content = obj->next_content;
				break;
			}
		}

		if (prev == NULL)
			bug("Obj_from_char: obj not in list.", 0);
	}

	obj->carried_by = NULL;
	obj->next_content = NULL;
	ch->carry_number -= get_obj_number(obj);
	ch->carry_weight -= get_obj_weight(obj);
	return;
}



/*
 * Find the ac value of an obj, including position effect.
 */
long apply_ac(OBJ_DATA *obj, int iWear, int type)
{
	if (obj->item_type != ITEM_ARMOR)
		return 0;

	switch (iWear) {
	case WEAR_BODY:
		return 3 * obj->value[type];
	case WEAR_HEAD:
		return 2 * obj->value[type];
	case WEAR_FACE:
		return obj->value[type];
	case WEAR_LEGS:
		return 2 * obj->value[type];
	case WEAR_FEET:
		return obj->value[type];
	case WEAR_HANDS:
		return obj->value[type];
	case WEAR_ARMS:
		return obj->value[type];
	case WEAR_SHIELD:
		return obj->value[type];
	case WEAR_EAR_L:
		return obj->value[type];
	case WEAR_EAR_R:
		return obj->value[type];
	case WEAR_FINGER_L:
		return 0;
	case WEAR_FINGER_R:
		return obj->value[type];
	case WEAR_NECK_1:
		return obj->value[type];
	case WEAR_NECK_2:
		return obj->value[type];
	case WEAR_ABOUT:
		return 2 * obj->value[type];
	case WEAR_WAIST:
		return obj->value[type];
	case WEAR_WRIST_L:
		return obj->value[type];
	case WEAR_WRIST_R:
		return obj->value[type];
	case WEAR_HOLD:
		return obj->value[type];
	case WEAR_TATTOO:
		return obj->value[type];
	}

	return 0;
}



/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char(CHAR_DATA *ch, int iWear)
{
	OBJ_DATA *obj;

	if (ch == NULL)
		return NULL;

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
		if (obj->wear_loc == iWear)
			return obj;

	return NULL;
}



/*
 * Equip a char with an obj.
 */
void equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int iWear)
{
	AFFECT_DATA *paf;
	int i;

	if (get_eq_char(ch, iWear) != NULL) {
		printf_bug("equip_char: already equipped.\narea: %s\ncharacter: %s\nroom: %d\nobj: %d",
			   (ch->in_room != NULL && ch->in_room->area != NULL) ? ch->in_room->area->file_name : 0,
			   ch->name,
			   (ch->in_room != NULL) ? ch->in_room->vnum : 0,
			   obj->obj_idx->vnum);

		return;
	}

	if (ch->class != class_lookup("witch") && obj->item_type == ITEM_DOLL) {
		act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
		act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		save_char_obj(ch);
		return;
	}

	if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
	    || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
	    || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
		/*
		 * Thanks to Morgenes for the bug fix here!
		 */
		act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
		act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		save_char_obj(ch);
		return;
	}

	for (i = 0; i < 4; i++)
		ch->armor[i] -= apply_ac(obj, iWear, i);
	obj->wear_loc = iWear;

	if (!obj->enchanted)
		for (paf = obj->obj_idx->affected; paf != NULL; paf = paf->next)
			if (paf->location != APPLY_SPELL_AFFECT)
				affect_modify(ch, paf, TRUE);
	for (paf = obj->affected; paf != NULL; paf = paf->next)
		if (paf->location == APPLY_SPELL_AFFECT)
			affect_to_char(ch, paf);
		else
			affect_modify(ch, paf, TRUE);

	if (obj->item_type == ITEM_LIGHT
	    && obj->value[2] != 0
	    && ch->in_room != NULL)
		++ch->in_room->light;

	return;
}



/*
 * Unequip a char with an obj.
 */
void unequip_char(CHAR_DATA *ch, OBJ_DATA *obj)
{
	AFFECT_DATA *paf = NULL;
	AFFECT_DATA *lpaf = NULL;
	AFFECT_DATA *lpaf_next = NULL;
	int i;

	if (obj == NULL) {
		bug("Unequip_char: no object.", 0);
		return;
	}

	if (obj->wear_loc == WEAR_NONE) {
		bug("Unequip_char: already unequipped.", 0);
		return;
	}

	for (i = 0; i < 4; i++)
		ch->armor[i] += apply_ac(obj, obj->wear_loc, i);
	obj->wear_loc = -1;

	if (!obj->enchanted) {
		for (paf = obj->obj_idx->affected; paf != NULL; paf = paf->next) {
			if (paf->location == APPLY_SPELL_AFFECT) {
				for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next) {
					lpaf_next = lpaf->next;
					if ((lpaf->type == paf->type) &&
					    (lpaf->level == paf->level) &&
					    (lpaf->location == APPLY_SPELL_AFFECT)) {
						affect_remove(ch, lpaf);
						lpaf_next = NULL;
					}
				}
			} else {
				affect_modify(ch, paf, FALSE);
				affect_check(ch, paf->where, paf->bitvector);
			}
		}
	}
	for (paf = obj->affected; paf != NULL; paf = paf->next)
		if (paf->location == APPLY_SPELL_AFFECT) {
			bug("Norm-Apply: %d", 0);
			for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next) {
				lpaf_next = lpaf->next;
				if ((lpaf->type == paf->type) &&
				    (lpaf->level == paf->level) &&
				    (lpaf->location == APPLY_SPELL_AFFECT)) {
					bug("location = %d", lpaf->location);
					bug("type = %d", lpaf->type);
					affect_remove(ch, lpaf);
					lpaf_next = NULL;
				}
			}
		} else {
			affect_modify(ch, paf, FALSE);
			affect_check(ch, paf->where, paf->bitvector);
		}

	if (obj->item_type == ITEM_LIGHT
	    && obj->value[2] != 0
	    && ch->in_room != NULL
	    && ch->in_room->light > 0)
		--ch->in_room->light;

	return;
}



/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list(OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list)
{
	OBJ_DATA *obj;
	int nMatch;

	nMatch = 0;
	for (obj = list; obj != NULL; obj = obj->next_content)
		if (obj->obj_idx == pObjIndex)
			nMatch++;

	return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room(OBJ_DATA *obj)
{
	ROOM_INDEX_DATA *in_room;
	CHAR_DATA *ch;

	if ((in_room = obj->in_room) == NULL) {
		bug("obj_from_room: NULL.", 0);
		return;
	}

	for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
		if (ch->on == obj)
			ch->on = NULL;

	if (obj == in_room->contents) {
		in_room->contents = obj->next_content;
	} else {
		OBJ_DATA *prev;

		for (prev = in_room->contents; prev; prev = prev->next_content) {
			if (prev->next_content == obj) {
				prev->next_content = obj->next_content;
				break;
			}
		}

		if (prev == NULL) {
			bug("obj_from_room: obj not found.", 0);
			return;
		}
	}

	obj->in_room = NULL;
	obj->next_content = NULL;
	return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex)
{
/*    int max_obj;
 *
 * max_obj = 200;
 * if(max_obj >= obj->in_room)
 * {
 * act( "$p bounces around all the other objects and falls to pieces!",
 * NULL, obj, NULL, TO_ROOM );
 * return;
 * }
 */
	if (pRoomIndex == NULL)
		return;
	obj->next_content = pRoomIndex->contents;
	pRoomIndex->contents = obj;
	obj->in_room = pRoomIndex;
	obj->carried_by = NULL;
	obj->in_obj = NULL;
	return;
}



/*
 * Move an object into an object.
 */
void obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to)
{
	obj->next_content = obj_to->contains;
	obj_to->contains = obj;
	obj->in_obj = obj_to;
	obj->in_room = NULL;
	obj->carried_by = NULL;

	if (obj_to->obj_idx->vnum == OBJ_VNUM_PIT)
		obj->cost = 0;

	for (; obj_to != NULL; obj_to = obj_to->in_obj) {
		if (obj_to == obj_to->in_obj)
			break;

		if (obj_to->carried_by != NULL) {
			obj_to->carried_by->carry_number += get_obj_number(obj);
			obj_to->carried_by->carry_weight += get_obj_weight(obj) * WEIGHT_MULT(obj_to) / 100;
		}
	}

	return;
}



/*
 * Move an object out of an object.
 */
void obj_from_obj(OBJ_DATA *obj)
{
	OBJ_DATA *obj_from;

	if ((obj_from = obj->in_obj) == NULL) {
		bug("Obj_from_obj: null obj_from.", 0);
		return;
	}

	if (obj == obj_from->contains) {
		obj_from->contains = obj->next_content;
	} else {
		OBJ_DATA *prev;

		for (prev = obj_from->contains; prev; prev = prev->next_content) {
			if (prev->next_content == obj) {
				prev->next_content = obj->next_content;
				break;
			}
		}

		if (prev == NULL) {
			bug("Obj_from_obj: obj not found.", 0);
			return;
		}
	}

	obj->next_content = NULL;
	obj->in_obj = NULL;

	for (; obj_from != NULL; obj_from = obj_from->in_obj) {
		if (obj_from->carried_by != NULL) {
			obj_from->carried_by->carry_number -= get_obj_number(obj);
			obj_from->carried_by->carry_weight -= get_obj_weight(obj)
							      * WEIGHT_MULT(obj_from) / 100;
		}
	}

	return;
}



/*
 * Extract an obj from the world.
 */
void extract_obj(OBJ_DATA *obj)
{
	OBJ_DATA *obj_content;
	OBJ_DATA *obj_next;

	if (obj->in_room != NULL)
		obj_from_room(obj);
	else if (obj->carried_by != NULL)
		obj_from_char(obj);
	else if (obj->in_obj != NULL)
		obj_from_obj(obj);

	for (obj_content = obj->contains; obj_content; obj_content = obj_next) {
		obj_next = obj_content->next_content;
		extract_obj(obj_content);
	}

	if (globalSystemState.object_head == obj) {
		globalSystemState.object_head = obj->next;
	} else {
		OBJ_DATA *prev;

		for (prev = globalSystemState.object_head; prev != NULL; prev = prev->next) {
			if (prev->next == obj) {
				prev->next = obj->next;
				break;
			}
		}

		if (prev == NULL) {
			bug_long("Extract_obj: obj %d not found.", obj->obj_idx->vnum);
			return;
		}
	}

	--obj->obj_idx->count;
	free_obj(obj);
	return;
}





/*
 * Extract a char from the world.
 */
void extract_char(CHAR_DATA *ch, bool extract)
{
	CHAR_DATA *wch;

	if (ch->in_room == NULL) {
		bug("extract_char: NULL room", 0);
		return;
	}

	nuke_pets(ch);
	ch->pet = NULL; /* just in case */

	if (extract)
		die_follower(ch);

	stop_fighting(ch, TRUE);
	char_from_room(ch);

	if (!extract && !IS_NPC(ch)) {
		char_to_room(ch, get_death_room(ch));
		return;
	}

	if (IS_NPC(ch))
		--ch->mob_idx->count;

	if (ch->desc != NULL
	    && ch->desc->original != NULL) {
		do_return(ch, "");
		ch->desc = NULL;
	}

	for (wch = char_list; wch != NULL; wch = wch->next) {
		if (wch->reply == ch)
			wch->reply = NULL;

		if (ch->mprog_target == wch)
			wch->mprog_target = NULL;
	}

	if (ch == char_list) {
		char_list = ch->next;
	} else {
		CHAR_DATA *prev;

		for (prev = char_list; prev != NULL; prev = prev->next) {
			if (prev->next == ch) {
				prev->next = ch->next;
				break;
			}
		}

		if (prev == NULL) {
			bug("extract_char: char not found.", 0);
			return;
		}
	}

	if (ch->desc != NULL)
		ch->desc->character = NULL;

	free_char(ch);
	return;
}



/***************************************************************************
*	get_char_room
*
*	get a character in the room
***************************************************************************/
CHAR_DATA *get_char_room(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;
	char arg[MIL];
	char *temp;
	int number;
	int count;

	temp = check_nickname(ch, argument);

	if (temp)
		argument = temp;

	number = number_argument(argument, arg);
	count = 0;
	if (!str_cmp(arg, "self"))
		return ch;

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (!can_see(ch, rch) || !is_name(arg, rch->name))
			continue;

		if (++count == number)
			return rch;
	}

	return NULL;
}



/***************************************************************************
*	get_char_world
*
*	find a character somewhere in the world
***************************************************************************/
CHAR_DATA *get_char_world(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *wch;
	char arg[MIL];
	char *temp;
	int number;
	int count;

	if ((wch = get_char_room(ch, argument)) != NULL)
		return wch;

	temp = check_nickname(ch, argument);
	if (temp)
		argument = temp;

	number = number_argument(argument, arg);
	count = 0;
	for (wch = char_list; wch != NULL; wch = wch->next) {
		if (wch->in_room == NULL || !can_see(ch, wch)
		    || !is_name(arg, wch->name))
			continue;

		if (++count == number)
			return wch;
	}

	return NULL;
}



/***************************************************************************
*	get_obj_type
*
*	find an object with a given index data
***************************************************************************/
OBJ_DATA *get_obj_type(OBJ_INDEX_DATA *obj_idx)
{
	OBJ_DATA *obj;

	for (obj = globalSystemState.object_head; obj != NULL; obj = obj->next)
		if (obj->obj_idx == obj_idx)
			return obj;

	return NULL;
}

/***************************************************************************
*	get_obj_list
*
*	find an object in an arbitrary object list
***************************************************************************/
OBJ_DATA *get_obj_list(CHAR_DATA *ch, char *argument, OBJ_DATA *list)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = list; obj != NULL; obj = obj->next_content) {
		if (can_see_obj(ch, obj) && is_name(arg, obj->name)) {
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}



/***************************************************************************
*	get_obj_carry
*
*	find an object in the characters inventory
***************************************************************************/
OBJ_DATA *get_obj_carry(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if (obj->wear_loc == WEAR_NONE
		    && (can_see_obj(ch, obj))
		    && is_name(arg, obj->name)) {
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}


/***************************************************************************
*	get_obj_wear
*
*	find an object worn by the character
***************************************************************************/
OBJ_DATA *get_obj_wear(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if (obj->wear_loc != WEAR_NONE
		    && can_see_obj(ch, obj)
		    && is_name(arg, obj->name)) {
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}



/***************************************************************************
*	get_obj_here
*
*	find an object in the room or in inventory or worn
***************************************************************************/
OBJ_DATA *get_obj_here(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;

	obj = get_obj_list(ch, argument, ch->in_room->contents);
	if (obj != NULL)
		return obj;

	if ((obj = get_obj_carry(ch, argument)) != NULL)
		return obj;

	if ((obj = get_obj_wear(ch, argument)) != NULL)
		return obj;

	return NULL;
}

/***************************************************************************
*	get_obj_world
*
*	find an object somewhere in the world
***************************************************************************/
OBJ_DATA *get_obj_world(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int number;
	int count;

	if ((obj = get_obj_here(ch, argument)) != NULL)
		return obj;

	number = number_argument(argument, arg);
	count = 0;
	for (obj = globalSystemState.object_head; obj != NULL; obj = obj->next) {
		if (can_see_obj(ch, obj) && is_name(arg, obj->name)) {
			if (++count == number)
				return obj;
		}
	}

	return NULL;
}


/***************************************************************************
*	deduct_cost
*
*	deduct the cost of something from a character
*	uses silver then gold
***************************************************************************/
void deduct_cost(CHAR_DATA *ch, unsigned int cost)
{
	unsigned int silver = 0, gold = 0;

	silver = UMIN(ch->silver, cost);

	if (silver < cost) {
		gold = (unsigned int)(((cost - silver + 99) / 100));
		silver = (unsigned int)(cost - 100 * gold);
	}

	if (ch->gold < gold) {
		bug_long("deduct costs: player gold less than amount to be deducted.  %ld", 0);
		gold = ch->gold;
	}
	if (ch->silver < silver) {
		bug_long("deduct costs: silver less than amount to be deducted. %ld", 0);
		silver = ch->silver;
	}


	ch->gold -= gold;
	ch->silver -= silver;
}



/***************************************************************************
*	create_money
*
*	create money
***************************************************************************/
OBJ_DATA *create_money(unsigned int gold, unsigned int silver)
{
	char buf[MSL];
	OBJ_DATA *obj;

	if ((gold == 0 && silver == 0)) {
		printf_bug("create_money: zerod money: %u - %u", gold, silver);
		gold = UMAX(1u, gold);
		silver = UMAX(1u, silver);
	}

	/* create the object - assign short desc where applicable */
	if (gold == 0 && silver == 1) {
		obj = create_object(get_obj_index(OBJ_VNUM_SILVER_ONE), 0);
	} else if (gold == 1 && silver == 0) {
		obj = create_object(get_obj_index(OBJ_VNUM_GOLD_ONE), 0);
	} else if (silver == 0) {
		obj = create_object(get_obj_index(OBJ_VNUM_GOLD_SOME), 0);

		sprintf(buf, obj->short_descr, gold);
		free_string(obj->short_descr);

		obj->short_descr = str_dup(buf);
		obj->value[1] = (long)gold;
		obj->cost = gold;
	} else if (gold == 0) {
		obj = create_object(get_obj_index(OBJ_VNUM_SILVER_SOME), 0);

		sprintf(buf, obj->short_descr, silver);
		free_string(obj->short_descr);

		obj->short_descr = str_dup(buf);
		obj->value[0] = (long)silver;
		obj->cost = silver;
	} else {
		obj = create_object(get_obj_index(OBJ_VNUM_COINS), 0);

		sprintf(buf, obj->short_descr, silver, gold);
		free_string(obj->short_descr);

		obj->short_descr = str_dup(buf);
		obj->value[0] = (long)silver;
		obj->value[1] = (long)gold;
		obj->cost = (unsigned int)(100 * gold + silver);
	}

	return obj;
}



/***************************************************************************
*	get_obj_number
*
*	get the number of objects which an object counts as
*	certain object types count as 0 objects, containers
*	may contain several objects
***************************************************************************/
int get_obj_number(OBJ_DATA *obj)
{
	OBJ_DATA *obj_idx;
	int number;

	if (obj->item_type == ITEM_CONTAINER
	    || obj->item_type == ITEM_MONEY
	    || obj->item_type == ITEM_GEM
	    || obj->item_type == ITEM_JEWELRY
	    || obj->item_type == ITEM_POTION
	    || obj->item_type == ITEM_SCROLL
	    || obj->item_type == ITEM_PILL)
		number = 0;
	else
		number = 1;

	for (obj_idx = obj->contains;
	     obj_idx != NULL;
	     obj_idx = obj_idx->next_content)
		number += get_obj_number(obj_idx);

	return number;
}


/***************************************************************************
*	get_object_weight
*
*	get an objects weight figuring in weight multiplers
***************************************************************************/
int get_obj_weight(OBJ_DATA *obj)
{
	OBJ_DATA *obj_idx;
	int weight;

	weight = obj->weight;
	for (obj_idx = obj->contains;
	     obj_idx != NULL;
	     obj_idx = obj_idx->next_content)
		weight += get_obj_weight(obj_idx) * WEIGHT_MULT(obj) / 100;

	return weight;
}

/***************************************************************************
*	get_true_weight
*
*	get an objects true weight - handles objects inside of
*	other objects
***************************************************************************/
int get_true_weight(OBJ_DATA *obj)
{
	OBJ_DATA *obj_idx;
	int weight;

	weight = obj->weight;
	for (obj_idx = obj->contains;
	     obj_idx != NULL;
	     obj_idx = obj_idx->next_content)
		weight += get_obj_weight(obj_idx);

	return weight;
}


/***************************************************************************
*	is_room_dark
*
*	is the room dark?
***************************************************************************/
bool room_is_dark(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
	if (room == NULL)
		return FALSE;

	if (IS_SET(room->room_flags, ROOM_SAFE))
		return FALSE;

	if (ch != NULL) {
		CHAR_DATA *vch;

		if (is_affected(ch, gsp_darkness))
			return FALSE;

		for (vch = room->people; vch != NULL; vch = vch->next_in_room)
			if (is_affected(vch, gsp_darkness))
				return TRUE;

		if (IS_AFFECTED(ch, AFF_INFRARED))
			return FALSE;
	}

	if (room->light > 0)
		return FALSE;

	if (IS_SET(room->room_flags, ROOM_DARK))
		return TRUE;

	if (room->sector_type == SECT_INSIDE
	    || room->sector_type == SECT_CITY)
		return FALSE;

	if (weather_info.sunlight == SUN_SET
	    || weather_info.sunlight == SUN_DARK)
		return TRUE;

	return FALSE;
}


/***************************************************************************
*	is_room_owner
*
*	is the character the owner of a room?
***************************************************************************/
bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
	if (room->owner == NULL
	    || room->owner[0] == '\0')
		return FALSE;

	return is_name(ch->name, room->owner);
}

/***************************************************************************
*	room_is_private
*
*	checks to see if a room is private
***************************************************************************/
bool room_is_private(ROOM_INDEX_DATA *room)
{
	CHAR_DATA *rch;
	int count;

	if (room->owner != NULL
	    && room->owner[0] != '\0')
		return TRUE;

	count = 0;
	for (rch = room->people; rch != NULL; rch = rch->next_in_room)
		count++;

	if (IS_SET(room->room_flags, ROOM_PRIVATE)
	    && count >= 2)
		return TRUE;

	if (IS_SET(room->room_flags, ROOM_SOLITARY)
	    && count >= 1)
		return TRUE;

	if (IS_SET(room->room_flags, ROOM_IMP_ONLY))
		return TRUE;

	return FALSE;
}


/***************************************************************************
*	room_is_bfield
*
*	is the room in the battlefield?
***************************************************************************/
bool room_is_bfield(ROOM_INDEX_DATA *pRoomIndex)
{
	if (IS_SET(pRoomIndex->room_flags, ROOM_BFIELD))
		return TRUE;

	return FALSE;
}

/***************************************************************************
*	can_see_room
*
*	can a character see a room
***************************************************************************/
bool can_see_room(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
	/* imp-only rooms */
	if (IS_SET(room->room_flags, ROOM_IMP_ONLY)
	    && get_trust(ch) < MAX_LEVEL)
		return FALSE;

	/* imm-only rooms */
	if (IS_SET(room->room_flags, ROOM_GODS_ONLY)
	    && !IS_IMMORTAL(ch))
		return FALSE;

	/* hero-only rooms */
	if (IS_SET(room->room_flags, ROOM_HEROES_ONLY)
	    && !IS_IMMORTAL(ch))
		return FALSE;

	/* newbie-only rooms */
	if (IS_SET(room->room_flags, ROOM_NEWBIES_ONLY)
	    && ch->level > 5 && !IS_IMMORTAL(ch))
		return FALSE;


	/* level 150 and up only */
	if (IS_SET(room->room_flags, ROOM_HIGHER_ONLY)
	    && ch->level < 150 && !IS_IMMORTAL(ch))
		return FALSE;

	/* level 300 and up only */
	if (IS_SET(room->room_flags, ROOM_HIGHEST_ONLY)
	    && ch->level < 300 && !IS_IMMORTAL(ch))
		return FALSE;

	return TRUE;
}



/***************************************************************************
*	can_see
*
*	can a character see another character?
***************************************************************************/
bool can_see(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (ch == victim)
		return TRUE;

	/* wizi */
	if (get_trust(ch) < victim->invis_level)
		return FALSE;


	/* incog */
	if (get_trust(ch) < victim->incog_level
	    && ch->in_room != victim->in_room)
		return FALSE;

	if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
	    || (IS_NPC(ch) && IS_IMMORTAL(ch)))
		return TRUE;

	if (IS_AFFECTED(ch, AFF_BLIND))
		return FALSE;

	if (room_is_dark(ch, ch->in_room))
		return FALSE;

	if (IS_AFFECTED(victim, AFF_INVISIBLE)
	    && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
		return FALSE;

	if (IS_AFFECTED(victim, AFF_HIDE)
	    && (!IS_NPC(victim))
	    && victim->fighting == NULL)
		return FALSE;

	return TRUE;
}



/***************************************************************************
*	can_see_obj
*
*	can a character see an object?
***************************************************************************/
bool can_see_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return TRUE;

	if (IS_SET(obj->extra_flags, ITEM_VIS_DEATH))
		return FALSE;

	if (IS_AFFECTED(ch, AFF_BLIND)
	    && obj->item_type != ITEM_POTION)
		return FALSE;

	if (obj->item_type == ITEM_LIGHT
	    && obj->value[2] != 0)
		return TRUE;

	if (IS_SET(obj->extra_flags, ITEM_INVIS)
	    && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
		return FALSE;

	if (IS_OBJ_STAT(obj, ITEM_GLOW))
		return TRUE;

	if (room_is_dark(ch, ch->in_room))
		return FALSE;

	return TRUE;
}



/***************************************************************************
*	can_drop_obj
*
*	can a character drop an object?
***************************************************************************/
bool can_drop_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (!IS_SET(obj->extra_flags, ITEM_NODROP))
		return TRUE;

	if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
		return TRUE;

	return FALSE;
}


/***************************************************************************
*	room_flag_bit
*
*	return the room flags for a room
***************************************************************************/
char *room_flag_bit_name(ROOM_INDEX_DATA *room)
{
	return flag_string(room_flags, room->room_flags);
}


/***************************************************************************
*	item_type_name
*
*	get the ascii name of an item type
***************************************************************************/
char *item_type_name(OBJ_DATA *obj)
{
	return flag_string(type_flags, (long)obj->item_type);
}

/***************************************************************************
*	functions provided for backwards compatibility
***************************************************************************/
/***************************************************************************
*	affect_loc_name
*
*	return an affect location name
***************************************************************************/
char *affect_loc_name(long flags)
{
	return flag_string(apply_flags, flags);
}


/***************************************************************************
*	extra_bit_name
*
*	return an extra bit name
***************************************************************************/
char *extra_bit_name(long flags)
{
	return flag_string(extra_flags, flags);
}

char *extra2_bit_name(long flags)
{
	return flag_string(extra2_flags, flags);
}

/***************************************************************************
*	exit_bit_name
*
*	return an exit bit name
***************************************************************************/
char *exit_bit_name(long flags)
{
	return flag_string(exit_flags, flags);
}


/***************************************************************************
*	affect_bit_name
*
*	return an affect bit name
***************************************************************************/
char *affect_bit_name(long flags)
{
	return flag_string(affect_flags, flags);
}

/***************************************************************************
*	act_bit_name
*
*	return an act bit name - differentiates between mobs and players
***************************************************************************/
char *act_bit_name(long flags)
{
	if (IS_SET(flags, ACT_IS_NPC))
		return flag_string(act_flags, flags);
	else
		return flag_string(plr_flags, flags);
}

/***************************************************************************
*	comm_bit_name
*
*	return a communication bit string
***************************************************************************/
char *comm_bit_name(long flags)
{
	return flag_string(comm_flags, flags);
}

/***************************************************************************
*	imm_bit_name
*
*	return an immunity bit string
***************************************************************************/
char *imm_bit_name(long flags)
{
	return flag_string(imm_flags, flags);
}


/***************************************************************************
*	wear_bit_name
*
*	return a wear bit string
***************************************************************************/
char *wear_bit_name(long flags)
{
	return flag_string(wear_flags, flags);
}

/***************************************************************************
*	form_bit_name
*
*	return a form bit string
***************************************************************************/
char *form_bit_name(long flags)
{
	return flag_string(form_flags, flags);
}


/***************************************************************************
*	part_bit_name
*
*	return a body part bit string
***************************************************************************/
char *part_bit_name(long flags)
{
	return flag_string(part_flags, flags);
}

/***************************************************************************
*	weapon_bit_name
*
*	return a weapon flag string
***************************************************************************/
char *weapon_bit_name(long flags)
{
	return flag_string(weapon_flag_type, flags);
}

/***************************************************************************
*	cont_bit_name
*
*	return a container bit string
***************************************************************************/
char *cont_bit_name(long flags)
{
	return flag_string(container_flags, flags);
}


/***************************************************************************
*	off_bit_name
*
*	return a offense flags bit string
***************************************************************************/
char *off_bit_name(long flags)
{
	return flag_string(off_flags, flags);
}



/***************************************************************************
*	uncolor_str
*
*	remove the color codes from a string
*	WARNING: allocates a new string, so you MUST free it
***************************************************************************/
char *uncolor_str(char *txt)
{
	char *idx;
	char *rpl;
	char *buf;
	int cnt;
	int len;

	if (txt == NULL
	    || txt[0] == '\0')
		return str_dup(txt);

	/*
	 * go through once to figure out how much
	 * memory to allocate
	 */
	idx = txt;
	cnt = 0;
	while (*idx != '\0') {
		if (*idx == '`') {
			idx++;
			if (*idx != '\0')
				idx++;
		} else {
			cnt++;
			idx++;
		}
	}

	/* create a new output buffer */
	buf = alloc_mem((unsigned int)cnt + 1);

	/*
	 * iterate through a second time filling
	 * the new buffer
	 */
	idx = txt;
	rpl = buf;
	len = 0;
	while (*idx != '\0' && len < cnt) {
		if (*idx == '`') {
			idx++;
			if (*idx != '\0')
				idx++;
		} else {
			*rpl++ = *idx++;
			len++;
		}
	}

	*rpl = '\0';
	return buf;
}


/***************************************************************************
*	identify_item
***************************************************************************/
void identify_item(CHAR_DATA *ch, OBJ_DATA *obj)
{
	AFFECT_DATA *paf;
	int iter;
	SKILL *skill;


	printf_to_char(ch, "Object '%s' is type %s\n\rExtra flags %s.\n\r",
		       obj->name,
		       item_type_name(obj),
		       extra_bit_name((long)obj->extra_flags));
	printf_to_char(ch, "Extra2 flags %s\n\r"
		       "Weight is %d, value is %d, level is %d.\n\r",
		       extra2_bit_name((long)obj->extra2_flags),
		       (obj->weight > 0) ? obj->weight / 10 : 0,
		       obj->cost,
		       obj->level);

	switch (obj->item_type) {
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_PILL:
		printf_to_char(ch, "Level %d spells of:", obj->value[0]);

		for (iter = 1; iter <= 4; iter++)
			if ((skill = resolve_skill_sn((int)obj->value[iter])) != NULL)
				printf_to_char(ch, " '%s'", skill->name);

		send_to_char(".\n\r", ch);
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		printf_to_char(ch, "Has %ld charges of level %ld", obj->value[2], obj->value[0]);

		if ((skill = resolve_skill_sn((int)obj->value[3])) != NULL) {
			send_to_char(" '", ch);
			send_to_char(skill->name, ch);
			send_to_char("'", ch);
		}

		send_to_char(".\n\r", ch);
		break;
	case ITEM_DICE:
		printf_to_char(ch, "%d - %d sided dice.\n\r", obj->value[0], obj->value[1]);
		break;
	case ITEM_DOLL:
		if (obj->target != NULL)
			printf_to_char(ch, "Is currently linked to %s.\n\r",
				       obj->target->name);
		break;

	case ITEM_DRINK_CON:
		printf_to_char(ch, "It holds %s-colored %s.\n\r",
			       liq_table[obj->value[2]].liq_color,
			       liq_table[obj->value[2]].liq_name);
		break;

	case ITEM_CONTAINER:
		printf_to_char(ch, "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
			       obj->value[0], obj->value[3],
			       cont_bit_name((long)obj->value[1]));

		if (obj->value[4] != 100)
			printf_to_char(ch, "Weight multiplier: %d%%\n\r",
				       obj->value[4]);
		break;

	case ITEM_WEAPON:
		send_to_char("Weapon type is ", ch);
		switch (obj->value[0]) {
		case (WEAPON_EXOTIC):
			send_to_char("exotic.\n\r", ch);
			break;
		case (WEAPON_SWORD):
			send_to_char("sword.\n\r", ch);
			break;
		case (WEAPON_DAGGER):
			send_to_char("dagger.\n\r", ch);
			break;
		case (WEAPON_SPEAR):
			send_to_char("spear/staff.\n\r", ch);
			break;
		case (WEAPON_MACE):
			send_to_char("mace/club.\n\r", ch);
			break;
		case (WEAPON_AXE):
			send_to_char("axe.\n\r", ch);
			break;
		case (WEAPON_FLAIL):
			send_to_char("flail.\n\r", ch);
			break;
		case (WEAPON_WHIP):
			send_to_char("whip.\n\r", ch);
			break;
		case (WEAPON_POLEARM):
			send_to_char("polearm.\n\r", ch);
			break;
		default:
			send_to_char("unknown.\n\r", ch);
			break;
		}

		if (obj->obj_idx->new_format) {
			printf_to_char(ch, "Damage is %dd%d(average %d).\n\r",
				       obj->value[1], obj->value[2],
				       (1 + obj->value[2]) * obj->value[1] / 2);
		} else {
			printf_to_char(ch, "Damage is %d to %d(average %d).\n\r",
				       obj->value[1], obj->value[2],
				       (obj->value[1] + obj->value[2]) / 2);
		}

		printf_to_char(ch, "Damage noun is %s.\n\r", attack_table[obj->value[3]].noun);

		if (obj->value[4])       /* weapon flags */
			printf_to_char(ch, "Weapons flags: %s\n\r", weapon_bit_name((long)obj->value[4]));
		break;

	case ITEM_ARMOR:
		printf_to_char(ch, "Worn on: %s\n\r", wear_bit_name((long)obj->wear_flags));
		printf_to_char(ch, "Armor class is %ld pierce, %ld bash, %ld slash, and %ld vs. magic.\n\r",
			       obj->value[0], obj->value[1], obj->value[2], obj->value[3]);

		break;
	case ITEM_SOCKETS:
		send_to_char("Gem type is: ", ch);
		switch (obj->value[0]) {
		case 0: send_to_char("none.  Tell an immortal.\n\r", ch);    break;
		case 1: send_to_char("sapphire.\n\r", ch);   break;
		case 2: send_to_char("ruby.\n\r", ch);   break;
		case 3: send_to_char("emerald.\n\r", ch);    break;
		case 4: send_to_char("diamond.\n\r", ch);    break;
		case 5: send_to_char("topaz.\n\r", ch);  break;
		case 6: send_to_char("skull.\n\r", ch);  break;
		default: send_to_char("Unknown.  Tell an immortal.\n\r", ch);    break;
		}
		send_to_char("Gem value is: ", ch);
		switch (obj->value[1]) {
		case 0: send_to_char("chip.\n\r", ch);    break;
		case 1: send_to_char("flawed.\n\r", ch);   break;
		case 2: send_to_char("flawless.\n\r", ch);   break;
		case 3: send_to_char("perfect.\n\r", ch);    break;
		default: send_to_char("Unknown.  Tell an immortal.\n\r", ch);   break;
		}
		break;
	}

	if (!obj->enchanted) {
		for (paf = obj->obj_idx->affected; paf != NULL; paf = paf->next) {
			if (paf->location != APPLY_NONE && paf->modifier != 0) {
				printf_to_char(ch, "Affects %s by %d",
					       affect_loc_name((long)paf->location),
					       paf->modifier);
				if ((paf->location == APPLY_MLAG) || (paf->location == APPLY_TLAG))
					send_to_char("%", ch);
				send_to_char(".\n\r", ch);
			}

			if (paf->bitvector) {
				switch (paf->where) {
				case TO_AFFECTS:
					printf_to_char(ch, "Adds %s affect.\n\r",
						       affect_bit_name(paf->bitvector));
					break;
				case TO_OBJECT:
					printf_to_char(ch, "Adds %s object flag.\n\r",
						       extra_bit_name(paf->bitvector));
					break;
				case TO_IMMUNE:
					printf_to_char(ch, "Adds immunity to %s.\n\r",
						       imm_bit_name(paf->bitvector));
					break;
				case TO_RESIST:
					printf_to_char(ch, "Adds resistance to %s.\n\r",
						       imm_bit_name(paf->bitvector));
					break;
				case TO_VULN:
					printf_to_char(ch, "Adds vulnerability to %s.\n\r",
						       imm_bit_name(paf->bitvector));
					break;
				default:
					printf_to_char(ch, "Unknown bit %d: %d\n\r",
						       paf->where, paf->bitvector);
					break;
				}
			}
		}
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next) {
		if (paf->location != APPLY_NONE && paf->modifier != 0) {
			printf_to_char(ch, "Affects %s by %d",
				       affect_loc_name((long)paf->location), paf->modifier);
			if (paf->duration > -1)
				printf_to_char(ch, ", %d hours.\n\r", paf->duration);
			else
				send_to_char(".\n\r", ch);
		}

		if (paf->bitvector) {
			switch (paf->where) {
			case TO_AFFECTS:
				printf_to_char(ch, "Adds %s affect.\n\r",
					       affect_bit_name(paf->bitvector));
				break;
			case TO_OBJECT:
				printf_to_char(ch, "Adds %s object flag.\n\r",
					       extra_bit_name(paf->bitvector));
				break;
			case TO_WEAPON:
				printf_to_char(ch, "Adds %s weapon flags.\n\r",
					       weapon_bit_name(paf->bitvector));
				break;
			case TO_IMMUNE:
				printf_to_char(ch, "Adds immunity to %s.\n\r",
					       imm_bit_name(paf->bitvector));
				break;
			case TO_RESIST:
				printf_to_char(ch, "Adds resistance to %s.\n\r",
					       imm_bit_name(paf->bitvector));
				break;
			case TO_VULN:
				printf_to_char(ch, "Adds vulnerability to %s.\n\r",
					       imm_bit_name(paf->bitvector));
				break;
			default:
				printf_to_char(ch, "Unknown bit %d: %d\n\r",
					       paf->where, paf->bitvector);
				break;
			}
		}
	}
}



/********************************************************************
*       furniture_check
*
*       check to make sure that the character is no longer
*       on a carry-able piece of furniture
********************************************************************/
void furniture_check(CHAR_DATA *ch)
{
	OBJ_DATA *obj_on;

	if (ch->on != NULL && IS_SET(ch->on->wear_flags, ITEM_TAKE)) {
		obj_on = ch->on;
		ch->on = NULL;

		get_obj(ch, obj_on, NULL);
	}
}



/***************************************************************************
*	get_death_room
*
*	get the deathroom for a character
***************************************************************************/
ROOM_INDEX_DATA *get_death_room(CHAR_DATA *ch)
{
	ROOM_INDEX_DATA *location;
	long deathroom;

	deathroom = ROOM_VNUM_ALTAR;

	if (ch->deathroom > 0)
		deathroom = ch->deathroom;

	if ((location = get_room_index(deathroom)) == NULL)
		return NULL;

	return location;
}

/*
 * Get a character's hours.
 */
int get_hours_played(CHAR_DATA *ch)
{
	return (ch->played + (int)(globalSystemState.current_time - ch->logon)) / 3600;
}

/*
 * To go with get_hours_played
 */
int get_minutes_played(CHAR_DATA *ch)
{
	return ((ch->played + (int)(globalSystemState.current_time - ch->logon)) / 60) % 60;
}

/*
 * To go with get_minutes_played
 */
int get_seconds_played(CHAR_DATA *ch)
{
	return (ch->played + (int)(globalSystemState.current_time - ch->logon)) % 60;
}

int get_session_hours(CHAR_DATA *ch)
{
	return (int)(globalSystemState.current_time - ch->logon) / 3600;
}

int get_session_minutes(CHAR_DATA *ch)
{
	return (((int)(globalSystemState.current_time - ch->logon)) / 60) % 60;
}

int get_session_seconds(CHAR_DATA *ch)
{
	return (int)(globalSystemState.current_time - ch->logon) % 60;
}

