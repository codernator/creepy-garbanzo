#include "merc.h"



ROOM_INDEX_DATA *get_scan_room(ROOM_INDEX_DATA * room, int direction, int distance);


/***************************************************************************
*	local declarations
***************************************************************************/
/* local functions */
static void scan_list(ROOM_INDEX_DATA * scan_room, CHAR_DATA * ch, int depth, int door, const char *argument);

/*
 * Right Here
 * Nearby South
 * Not Far South
 * Far off South
 */
static char *const distance_name[6] =
{
	"`#Right Here``:\n\r",
	"`!Nearby `1%s``:\n\r",
	"`!Not Far `1%s``:\n\r",
	"`!Far off `1%s``:\n\r",
	"`!Really Far `1%s``:\n\r",
	"`!Really Really Far `1%s``:\n\r"
};

static char *const direction_name[] =
{
	"North", "East", "South", "West", "Up", "Down"
};




/***************************************************************************
*	do_scan
*
*	scan the area
***************************************************************************/
void do_scan(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *room;
	int max_dist;
	int dist;
	int dir;
	int chosen_dir;


	/* allow for "scan north", "scan south" etc. */
	chosen_dir = -1;
	for (dir = 0; dir < 6; dir++) {
		if (!str_prefix(direction_name[dir], argument)) {
			chosen_dir = dir;
			break;
		}
	}

	/* scan the current room */
	act("$n looks all around.", ch, NULL, NULL, TO_ROOM);
	scan_list(ch->in_room, ch, 0, -1, argument);


	/* get the maximum distance - 3 if affected by farsight
	 * otherwise it is just 1 room */
	max_dist = (is_affected(ch, skill_lookup("farsight"))) ? 3 : 1;

	/* if we are scanning a given distance, then go further */
	max_dist += (chosen_dir >= 0) ? 2 : 0;

	/* scan the given distance */
	for (dist = 1; dist <= max_dist; dist++) {
		if (chosen_dir >= 0) {
			/* we have a specific direction we are scanning */
			if ((room = get_scan_room(ch->in_room, dir, dist)) != NULL)
				scan_list(room, ch, dist, dir, "");
		} else {
			/* scan in the 6 directions at the given distance
			 * this will show us a list of everything that is at
			 * the first level in the 6 directions, then everything
			 * at the second level and so on */
			for (dir = 0; dir < 6; dir++) {
				if ((room = get_scan_room(ch->in_room, dir, dist)) != NULL)
					scan_list(room, ch, dist, dir, argument);

			}
		}
	}
}




/***************************************************************************
*	scan_list
*
*	generate a list of all the people in a give room
*	or just the people who match <argument>
***************************************************************************/
static void scan_list(ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, int depth, int direction, const char *argument)
{
	CHAR_DATA *rch;
	bool found;

	found = false;
	if (scan_room != NULL) {
		for (rch = scan_room->people; rch != NULL; rch = rch->next_in_room) {
			if (rch == ch)
				continue;


			if (!IS_NPC(rch) && rch->invis_level > get_trust(ch))
				continue;


			/* if we have an argument and the characters name does not match it
			 * then just continue */
			if (argument[0] != '\0' && !is_name(argument, rch->name))
				continue;

			if (can_see(ch, rch)) {
				if (!found) {
					/* print the header */
					printf_to_char(ch, distance_name[depth], (direction >= 0) ? direction_name[direction] : "");
					found = true;
				}

				printf_to_char(ch, " - %s\n\r", PERS(rch, ch));
			}
		}
	}
}


/***************************************************************************
*	utility functions
***************************************************************************/
/***************************************************************************
*	get_scan_room
*
*	get a room in a direct <direction> - distance rooms away
*	returns NULL if there is no room there
***************************************************************************/
ROOM_INDEX_DATA *get_scan_room(ROOM_INDEX_DATA *room,
			       int		direction,
			       int		distance)
{
	ROOM_INDEX_DATA *scan_room;
	EXIT_DATA *exit;
	int depth;

	scan_room = room;
	if (NULL != scan_room) {
		for (depth = 1; depth <= distance; depth++) {
			if ((exit = scan_room->exit[direction]) != NULL)
				/* we have an exit in that direction, so set
				 * scan_room to that room and keep going */
				scan_room = exit->u1.to_room;
			else
				/* there is not an exit in that direction -
				*  which means we fell short so return NULL */
				return NULL;
		}
	}


	return (scan_room == room) ? NULL : scan_room;
}

/***************************************************************************
*	get_char_scan
*
*	get a character that is within scan-able range
*	this function will be used by a couple of skills
***************************************************************************/
CHAR_DATA *get_char_scan(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *room;
	CHAR_DATA *rch;
	CHAR_DATA *vch;
	int max_dist;
	int dist;
	int dir;

	vch = NULL;
	if ((vch = get_char_room(ch, argument)) == NULL) {
		/* get the maximum distance - 3 if affected by farsight
		 * otherwise it is just 1 room */
		max_dist = (is_affected(ch, skill_lookup("farsight"))) ? 3 : 1;

		/* loop through the 6 directions */
		for (dir = 0; dir < 6; dir++) {
			/* and go <dist> rooms deep */
			for (dist = 1; dist <= max_dist; dist++) {
				/* get a room */
				if ((room = get_scan_room(ch->in_room, dir, dist)) != NULL) {
					/* see if anyone in the room matches the argument */
					for (rch = room->people; rch != NULL; rch = rch->next_in_room) {
						if (can_see(ch, rch) && is_name(argument, rch->name)) {
							/* we found a match, set the return value and break */
							vch = rch;
							break;
						}
					}
				}

				if (vch != NULL)
					/* we have a result */
					break;
			}

			if (vch != NULL)
				/* we have a result */
				break;
		}
	}

	return vch;
}
