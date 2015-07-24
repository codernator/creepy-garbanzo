/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor(rtaylor@hypercube.org)                                 *
*       Gabrielle Taylor(gtaylor@hypercube.org)                            *
*       Brian Moore(zump@rom.org)                                          *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
 *      smoking routine added by Eo for Bad Trip - Finished 10/03/1996      *
 ****************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#if defined(WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "interp.h"
#include "magic.h"


/***************************************************************************
*	defines so i can remember what value indexes mean
***************************************************************************/
#define STASH_LEVEL             0
#define STASH_AMOUNT    1               /* the amount left in a bag or paraphenalia */
#define STASH_SIZE              2       /* the amount a bag or paraphenalia can hold */
#define STASH_STRENGTH  3               /* the potency of the weed */
#define STASH_EXTRA             4       /* extra properties (spells?) */


/***************************************************************************
*	do_pack_bowl
*
*	pack a bowl
***************************************************************************/
void do_pack_bowl(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *herb;
	OBJ_DATA *paraph;
	char arg[MIL];
	int amt;
	int opn;

	argument = one_argument(argument, arg);

	/* cant pack while fighting */
	if (ch->fighting != NULL) {
		send_to_char("You're too busy fighting!\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		send_to_char("Pack what?!  With what?!?\n\r", ch);
		return;
	}


	/* make sure that we are holding paraphenalia */
	if ((paraph = get_obj_wear(ch, arg)) == NULL
	    && (paraph = get_obj_carry(ch, arg)) == NULL) {
		send_to_char("You aren't holding that.\n\r", ch);
		return;
	}

	if (paraph->item_type != ITEM_PARAPH) {
		send_to_char("Are you crazy?!\n\r", ch);
		return;
	}

	/* find the weed object check the argument first */
	herb = NULL;
	if (argument[0] != '\0')
		herb = get_obj_carry(ch, argument);

	if (herb == NULL
	    && (herb = get_obj_carry(ch, "weed")) == NULL) {
		send_to_char("Pack it with what?!\n\r", ch);
		return;
	}

	if (herb->value[STASH_AMOUNT] <= 0) {
		send_to_char("No good - your sack is `8empty``!\n\r", ch);
		return;
	}

	if (herb->value[STASH_SIZE] <= 0) {
		send_to_char("Your stash seems to be depleted...\n\r", ch);
		return;
	}


	/* return if the bong is over 1/2 full */
	if (paraph->value[STASH_AMOUNT] >= (paraph->value[STASH_SIZE] / 2)) {
		send_to_char("The bowl still looks pretty loaded..\n\r", ch);
		return;
	}

	if (herb->value[STASH_EXTRA] > 0
	    && paraph->value[STASH_AMOUNT] > 0
	    && paraph->value[STASH_EXTRA] != herb->value[STASH_EXTRA]) {
		send_to_char("You cannot mix that weed what what is in your bong.\n\r", ch);
		return;
	}

	/* get the amount left in a bag */
	amt = (int)herb->value[STASH_AMOUNT];
	/* the open amount is equal to the size minus the amount */
	opn = (int)(paraph->value[STASH_SIZE] - paraph->value[STASH_AMOUNT]);
	if (amt <= opn) {
		/* we have more free space than what is left in the bag */
		paraph->value[STASH_AMOUNT] += amt;
		herb->value[STASH_AMOUNT] = 0;

		printf_to_char(ch, "You pack %s.\n\r", paraph->short_descr);

		act("$n packs $p.", ch, paraph, NULL, TO_ROOM);
		send_to_char("You've used up your stash!\n\r", ch);

		free_string(herb->short_descr);
		free_string(herb->description);

		herb->short_descr = str_dup("an empty plastic bag");
		herb->description = str_dup("An empty plastic bag lies here.");
	} else {
		paraph->value[STASH_AMOUNT] = paraph->value[STASH_SIZE];
		herb->value[STASH_AMOUNT] -= opn;

		printf_to_char(ch, "You pack %s full.\n\r", paraph->short_descr);
		act("$n packs $p full.", ch, paraph, NULL, TO_ROOM);
	}

	paraph->value[STASH_LEVEL] = herb->value[STASH_LEVEL];
	paraph->value[STASH_STRENGTH] = herb->value[STASH_STRENGTH];
	paraph->value[STASH_EXTRA] = herb->value[STASH_EXTRA];
}


/***************************************************************************
*	do_unpack_bowl
*
*	unpack a bowl
***************************************************************************/
void do_unpack_bowl(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *paraph;

	if (ch->fighting != NULL) {
		send_to_char("You're too busy fighting!\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
		send_to_char("Unpack what?!\n\r", ch);
		return;
	}


	/* make sure that we are holding paraphenalia */
	if ((paraph = get_obj_wear(ch, argument)) == NULL
	    && (paraph = get_obj_carry(ch, argument)) == NULL) {
		send_to_char("You aren't holding or carrying that.\n\r", ch);
		return;
	}

	if (paraph->value[STASH_AMOUNT] > 0) {
		printf_to_char(ch, "You scrape %s empty.\n\r", paraph->short_descr);
		act("$n scrapes everything out of $p.", ch, paraph, NULL, TO_ROOM);
		paraph->value[STASH_AMOUNT] = 0;
	} else {
		send_to_char("It already appears to be empty.\n\r", ch);
	}
}

/***************************************************************************
*	do_hit_bowl
*
*	hit a bowl
***************************************************************************/
void do_hit_bowl(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *paraph;
	SKILL *skill;
	char arg[MIL];
	int chance;
	int mod;
	int idx;
	int hits;

	if (ch->fighting != NULL) {
		send_to_char("You're too busy fighting!\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Hit what?!\n\r", ch);
		return;
	}

	if ((paraph = get_obj_wear(ch, arg)) == NULL) {
		send_to_char("You aren't holding that.\n\r", ch);
		return;
	}

	if (paraph->item_type != ITEM_PARAPH) {
		send_to_char("Are you crazy?!\n\r", ch);
		return;
	}

	if (paraph->value[STASH_AMOUNT] < 1) {
		send_to_char("The bowl is `3T`1O`#A`8S`3T``.\n\r", ch);
		return;
	}

	mod = 0;
	if (paraph->value[STASH_SIZE] == 1) {
		send_to_char("You toast the bowl. `8Yecccch.. `1S`!c`8reen ``H`&i`#t``!\n\r", ch);
		send_to_char("You feel a little `2better``..\n\r", ch);

		act("$n hits $p and toasts the bowl.", ch, paraph, NULL, TO_ROOM);
		mod = (int)(paraph->value[STASH_STRENGTH] * 50);

		ch->hit += 100 + mod;
		ch->hit = UMIN(ch->hit, ch->max_hit);

		paraph->value[STASH_SIZE] = 0;
		WAIT_STATE(ch, 1);
		return;
	}

	chance = number_range(1, 10);
	switch (chance) {
	case 7:
		/* torch the bowl */
		send_to_char("You `1T`!O`#R`&C`8H`` the bowl in one hit!\n\r", ch);
		send_to_char("You feel `#MUCH `@better``!\n\r", ch);
		act("$n hits $p and `1T`!O`#R`&C`8H``E`#S`` the bowl.", ch, paraph, NULL, TO_ROOM);

		/* add the modifier to the characters hp */
		mod = (int)((paraph->value[STASH_STRENGTH] * 50) + 50);
		ch->hit += (paraph->value[STASH_AMOUNT] * 15) + 100 + mod;
		ch->hit = UMIN(ch->hit, ch->max_hit);

		hits = (int)paraph->value[STASH_AMOUNT];
		paraph->value[STASH_AMOUNT] = 0;
		WAIT_STATE(ch, 3);
		break;
	case 3:
		/* bad hit - coughing fit */
		send_to_char("You go into a massive coughing fit! `8*`1hack, hack`8*``\n\r", ch);
		act("$n takes a massive hit off of $p and starts coughing!", ch, paraph, NULL, TO_ROOM);

		/* take damage */
		mod = (int)((paraph->value[STASH_STRENGTH] * 50) / 2);
		ch->hit -= mod;
		ch->hit = UMAX(ch->hit, 1);

		hits = 0;
		paraph->value[STASH_AMOUNT] -= 1;
		break;
	default:
		printf_to_char(ch, "You take a massive hit off of %s.\n\r", paraph->short_descr);
		send_to_char("You feel `@better``!\n\r", ch);

		act("$n takes a massive hit off of $p.", ch, paraph, NULL, TO_ROOM);

		mod = (int)(paraph->value[3] * 50);
		ch->hit += 100 + mod;
		ch->hit = UMIN(ch->hit, ch->max_hit);

		hits = 1;
		paraph->value[STASH_AMOUNT] -= 1;
		WAIT_STATE(ch, 10);
		break;
	}

	/* check to see if there is some extra funk on the stash */
	if ((skill = resolve_skill_sn((int)paraph->value[STASH_EXTRA])) != NULL
	    && skill->spells != NULL) {
		for (idx = 0; idx < hits; idx++)
			cast_spell(ch, skill, UMAX((int)paraph->value[STASH_LEVEL], ch->level), ch, skill->target, "");
	}
}
