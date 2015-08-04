#include <stdlib.h>
#include <math.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "sysinternals.h"

extern int parse_int(char *test);

/***************************************************************************
* Repair code - 08-09-2002
***************************************************************************/
static bool is_negative_affect(AFFECT_DATA * paf);
static unsigned int repair_cost(OBJ_DATA * obj, char *stat);
static void enchant_item(OBJ_DATA * obj);

/***************************************************************************
*	item_affect_stats
***************************************************************************/
static struct item_affect {
	char *	stat;
	char *	desc;
	long	flag;
	bool	is_negative;
	long	positive_cost;
	long	repair_cost;
}
item_affect_stats[] =
{
	{ "str",     "Strength",	 APPLY_STR,	     FALSE, 15000, 30000 },
	{ "dex",     "Dexterity",	 APPLY_DEX,	     FALSE, 15000, 30000 },
	{ "int",     "Intelligence",	 APPLY_INT,	     FALSE, 15000, 30000 },
	{ "wis",     "Wisdom",		 APPLY_WIS,	     FALSE, 15000, 30000 },
	{ "con",     "Constitution",	 APPLY_CON,	     FALSE, 15000, 30000 },
	{ "mana",    "Mana",		 APPLY_MANA,	     FALSE, 4000,  7000	 },
	{ "hp",	     "Hit Points",	 APPLY_HIT,	     FALSE, 4000,  7000	 },
	{ "moves",   "Movement",	 APPLY_MOVE,	     FALSE, 1000,  2000	 },
	{ "ac",	     "Armor Class",	 APPLY_AC,	     TRUE,  1000,  1000	 },
	{ "damroll", "Damage Roll",	 APPLY_DAMROLL,	     FALSE, 40000, 80000 },
	{ "hitroll", "Hit Roll",	 APPLY_HITROLL,	     FALSE, 30000, 50000 },
	{ "saves",   "Saves vs. Spells", APPLY_SAVING_SPELL, TRUE,  40000, 80000 },
	{ "",	     "",		 -1,		     FALSE, 0,	   0	 }
};



/***************************************************************************
*	do_enchant
*
*	put an enchantment an item
***************************************************************************/
void do_enchant(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	AFFECT_DATA af;
	char item[MIL];
	char affect[MIL];
	char mod[MIL];
	int idx;
	int modifier = 0;
	char *affect_name = NULL;
	int apply_to;

	DENY_NPC(ch);

	argument = one_argument(argument, item);
	argument = one_argument(argument, affect);
	argument = one_argument(argument, mod);

	if (item[0] == '\0' || affect[0] == '\0' || item[0] == '?' || !str_prefix(item, "help")) {
		send_to_char("Enchant <item> <affect> [modifier]\n\r", ch);

		/* print the valid affects list */
		send_to_char("\n\rValid Affects:\n\r\t", ch);
		for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
			printf_to_char(ch, "%s ", item_affect_stats[idx].stat);
			if (idx != 0 && (idx % 6) == 0)
				send_to_char("\n\r\t", ch);
		}
		if (idx % 6 == 0)
			send_to_char("\n\r", ch);

		send_to_char("\n\r", ch);

		if (get_trust(ch) >= MAX_LEVEL) {
			send_to_char("\n\rValid Spell Affects:\n\r\t", ch);
			for (idx = 0; affect_flags[idx].name != NULL; idx++) {
				printf_to_char(ch, "%-15.15s ", affect_flags[idx].name);
				if (idx != 0 && (idx % 3) == 0)
					send_to_char("\n\r\t", ch);
			}

			if (idx % 6 == 0)
				send_to_char("\n\r", ch);
			send_to_char("\n\r", ch);
		}
		return;
	}

	/* get the object we are enchanting */
	if ((obj = get_obj_carry(ch, item)) == NULL) {
		send_to_char("You don't have that item.\n\r", ch);
		return;
	}

	/* get the apply to location */
	apply_to = -1;
	for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
		if (!str_prefix(affect, item_affect_stats[idx].stat)) {
			apply_to = (int)item_affect_stats[idx].flag;
			affect_name = item_affect_stats[idx].stat;
			break;
		}
	}

	if (apply_to >= 0) {
		/* get the modifier value */
		if (mod[0] != '\0')
			modifier = (mod[0] != '\0') ? parse_int(mod) : ch->level;

		/* make sure the item is listed as enchanted */
		enchant_item(obj);

		af.level = ch->level;
		af.duration = -1;
		af.bitvector = 0;
		af.type = -1;
		af.location = apply_to;
		af.modifier = modifier;
		affect_to_obj(obj, &af);

		printf_to_char(ch, "Added affect: %s by %d.\n\r", affect_name, modifier);
		return;
	} else {
		for (idx = 0; affect_flags[idx].name != NULL; idx++) {
			if (!str_prefix(affect, affect_flags[idx].name)) {
				apply_to = (int)affect_flags[idx].bit;
				affect_name = affect_flags[idx].name;
				break;
			}
		}

		if (apply_to >= 0) {
			if (get_trust(ch) < MAX_LEVEL) {
				send_to_char("You are not high enough level to add that affect.\n\r", ch);
				return;
			}

			/* make sure the item is enchanted */
			enchant_item(obj);

			af.level = ch->level;
			af.where = TO_AFFECTS;
			af.duration = -1;
			af.bitvector = (long)apply_to;
			af.type = -1;
			af.location = -1;
			af.modifier = 0;
			affect_to_obj(obj, &af);

			printf_to_char(ch, "Added affect: %s.\n\r", affect_name);
			return;
		}
	}

	/* we did not find a valid affect - print help */
	send_to_char("Invalid Affect.\n\r\n\r", ch);
	do_enchant(ch, "");
}


/***************************************************************************
*	do_disenchant
*
*	remove an enchantment from an item
***************************************************************************/
void do_disenchant(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
	char item[MIL];
	char affect[MIL];
	int number;
	int count;
	int apply_to;
	char *affect_name = NULL;
	int idx;

	DENY_NPC(ch);

	argument = one_argument(argument, item);

	if (item[0] == '\0') {
		send_to_char("Disenchant <item> [[<number>.]<affect>|negative|all]\n\r", ch);

		/* print the valid affects list */
		send_to_char("\n\rValid Affects:\n\r\t", ch);
		for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
			printf_to_char(ch, "%s ", item_affect_stats[idx].stat);
			if (idx != 0 && (idx % 6) == 0)
				send_to_char("\n\r\t", ch);
		}
		send_to_char("\n\r", ch);
		return;
	}

	if ((obj = get_obj_carry(ch, item)) == NULL) {
		send_to_char("You don't have that item.\n\r", ch);
		return;
	}

	/* make sure the item is enchanted */

	enchant_item(obj);
	/* we have no futher arguments or the argument is
	 * "all" then remove everything */
	if (argument[0] == '\0' || !str_prefix(argument, "all")) {
		for (paf = obj->affected; paf != NULL; paf = paf_next) {
			paf_next = paf->next;
			free_affect(paf);
		}

		obj->affected = NULL;
		obj->extra_flags = 0;
		obj->enchanted = TRUE;
		send_to_char("This object now has no enchantments upon it.\n\r", ch);
		return;
	}

	/* remove all negative affects */
	if (!str_prefix(argument, "negative")) {
		for (paf = obj->affected; paf != NULL; paf = paf_next) {
			/* we may be deleting the affect, so use a temp
			 * variable for the next item in the list */
			paf_next = paf->next;
			if (is_negative_affect(paf))
				affect_remove_obj(obj, paf);
		}

		send_to_char("All negative enchantment have been removed.\n\r", ch);
		return;
	}

	/* assume we have disenchant <object> [[number].]<affect> */
	number = number_argument(argument, affect);

	/* get the apply to location */
	apply_to = -1;
	for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
		if (!str_prefix(affect, item_affect_stats[idx].stat)) {
			apply_to = (int)item_affect_stats[idx].flag;
			affect_name = item_affect_stats[idx].stat;
			break;
		}
	}

	if (apply_to >= 0) {
		count = 1;
		for (paf = obj->affected; paf != NULL; paf = paf_next) {
			paf_next = paf->next;
			if ((paf->location == apply_to || paf->type == apply_to)
			    && number == count++) {
				long modifier;

				modifier = paf->modifier;

				/* remove the affect */
				affect_remove_obj(obj, paf);

				/* generate a message */
				printf_to_char(ch, "Affect removed: %s by %d.\n\r", affect_name, modifier);
				return;
			}
		}
	} else {
		for (idx = 0; affect_flags[idx].name != NULL; idx++) {
			if (!str_prefix(affect, affect_flags[idx].name)) {
				apply_to = (int)affect_flags[idx].bit;
				affect_name = affect_flags[idx].name;
				break;
			}
		}

		count = 1;
		for (paf = obj->affected; paf != NULL; paf = paf_next) {
			paf_next = paf->next;
			if (paf->bitvector == (long)apply_to
			    && number == count++) {
				/* remove the affect */
				affect_remove_obj(obj, paf);

				/* generate a message */
				printf_to_char(ch, "Affect removed: %s.\n\r", affect_name);
				return;
			}
		}
	}

	/* if we got to here, we couldnt find the affect so generate help */
	send_to_char("Affect could not be found.\n\r\n\r", ch);
	do_disenchant(ch, "");
	return;
}



/***************************************************************************
*	is_negative_affect
*
*	check to see if the affect we are looking at is a good
*	affect or a bad affect
***************************************************************************/
bool is_negative_affect(AFFECT_DATA *paf)
{
	int idx;

	for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
		if (item_affect_stats[idx].flag == paf->location
		    || item_affect_stats[idx].flag == paf->type)
			/* we found the affect, if it is a negative affect and the modifier is less than 1
			 * or if it is a positive affect and the modifier is greater than 1, return true,
			 * otherwise return false */
			return (item_affect_stats[idx].is_negative && paf->modifier > 0) || (!item_affect_stats[idx].is_negative && paf->modifier < 0);

	}

	return FALSE;
}


/***************************************************************************
*	do_repair
*
*	repair negative affects on an item
***************************************************************************/
void do_repair(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	AFFECT_DATA *paf;
	SKILL *skill_haggle;
	char cmd[MSL];
	char stat[MSL];
	int idx;
	unsigned int cost;
	int roll;

	/* cmd should either be a command or the name of the object */
	argument = one_argument(argument, cmd);

	/* check for a command first */
	if (cmd[0] == '\0' || cmd[0] == '?' || !str_cmp(cmd, "help")) {
		do_help(ch, "blacksmith_repair");
		return;
	}

	for (mob = ch->in_room->people; mob; mob = mob->next_in_room) {
		if (IS_NPC(mob)
		    && IS_SET(mob->act, ACT_IS_BLACKSMITH))
			break;
	}

	if (mob == NULL) {
		send_to_char("No one here is skilled enough to do that.  You need to find a Blacksmith\n\r", ch);
		return;
	}


	if (!str_prefix(cmd, "list")) {
		send_to_char("I can repair the following stats:\n\r", ch);
		send_to_char("stat name       description\n\r", ch);
		send_to_char("=============================================\n\r", ch);

		for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++)
			printf_to_char(ch, "   `#%-9.10s`` - %s\n\r", item_affect_stats[idx].stat, item_affect_stats[idx].desc);
		return;
	}

	if (!str_prefix(cmd, "estimate")) {
		char item[MSL];
		argument = one_argument(argument, item);

		if (item[0] == '\0' || ((obj = get_obj_carry(ch, item)) == NULL)) {
			printf_to_char(ch, "%s says '`PYou don't have that item.`7'\n\r", mob->short_descr);
			return;
		}

		/* we need to get the stat we are looking for and any 2., 3. etc. */
		argument = one_argument(argument, stat);
		if (stat[0] == '\0') {
			printf_to_char(ch, "%s says '`PI dont know what you mean.`7'\n\r", mob->short_descr);
			return;
		}


		/* okay...we have an object, a stat name and a number
		 * start calculating */
		cost = repair_cost(obj, stat);
		if (cost == 0) {
			printf_to_char(ch, "%s says '`PThat affect doesnt exist on your item.`7'\n\r", mob->short_descr);
			return;
		}

		printf_to_char(ch, "%s says '`PIt will cost %ld gold to remove that stat from %s.`7\n\r", mob->short_descr, cost, obj->short_descr);
		return;
	}


	/* okay....assume that they arent looking for help, list, or estimate
	 * so try to find an item that the character has in the inventory */
	if ((obj = get_obj_carry(ch, cmd)) == NULL) {
		printf_to_char(ch, "%s says '`PYou don't have that item.`7'\n\r", mob->short_descr);
		return;
	}

	/* we have a valid object...get the stat we want to fix
	 *  we need to get the stat we are looking for and any 2., 3. etc */
	argument = one_argument(argument, stat);
	if (stat[0] == '\0') {
		printf_to_char(ch, "%s says '`PI dont know what you mean.`7'\n\r", mob->short_descr);
		return;
	}


	/* okay...we have an object, a stat name and a number
	 * start calculating */
	cost = repair_cost(obj, stat);
	if (cost == 0) {
		printf_to_char(ch, "%s says '`PThat affect doesnt exist on your item.`7'\n\r", mob->short_descr);
		return;
	}

	/* haggle */
	roll = number_percent();
	if ((skill_haggle = skill_lookup("haggle")) != NULL
	    && roll < get_learned_percent(ch, skill_haggle)) {
		cost -= cost / 2 * roll / 100;
		printf_to_char(ch, "You haggle the price down to %d `#gold``.\n\r", cost);
		check_improve(ch, skill_haggle, TRUE, 4);
	}

	if (cost > ch->gold) {
		printf_to_char(ch, "%s says, '`PYou do not have enough gold for my services.`7'\n\r", mob->short_descr);
		return;
	}

	/*
	 * everything is together now right?
	 * we have an object, an affect name and number
	 * the player has enough money...remove the affect?
	 *
	 * go through the affect list on the object index
	 * can we remove it if it is on the index?  need to test this, if not
	 * it affects how we handle the other function too
	 */
	if (!obj->enchanted)
		enchant_item(obj);

	/* so we have an enchanted weapon, find the affect
	 * in the list and strip it */
	if (obj->enchanted) {
		for (paf = obj->affected; paf != NULL; paf = paf->next) {
			for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
				if (paf->location == (int)item_affect_stats[idx].flag
				    && !str_prefix(stat, item_affect_stats[idx].stat)) {
					if ((item_affect_stats[idx].is_negative && paf->modifier > 0)
					    || (!item_affect_stats[idx].is_negative && paf->modifier < 0)) {
						affect_remove_obj(obj, paf);
						printf_to_char(ch, "%s tinkers with your %s, there is a huge `#flash``, he grins and hands it back to you.\n\r", mob->short_descr, obj->short_descr);
						printf_to_char(ch, "\n\r%s says, '`PThanks for doing business with me.`7'\n\r", mob->short_descr);
						ch->gold -= cost;
						return;
					}
				}
			}
		}

		printf_to_char(ch, "%s says '`PThat affect doesnt exist on that item.`7'\n\r", mob->short_descr);
	}

	return;
}



/***************************************************************************
*	repair_cost
*
*	get the cost to repair an item
***************************************************************************/
unsigned int repair_cost(OBJ_DATA *obj, char *stat)
{
	AFFECT_DATA *paf;
	unsigned int cost = 0;
	int idx;
	bool found = FALSE;

	if (!obj->enchanted) {
		for (paf = obj->obj_idx->affected; paf != NULL; paf = paf->next) {
			for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
				if (paf->location == (int)item_affect_stats[idx].flag) {
					if ((item_affect_stats[idx].is_negative && paf->modifier < 0)
					    || (!item_affect_stats[idx].is_negative && paf->modifier > 0))
						cost += abs((int)(paf->modifier * item_affect_stats[idx].positive_cost));

					if (!str_prefix(stat, item_affect_stats[idx].stat)) {
						if ((item_affect_stats[idx].is_negative && paf->modifier > 0)
						    || (!item_affect_stats[idx].is_negative && paf->modifier < 0)) {
							if (!found) {
								cost += abs((int)(paf->modifier * item_affect_stats[idx].repair_cost));
								found = TRUE;
							}
						}
					}

					break;
				}
			}
		}
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next) {
		for (idx = 0; item_affect_stats[idx].stat[0] != '\0'; idx++) {
			if (paf->location == (int)item_affect_stats[idx].flag) {
				if ((item_affect_stats[idx].is_negative && paf->modifier < 0)
				    || (!item_affect_stats[idx].is_negative && paf->modifier > 0))
					cost += abs((int)(paf->modifier * item_affect_stats[idx].positive_cost));
				if (!str_prefix(stat, item_affect_stats[idx].stat)) {
					if ((item_affect_stats[idx].is_negative && paf->modifier > 0)
					    || (!item_affect_stats[idx].is_negative && paf->modifier < 0)) {
						if (!found) {
							cost += abs((int)(paf->modifier * item_affect_stats[idx].repair_cost));
							found = TRUE;
						}
					}
				}

				break;
			}
		}
	}

	if (!found)
		cost = 0;

	return cost;
}



/***************************************************************************
*	enchant_item
*
*	take item affects from an object index and copy them to the
*	affects specific to the item
***************************************************************************/
void enchant_item(OBJ_DATA *obj)
{
	AFFECT_DATA *copy;
	AFFECT_DATA *copy_next;
	AFFECT_DATA *new_af;

	if (!obj->enchanted) {
		for (copy = obj->obj_idx->affected; copy != NULL; copy = copy_next) {
			copy_next = copy->next;

			new_af = new_affect();

			new_af->where = copy->where;
			new_af->type = UMAX(0, copy->type);
			new_af->level = copy->level;
			new_af->duration = copy->duration;
			new_af->location = copy->location;
			new_af->modifier = copy->modifier;
			new_af->bitvector = copy->bitvector;

			new_af->next = obj->affected;
			obj->affected = new_af;
		}

		obj->enchanted = TRUE;
	}
}
