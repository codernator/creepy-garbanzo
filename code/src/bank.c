/**************************************************************************
 *   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
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
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
 * Oct 2009                                                                 *
 * Completely rewritten by Brandon Griffin (aka Codernator) to remove       *
 * c&p coding suckage.  Absorbed atm.c                                      *
 ****************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"


extern OBJ_DATA *get_object_by_itemtype_and_room(int item_type, ROOM_INDEX_DATA *room, CHAR_DATA *ch);
extern void sick_harvey_proctor(CHAR_DATA *ch, enum e_harvey_proctor_is, const char *message);
extern int number_range(int from, int to);


static void complete_transaction(CHAR_DATA *ch, bool withdraw, unsigned int amount, unsigned int *purse, unsigned int *drawer, const char *tender, OBJ_DATA *atm);
static bool check_for_bank(CHAR_DATA *ch, /*@out@*/ OBJ_DATA **atm);
static void evaluate_transaction(CHAR_DATA *ch, bool withdraw, char *arg_amount, char *arg_tender);
static void find_money(CHAR_DATA *ch);
static void give_receipt(CHAR_DATA *ch, unsigned int amount, const char *action, const char *tender, OBJ_DATA *atm);
static void report_balance(CHAR_DATA *ch);


void do_atm_withdraw(CHAR_DATA *ch, char *argument)
{
	do_withdraw(ch, argument);
}

void do_atm_balance(CHAR_DATA *ch, char *argument)
{
	do_balance(ch, argument);
}

void do_atm_deposit(CHAR_DATA *ch, char *argument)
{
	do_deposit(ch, argument);
}


void do_balance(CHAR_DATA *ch, char *vo)
{
	OBJ_DATA *atm;

	DENY_NPC(ch)

	if (check_for_bank(ch, &atm))
		report_balance(ch);
}

void do_deposit(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	DENY_NPC(ch)

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	evaluate_transaction(ch, FALSE, arg1, arg2);
}

void do_withdraw(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	DENY_NPC(ch)

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	evaluate_transaction(ch, TRUE, arg1, arg2);
}

void evaluate_transaction(CHAR_DATA *ch, bool withdraw, char *arg_amount, char *arg_tender)
{
	char buf[MIL];
	OBJ_DATA *atm = NULL;

	if (check_for_bank(ch, &atm)) {
		long parsed_arg = 0;
		unsigned int amount = 0;

		find_money(ch);

		if (arg_amount[0] == '\0' || !is_number(arg_amount)) {
			snprintf(buf, MIL, "Try %s <amount> <gold or silver>.", withdraw ? "withdraw" : "deposit");
			sick_harvey_proctor(ch, hp_irritated, buf);
			return;
		}

		parsed_arg = atol(arg_amount);
		if (parsed_arg <= 0) {
			snprintf(buf, MIL, "If you want to %s money, use the %s command, Jackass.",
				 withdraw ? "deposit" : "withdraw",
				 withdraw ? "deposit" : "withdraw");
			sick_harvey_proctor(ch, hp_irritated, buf);
			return;
		}

		amount = (unsigned int)parsed_arg;

		if (arg_tender[0] == '\0' || str_cmp(arg_tender, "gold") == 0)
			complete_transaction(ch, withdraw, amount, &ch->gold, &ch->pcdata->gold_in_bank, "gold", atm);
		else
		if (str_cmp(arg_tender, "silver") == 0)
			complete_transaction(ch, withdraw, amount, &ch->silver, &ch->pcdata->silver_in_bank, "silver", atm);
		else
			sick_harvey_proctor(ch, hp_pissed_off, "Look, jackass!  We only trade in gold and silver here!");
	}
}

void find_money(CHAR_DATA *ch)
{
	int j;

	if ((ch->pcdata->last_bank - current_time) > 86400) {
		for (j = (int)((ch->pcdata->last_bank - current_time) / 86400); j != 0; j--) {
			ch->pcdata->gold_in_bank += ch->pcdata->gold_in_bank / 50;
			ch->pcdata->silver_in_bank += ch->pcdata->silver_in_bank / 50;
		}
		ch->pcdata->last_bank = time(NULL);
	}

	/* HACK!!! */
	if ((ch->pcdata->gold_in_bank > 1000000u))
		ch->pcdata->gold_in_bank = 1000000u;

	if ((ch->pcdata->silver_in_bank > 10000000u))
		ch->pcdata->silver_in_bank = 10000000u;
}

bool check_for_bank(CHAR_DATA *ch, /*@out@*/ OBJ_DATA **atm)
{
	OBJ_DATA *temp_atm = NULL;

	if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) {
		temp_atm = get_object_by_itemtype_and_room(ITEM_ATM, ch->in_room, ch);
		*atm = temp_atm;
		if (temp_atm == NULL) {
			OBJ_DATA *shit = NULL;

			if (number_range(0, 1000) == 1)
				shit = create_object(get_obj_index(OBJ_VNUM_SHIT), 0);

			send_to_char("You shove your ATM card into a random ass and withdraw some shit.\n\r", ch);
			if (shit != NULL)
				obj_to_char(shit, ch);

			return FALSE;
		} else {
			return TRUE;
		}
	} else {
		*atm = NULL;
		return TRUE;
	}
}

void complete_transaction(CHAR_DATA *ch,
			  bool withdraw,
			  unsigned int amount,
			  unsigned int *purse,
			  unsigned int *drawer, const char *tender,
			  OBJ_DATA *atm)
{
	char buf[MIL];

	if ((withdraw && *drawer < amount) || (!withdraw && *purse < amount)) {
		snprintf(buf, MIL, "You don't have that much %s %s!", tender, withdraw ? "in your account" : "on you");
		sick_harvey_proctor(ch, hp_irritated, buf);
	} else {
		if (withdraw) {
			*drawer -= amount;
			*purse += amount;
		} else {
			*drawer += amount;
			*purse -= amount;
		}

		report_balance(ch);

		if (atm != NULL)
			give_receipt(ch, amount, withdraw ? "Withdrawal" : "Deposit", tender, atm);
	}
}


void report_balance(CHAR_DATA *ch)
{
	char buf[MAX_INPUT_LENGTH];

	find_money(ch);
	sprintf(buf, "You have %u gold and %u silver pieces in the bank.\n\r",
		ch->pcdata->gold_in_bank, ch->pcdata->silver_in_bank);
	send_to_char(buf, ch);
}

void give_receipt(CHAR_DATA *ch, unsigned int amount, const char *action, const char *tender, OBJ_DATA *atm)
{
	char buf[MAX_INPUT_LENGTH];
	OBJ_DATA *receipt = create_object(get_obj_index(OBJ_VNUM_RECEIPT), 0);

	sprintf(buf, "%s deposits some %s.", ch->name, tender);
	act(buf, ch, NULL, NULL, TO_ROOM);

	sprintf(buf, "%s gives %s a receipt.", atm->short_descr, ch->name);
	act(buf, ch, NULL, NULL, TO_ROOM);

	send_to_char("The atm receipt materializes in your pocket.\n\r", ch);

	sprintf(buf,
		"obj receipt ed receipt ``%s - %s: `B%u %s  ``Balance: `#%ugp `&%usp``.",
		ch->name,
		action, amount, tender,
		ch->pcdata->gold_in_bank,
		ch->pcdata->silver_in_bank);
	do_string(ch, buf);
	obj_to_char(receipt, ch);
}
