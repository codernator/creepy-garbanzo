#include "merc.h"
#include "object.h"
#include "character.h"
#include "magic.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"
#include "channels.h"
#include "help.h"
#include <stdio.h>
#include <string.h>


/***************************************************************************
 *	external commands
 ***************************************************************************/

extern void mp_bribe_trigger(CHAR_DATA * mob, CHAR_DATA * ch, long amount);
extern void mp_give_trigger(CHAR_DATA * mob, CHAR_DATA * ch, GAMEOBJECT * obj);

extern SKILL *gsp_poison;
extern SKILL *gsp_hand_to_hand;
extern SKILL *gsp_haggle;

/*
 * Local functions.
 */
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace);
void wear_obj(CHAR_DATA * ch, GAMEOBJECT * obj, bool fReplace);
CHAR_DATA *find_keeper(CHAR_DATA * ch);
unsigned int get_cost(CHAR_DATA * keeper, GAMEOBJECT * obj, bool fBuy);
void obj_to_keeper(GAMEOBJECT * obj, CHAR_DATA * ch);
GAMEOBJECT *get_obj_keeper(CHAR_DATA * ch, CHAR_DATA * keeper, char *argument);
int count_slots(GAMEOBJECT * obj);


bool can_loot(CHAR_DATA *ch, GAMEOBJECT *obj)
{
    /*@dependent@*/const char *ownername;
    CHAR_DATA *owner;
    CHAR_DATA *wch;

    if (IS_IMMORTAL(ch))
        return true;

    ownername = object_ownername_get(obj);
    if (ownername == NULL)
        return true;

    owner = NULL;
    for (wch = char_list; wch != NULL; wch = wch->next)
        if (!str_cmp(wch->name, ownername))
            owner = wch;

    if (owner == NULL)
        return true;

    if (!str_cmp(ch->name, ownername))
        return true;

    if (!IS_NPC(owner))
        return false;

    if (!IS_NPC(owner) && IS_SET(owner->act, PLR_CANLOOT))
        return true;

    if (is_same_group(ch, owner))
        return true;

    return true;
}


void affect_join_obj(GAMEOBJECT *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = obj->affected; paf_old != NULL; paf_old = paf_old->next) {
        if (paf_old->type == paf->type) {
            paf->level = (paf->level + paf_old->level) / (int)2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove_obj(obj, paf_old);
            break;
        }
    }

    affect_to_obj(obj, paf);
    return;
}

void do_get(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    GAMEOBJECT *obj_next;
    GAMEOBJECT *container;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    bool found;
    int item_ctr;

    argument = one_argument(argument, arg1);
    if (arg1[0] == '\0') {
        send_to_char("Get what?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg2);
    if (!str_cmp(arg2, "from"))
        argument = one_argument(argument, arg2);

    if (arg2[0] == '\0') {
        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            obj = get_obj_list(ch, arg1, ch->in_room->contents);
            if (obj == NULL) {
                act("I see no $T here.", ch, NULL, arg1, TO_CHAR);
                return;
            }

            get_obj(ch, obj, NULL);
        } else {
            found = false;
            item_ctr = 0;

            for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
                obj_next = obj->next_content;
                if ((arg1[3] == '\0' || is_name(&arg1[4], object_name_get(obj)))
                    && can_see_obj(ch, obj)) {
                    found = true;
                    get_obj(ch, obj, NULL);
                    item_ctr++;
                    if ((item_ctr >= MAX_GET)
                        && !IS_IMMORTAL(ch)) {
                        send_to_char("Your hands cramp up!\n\r", ch);
                        break;
                    }
                }
            }

            if (!found) {
                if (arg1[3] == '\0')
                    send_to_char("I see nothing here.\n\r", ch);
                else
                    act("I see no $T here.", ch, NULL, &arg1[4], TO_CHAR);
            }
        }
    } else {
        if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }

        if ((container = get_obj_here(ch, arg2)) == NULL) {
            act("I see no $T here.", ch, NULL, arg2, TO_CHAR);
            return;
        }

        switch (container->item_type) {
          default:
              send_to_char("That's not a container.\n\r", ch);
              return;

          case ITEM_CONTAINER:
          case ITEM_CORPSE_NPC:
              break;

          case ITEM_CORPSE_PC:
              if (!can_loot(ch, container)) {
                  send_to_char("Looting is expressly forbidden.\n\r", ch);
                  return;
              }
              break;
        }

        if (IS_SET(container->value[1], CONT_CLOSED)) {
            act("The $d is closed.", ch, NULL, object_name_get(container), TO_CHAR);
            return;
        }

        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            obj = get_obj_list(ch, arg1, container->contains);
            if (obj == NULL) {
                act("I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR);
                return;
            }

            get_obj(ch, obj, container);
        } else {
            const char *ownername;

            ownername = object_ownername_get(container);
            found = false;
            item_ctr = 0;
            for (obj = container->contains; obj != NULL; obj = obj_next) {
                obj_next = obj->next_content;

                if ((arg1[3] == '\0' || is_name(&arg1[4], object_name_get(obj))) && can_see_obj(ch, obj)) {
                    found = true;

                    if (container->objprototype->vnum == OBJ_VNUM_PIT && !IS_IMMORTAL(ch)) {
                        send_to_char("Don't be so greedy!\n\r", ch);
                        return;
                    }

                    if (container->item_type == ITEM_CORPSE_PC && (str_cmp(ownername, ch->name)) && ownername != NULL && !IS_IMMORTAL(ch)) {
                        send_to_char("Don't be so greedy!\n\r", ch);
                        return;
                    }

                    get_obj(ch, obj, container);
                    item_ctr++;
                    if ((item_ctr >= get_curr_stat(ch, STAT_STR)) && !IS_IMMORTAL(ch)) {
                        send_to_char("Your hands cramp up!\n\r", ch);
                        break;
                    }
                }
            }

            if (!found) {
                if (arg1[3] == '\0') {
                    act("I see nothing in the $T.", ch, NULL, arg2, TO_CHAR);
                } else {
                    act("I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR);
                }
            }
        }
    }

    save_char_obj(ch);
}

void do_put(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *container;
    GAMEOBJECT *obj;
    GAMEOBJECT *obj_next;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int item_ctr;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if (!str_cmp(arg2, "in") || !str_cmp(arg2, "on"))
        argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Put what in what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    if ((container = get_obj_here(ch, arg2)) == NULL) {
        act("I see no $T here.", ch, NULL, arg2, TO_CHAR);
        return;
    }

    if (container->item_type != ITEM_CONTAINER) {
        send_to_char("That's not a container.\n\r", ch);
        return;
    }

    if (IS_SET(container->value[1], CONT_CLOSED)) {
        act("The $d is closed.", ch, NULL, object_name_get(container), TO_CHAR);
        return;
    }

    if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
        if ((obj = get_obj_carry(ch, arg1)) == NULL) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (obj == container) {
            send_to_char("You can't fold it into itself.\n\r", ch);
            return;
        }

        if (IS_SET(obj->extra_flags, ITEM_DEATH_DROP)) {
            send_to_char("You cannot put that item into a container.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
            return;
        }

        if ((WEIGHT_MULT(obj) != 100) && (!IS_IMMORTAL(ch))) {
            send_to_char("You have a feeling that would be a bad idea.\n\r", ch);
            return;
        }

        if ((long)(get_obj_weight(obj) + get_true_weight(container)) > (container->value[0] * 10l)
            || (long)(get_obj_weight(obj)) > (container->value[3] * 10l)) {
            send_to_char("It won't fit.\n\r", ch);
            return;
        }

        item_ctr = 0;
        if (container->objprototype->vnum == OBJ_VNUM_PIT
            && !CAN_WEAR(container, ITEM_TAKE)) {
            if (obj->timer)
                SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
            else
                obj->timer = number_range(100, 200);
        }
        obj_from_char(obj);
        obj_to_obj(obj, container);

        item_ctr++;
        if ((item_ctr >= MAX_GET) && !IS_IMMORTAL(ch)) {
            send_to_char("You just can't put everything in that fast!\n\r", ch);
            return;
        }

        if (IS_SET(container->value[1], CONT_PUT_ON)) {
            act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
            act("You put $p on $P.", ch, obj, container, TO_CHAR);
        } else {
            act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
            act("You put $p in $P.", ch, obj, container, TO_CHAR);
        }
    } else {
        item_ctr = 0;
        for (obj = ch->carrying; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;

            if (IS_SET(obj->extra_flags, ITEM_DEATH_DROP)) {
                send_to_char("You cannot put that item into a container.\n\r", ch);
                continue;
            }
            if ((arg1[3] == '\0' || is_name(&arg1[4], object_name_get(obj)))
                && can_see_obj(ch, obj)
                && WEIGHT_MULT(obj) == 100
                && obj->wear_loc == WEAR_NONE
                && obj != container
                && can_drop_obj(ch, obj)
                && (long)(get_obj_weight(obj) + get_true_weight(container)) <= (container->value[0] * 10l)
                && (long)(get_obj_weight(obj)) < (container->value[3] * 10l)) {
                if (container->objprototype->vnum == OBJ_VNUM_PIT
                    && !CAN_WEAR(obj, ITEM_TAKE)) {
                    if (obj->timer)
                        SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
                    else
                        obj->timer = number_range(100, 200);
                }

                obj_from_char(obj);
                obj_to_obj(obj, container);
                item_ctr++;

                if (IS_SET(container->value[1], CONT_PUT_ON)) {
                    act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
                    act("You put $p on $P.", ch, obj, container, TO_CHAR);
                } else {
                    act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
                    act("You put $p in $P.", ch, obj, container, TO_CHAR);
                }

                item_ctr++;
                if ((item_ctr >= MAX_GET) && !IS_IMMORTAL(ch)) {
                    send_to_char("You just can't put everything in that fast!\n\r", ch);
                    return;
                }
            }
        }
    }

    save_char_obj(ch);
    return;
}

void do_drop(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    GAMEOBJECT *obj_next;
    char arg[MAX_INPUT_LENGTH];
    bool found;
    int item_ctr;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Drop what?\n\r", ch);
        return;
    }

    if (is_number(arg)) {
        unsigned int amount;
        unsigned int gold = 0;
        unsigned int silver = 0;

        amount = parse_unsigned_int(arg);
        argument = one_argument(argument, arg);
        if ((str_cmp(arg, "coins") && str_cmp(arg, "coin")
             && str_cmp(arg, "gold") && str_cmp(arg, "silver"))) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }

        if (!str_cmp(arg, "coins")
            || !str_cmp(arg, "coin")
            || !str_cmp(arg, "silver")) {
            if (ch->silver < amount) {
                send_to_char("You don't have that much silver.\n\r", ch);
                return;
            }

            ch->silver -= amount;
            silver = amount;
        } else {
            if (ch->gold < amount) {
                send_to_char("You don't have that much gold.\n\r", ch);
                return;
            }

            ch->gold -= amount;
            gold = amount;
        }

        for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;

            switch (obj->objprototype->vnum) {
              case OBJ_VNUM_SILVER_ONE:
                  silver += 1;
                  extract_obj(obj);
                  break;

              case OBJ_VNUM_GOLD_ONE:
                  gold += 1;
                  extract_obj(obj);
                  break;

              case OBJ_VNUM_SILVER_SOME:
                  silver += obj->value[0];
                  extract_obj(obj);
                  break;

              case OBJ_VNUM_GOLD_SOME:
                  gold += obj->value[1];
                  extract_obj(obj);
                  break;

              case OBJ_VNUM_COINS:
                  silver += obj->value[0];
                  gold += obj->value[1];
                  extract_obj(obj);
                  break;
            }
        }

        obj_to_room(create_money(gold, silver), ch->in_room);
        act("$n drops some coins.", ch, NULL, NULL, TO_ROOM);
        send_to_char("OK.\n\r", ch);
        save_char_obj(ch);
        return;
    }

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        if ((obj = get_obj_carry(ch, arg)) == NULL) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
            return;
        }

        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        act("$n drops $p.", ch, obj, NULL, TO_ROOM);
        act("You drop $p.", ch, obj, NULL, TO_CHAR);
        save_char_obj(ch);
        if (IS_OBJ_STAT(obj, ITEM_MELT_DROP)) {
            act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM);
            act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR);
            extract_obj(obj);
        }
    } else {
        found = false;
        item_ctr = 0;
        for (obj = ch->carrying; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;

            if ((arg[3] == '\0'
                 || is_name(&arg[4], object_name_get(obj)))
                && can_see_obj(ch, obj)
                && obj->wear_loc == WEAR_NONE
                && can_drop_obj(ch, obj)) {
                found = true;
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
                item_ctr++;
                act("$n drops $p.", ch, obj, NULL, TO_ROOM);
                act("You drop $p.", ch, obj, NULL, TO_CHAR);

                if (IS_OBJ_STAT(obj, ITEM_MELT_DROP)) {
                    act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM);
                    act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR);
                    extract_obj(obj);
                }

                if ((item_ctr >= MAX_GET) && !IS_IMMORTAL(ch))
                    break;
            }
        }
        save_char_obj(ch);

        if (!found) {
            if (arg[3] == '\0')
                act("You are not carrying anything.", ch, NULL, arg, TO_CHAR);
            else
                act("You are not carrying any $T.", ch, NULL, &arg[4], TO_CHAR);
        }
    }
    return;
}

void do_give(CHAR_DATA *ch, const char *argument)
{
    CHAR_DATA *victim;
    GAMEOBJECT *obj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];


    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Give what to whom?\n\r", ch);
        return;
    }

    if (is_number(arg1)) {
        unsigned int amount;
        bool silver;

        amount = parse_unsigned_int(arg1);
        if (str_cmp(arg2, "coins")
            && str_cmp(arg2, "coin")
            && str_cmp(arg2, "gold")
            && str_cmp(arg2, "silver")) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }

        silver = str_cmp(arg2, "gold");
        argument = one_argument(argument, arg2);
        if (arg2[0] == '\0') {
            send_to_char("Give what to whom?\n\r", ch);
            return;
        }

        if ((victim = get_char_room(ch, arg2)) == NULL) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if ((!silver && ch->gold < amount) || (silver && ch->silver < amount)) {
            send_to_char("You haven't got that much.\n\r", ch);
            return;
        }

        if (silver) {
            ch->silver -= amount;
            victim->silver += amount;
        } else {
            ch->gold -= amount;
            victim->gold += amount;
        }

        if (IS_IMMORTAL(ch)) {
            log_string("%s gave %s %u %s coins", ch->name, victim->name, amount, silver ? "silver" : "gold");
        }
        sprintf(buf, "$n gives you %u %s.", amount, silver ? "silver" : "gold");
        act(buf, ch, NULL, victim, TO_VICT);
        act("$n gives $N some coins.", ch, NULL, victim, TO_NOTVICT);
        sprintf(buf, "You give $N %u %s.", amount, silver ? "silver" : "gold");
        act(buf, ch, NULL, victim, TO_CHAR);
        save_char_obj(ch);
        save_char_obj(victim);

        /*
         * Bribe trigger
         */
        if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_BRIBE))
            mp_bribe_trigger(victim, ch, silver ? (long)amount : (long)(amount * 100));

        if (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_CHANGER)) {
            unsigned int change;

            change = (unsigned int)(silver ? 95 * amount / 100 / 100 : 95 * amount);

            if (!silver && change > victim->silver)
                victim->silver += change;

            if (silver && change > victim->gold)
                victim->gold += change;

            if (change < 1 && can_see(victim, ch)) {
                act("`@$n tells you '`1I'm sorry, you did not give me enough to change.`@'``",
                    victim, NULL, ch, TO_VICT);

                ch->reply = victim;
                sprintf(buf, "%u %s %s", amount, silver ? "silver" : "gold", ch->name);
                do_give(victim, buf);
            } else if (can_see(victim, ch)) {
                sprintf(buf, "%u %s %s", change, silver ? "gold" : "silver", ch->name);
                do_give(victim, buf);
                if (silver) {
                    sprintf(buf, "%u silver %s", (unsigned int)(95 * amount / 100 - change * 100), ch->name);
                    do_give(victim, buf);
                }
                act("`@$n tells you '`1Thank you, come again.`@'``", victim, NULL, ch, TO_VICT);
                ch->reply = victim;
            }
        }

        return;
    }

    if ((obj = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (obj->wear_loc != WEAR_NONE) {
        send_to_char("You must remove it first.\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_SHOPKEEPER(victim)) {
        act("$N tells you 'Sorry, you'll have to sell that.'", ch, NULL, victim, TO_CHAR);
        ch->reply = victim;
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim)) {
        act("$N has $S hands full.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim->carry_weight + get_obj_weight(obj) > can_carry_w(victim)) {
        act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (!can_see_obj(victim, obj)) {
        act("$N can't see it.", ch, NULL, victim, TO_CHAR);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, victim);
    if (IS_IMMORTAL(ch)) {
        log_string("%s gave %s to %s", ch->name, obj->short_descr, victim->name);
    }

    act_new("$n gives $p to $N.", ch, obj, victim, TO_NOTVICT, POS_RESTING, true);
    act_new("$n gives you $p.", ch, obj, victim, TO_VICT, POS_RESTING, true);
    act_new("You give $p to $N.", ch, obj, victim, TO_CHAR, POS_RESTING, true);

    /*
     * Give trigger
     */
    if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_GIVE))
        mp_give_trigger(victim, ch, obj);

    save_char_obj(ch);
    save_char_obj(victim);
    return;
}


/***************************************************************************
 *	do_fill
 ***************************************************************************/
void do_fill(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    GAMEOBJECT *fountain;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    bool found;

    (void)one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Fill what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    found = false;
    for (fountain = ch->in_room->contents;
         fountain != NULL;
         fountain = fountain->next_content) {
        if (fountain->item_type == ITEM_FOUNTAIN) {
            found = true;
            break;
        }
    }

    if (!found || fountain == NULL) {
        send_to_char("There is no fountain here!\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_DRINK_CON) {
        send_to_char("You can't fill that.\n\r", ch);
        return;
    }

    if (obj->value[1] != 0 && obj->value[2] != fountain->value[2]) {
        send_to_char("There is already another liquid in it.\n\r", ch);
        return;
    }

    if (obj->value[1] >= obj->value[0]) {
        send_to_char("Your container is full.\n\r", ch);
        return;
    }

    sprintf(buf, "You fill $p with %s from $P.", liq_table[fountain->value[2]].liq_name);
    act(buf, ch, obj, fountain, TO_CHAR);
    sprintf(buf, "$n fills $p with %s from $P.", liq_table[fountain->value[2]].liq_name);
    act(buf, ch, obj, fountain, TO_ROOM);

    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}


/***************************************************************************
 *	do_pour
 ***************************************************************************/
void do_pour(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *out;
    GAMEOBJECT *in;
    CHAR_DATA *vch = NULL;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    long amount;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0' || argument[0] == '\0') {
        send_to_char("Pour what into what?\n\r", ch);
        return;
    }

    if ((out = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You don't have that item.\n\r", ch);
        return;
    }

    if (out->item_type != ITEM_DRINK_CON) {
        send_to_char("That's not a drink container.\n\r", ch);
        return;
    }

    if (!str_cmp(argument, "out")) {
        if (out->value[1] == 0) {
            send_to_char("It's already empty.\n\r", ch);
            return;
        }

        out->value[1] = 0;
        out->value[3] = 0;
        sprintf(buf, "You invert $p, spilling %s all over the ground.", liq_table[out->value[2]].liq_name);
        act(buf, ch, out, NULL, TO_CHAR);

        sprintf(buf, "$n inverts $p, spilling %s all over the ground.", liq_table[out->value[2]].liq_name);
        act(buf, ch, out, NULL, TO_ROOM);
        return;
    }

    if ((in = get_obj_here(ch, argument)) == NULL) {
        vch = get_char_room(ch, argument);
        if (vch == NULL) {
            send_to_char("Pour into what?\n\r", ch);
            return;
        }

        in = get_eq_char(vch, WEAR_HOLD);
        if (in == NULL) {
            send_to_char("They aren't holding anything.", ch);
            return;
        }
    }

    if (in->item_type != ITEM_DRINK_CON) {
        send_to_char("You can only pour into other drink containers.\n\r", ch);
        return;
    }

    if (in == out) {
        send_to_char("You cannot change the laws of physics!\n\r", ch);
        return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2]) {
        send_to_char("They don't hold the same liquid.\n\r", ch);
        return;
    }

    if (out->value[1] == 0) {
        act("There's nothing in $p to pour.", ch, out, NULL, TO_CHAR);
        return;
    }

    if (in->value[1] >= in->value[0]) {
        act("$p is already filled to the top.", ch, in, NULL, TO_CHAR);
        return;
    }

    amount = UMIN(out->value[1], in->value[0] - in->value[1]);
    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];

    if (vch == NULL) {
        sprintf(buf, "You pour %s from $p into $P.", liq_table[out->value[2]].liq_name);
        act(buf, ch, out, in, TO_CHAR);
        sprintf(buf, "$n pours %s from $p into $P.", liq_table[out->value[2]].liq_name);
        act(buf, ch, out, in, TO_ROOM);
    } else {
        sprintf(buf, "You pour some %s for $N.", liq_table[out->value[2]].liq_name);
        act(buf, ch, NULL, vch, TO_CHAR);
        sprintf(buf, "$n pours you some %s.", liq_table[out->value[2]].liq_name);
        act(buf, ch, NULL, vch, TO_VICT);
        sprintf(buf, "$n pours some %s for $N.", liq_table[out->value[2]].liq_name);
        act(buf, ch, NULL, vch, TO_NOTVICT);
    }
}


/***************************************************************************
 *	do_drink
 ***************************************************************************/
void do_drink(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char arg[MAX_INPUT_LENGTH];
    long amount;
    long liquid;

    DENY_NPC(ch);

    (void)one_argument(argument, arg);
    if (arg[0] == '\0') {
        for (obj = ch->in_room->contents; obj; obj = obj->next_content)
            if (obj->item_type == ITEM_FOUNTAIN)
                break;

        if (obj == NULL) {
            send_to_char("Drink what?\n\r", ch);
            return;
        }
    } else {
        if ((obj = get_obj_here(ch, arg)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }


    switch (obj->item_type) {
      default:
          send_to_char("You can't drink from that.\n\r", ch);
          return;

      case ITEM_FOUNTAIN:
          if ((liquid = obj->value[2]) < 0) {
              log_bug("Do_drink: bad liquid number %ld.", liquid);
              liquid = obj->value[2] = 0;
          }
          amount = liq_table[liquid].liq_affect[4] * 3;
          break;

      case ITEM_DRINK_CON:
          if (obj->value[1] <= 0) {
              send_to_char("It is already empty.\n\r", ch);
              return;
          }

          if ((liquid = obj->value[2]) < 0) {
              log_bug("Do_drink: bad liquid number %ld.", liquid);
              liquid = obj->value[2] = 0;
          }

          amount = liq_table[liquid].liq_affect[4];
          amount = UMIN(amount, obj->value[1]);
          break;
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
        && ch->pcdata->condition[COND_FULL] > 48) {
        send_to_char("You're too full to drink more.\n\r", ch);
        return;
    }

    act("$n drinks $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_ROOM);
    act("You drink $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_CHAR);

    gain_condition(ch, COND_FULL, amount * liq_table[liquid].liq_affect[COND_FULL] / 4);
    if (ch->pcdata->condition[COND_THIRST] < 0
        && ch->pcdata->condition[COND_THIRST] > -151)
        ch->pcdata->condition[COND_THIRST] = 0;
    gain_condition(ch, COND_THIRST, amount * liq_table[liquid].liq_affect[COND_THIRST] / 10);
    if (ch->pcdata->condition[COND_HUNGER] >= 0)
        gain_condition(ch, COND_HUNGER, amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 35)
        send_to_char("You are full.\n\r", ch);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40)
        send_to_char("Your thirst is quenched.\n\r", ch);

    if (obj->value[3] != 0) {
        /* The drink was poisoned ! */
        AFFECT_DATA af;
        SKILL *skill_poison;

        if ((skill_poison = gsp_poison) != NULL) {
            act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM);
            send_to_char("You choke and gag.\n\r", ch);

            af.where = TO_AFFECTS;
            af.type = skill_poison->sn;
            af.skill = skill_poison;
            af.level = (int)(UMAX(LEVEL_HERO, number_fuzzy_long(amount)));
            af.duration = (int)(UMAX(60, number_fuzzy_long(3 * amount)));
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_POISON;
            affect_join(ch, &af);
        }
    }

    if (obj->value[0] > 0)
        obj->value[1] -= amount;

    return;
}



/***************************************************************************
 *	do_eat
 ***************************************************************************/
void do_eat(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char arg[MAX_INPUT_LENGTH];

    (void)one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Eat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (!IS_IMMORTAL(ch)) {
        if (obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL) {
            send_to_char("That's not edible.\n\r", ch);
            return;
        }

        if (obj->objprototype->vnum == OBJ_VNUM_BLANK_PILL) {
            send_to_char("That has to be 'sprinkled', not eaten.\n\r", ch);
            return;
        }

        if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 48) {
            send_to_char("You are too full to eat more.\n\r", ch);
            return;
        }
    }

    act("$n eats $p.", ch, obj, NULL, TO_ROOM);
    act("You eat $p.", ch, obj, NULL, TO_CHAR);

    switch (obj->item_type) {
      case ITEM_FOOD:
          if (!IS_NPC(ch)) {
              long condition;

              if (ch->pcdata->condition[COND_HUNGER] <= 0
                  && ch->pcdata->condition[COND_HUNGER] > -151)
                  ch->pcdata->condition[COND_HUNGER] = 0;

              condition = ch->pcdata->condition[COND_HUNGER];
              gain_condition(ch, COND_FULL, obj->value[0]);
              gain_condition(ch, COND_HUNGER, obj->value[1]);

              if (condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0)
                  send_to_char("You are no longer hungry.\n\r", ch);
              else if (ch->pcdata->condition[COND_FULL] > 30)
                  send_to_char("You are full.\n\r", ch);
          }

          if (obj->value[3] != 0) {
              AFFECT_DATA af;
              SKILL *skill_poison;

              if ((skill_poison = gsp_poison) != NULL) {
                  act("$n chokes and gags.", ch, 0, 0, TO_ROOM);
                  send_to_char("You choke and gag.\n\r", ch);

                  af.where = TO_AFFECTS;
                  af.type = skill_poison->sn;
                  af.skill = skill_poison;
                  af.level = (int)(UMAX(LEVEL_HERO, number_fuzzy_long(obj->value[0])));
                  af.duration = (int)(UMAX(60, number_fuzzy_long(2 * obj->value[0])));
                  af.location = APPLY_NONE;
                  af.modifier = 0;
                  af.bitvector = AFF_POISON;
                  affect_join(ch, &af);
              }
          }
          break;

      case ITEM_PILL:
          obj_cast_spell((int)obj->value[1], (int)(UMAX(LEVEL_HERO, obj->value[0])), ch, ch, NULL);
          obj_cast_spell((int)obj->value[2], (int)(UMAX(LEVEL_HERO, obj->value[0])), ch, ch, NULL);
          obj_cast_spell((int)obj->value[3], (int)(UMAX(LEVEL_HERO, obj->value[0])), ch, ch, NULL);
          obj_cast_spell((int)obj->value[4], (int)(UMAX(LEVEL_HERO, obj->value[0])), ch, ch, NULL);
          break;
    }

    extract_obj(obj);
    return;
}



/***************************************************************************
 *	remove_obj
 ***************************************************************************/
bool remove_obj(CHAR_DATA *ch, int iWear, bool fReplace)
{
    GAMEOBJECT *obj;

    if ((obj = get_eq_char(ch, iWear)) == NULL)
        return true;

    if (!fReplace)
        return false;

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE)) {
        act("You can't remove $p.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    unequip_char(ch, obj);

    act("$n stops using $p.", ch, obj, NULL, TO_ROOM);
    act("You stop using $p.", ch, obj, NULL, TO_CHAR);
    return true;
}



/***************************************************************************
 *	wear_obj
 ***************************************************************************/
void wear_obj(CHAR_DATA *ch, GAMEOBJECT *obj, bool fReplace)
{
    char buf[MAX_STRING_LENGTH];

    if ((2 * (ch->level) + 30) < obj->level) {
        sprintf(buf, "You must be level %d or that level, +1 to use this object.\n\r", (((obj->level) - 30) / 2));
        send_to_char(buf, ch);
        return;
    }

    if (obj->item_type == ITEM_LIGHT) {
        if (!remove_obj(ch, WEAR_LIGHT, fReplace))
            return;

        act("$n lights $p and holds it.", ch, obj, NULL, TO_ROOM);
        act("You light $p and hold it.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_LIGHT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
        /* 08-09-2002 - joe - added for human 4 rings*/
        if (get_eq_char(ch, WEAR_FINGER_L) != NULL
            && get_eq_char(ch, WEAR_FINGER_R) != NULL
            && (get_eq_char(ch, WEAR_FINGER_L2) != NULL)
            && (get_eq_char(ch, WEAR_FINGER_R2) != NULL)
            && !remove_obj(ch, WEAR_FINGER_L, fReplace)
            && !remove_obj(ch, WEAR_FINGER_R, fReplace)
            && (!remove_obj(ch, WEAR_FINGER_L2, fReplace))
            && (!remove_obj(ch, WEAR_FINGER_R2, fReplace)))
            return;

        if (get_eq_char(ch, WEAR_FINGER_L) == NULL) {
            act("$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your left finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_L);
            return;
        }

        if (get_eq_char(ch, WEAR_FINGER_R) == NULL) {
            act("$n wears $p on $s right finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your right finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_R);
            return;
        }

        /* 08-09-2002 - joe - added for human 4 rings*/
        if (get_eq_char(ch, WEAR_FINGER_L2) == NULL) {
            act("$n wears $p on $s second left finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your second left finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_L2);
            return;
        }

        if (get_eq_char(ch, WEAR_FINGER_R2) == NULL) {
            act("$n wears $p on $s second right finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your second right finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_R2);
            return;
        }

        log_bug("Wear_obj: no free finger.");
        send_to_char("You already wear two rings.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
        if (get_eq_char(ch, WEAR_NECK_1) != NULL
            && get_eq_char(ch, WEAR_NECK_2) != NULL
            && !remove_obj(ch, WEAR_NECK_1, fReplace)
            && !remove_obj(ch, WEAR_NECK_2, fReplace))
            return;


        if (get_eq_char(ch, WEAR_NECK_1) == NULL) {
            act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_NECK_1);
            return;
        }

        if (get_eq_char(ch, WEAR_NECK_2) == NULL) {
            act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_NECK_2);
            return;
        }

        log_bug("Wear_obj: no free neck.");
        send_to_char("You already wear two neck items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
        if (!remove_obj(ch, WEAR_BODY, fReplace))
            return;

        act("$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your torso.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_BODY);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
        if (!remove_obj(ch, WEAR_HEAD, fReplace))
            return;
        act("$n wears $p on $s head.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your head.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HEAD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FACE)) {
        if (!remove_obj(ch, WEAR_FACE, fReplace))
            return;
        act("$n wears $p on $s face.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your face.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_FACE);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_EAR)) {
        if (get_eq_char(ch, WEAR_EAR_L) != NULL
            && get_eq_char(ch, WEAR_EAR_R) != NULL
            && !remove_obj(ch, WEAR_EAR_L, fReplace)
            && !remove_obj(ch, WEAR_EAR_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_EAR_L) == NULL) {
            act("$n wears $p in $s left ear.", ch, obj, NULL, TO_ROOM);
            act("You wear $p in your left ear.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_EAR_L);
            return;
        }

        if (get_eq_char(ch, WEAR_EAR_R) == NULL) {
            act("$n wears $p in $s right ear.", ch, obj, NULL, TO_ROOM);
            act("You wear $p in your right ear.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_EAR_R);
            return;
        }

        log_bug("Wear_obj: no free ear.");
        send_to_char("You already wear two ear items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
        if (!remove_obj(ch, WEAR_LEGS, fReplace))
            return;
        act("$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your legs.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_LEGS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
        if (!remove_obj(ch, WEAR_FEET, fReplace))
            return;
        act("$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your feet.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_FEET);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
        if (!remove_obj(ch, WEAR_HANDS, fReplace))
            return;
        act("$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your hands.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HANDS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ARMS)) {
        if (!remove_obj(ch, WEAR_ARMS, fReplace))
            return;
        act("$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your arms.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_ARMS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
        if (!remove_obj(ch, WEAR_ABOUT, fReplace))
            return;
        act("$n wears $p about $s torso.", ch, obj, NULL, TO_ROOM);
        act("You wear $p about your torso.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_ABOUT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
        if (!remove_obj(ch, WEAR_WAIST, fReplace))
            return;
        act("$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM);
        act("You wear $p about your waist.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_WAIST);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
        if (get_eq_char(ch, WEAR_WRIST_L) != NULL
            && get_eq_char(ch, WEAR_WRIST_R) != NULL
            && !remove_obj(ch, WEAR_WRIST_L, fReplace)
            && !remove_obj(ch, WEAR_WRIST_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_WRIST_L) == NULL) {
            act("$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your left wrist.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_WRIST_L);
            return;
        }

        if (get_eq_char(ch, WEAR_WRIST_R) == NULL) {
            act("$n wears $p around $s right wrist.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your right wrist.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_WRIST_R);
            return;
        }

        log_bug("Wear_obj: no free wrist.");
        send_to_char("You already wear two wrist items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
        GAMEOBJECT *weapon;

        if (!remove_obj(ch, WEAR_SHIELD, fReplace))
            return;

        weapon = get_eq_char(ch, WEAR_WIELD);
        if (weapon != NULL && ch->size < SIZE_LARGE
            && IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)) {
            send_to_char("Your hands are tied up with your weapon!\n\r", ch);
            return;
        }

        if (get_eq_char(ch, WEAR_SECONDARY) != NULL) {
            send_to_char("You cannot use a shield while using a secondary weapon.\n\r", ch);
            return;
        }

        act("$n wears $p as a shield.", ch, obj, NULL, TO_ROOM);
        act("You wear $p as a shield.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_SHIELD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WIELD)) {
        SKILL *skill_h2h;
        int sn;
        int skill;

        if (!remove_obj(ch, WEAR_WIELD, fReplace))
            return;

        if (!IS_NPC(ch)
            && get_obj_weight(obj) > get_wield_weight(ch)) {
            send_to_char("It is too heavy for you to wield.\n\r", ch);
            return;
        }

        if (!IS_NPC(ch) && ch->size < SIZE_LARGE
            && IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
            && get_eq_char(ch, WEAR_SHIELD) != NULL
            && get_eq_char(ch, WEAR_SECONDARY) != NULL) {
            send_to_char("You need two hands free for that weapon.\n\r", ch);
            return;
        }

        act("$n wields $p.", ch, obj, NULL, TO_ROOM);
        act("You wield $p.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_WIELD);

        sn = get_weapon_sn(ch, NULL);

        if ((skill_h2h = gsp_hand_to_hand) == NULL
            || skill_h2h->sn == skill_h2h->sn)
            return;

        skill = get_weapon_skill(ch, sn);
        if (skill >= 100)
            act("$p feels like a part of you!", ch, obj, NULL, TO_CHAR);
        else if (skill > 85)
            act("You feel quite confident with $p.", ch, obj, NULL, TO_CHAR);
        else if (skill > 70)
            act("You are skilled with $p.", ch, obj, NULL, TO_CHAR);
        else if (skill > 50)
            act("Your skill with $p is adequate.", ch, obj, NULL, TO_CHAR);
        else if (skill > 25)
            act("$p feels a little clumsy in your hands.", ch, obj, NULL, TO_CHAR);
        else if (skill > 1)
            act("You fumble and almost drop $p.", ch, obj, NULL, TO_CHAR);
        else
            act("You don't even know which end is up on $p.", ch, obj, NULL, TO_CHAR);

        return;
    }

    if (CAN_WEAR(obj, ITEM_HOLD)) {
        if (!remove_obj(ch, WEAR_HOLD, fReplace))
            return;

        if ((get_eq_char(ch, WEAR_SECONDARY) != NULL) && (!IS_IMMORTAL(ch))) {
            send_to_char("You cannot hold an item while using a secondary weapon.\n\r", ch);
            return;
        }

        act("$n holds $p in $s hand.", ch, obj, NULL, TO_ROOM);
        act("You hold $p in your hand.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HOLD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_TATTOO)) {
        if (!remove_obj(ch, WEAR_TATTOO, fReplace))
            return;
        act("$n wears $p as a tattoo.", ch, obj, NULL, TO_ROOM);
        act("You wear $p as a tattoo.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_TATTOO);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FLOAT)) {
        if (!remove_obj(ch, WEAR_FLOAT, fReplace))
            return;
        act("$n releases $p to float next to $m.", ch, obj, NULL, TO_ROOM);
        act("You release $p and it floats next to you.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_FLOAT);
        return;
    }

    if (fReplace)
        send_to_char("You can't wear, wield, or hold that.\n\r", ch);

    return;
}



/***************************************************************************
 *	do_wear
 ***************************************************************************/
void do_wear(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char arg[MAX_INPUT_LENGTH];

    (void)one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Wear, wield, or hold what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        GAMEOBJECT *obj_next;

        for (obj = ch->carrying; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;
            if (obj->wear_loc == WEAR_NONE
                && can_see_obj(ch, obj))
                wear_obj(ch, obj, false);
        }
        return;
    } else {
        if ((obj = get_obj_carry(ch, arg)) == NULL) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        wear_obj(ch, obj, true);
    }

    return;
}

/***************************************************************************
 *	do_remove
 ***************************************************************************/
void do_remove(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char arg[MAX_INPUT_LENGTH];

    (void)one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Remove what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        GAMEOBJECT *obj_next;

        for (obj = ch->carrying; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;
            if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj))
                remove_obj(ch, obj->wear_loc, true);
        }
        return;
    } else if ((obj = get_obj_wear(ch, arg)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    remove_obj(ch, obj->wear_loc, true);
    return;
}



/***************************************************************************
 *	do_sacrifice
 ***************************************************************************/
void do_sacrifice(CHAR_DATA *ch, const char *argument)
{
    static char *god_name_table[] =
    {
        "`1L`!a`Or`!a`1h``", "`#P`3ee`#j``", "`&Mota``", "`5E`Po``", "`2D`@a`8i`7g`@e`2n``", "`2N`8i`2BB``", "`OP`^u`6c`7k``", "`&Sa`8to`&ri``"
    };

    GAMEOBJECT *obj;
    CHAR_DATA *gch;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int MAX_GOD_NAME;
    char *god;
    unsigned int silver = 0l;
    int count;

    MAX_GOD_NAME = 7;

    (void)one_argument(argument, arg);

    if (arg[0] == '\0' || !str_cmp(arg, ch->name)) {
        act("$n offers $mself to `2A`8n`3a`2r`3c`8h``, who graciously declines.", ch, NULL, NULL, TO_ROOM);
        send_to_char("`2A`8n`3a`2r`3c`8h`` appreciates your offer and may accept it later.\n\r", ch);
        return;
    }

    god = god_name_table[number_range(0, MAX_GOD_NAME)];

    if (!str_cmp(arg, "all")) {
        GAMEOBJECT *obj_next;
        long total;

        total = 0;
        count = 0;
        for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;

            /* ignore PC corpses that are not empty */
            if (obj->item_type == ITEM_CORPSE_PC
                && obj->contains)
                continue;

            /* if it is not ITEM_TAKE or it is ITEM_NO_SAC ignore it */
            if (!CAN_WEAR(obj, ITEM_TAKE)
                || CAN_WEAR(obj, ITEM_NO_SAC))
                continue;


            /* check furniture */
            if (obj->in_room != NULL) {
                int count;

                count = 0;
                for (gch = obj->in_room->people;
                     gch != NULL;
                     gch = gch->next_in_room) {
                    if (gch->on == obj) {
                        count++;
                        break;
                    }
                }
                if (count > 0)
                    continue;
            }

            silver = (unsigned int)UMAX(1, obj->level * 3);
            if (obj->item_type != ITEM_CORPSE_NPC
                && obj->item_type != ITEM_CORPSE_PC)
                silver = UMIN(silver, obj->cost);

            extract_obj(obj);
            total += silver;

            if (++count > MAX_GET)
                break;
        }

        if (count > 0) {
            send_to_char("You go on a crazy sacrificing spree!!\n\r", ch);
            WAIT_STATE(ch, PULSE_VIOLENCE);
        }

        ch->silver += total;
        if (total == 1)
            printf_to_char(ch, "%s gives you a silver coin for your sacrifice.\n\r", god);
        else
            printf_to_char(ch, "%s gives you %d silver coins for your sacrifice.\n\r", god, total);

        /* message to the room */
        wiznet("$N sends up everything in their as a burnt offering.", ch, NULL, WIZ_SACCING, 0, 0);
        if (count <= MAX_GET)
            act("$n sacrifices everything in the room.", ch, NULL, NULL, TO_ROOM);
        else
            act("$n sacrifices a lot of items in the room.", ch, NULL, NULL, TO_ROOM);


        /* autosplit */
        if (IS_SET(ch->act, PLR_AUTOSPLIT) && silver > 1) {
            count = 0;
            for (gch = ch->in_room->people;
                 gch != NULL;
                 gch = gch->next_in_room) {
                if (is_same_group(gch, ch)) {
                    if (++count > 1) {
                        sprintf(buf, "%ld", total);
                        do_split(ch, buf);
                        break;
                    }
                }
            }
        }
    } else {
        obj = get_obj_list(ch, arg, ch->in_room->contents);
        if (obj == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }

        if (obj->item_type == ITEM_CORPSE_PC && obj->contains) {
            send_to_char("No one would like that.\n\r", ch);
            return;
        }

        if (!CAN_WEAR(obj, ITEM_TAKE)
            || CAN_WEAR(obj, ITEM_NO_SAC)) {
            act("$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR);
            return;
        }

        if (obj->in_room != NULL) {
            for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room) {
                if (gch->on == obj) {
                    act("$N appears to be using $p.", ch, obj, gch, TO_CHAR);
                    return;
                }
            }
        }

        silver = (unsigned int)UMAX(1, obj->level * 3);
        if (obj->item_type != ITEM_CORPSE_NPC
            && obj->item_type != ITEM_CORPSE_PC)
            silver = UMIN(silver, obj->cost);

        if (silver == 1)
            printf_to_char(ch, "%s gives you a silver coin for your sacrifice.\n\r", god);
        else
            printf_to_char(ch, "%s gives you %ld silver coins for your sacrifice.\n\r", god, silver);

        ch->silver += silver;
        if (IS_SET(ch->act, PLR_AUTOSPLIT) && silver > 1) {
            count = 0;
            for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
                if (is_same_group(gch, ch)) {
                    if (count++ > 1) {
                        sprintf(buf, "%u", silver);
                        do_split(ch, buf);
                        break;
                    }
                }
            }
        }

        act("$n sacrifices $p.", ch, obj, NULL, TO_ROOM);
        wiznet("$N sends up $p as a burnt offering.", ch, obj, WIZ_SACCING, 0, 0);
        extract_obj(obj);
    }
}

/***************************************************************************
 *	donate_obj
 ***************************************************************************/
void donate_obj(CHAR_DATA *ch, GAMEOBJECT *obj)
{
    char buf[MAX_INPUT_LENGTH];

    if ((!CAN_WEAR(obj, ITEM_TAKE)
         && obj->item_type != ITEM_CORPSE_PC)
        || (IS_SET(obj->extra2_flags, ITEM2_NODONATE)
            && !IS_IMMORTAL(ch))) {
        send_to_char("You can't donate that!\n\r", ch);
        return;
    }

    if (count_users(obj) > 0)
        return;

    send_to_char("Your donation is greatly appreciated\n\r", ch);
    sprintf(buf, "%s disappears in a puff of smoke\n\r", obj->short_descr);
    send_to_char(buf, ch);

    act(buf, ch, NULL, NULL, TO_ROOM);
    obj_from_room(obj);
    obj_to_room(obj, get_room_index(ROOM_VNUM_DONATION));

    for (ch = char_list; ch != NULL; ch = ch->next) {
        if (ch->in_room == get_room_index(ROOM_VNUM_DONATION)) {
            char *temp;

            temp = obj->short_descr;
            if (temp[0] > 'a' && temp[0] < 'z')
                temp[0] = temp[0] - (char)32;

            sprintf(buf, "%s falls from the heavens and lands by your feet.\n\r", temp);
            send_to_char(buf, ch);
        }
    }
}

/***************************************************************************
 *	do_donate
 ***************************************************************************/
void do_donate(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char arg[MAX_INPUT_LENGTH];
    GAMEOBJECT *obj_next;
    bool found;
    int max_don;

    (void)one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Donate What?!\n\r", ch);
        return;
    }

    if (ch->in_room == get_room_index(ROOM_VNUM_DONATION)) {
        send_to_char("Soo.. tell me..  why would you want to do that?!", ch);
        return;
    }

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        obj = get_obj_list(ch, arg, ch->in_room->contents);
        /* this should be a better call */
        if (obj == NULL) {
            obj = get_obj_list(ch, arg, ch->carrying);
            if (obj == NULL) {
                send_to_char("I don't see that here!\n\r", ch);
                return;
            }

            if (obj->wear_loc != WEAR_NONE) {
                send_to_char("You can't donate something you are wearing.\n\r", ch);
                return;
            }

            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
        }
        donate_obj(ch, obj);
    } else {
        found = false;
        max_don = 0;
        for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;
            if ((arg[3] == '\0' || is_name(&arg[4], object_name_get(obj)))
                && count_users(obj) == 0
                && can_see_obj(ch, obj)) {
                found = true;
                max_don++;
                if (max_don > 150) {
                    send_to_char("Woah, 150 things at once is enough!\n\r", ch);
                    return;
                }
                donate_obj(ch, obj);
            }
        }

        if (!found)
            send_to_char("I don't see that here.\n\r", ch);
    }
}





/***************************************************************************
 *	do_steal
 ***************************************************************************/
void do_steal(CHAR_DATA *ch, const char *argument)
{
    CHAR_DATA *victim;
    GAMEOBJECT *obj;
    SKILL *skill_steal;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int percent;
    int learned;

    if ((skill_steal = skill_lookup("steal")) == NULL) {
        send_to_char("Huh?", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Steal what from whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("That's pointless.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (!IS_NPC(victim)) {
        send_to_char("You cannot steal from other players.\n\r", ch);
        return;
    }

    if (IS_NPC(victim)
        && victim->position == POS_FIGHTING) {
        send_to_char("You'd better not -- you might get hit.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_steal->wait);
    percent = number_percent();
    if ((learned = get_learned_percent(ch, skill_steal)) >= 1)
        percent += (IS_AWAKE(victim) ? 10 : -50);

    if (((ch->level + 7 < victim->level
          || ch->level - 7 > victim->level)
         && !IS_NPC(victim) && !IS_NPC(ch))
        || (!IS_NPC(ch) && percent > learned)
        || (IS_IMMORTAL(victim))) {
        send_to_char("Oops.\n\r", ch);
        act("$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT);
        act("$n tried to steal from $N.\n\r", ch, NULL, victim, TO_NOTVICT);
        switch (number_range(0, 3)) {
          case 0:
              sprintf(buf, "%s is a lousy thief!", ch->name);
              break;
          case 1:
              sprintf(buf, "%s couldn't rob %s way out of a paper bag!", ch->name, (ch->sex == 2) ? "her" : "his");
              break;
          case 2:
              sprintf(buf, "%s tried to rob me!", ch->name);
              break;
          case 3:
              sprintf(buf, "Keep your hands out of there, %s!", ch->name);
              break;
        }
        broadcast_channel(victim, channels_find(CHANNEL_SHOUT), NULL, buf);
        if (!IS_NPC(ch)) {
            if (IS_NPC(victim)) {
                check_improve(ch, skill_steal, false, 2);
                multi_hit(victim, ch, TYPE_UNDEFINED);
            } else {
                sprintf(buf, "$N tried to steal from %s.", victim->name);
                wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);

                if (!IS_SET(ch->act, PLR_THIEF) && !IS_NPC(ch)
                    && !IS_IMMORTAL(ch)) {
                    SET_BIT(ch->act, PLR_THIEF);
                    send_to_char("*** You are now a THIEF!! ***\n\r", ch);
                    ch->pcdata->thief_time = time(NULL);
                    save_char_obj(ch);
                }
            }
        }

        return;
    }

    if (!str_cmp(arg1, "coin")
        || !str_cmp(arg1, "coins")
        || !str_cmp(arg1, "gold")
        || !str_cmp(arg1, "silver")) {
        unsigned int gold;
        unsigned int silver;

        gold = UMAX(victim->gold, (unsigned int)(victim->gold * number_range(1, ch->level) / 60));
        silver = UMAX(victim->silver, (unsigned int)(victim->silver * number_range(1, ch->level) / 60));

        ch->gold += gold;
        ch->silver += silver;
        victim->silver -= silver;
        victim->gold -= gold;

        if (silver == 0)
            sprintf(buf, "Bingo!  You got %u gold coins.\n\r", gold);
        else if (gold == 0)
            sprintf(buf, "Bingo!  You got %u silver coins.\n\r", silver);
        else
            sprintf(buf, "Bingo!  You got %u silver and %u gold coins.\n\r", silver, gold);

        send_to_char(buf, ch);
        check_improve(ch, skill_steal, true, 2);
        return;
    }

    if ((obj = get_obj_carry(victim, arg1)) == NULL) {
        send_to_char("You can't find it.\n\r", ch);
        return;
    }

    if (!can_drop_obj(ch, obj)
        || IS_SET(obj->extra_flags, ITEM_INVENTORY)
        || obj->level > ch->level) {
        send_to_char("You can't pry it away.\n\r", ch);
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
        send_to_char("You have your hands full.\n\r", ch);
        return;
    }

    if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
        send_to_char("You can't carry that much weight.\n\r", ch);
        return;
    }

    if (!can_see_obj(ch, get_obj_carry(victim, arg1))) {
        send_to_char("You cannot see that.\n\r", ch);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, ch);
    check_improve(ch, skill_steal, true, 2);
    send_to_char("Got it!\n\r", ch);
    return;
}



/***************************************************************************
 *	find_keeper
 ***************************************************************************/
CHAR_DATA *find_keeper(CHAR_DATA *ch)
{
    CHAR_DATA *keeper;
    struct shop_data *shop;

    shop = NULL;
    for (keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room) {
        if (IS_SHOPKEEPER(keeper)) {
            shop = keeper->mob_idx->shop;
            break;
        }
    }

    if (shop == NULL) {
        send_to_char("You can't do that here.\n\r", ch);
        return NULL;
    }

    if (globalGameState.gametime->hour < shop->open_hour) {
        broadcast_channel(keeper, channels_find(CHANNEL_SAY), NULL, "Sorry, I am closed. Come back later.");
        return NULL;
    }

    if (globalGameState.gametime->hour > shop->close_hour) {
        broadcast_channel(keeper, channels_find(CHANNEL_SAY), NULL, "Sorry, I am closed. Come back tomorrow.");
        return NULL;
    }

    if (!can_see(keeper, ch)) {
        broadcast_channel(keeper, channels_find(CHANNEL_SAY), NULL, "I don't trade with folks I can't see.");
        return NULL;
    }

    return keeper;
}


/***************************************************************************
 *	obj_to_keeper
 ***************************************************************************/
void obj_to_keeper(GAMEOBJECT *obj, CHAR_DATA *ch)
{
    GAMEOBJECT *t_obj;
    GAMEOBJECT *t_obj_next;

    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next) {
        t_obj_next = t_obj->next_content;

        if (obj->objprototype == t_obj->objprototype
            && !str_cmp(obj->short_descr, t_obj->short_descr)) {
            if (IS_OBJ_STAT(t_obj, ITEM_INVENTORY)) {
                extract_obj(obj);
                return;
            }
            obj->cost = t_obj->cost;        /* keep it standard */
            break;
        }
    }

    if (t_obj == NULL) {
        obj->next_content = ch->carrying;
        ch->carrying = obj;
    } else {
        obj->next_content = t_obj->next_content;
        t_obj->next_content = obj;
    }

    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += get_obj_number(obj);
    ch->carry_weight += get_obj_weight(obj);
}


/***************************************************************************
 *	get_obj_keeper
 ***************************************************************************/
GAMEOBJECT *get_obj_keeper(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument)
{
    GAMEOBJECT *obj;
    char arg[MAX_INPUT_LENGTH];
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    for (obj = keeper->carrying; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc == WEAR_NONE
            && can_see_obj(keeper, obj)
            && can_see_obj(ch, obj)
            && is_name(arg, object_name_get(obj))) {
            if (++count == number)
                return obj;

            while (obj->next_content != NULL &&
                   obj->objprototype == obj->next_content->objprototype &&
                   !str_cmp(obj->short_descr, obj->next_content->short_descr))
                obj = obj->next_content;
        }
    }

    return NULL;
}


/***************************************************************************
 *	get_cost
 ***************************************************************************/
unsigned int get_cost(CHAR_DATA *keeper, GAMEOBJECT *obj, bool fBuy)
{
    unsigned int cost;

    if (obj == NULL || !IS_SHOPKEEPER(keeper))
        return 0;

    if (fBuy) {
        cost = (unsigned int)(obj->cost * keeper->mob_idx->shop->profit_buy / 100);
    } else {
        GAMEOBJECT *obj_inv;
        int itype;

        cost = 0;
        for (itype = 0; itype < MAX_TRADE; itype++) {
            if (obj->item_type == keeper->mob_idx->shop->buy_type[itype]) {
                cost = (unsigned int)(obj->cost * keeper->mob_idx->shop->profit_sell / 100);
                break;
            }
        }

        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)) {
            for (obj_inv = keeper->carrying;
                 obj_inv != NULL;
                 obj_inv = obj_inv->next_content) {
                if (obj->objprototype == obj_inv->objprototype
                    && !str_cmp(obj->short_descr, obj_inv->short_descr)) {
                    if (IS_OBJ_STAT(obj_inv, ITEM_INVENTORY))
                        cost /= 2;
                    else
                        cost = cost * 3 / 4;
                }
            }
        }
    }

    if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND) {
        if (obj->value[1] == 0)
            cost /= 4;
        else
            cost = cost * (unsigned int)obj->value[2] / (unsigned int)obj->value[1];
    }

    return cost;
}


/***************************************************************************
 *	do_buy
 ***************************************************************************/
void do_buy(CHAR_DATA *ch, const char *argument)
{
    SKILL *skill;
    char buf[MAX_STRING_LENGTH];
    unsigned int cost;
    int roll;

    skill = gsp_haggle;
    if (IS_NPC(ch)) {
        send_to_char("Mobs can't do that.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("Buy what?\n\r", ch);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        char arg[MAX_INPUT_LENGTH];
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *pet;
        struct room_index_data *pRoomIndexNext;
        struct room_index_data *in_room;

        if (IS_NPC(ch))
            return;

        argument = argument;
        argument = one_argument(argument, arg);

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);

        if (pRoomIndexNext == NULL) {
            log_bug("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
            send_to_char("Sorry, you can't buy that here.\n\r", ch);
            return;
        }

        in_room = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet = get_char_room(ch, arg);
        ch->in_room = in_room;

        if (pet == NULL || !IS_SET(pet->act, ACT_PET)) {
            send_to_char("Sorry, you can't buy that here.\n\r", ch);
            return;
        }

        if (ch->pet != NULL) {
            send_to_char("You already own a pet.\n\r", ch);
            return;
        }

        cost = (unsigned int)(10 * pet->level * pet->level);

        if ((unsigned int)(ch->silver * 100 + ch->gold) < cost) {
            send_to_char("You can't afford it.\n\r", ch);
            return;
        }

        if (ch->level < pet->level) {
            send_to_char("You're not powerful enough to master this pet.\n\r", ch);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (roll < get_learned_percent(ch, skill)) {
            cost -= (unsigned int)(cost / 2 * roll / 100);
            sprintf(buf, "You haggle the price down to %u coins.\n\r", cost);
            send_to_char(buf, ch);
            check_improve(ch, skill, true, 4);
        }

        deduct_cost(ch, cost);
        pet = create_mobile(pet->mob_idx);
        SET_BIT(pet->act, ACT_PET);
        SET_BIT(pet->affected_by, AFF_CHARM);

        pet->channels_denied = CHANNEL_SHOUT | CHANNEL_TELL;
        pet->zone = ch->in_room->area;

        argument = one_argument(argument, arg);
        if (arg[0] != '\0') {
            sprintf(buf, "%s %s", pet->name, arg);
            free_string(pet->name);
            pet->name = str_dup(buf);
        }

        sprintf(buf, "%sA neck tag says 'I belong to %s'.\n\r", pet->description, ch->name);
        free_string(pet->description);
        pet->description = str_dup(buf);

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        pet->leader = ch;
        ch->pet = pet;
        send_to_char("Enjoy your pet.\n\r", ch);
        act("$n bought $N as a pet.", ch, NULL, pet, TO_ROOM);
        return;
    } else {
        CHAR_DATA *keeper;
        GAMEOBJECT *obj;
        GAMEOBJECT *t_obj;
        char arg[MAX_INPUT_LENGTH];
        int number;
        int count = 1;

        if ((keeper = find_keeper(ch)) == NULL)
            return;

        number = mult_argument(argument, arg);
        obj = get_obj_keeper(ch, keeper, arg);
        cost = get_cost(keeper, obj, true);

        if (number > 50 || number < 1) {
            act("`@$n tells you '`1Are you insane?`@'``", keeper, NULL, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (cost == 0 || !can_see_obj(ch, obj)) {
            act("$n tells you 'I don't sell that -- try 'list''.", keeper, NULL, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (!IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
            for (t_obj = obj->next_content;
                 count < number && t_obj != NULL;
                 t_obj = t_obj->next_content) {
                if (t_obj->objprototype == obj->objprototype
                    && !str_cmp(t_obj->short_descr, obj->short_descr))
                    count++;
                else
                    break;
            }

            if (count < number) {
                act("$n tells you 'I don't have that many in stock.",
                    keeper, NULL, ch, TO_VICT);
                ch->reply = keeper;
                return;
            }
        }

        if (number < 1) {
            send_to_char("Sorry, no cheating allowed.\n\r", ch);
            log_string("%s tried to use the negative buy bug.", ch->name);
            return;
        }



        /*		if((ch->silver + ch->gold * 100) < cost * number)*/
        if ((ch->gold * 100 + ch->silver) < cost * number) {
            if (number > 1)
                act("$n tells you 'You can't afford to buy that many.", keeper, obj, ch, TO_VICT);
            else
                act("$n tells you 'You can't afford to buy $p'.", keeper, obj, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (obj->level > ch->level) {
            act("$n tells you 'You can't use $p yet'.",
                keeper, obj, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (ch->carry_number + (number * get_obj_number(obj)) > can_carry_n(ch)) {
            send_to_char("You can't carry that many items.\n\r", ch);
            return;
        }

        if (ch->carry_weight + (number * get_obj_weight(obj)) > can_carry_w(ch)) {
            send_to_char("You can't carry that much weight.\n\r", ch);
            return;
        }


        /* haggle */
        roll = number_percent();
        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)
            && roll < get_learned_percent(ch, skill)) {
            cost -= obj->cost / 2 * roll / 100;
            act("You haggle with $N.", ch, NULL, keeper, TO_CHAR);
            check_improve(ch, skill, true, 4);
        }

        if (number > 50) {
            number = 50;
            send_to_char("Sorry, you can't buy more than 50 items at a time.\n\r", ch);
        }

        if (number > 1) {
            sprintf(buf, "$n buys $p[%d].", number);
            act(buf, ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You buy $p[%d] for %u silver.", number, cost * (unsigned int)number);
            act(buf, ch, obj, NULL, TO_CHAR);
        } else {
            act("$n buys $p.", ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You buy $p for %u silver.", cost);
            act(buf, ch, obj, NULL, TO_CHAR);
        }
        deduct_cost(ch, cost * number);
        keeper->gold += cost * number / 100;
        keeper->silver += cost * number - (cost * number / 100) * 100;

        for (count = 0; count < number; count++) {
            if (IS_SET(obj->extra_flags, ITEM_INVENTORY)) {
                t_obj = create_object(obj->objprototype, obj->level);
            } else {
                t_obj = obj;
                obj = obj->next_content;
                obj_from_char(t_obj);
            }

            if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj, ITEM_HAD_TIMER))
                t_obj->timer = 0;
            REMOVE_BIT(t_obj->extra_flags, ITEM_HAD_TIMER);
            obj_to_char(t_obj, ch);
            if (cost < t_obj->cost)
                t_obj->cost = cost;
        }
    }
}


/***************************************************************************
 *	do_list
 ***************************************************************************/
void do_list(CHAR_DATA *ch, const char *argument)
{
    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        struct room_index_data *pRoomIndexNext;
        CHAR_DATA *pet;
        bool found;

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);

        if (pRoomIndexNext == NULL) {
            log_bug("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
            send_to_char("You can't do that here.\n\r", ch);
            return;
        }

        found = false;
        for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room) {
            if (IS_SET(pet->act, ACT_PET)) {
                if (!found) {
                    found = true;
                    send_to_char("Pets for sale:\n\r", ch);
                }

                printf_to_char(ch, "[%2d] %8d - %s\n\r", pet->level, 10 * pet->level * pet->level, pet->short_descr);
            }
        }

        if (!found)
            send_to_char("Sorry, we're out of pets right now.\n\r", ch);
        return;
    } else {
        CHAR_DATA *keeper;
        GAMEOBJECT *obj;
        unsigned int cost;
        int count;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == NULL)
            return;
        one_argument(argument, arg);

        found = false;
        for (obj = keeper->carrying; obj; obj = obj->next_content) {
            if (obj->wear_loc == WEAR_NONE
                && can_see_obj(ch, obj)
                && (cost = get_cost(keeper, obj, true)) > 0
                && (arg[0] == '\0' || is_name(arg, object_name_get(obj)))) {
                if (!found) {
                    found = true;
                    send_to_char("[Lv Price Qty] Item\n\r", ch);
                }

                if (IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
                    printf_to_char(ch, "[%2d %10u -- ] %s\n\r", obj->level, cost, obj->short_descr);
                } else {
                    count = 1;

                    while (obj->next_content != NULL &&
                           obj->objprototype == obj->next_content->objprototype &&
                           !str_cmp(obj->short_descr, obj->next_content->short_descr)) {
                        obj = obj->next_content;
                        count++;
                    }
                    printf_to_char(ch, "[%2d %10u %2d ] %s\n\r", obj->level, cost, count, obj->short_descr);
                }
            }
        }

        if (!found)
            send_to_char("You can't buy anything here.\n\r", ch);
        return;
    }
}


/***************************************************************************
 *	do_sell
 ***************************************************************************/
void do_sell(CHAR_DATA *ch, const char *argument)
{
    CHAR_DATA *keeper;
    GAMEOBJECT *obj;
    SKILL *skill_haggle;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    unsigned int cost;
    int roll;

    (void)one_argument(argument, arg);
    skill_haggle = gsp_haggle;

    if (arg[0] == '\0') {
        send_to_char("Sell what?\n\r", ch);
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
        return;

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        act("$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT);
        ch->reply = keeper;
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
        return;
    }

    if ((cost = get_cost(keeper, obj, false)) == 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    /*   if(cost >(keeper->silver + 100 * keeper->gold))
     * {
     *             act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.",
     *                      keeper, obj, ch, TO_VICT);
     *             return;
     * }
     */

    act("$n sells $p.", ch, obj, NULL, TO_ROOM);

    roll = number_percent();
    if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)
        && roll < get_learned_percent(ch, skill_haggle)) {
        send_to_char("You haggle with the shopkeeper.\n\r", ch);

        cost += obj->cost / 2 * roll / 100;
        cost = UMIN(cost, (unsigned int)(95 * get_cost(keeper, obj, true) / 100));
        cost = UMIN(cost, (unsigned int)(keeper->silver + 100 * keeper->gold));

        check_improve(ch, skill_haggle, true, 4);
    }

    sprintf(buf, "You sell $p for %u silver and %u gold piece%s.",
            (unsigned int)(cost - (cost / 100) * 100),
            (unsigned int)(cost / 100), cost == 1 ? "" : "s");
    act(buf, ch, obj, NULL, TO_CHAR);
    ch->gold += cost / 100;
    ch->silver += cost - (cost / 100) * 100;

    if (obj->item_type == ITEM_TRASH
        || IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)) {
        extract_obj(obj);
    } else {
        obj_from_char(obj);
        if (obj->timer)
            SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        else
            obj->timer = number_range(50, 100);
        obj_to_keeper(obj, keeper);
    }

    return;
}

/***************************************************************************
 *	do_value
 ***************************************************************************/
void do_value(CHAR_DATA *ch, const char *argument)
{
    CHAR_DATA *keeper;
    GAMEOBJECT *obj;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    unsigned int cost;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Value what?\n\r", ch);
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
        return;

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        act("$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT);
        ch->reply = keeper;
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if ((cost = get_cost(keeper, obj, false)) == 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    sprintf(buf, "$n tells you 'I'll give you %u silver and %u gold coins for $p'.",
            (unsigned int)(cost - (cost / 100) * 100), (unsigned int)(cost / 100));
    act(buf, keeper, obj, ch, TO_VICT);
    ch->reply = keeper;
    return;
}


/***************************************************************************
 *	do_second
 ***************************************************************************/
void do_second(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        send_to_char("Wear which weapon in your off-hand?\n\r", ch);
        return;
    }

    obj = get_obj_carry(ch, argument);
    if (obj == NULL) {
        send_to_char("You have no such thing.\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_WEAPON) {
        send_to_char("You can only wield a WEAPON .. Bonehead.\n\r", ch);
        return;
    }

    if ((get_eq_char(ch, WEAR_SHIELD) != NULL) ||
        (get_eq_char(ch, WEAR_HOLD) != NULL)) {
        send_to_char("You cannot use a secondary weapon while using a shield or holding an item\n\r", ch);
        return;
    }


    if (ch->level < obj->level) {
        sprintf(buf, "You must be level %d to second wield this weapon.\n\r",
                obj->level);
        send_to_char(buf, ch);
        act("$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == NULL) {
        send_to_char("You need to wield a primary weapon, before using a secondary one!\n\r", ch);
        return;
    }

    if ((get_obj_weight(obj) * 2) > get_obj_weight(get_eq_char(ch, WEAR_WIELD))) {
        send_to_char("Your secondary weapon has to be considerably lighter than the primary one.\n\r", ch);
        return;
    }

    if (!remove_obj(ch, WEAR_SECONDARY, true))
        return;

    act("$n wields $p in $s off-hand.", ch, obj, NULL, TO_ROOM);
    act("You wield $p in your off-hand.", ch, obj, NULL, TO_CHAR);
    equip_char(ch, obj, WEAR_SECONDARY);
    return;
}


/***************************************************************************
 *	do_envenom
 ***************************************************************************/
void do_envenom(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    AFFECT_DATA af;
    SKILL *skill_envenom;
    SKILL *skill_poison;
    int percent;
    int skill;

    if ((skill_envenom = skill_lookup("envenom")) == NULL
        || (skill_poison = skill_lookup("poison")) == NULL) {
        send_to_char("Huh?", ch);
        return;
    }


    /* find out what */
    if (argument[0] == '\0') {
        send_to_char("Envenom what item?\n\r", ch);
        return;
    }

    obj = get_obj_list(ch, argument, ch->carrying);
    if (obj == NULL) {
        send_to_char("You don't have that item.\n\r", ch);
        return;
    }

    if ((skill = get_learned_percent(ch, skill_envenom)) < 1) {
        send_to_char("Are you crazy? You'd poison yourself!\n\r", ch);
        return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON) {
        if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
            act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
            return;
        }

        if (number_percent() < skill) {  /* success! */
            act("$n treats $p with deadly poison.", ch, obj, NULL, TO_ROOM);
            act("You treat $p with deadly poison.", ch, obj, NULL, TO_CHAR);

            if (!obj->value[3]) {
                obj->value[3] = 1;
                check_improve(ch, skill_envenom, true, 4);
            }
            WAIT_STATE(ch, skill_envenom->wait);
            return;
        }

        act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
        if (!obj->value[3])
            check_improve(ch, skill_envenom, false, 4);

        WAIT_STATE(ch, skill_envenom->wait);
        return;
    }

    if (obj->item_type == ITEM_WEAPON) {
        if (obj->value[3] < 0
            || attack_table[obj->value[3]].damage == DAM_BASH) {
            send_to_char("You can only envenom edged weapons.\n\r", ch);
            return;
        }

        if (IS_WEAPON_STAT(obj, WEAPON_POISON)) {
            act("$p is already envenomed.", ch, obj, NULL, TO_CHAR);
            return;
        }

        percent = number_percent();
        if (percent < skill) {
            af.where = TO_WEAPON;
            af.type = skill_poison->sn;
            af.skill = skill_poison;
            af.level = (int)(ch->level * percent / 100);
            af.duration = (int)(ch->level / 2 * percent / 100);
            af.location = 0;
            af.modifier = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj, &af);

            act("$n coats $p with deadly venom.", ch, obj, NULL, TO_ROOM);
            act("You coat $p with venom.", ch, obj, NULL, TO_CHAR);
            check_improve(ch, skill_envenom, true, 3);
            WAIT_STATE(ch, skill_envenom->wait);
            return;
        } else {
            act("You fail to envenom $p.", ch, obj, NULL, TO_CHAR);
            check_improve(ch, skill_envenom, false, 3);
            WAIT_STATE(ch, skill_envenom->wait);
            return;
        }
    }

    act("You can't poison $p.", ch, obj, NULL, TO_CHAR);
    return;
}


/***************************************************************************
 *	do_dice
 *
 *	roll dice if it is in your inventory
 ***************************************************************************/
void do_dice(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    char buf[MAX_STRING_LENGTH];
    char tmp[MAX_STRING_LENGTH];
    int number;
    int sides;
    int result;
    int total;
    int iter;

    if (argument[0] == '\0' || argument[0] == '?' || !str_prefix(argument, "help")) {
        show_help(ch->desc, "dice_syntax", NULL);
        return;
    }

    if ((obj = get_obj_carry(ch, argument)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_DICE || obj->value[0] < 1) {
        send_to_char("You can only roll dice.\n\r", ch);
        return;
    }

    number = (int)obj->value[0];
    sides = (int)obj->value[1];
    if (number > 1) {
        act("You roll $p and they bounce around erratically...\n\r", ch, obj, NULL, TO_CHAR);
        act("$n rolls $p and they bounce around erratically....\n\r", ch, obj, NULL, TO_ROOM);
    } else {
        act("You roll $p and it bounces around erratically...\n\r", ch, obj, NULL, TO_CHAR);
        act("$n rolls $p and it bounces around erratically...\n\r", ch, obj, NULL, TO_ROOM);
    }

    buf[0] = '\0';
    total = 0;

    strcat(buf, "The `!D`1i`!c`1e R`!o`1ll``:\n\r\n\r ");
    for (iter = 0; iter < number; iter++) {
        result = dice(1, sides);
        if (iter < number - 1)
            sprintf(tmp, "`#%d`3 -`` ", result);
        else
            sprintf(tmp, "`#%d`` ", result);
        strcat(buf, tmp);
        total += result;
    }
    sprintf(tmp, "\n\r\n\r`@T`2otal `@D`2ice `2R`@oll`8: `@%d``", total);
    strcat(buf, tmp);

    act(buf, ch, obj, NULL, TO_CHAR);
    act(buf, ch, obj, NULL, TO_ROOM);
}

/***************************************************************************
 *   do_objident
 ***************************************************************************/
void do_objident(CHAR_DATA *ch, const char *argument)
{
    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
        send_to_char("You cannot identify pets.\n\r", ch);
        return;
    } else {
        CHAR_DATA *keeper;
        GAMEOBJECT *obj;
        unsigned int cost;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == NULL)
            return;

        one_argument(argument, arg);

        if (arg[0] == '\0') {
            send_to_char("What object would you like to evaluate?\n\r", ch);
            return;
        }
        if (!str_prefix(arg, keeper->name)) {
            argument = one_argument(argument, arg);
            one_argument(argument, arg);
            if (arg[0] == '\0') {
                send_to_char("What object would you like to evaluate?\n\r", ch);
                return;
            }
        }
        found = false;
        for (obj = keeper->carrying; obj; obj = obj->next_content) {
            if (obj->wear_loc == WEAR_NONE
                && can_see_obj(ch, obj)
                && (cost = get_cost(keeper, obj, true)) > 0
                && (arg[0] == '\0' || is_name(arg, object_name_get(obj)))) {
                {
                    if (!found) {
                        found = true;
                        identify_item(ch, obj);
                    }
                }
            }
        }

        if (!found)
            send_to_char("You can't buy anything here, so there is nothing to identify.\n\r", ch);
        return;
    }
}
