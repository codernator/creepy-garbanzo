#include <string.h>
#include <stdio.h>
#include "merc.h"
#include "object.h"
#include "magic.h"
#include "recycle.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"



/***************************************************************************
 *	local functions
 ***************************************************************************/
int count_slots(GAMEOBJECT * obj);


/***************************************************************************
 *	spell_imprint
 *
 *	imprint a spell onto an object - scroll or potion
 *	used for brew and scribe
 ***************************************************************************/
static void spell_imprint(SKILL *skill, int level, CHAR_DATA *ch, void *vo)
{
    GAMEOBJECT *obj = (GAMEOBJECT *)vo;
    LEVEL_INFO *level_info;
    LEARNED *learned;
    SKILL *skill_retro;
    char buf[MAX_STRING_LENGTH];
    char short_descr[MAX_STRING_LENGTH];
    char name[MAX_STRING_LENGTH];
    int sp_slot;
    int iter;
    int mana;

    if (skill == NULL || skill->spells == NULL) {
	send_to_char("That is not a spell.\n\r", ch);
	return;
    }

    /* counting the number of spells contained within */
    if ((sp_slot = count_slots(obj)) > 3) {
	act("$p cannot contain any more spells.", ch, obj, NULL, TO_CHAR);
	return;
    }

    /*
     * scribe/brew costs <difficulty> times the normal mana
     * required to cast the spell
     */

    mana = 0;
    level_info = get_skill_level(ch, skill);
    if (level_info == NULL || level_info->level > ch->level)
	send_to_char("You do not knnow that spell.\n\r", ch);

    if (skill->min_mana > 0
	    && ch->level >= level_info->level)
	mana = skill->difficulty * skill->min_mana;

    if (mana <= 0) {
	send_to_char("That spell is messed up. You cannot imprint it, please contact an imp.\n\r", ch);
	return;
    }

    if (!IS_NPC(ch) && ch->mana < mana) {
	send_to_char("You don't have enough mana.\n\r", ch);
	return;
    }

    learned = get_learned_skill(ch, skill);
    if (learned == NULL
	    || number_percent() > learned->percent) {
	send_to_char("You lost your concentration.\n\r", ch);

	ch->mana -= mana / 2;
	return;
    }

    /* executing the imprinting process */
    ch->mana -= mana;
    obj->value[sp_slot] = skill->sn;

    /* Making it successively harder to pack more spells into potions or
     * scrolls - JH */
    switch (sp_slot) {
	default:
	    log_bug("sp_slot has more than %d spells.", sp_slot);
	    return;
	case 1:
	    if (number_percent() > 95) {
		printf_to_char(ch, "The magic enchantment has failed --- the %s vanishes.\n\r", item_type_name(obj));
		extract_obj(obj);
		return;
	    }
	    break;
	case 2:
	    if (number_percent() > 60) {
		printf_to_char(ch, "The magic enchantment has failed --- the %s vanishes.\n\r", item_type_name(obj));
		extract_obj(obj);
		return;
	    }
	    break;
	case 3:
	    if (number_percent() > 35) {
		printf_to_char(ch, "The magic enchantment has failed --- the %s vanishes.\n\r", item_type_name(obj));
		extract_obj(obj);
		return;
	    }
	    break;
    }


    /* labeling the item */
    free_string(obj->short_descr);
    sprintf(short_descr, "a %s of ", item_type_name(obj));
    sprintf(name, "%s %s ", object_name_get(obj), item_type_name(obj));

    for (iter = 1; iter <= sp_slot; iter++) {
	if (obj->value[iter] != -1) {
	    strcat(short_descr, skill->name);
	    if (iter != sp_slot)
		strcat(short_descr, ", ");
	    else
		strcat(short_descr, "");

	    if ((skill_retro = resolve_skill_sn((int)obj->value[iter])) != NULL) {
		strcat(name, skill_retro->name);
		if (iter != sp_slot)
		    strcat(name, " ");
		else
		    strcat(name, "");
	    }
	}
    }

    obj->short_descr = str_dup(short_descr);
    object_name_set(obj, name);

    sprintf(buf, "You have imbued a new spell to the %s.\n\r", item_type_name(obj));
    send_to_char(buf, ch);
}



/***************************************************************************
 *	do_brew
 ***************************************************************************/
void do_brew(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    SKILL *skill;
    LEARNED *learned_brew;
    LEARNED *learned_spell;
    char arg[MAX_INPUT_LENGTH];
    int slots;
    int modifier;
    int chance;
    int level;

    learned_brew = NULL;
    if (IS_NPC(ch)) {
	send_to_char("You do not know how to brew potions.\n\r", ch);
	return;
    }

    if ((learned_brew = get_learned(ch, "brew")) == NULL) {
	send_to_char("You do not know how to brew potions.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Brew what spell?\n\r", ch);
	return;
    }

    /* Do we have a vial to brew potions? */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
	if (obj->item_type == ITEM_POTION
		&& obj->wear_loc == WEAR_HOLD)
	    break;
    }

    if (!obj) {
	send_to_char("You are not holding a vial.\n\r", ch);
	return;
    }

    if ((skill = skill_lookup(arg)) == NULL
	    || skill->spells == NULL) {
	send_to_char("That spell does not exist.\n\r", ch);
	return;
    }

    if ((learned_spell = get_learned_skill(ch, skill)) == NULL) {
	send_to_char("You don't know any spells by that name.\n\r", ch);
	return;
    }

    if (IS_SET(skill->flags, SPELL_NOBREW)) {
	send_to_char("You cannot brew that spell.\n\r", ch);
	return;
    }

    obj->level = ch->level;

    slots = count_slots(obj);
    chance = (learned_brew == NULL) ? 75 : learned_brew->percent;
    switch (slots) {
	case (1):
	    break;
	case (2):
	    chance -= 5;
	    break;
	case (3):
	    chance -= 15;
	    break;
	default:
	    act("$p cannot contain any more spells.", ch, obj, NULL, TO_CHAR);
	    return;
    }

    act("$n begins preparing a potion.", ch, obj, NULL, TO_ROOM);
    if (!IS_NPC(ch))
	WAIT_STATE(ch, learned_brew->skill->wait);

    if (!IS_NPC(ch) && number_percent() > chance) {
	act("$p bursts in flames!", ch, obj, NULL, TO_CHAR);
	act("$p bursts in flames!", ch, obj, NULL, TO_ROOM);
	spell_fireball(skill_lookup("fireball"), LEVEL_HERO - 1, ch, ch, TARGET_CHAR, "");

	extract_obj(obj);
	if (!IS_NPC(ch))
	    check_improve(ch, skill, false, 6);
	return;
    }


    modifier = learned_brew->percent * 3 / 100;
    chance = number_range(0, 10) + modifier;

    switch (chance) {
	case (0):
	case (1):
	case (2):
	case (3):
	case (4):
	    level = (int)(ch->level / 2);
	    break;
	case (5):
	case (6):
	case (7):
	case (8):
	case (9):
	    level = (int)((ch->level * 3) / 4);
	    break;
	default:
	    level = ch->level;
    }

    if (obj->level >= (int)(ch->level / 2))
	obj->value[0] = UMIN(obj->level, level);
    else
	obj->value[0] = level;

    if (!IS_NPC(ch) && learned_brew != NULL)
	check_improve(ch, learned_brew->skill, true, 6);

    spell_imprint(learned_spell->skill, ch->level, ch, obj);
}


/***************************************************************************
 *	do_scribe
 ***************************************************************************/
void do_scribe(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    SKILL *skill;
    LEARNED *learned_scribe;
    LEARNED *learned_spell;
    char arg[MAX_INPUT_LENGTH];
    int slots;
    int modifier;
    int chance;
    int level;


    if (IS_NPC(ch)) {
	send_to_char("You do not know how to scribe scrolls.\n\r", ch);
	return;
    }

    if ((learned_scribe = get_learned(ch, "scribe")) == NULL) {
	send_to_char("You do not know how to scribe scrolls.\n\r", ch);
	return;
    }


    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Scribe what spell?\n\r", ch);
	return;
    }

    /* Do we have a parchment to scribe spells? */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
	if (obj->item_type == ITEM_SCROLL
		&& obj->wear_loc == WEAR_HOLD)
	    break;
    }

    if (!obj) {
	send_to_char("You are not holding a parchment.\n\r", ch);
	return;
    }


    if ((skill = skill_lookup(arg)) == NULL
	    || skill->spells == NULL) {
	send_to_char("That spell does not exist.\n\r", ch);
	return;
    }

    if ((learned_spell = get_learned_skill(ch, skill)) == NULL) {
	send_to_char("You don't know any spells by that name.\n\r", ch);
	return;
    }


    if (IS_SET(skill->flags, SPELL_NOSCRIBE)) {
	send_to_char("You cannot scribe that spell.\n\r", ch);
	return;
    }

    obj->level = ch->level;
    slots = count_slots(obj);
    chance = learned_scribe->percent;
    switch (slots) {
	case (1):
	    break;
	case (2):
	    chance -= 5;
	    break;
	case (3):
	    chance -= 15;
	    break;
	default:
	    act("$p cannot contain any more spells.", ch, obj, NULL, TO_CHAR);
	    return;
    }

    act("$n begins writing a scroll.", ch, obj, NULL, TO_ROOM);
    WAIT_STATE(ch, learned_scribe->skill->wait);

    /* Check the skill percentage, fcn(int,wis,skill) */
    if (!IS_NPC(ch) && number_percent() > chance) {
	act("$p bursts in flames!", ch, obj, NULL, TO_CHAR);
	act("$p bursts in flames!", ch, obj, NULL, TO_ROOM);
	spell_fireball(skill_lookup("fireball"), LEVEL_HERO - 1, ch, ch, TARGET_CHAR, "");

	extract_obj(obj);
	check_improve(ch, learned_scribe->skill, false, 6);
	return;
    }

    /* basically, making scrolls more potent than potions; also, scrolls
     * are not limited in the choice of spells, i.e. scroll of enchant weapon
     * has no analogs in potion forms --- JH */

    modifier = learned_scribe->percent * 3 / 100;
    chance = number_range(0, 10) + modifier;

    switch (chance) {
	case (0):
	case (1):
	case (2):
	case (3):
	case (4):
	    level = ch->level / 2;
	    break;
	case (5):
	case (6):
	case (7):
	case (8):
	case (9):
	    level = (ch->level * 3) / 4;
	    break;
	default:
	    level = ch->level;
    }

    if (obj->value[0] >= ch->level / 2)
	obj->value[0] = UMIN(obj->value[0], level);
    else
	obj->value[0] = level;

    obj->level = (int)(ch->level / 2);

    check_improve(ch, learned_scribe->skill, true, 6);
    spell_imprint(learned_spell->skill, ch->level, ch, obj);
}


/***************************************************************************
 *	count_slots
 ***************************************************************************/
int count_slots(GAMEOBJECT *obj)
{
    SKILL *skill;
    int slots;
    int iter;

    for (slots = iter = 1; iter < 4; iter++)
	if ((skill = resolve_skill_sn((int)obj->value[iter])) != NULL)
	    slots++;

    return slots;
}
