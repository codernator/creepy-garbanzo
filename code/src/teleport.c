/***************************************************************************
*	includes
***************************************************************************/
#include "merc.h"
#include "recycle.h"


extern DECLARE_DO_FUN(do_look);
extern DECLARE_DO_FUN(do_help);


#define _MAX_FOUND              200

void do_teleport(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *teleporter;
	OBJ_DATA *obj, *opending;
	OBJ_DATA *in_obj;
	ROOM_INDEX_DATA *location;
	char arg[MIL];
	BUFFER *buf;
	int count;
	int temp;

	argument = one_argument(argument, arg);
	for (teleporter = ch->in_room->contents; teleporter != NULL; teleporter = teleporter->next_content)
		if (teleporter->item_type == ITEM_TELEPORT && can_see_obj(ch, teleporter))
			break;

	if (teleporter == NULL) {
		send_to_char("You need to find a teleporter first.\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "00000")) {
		send_to_char("That is a private teleporter ID.\n\r", ch);
		return;
	}

	if ((!str_prefix(arg, "scan")) || (!str_prefix(arg, "index")) || (arg[0] == '\0')) {
		send_to_char("`BTeleport Index - Public Teleportation Systems\n\r", ch);
		send_to_char("`4---------------------------------------------\n\r\n\r", ch);
		send_to_char("`P 00000 - `5Unlisted\n\r\n\r", ch);

		count = 0;
		buf = new_buf();
        opending = object_iterator_start(&object_empty_filter);
        while ((obj = opending) != NULL) {
            opending = object_iterator(obj, &object_empty_filter);
			if (!can_see_obj(ch, obj)
			    || !is_name("teleporter_public", obj->name))
				continue;

			count++;
			for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj) {
			}

			if (in_obj->in_room != NULL && can_see_room(ch, in_obj->in_room)) {
				printf_buf(buf, "`P %05d - `5%s\n\r",
					   in_obj->in_room->vnum,
					   in_obj->in_room->name);
			}

			if (count++ >= _MAX_FOUND)
				break;
		}
		page_to_char(buf_string(buf), ch);
		free_buf(buf);

		if (IS_IMMORTAL(ch)) {
			count = 0;
			buf = new_buf();
            opending = object_iterator_start(&object_empty_filter);
            while ((obj = opending) != NULL) {
                opending = object_iterator(obj, &object_empty_filter);
				if (!can_see_obj(ch, obj)
				    || !is_name("teleporter_private", obj->name))
					continue;

				count++;
				for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj) {
				}

				if (in_obj->in_room != NULL && can_see_room(ch, in_obj->in_room)) {
					printf_buf(buf, "`P %05d - `5%s\n\r",
						   in_obj->in_room->vnum,
						   in_obj->in_room->name);
				}


				if (count >= _MAX_FOUND)
					break;
			}

			page_to_char(buf_string(buf), ch);
			free_buf(buf);
		}

		send_to_char("\n\r```5This teleporter's ID is `P", ch);

		if ((teleporter->value[2]) != 0)
			printf_to_char(ch, "00000```5.\n\r");
		else
			printf_to_char(ch, "%05d```5.\n\r", ch->in_room->vnum);

		return;
	}

	temp = parse_int(arg);
	if (temp < 0)
		return;

	location = get_room_index(temp);

	if (ch->in_room == location) {
		send_to_char("You are already there.\n\r", ch);
		return;
	}
	if ((location == NULL)) {
		send_to_char("No teleporter has that ID.\n\r", ch);
		return;
	}

	if (IS_IMMORTAL(ch))
		ch->gold += 1;

	if ((ch->gold < 1) && (ch->silver < 10)) {
		send_to_char("Teleportation costs `#1 gold``.\n\r", ch);
		return;
	}

	teleporter = get_obj_list(ch, "teleporter", location->contents);
	if (teleporter == NULL) {
		send_to_char("No teleporter has that ID.\n\r", ch);
		return;
	}

	if ((ch->gold < 1) && (ch->silver > 10))
		ch->silver -= 10;
	else
		ch->gold -= 1;

	act("$n steps into a teleporter and vanishes.", ch, NULL, NULL, TO_ROOM);
	act("You step into a teleporter and vanish.", ch, NULL, NULL, TO_CHAR);
	char_from_room(ch);
	char_to_room(ch, location);
	act("With a blinding flash, $n appears in a teleporter.", ch, NULL, NULL, TO_ROOM);
	act("You reappear in another place.", ch, NULL, NULL, TO_CHAR);
	do_look(ch, "auto");
	return;
}
