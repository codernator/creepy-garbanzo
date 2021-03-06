#include "merc.h"
#include "object.h"
#include "character.h"
#include "recycle.h"
#include <string.h>
#include <stdio.h>


extern struct dynamic_skill *gsp_detect_magic;
extern struct dynamic_skill *gsp_faerie_fog;
extern struct dynamic_skill *gsp_web;
extern struct dynamic_skill *gsp_peek;



// TODO - used by look
extern DO_FUN do_exits;

// TODO - used by look_object (weird show exits logic when looking at portals)
extern DO_FUN do_at;

// TODO - used by get_obj
extern DO_FUN do_split;

extern void print_weather(struct char_data * ch);


void show_char_to_char(struct char_data * list, struct char_data * ch);
void show_char_to_char_2(struct char_data * victim, struct char_data * ch);

static void show_char_to_char_0(struct char_data * victim, struct char_data * ch);
static void show_char_to_char_1(struct char_data * victim, struct char_data * ch);
static char *format_obj_to_char(struct gameobject * obj, struct char_data * ch, bool fShort);


static bool validate_look(struct char_data *ch);
static struct { int	wear_loc; char *desc; } where_name[] =
{
    { WEAR_LIGHT,	  "<used as light>     " },
    { WEAR_FINGER_L,  "<worn on finger>    " },
    { WEAR_FINGER_R,  "<worn on finger>    " },
    { WEAR_FINGER_L2, "<worn on finger>    " },
    { WEAR_FINGER_R2, "<worn on finger>    " },
    { WEAR_NECK_1,	  "<worn around neck>  " },
    { WEAR_NECK_2,	  "<worn around neck>  " },
    { WEAR_BODY,	  "<worn on torso>     " },
    { WEAR_HEAD,	  "<worn on head>      " },
    { WEAR_FACE,	  "<worn on face>      " },
    { WEAR_EAR_L,	  "<worn in left ear>  " },
    { WEAR_EAR_R,	  "<worn in right ear> " },
    { WEAR_LEGS,	  "<worn on legs>      " },
    { WEAR_FEET,	  "<worn on feet>      " },
    { WEAR_HANDS,	  "<worn on hands>     " },
    { WEAR_ARMS,	  "<worn on arms>      " },
    { WEAR_SHIELD,	  "<worn as shield>    " },
    { WEAR_ABOUT,	  "<worn about body>   " },
    { WEAR_WAIST,	  "<worn about waist>  " },
    { WEAR_WRIST_L,	  "<worn around wrist> " },
    { WEAR_WRIST_R,	  "<worn around wrist> " },
    { WEAR_WIELD,	  "<wielded>           " },
    { WEAR_HOLD,	  "<held>              " },
    { WEAR_FLOAT,	  "<floating nearby>   " },
    { WEAR_SECONDARY, "<secondary weapon>  " },
    { WEAR_THIRD,	  "<third weapon>      " },
    { WEAR_TATTOO,	  "<worn as a tattoo>  " },
    { -1,		  ""			 }
};

void toggle_afk(struct char_data *ch, const char *message)
{
    bool now_on;
    now_on = character_toggle_comm(ch, COMM_AFK);

    if (now_on) {
	send_to_char("You are now in `!A`@F`OK`` mode.\n\r", ch);
	if (message != NULL && message[0] != '\0') {
	    act("$n is `!A`@F`OK`7: `O(`7$T`O)``", ch, NULL, message, TO_ROOM);
	    ch->pcdata->afk_message = str_dup(message);
	} else {
	    ch->pcdata->afk_message = str_dup("");
	}
    } else {
	send_to_char("`!A`@F`OK`` mode removed. Type 'replay' to see tells.\n\r", ch);
    }
}

void show_channels(struct char_data *ch)
{
    send_to_char("CHANNEL        STATUS\n\r", ch);
    send_to_char("```&---------------------``\n\r", ch);

    send_to_char("\n\r", ch);
    if (character_has_comm(ch, COMM_AFK))
	send_to_char("You are ```!A```@F```OK``.\n\r", ch);

    if (character_has_comm(ch, COMM_BUSY))
	send_to_char("You are Busy.\n\r", ch);

    if (character_has_comm(ch, COMM_CODING))
	send_to_char("You are `@Coding``.\n\r", ch);

    if (character_has_comm(ch, COMM_BUILD))
	send_to_char("You are `3Building``.\n\r", ch);

    if (ch->lines != PAGELEN) {
	if (ch->lines > 0)
	    printf_to_char(ch, "You display %d lines of scroll.\n\r", ch->lines + 2);
	else
	    send_to_char("Scroll buffering is off.\n\r", ch);
    }

    if (ch->prompt != NULL)
	printf_to_char(ch, "Your current prompt is: %s\n\r", ch->prompt);

    return;
}

void replay(struct char_data *ch) {
    page_to_char(buf_string(ch->pcdata->buffer), ch);
    clear_buf(ch->pcdata->buffer);
    return;
}

void look_equipment(struct char_data *ch) {
    struct gameobject *obj;
    int iWear;

    send_to_char("`&  You are using: \n\r ------------------``\n\r", ch);
    for (iWear = 0; where_name[iWear].wear_loc >= 0; iWear++) {
	if ((obj = get_eq_char(ch, where_name[iWear].wear_loc)) == NULL) {
	    if (!(IS_SET(ch->act, PLR_AUTOEQ)))
		continue;

	    send_to_char("`1", ch);
	    send_to_char(where_name[iWear].desc, ch);

	    if (where_name[iWear].wear_loc == WEAR_THIRD) {
		send_to_char("`1     --Not Available--``\r\n", ch);
	    } else {
		if ((where_name[iWear].wear_loc == WEAR_FINGER_L2)) {
		    send_to_char("`1     --Not Available--``\r\n", ch);
		} else {
		    if ((where_name[iWear].wear_loc == WEAR_FINGER_R2)) {
			send_to_char("`1     --Not Available--``\r\n", ch);
		    } else {
			send_to_char("`1     --Empty--``\r\n", ch);
		    }
		}
	    }
	    continue;
	}

	send_to_char("`7", ch);
	send_to_char(where_name[iWear].desc, ch);
	send_to_char("`&", ch);
	if (can_see_obj(ch, obj)) {
	    send_to_char("`&", ch);
	    send_to_char(format_obj_to_char(obj, ch, true), ch);
	    send_to_char("``\n\r", ch);
	} else {
	    send_to_char("`&something.\n\r", ch);
	}
    }

    send_to_char("`8", ch);
}

void look_direction(struct char_data *ch, const int door) {
    struct exit_data *pexit;

    if (!validate_look(ch)) {
	return;
    }

    if (ch->in_room == NULL) {
	send_to_char("NOWHERE.", ch);
	return;
    }

    /* 'look direction' */
    if ((pexit = ch->in_room->exit[door]) == NULL) {
	send_to_char("Nothing special there.\n\r", ch);
	return;
    }

    if (pexit->description != NULL && pexit->description[0] != '\0')
	send_to_char(pexit->description, ch);
    else
	send_to_char("Nothing special there.\n\r", ch);

    if (pexit->keyword != NULL
	    && pexit->keyword[0] != '\0'
	    && pexit->keyword[0] != ' ') {
	if (IS_SET(pexit->exit_info, EX_CLOSED))
	    act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
	else if (IS_SET(pexit->exit_info, EX_ISDOOR))
	    act("The $d is open.", ch, NULL, pexit->keyword, TO_CHAR);
    }

    return;
}

static bool try_look_object(struct char_data *ch, struct gameobject *list, const char *name, const int number, int *count)
{
    struct gameobject *obj;
    struct extra_descr_data *pdesc;

    for (obj = list; obj != NULL; obj = obj->next_content) {
        if (can_see_obj(ch, obj)) {
            pdesc = extradescrdata_match(OBJECT_EXTRA(obj), name);
            if (pdesc != NULL) {
                if (++(*count) == number) {
                    send_to_char(pdesc->description, ch);
                    return true;
                } else {
                    continue;
                }
            }

            if (is_name(name, object_name_get(obj))) {
                if (++(*count) == number) {
                    send_to_char(OBJECT_LONG(obj), ch);
                    send_to_char("\n\r", ch);
                    return true;
                }
            }
        }
    }
    return false;
}

void look_extras(struct char_data *ch, const char *name, const int number)
{
    int count;

    if (!validate_look(ch)) {
        return;
    }

    count = 0;
    if (try_look_object(ch, ch->carrying, name, number, &count))
        return;
    if (try_look_object(ch, ch->in_room->contents, name, number, &count))
        return;


    // TODO put this back after room->extra_descr is properly a HEAD list.
    //pdesc = extradescrdata_match(ch->in_room->extra_descr, name);
    //if (pdesc != NULL) {
    //    if (++count == number) {
    //        send_to_char(pdesc, ch);
    //        return;
    //    }
    //}

    if (count == 0) {
        send_to_char("You don't see that here.", ch);
        return;
    }

    if (count > 0 && count != number) {
        if (count == 1)
            printf_to_char(ch, "You only see one %s here.\n\r", name);
        else
            printf_to_char(ch, "You only see %d of those here.\n\r", count);
        return;
    }

    return;
}

void look_character(struct char_data *ch, struct char_data *victim) {
    if (!validate_look(ch)) {
	return;
    }

    show_char_to_char_1(victim, ch);
}

void look_room(struct char_data *ch, struct roomtemplate *in_room) {
    if (!validate_look(ch)) {
	return;
    }

    if (in_room == NULL) {
	send_to_char("NOWHERE.", ch);
	return;
    }

    if (room_is_dark(ch, in_room) && !IS_SET(ch->act, PLR_HOLYLIGHT)) {
	send_to_char("It is pitch ```8black...`` \n\r", ch);
	show_char_to_char(in_room->people, ch);
	return;
    }


    send_to_char(in_room->name, ch);
    if ((IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act, PLR_HOLYLIGHT))) || IS_BUILDER(ch, in_room->area)) {
	printf_to_char(ch, " [`1Room `!%d``]", in_room->vnum);
    }

    send_to_char("\n\r", ch);
    if ((!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF))) {
	send_to_char("  ", ch);
	send_to_char(in_room->description, ch);
    }

    send_to_char("\n\r", ch);

    if ((is_affected(ch, gsp_detect_magic) || IS_SET(ch->act, PLR_HOLYLIGHT))) {
	struct affect_data *paf;
	for (paf = in_room->affected; paf != NULL; paf = paf->next) {
	    send_to_char(room_affect(paf), ch);
	}
	send_to_char("\n\r", ch);
    }

    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT))
	do_exits(ch, "auto");

    if (is_affected_room(in_room, gsp_faerie_fog))
	send_to_char("There is a `Ppurple haze`` floating throughout the room.\n\r", ch);

    show_list_to_char(in_room->contents, ch, false, false);
    show_char_to_char(in_room->people, ch);
    return;
}

void look_object(struct char_data *ch, struct gameobject *obj, const char *argument) {
    struct gameobject *portal;
    struct roomtemplate *location;

    if (!validate_look(ch)) {
	return;
    }

    switch (OBJECT_TYPE(obj)) {
	default:
	    send_to_char("That is not a container.\n\r", ch);
	    break;

	case ITEM_DRINK_CON:
	    if (obj->value[1] == 0) {
		send_to_char("It is empty.\n\r", ch);
	    } else {
		printf_to_char(ch, "It's %sfilled with  a %s liquid.\n\r",
			obj->value[1] < obj->value[0] / 4
			? "less than half-" :
			obj->value[1] < 3 * obj->value[0] / 4
			? "about half-" : "more than half-",
			liq_table[obj->value[2]].liq_color);
	    }
	    break;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    if (IS_SET(obj->value[1], CONT_CLOSED)) {
		send_to_char("It is closed.\n\r", ch);
	    } else {
		act("$p holds```8:``", ch, obj, NULL, TO_CHAR);
		show_list_to_char(obj->contains, ch, true, true);
	    }
	    break;
	case ITEM_PORTAL:
	    portal = get_obj_list(ch, argument, ch->in_room->contents);
	    if (portal != NULL) {
		location = get_room_index(portal->value[3]);
		if (location == NULL) {
		    send_to_char("It looks very empty..\n\r", ch);
		} else {
		    send_to_char(location->name, ch);
		    send_to_char("\n\r", ch);
		    if (!IS_SET(location->room_flags, ROOM_INDOORS))
			print_weather(ch);
		    else
			send_to_char("You can not discern weather conditions beyond this portal.\n\r", ch);
		    if (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF)) {
			send_to_char(location->description, ch);
		    }
		    send_to_char("\n\r", ch);

		    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT)) {
			char showexit[100];

			sprintf(showexit, "%ld exits auto", location->vnum);
			do_at(ch, showexit);
		    }

		    show_list_to_char(location->contents, ch, false, false);
		    show_char_to_char(location->people, ch);
		}
	    }
	    break;
    }
}

void sit(struct char_data *ch, struct gameobject *on)
{
    // TODO - it is currently possible to defeat object pickup restrictions by
    // sitting on them. Need to extract can_get_obj from get_obj and use it!

    // Impassible positions.
    switch (ch->position) {
	case POS_SITTING:
	    if (on == NULL || on == ch->on) {
		send_to_char("You are already sitting down.\n\r", ch);
		return;
	    }
	    break;

	case POS_FIGHTING:
	    send_to_char("Maybe you should finish this fight first?\n\r", ch);
	    return;

	case POS_SLEEPING:
	    if (IS_AFFECTED(ch, AFF_SLEEP)) {
		send_to_char("You can't wake up!\n\r", ch);
		return;
	    }
	    break;
    }

    // Validate target object, if any.
    if (on != NULL) {
	if (!is_situpon(on)) {
	    send_to_char("You can't sit on that.\n\r", ch);
	    return;
	}

	if (ch->position == POS_SLEEPING && on != ch->on) {
	    send_to_char("You'll need to wake up to find it.\n\r", ch);
	    return;
	}

	if (on->carried_by != NULL && on->carried_by != ch) {
	    act_new("The $p is being held by someone!", ch, on, NULL, TO_CHAR, POS_DEAD, false);
	    return;
	}

	if (on->in_obj != NULL) {
	    if (on->in_obj->in_room != ch->in_room) {
		send_to_char("What? Where?\n\r", ch);
		return;
	    }
	}

	if (ch->on != on && (long)count_users(on) >= on->value[0]) {
	    act_new("There's no more room on $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
	    return;
	}
    }

    // Sit down, possibly on and object.
    ch->position = POS_SITTING;
    if (on != NULL) {
	ch->on = on;

	if (on->carried_by == ch) {
	    obj_from_char(on);
	    obj_to_room(on, ch->in_room);
	} else if (on->in_obj != NULL) {
	    obj_from_obj(on);
	    obj_to_room(on, ch->in_room);
	}
    }

    // Notify
    switch (ch->position) {
	default:
	    if (on == NULL) {
		send_to_char("You sit down.\n\r", ch);
		act("$n sits down on the ground.", ch, NULL, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], SIT_AT)) {
		act("You sit down at $p.", ch, on, NULL, TO_CHAR);
		act("$n sits down at $p.", ch, on, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], SIT_ON)) {
		act("You sit on $p.", ch, on, NULL, TO_CHAR);
		act("$n sits on $p.", ch, on, NULL, TO_ROOM);
	    } else {
		act("You sit down in $p.", ch, on, NULL, TO_CHAR);
		act("$n sits down in $p.", ch, on, NULL, TO_ROOM);
	    }
	    break;
	case POS_SLEEPING:
	    if (on == NULL) {
		send_to_char("You wake and sit up.\n\r", ch);
		act("$n wakes and sits up.", ch, NULL, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], SIT_AT)) {
		act_new("You wake and sit at $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
		act("$n wakes and sits at $p.", ch, on, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], SIT_ON)) {
		act_new("You wake and sit on $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
		act("$n wakes and sits at $p.", ch, on, NULL, TO_ROOM);
	    } else {
		act_new("You wake and sit in $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
		act("$n wakes and sits in $p.", ch, on, NULL, TO_ROOM);
	    }
	    break;
	case POS_RESTING:
	    if (on == NULL) {
		send_to_char("You stop resting.\n\r", ch);
	    } else if (IS_SET(on->value[2], SIT_AT)) {
		act("You sit at $p.", ch, on, NULL, TO_CHAR);
		act("$n sits at $p.", ch, on, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], SIT_ON)) {
		act("You sit on $p.", ch, on, NULL, TO_CHAR);
		act("$n sits on $p.", ch, on, NULL, TO_ROOM);
	    }
	    break;
    }
}

void stand(struct char_data *ch, struct gameobject *on)
{
    // TODO - need to extract can_get_obj from get_obj

    // Impassible positions.
    switch (ch->position) {
	case POS_STANDING:
	    if (on == NULL || on == ch->on) {
		send_to_char("You are already standing.\n\r", ch);
		return;
	    }
	    break;

	case POS_FIGHTING:
	    send_to_char("Maybe you should finish this fight first?\n\r", ch);
	    return;

	case POS_SLEEPING:
	    if (IS_AFFECTED(ch, AFF_SLEEP)) {
		send_to_char("You can't wake up!\n\r", ch);
		return;
	    }
	    break;
    }

    // Validate target object, if any.
    if (on != NULL) {
	if (!is_standupon(on)) {
	    send_to_char("You can't seem to find a place to stand.\n\r", ch);
	    return;
	}

	if (ch->on != on && (long)count_users(on) >= on->value[0]) {
	    act_new("There's no room to stand on $p.", ch, on, NULL, TO_ROOM, POS_DEAD, false);
	    return;
	}

	// TODO check if object is gettable
    }


    // stand up, possibly on something.
    ch->position = POS_STANDING;
    if (on != ch->on) {
	struct gameobject *old_on = ch->on;
	ch->on = on;
	if (old_on != NULL && can_see_obj(ch, old_on)) {
	    // TODO - only if object is gettable.
	    get_obj(ch, old_on, NULL);
	}
    }

    // Notify.

    switch (ch->position) {
	default:
	    if (on == NULL) {
		send_to_char("You stand up.\n\r", ch);
		act("$n stands up.", ch, NULL, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], STAND_AT)) {
		act("You stand at $p.", ch, on, NULL, TO_CHAR);
		act("$n stands at $p.", ch, on, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], STAND_ON)) {
		act("You stand on $p.", ch, on, NULL, TO_CHAR);
		act("$n stands on $p.", ch, on, NULL, TO_ROOM);
	    } else {
		act("You stand in $p.", ch, on, NULL, TO_CHAR);
		act("$n stands on $p.", ch, on, NULL, TO_ROOM);
	    }
	    break;
	case POS_SLEEPING:
	    if (on == NULL) {
		send_to_char("You wake and stand up.\n\r", ch);
		act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], STAND_AT)) {
		act_new("You wake and stand at $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
		act("$n wakes and stands at $p.", ch, on, NULL, TO_ROOM);
	    } else if (IS_SET(on->value[2], STAND_ON)) {
		act_new("You wake and stand on $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
		act("$n wakes and stands on $p.", ch, on, NULL, TO_ROOM);
	    } else {
		act_new("You wake and stand in $p.", ch, on, NULL, TO_CHAR, POS_DEAD, false);
		act("$n wakes and stands in $p.", ch, on, NULL, TO_ROOM);
	    }
	    look_room(ch, ch->in_room);
	    break;
    }
}

void get_obj(struct char_data *ch, struct gameobject *obj, struct gameobject *container)
{
    struct char_data *gch;
    int members;
    char buffer[100];

    if (!CAN_WEAR(obj, ITEM_TAKE)) {
	send_to_char("You can't take that.\n\r", ch);
	return;
    }

    if ((ch->carry_number + get_obj_number(obj)) > can_carry_n(ch)
	    && (!IS_IMMORTAL(ch))) {
	act("$d: you can't carry that many items.",
		ch, NULL, object_name_get(obj), TO_CHAR);
	return;
    }


    if ((ch->carry_weight + get_obj_weight(obj)) > can_carry_w(ch)) {
	act("$d: you can't carry that much weight.", ch, NULL, object_name_get(obj), TO_CHAR);
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


    if (container != NULL) {
        if (container->objtemplate->vnum == OBJ_VNUM_PIT
                && !CAN_WEAR(container, ITEM_TAKE)
                && !IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
        {
            obj->timer = 0;
        }
        act_new("You get $p from $P.", ch, obj, container, TO_CHAR, POS_RESTING, true);
        act_new("$n gets $p from $P.", ch, obj, container, TO_ROOM, POS_RESTING, true);
        REMOVE_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        obj_from_obj(obj);
    } else {
        act_new("You get $p.", ch, obj, container, TO_CHAR, POS_RESTING, true);
        act_new("$n gets $p.", ch, obj, container, TO_ROOM, POS_RESTING, true);
        obj_from_room(obj);
    }

    if (OBJECT_TYPE(obj) == ITEM_MONEY) {
	ch->silver += obj->value[0];
	ch->gold += obj->value[1];

	if (IS_SET(ch->act, PLR_AUTOSPLIT)) {
	    members = 0;
	    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
		if (!IS_AFFECTED(gch, AFF_CHARM) && is_same_group(gch, ch))
		    members++;

	    if (members > 1 && (obj->value[0] > 1 || obj->value[1])) {
		sprintf(buffer, "%ld %ld", obj->value[0], obj->value[1]);
		do_split(ch, buffer);
	    }
	}

	extract_obj(obj);
    } else {
	obj_to_char(obj, ch);
    }

    return;
}

bool validate_look(struct char_data *ch) {
    if (ch->desc == NULL)
	return false;

    switch (ch->position) {
	case POS_DEAD:
	    send_to_char("NOTHING\n\r", ch);
	    return false;

	case POS_MORTAL:
	case POS_INCAP:
	case POS_STUNNED:
	    send_to_char("Stars and birds swirl about your head.\n\r", ch);
	    return false;

	case POS_SLEEPING:
	    send_to_char("You can't see anything, you're sleeping!\n\r", ch);
	    return false;
    }

    if (character_is_blind(ch)) {
	send_to_char("You can't see a thing!  You're blind!!!\n\r", ch);
	return false;
    }

    return true;
}


void show_char_to_char(struct char_data *list, struct char_data *ch)
{
    struct char_data *rch;

    for (rch = list; rch != NULL; rch = rch->next_in_room) {
	if (rch == ch)
	    continue;

	if (get_trust(ch) < rch->invis_level)
	    continue;

	if (can_see(ch, rch))
	    show_char_to_char_0(rch, ch);
	else if (room_is_dark(ch, ch->in_room)
		&& IS_AFFECTED(rch, AFF_INFRARED))
	    send_to_char("You see glowing ```1red ``eyes watching YOU!\n\r", ch);
    }

    return;
}

void show_char_to_char_0(struct char_data *victim, struct char_data *ch)
{
    char buf[MAX_STRING_LENGTH], message[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if (IS_SET(victim->comm, COMM_AFK)) strcat(buf, "```!A```@F```OK ``");
    if (IS_SET(victim->comm, COMM_BUSY))
	strcat(buf, "[`1Busy``] ");

    if (IS_SET(victim->comm, COMM_CODING)) strcat(buf, "[`@CODING``] ");
    if (IS_SET(victim->comm, COMM_BUILD)) strcat(buf, "[`3BUILDING``] ");
    if (IS_AFFECTED(victim, AFF_INVISIBLE)) strcat(buf, "``(`iInvis``) ");
    if (victim->invis_level >= LEVEL_HERO) strcat(buf, "`8(`wWiZi`8) ``");
    if (!IS_NPC(victim))
	if (IS_AFFECTED(victim, AFF_HIDE)) strcat(buf, "```4(`hHide```4) ``");
    if (IS_AFFECTED(victim, AFF_CHARM)) strcat(buf, "```#(`cCharmed```#) ``");
    if (IS_AFFECTED(victim, AFF_PASS_DOOR)) strcat(buf, "```6(```^Translucent```6) ``");
    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE)) strcat(buf, "```!(```PPink Aura```!) ``");
    if (IS_AFFECTED(victim, AFF_SANCTUARY)) strcat(buf, "(```&White Aura``) ");
    if (IS_AFFECTED(victim, AFF_DRUID_CALL)) strcat(buf, "(`8Grey Aura``) ");
    if (IS_AFFECTED(victim, AFF_CALLOUSED)) strcat(buf, "`&(`6C`^a`6ll`^ou`6s`^e`6d`&)`` ");
    if (is_affected(victim, gsp_web)) strcat(buf, "`8(`2St`@i`2c`@k`2y`8)`` ");

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER)) strcat(buf, "-```1K```!i```1LLER``- ");
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF)) strcat(buf, "-```8TH``i```8EF``- ");
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_LINKDEAD)) strcat(buf, "`7[`8LINKDEAD`7] ");
    if (victim->position == victim->start_pos && victim->long_descr[0] != '\0') {
	strcat(buf, victim->long_descr);
	send_to_char(buf, ch);
	return;
    }

    strcat(buf, PERS(victim, ch));

    if (!IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF)
	    && victim->position == POS_STANDING && ch->on == NULL)
	strcat(buf, victim->pcdata->title);

    switch (victim->position) {
	case POS_DEAD:
	    strcat(buf, " is ```1D```8E```1A```8D``!!");
	    break;
	case POS_MORTAL:
	    strcat(buf, " is mortally wounded.");
	    break;
	case POS_INCAP:
	    strcat(buf, " is incapacitated.");
	    break;
	case POS_STUNNED:
	    strcat(buf, " is lying here stunned.");
	    break;
	case POS_SLEEPING:
	    if (victim->on != NULL) {
		if (IS_SET(victim->on->value[2], SLEEP_AT)) {
		    sprintf(message, " is sleeping at %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else if (IS_SET(victim->on->value[2], SLEEP_ON)) {
		    sprintf(message, " is sleeping on %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else {
		    sprintf(message, " is sleeping in %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		}
	    } else {
		strcat(buf, " is sleeping here.");
	    }

	    break;
	case POS_RESTING:
	    if (victim->on != NULL) {
		if (IS_SET(victim->on->value[2], REST_AT)) {
		    sprintf(message, " is resting at %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else if (IS_SET(victim->on->value[2], REST_ON)) {
		    sprintf(message, " is resting on %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else {
		    sprintf(message, " is resting in %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		}
	    } else {
		strcat(buf, " is resting here.");
	    }

	    break;
	case POS_SITTING:
	    if (victim->on != NULL) {
		if (IS_SET(victim->on->value[2], SIT_AT)) {
		    sprintf(message, " is sitting at %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else if (IS_SET(victim->on->value[2], SIT_ON)) {
		    sprintf(message, " is sitting on %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else {
		    sprintf(message, " is sitting in %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		}
	    } else {
		strcat(buf, " is sitting here.");
	    }

	    break;
	case POS_STANDING:
	    if (victim->on != NULL) {
		if (IS_SET(victim->on->value[2], STAND_AT)) {
		    sprintf(message, " is standing at %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else if (IS_SET(victim->on->value[2], STAND_ON)) {
		    sprintf(message, " is standing on %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		} else {
		    sprintf(message, " is standing in %s.", OBJECT_SHORT(victim->on));
		    strcat(buf, message);
		}
	    } else {
		strcat(buf, " is here.");
	    }

	    break;
	case POS_FIGHTING:
	    strcat(buf, " is here, `!fighting`` ");
	    if (victim->fighting == NULL) {
		strcat(buf, "thin air??");
	    } else if (victim->fighting == ch) {
		strcat(buf, "```!YOU``!");
	    } else if (victim->in_room == victim->fighting->in_room) {
		strcat(buf, PERS(victim->fighting, ch));
		strcat(buf, ".");
	    } else {
		strcat(buf, "someone who left??");
	    }

	    break;
    }

    strcat(buf, "\n\r");
    buf[0] = UPPER(buf[0]);
    send_to_char(buf, ch);
    return;
}

void show_char_to_char_1(struct char_data *victim, struct char_data *ch)
{
    if (can_see(victim, ch)) {
	if (ch == victim) {
	    act("$n looks at $mself.", ch, NULL, NULL, TO_ROOM);
	} else {
	    act("$n looks at you.", ch, NULL, victim, TO_VICT);
	    act("$n looks at $N.", ch, NULL, victim, TO_NOTVICT);
	}
    }

    if (victim->description[0] != '\0')
	send_to_char(victim->description, ch);
    else
	act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);

    show_damage_display(ch, victim);

    return;
}

void show_char_to_char_2(struct char_data *victim, struct char_data *ch)
{
    struct gameobject *obj;
    struct dynamic_skill *skill_peek;
    int iWear;
    bool found;

    if (can_see(victim, ch)) {
	if (ch == victim) {
	    act("$n glances at $mself.", ch, NULL, NULL, TO_ROOM);
	} else {
	    act("$n glances at you.", ch, NULL, victim, TO_VICT);
	    act("$n glances at $N.", ch, NULL, victim, TO_NOTVICT);
	}
    }

    show_damage_display(ch, victim);

    found = false;
    for (iWear = 0; where_name[iWear].wear_loc >= 0; iWear++) {
	if ((obj = get_eq_char(victim, where_name[iWear].wear_loc)) != NULL
		&& can_see_obj(ch, obj)) {
	    if (!found) {
		send_to_char("\n\r", ch);
		act("$N is using:", ch, NULL, victim, TO_CHAR);
		found = true;
	    }
	    send_to_char(where_name[iWear].desc, ch);
	    send_to_char("(", ch);
	    send_to_char(format_obj_to_char(obj, ch, true), ch);
	    send_to_char(")", ch);
	    send_to_char("\n\r", ch);
	}
    }

    if (victim != ch && !IS_NPC(ch)
	    && (skill_peek = gsp_peek) != NULL
	    && number_percent() < get_learned_percent(ch, skill_peek)) {
	send_to_char("\n\rYou peek at the inventory:\n\r", ch);
	check_improve(ch, skill_peek, true, 4);
	show_list_to_char(victim->carrying, ch, true, true);
    }

    return;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(struct gameobject *list, struct char_data *ch, bool fShort, bool fShowNothing)
{
    struct gameobject *obj;
    char buf[13000];
    char **prgpstrShow;
    char *pstrShow;
    int *prgnShow;
    int nShow;
    int iShow;
    unsigned int count;
    bool fCombine;

    if (ch->desc == NULL)
	return;

    /*
     * Alloc space for output lines.
     */

    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
	count++;


    prgpstrShow = alloc_mem(count * (unsigned int)sizeof(char *));
    prgnShow = alloc_mem(count * (unsigned int)sizeof(int));

    nShow = 0;

    /*
     * Format the list of objects.
     */
    for (obj = list; obj != NULL; obj = obj->next_content) {
	if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj)) {
	    pstrShow = format_obj_to_char(obj, ch, fShort);

	    fCombine = false;

	    if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
		/*
		 * Look for duplicates, case sensitive.
		 * Matches tend to be near end so run loop backwords.
		 */
		for (iShow = nShow - 1; iShow >= 0; iShow--) {
		    if (!strcmp(prgpstrShow[iShow], pstrShow)) {
			prgnShow[iShow]++;
			fCombine = true;
			break;
		    }
		}
	    }

	    /*
	     * Couldn't combine, or didn't want to.
	     */
	    if (!fCombine) {
		prgpstrShow[nShow] = str_dup(pstrShow);
		prgnShow[nShow] = 1;
		nShow++;

		if (nShow > 125) {
		    send_to_char("Tell whoever it is you are looking at to drop some crap.\n\r", ch);
		    send_to_char("Maximum number of items in a list exceeded.\n\r", ch);
		    return;
		}
	    }
	}
    }

    /*
     * Output the formatted list.
     */
    for (iShow = 0; iShow < nShow; iShow++) {
	if (prgpstrShow[iShow][0] == '\0') {
	    free_string(prgpstrShow[iShow]);
	    continue;
	}

	if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
	    if (prgnShow[iShow] != 1) {
		sprintf(buf, "(%3d) ", prgnShow[iShow]);
		send_to_char(buf, ch);
	    } else {
		send_to_char("      ", ch);
	    }
	}
	page_to_char(prgpstrShow[iShow], ch);
	send_to_char("\n\r", ch);
	free_string(prgpstrShow[iShow]);
    }

    if (fShowNothing && nShow == 0) {
	if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
	    send_to_char("     ", ch);

	send_to_char("Nothing.\n\r", ch);
    }

    /*
     * Clean up.
     */
    free_mem(prgpstrShow, count * (int)sizeof(char *));
    free_mem(prgnShow, count * (int)sizeof(int));

    return;
}

char *format_obj_to_char(struct gameobject *obj, struct char_data *ch, bool fShort)
{
    static char buf[MAX_STRING_LENGTH * 2];

    buf[0] = '\0';

    if ((fShort && (OBJECT_SHORT(obj) == NULL || OBJECT_SHORT(obj)[0] == '\0'))
	    || (OBJECT_LONG(obj) == NULL || OBJECT_LONG(obj)[0] == '\0'))
	return buf;

    if (IS_OBJ_STAT(obj, ITEM_INVIS) ||
	    (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC)) ||
	    IS_OBJ_STAT(obj, ITEM_GLOW) ||
	    IS_OBJ_STAT(obj, ITEM_HUM) ||
	    IS_OBJ_STAT(obj, ITEM_CACHE) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY1) ||
	    IS_OBJ_STAT2(obj, ITEM2_RELIC) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY2))
	strcat(buf, "(");
    ;


    if (IS_OBJ_STAT(obj, ITEM_INVIS))
	strcat(buf, "```8I``");
    if (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC))
	strcat(buf, "```^M``");
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
	strcat(buf, "```@G``");
    if (IS_OBJ_STAT(obj, ITEM_HUM))
	strcat(buf, "```PH``");
    if (IS_OBJ_STAT(obj, ITEM_CACHE))
	strcat(buf, "`@C``");
    if (IS_OBJ_STAT(obj, ITEM_INLAY1))
	strcat(buf, "```2G``");
    if (IS_OBJ_STAT2(obj, ITEM2_RELIC))
	strcat(buf, "`!Relic``");
    if (IS_OBJ_STAT(obj, ITEM_INLAY2))
	strcat(buf, "```4B``");

    if (IS_OBJ_STAT(obj, ITEM_INVIS) ||
	    (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC)) ||
	    IS_OBJ_STAT(obj, ITEM_GLOW) ||
	    IS_OBJ_STAT(obj, ITEM_HUM) ||
	    IS_OBJ_STAT(obj, ITEM_CACHE) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY1) ||
	    IS_OBJ_STAT2(obj, ITEM2_RELIC) ||
	    IS_OBJ_STAT(obj, ITEM_INLAY2))
	strcat(buf, ") ");
    ;


    if (fShort) {
	if (OBJECT_SHORT(obj) != NULL)
	    strcat(buf, OBJECT_SHORT(obj));
    } else {
	if (OBJECT_LONG(obj) != NULL)
	    strcat(buf, OBJECT_LONG(obj));
    }

    return buf;
}

