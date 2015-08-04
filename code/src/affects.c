#include <stdio.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"



/***************************************************************************
*	external functions
***************************************************************************/
void dam_message(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, bool immune);

/***************************************************************************
*	affect_enchant
***************************************************************************/
void affect_enchant(OBJ_DATA *obj)
{
	if (!obj->enchanted) {
		AFFECT_DATA *paf;
		AFFECT_DATA *af_new;

		obj->enchanted = TRUE;

		for (paf = obj->obj_idx->affected; paf != NULL; paf = paf->next) {
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
}


/***************************************************************************
*	affect_modify
*
*	modify the properties of an affect
***************************************************************************/
void affect_modify(CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd)
{
	OBJ_DATA *wield;
	long mod;
	int iter;

	mod = paf->modifier;

	if (fAdd) {
		switch (paf->where) {
		case TO_AFFECTS:
			SET_BIT(ch->affected_by, paf->bitvector);
			break;
		case TO_ACT_FLAG:
			SET_BIT(ch->act, paf->bitvector);
			break;
		case TO_IMMUNE:
			SET_BIT(ch->imm_flags, paf->bitvector);
			break;
		case TO_RESIST:
			SET_BIT(ch->res_flags, paf->bitvector);
			break;
		case TO_VULN:
			SET_BIT(ch->vuln_flags, paf->bitvector);
			break;
		}
	} else {
		switch (paf->where) {
		case TO_AFFECTS:
			REMOVE_BIT(ch->affected_by, paf->bitvector);
			break;
		case TO_ACT_FLAG:
			REMOVE_BIT(ch->act, paf->bitvector);
			break;
		case TO_IMMUNE:
			REMOVE_BIT(ch->imm_flags, paf->bitvector);
			break;
		case TO_RESIST:
			REMOVE_BIT(ch->res_flags, paf->bitvector);
			break;
		case TO_VULN:
			REMOVE_BIT(ch->vuln_flags, paf->bitvector);
			break;
		}

		mod = 0 - mod;
	}

	switch (paf->location) {
	default:
		/*bug("affect_modify: unknown location %d.", paf->location);*/
		return;

	case APPLY_NONE:
		break;
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
	case APPLY_CLASS:
		break;
	case APPLY_LEVEL:
		break;
	case APPLY_AGE:
		break;
	case APPLY_HEIGHT:
		break;
	case APPLY_WEIGHT:
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
	case APPLY_GOLD:
		break;
	case APPLY_EXP:
		break;
	case APPLY_AC:
		for (iter = 0; iter < 4; iter++)
			ch->armor[iter] += mod;
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
	case APPLY_MLAG:
		ch->mLag += mod;
		break;
	case APPLY_TLAG:
		ch->tLag += mod;
		break;
	case APPLY_SPELL_AFFECT:
		break;
	}

	if (!IS_NPC(ch) && (wield = get_eq_char(ch, WEAR_WIELD)) != NULL
	    && get_obj_weight(wield) > get_wield_weight(ch)) {
		static int depth;

		if (depth == 0) {
			depth++;
			act("You drop $p.", ch, wield, NULL, TO_CHAR);
			act("$n drops $p.", ch, wield, NULL, TO_ROOM);
			obj_from_char(wield);
			obj_to_room(wield, ch->in_room);
			save_char_obj(ch);
			depth--;
		}
	}

	/* Check for racial permanents.
	 * Added by Monrick, 2/2008    */
	ch->affected_by = ch->affected_by | race_table[ch->race].aff;
	ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
	ch->res_flags = ch->res_flags | race_table[ch->race].res;
	ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;

	return;
}


/***************************************************************************
*	affect_find
*
*	find an affect in an affects list
***************************************************************************/
AFFECT_DATA *affect_find(AFFECT_DATA *paf, SKILL *skill)
{
	AFFECT_DATA *paf_find;

	if (skill != NULL && paf != NULL) {
		for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next)
			if (paf_find->type == skill->sn)
				return paf_find;
	}

	return NULL;
}

/***************************************************************************
*	affect_check
*
*	fix a characters affects when they remove an item
***************************************************************************/
void affect_check(CHAR_DATA *ch, int where, long vector)
{
	AFFECT_DATA *paf;
	OBJ_DATA *obj;

	if (where == TO_OBJECT
	    || where == TO_WEAPON
	    || vector == 0)
		return;

	for (paf = ch->affected; paf != NULL; paf = paf->next) {
		if (paf->where == where && paf->bitvector == vector) {
			switch (where) {
			case TO_AFFECTS:
				SET_BIT(ch->affected_by, vector);
				break;
			case TO_ACT_FLAG:
				SET_BIT(ch->act, vector);
				break;
			case TO_IMMUNE:
				SET_BIT(ch->imm_flags, vector);
				break;
			case TO_RESIST:
				SET_BIT(ch->res_flags, vector);
				break;
			case TO_VULN:
				SET_BIT(ch->vuln_flags, vector);
				break;
			}
			return;
		}
	}

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if (obj->wear_loc == -1)
			continue;

		for (paf = obj->affected; paf != NULL; paf = paf->next) {
			if (paf->where == where && paf->bitvector == vector) {
				switch (where) {
				case TO_AFFECTS:
					SET_BIT(ch->affected_by, vector);
					break;
				case TO_ACT_FLAG:
					SET_BIT(ch->act, vector);
					break;
				case TO_IMMUNE:
					SET_BIT(ch->imm_flags, vector);
					break;
				case TO_RESIST:
					SET_BIT(ch->res_flags, vector);
					break;
				case TO_VULN:
					SET_BIT(ch->vuln_flags, vector);
					break;
				}
				return;
			}
		}

		if (obj->enchanted)
			continue;

		for (paf = obj->obj_idx->affected; paf != NULL; paf = paf->next) {
			if (paf->where == where && paf->bitvector == vector) {
				switch (where) {
				case TO_AFFECTS:
					SET_BIT(ch->affected_by, vector);
					break;
				case TO_ACT_FLAG:
					SET_BIT(ch->act, vector);
					break;
				case TO_IMMUNE:
					SET_BIT(ch->imm_flags, vector);
					break;
				case TO_RESIST:
					SET_BIT(ch->res_flags, vector);
					break;
				case TO_VULN:
					SET_BIT(ch->vuln_flags, vector);
					break;
				}

				return;
			}
		}
	}
}

/***************************************************************************
*	affect_to_char
*
*	give an affect to a character
***************************************************************************/
void affect_to_char(CHAR_DATA *ch, AFFECT_DATA *paf)
{
	AFFECT_DATA *paf_new;

	paf_new = new_affect();
	*paf_new = *paf;

	if (paf_new->skill == NULL)
		paf_new->skill = resolve_skill_sn(paf->type);

	paf_new->next = ch->affected;
	ch->affected = paf_new;

	affect_modify(ch, paf_new, TRUE);
	return;
}


/***************************************************************************
*	affect_to_object
*
*	give an affect to an object
***************************************************************************/
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
	AFFECT_DATA *paf_new;

	paf_new = new_affect();
	*paf_new = *paf;

	if (paf_new->skill == NULL)
		paf_new->skill = resolve_skill_sn(paf->type);

	paf_new->next = obj->affected;
	obj->affected = paf_new;

	/* apply any affect vectors to the object's extra_flags */
	if (paf->bitvector) {
		switch (paf->where) {
		case TO_OBJECT:
			SET_BIT(obj->extra_flags, paf->bitvector);
			break;
		case TO_WEAPON:
			if (obj->item_type == ITEM_WEAPON)
				SET_BIT(obj->value[4], paf->bitvector);
			break;
		}
	}

	return;
}


/***************************************************************************
*	affect_to_room
*
*	add an affect to a room
***************************************************************************/
void affect_to_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
	AFFECT_DATA *paf_new;

	paf_new = new_affect();
	*paf_new = *paf;

	if (paf_new->skill == NULL)
		paf_new->skill = resolve_skill_sn(paf->type);

	paf_new->next = room->affected;
	room->affected = paf_new;
	return;
}



/***************************************************************************
*	affect_remove
*
*	remove an affect from a character
***************************************************************************/
void affect_remove(CHAR_DATA *ch, AFFECT_DATA *paf)
{
	int where;
	long vector;

	if (ch->affected == NULL) {
		bug("affect_remove: no affect.", 0);
		return;
	}

	affect_modify(ch, paf, FALSE);
	where = paf->where;
	vector = paf->bitvector;
	if (paf == ch->affected) {
		ch->affected = paf->next;
	} else {
		AFFECT_DATA *prev;

		for (prev = ch->affected; prev != NULL; prev = prev->next) {
			if (prev->next == paf) {
				prev->next = paf->next;
				break;
			}
		}

		if (prev == NULL) {
			bug("affect_remove: cannot find paf.", 0);
			return;
		}
	}

	free_affect(paf);
	affect_check(ch, where, vector);
	return;
}


/***************************************************************************
*	affect_remove_obj
*
*	remove an affect from an object
***************************************************************************/
void affect_remove_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
	int where;
	long vector;

	if (obj->affected == NULL) {
		bug("affect_remove_object: no affect.", 0);
		return;
	}

	if (obj->carried_by != NULL && obj->wear_loc != -1)
		affect_modify(obj->carried_by, paf, FALSE);
	where = paf->where;
	vector = paf->bitvector;

	/* remove flags from the object if needed */
	if (paf->bitvector) {
		switch (paf->where) {
		case TO_OBJECT:
			REMOVE_BIT(obj->extra_flags, paf->bitvector);
			break;
		case TO_WEAPON:
			if (obj->item_type == ITEM_WEAPON)
				REMOVE_BIT(obj->value[4], paf->bitvector);
			break;
		}
	}

	if (paf == obj->affected) {
		obj->affected = paf->next;
	} else {
		AFFECT_DATA *prev;

		for (prev = obj->affected; prev != NULL; prev = prev->next) {
			if (prev->next == paf) {
				prev->next = paf->next;
				break;
			}
		}

		if (prev == NULL) {
			bug("affect_remove_object: cannot find paf.", 0);
			return;
		}
	}

	free_affect(paf);
	if (obj->carried_by != NULL && obj->wear_loc != -1)
		affect_check(obj->carried_by, where, vector);
	return;
}


/***************************************************************************
*	affect_remove_room
*
*	remove an affect from a room
***************************************************************************/
void affect_remove_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
	if (room->affected == NULL) {
		bug("affect_remove_room: no affect.", 0);
		return;
	}

	if (paf == room->affected) {
		room->affected = paf->next;
	} else {
		AFFECT_DATA *prev;

		for (prev = room->affected; prev != NULL; prev = prev->next) {
			if (prev->next == paf) {
				prev->next = paf->next;
				break;
			}
		}

		if (prev == NULL) {
			bug("affect_remove_room: cannot find paf.", 0);
			return;
		}
	}

	free_affect(paf);
	return;
}


/***************************************************************************
*	affect_strip
*
*	strips all affects of the given sn
***************************************************************************/
void affect_strip(CHAR_DATA *ch, SKILL *skill)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

	if (ch != NULL) {
		for (paf = ch->affected; paf != NULL; paf = paf_next) {
			paf_next = paf->next;
			if (paf->type == skill->sn)
				affect_remove(ch, paf);
		}
	}
	return;
}

/***************************************************************************
*	affect_strip_room
*
*	strips all affects of the given sn
***************************************************************************/
void affect_strip_room(ROOM_INDEX_DATA *room, int sn)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

	if (room != NULL) {
		for (paf = room->affected; paf != NULL; paf = paf_next) {
			paf_next = paf->next;
			if (paf->type == sn)
				affect_remove_room(room, paf);
		}
	}
	return;
}

/***************************************************************************
*	affect_join
*
*	add the properties of one affect to an existing affect of
*	the same type
***************************************************************************/
void affect_join(CHAR_DATA *ch, AFFECT_DATA *paf)
{
	AFFECT_DATA *paf_old;

	for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next) {
		if (paf_old->type == paf->type) {
			paf->level = (int)((paf->level + paf_old->level) / 2);
			paf->duration += paf_old->duration;
			paf->modifier += paf_old->modifier;
			affect_remove(ch, paf_old);
			break;
		}
	}

	affect_to_char(ch, paf);
	return;
}



/***************************************************************************
*	is_affected
*
*	check to see if a character is affected by a spell
***************************************************************************/
bool is_affected(CHAR_DATA *ch, SKILL *skill)
{
	AFFECT_DATA *paf;

	if (skill != NULL) {
		for (paf = ch->affected; paf != NULL; paf = paf->next)
			if (paf->type == skill->sn)
				return TRUE;
	}

	return FALSE;
}

/***************************************************************************
*	is_affected_room
*
*	check to see if a room is affected by a spell
***************************************************************************/
bool is_affected_room(ROOM_INDEX_DATA *room, SKILL *skill)
{
	AFFECT_DATA *paf;

	if (room == NULL)
		return FALSE;

	for (paf = room->affected; paf != NULL; paf = paf->next)
		if (paf->type == skill->sn)
			return TRUE;

	return FALSE;
}



/***************************************************************************
*	room_affect
*
*	show an affect string for a room affect
***************************************************************************/
char *room_affect(AFFECT_DATA *paf)
{
	static char buf[512];

	buf[0] = '\0';
	if (paf->type == get_skill_number("displacement"))
		sprintf(buf, "The room is `4d`Oi`^z`6z`4y`` and `4d`Oi`^s`Oo`4r`Oi`6e`On`4t`Oi`^n`Og``.\n\r");
	else if (paf->type == get_skill_number("haven"))
		sprintf(buf, "A wave of `#p`7e`&a`7c`#e`` and `!c`&a`7l`!m`` fills the room.\n\r");
	else if (paf->type == get_skill_number("mana vortex"))
		sprintf(buf, "The room is filled with a `Ps`5w`^i`6rl`^i`5n`Pg `Pm`5a`^s`5s`` of `Pe`5n`^e`6r`5g`Py``.\n\r");
	else if (paf->type == get_skill_number("parasitic_cloud"))
		sprintf(buf, "A `1m`!a`1l`!i`1c`!iou`1s `1cl`!ou`1d`` fills the room.\n\r");


	return (buf[0] != '\0') ? buf : "";
}



/***************************************************************************
*	spell affects
***************************************************************************/

/***************************************************************************
*	room affects
***************************************************************************/



/***************************************************************************
*	affect_displacement
*
*	do the displacement affect for a room
***************************************************************************/
void affect_displacement(SKILL *skill, void *target, int type, AFFECT_DATA *paf)
{
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *)target;
	ROOM_INDEX_DATA *to = NULL;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int iter;
	int count = 0;

	if ((type = AFFECT_TYPE_ROOM) && paf != NULL) {
		for (vch = room->people; vch != NULL; vch = vch_next) {
			vch_next = vch->next_in_room;
			if (!IS_NPC(vch) && !IS_IMMORTAL(vch)) {
				if (!IS_SET(room->room_flags, ROOM_BFIELD)) {
					ROOM_INDEX_DATA *temp;

					for (iter = 0; iter < 5; iter++) {
						temp = get_room_index(number_range_long(ROOM_VNUM_BFS, ROOM_VNUM_BF_END));
						if (temp != NULL && number_range(0, count) == 0) {
							to = temp;
							count++;
						}
					}
				} else {
					to = get_random_room(vch, NULL);
				}

				if (to != NULL) {
					send_to_char("You have been t`@e``l`2e``p`4o``r`4t``e`8d``!\n\r", vch);
					act("$n vanishes!", vch, NULL, NULL, TO_ROOM);
					char_from_room(vch);
					char_to_room(vch, to);
					act("$n slowly `8f`7a`&d`7e`8s`` into existence.", vch, NULL, NULL, TO_ROOM);
					do_look(vch, "auto");
				}
			}
		}
	}
}



/***************************************************************************
*	affect_parasitic_cloud
*
*	to be used in the vault and in the battlefield
***************************************************************************/
void affect_parasitic_cloud(SKILL *skill, void *target, int type, AFFECT_DATA *paf)
{
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *)target;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if ((type = AFFECT_TYPE_ROOM) && paf != NULL) {
		for (vch = room->people; vch != NULL; vch = vch_next) {
			vch_next = vch->next_in_room;
			if (!IS_IMMORTAL(vch) && !IS_NPC(vch)) {
				int dam;

				dam = number_range(paf->level * 3, paf->level * 6);
				if ((dam = damage(vch, vch, dam, skill->sn, DAM_NEGATIVE, FALSE)) > 0) {
					vch->mana -= dam;
					vch->move -= dam;

					act("A `1sw`!i`1rl`!i`1ng `1cl`!ou`1d`` comes to life `1v`!i`1c`!iou`1sl`!y`` attacks you!!",
					    vch, NULL, NULL, TO_CHAR);
					act("A `1sw`!i`1rl`!i`1ng `1cl`!ou`1d`` comes to life `1v`!i`1c`!iou`1sl`!y`` attacks $n!!",
					    vch, NULL, NULL, TO_ROOM);
					dam_message(vch, NULL, dam, skill->sn, FALSE);
					vch->position = POS_RESTING;
				}
			}
		}
	}
}




/***************************************************************************
*	character affects
***************************************************************************/
/**************************************************************************
 *	affect_black_plague
 *
 *	do the black plague affect for a character
 ***************************************************************************/
void affect_black_plague(SKILL *skill, void *target, int type, AFFECT_DATA *paf)
{
	CHAR_DATA *ch = (CHAR_DATA *)target;
	int dam;

	if (paf->level == 1)
		return;

	if ((type = AFFECT_TYPE_CHAR) && paf != NULL && ch != NULL) {
		if (ch->in_room == NULL)
			return;

		act("Blood gushes from $n's pores as the black plague slowly eats away at $s body.",
		    ch, NULL, NULL, TO_ROOM);
		send_to_char("Blood gushes from your pores as the black plague slowly eats away at your body.\n\r", ch);


		dam = 50 * UMIN(ch->level, (int)(paf->level / 5 + 1));
		ch->mana -= dam;
		ch->move -= dam;
		damage(ch, ch, dam, skill->sn, DAM_DISEASE, FALSE);
	}
}


/**************************************************************************
 *	affect_disease
 *
 *	do the disease affect for a character
 ***************************************************************************/
void affect_disease(SKILL *skill, void *target, int type, AFFECT_DATA *paf)
{
	CHAR_DATA *ch = (CHAR_DATA *)target;
	CHAR_DATA *vch;
	AFFECT_DATA af;
	int dam;


	if (paf->level == 1) {
		REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
		return;
	}

	if ((type = AFFECT_TYPE_CHAR) && paf != NULL && ch != NULL) {
		if (ch->in_room == NULL)
			return;

		af.where = TO_AFFECTS;
		af.type = skill->sn;
		af.skill = skill;
		af.level = paf->level;
		af.duration = (int)(number_range(1, 2 * af.level));
		af.location = APPLY_STR;
		af.modifier = -5;
		af.bitvector = AFF_PLAGUE;

		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			if (!saves_spell(af.level + 5, vch, DAM_DISEASE)
			    && !IS_IMMORTAL(vch)
			    && !IS_AFFECTED(vch, AFF_PLAGUE) && number_bits(4) == 0) {
				send_to_char("You feel hot and feverish.\n\r", vch);
				act("$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM);
				affect_join(vch, &af);
			}
		}

		act("$n writhes in agony as plague sores erupt from $s skin.", ch, NULL, NULL, TO_ROOM);
		send_to_char("You writhe in agony from the plague.\n\r", ch);

		dam = 20 * UMIN(ch->level, (int)(paf->level / 5 + 1));
		ch->mana -= dam;
		ch->move -= dam;
		damage(ch, ch, dam, skill->sn, DAM_DISEASE, FALSE);
	}
}


/**************************************************************************
 *	affect_burning_flames
 *
 *	do the burning flames affect
 ***************************************************************************/
void affect_burning_flames(SKILL *skill, void *target, int type, AFFECT_DATA *paf)
{
	CHAR_DATA *ch = (CHAR_DATA *)target;

	if (paf->level == 1)
		return;

	if ((type = AFFECT_TYPE_CHAR) && paf != NULL && ch != NULL) {
		int dam;

		if (ch->in_room == NULL)
			return;

		dam = number_range(paf->level, paf->level * 3);
		if (damage(ch, ch, dam, skill->sn, DAM_FIRE, FALSE) > 0) {
			act("$n `8screams`` in agony as `!flames`` sear their flesh!", ch, NULL, NULL, TO_ROOM);
			send_to_char("You `8scream`` in agony as `!flames`` engulf you!\n\r", ch);
		}
	}
}


/**************************************************************************
 *	affect_poison
 *
 *	do the poison affect
 ***************************************************************************/
void affect_poison(SKILL *skill, void *target, int type, AFFECT_DATA *paf)
{
	CHAR_DATA *ch = (CHAR_DATA *)target;

	if (paf->level == 1)
		return;

	if ((type = AFFECT_TYPE_CHAR) && paf != NULL && ch != NULL) {
		int dam;
		if (ch->in_room == NULL)
			return;

		dam = number_range(paf->level / 10 + 1, paf->level / 3 + 1);
		if (!IS_AFFECTED(ch, AFF_SLOW)
		    && damage(ch, ch, dam, skill->sn, DAM_POISON, FALSE) > 0) {
			act("$n shivers and suffers.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You shiver and suffer.\n\r", ch);
		}
	}
}




/***************************************************************************
*	blood_rage
*
*	add the blood rage affect to a werebeast
***************************************************************************/
void blood_rage(CHAR_DATA *ch)
{
	AFFECT_DATA af;
	SKILL *skill;

	if (ch->race == race_lookup("werebeast")
	    && (skill = skill_lookup("blood rage")) != NULL) {
		affect_strip(ch, skill);

		af.where = TO_AFFECTS;
		af.type = skill->sn;
		af.skill = skill;
		af.level = 0;
		af.modifier = ch->level;
		af.duration = -1;
		af.bitvector = 0;
		af.location = APPLY_HITROLL;
		affect_to_char(ch, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(ch, &af);
	}
}
