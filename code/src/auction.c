/**************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,        *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael          *
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
*       Russ Taylor(rtaylor@hypercube.org)                                *
*       Gabrielle Taylor(gtaylor@hypercube.org)                           *
*       Brian Moore(zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "channels.h"

#include <stdio.h>
#include <string.h>


extern bool is_digit(const char test);
extern unsigned int parse_unsigned_int(char *string);
extern int parse_int(char *string);

extern void recursive_clone(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * clone);

bool check_bid(CHAR_DATA * ch, unsigned int bid, int type);
void credit_player_bid(CHAR_DATA * ch, long bid, int type);
int auction_type_lookup(int type);
int auction_name_lookup(char *name);

/***************************************************************************
*	auction_type_table
*
*	used to handle support for multiple auction types
***************************************************************************/
#define AUCTION_TYPE_GOLD               100
#define AUCTION_TYPE_QP                 101

const struct auction_types auction_type_table[] =
{
	{ "gold", "`3Gold", AUCTION_TYPE_GOLD, 1000u, 2000000000u, TRUE	 },
	{ "",	  "",	    -1,		       0u,    0u,	   FALSE }
};



/***************************************************************************
*	advatoi
*
*	allows numbers like 10M to be parsed as 10,000,000
*	or 100k as 100,000 and 100k50 to come out to 100,050
***************************************************************************/
unsigned int advatoi(const char *s)
{
	char string[MIL];               /* a buffer to hold a copy of the argument */
	char *stringptr = string;       /* a pointer to the buffer so we can move around */
	char tempstring[2];             /* a small temp buffer to pass to atoi */
	unsigned int number = 0;        /* number to be returned */
	unsigned int multiplier = 0;    /* multiplier used to get the extra digits right */


	strcpy(string, s);              /* working copy */

	while (is_digit(*stringptr)) { /* as long as the current character is a digit */
		strncpy(tempstring, stringptr, 1);                              /* copy first digit */
		number = (number * 10) + parse_unsigned_int(tempstring);        /* add to current number */
		stringptr++;                                                    /* advance */
	}

	switch (UPPER(*stringptr)) {
	case 'K':
		multiplier = 1000;
		number *= multiplier;
		stringptr++;
		break;
	case 'M':
		multiplier = 1000000;
		number *= multiplier;
		stringptr++;
		break;
	case '\0':
		break;
	default:
		return 0; /* not k nor m nor NUL - return 0! */
	}

	while (is_digit(*stringptr) && (multiplier > 1)) {      /* if any digits follow k/m, add those too */
		strncpy(tempstring, stringptr, 1);              /* copy first digit */
		multiplier = multiplier / 10;                   /* the further we get to right, the less are the digit 'worth' */
		number = number + (parse_unsigned_int(tempstring) * multiplier);
		stringptr++;
	}

	if (*stringptr != '\0' && !is_digit(*stringptr))
		return 0;

	return number;
}


/***************************************************************************
*	parsebet
*
*	given an argument string, parse out the value
***************************************************************************/
unsigned int parsebet(const unsigned int currentbet, const char *argument)
{
	unsigned int newbet = 0;                                                                                /* a variable to temporarily hold the new bet */
	char string[MIL];                                                                                       /* a buffer to modify the bet string */
	char *stringptr = string;                                                                               /* a pointer we can move around */

	strcpy(string, argument);                                                                               /* make a work copy of argument */

	if (*stringptr != '\0') {                                                                               /* check for an empty string */
		if (is_digit(*stringptr)) {                                                                     /* first char is a digit assume e.g. 433k */
			newbet = advatoi(stringptr);                                                            /* parse and set newbet to that value */
		} else if (*stringptr == '+') {                                                                 /* add ?? percent */
			if (strlen(stringptr) == 1)                                                             /* only + specified, assume default */
				newbet = (currentbet * 125) / 100;                                              /* default: add 25% */
			else
				newbet = (currentbet * (100u + parse_unsigned_int(++stringptr))) / 100u;        /* cut off the first char */
		} else {
			if ((*stringptr == '*') || (*stringptr == 'x')) {                                       /* multiply */
				if (strlen(stringptr) == 1)                                                     /* only x specified, assume default */
					newbet = currentbet * 2u;                                               /* default: twice */
				else                                                                            /* user specified a number */
					newbet = currentbet * parse_unsigned_int(++stringptr);                  /* cut off the first char */
			}
		}
	}

	return newbet;  /* return the calculated bet */
}

/***************************************************************************
*	auction_type_lookup
*
*	look up an auction index by type
***************************************************************************/
int auction_type_lookup(int type)
{
	int idx;

	for (idx = 0; auction_type_table[idx].name[0] != '\0'; idx++)
		if (auction_type_table[idx].type == type)
			return idx;

	return 0;
}

/***************************************************************************
*	auction_name_lookup
*
*	look up an auction index by name
***************************************************************************/
int auction_name_lookup(char *name)
{
	int idx;

	for (idx = 0; auction_type_table[idx].name[0] != '\0'; idx++)
		if (!str_prefix(name, auction_type_table[idx].name))
			return idx;

	return auction_type_lookup(AUCTION_TYPE_GOLD);
}

/***************************************************************************
*	check_bid
*
*	see if a player has the proper bid of a given type
***************************************************************************/
bool check_bid(CHAR_DATA *ch, unsigned int bid, int type)
{
	bool check = FALSE;

	if (IS_NPC(ch))
		return FALSE;

	switch (type) {
	case AUCTION_TYPE_GOLD:
		if (ch->gold >= UMAX(0, bid))
			check = TRUE;
		break;
	}

	return check;
}

/***************************************************************************
*	credit_player_bid
*
*	see if a player has the proper bid of a given type
***************************************************************************/
void credit_player_bid(CHAR_DATA *ch, long bid, int type)
{
	unsigned int *target;
	long result;

	if (IS_NPC(ch))
		return;

	switch (type) {
	case AUCTION_TYPE_GOLD:
		target = &ch->gold;
		break;
	default:
		bug("Unknown auction type %d", type);
		return;
	}

	result = *target + bid;
	*target = (unsigned int)UMAX(0, result);
}

/***************************************************************************
*	do_auction
***************************************************************************/
void do_auction(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg1[MIL];
	char arg2[MIL];
	char buf[MSL];
	int auction_idx;
	unsigned int auction_reserve;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (IS_NPC(ch))
		return;

	if (arg1[0] == '\0') {
		if (auction->item != NULL) {
			auction_idx = auction_type_lookup(auction->type);
			if (auction->bet > 0) {
				printf_to_char(ch, "`7Current bid on this item is `#%d`7 %s%s``.\n\r",
					       auction->bet,
					       auction_type_table[auction_idx].display,
					       (auction->bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
			} else {
				send_to_char("No bids on this item have been placed.\n\r", ch);
			}

			printf_to_char(ch, "Minimum bid is: `#%u`` %s%s``\n\r",
				       (auction->bet > 0) ? auction->bet + ((auction->bet * 10) / 100) : auction->reserve,
				       auction_type_table[auction_idx].display,
				       (auction->bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");


			printf_to_char(ch, "Item auctioned by: %s\n\r", auction->seller->name);
			if (auction->buyer != NULL)
				printf_to_char(ch, "Current buyer: %s\n\r", auction->buyer->name);

			send_to_char("\n\r", ch);
			identify_item(ch, auction->item);
			return;
		} else {
			send_to_char("What do you want to auction? ..\n\r", ch);
			return;
		}
	}

	if (IS_IMMORTAL(ch) || !str_cmp(ch->name, auction->seller->name)) {
		if (!str_cmp(arg1, "stop")) {
			if (auction->item == NULL) {
				send_to_char("There is no auction going on you can stop.\n\r", ch);
				return;
			} else {
				sprintf(buf, "Sale of %s has been stopped by %s.\n\r",
					auction->item->short_descr,
					ch->name);
                broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), buf);
				obj_to_char(auction->item, ch);
				auction->item = NULL;

				if (auction->buyer != NULL) {            /* return money to the buyer */
					credit_player_bid(auction->buyer, (long)auction->bet, auction->type);
					send_to_char("Your bid has been returned.\n\r", auction->buyer);
				}
				return;
			}
		}

		if (auction->item != NULL && !str_cmp(arg1, "clone")) {
			OBJ_DATA *obj_new = NULL;

			obj_new = create_object(auction->item->obj_idx, auction->item->level);
			clone_object(auction->item, obj_new);
			obj_to_char(obj_new, ch);
			recursive_clone(ch, auction->item, obj_new);

			send_to_char("Object cloned.\n\r", ch);
			return;
		}

		if (!str_cmp(arg1, "reset"))
			auction->item = NULL;

	}



	if (!str_cmp(arg1, "bid")) {
		if (auction->item != NULL) {
			unsigned int bid = 0;
			unsigned int min = 0;

			auction_idx = auction_type_lookup(auction->type);

			if (arg2[0] == '\0') {
				sprintf(buf, "How much do you want to bid on %s?\n\r",
					auction->item->short_descr);
				send_to_char(buf, ch);
				return;
			}

			if (ch == auction->seller) {
				send_to_char("You want to bid on something you already own?\n\r", ch);
				return;
			}

			bid = parsebet(auction->bet, arg2);

			min = UMAX(1u, (auction->bet / 10u));
			/*((auction->bet > 10000) ?(auction->bet / 10) : 1000); */

			if (bid >= auction->reserve) {
				if (bid < (min + auction->bet)) {
					printf_to_char(ch, "You must bid at least `#%u`` %s%s`` over the current bid.\n\r",
						       min,
						       auction_type_table[auction_idx].display,
						       (min > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
					printf_to_char(ch, "The currrent bid is `#%u`` %s%s``.\n\r",
						       auction->bet,
						       auction_type_table[auction_idx].display,
						       (auction->bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
					return;
				}
			} else {
				printf_to_char(ch, "You must bid at least `#%u`` %s%s``.\n\r",
					       auction->reserve, auction_type_table[auction_idx].display,
					       (auction->reserve > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
				return;
			}

			if (!check_bid(ch, bid, auction->type)) {
				printf_to_char(ch, "You don't have that %s %s%s``!\n\r",
					       (!auction_type_table[auction_idx].is_coins) ? "many" : "much",
					       auction_type_table[auction_idx].display,
					       (!auction_type_table[auction_idx].is_coins) ? "s" : "");
				return;
			}


			if (auction->buyer != NULL)
				credit_player_bid(auction->buyer, (long)auction->bet, auction->type);
				/*auction->buyer->gold += auction->bet; */

			/*ch->gold		-= bid;*/
			credit_player_bid(ch, (long)(-1 * bid), auction->type);

			auction->buyer = ch;
			auction->bet = bid;
			auction->going = 0;
			auction->pulse = PULSE_AUCTION;

			sprintf(buf, "A bid of `#%u`7 %s%s`` has been received on %s.\n\r",
				bid,
				auction_type_table[auction_idx].display,
				(bid > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "",
				auction->item->short_descr);
			broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), buf);
			return;
		} else {
			send_to_char("There isn't anything being auctioned right now.\n\r", ch);
			return;
		}
	}

	if ((obj = get_obj_carry(ch, arg1)) == NULL) {
		send_to_char("You aren't carrying that.\n\r", ch);
		return;
	}

	if (auction->item == NULL) {
		/* parsing the request is a little tougher now */
		auction_idx = auction_type_lookup(AUCTION_TYPE_GOLD);
		auction_reserve = 0;

		if (arg2[0] != '\0' && is_digit(arg2[0])) {
			auction_reserve = parse_unsigned_int(arg2);
			argument = one_argument(argument, arg2);
		}

		if (arg2[0] != '\0')
			/* we have a type - look it up */
			auction_idx = auction_name_lookup(arg2);

		/* make sure the reserve falls into certain parameters */
		if (auction_reserve < auction_type_table[auction_idx].min_reserve)
			auction_reserve = auction_type_table[auction_idx].min_reserve;

		if (auction_reserve > auction_type_table[auction_idx].max_reserve)
			auction_reserve = auction_type_table[auction_idx].max_reserve;

		if (IS_SET(obj->extra_flags, ITEM_NOAUC)) {
			send_to_char("You cannot auction that.\n\r", ch);
			return;
		}

		switch (obj->item_type) {
		default:
			send_to_char("You cannot auction that.\n\r", ch);
			return;

		case ITEM_LIGHT:
		case ITEM_POTION:
		case ITEM_CLOTHING:
		case ITEM_CONTAINER:
		case ITEM_BOAT:
		case ITEM_PILL:
		case ITEM_MAP:
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_STAFF:
		case ITEM_WAND:
		case ITEM_SCROLL:
		case ITEM_DOLL:
		case ITEM_DICE:
		case ITEM_FURNITURE:
		case ITEM_JEWELRY:
			obj_from_char(obj);
			auction->item = obj;
			auction->bet = 0;
			auction->buyer = NULL;
			auction->seller = ch;
			auction->pulse = PULSE_AUCTION;
			auction->going = 0;
			auction->type = auction_type_table[auction_idx].type;

			/*  this needs to be fixed */
			auction->reserve = auction_reserve;

			sprintf(buf, "A new item has been received: %s.", obj->short_descr);
			broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), buf);
			sprintf(buf, "Bidding will start at `#%u`` %s%s``.",
				auction->reserve,
				auction_type_table[auction_idx].display,
				(auction->reserve > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
			broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), buf);

			return;
		}
	} else {
		send_to_char("Try again later - something is being auctioned right now!\n\r", ch);
		return;
	}
}
