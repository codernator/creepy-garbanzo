#include "merc.h"
#include "recycle.h"


void acid_effect(void *vo, int level, int dam, int target)
{
	GAMEOBJECT *obj;
	GAMEOBJECT *obj_next;

	if (target == TARGET_ROOM) { /* nail objects on the floor */
		struct room_index_data *room = (struct room_index_data *)vo;

		for (obj = room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			acid_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR) { /* do the effect on a victim */
		CHAR_DATA *victim = (CHAR_DATA *)vo;

		/* let's toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			acid_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ) { /* toast an object */
		GAMEOBJECT *t_obj;
		GAMEOBJECT *n_obj;
		int chance;
		char *msg;

		obj = (GAMEOBJECT *)vo;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
		    || IS_OBJ_STAT(obj, ITEM_NOPURGE)
		    || number_range(0, 4) == 0)
			return;

		chance = level / 4 + dam / 10;

		if (chance > 25)
			chance = (chance - 25) / 2 + 25;

		if (chance > 50)
			chance = (chance - 50) / 2 + 50;

		if (IS_OBJ_STAT(obj, ITEM_BLESS))
			chance -= 5;
		chance -= obj->level * 2;

		if (chance > 25)
			chance = 25;

		switch (obj->item_type) {
		default:
			return;
		case ITEM_CONTAINER:
		case ITEM_CORPSE_PC:
		case ITEM_CORPSE_NPC:
			msg = "$p fumes and dissolves.";
			break;
		case ITEM_ARMOR:
			msg = "$p is pitted and etched.";
			break;
		case ITEM_CLOTHING:
			msg = "$p is corroded into scrap.";
			break;
		case ITEM_STAFF:
		case ITEM_WAND:
			chance -= 10;
			msg = "$p corrodes and breaks.";
			break;
		case ITEM_SCROLL:
			chance += 10;
			msg = "$p is burned into waste.";
			break;
		}

		chance = URANGE(5, chance, 95);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL);


		if (obj->item_type == ITEM_ARMOR) {      /* etch it */
			AFFECT_DATA *paf;
			bool af_found = false;
			int idx;

			affect_enchant(obj);

			for (paf = obj->affected; paf != NULL; paf = paf->next) {
				if (paf->location == APPLY_AC) {
					af_found = true;
					paf->type = -1;
					paf->modifier += 1;
					paf->level = UMAX(paf->level, level);
					break;
				}
			}

			/* needs a new affect */
			if (!af_found) {
				paf = new_affect();

				paf->type = -1;
				paf->level = level;
				paf->duration = -1;
				paf->location = APPLY_AC;
				paf->modifier = 1;
				paf->bitvector = 0;
				paf->next = obj->affected;
				obj->affected = paf;
			}

			if (obj->carried_by != NULL && obj->wear_loc != WEAR_NONE) {
				for (idx = 0; idx < 4; idx++)
					obj->carried_by->armor[idx] += 1;
			}
			return;
		}

		/* get rid of the object */
		if (obj->contains) {     /* dump contents */
			for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj) {
				n_obj = t_obj->next_content;
				obj_from_obj(t_obj);
				if (obj->in_room != NULL) {
					obj_to_room(t_obj, obj->in_room);
				} else if (obj->carried_by != NULL) {
					obj_to_room(t_obj, obj->carried_by->in_room);
				} else {
					extract_obj(t_obj);
					continue;
				}
				acid_effect(t_obj, level / (int)2, dam / 2, TARGET_OBJ);
			}
		}

		extract_obj(obj);
		return;
	}
}


void cold_effect(void *vo, int level, int dam, int target)
{
	GAMEOBJECT *obj;
	GAMEOBJECT *obj_next;

	if (target == TARGET_ROOM) { /* nail objects on the floor */
		struct room_index_data *room = (struct room_index_data *)vo;
		GAMEOBJECT *obj;
		GAMEOBJECT *obj_next;

		for (obj = room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			cold_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR) { /* whack a character */
		CHAR_DATA *victim = (CHAR_DATA *)vo;
		SKILL *skill;

		skill = skill_lookup("chill touch");
		/* chill touch effect */
		if (skill != NULL
		    && !saves_spell(level / 2 + dam * 2, victim, DAM_COLD)) {
			AFFECT_DATA af;

			act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM);
			act("A chill sinks deep into your bones.", victim, NULL, NULL, TO_CHAR);
			af.where = TO_AFFECTS;
			af.type = skill->sn;
			af.skill = skill;
			af.level = level;
			af.duration = 6;
			af.location = APPLY_STR;
			af.modifier = number_range(1, (level / 30)) * -1;
			af.bitvector = 0;
			affect_join(victim, &af);
		}

		/* hunger! (warmth sucked out */
		if (!IS_NPC(victim) && victim->pcdata->condition[COND_HUNGER] != -151)
			gain_condition(victim, COND_HUNGER, dam / 20);

		/* let's toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			cold_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ) { /* toast an object */
		int chance;
		char *msg;

		obj = (GAMEOBJECT *)vo;
		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
		    || IS_OBJ_STAT(obj, ITEM_NOPURGE)
		    || number_range(0, 4) == 0)
			return;

		chance = level / 4 + dam / 10;

		if (chance > 25)
			chance = (chance - 25) / 2 + 25;
		if (chance > 50)
			chance = (chance - 50) / 2 + 50;

		if (IS_OBJ_STAT(obj, ITEM_BLESS))
			chance -= 5;

		chance -= obj->level * 2;

		switch (obj->item_type) {
		default:
			return;
		case ITEM_POTION:
			msg = "$p freezes and shatters!";
			chance += 25;
			break;
		case ITEM_DRINK_CON:
			msg = "$p freezes and shatters!";
			chance += 5;
			break;
		}

		chance = URANGE(5, chance, 95);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL);

		extract_obj(obj);
		return;
	}
}



void fire_effect(void *vo, int level, int dam, int target)
{
	GAMEOBJECT *obj;
	GAMEOBJECT *obj_next;

	if (target == TARGET_ROOM) {     /* nail objects on the floor */
		struct room_index_data *room = (struct room_index_data *)vo;

		for (obj = room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			fire_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR) {     /* do the effect on a victim */
		CHAR_DATA *victim = (CHAR_DATA *)vo;
		SKILL *skill;

		skill = skill_lookup("fire breath");

		/* chance of blindness */
		if (skill != NULL
		    && !IS_AFFECTED(victim, AFF_BLIND)
		    && !saves_spell(level / 4 + dam / 20, victim, DAM_FIRE)) {
			AFFECT_DATA af;

			act("$n is blinded by smoke!", victim, NULL, NULL, TO_ROOM);
			act("Your eyes tear up from smoke...you can't see a thing!",
			    victim, NULL, NULL, TO_CHAR);

			af.where = TO_AFFECTS;
			af.type = skill->sn;
			af.skill = skill;
			af.level = level;
			af.duration = (int)number_range(0, 10);
			af.location = APPLY_HITROLL;
			af.modifier = -4;
			af.bitvector = AFF_BLIND;

			affect_to_char(victim, &af);
		}

		/* getting thirsty */
		if (!IS_NPC(victim))
			gain_condition(victim, COND_THIRST, dam / 20);

		/* let's toast some gear! */
		for (obj = victim->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			fire_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ) { /* toast an object */
		GAMEOBJECT *t_obj;
		GAMEOBJECT *n_obj;
		int chance;
		char *msg;

		obj = (GAMEOBJECT *)vo;
		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
		    || IS_OBJ_STAT(obj, ITEM_NOPURGE)
		    || number_range(0, 4) == 0)
			return;

		chance = level / 4 + dam / 10;

		if (chance > 25)
			chance = (chance - 25) / 2 + 25;
		if (chance > 50)
			chance = (chance - 50) / 2 + 50;

		if (IS_OBJ_STAT(obj, ITEM_BLESS))
			chance -= 5;
		chance -= obj->level * 2;

		switch (obj->item_type) {
		default:
			return;
		case ITEM_CONTAINER:
			msg = "$p ignites and burns!";
			break;
		case ITEM_POTION:
			chance += 25;
			msg = "$p bubbles and boils!";
			break;
		case ITEM_SCROLL:
			chance += 50;
			msg = "$p crackles and burns!";
			break;
		case ITEM_STAFF:
			chance += 10;
			msg = "$p smokes and chars!";
			break;
		case ITEM_WAND:
			msg = "$p sparks and sputters!";
			break;
		case ITEM_FOOD:
			msg = "$p blackens and crisps!";
			break;
		case ITEM_PILL:
			msg = "$p melts and drips!";
			break;
		}

		chance = URANGE(5, chance, 95);

		if (number_percent() > chance)
			return;

		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL);

		if (obj->contains) {
			for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj) {
				n_obj = t_obj->next_content;

				obj_from_obj(t_obj);
				if (obj->in_room != NULL) {
					obj_to_room(t_obj, obj->in_room);
				} else if (obj->carried_by != NULL) {
					obj_to_room(t_obj, obj->carried_by->in_room);
				} else {
					extract_obj(t_obj);
					continue;
				}
				fire_effect(t_obj, level / (int)2, dam / 2, TARGET_OBJ);
			}
		}

		extract_obj(obj);
		return;
	}
}

void poison_effect(void *vo, int level, int dam, int target)
{
	GAMEOBJECT *obj;
	GAMEOBJECT *obj_next;

	if (target == TARGET_ROOM) { /* nail objects on the floor */
		struct room_index_data *room = (struct room_index_data *)vo;

		for (obj = room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			poison_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR) { /* do the effect on a victim */
		CHAR_DATA *victim = (CHAR_DATA *)vo;
		SKILL *skill;
		/* chance of poisoning */
		skill = skill_lookup("poison");

		if (skill != NULL
		    && !saves_spell(level / 4 + dam / 20, victim, DAM_POISON)) {
			AFFECT_DATA af;

			send_to_char("You feel poison coursing through your veins.\n\r", victim);
			act("$n looks very ill.", victim, NULL, NULL, TO_ROOM);

			af.where = TO_AFFECTS;
			af.type = skill->sn;
			af.skill = skill;
			af.level = level;
			af.duration = level / (int)2;
			af.location = APPLY_STR;
			af.modifier = -1;
			af.bitvector = AFF_POISON;
			affect_join(victim, &af);
		}

		/* equipment */
		for (obj = victim->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			poison_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ) { /* do some poisoning */
		int chance;

		obj = (GAMEOBJECT *)vo;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
		    || IS_OBJ_STAT(obj, ITEM_BLESS)
		    || number_range(0, 4) == 0)
			return;

		chance = level / 4 + dam / 10;
		if (chance > 25)
			chance = (chance - 25) / 2 + 25;

		if (chance > 50)
			chance = (chance - 50) / 2 + 50;
		chance -= obj->level * 2;

		switch (obj->item_type) {
		default:
			return;
		case ITEM_FOOD:
			break;
		case ITEM_DRINK_CON:
			if (obj->value[0] == obj->value[1])
				return;
			break;
		}

		chance = URANGE(5, chance, 95);

		if (number_percent() > chance)
			return;

		obj->value[3] = 1;
		return;
	}
}


void shock_effect(void *vo, int level, int dam, int target)
{
	GAMEOBJECT *obj;
	GAMEOBJECT *obj_next;

	if (target == TARGET_ROOM) {
		struct room_index_data *room = (struct room_index_data *)vo;

		for (obj = room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			shock_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR) {
		CHAR_DATA *victim = (CHAR_DATA *)vo;

		/* toast some gear */
		for (obj = victim->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			shock_effect(obj, level, dam, TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_OBJ) {
		int chance;
		char *msg;

		obj = (GAMEOBJECT *)vo;

		if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF)
		    || IS_OBJ_STAT(obj, ITEM_NOPURGE)
		    || number_range(0, 4) == 0)
			return;
		chance = level / 4 + dam / 10;

		if (chance > 25)
			chance = (chance - 25) / 2 + 25;
		if (chance > 50)
			chance = (chance - 50) / 2 + 50;

		if (IS_OBJ_STAT(obj, ITEM_BLESS))
			chance -= 5;
		chance -= obj->level * 2;

		switch (obj->item_type) {
		default:
			return;
		case ITEM_WAND:
		case ITEM_STAFF:
			chance += 10;
			msg = "$p overloads and explodes!";
			break;
		case ITEM_JEWELRY:
			chance -= 10;
			msg = "$p is fused into a worthless lump.";
		}

		chance = URANGE(5, chance, 95);

		if (number_percent() > chance)
			return;


		if (obj->carried_by != NULL)
			act(msg, obj->carried_by, obj, NULL, TO_ALL);
		else if (obj->in_room != NULL && obj->in_room->people != NULL)
			act(msg, obj->in_room->people, obj, NULL, TO_ALL);

		extract_obj(obj);
		return;
	}
}

bool vorpal_effect(CHAR_DATA *ch, CHAR_DATA *victim, GAMEOBJECT *wield)
{
	int vorpal_chance = 0;

	vorpal_chance = number_range(1, 10000);
	if (vorpal_chance > 9998 && vorpal_chance <= 10000) {
		if (wield != NULL)
			act("$n is decapitated by $p!", victim, wield, NULL, TO_ROOM);
		else
			act("$n has been decapitated!!", victim, NULL, NULL, TO_ROOM);

		if (wield != NULL)
			act("You feel $p sever your head from the rest of your body.", victim, wield, NULL, TO_CHAR);
		else
			act("You feel your head severed from the rest of your body.", victim, NULL, NULL, TO_CHAR);

		raw_kill(victim, ch);
		return true;
	}

	return false;
}
