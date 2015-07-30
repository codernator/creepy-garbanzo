#include "merc.h"
#include "character.h"
#include "tables.h"
#include "interp.h"

#include <stdio.h>


extern unsigned int parse_unsigned_int(char *string);
extern bool mp_percent_trigger(CHAR_DATA * mob, CHAR_DATA * ch, const void *arg1, const void *arg2, int type);
extern bool mp_exit_trigger(CHAR_DATA * ch, int dir);
extern void mp_greet_trigger(CHAR_DATA * ch);

/***************************************************************************
*	direction constants
***************************************************************************/
char *const dir_name[] =
{
	"north", "east", "south", "west", "up", "down"
};

const int rev_dir[] =
{
	2, 3, 0, 1, 5, 4
};

const int movement_loss[SECT_MAX] =
{
	1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6
};



/***************************************************************************
*	local functions
***************************************************************************/
bool has_key(CHAR_DATA * ch, long key);
int check_dir(CHAR_DATA * ch, int dir);
int direction_lookup(char *name);


/***************************************************************************
*	direction_lookup
*
*	take a direction like "s" or "south" and resolve it to its
*	numeric value
***************************************************************************/
int direction_lookup(char *direction)
{
	int idx;

	for (idx = 0; idx < 6; idx++) {
		if (LOWER(direction[0]) == LOWER(dir_name[idx][0])
		    || !str_cmp(direction, dir_name[idx]))
			return idx;
	}

	return -1;
}

/***************************************************************************
*	check_dir
*
*	check to make sure that a valid direction is given - handle
*	drunk, confusion, and IDIOT flags
***************************************************************************/
int check_dir(CHAR_DATA *ch, int dir)
{
	if (dir < 0 || dir > 5)
		return dir;

	return dir;
}

/***************************************************************************
*	move_char
*
*	move a character in a given direction
***************************************************************************/
void move_char(CHAR_DATA *ch, int door, bool follow)
{
	CHAR_DATA *fch;
	CHAR_DATA *fch_next;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	DESCRIPTOR_DATA *d;
	EXIT_DATA *pexit;
	char buf[MSL];
	int race;


	if (door < 0 || door > 5) {
		bug("Do_move: bad door %d.", door);
		return;
	}

	/*
	 * Exit trigger, if activated, bail out. Only PCs are triggered.
	 */
	if (!IS_NPC(ch) && mp_exit_trigger(ch, door))
		return;

	in_room = ch->in_room;
	if ((pexit = in_room->exit[door]) == NULL
	    || (to_room = pexit->u1.to_room) == NULL
	    || !can_see_room(ch, pexit->u1.to_room)) {
		send_to_char("Alas, you cannot go that way.\n\r", ch);
		return;
	}

	if (is_affected(ch, gsp_web)) {
		send_to_char("You are rooted to the ground with a sticky web.\n\r", ch);
		return;
	}

	if (IS_SET((int)pexit->exit_info, EX_CLOSED)
	    && (!IS_AFFECTED(ch, AFF_PASS_DOOR) || IS_SET((int)pexit->exit_info, EX_NOPASS))
	    && !IS_TRUSTED(ch, ANGEL)) {
		act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
		return;
	}

	if (room_is_private(to_room) && ch->pk_timer > 0) {
		send_to_char("You can't enter that room with a pk timer!\n\r", ch); return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)
	    && ch->master != NULL
	    && in_room == ch->master->in_room) {
		send_to_char("What?  And leave your beloved master?\n\r", ch);
		return;
	}


	if (!is_room_owner(ch, to_room) && room_is_private(to_room)) {
		send_to_char("That room is private right now. (You are not the owner)\n\r", ch);
		return;
	}

	if (!IS_NPC(ch)) {
		int iClass, iGuild;
		int move;

		for (iClass = 0; iClass < MAX_CLASS; iClass++) {
			for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
				if (iClass != ch->class
				    && to_room->vnum == class_table[iClass].guild[iGuild]) {
					send_to_char("You aren't allowed in there.\n\r", ch);
					return;
				}
			}
		}

		if (in_room->sector_type == SECT_AIR
		    || to_room->sector_type == SECT_AIR) {
			if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch)) {
				send_to_char("You can't fly.\n\r", ch);
				return;
			}
		}

		if ((in_room->sector_type == SECT_WATER_NOSWIM
		     || to_room->sector_type == SECT_WATER_NOSWIM)
		    && !IS_AFFECTED(ch, AFF_FLYING)) {
			OBJ_DATA *obj;
			bool found;

			/*
			 * Look for a boat.
			 */
			found = FALSE;

			if (IS_IMMORTAL(ch))
				found = TRUE;

			for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
				if (obj->item_type == ITEM_BOAT) {
					found = TRUE;
					break;
				}
			}

			if (!found) {
				send_to_char("You need a boat to go there.\n\r", ch);
				return;
			}
		}

		move = movement_loss[UMIN(SECT_MAX - 1, in_room->sector_type)]
		       + movement_loss[UMIN(SECT_MAX - 1, to_room->sector_type)];

		move /= 2;      /* i.e. the average */


		/* conditional effects */
		if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_HASTE))
			move /= 2;

		if (IS_AFFECTED(ch, AFF_SLOW))
			move *= 2;

		if (ch->move < move) {
			send_to_char("You are too exhausted.\n\r", ch);
			return;
		}

		WAIT_STATE(ch, 1);
		if (ch->race != (race = race_lookup("vampire"))) {
			if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] < -50 && ch->level < LEVEL_IMMORTAL) {
				if (ch->pcdata->condition[COND_THIRST] >= -75)
					move *= 2;
				else if (ch->pcdata->condition[COND_THIRST] >= -100)
					move *= 4;
				else if (ch->pcdata->condition[COND_THIRST] >= -125)
					move *= 8;
				else if (ch->pcdata->condition[COND_THIRST] != -151)
					move *= 16;
			}
		}
		ch->move -= move;
	}

	if (is_affected(ch, gsp_fear)) {
		send_to_char("You feel too afraid to move..\n\r", ch);
		WAIT_STATE(ch, 2);
		return;
	}

	if (!IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO) {
		act("$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM);
	} else {
		CHAR_DATA *watcher;
		CHAR_DATA *next_watcher;

		for (watcher = ch->in_room->people; watcher; watcher = next_watcher) {
			next_watcher = watcher->next_in_room;

			if (IS_SET(watcher->act, PLR_HOLYLIGHT) && can_see(watcher, ch)) {
				sprintf(buf, "%s leaves %s.\n\r", ch->name, dir_name[door]);
				send_to_char(buf, watcher);
			}
		}
	}

	if (IS_SET(ch->act, PLR_ESP)) {
		for (d = descriptor_list; d != NULL; d = d->next) {
			CHAR_DATA *wch;

			if (d->connected != CON_PLAYING)
				continue;

			wch = (d->original != NULL) ? d->original : d->character;

			if (wch->linked == ch) {
				if (can_see(wch, ch)) {
					sprintf(buf, "%s's thoughts `&*`7*`8*`!BOOM`8*`7*`&*`7 into your minds as '`&%s`7'.\n\r", ch->name, dir_name[door]);
					send_to_char(buf, wch);
				} else {
					sprintf(buf, "%s's thoughts `&*`7*`8*`!BOOM`8*`7*`&*`7 into your minds as '`&%s`7'.\n\r", "Someone", dir_name[door]);
					send_to_char(buf, wch);
				}
			}
		}
	}


	char_from_room(ch);
	char_to_room(ch, to_room);
	if (!IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO) {
		act("$n has arrived.", ch, NULL, NULL, TO_ROOM);
	} else {
		CHAR_DATA *watcher;
		CHAR_DATA *next_watcher;

		for (watcher = ch->in_room->people; watcher; watcher = next_watcher)
			next_watcher = watcher->next_in_room;
	}

	if (is_affected_room(ch->in_room, gsp_faerie_fog)) {
		send_to_char("You are surrounded by a `Ppurple haze`` and are revealed!\n\r", ch);
		affect_strip(ch, gsp_invisibility);
		affect_strip(ch, gsp_mass_invisibility);
		affect_strip(ch, gsp_sneak);
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
		REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
		REMOVE_BIT(ch->affected_by, AFF_SNEAK);
	}

	if ((IS_SET(in_room->room_flags, ROOM_INDOORS))
	    && (!IS_SET(to_room->room_flags, ROOM_INDOORS)))
		do_weather(ch, NULL);

	do_look(ch, "auto");

	if (in_room == to_room) /* no circular follows */
		return;

	for (fch = in_room->people; fch != NULL; fch = fch_next) {
		fch_next = fch->next_in_room;

		if (fch->master == ch
		    && IS_AFFECTED(fch, AFF_CHARM)
		    && fch->position < POS_STANDING)
			do_stand(fch, "");

		if (fch->master == ch
		    && fch->position == POS_STANDING
		    && can_see_room(fch, to_room)) {
			if (IS_NPC(fch)
			    && IS_SET(fch->act, ACT_AGGRESSIVE)
			    && IS_AFFECTED(fch, AFF_CHARM)) {
				act("You can't force $N into moving.",
				    ch, NULL, fch, TO_CHAR);
				act("You aren't allowed to move.",
				    fch, NULL, NULL, TO_CHAR);
				continue;
			}

			act("You follow $N.", fch, NULL, ch, TO_CHAR);
			move_char(fch, door, TRUE);
		}
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


/***************************************************************************
*	do_push
*
*	push a character out of the room
***************************************************************************/
void do_push(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	CHAR_DATA *victim;
	EXIT_DATA *pexit;
	char arg[MIL];
	int door;
	int result;
	int fail;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Who do you want to push?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch->position == POS_RESTING) {
		send_to_char("Maybe you should stand first?\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("You try to push yourself and end up looking like a `PLOSER`7!\n\r", ch);
		return;
	}

	if (!IS_AWAKE(ch)) {
		send_to_char("Try waking up first!\n\r", ch);
		return;
	}

	if (victim->position == POS_RESTING || !IS_AWAKE(victim)) {
		act("$N is resting. Try dragging $M.\n\r", ch, NULL, victim, TO_CHAR);
		return;
	}


	if (ch->position == POS_FIGHTING) {
		send_to_char("Maybe you should stop fighting first?\n\r", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		act("$N is fighting. Wait your turn!\n\r", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_PUSH_NO_DRAG)
	    && (victim->ticks_since_last_fight > 3)) {
		send_to_char("You can't do that here.\n\r", ch);
		return;
	}

	if (IS_SHOPKEEPER(victim)) {
		send_to_char("The shopkeeper wouldn't like that.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if ((door = direction_lookup(arg)) < 0) {
		send_to_char("You can't push them through the door!\n\r", ch);
		return;
	}



	in_room = ch->in_room;
	if ((pexit = in_room->exit[door]) == NULL
	    || (to_room = pexit->u1.to_room) == NULL
	    || pexit->u1.vnum == 0) {
		send_to_char("There's no exit in that direction!\n\r", ch);
		return;
	}

	if (IS_NPC(victim))
		ch->mobmem = NULL;

	fail = 35;
	result = number_percent();
	if ((get_curr_stat(ch, STAT_STR) > get_curr_stat(victim, STAT_STR))
	    || (get_curr_stat(ch, STAT_STR) == get_curr_stat(victim, STAT_STR) && result <= fail)) {
		push_char(ch, victim, door, FALSE);
		if (!IS_NPC(ch) || IS_SET(ch->act, PLR_BATTLE))
			victim->last_fight = time(NULL);
		if (!IS_NPC(victim) || IS_SET(victim->act, PLR_BATTLE))
			ch->last_fight = time(NULL);
	} else {
		act("$n tries unsuccessfully to push $N.\n\r", ch, NULL, victim, TO_ROOM);
		act("$N looks at you with contempt and ignores you.\n\r", ch, NULL, victim, TO_CHAR);
	}

	return;
}



/***************************************************************************
*	do_drag
*
*	drag a character from one room to another
***************************************************************************/
void do_drag(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	char arg[MIL];
	int door;
	int result;
	int fail;

	argument = one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Who do you want to drag?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch->position == POS_RESTING) {
		send_to_char("Maybe you should stand first?\n\r", ch);
		return;
	}

	if (victim->position == POS_STANDING) {
		act("$N is standing. Try pushing $M.\n\r", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_AWAKE(ch)) {
		send_to_char("Try waking up first!\n\r", ch);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		send_to_char("Maybe you should stop fighting first?\n\r", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		act("$N is fighting. Wait your turn!\n\r", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NO_PUSH_NO_DRAG)
	    && (victim->ticks_since_last_fight > 3)) {
		send_to_char("You can't do that here.\n\r", ch);
		return;
	}

	if (IS_SHOPKEEPER(victim)) {
		send_to_char("The shopkeeper wouldn't like that.\n\r", ch);
		return;
	}



	argument = one_argument(argument, arg);
	if ((door = direction_lookup(arg)) < 0)
		return;

	in_room = ch->in_room;
	if ((pexit = in_room->exit[door]) == NULL
	    || (to_room = pexit->u1.to_room) == NULL
	    || pexit->u1.vnum == 0
	    || !can_see_room(ch, pexit->u1.to_room)
	    || !can_see_room(victim, pexit->u1.to_room)) {
		send_to_char("There's no exit in that direction!\n\r", ch);
		return;
	}

	if (IS_NPC(victim))
		ch->mobmem = NULL;

	fail = 40;
	result = number_percent();
	if ((get_curr_stat(ch, STAT_STR) > get_curr_stat(victim, STAT_STR))
	    || (get_curr_stat(ch, STAT_STR) == get_curr_stat(victim, STAT_STR) && result <= fail)) {
		drag_char(ch, victim, door, FALSE);
		victim->last_fight = time(NULL);
		ch->last_fight = time(NULL);
	} else {
		printf_to_char(victim, "%s tries to drag you, but fails.\n\r", ch->name);
		act("$n tries unsuccessfully to drag $M.\n\r", ch, NULL, victim, TO_NOTVICT);
		printf_to_char(ch, "You try to drag %s, but `1fail``.\n\r", victim->name);
	}

	return;
}


/***************************************************************************
*	push_char
*
*	push a character out of the room
***************************************************************************/
void push_char(CHAR_DATA *ch, CHAR_DATA *vch, int door, bool follow)
{
	CHAR_DATA *fch;
	CHAR_DATA *fch_next;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	char buf[256];

	if (check_affected(vch, "web")) {
		send_to_char("You attempt to leave the room, but the webs hold you tight.\n\r", ch);
		act("$n struggles vainly against the webs which hold $m in place.", ch, NULL, NULL, TO_ROOM);
		return;
	}

	if (door < 0 || door > 5) {
		bug("Do_move: bad door %d.", door);
		return;
	}

	in_room = vch->in_room;
	if ((pexit = in_room->exit[door]) == NULL
	    || (to_room = pexit->u1.to_room) == NULL
	    || pexit->u1.vnum == 0) {
		send_to_char("You are `!slammed`` up against a wall.\n\r", vch);
		return;
	}

	if (IS_SET((int)pexit->exit_info, EX_CLOSED)
	    && !IS_AFFECTED(vch, AFF_PASS_DOOR)) {
		act("The $d is closed, you hit your head.", vch, NULL, pexit->keyword, TO_CHAR);
		return;
	}

	if (room_is_private(to_room)) {
		send_to_char("That room is private right now.\n\r", vch);
		return;
	}


	if (in_room->sector_type == SECT_AIR
	    || to_room->sector_type == SECT_AIR) {
		if (!IS_AFFECTED(vch, AFF_FLYING) && !IS_IMMORTAL(vch)) {
			send_to_char("You are pushed into the air and fall down.\n\r", vch);
			return;
		}
	}


	WAIT_STATE(vch, 1);

	printf_to_char(ch, "You `!slam`` into %s, pushing them %s!\n\r", vch->name, dir_name[door]);
	sprintf(buf, "``$n `!slams`` into $N, pushing $M %s!", dir_name[door]);
	act(buf, ch, NULL, vch, TO_ROOM);

	char_from_room(vch);
	char_to_room(vch, to_room);

	act("$n flies into the room.", vch, NULL, NULL, TO_ROOM);
	do_look(vch, "auto");

	if (in_room == to_room)  /* no circular follows */
		return;

	for (fch = in_room->people; fch != NULL; fch = fch_next) {
		fch_next = fch->next_in_room;

		if (fch->master == vch
		    && IS_AFFECTED(fch, AFF_CHARM)
		    && fch->position < POS_STANDING)
			do_stand(fch, "");

		if (fch->master == vch && fch->position == POS_STANDING) {
			if (IS_SET(vch->in_room->room_flags, ROOM_LAW)
			    && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE))) {
				act("You can't bring $N into the city.", vch, NULL, fch, TO_CHAR);
				act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR);
				return;
			}

			act("You follow $N.", fch, NULL, vch, TO_CHAR);
			move_char(fch, door, TRUE);
		}
	}

	return;
}



/***************************************************************************
*	drag_char
*
*	drag a character from a room to another
***************************************************************************/
void drag_char(CHAR_DATA *ch, CHAR_DATA *victim, int door, bool follow)
{
	CHAR_DATA *fch;
	CHAR_DATA *fch_next;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;

	if (check_affected(ch, "web")) {
		send_to_char("You attempt to leave the room, but the webs hold you tight.\n\r", ch);
		return;
	}

	if (door < 0 || door > 5) {
		bug("Do_move: bad door %d.", door);
		return;
	}

	in_room = ch->in_room;
	pexit = in_room->exit[door];
	to_room = pexit->u1.to_room;

	furniture_check(victim);

	if ((pexit) == NULL
	    || (to_room) == NULL
	    || !can_see_room(ch, pexit->u1.to_room)) {
		if (IS_AWAKE(victim)) {
			send_to_char("You get dragged around the room!\n\r", ch);
			return;
		}
	}

	if (pexit->u1.vnum == 0 ||
	    pexit->u1.to_room == NULL) {
		send_to_char("For some reason you dream that your head is being slammed into a wall!\n\r", victim);
		send_to_char("There is not an exit in that direction.\n\r", ch);
		return;
	}
	if (IS_SET((int)pexit->exit_info, EX_CLOSED)
	    && !IS_AFFECTED(ch, AFF_PASS_DOOR)) {
		act("The $d is closed, you are dragged against it.", victim, NULL, pexit->keyword, TO_CHAR);
		return;
	}


	if (room_is_private(to_room)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}


	if (in_room->sector_type == SECT_AIR
	    || to_room->sector_type == SECT_AIR) {
		if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch)) {
			send_to_char("You are dragged into the air and fall down.\n\r", victim);
			return;
		}
	}


	WAIT_STATE(ch, 1);

	if (ch->invis_level < LEVEL_HERO) {
		act("$n is dragged $T.", victim, NULL, dir_name[door], TO_ROOM);
		printf_to_char(ch, "You are dragged %s!\n\r", dir_name[door]);
	}

	send_to_char("You drag your victim!\n\r", ch);
	char_from_room(ch);
	char_from_room(victim);
	char_to_room(ch, to_room);
	char_to_room(victim, to_room);

	if (!IS_AFFECTED(ch, AFF_SNEAK)
	    && (IS_NPC(ch) || (ch->invis_level < LEVEL_HERO))) {
		act("$n drags $N into the room.\n\r", ch, NULL, victim, TO_ROOM);
		do_look(ch, "auto");
	}

	if (in_room == to_room)  /* no circular follows */
		return;

	for (fch = in_room->people; fch != NULL; fch = fch_next) {
		fch_next = fch->next_in_room;

		if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
		    && fch->position < POS_STANDING)
			do_stand(fch, "");

		if (fch->master == ch && fch->position == POS_STANDING) {
			if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
			    && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE))) {
				act("You can't bring $N into the city.", ch, NULL, fch, TO_CHAR);
				act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR);
				return;
			}

			act("You follow $N.", fch, NULL, ch, TO_CHAR);
			move_char(fch, door, TRUE);
		}
	}

	return;
}

void do_follow(CHAR_DATA *ch, char *argument)
{
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
	CHAR_DATA *victim;
	char arg[MIL];

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Follow whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL) {
		act("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR);
		return;
	}

	if (victim == ch) {
		if (ch->master == NULL) {
			send_to_char("You already follow yourself.\n\r", ch);
			return;
		}
		stop_follower(ch);
		return;
	}

	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOFOLLOW) && !IS_IMMORTAL(ch)) {
		act("$N doesn't seem to want any followers.\n\r", ch, NULL, victim, TO_CHAR);
		return;
	}

	REMOVE_BIT(ch->act, PLR_NOFOLLOW);

	if (ch->master != NULL)
		stop_follower(ch);

	add_follower(ch, victim);
}

void do_group(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg[MIL];
	CHAR_DATA *victim;

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		CHAR_DATA *gch;
		CHAR_DATA *leader;

		leader = (ch->leader != NULL) ? ch->leader : ch;
		(void)snprintf(buf, MSL, "%s's group:\n\r", PERS(leader, ch));
		send_to_char(buf, ch);

		for (gch = char_list; gch != NULL; gch = gch->next) {
			if (is_same_group(gch, ch)) {
				(void)snprintf(buf, MSL, "%-5s `!%4d``/`1%4d`` hp `@%4d``/`2%4d`` mana `$%4d``/`4%4d`` mv `&%5d`` xp\n\r",
					       capitalize(PERS(gch, ch)),
					       gch->hit, gch->max_hit,
					       gch->mana, gch->max_mana,
					       gch->move, gch->max_move,
					       gch->exp);
				send_to_char(buf, ch);
			}
		}
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch)) {
		send_to_char("But you are following someone else!\n\r", ch);
		return;
	}

	if (victim->master != ch && ch != victim) {
		act("$N isn't following you.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(victim, AFF_CHARM)) {
		send_to_char("You can't remove charmed mobs from your group.\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		act("You like your master too much to leave $m!", ch, NULL, victim, TO_VICT);
		return;
	}

	if (is_same_group(victim, ch) && ch != victim) {
		victim->leader = NULL;
		act("$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT);
		act("$n removes you from $s group.", ch, NULL, victim, TO_VICT);
		act("You remove $N from your group.", ch, NULL, victim, TO_CHAR);
		return;
	}

	victim->leader = ch;
	act("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT);
	act("You join $n's group.", ch, NULL, victim, TO_VICT);
	act("$N joins your group.", ch, NULL, victim, TO_CHAR);
}

void do_split(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg1[MIL], arg2[MIL];
	CHAR_DATA *gch;
	int members;
	unsigned int amount_gold = 0, amount_silver = 0;
	unsigned int share_gold, share_silver;
	unsigned int extra_gold, extra_silver;

	argument = one_argument(argument, arg1);
	(void)one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("Split how much?\n\r", ch);
		return;
	}

	amount_silver = parse_unsigned_int(arg1);

	if (arg2[0] != '\0')
		amount_gold = parse_unsigned_int(arg2);

	if (amount_gold == 0 && amount_silver == 0) {
		send_to_char("You hand out zero coins, but no one notices.\n\r", ch);
		return;
	}

	if (ch->gold < amount_gold || ch->silver < amount_silver) {
		send_to_char("You don't have that much to split.\n\r", ch);
		return;
	}

	members = 0;
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
		if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
			members++;

	if (members < 2) {
		send_to_char("Just keep it all.\n\r", ch);
		return;
	}

	share_silver = amount_silver / members;
	extra_silver = amount_silver % members;

	share_gold = amount_gold / members;
	extra_gold = amount_gold % members;

	if (share_gold == 0 && share_silver == 0) {
		send_to_char("Don't even bother, cheapskate.\n\r", ch);
		return;
	}

	ch->silver -= amount_silver;
	ch->silver += share_silver + extra_silver;
	ch->gold -= amount_gold;
	ch->gold += share_gold + extra_gold;

	if (share_silver > 0) {
		(void)snprintf(buf, 2 * MIL,
			       "You split %u silver coins. Your share is %u silver.\n\r",
			       amount_silver, share_silver + extra_silver);
		send_to_char(buf, ch);
	}

	if (share_gold > 0) {
		(void)snprintf(buf, 2 * MIL,
			       "You split %u gold coins. Your share is %u gold.\n\r",
			       amount_gold, share_gold + extra_gold);
		send_to_char(buf, ch);
	}

	if (share_gold == 0) {
		(void)snprintf(buf, 2 * MIL, "$n splits %u silver coins. Your share is %u silver.",
			       amount_silver, share_silver);
	} else if (share_silver == 0) {
		(void)snprintf(buf, 2 * MIL, "$n splits %u gold coins. Your share is %u gold.",
			       amount_gold, share_gold);
	} else {
		(void)snprintf(buf, 2 * MIL,
			       "$n splits %u silver and %u gold coins, giving you %u silver and %u gold.\n\r",
			       amount_silver, amount_gold, share_silver, share_gold);
	}

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
		if (gch != ch && is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM)) {
			act(buf, ch, NULL, gch, TO_VICT);
			gch->gold += share_gold;
			gch->silver += share_silver;
		}
	}
}


/***************************************************************************
*	direction commands
***************************************************************************/
/***************************************************************************
*	do_north
***************************************************************************/
void do_north(CHAR_DATA *ch, char *argument)
{
	move_char(ch, check_dir(ch, DIR_NORTH), FALSE);
}


/***************************************************************************
*	do_east
***************************************************************************/
void do_east(CHAR_DATA *ch, char *argument)
{
	move_char(ch, check_dir(ch, DIR_EAST), FALSE);
}


/***************************************************************************
*	do_south
***************************************************************************/
void do_south(CHAR_DATA *ch, char *argument)
{
	move_char(ch, check_dir(ch, DIR_SOUTH), FALSE);
}


/***************************************************************************
*	do_west
***************************************************************************/
void do_west(CHAR_DATA *ch, char *argument)
{
	move_char(ch, check_dir(ch, DIR_WEST), FALSE);
}


/***************************************************************************
*	do_up
***************************************************************************/
void do_up(CHAR_DATA *ch, char *argument)
{
	move_char(ch, check_dir(ch, DIR_UP), FALSE);
}

/***************************************************************************
*	do_down
***************************************************************************/
void do_down(CHAR_DATA *ch, char *argument)
{
	move_char(ch, check_dir(ch, DIR_DOWN), FALSE);
}



/***************************************************************************
*	find_door
*
*	find a door by direction or name
***************************************************************************/
int find_door(CHAR_DATA *ch, char *arg)
{
	EXIT_DATA *pexit;
	int door;

	if ((door = direction_lookup(arg)) < 0) {
		for (door = 0; door <= 5; door++) {
			if ((pexit = ch->in_room->exit[door]) != NULL
			    && IS_SET((int)pexit->exit_info, EX_ISDOOR)
			    && pexit->keyword != NULL
			    && is_name(arg, pexit->keyword))
				return door;
		}

		act("I see no $T here.", ch, NULL, arg, TO_CHAR);
		return -1;
	}

	if ((pexit = ch->in_room->exit[door]) == NULL) {
		act("I see no door $T here.", ch, NULL, arg, TO_CHAR);
		return -1;
	}

	if (!IS_SET((int)pexit->exit_info, EX_ISDOOR)) {
		send_to_char("You can't do that.\n\r", ch);
		return -1;
	}

	return door;
}


/***************************************************************************
*	find_exit
*
*	find an exit by direction or name
***************************************************************************/
int find_exit(CHAR_DATA *ch, char *arg)
{
	EXIT_DATA *pexit;
	int door;

	if ((door = direction_lookup(arg)) < 0) {
		for (door = 0; door <= 5; door++) {
			if ((pexit = ch->in_room->exit[door]) != NULL
			    && IS_SET((int)pexit->exit_info, EX_ISDOOR)
			    && pexit->keyword != NULL
			    && is_name(arg, pexit->keyword))
				return door;
		}
		act("I see no $T here.", ch, NULL, arg, TO_CHAR);
		return -1;
	}

	if ((pexit = ch->in_room->exit[door]) == NULL) {
		act("I see no door $T here.", ch, NULL, arg, TO_CHAR);
		return -1;
	}

	return door;
}



/***************************************************************************
*	do_open
*
*	open a door or a container
***************************************************************************/
void do_open(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int door;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Open what?\n\r", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* open portal */
		if (obj->item_type == ITEM_PORTAL) {
			if (!IS_SET((int)obj->value[1], EX_ISDOOR)) {
				send_to_char("You can't do that.\n\r", ch);
				return;
			}

			if (!IS_SET((int)obj->value[1], EX_CLOSED)) {
				send_to_char("It's already open.\n\r", ch);
				return;
			}

			if (IS_SET((int)obj->value[1], EX_LOCKED)) {
				send_to_char("It's locked.\n\r", ch);
				return;
			}

			REMOVE_BIT(obj->value[1], EX_CLOSED);
			act("You open $p.", ch, obj, NULL, TO_CHAR);
			act("$n opens $p.", ch, obj, NULL, TO_ROOM);
			return;
		}

		/* 'open object' */
		if (obj->item_type != ITEM_CONTAINER) {
			send_to_char("That's not a container.\n\r", ch);
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSED)) {
			send_to_char("It's already open.\n\r", ch);
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSEABLE)) {
			send_to_char("You can't do that.\n\r", ch);
			return;
		}
		if (IS_SET(obj->value[1], CONT_LOCKED)) {
			send_to_char("It's locked.\n\r", ch);
			return;
		}

		REMOVE_BIT(obj->value[1], CONT_CLOSED);
		act("You open $p.", ch, obj, NULL, TO_CHAR);
		act("$n opens $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'open door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET((int)pexit->exit_info, EX_CLOSED)) {
			send_to_char("It's already open.\n\r", ch);
			return;
		}

		if (IS_SET((int)pexit->exit_info, EX_LOCKED)) {
			send_to_char("It's locked.\n\r", ch);
			return;
		}

		REMOVE_BIT(pexit->exit_info, EX_CLOSED);
		act("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM);
		send_to_char("Ok.\n\r", ch);

		/* open the other side */
		if ((to_room = pexit->u1.to_room) != NULL
		    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		    && pexit_rev->u1.to_room == ch->in_room) {
			CHAR_DATA *rch;

			REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
			for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
				act("The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR);
		}
	}
}



/***************************************************************************
*	do_close
*
*	close a door or a container
***************************************************************************/
void do_close(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int door;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Close what?\n\r", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL) {
			if (!IS_SET((int)obj->value[1], EX_ISDOOR)
			    || IS_SET((int)obj->value[1], EX_NOCLOSE)) {
				send_to_char("You can't do that.\n\r", ch);
				return;
			}

			if (IS_SET((int)obj->value[1], EX_CLOSED)) {
				send_to_char("It's already closed.\n\r", ch);
				return;
			}

			SET_BIT(obj->value[1], EX_CLOSED);
			act("You close $p.", ch, obj, NULL, TO_CHAR);
			act("$n closes $p.", ch, obj, NULL, TO_ROOM);
			return;
		}

		/* 'close object' */
		if (obj->item_type != ITEM_CONTAINER) {
			send_to_char("That's not a container.\n\r", ch);
			return;
		}

		if (IS_SET(obj->value[1], CONT_CLOSED)) {
			send_to_char("It's already closed.\n\r", ch);
			return;
		}

		if (!IS_SET(obj->value[1], CONT_CLOSEABLE)) {
			send_to_char("You can't do that.\n\r", ch);
			return;
		}

		SET_BIT(obj->value[1], CONT_CLOSED);
		act("You close $p.", ch, obj, NULL, TO_CHAR);
		act("$n closes $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'close door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (IS_SET((int)pexit->exit_info, EX_CLOSED)) {
			send_to_char("It's already closed.\n\r", ch);
			return;
		}

		SET_BIT(pexit->exit_info, EX_CLOSED);
		act("$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM);
		send_to_char("Ok.\n\r", ch);

		/* close the other side */
		if ((to_room = pexit->u1.to_room) != NULL
		    && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
		    && pexit_rev->u1.to_room == ch->in_room) {
			CHAR_DATA *rch;

			SET_BIT(pexit_rev->exit_info, EX_CLOSED);
			for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
				act("The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR);
		}
	}
}



/***************************************************************************
*	has_key
*
*	check to see if a character has a given key
***************************************************************************/
bool has_key(CHAR_DATA *ch, long key)
{
	OBJ_DATA *obj;

	for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
		if (obj->obj_idx->vnum == key)
			return TRUE;

	return FALSE;
}


/***************************************************************************
*	do_lock
*
*	lock a container or door
***************************************************************************/
void do_lock(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int door;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Lock what?\n\r", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL) {
			if (!IS_SET((int)obj->value[1], EX_ISDOOR)
			    || IS_SET((int)obj->value[1], EX_NOCLOSE)) {
				send_to_char("You can't do that.\n\r", ch);
				return;
			}
			if (!IS_SET((int)obj->value[1], EX_CLOSED)) {
				send_to_char("It's not closed.\n\r", ch);
				return;
			}

			if (obj->value[4] < 0 || IS_SET((int)obj->value[1], EX_NOLOCK)) {
				send_to_char("It can't be locked.\n\r", ch);
				return;
			}

			if (!has_key(ch, obj->value[4])) {
				send_to_char("You lack the key.\n\r", ch);
				return;
			}

			if (IS_SET((int)obj->value[1], EX_LOCKED)) {
				send_to_char("It's already locked.\n\r", ch);
				return;
			}

			SET_BIT(obj->value[1], EX_LOCKED);
			act("You lock $p.", ch, obj, NULL, TO_CHAR);
			act("$n locks $p.", ch, obj, NULL, TO_ROOM);
			return;
		}

		/* 'lock object' */
		if (obj->item_type != ITEM_CONTAINER) {
			send_to_char("That's not a container.\n\r", ch);
			return;
		}

		if (!IS_SET(obj->value[1], CONT_CLOSED)) {
			send_to_char("It's not closed.\n\r", ch);
			return;
		}

		if (obj->value[2] < 0) {
			send_to_char("It can't be locked.\n\r", ch);
			return;
		}

		if (!has_key(ch, obj->value[2])) {
			send_to_char("You lack the key.\n\r", ch);
			return;
		}

		if (IS_SET(obj->value[1], CONT_LOCKED)) {
			send_to_char("It's already locked.\n\r", ch);
			return;
		}

		SET_BIT(obj->value[1], CONT_LOCKED);
		act("You lock $p.", ch, obj, NULL, TO_CHAR);
		act("$n locks $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'lock door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
			send_to_char("It's not closed.\n\r", ch);
			return;
		}

		if (pexit->key < 0) {
			send_to_char("It can't be locked.\n\r", ch);
			return;
		}
		if (!has_key(ch, pexit->key)) {
			send_to_char("You lack the key.\n\r", ch);
			return;
		}
		if (IS_SET((int)pexit->exit_info, EX_LOCKED)) {
			send_to_char("It's already locked.\n\r", ch);
			return;
		}

		SET_BIT(pexit->exit_info, EX_LOCKED);
		send_to_char("*Click*\n\r", ch);
		act("$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

		/* lock the other side */
		if ((to_room = pexit->u1.to_room) != NULL
		    && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
		    && pexit_rev->u1.to_room == ch->in_room)
			SET_BIT(pexit_rev->exit_info, EX_LOCKED);
	}
}


/***************************************************************************
*	do_unlock
*
*	unlock a container or door
***************************************************************************/
void do_unlock(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	char arg[MIL];
	int door;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Unlock what?\n\r", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL) {
			if (IS_SET((int)obj->value[1], EX_ISDOOR)) {
				send_to_char("You can't do that.\n\r", ch);
				return;
			}

			if (!IS_SET((int)obj->value[1], EX_CLOSED)) {
				send_to_char("It's not closed.\n\r", ch);
				return;
			}

			if (obj->value[4] < 0) {
				send_to_char("It can't be unlocked.\n\r", ch);
				return;
			}

			if (!has_key(ch, obj->value[4])) {
				send_to_char("You lack the key.\n\r", ch);
				return;
			}

			if (!IS_SET((int)obj->value[1], EX_LOCKED)) {
				send_to_char("It's already unlocked.\n\r", ch);
				return;
			}

			REMOVE_BIT(obj->value[1], EX_LOCKED);
			act("You unlock $p.", ch, obj, NULL, TO_CHAR);
			act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
			return;
		}

		/* 'unlock object' */
		if (obj->item_type != ITEM_CONTAINER) {
			send_to_char("That's not a container.\n\r", ch);
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSED)) {
			send_to_char("It's not closed.\n\r", ch);
			return;
		}
		if (obj->value[2] < 0) {
			send_to_char("It can't be unlocked.\n\r", ch);
			return;
		}
		if (!has_key(ch, obj->value[2])) {
			send_to_char("You lack the key.\n\r", ch);
			return;
		}
		if (!IS_SET(obj->value[1], CONT_LOCKED)) {
			send_to_char("It's already unlocked.\n\r", ch);
			return;
		}

		REMOVE_BIT(obj->value[1], CONT_LOCKED);
		act("You unlock $p.", ch, obj, NULL, TO_CHAR);
		act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'unlock door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET((int)pexit->exit_info, EX_CLOSED)) {
			send_to_char("It's not closed.\n\r", ch);
			return;
		}
		if (pexit->key < 0) {
			send_to_char("It can't be unlocked.\n\r", ch);
			return;
		}
		if (!has_key(ch, pexit->key)) {
			send_to_char("You lack the key.\n\r", ch);
			return;
		}
		if (!IS_SET((int)pexit->exit_info, EX_LOCKED)) {
			send_to_char("It's already unlocked.\n\r", ch);
			return;
		}

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		send_to_char("*Click*\n\r", ch);
		act("$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

		/* unlock the other side */
		if ((to_room = pexit->u1.to_room) != NULL
		    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		    && pexit_rev->u1.to_room == ch->in_room)
			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
	}

	return;
}


/***************************************************************************
*	do_pick
*
*	pick a lock
***************************************************************************/
void do_pick(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *gch;
	OBJ_DATA *obj;
	SKILL *skill;
	char arg[MIL];
	int door;
	int percent;

	if ((skill = skill_lookup("pick lock")) == NULL) {
		send_to_char("Huh?", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Pick what?\n\r", ch);
		return;
	}

	WAIT_STATE(ch, skill->wait);

	/* look for guards */
	for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
		if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level) {
			act("$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR);
			return;
		}
	}

	percent = get_learned_percent(ch, skill);
	if (!IS_NPC(ch) && number_percent() > percent) {
		send_to_char("You failed.\n\r", ch);
		check_improve(ch, skill, FALSE, 2);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->item_type == ITEM_PORTAL) {
			if (!IS_SET((int)obj->value[1], EX_ISDOOR)) {
				send_to_char("You can't do that.\n\r", ch);
				return;
			}

			if (!IS_SET((int)obj->value[1], EX_CLOSED)) {
				send_to_char("It's not closed.\n\r", ch);
				return;
			}

			if (obj->value[4] < 0) {
				send_to_char("It can't be unlocked.\n\r", ch);
				return;
			}

			if (IS_SET((int)obj->value[1], EX_PICKPROOF)) {
				send_to_char("You failed.\n\r", ch);
				return;
			}

			REMOVE_BIT(obj->value[1], EX_LOCKED);
			act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
			act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
			check_improve(ch, skill, TRUE, 2);
			return;
		}

		/* 'pick object' */
		if (obj->item_type != ITEM_CONTAINER) {
			send_to_char("That's not a container.\n\r", ch);
			return;
		}
		if (!IS_SET(obj->value[1], CONT_CLOSED)) {
			send_to_char("It's not closed.\n\r", ch);
			return;
		}
		if (obj->value[2] < 0) {
			send_to_char("It can't be unlocked.\n\r", ch);
			return;
		}
		if (!IS_SET(obj->value[1], CONT_LOCKED)) {
			send_to_char("It's already unlocked.\n\r", ch);
			return;
		}
		if (IS_SET(obj->value[1], CONT_PICKPROOF)) {
			send_to_char("You failed.\n\r", ch);
			return;
		}

		REMOVE_BIT(obj->value[1], CONT_LOCKED);
		act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
		act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
		check_improve(ch, skill, TRUE, 2);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'pick door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET((int)pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch)) {
			send_to_char("It's not closed.\n\r", ch);
			return;
		}
		if (pexit->key < 0 && !IS_IMMORTAL(ch)) {
			send_to_char("It can't be picked.\n\r", ch);
			return;
		}
		if (!IS_SET((int)pexit->exit_info, EX_LOCKED)) {
			send_to_char("It's already unlocked.\n\r", ch);
			return;
		}
		if (IS_SET((int)pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch)) {
			send_to_char("You failed.\n\r", ch);
			return;
		}

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		send_to_char("*Click*\n\r", ch);
		act("$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM);
		check_improve(ch, skill, TRUE, 2);

		/* pick the other side */
		if ((to_room = pexit->u1.to_room) != NULL
		    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		    && pexit_rev->u1.to_room == ch->in_room)
			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
	}
}



/***************************************************************************
*	do_sneak
***************************************************************************/
void do_sneak(CHAR_DATA *ch, char *argument)
{
	SKILL *skill;
	AFFECT_DATA af;

	if ((skill = gsp_sneak) == NULL) {
		send_to_char("Huh?", ch);
		return;
	}

	send_to_char("You attempt to move silently.\n\r", ch);
	if (IS_AFFECTED(ch, AFF_SNEAK))
		affect_strip(ch, skill);

	if (number_percent() < get_learned_percent(ch, skill)) {
		check_improve(ch, skill, TRUE, 3);
		af.where = TO_AFFECTS;
		af.type = skill->sn;
		af.skill = skill;
		af.level = ch->level;
		af.duration = ch->level;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_SNEAK;
		affect_to_char(ch, &af);
	} else {
		check_improve(ch, skill, FALSE, 3);
	}

	return;
}

/***************************************************************************
*	do_hide
***************************************************************************/
void do_hide(CHAR_DATA *ch, char *argument)
{
	SKILL *skill;

	if ((skill = gsp_hide) == NULL) {
		send_to_char("Huh?", ch);
		return;
	}


	if (ch->last_fight
	    && (current_time - ch->last_fight < 90)) {
		send_to_char("You can't hide, your opponent can hear your heartbeat!\n\r", ch);
		return;
	}

	send_to_char("You attempt to hide.\n\r", ch);
	if (IS_AFFECTED(ch, AFF_HIDE)) {
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
		affect_strip(ch, skill);
	}

	if (number_percent() < get_learned_percent(ch, skill)) {
		SET_BIT(ch->affected_by, AFF_HIDE);
		check_improve(ch, skill, TRUE, 3);
	} else {
		check_improve(ch, skill, FALSE, 3);
	}

	return;
}




/***************************************************************************
*	do_visible
*
*	strip all invisiblity-type affects
***************************************************************************/
void do_visible(CHAR_DATA *ch, char *argument)
{
	affect_strip(ch, gsp_invisibility);
	affect_strip(ch, gsp_mass_invisibility);
	affect_strip(ch, gsp_sneak);
	affect_strip(ch, gsp_darkness);
	REMOVE_BIT(ch->affected_by, AFF_HIDE);
	REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
	REMOVE_BIT(ch->affected_by, AFF_SNEAK);
	send_to_char("Ok.\n\r", ch);
	return;
}


/***************************************************************************
*	rest/sit/sleep/stand
***************************************************************************/
/***************************************************************************
*	do_sit
***************************************************************************/
void do_sit(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *on = NULL;

    if (argument[0] != '\0') {
        on = get_obj_list(ch, argument, ch->in_room->contents);
        if (on == NULL) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    } else {
        OBJ_DATA *obj = NULL;
        /* Try to pull a sittable object from inventory. */
        for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
            if (is_situpon(obj)
                && can_see_obj(ch, obj)
                && can_drop_obj(ch, obj)) {
                on = obj;
                break;
            }
        }
    }

    sit(ch, on);

	return;
}

/***************************************************************************
*	do_stand
***************************************************************************/
void do_stand(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj = NULL;

	if (argument[0] != '\0') {
		obj = get_obj_list(ch, argument, ch->in_room->contents);
		if (obj == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	} else {
        // implicitly stand on whatever already on.
        obj = ch->on;
    }

    stand(ch, obj);

	return;
}


/***************************************************************************
*	do_rest
***************************************************************************/
void do_rest(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj = NULL;
	OBJ_DATA *obj_on = NULL;

	if (ch->position == POS_FIGHTING) {
		send_to_char("You are already fighting!\n\r", ch);
		return;
	}

	if (argument[0] != '\0') {
		obj = get_obj_list(ch, argument, ch->in_room->contents);
		if (obj == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	} else {
		obj = ch->on;
	}

	if (argument[0] == '\0') {
		for (obj_on = ch->carrying; obj_on != NULL; obj_on = obj_on->next_content) {
			if ((obj_on->item_type == ITEM_FURNITURE)
			    && (IS_SET(obj_on->value[2], REST_ON)
				|| IS_SET(obj_on->value[2], REST_IN)
				|| IS_SET(obj_on->value[2], REST_AT))
			    && can_see_obj(ch, obj_on)
			    && can_drop_obj(ch, obj_on)) {
				obj_from_char(obj_on);
				obj_to_room(obj_on, ch->in_room);
				act("$n drops $p.", ch, obj_on, NULL, TO_ROOM);
				act("You drop $p.", ch, obj_on, NULL, TO_CHAR);
				obj = obj_on;
				break;
			}
		}
	}
	if (obj != NULL) {
		if (!IS_SET(obj->item_type, ITEM_FURNITURE)
		    || (!IS_SET(obj->value[2], REST_ON)
			&& !IS_SET(obj->value[2], REST_IN)
			&& !IS_SET(obj->value[2], REST_AT))) {
			send_to_char("You can't rest on that.\n\r", ch);
			return;
		}

		if (obj != NULL && ch->on != obj && (long)count_users(obj) >= obj->value[0]) {
			act_new("There's no more room on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD, FALSE);
			return;
		}

		ch->on = obj;
	}

	switch (ch->position) {
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_SLEEP)) {
			send_to_char("You can't wake up!\n\r", ch);
			return;
		}

		if (obj == NULL) {
			send_to_char("You wake up and start resting.\n\r", ch);
			act("$n wakes up and starts resting.", ch, NULL, NULL, TO_ROOM);
		} else if (IS_SET(obj->value[2], REST_AT)) {
			act_new("You wake up and rest at $p.", ch, obj, NULL, TO_CHAR, POS_SLEEPING, FALSE);
			act("$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM);
		} else if (IS_SET(obj->value[2], REST_ON)) {
			act_new("You wake up and rest on $p.", ch, obj, NULL, TO_CHAR, POS_SLEEPING, FALSE);
			act("$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM);
		} else {
			act_new("You wake up and rest in $p.", ch, obj, NULL, TO_CHAR, POS_SLEEPING, FALSE);
			act("$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_RESTING;
		break;

	case POS_RESTING:
		send_to_char("You are already resting.\n\r", ch);
		break;

	case POS_STANDING:
		if (obj == NULL) {
			send_to_char("You rest.\n\r", ch);
			act("$n sits down and rests.", ch, NULL, NULL, TO_ROOM);
		} else if (IS_SET(obj->value[2], REST_AT)) {
			act("You sit down at $p and rest.", ch, obj, NULL, TO_CHAR);
			act("$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM);
		} else if (IS_SET(obj->value[2], REST_ON)) {
			act("You sit on $p and rest.", ch, obj, NULL, TO_CHAR);
			act("$n sits on $p and rests.", ch, obj, NULL, TO_ROOM);
		} else {
			act("You rest in $p.", ch, obj, NULL, TO_CHAR);
			act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_RESTING;
		break;

	case POS_SITTING:
		if (obj == NULL) {
			send_to_char("You rest.\n\r", ch);
			act("$n rests.", ch, NULL, NULL, TO_ROOM);
		} else if (IS_SET(obj->value[2], REST_AT)) {
			act("You rest at $p.", ch, obj, NULL, TO_CHAR);
			act("$n rests at $p.", ch, obj, NULL, TO_ROOM);
		} else if (IS_SET(obj->value[2], REST_ON)) {
			act("You rest on $p.", ch, obj, NULL, TO_CHAR);
			act("$n rests on $p.", ch, obj, NULL, TO_ROOM);
		} else {
			act("You rest in $p.", ch, obj, NULL, TO_CHAR);
			act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_RESTING;
		break;
	}

	if (ch->dream != NULL && ch->position != POS_SLEEPING) {
		send_to_char("Your dreams are interrupted\n\r", ch);
		ch->dream = NULL;
	}

	return;
}


/***************************************************************************
*	do_sleep
***************************************************************************/
void do_sleep(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj = NULL;
	OBJ_DATA *obj_on = NULL;

	for (obj = ch->carrying; obj; obj = obj->next_content) {
		if ((obj->item_type == ITEM_FURNITURE)
		    && (IS_SET(obj->value[2], SLEEP_ON)
			|| IS_SET(obj->value[2], SLEEP_IN))
		    && can_see_obj(ch, obj)
		    && can_drop_obj(ch, obj)) {
			obj_from_char(obj);
			obj_to_room(obj, ch->in_room);
			act("$n drops $p.", ch, obj, NULL, TO_ROOM);
			act("You drop $p.", ch, obj, NULL, TO_CHAR);

			obj_on = obj;
			break;
		}
	}

	switch (ch->position) {
	case POS_SLEEPING:
		send_to_char("You are already sleeping.\n\r", ch);
		break;
	case POS_RESTING:
	case POS_SITTING:
	case POS_STANDING:
		if (argument[0] == '\0' && ch->on == NULL && obj_on == NULL) {
			send_to_char("You go to sleep.\n\r", ch);
			act("$n goes to sleep.", ch, NULL, NULL, TO_ROOM);
			ch->position = POS_SLEEPING;
		} else {
			if (argument[0] == '\0') {
				if (obj_on == NULL)
					obj = ch->on;
				else
					obj = obj_on;
			} else {
				obj = get_obj_list(ch, argument, ch->in_room->contents);
			}

			if (obj == NULL) {
				send_to_char("You don't see that here.\n\r", ch);
				return;
			}

			if (obj->item_type != ITEM_FURNITURE
			    || (!IS_SET(obj->value[2], SLEEP_ON)
				&& !IS_SET(obj->value[2], SLEEP_IN)
				&& !IS_SET(obj->value[2], SLEEP_AT))) {
				send_to_char("You can't sleep on that!\n\r", ch);
				return;
			}

			if (ch->on != obj && (long)count_users(obj) >= obj->value[0]) {
				act_new("There is no room on $p for you.", ch, obj, NULL, TO_CHAR, POS_DEAD, FALSE);
				return;
			}

			ch->on = obj;
			if (IS_SET(obj->value[2], SLEEP_AT)) {
				act_new("You go to sleep at $p.", ch, obj, NULL, TO_CHAR, POS_RESTING, TRUE);
				act_new("$n goes to sleep at $p.", ch, obj, NULL, TO_ROOM, POS_RESTING, TRUE);
			} else if (IS_SET(obj->value[2], SLEEP_ON)) {
				act_new("You go to sleep on $p.", ch, obj, NULL, TO_CHAR, POS_RESTING, TRUE);
				act_new("$n goes to sleep on $p.", ch, obj, NULL, TO_ROOM, POS_RESTING, TRUE);
			} else {
				act_new("You go to sleep in $p.", ch, obj, NULL, TO_CHAR, POS_RESTING, TRUE);
				act_new("$n goes to sleep in $p.", ch, obj, NULL, TO_ROOM, POS_RESTING, TRUE);
			}
			ch->position = POS_SLEEPING;
		}
		break;

	case POS_FIGHTING:
		send_to_char("You are already fighting!\n\r", ch);
		break;
	}

	return;
}


/***************************************************************************
*	do_wake
***************************************************************************/
void do_wake(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		do_stand(ch, argument);
		return;
	}

	if (!IS_AWAKE(ch)) {
		send_to_char("You are asleep yourself!\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_AWAKE(victim)) {
		act("$N is already awake.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(victim, AFF_SLEEP)) {
		act("You can't wake $M!", ch, NULL, victim, TO_CHAR);
		return;
	}

	act_new("$n wakes you.", ch, NULL, victim, TO_VICT, POS_SLEEPING, FALSE);
	do_stand(victim, "");
	return;
}
