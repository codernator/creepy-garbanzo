#include "merc.h"
#include "object.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "channels.h"


#include <stdio.h>
#include <string.h>
#ifndef S_SPLINT_S
#include <ctype.h>
#endif

/** exports */
void auction_update(void);
bool is_auction_participant(CHAR_DATA *ch);
/*@shared@*//*@null@*/struct gameobject *get_auction_item();


/** imports */
extern void recursive_clone(CHAR_DATA * ch, struct gameobject * obj, struct gameobject * clone);


/** locals */
typedef struct auction_data AUCTION_DATA;
struct auction_data {
    struct gameobject * item;
    CHAR_DATA * seller;
    CHAR_DATA * buyer;
    unsigned int bet;
    int  going;
    int  pulse;
    unsigned int reserve;
    int  type;
};

struct auction_types {
    char *name;
    char *display;
    int type;
    unsigned int min_reserve;
    unsigned int max_reserve;
    bool is_coins;
};

static AUCTION_DATA currentAuction;
static int auction_type_lookup(int type);
static int auction_name_lookup(char *name);
static bool check_bid(CHAR_DATA * ch, unsigned int bid, int type);
static void credit_player_bid(CHAR_DATA * ch, long bid, int type);
static unsigned int parsebet(const unsigned int currentbet, const char *argument);
static unsigned int advatoi(const char *s);

#define AUCTION_TYPE_GOLD               100
#define AUCTION_TYPE_QP                 101

/***************************************************************************
 * support for multiple auction types
 ***************************************************************************/
static const struct auction_types auction_type_table[] =
{
    { "gold", "`3Gold", AUCTION_TYPE_GOLD, 1000u, 2000000000u, true	 },
    { "",	  "",	    -1,		       0u,    0u,	   false }
};



/***************************************************************************
 *	do_auction
 ***************************************************************************/
void do_auction(CHAR_DATA *ch, const char *argument)
{
    struct gameobject *obj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int auction_idx;
    unsigned int auction_reserve;

    DENY_NPC(ch);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        if (currentAuction.item != NULL) {
            auction_idx = auction_type_lookup(currentAuction.type);
            if (currentAuction.bet > 0) {
                printf_to_char(ch, "`7Current bid on this item is `#%d`7 %s%s``.\n\r",
                               currentAuction.bet,
                               auction_type_table[auction_idx].display,
                               (currentAuction.bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
            } else {
                send_to_char("No bids on this item have been placed.\n\r", ch);
            }

            printf_to_char(ch, "Minimum bid is: `#%u`` %s%s``\n\r",
                           (currentAuction.bet > 0) ? currentAuction.bet + ((currentAuction.bet * 10) / 100) : currentAuction.reserve,
                           auction_type_table[auction_idx].display,
                           (currentAuction.bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");


            printf_to_char(ch, "Item auctioned by: %s\n\r", currentAuction.seller->name);
            if (currentAuction.buyer != NULL)
                printf_to_char(ch, "Current buyer: %s\n\r", currentAuction.buyer->name);

            send_to_char("\n\r", ch);
            identify_item(ch, currentAuction.item);
            return;
        } else {
            send_to_char("What do you want to auction? ..\n\r", ch);
            return;
        }
    }

    if (IS_IMMORTAL(ch) || !str_cmp(ch->name, currentAuction.seller->name)) {
        if (!str_cmp(arg1, "stop")) {
            if (currentAuction.item == NULL) {
                send_to_char("There is no auction going on you can stop.\n\r", ch);
                return;
            } else {
                sprintf(buf, "Sale of %s has been stopped by %s.\n\r", currentAuction.item->short_descr, ch->name);
                broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);
                obj_to_char(currentAuction.item, ch);
                currentAuction.item = NULL;

                if (currentAuction.buyer != NULL) {            /* return money to the buyer */
                    credit_player_bid(currentAuction.buyer, (long)currentAuction.bet, currentAuction.type);
                    send_to_char("Your bid has been returned.\n\r", currentAuction.buyer);
                }
                return;
            }
        }

        if (currentAuction.item != NULL && !str_cmp(arg1, "clone")) {
            struct gameobject *obj_new = NULL;

            obj_new = object_clone(currentAuction.item);
            obj_to_char(obj_new, ch);
            recursive_clone(ch, currentAuction.item, obj_new);

            send_to_char("Object cloned.\n\r", ch);
            return;
        }

        if (!str_cmp(arg1, "reset")) {
            currentAuction.item = NULL;
        }

    }



    if (!str_cmp(arg1, "bid")) {
        if (currentAuction.item != NULL) {
            unsigned int bid = 0;
            unsigned int min = 0;

            auction_idx = auction_type_lookup(currentAuction.type);

            if (arg2[0] == '\0') {
                sprintf(buf, "How much do you want to bid on %s?\n\r", currentAuction.item->short_descr);
                send_to_char(buf, ch);
                return;
            }

            if (ch == currentAuction.seller) {
                send_to_char("You want to bid on something you already own?\n\r", ch);
                return;
            }

            bid = parsebet(currentAuction.bet, arg2);

            min = UMAX(1u, (currentAuction.bet / 10u));
            /*((currentAuction.bet > 10000) ?(currentAuction.bet / 10) : 1000); */

            if (bid >= currentAuction.reserve) {
                if (bid < (min + currentAuction.bet)) {
                    printf_to_char(ch, "You must bid at least `#%u`` %s%s`` over the current bid.\n\r",
                                   min,
                                   auction_type_table[auction_idx].display,
                                   (min > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
                    printf_to_char(ch, "The currrent bid is `#%u`` %s%s``.\n\r",
                                   currentAuction.bet,
                                   auction_type_table[auction_idx].display,
                                   (currentAuction.bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
                    return;
                }
            } else {
                printf_to_char(ch, "You must bid at least `#%u`` %s%s``.\n\r",
                               currentAuction.reserve, auction_type_table[auction_idx].display,
                               (currentAuction.reserve > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
                return;
            }

            if (!check_bid(ch, bid, currentAuction.type)) {
                printf_to_char(ch, "You don't have that %s %s%s``!\n\r",
                               (!auction_type_table[auction_idx].is_coins) ? "many" : "much",
                               auction_type_table[auction_idx].display,
                               (!auction_type_table[auction_idx].is_coins) ? "s" : "");
                return;
            }


            if (currentAuction.buyer != NULL)
                credit_player_bid(currentAuction.buyer, (long)currentAuction.bet, currentAuction.type);
            /*currentAuction.buyer->gold += currentAuction.bet; */

            /*ch->gold		-= bid;*/
            credit_player_bid(ch, (long)(-1 * bid), currentAuction.type);

            currentAuction.buyer = ch;
            currentAuction.bet = bid;
            currentAuction.going = 0;
            currentAuction.pulse = PULSE_AUCTION;

            sprintf(buf, "A bid of `#%u`7 %s%s`` has been received on %s.\n\r",
                    bid,
                    auction_type_table[auction_idx].display,
                    (bid > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "",
                    currentAuction.item->short_descr);
            broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);
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

    if (currentAuction.item == NULL) {
        /* parsing the request is a little tougher now */
        auction_idx = auction_type_lookup(AUCTION_TYPE_GOLD);
        auction_reserve = 0;

        if (arg2[0] != '\0' && isdigit((int)arg2[0])) {
            auction_reserve = parse_unsigned_int(arg2);
            argument = one_argument(argument, arg2);
        }

        if (arg2[0] != '\0') {
            /* we have a type - look it up */
            auction_idx = auction_name_lookup(arg2);
        }

        /* make sure the reserve falls into certain parameters */
        if (auction_reserve < auction_type_table[auction_idx].min_reserve) {
            auction_reserve = auction_type_table[auction_idx].min_reserve;
        }

        if (auction_reserve > auction_type_table[auction_idx].max_reserve) {
            auction_reserve = auction_type_table[auction_idx].max_reserve;
        }

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
              currentAuction.item = obj;
              currentAuction.bet = 0;
              currentAuction.buyer = NULL;
              currentAuction.seller = ch;
              currentAuction.pulse = PULSE_AUCTION;
              currentAuction.going = 0;
              currentAuction.type = auction_type_table[auction_idx].type;

              /*  this needs to be fixed */
              currentAuction.reserve = auction_reserve;

              sprintf(buf, "A new item has been received: %s.", obj->short_descr);
              broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);
              sprintf(buf, "Bidding will start at `#%u`` %s%s``.",
                      currentAuction.reserve,
                      auction_type_table[auction_idx].display,
                      (currentAuction.reserve > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
              broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);

              return;
        }
    } else {
        send_to_char("Try again later - something is being auctioned right now!\n\r", ch);
        return;
    }
}

/***************************************************************************
 *	automatted updating for auctions
 ***************************************************************************/
void auction_update(void)
{
    char buf[MAX_STRING_LENGTH];
    int auction_idx;

    if (currentAuction.item != NULL) {
        auction_idx = auction_type_lookup(currentAuction.type);

        if (--currentAuction.pulse <= 0) {            /* decrease pulse */
            currentAuction.pulse = PULSE_AUCTION;
            switch (++currentAuction.going) {     /* increase the going state */
              case 1:                         /* going once */
              case 2:                         /* going twice */
                  if (currentAuction.bet > 0) {
                      sprintf(buf, "%s going %s for `#%u`` %s%s``.",
                              currentAuction.item->short_descr,
                              ((currentAuction.going == 1) ? "once" : "twice"),
                              currentAuction.bet, auction_type_table[auction_idx].display,
                              (currentAuction.bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
                  } else {
                      sprintf(buf, "%s going %s `!(`7no bet received yet`!)``.", currentAuction.item->short_descr, ((currentAuction.going == 1) ? "once" : "twice"));
                  }
                  broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);
                  break;

              case 3: /* SOLD! */
                  if (currentAuction.bet > 0) {
                      sprintf(buf, "%s sold to %s for `#%u`7 %s%s``.",
                              currentAuction.item->short_descr,
                              IS_NPC(currentAuction.buyer) ? currentAuction.buyer->short_descr : currentAuction.buyer->name,
                              currentAuction.bet, auction_type_table[auction_idx].display,
                              (currentAuction.bet > 1 && !auction_type_table[auction_idx].is_coins) ? "s" : "");
                      broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);
                      obj_to_char(currentAuction.item, currentAuction.buyer);
                      act("The auctioneer appears before you in a puff of smoke and hands you $p.", currentAuction.buyer, currentAuction.item, NULL, TO_CHAR);
                      act("The auctioneer appears before $n, and hands $m $p", currentAuction.buyer, currentAuction.item, NULL, TO_ROOM);

                      /* credit the seller */
                      credit_player_bid(currentAuction.seller, (long)currentAuction.bet, currentAuction.type);
                      currentAuction.item = NULL;                   /* reset item */
                  } else {
                      sprintf(buf, "No bets received for %s - object has been `!removed`7.", currentAuction.item->short_descr);
                      broadcast_channel(NULL, channels_find(CHANNEL_AUCTION), NULL, buf);
                      act("The auctioneer appears before you to return $p to you.", currentAuction.seller, currentAuction.item, NULL, TO_CHAR);
                      act("The auctioneer appears before $n to return $p to $m.", currentAuction.seller, currentAuction.item, NULL, TO_ROOM);
                      obj_to_char(currentAuction.item, currentAuction.seller);
                      currentAuction.item = NULL;   /* clear auction */
                  }
            }
        }
    }
}

inline bool is_auction_participant(CHAR_DATA *ch)
{
    return (currentAuction.item != NULL && ((ch == currentAuction.buyer) || (ch == currentAuction.seller)));
}

inline struct gameobject *get_auction_item()
{
    return currentAuction.item;
}

/***************************************************************************
 *	allows numbers like 10M to be parsed as 10,000,000
 *	or 100k as 100,000 and 100k50 to come out to 100,050
 ***************************************************************************/
unsigned int advatoi(const char *s)
{
    char string[MAX_INPUT_LENGTH];               /* a buffer to hold a copy of the argument */
    char *stringptr = string;       /* a pointer to the buffer so we can move around */
    char tempstring[2];             /* a small temp buffer to pass to atoi */
    unsigned int number = 0;        /* number to be returned */
    unsigned int multiplier = 0;    /* multiplier used to get the extra digits right */


    strcpy(string, s);              /* working copy */

    while (isdigit((int)*stringptr)) { /* as long as the current character is a digit */
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

    while (isdigit((int)*stringptr) && (multiplier > 1)) {      /* if any digits follow k/m, add those too */
        strncpy(tempstring, stringptr, 1);              /* copy first digit */
        multiplier = multiplier / 10;                   /* the further we get to right, the less are the digit 'worth' */
        number = number + (parse_unsigned_int(tempstring) * multiplier);
        stringptr++;
    }

    if (*stringptr != '\0' && !isdigit((int)*stringptr))
        return 0;

    return number;
}


/***************************************************************************
 * given an argument string, parse out the value
 ***************************************************************************/
unsigned int parsebet(const unsigned int currentbet, const char *argument)
{
    unsigned int newbet = 0;                                                                                /* a variable to temporarily hold the new bet */
    char string[MAX_INPUT_LENGTH];                                                                                       /* a buffer to modify the bet string */
    char *stringptr = string;                                                                               /* a pointer we can move around */

    strcpy(string, argument);                                                                               /* make a work copy of argument */

    if (*stringptr != '\0') {                                                                               /* check for an empty string */
        if (isdigit((int)*stringptr)) {                                                                     /* first char is a digit assume e.g. 433k */
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
 * look up an auction index by type
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
 * look up an auction index by name
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
 * see if a player has the proper bid of a given type
 ***************************************************************************/
bool check_bid(CHAR_DATA *ch, unsigned int bid, int type)
{
    switch (type) {
      case AUCTION_TYPE_GOLD:
          return (ch->gold >= UMAX(0, bid));
      default:
          return false;
    }
}

/***************************************************************************
 * see if a player has the proper bid of a given type
 ***************************************************************************/
void credit_player_bid(CHAR_DATA *ch, long bid, int type)
{
    unsigned int *target;
    long result;

    switch (type) {
      case AUCTION_TYPE_GOLD:
          target = &ch->gold;
          break;
      default:
          log_bug("Unknown auction type %d", type);
          return;
    }

    result = *target + bid;
    *target = (unsigned int)UMAX(0, result);
}
