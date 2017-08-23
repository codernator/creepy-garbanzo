
#include "merc.h"
#include "interp.h"


extern bool mp_percent_trigger(struct char_data * mob, struct char_data * ch, const void *arg1, const void *arg2, int type);
extern void mp_greet_trigger(struct char_data * ch);

/* random room generation procedure */
struct roomtemplate *get_random_room(struct char_data *ch, struct area_data *area)
{
	struct roomtemplate *room;

	for (;; ) {
		room = get_room_index(number_range(1, MAX_KEY_HASH));
		if (room != NULL
		    && (area == NULL || room->area == area)) {
			if (can_see_room(ch, room)
			    && !room_is_private(room)
			    && !IS_SET(room->room_flags, ROOM_SAFE)
			    && !IS_SET(room->room_flags, ROOM_NORANDOM))
				break;
		}
	}

	return room;
}

/* RT Enter portals */
void do_enter(struct char_data *ch, const char *argument)
{
	struct roomtemplate *location;

	if (ch->fighting != NULL)
		return;

/* nifty portal stuff */
	if (argument[0] != '\0') {
		struct roomtemplate *old_room;
		struct gameobject *portal;
		struct char_data *fch, *fch_next;

		old_room = ch->in_room;

		portal = get_obj_list(ch, argument, ch->in_room->contents);

		if (portal == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}

		if (OBJECT_TYPE(portal) != ITEM_PORTAL
		    || (IS_SET(portal->value[1], (long)EX_CLOSED) && !IS_TRUSTED(ch, ANGEL))) {
			send_to_char("You can't seem to find a way in.\n\r", ch);
			return;
		}

		if (!IS_TRUSTED(ch, ANGEL) && !IS_SET(portal->value[2], GATE_NOCURSE)
		    && (IS_AFFECTED(ch, AFF_CURSE))) {
			send_to_char("Something prevents you from entering it...\n\r", ch);
			return;
		}

		if (IS_SET(portal->value[2], GATE_RANDOM) || portal->value[3] == -1) {
			location = get_random_room(ch, NULL);
			portal->value[3] = location->vnum; /* for record keeping :) */
		} else if (IS_SET(portal->value[2], GATE_BUGGY) && (number_percent() < 5)) {
			location = get_random_room(ch, NULL);
		} else {
			location = get_room_index(portal->value[3]);
		}


		if (location == NULL
		    || location == old_room
		    || !can_see_room(ch, location)
		    || (room_is_private(location) && !IS_TRUSTED(ch, ANGEL))) {
			act("$p doesn't seem to go anywhere.", ch, portal, NULL, TO_CHAR);
			return;
		}

		if (IS_NPC(ch) && IS_SET(ch->act, ACT_AGGRESSIVE)) {
			send_to_char("Something prevents you from entering it...\n\r", ch);
			return;
		}

		act("$n steps into $p.", ch, portal, NULL, TO_ROOM);

		if (IS_SET(portal->value[2], GATE_NORMAL_EXIT))
			act("You step through $p.", ch, portal, NULL, TO_CHAR);
		else
			act("You step through $p.",
			    ch, portal, NULL, TO_CHAR);

		char_from_room(ch);
		char_to_room(ch, location);

		if (IS_SET(portal->value[2], GATE_GOWITH)) { /* take the gate along */
			obj_from_room(portal);
			obj_to_room(portal, location);
		}

		if (IS_SET(portal->value[2], GATE_NORMAL_EXIT))
			act("$n has arrived through $p.", ch, portal, NULL, TO_ROOM);
		else
			act("$n has arrived through $p.", ch, portal, NULL, TO_ROOM);

		do_look(ch, "auto");

		/* charges */
		if (portal->value[0] > 0) {
			portal->value[0]--;
			if (portal->value[0] == 0)
				portal->value[0] = -1;
		}

		/* protect against circular follows */
		if (old_room == location)
			return;

		for (fch = old_room->people; fch != NULL; fch = fch_next) {
			fch_next = fch->next_in_room;

			if (portal == NULL || portal->value[0] == -1)
				/* no following through dead portals */
				continue;

			if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
			    && fch->position < POS_STANDING)
				do_stand(fch, "");

			if (fch->master == ch && fch->position == POS_STANDING) {
				if (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE)) {
					act("You can't force $N to follow.",
					    ch, NULL, fch, TO_CHAR);
					act("You aren't allowed to follow.",
					    fch, NULL, NULL, TO_CHAR);
					continue;
				}

				act("You follow $N.", fch, NULL, ch, TO_CHAR);
				do_enter(fch, argument);
			}
		}

		if (portal != NULL && portal->value[0] == -1) {
			act("$p slowly closes in on itself.", ch, portal, NULL, TO_CHAR);
			if (ch->in_room == old_room) {
				act("$p slowly closes in on itself.", ch, portal, NULL, TO_ROOM);
			} else if (old_room->people != NULL) {
				act("$p slowly closes in on itself.",
				    old_room->people, portal, NULL, TO_CHAR);
				act("$p fades out of existence.",
				    old_room->people, portal, NULL, TO_ROOM);
			}
			extract_obj(portal);
		}


		/*
		 * If someone is following the char, these triggers get activated
		 * for the followers before the char, but it's safer this way...
		 */
		if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
			mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
		if (!IS_NPC(ch))
			mp_greet_trigger(ch);


		return;
	}
}
