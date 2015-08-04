#include <stdio.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "recycle.h"
#include "sysinternals.h"


extern CHAR_DATA * find_keeper(CHAR_DATA * ch);
extern int get_cost(CHAR_DATA * keeper, OBJ_DATA * obj, bool fbuy);
extern OBJ_DATA * get_obj_keeper(CHAR_DATA * ch, CHAR_DATA * keeper, char *argument);
extern void obj_to_keeper(OBJ_DATA * obj, CHAR_DATA * ch);


/***************************************************************************
*	do_estimate
*
*	estimate the cost of a particular flag for a weapon
***************************************************************************/
void do_estimate(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg1[MIL];
	char arg2[MIL];
	unsigned int cost;


	for (mob = ch->in_room->people; mob; mob = mob->next_in_room) {
		if (IS_NPC(mob)
		    && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;
	}

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		act("$N says '`PEstimate what?`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (!str_prefix(arg1, "repair")) {
		char tmp[MSL];

		sprintf(tmp, "estimate %s %s", arg2, argument);

		do_repair(ch, tmp);
		return;
	}

	if ((obj = get_obj_carry(ch, arg2)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI only know weapons.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (!str_prefix(arg1, "poison")) {
		cost = (unsigned int)(obj->value[4] * 4000);
		printf_to_char(ch, "%s says '`PIt will cost you about %d gold to poison that weapon.`7'\n\r", mob->short_descr, cost);
		return;
	}

	if (!str_prefix(arg1, "engulf")) {
		cost = (unsigned int)(obj->value[4] * 6000);
		printf_to_char(ch, "%s says '`PIt will cost you about %d gold to set that weapon on fire.`7'\n\r", mob->short_descr, cost);
		return;
	}

	if (!str_prefix(arg1, "electrify")) {
		cost = (unsigned int)(obj->value[4] * 5000);
		printf_to_char(ch, "%s says '`PIt will cost you about %d gold to electrify that weapon.`7'\n\r", mob->short_descr, cost);
		return;
	}

	if (!str_prefix(arg1, "energize")) {
		cost = (unsigned int)(obj->value[4] * 4000);
		printf_to_char(ch, "%s says '`PIt will cost you about %d gold to energize that weapon.`7'\n\r", mob->short_descr, cost);
		return;
	}

	if (!str_prefix(arg1, "sharpen")) {
		cost = (unsigned int)(obj->value[4] * 7000);
		printf_to_char(ch, "%s says '`PIt will cost you about %d gold to sharpen that weapon.`7'\n\r", mob->short_descr, cost);
		return;
	}

	if (!str_prefix(arg1, "chill")) {
		cost = (unsigned int)(obj->value[4] * 5000);
		printf_to_char(ch, "%s says '`PIt will cost you about %d gold to freeze that weapon.`7'\n\r", mob->short_descr, cost);
		return;
	}
}


/***************************************************************************
*	do_poison
*
*	add the poison affect to a weapon
***************************************************************************/
void do_poison(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg1[MIL];
	char arg2[MIL];
	unsigned int cost;


	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		act("$N says '`PI can poison an item for you.`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if ((obj = get_obj_carry(ch, arg1)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI will only poison a weapon`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	cost = (unsigned int)(obj->value[4] * 4000);

	if (cost > ch->gold) {
		act("$N says, '`PYou do not have enough gold for my services.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	ch->gold -= cost;
	mob->gold += cost;

	act("$N takes $p from $n.\n\r$N treats $p with poison, and then returns it to $n", ch, obj, mob, TO_ROOM);
	act("$N takes $p from you.\n\r$N treast $p with poison.\n\r", ch, obj, mob, TO_CHAR);

	SET_BIT(obj->value[4], WEAPON_POISON);
}


/***************************************************************************
*	do_engulf
*
*	set the weapon on fire
***************************************************************************/
void do_engulf(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg[MIL];
	unsigned int cost;


	for (mob = ch->in_room->people; mob; mob = mob->next_in_room) {
		if (IS_NPC(mob)
		    && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;
	}

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		act("$N says '`PI will set an item on fire for you.`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI will only set a weapon on fire`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	cost = (unsigned int)(obj->value[4] * 6000);

	if (cost > ch->gold) {
		act("$N says, '`PYou do not have enough gold for my services.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	ch->gold -= cost;
	mob->gold += cost;

	act("$N takes $p from $n.\n\r$N put $p in a pit of glowing coals, and then returns it to $n", ch, obj, mob, TO_ROOM);
	act("$N takes $p from you.\n\r$N sets $p ablaze.\n\r", ch, obj, mob, TO_CHAR);

	SET_BIT(obj->value[4], WEAPON_FLAMING);
}


/***************************************************************************
*	do_sharpen
*
*	add the sharp flag to a weapon
***************************************************************************/
void do_sharpen(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg[MIL];
	unsigned int cost;

	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		act("$N says '`PI will sharpen an item for you.`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI will only sharpen a weapon`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	cost = (unsigned int)(obj->value[4] * 5000);

	if (cost > ch->gold) {
		act("$N says, '`PYou do not have enough gold for my services.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	ch->gold -= cost;
	mob->gold += cost;

	act("$N takes $p from $n.\n\r$N sharpens $p, and then returns it to $n", ch, obj, mob, TO_ROOM);
	act("$N takes $p from you.\n\r$N sharpens $p.\n\r", ch, obj, mob, TO_CHAR);

	SET_BIT(obj->value[4], WEAPON_SHARP);
}

/***************************************************************************
*	do_chill
*
*	add a chill affect to the weapon
***************************************************************************/
void do_chill(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg[MIL];
	unsigned int cost;

	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		act("$N says '`PI will freeze an item for you.`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI will only freeze a weapon`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	cost = (unsigned int)(obj->value[4] * 5000);

	if (cost > ch->gold) {
		act("$N says, '`PYou do not have enough gold for my services.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	ch->gold -= cost;
	mob->gold += cost;

	act("$N takes $p from $n.\n\r$N puts $p in a box of ice, and then returns it to $n", ch, obj, mob, TO_ROOM);
	act("$N takes $p from you.\n\r$N freezes $p.\n\r", ch, obj, mob, TO_CHAR);

	SET_BIT(obj->value[4], WEAPON_FROST);
}


/***************************************************************************
*	do_electrify
*
*	add an electricity attack to the weapon
***************************************************************************/
void do_electrify(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg[MIL];
	unsigned int cost;


	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		act("$N says '`PI will electrify an item for you.`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI will only electrify a weapon`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	cost = (unsigned int)(obj->value[4] * 4000);

	if (cost > ch->gold) {
		act("$N says, '`PYou do not have enough gold for my services.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	ch->gold -= cost;
	mob->gold += cost;

	act("$N takes $p from $n.\n\r$N puts $p in a large lead chest for a few minutes, and then returns it to $n", ch, obj, mob, TO_ROOM);
	act("$N takes $p from you.\n\r$N prepares to electrify $p.\n\r", ch, obj, mob, TO_CHAR);

	SET_BIT(obj->value[4], WEAPON_SHOCKING);
}


/***************************************************************************
*	do_energize
*
*	make the weapon vampiric
***************************************************************************/
void do_energize(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg[MIL];
	unsigned int cost;


	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		act("$N says '`PWell?`7'\n\r", ch, NULL, mob, TO_CHAR);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$N says '`PYou don't have that item`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (obj->item_type != ITEM_WEAPON) {
		act("$N says '`PI will only do that to a weapon`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	cost = (unsigned int)(obj->value[4] * 4000);

	if (cost > ch->gold) {
		act("$N says, '`PYou do not have enough gold for my services.`7'", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	ch->gold -= cost;
	mob->gold += cost;

	act("$N takes $p from $n.\n\r$N chants over $p, and then returns it to $n", ch, obj, mob, TO_ROOM);
	act("$N takes $p from you.\n\r$N chants over $p.\n\r", ch, obj, mob, TO_CHAR);

	SET_BIT(obj->value[4], WEAPON_VAMPIRIC);
}
