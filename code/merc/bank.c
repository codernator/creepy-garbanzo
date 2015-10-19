/***************************************************************************
 * Oct 2009                                                                 *
 * Completely rewritten by Brandon Griffin (aka Codernator) to remove       *
 * c&p coding suckage.  Absorbed atm.c                                      *
 ****************************************************************************/
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include <stdio.h>
#include <string.h>


/** imports */
extern struct gameobject *get_object_by_itemtype_and_room(int item_type, struct room_index_data *room, struct char_data *ch);
extern void sick_harvey_proctor(struct char_data *ch, enum e_harvey_proctor_is, const char *message);


/** locals */
static void complete_transaction(struct char_data *ch, bool withdraw, unsigned int amount, unsigned int *purse, unsigned int *drawer, const char *tender, struct gameobject *atm);
static bool check_for_bank(struct char_data *ch, /*@out@*/ struct gameobject **atm);
static void evaluate_transaction(struct char_data *ch, bool withdraw, char *arg_amount, char *arg_tender);
static void find_money(struct char_data *ch);
static void give_receipt(struct char_data *ch, unsigned int amount, const char *action, const char *tender, struct gameobject *atm);
static void report_balance(struct char_data *ch);


void do_atm_withdraw(struct char_data *ch, const char *argument)
{
    do_withdraw(ch, argument);
}

void do_atm_balance(struct char_data *ch, const char *argument)
{
    do_balance(ch, argument);
}

void do_atm_deposit(struct char_data *ch, const char *argument)
{
    do_deposit(ch, argument);
}


void do_balance(struct char_data *ch, const char *vo)
{
    struct gameobject *atm;

    DENY_NPC(ch);

    if (check_for_bank(ch, &atm)) {
	report_balance(ch);
    }
}

void do_deposit(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    DENY_NPC(ch);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    evaluate_transaction(ch, false, arg1, arg2);
}

void do_withdraw(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    DENY_NPC(ch);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    evaluate_transaction(ch, true, arg1, arg2);
}

void evaluate_transaction(struct char_data *ch, bool withdraw, char *arg_amount, char *arg_tender)
{
    char buf[MAX_INPUT_LENGTH];
    struct gameobject *atm = NULL;

    if (check_for_bank(ch, &atm)) {
	long parsed_arg = 0;
	unsigned int amount = 0;

	find_money(ch);

	if (arg_amount[0] == '\0' || !is_number(arg_amount)) {
	    snprintf(buf, MAX_INPUT_LENGTH, "Try %s <amount> <gold or silver>.", withdraw ? "withdraw" : "deposit");
	    sick_harvey_proctor(ch, hp_irritated, buf);
	    return;
	}

	parsed_arg = parse_long(arg_amount);
	if (parsed_arg <= 0) {
	    snprintf(buf, MAX_INPUT_LENGTH, "If you want to %s money, use the %s command, Jackass.", withdraw ? "deposit" : "withdraw", withdraw ? "deposit" : "withdraw");
	    sick_harvey_proctor(ch, hp_irritated, buf);
	    return;
	}

	amount = (unsigned int)parsed_arg;

	if (arg_tender[0] == '\0' || str_cmp(arg_tender, "gold") == 0) {
	    complete_transaction(ch, withdraw, amount, &ch->gold, &ch->pcdata->gold_in_bank, "gold", atm);
	} else {
	    if (str_cmp(arg_tender, "silver") == 0) {
		complete_transaction(ch, withdraw, amount, &ch->silver, &ch->pcdata->silver_in_bank, "silver", atm);
	    } else {
		sick_harvey_proctor(ch, hp_pissed_off, "Look, jackass!  We only trade in gold and silver here!");
	    }
	}
    }
}

void find_money(struct char_data *ch)
{
    int j;

    if ((ch->pcdata->last_bank - globalSystemState.current_time) > 86400) {
	for (j = (int)((ch->pcdata->last_bank - globalSystemState.current_time) / 86400); j != 0; j--) {
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

bool check_for_bank(struct char_data *ch, /*@out@*/ struct gameobject **atm)
{
    struct gameobject *temp_atm = NULL;

    if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) {
	temp_atm = get_object_by_itemtype_and_room(ITEM_ATM, ch->in_room, ch);
	*atm = temp_atm;
	if (temp_atm == NULL) {
	    send_to_char("There is no ATM here.\n\r", ch);
	    return false;
	} else {
	    return true;
	}
    } else {
	*atm = NULL;
	return true;
    }
}

void complete_transaction(struct char_data *ch, bool withdraw, unsigned int amount, unsigned int *purse, unsigned int *drawer, const char *tender, struct gameobject *atm)
{
    char buf[MAX_INPUT_LENGTH];

    if ((withdraw && *drawer < amount) || (!withdraw && *purse < amount)) {
	snprintf(buf, MAX_INPUT_LENGTH, "You don't have that much %s %s!", tender, withdraw ? "in your account" : "on you");
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


void report_balance(struct char_data *ch)
{
    find_money(ch);
    printf_to_char(ch, "You have %u gold and %u silver pieces in the bank.\n\r", ch->pcdata->gold_in_bank, ch->pcdata->silver_in_bank);
}

void give_receipt(struct char_data *ch, unsigned int amount, const char *action, const char *tender, struct gameobject *atm)
{
    char buf[MAX_INPUT_LENGTH];

    sprintf(buf, "%s deposits some %s.", ch->name, tender);
    act(buf, ch, NULL, NULL, TO_ROOM);

    sprintf(buf, "%s gives %s a receipt.", atm->short_descr, ch->name);
    act(buf, ch, NULL, NULL, TO_ROOM);

    printf_to_char(ch, "``%s: `B%u %s  ``Balance: `#%ugp `&%usp``.", action, amount, tender, ch->pcdata->gold_in_bank, ch->pcdata->silver_in_bank);
}
