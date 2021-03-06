/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander(markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include "merc.h"
#include "object.h"
#include "mob_cmds.h"
#include "magic.h"


extern DECLARE_DO_FUN(do_look);
extern struct roomtemplate *find_location(struct char_data *, const char *);
void mob_interpret(struct char_data * ch, const char *argument);

/*
 * Command functions.
 */
static DECLARE_DO_FUN(do_mpasound);
static DECLARE_DO_FUN(do_mpgecho);
static DECLARE_DO_FUN(do_mpzecho);
static DECLARE_DO_FUN(do_mpkill);
static DECLARE_DO_FUN(do_mpassist);
static DECLARE_DO_FUN(do_mpjunk);
static DECLARE_DO_FUN(do_mpechoaround);
static DECLARE_DO_FUN(do_mpecho);
static DECLARE_DO_FUN(do_mpechoat);
static DECLARE_DO_FUN(do_mpmload);
static DECLARE_DO_FUN(do_mpoload);
static DECLARE_DO_FUN(do_mppurge);
static DECLARE_DO_FUN(do_mpgoto);
static DECLARE_DO_FUN(do_mpat);
static DECLARE_DO_FUN(do_mptransfer);
static DECLARE_DO_FUN(do_mpgtransfer);
static DECLARE_DO_FUN(do_mpforce);
static DECLARE_DO_FUN(do_mpgforce);
static DECLARE_DO_FUN(do_mpvforce);
static DECLARE_DO_FUN(do_mpcast);
static DECLARE_DO_FUN(do_mpdamage);
static DECLARE_DO_FUN(do_mpremember);
static DECLARE_DO_FUN(do_mpforget);
static DECLARE_DO_FUN(do_mpdelay);
static DECLARE_DO_FUN(do_mpcancel);
static DECLARE_DO_FUN(do_mpcall);
static DECLARE_DO_FUN(do_mpflee);
static DECLARE_DO_FUN(do_mpotransfer);
static DECLARE_DO_FUN(do_mpremove);



/*
 * Command table.
 */
const struct  mob_cmd_type mob_cmd_table   [] =
{
    { "asound",	do_mpasound	},
    { "gecho",	do_mpgecho	},
    { "zecho",	do_mpzecho	},
    { "kill",	do_mpkill	},
    { "assist",	do_mpassist	},
    { "junk",	do_mpjunk	},
    { "echo",	do_mpecho	},
    { "echoaround", do_mpechoaround },
    { "echoat",	do_mpechoat	},
    { "mload",	do_mpmload	},
    { "oload",	do_mpoload	},
    { "purge",	do_mppurge	},
    { "goto",	do_mpgoto	},
    { "at",		do_mpat		},
    { "transfer",	do_mptransfer	},
    { "gtransfer",	do_mpgtransfer	},
    { "otransfer",	do_mpotransfer	},
    { "force",	do_mpforce	},
    { "gforce",	do_mpgforce	},
    { "vforce",	do_mpvforce	},
    { "cast",	do_mpcast	},
    { "damage",	do_mpdamage	},
    { "remember",	do_mpremember	},
    { "forget",	do_mpforget	},
    { "delay",	do_mpdelay	},
    { "cancel",	do_mpcancel	},
    { "call",	do_mpcall	},
    { "flee",	do_mpflee	},
    { "remove",	do_mpremove	},
    { "",		0		}
};

void do_mob(struct char_data *ch, const char *argument)
{
    if (ch->desc != NULL && get_trust(ch) < MAX_LEVEL)
	return;

    mob_interpret(ch, argument);
}


/*
 * Mob command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void mob_interpret(struct char_data *ch, const char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmd;

    argument = one_argument(argument, command);

    /*
     * Look for command in command table.
     */
    for (cmd = 0; mob_cmd_table[cmd].name[0] != '\0'; cmd++) {
	if (command[0] == mob_cmd_table[cmd].name[0] && !str_prefix(command, mob_cmd_table[cmd].name)) {
	    (*mob_cmd_table[cmd].do_fun)(ch, argument);
	    tail_chain();
	    return;
	}
    }

    log_bug("mob_interpret: invalid cmd from mob %d: '%s'", IS_NPC(ch) ? ch->mob_idx->vnum : 0, command);
}

char *mprog_type_to_name(int type)
{
    switch (type) {
	case TRIG_ACT:
	    return "ACT";
	case TRIG_SPEECH:
	    return "SPEECH";
	case TRIG_RANDOM:
	    return "RANDOM";
	case TRIG_FIGHT:
	    return "FIGHT";
	case TRIG_HPCNT:
	    return "HPCNT";
	case TRIG_DEATH:
	    return "DEATH";
	case TRIG_ENTRY:
	    return "ENTRY";
	case TRIG_GREET:
	    return "GREET";
	case TRIG_GRALL:
	    return "GRALL";
	case TRIG_GIVE:
	    return "GIVE";
	case TRIG_BRIBE:
	    return "BRIBE";
	case TRIG_KILL:
	    return "KILL";
	case TRIG_DELAY:
	    return "DELAY";
	case TRIG_SURR:
	    return "SURRENDER";
	case TRIG_EXIT:
	    return "EXIT";
	case TRIG_EXALL:
	    return "EXALL";
	default:
	    return "ERROR";
    }
}

/*
 * Displays MOBprogram triggers of a mobile
 *
 * Syntax: mpstat [name]
 */
void do_mpstat(struct char_data *ch, const char *argument)
{
    struct mprog_list *mprg;
    struct char_data *victim;
    int idx;

    if (argument[0] == '\0') {
	send_to_char("Mpstat whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL) {
	send_to_char("No such creature.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim)) {
	send_to_char("That is not a mobile.\n\r", ch);
	return;
    }


    printf_to_char(ch, "Mobile #%-6d [%s]\n\r", victim->mob_idx->vnum, victim->short_descr);

    printf_to_char(ch, "Delay   %-6d [%s]\n\r",
	    victim->mprog_delay,
	    victim->mprog_target == NULL ? "No target" : victim->mprog_target->name);

    if (!victim->mob_idx->mprog_flags) {
	send_to_char("[No programs set]\n\r", ch);
	return;
    }

    for (idx = 0, mprg = victim->mob_idx->mprogs; mprg != NULL; mprg = mprg->next) {
	printf_to_char(ch, "[%2d] Trigger [%-8s] Program [%4d] Phrase [%s]\n\r",
		++idx,
		mprog_type_to_name(mprg->trig_type),
		mprg->vnum,
		mprg->trig_phrase);
    }

    return;
}

/*
 * Displays the source code of a given MOBprogram
 *
 * Syntax: mpdump [vnum]
 */
void do_mpdump(struct char_data *ch, const char *argument)
{
    struct mprog_code *mprg;
    char buf[MAX_INPUT_LENGTH];

    one_argument(argument, buf);

    if ((mprg = get_mprog_index(parse_long(buf))) == NULL) {
	send_to_char("No such MOBprogram.\n\r", ch);
	return;
    }

    page_to_char(mprg->code, ch);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: mob gecho [string]
 */
void do_mpgecho(struct char_data *ch, const char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    struct descriptor_data *d;
    struct descriptor_data *dpending;

    if (argument[0] == '\0') {
	log_bug("MpGEcho: missing argument from vnum %d", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	printf_to_char(d->character, "%s%s\n\r", IS_IMMORTAL(d->character) ? "Mob echo>" : "", argument);
    }
}

/*
 * Prints the argument to all players in the same area as the mob
 *
 * Syntax: mob zecho [string]
 */
void do_mpzecho(struct char_data *ch, const char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    struct descriptor_data *d;
    struct descriptor_data *dpending;

    if (argument[0] == '\0') {
	log_bug("MpZEcho: missing argument from vnum %d", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (ch->in_room == NULL)
	return;

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	if (d->character->in_room != NULL && d->character->in_room->area == ch->in_room->area) {
	    printf_to_char(d->character, "%s%s\n\r", IS_IMMORTAL(d->character) ? "Mob echo>" : "", argument);
	}
    }
}

/*
 * Prints the argument to all the rooms aroud the mobile
 *
 * Syntax: mob asound [string]
 */
void do_mpasound(struct char_data *ch, const char *argument)
{
    struct roomtemplate *was_in_room;
    int door;

    if (argument[0] == '\0')
	return;

    was_in_room = ch->in_room;
    for (door = 0; door < 6; door++) {
	struct exit_data *pexit;

	if ((pexit = was_in_room->exit[door]) != NULL && pexit->to_room != NULL && pexit->to_room != was_in_room) {
	    ch->in_room = pexit->to_room;
	    act_new(argument, ch, NULL, NULL, TO_ROOM, POS_RESTING, false);
	}
    }

    ch->in_room = was_in_room;
    return;
}

/*
 * Prints the message to everyone in the room other than the mob and victim
 *
 * Syntax: mob echoaround [victim] [string]
 */
void do_mpechoaround(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
	return;

    if ((victim = get_char_room(ch, arg)) == NULL)
	return;

    act(argument, ch, NULL, victim, TO_NOTVICT);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: mob echoat [victim] [string]
 */
void do_mpechoat(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);
    if (arg[0] == '\0' || argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, arg)) == NULL)
	return;

    act(argument, ch, NULL, victim, TO_VICT);
}

/*
 * Prints the message to the room at large
 *
 * Syntax: mob echo [string]
 */
void do_mpecho(struct char_data *ch, const char *argument)
{
    if (argument[0] == '\0')
	return;

    act(argument, ch, NULL, NULL, TO_ROOM);
}


/*
 * Lets the mobile kill any player or mobile without murder
 *
 * Syntax: mob kill [victim]
 */
void do_mpkill(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if ((victim = get_char_room(ch, arg)) == NULL)
	return;

    if (victim == ch || IS_NPC(victim) || ch->position == POS_FIGHTING)
	return;

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
	log_bug("MpKill - Charmed mob attacking master from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    multi_hit(ch, victim, TYPE_UNDEFINED);
    return;
}

/*
 * Lets the mobile assist another mob or player
 *
 * Syntax: mob assist [character]
 */
void do_mpassist(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0')
	return;

    if ((victim = get_char_room(ch, arg)) == NULL)
	return;

    if (victim == ch || ch->fighting != NULL || victim->fighting == NULL)
	return;

    multi_hit(ch, victim->fighting, TYPE_UNDEFINED);
    return;
}


/*
 * Lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 *
 * Syntax: mob junk [item]
 */
void do_mpjunk(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    struct gameobject *obj;
    struct gameobject *obj_next;

    one_argument(argument, arg);

    if (arg[0] == '\0')
	return;

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
	if ((obj = get_obj_wear(ch, arg)) != NULL) {
	    unequip_char(ch, obj);
	    extract_obj(obj);
	    return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL)
	    return;
	extract_obj(obj);
    } else {
	for (obj = ch->carrying; obj != NULL; obj = obj_next) {
	    obj_next = obj->next_content;
	    if (arg[3] == '\0' || is_name(&arg[4], object_name_get(obj))) {
		if (obj->wear_loc != WEAR_NONE)
		    unequip_char(ch, obj);
		extract_obj(obj);
	    }
	}
    }

    return;
}


/*
 * Lets the mobile load another mobile.
 *
 * Syntax: mob mload [vnum]
 */
void do_mpmload(struct char_data *ch, const char *argument)
{
    struct mob_index_data *pMobIndex;
    struct char_data *victim;
    char arg[MAX_INPUT_LENGTH];
    int vnum;

    one_argument(argument, arg);

    if (ch->in_room == NULL || arg[0] == '\0' || !is_number(arg))
	return;

    vnum = parse_long(arg);
    if ((pMobIndex = get_mob_index(vnum)) == NULL) {
	log_bug("Mpmload: bad mob index(%d) from mob %d", vnum, IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    victim = create_mobile(pMobIndex);
    char_to_room(victim, ch->in_room);
    return;
}

/*
 * Lets the mobile load an object
 *
 * Syntax: mob oload [vnum] [level] {R}
 */
void do_mpoload(struct char_data *ch, const char *argument)
{
    struct objecttemplate *objtemplate;
    struct gameobject *obj;
    char arg[MAX_INPUT_LENGTH];
    long vnum;
    bool to_room;
    bool to_wear;

    argument = one_argument(argument, arg);

    if (!str_prefix(arg, "random")) {
        char mm_buf[MAX_INPUT_LENGTH];
        int min_vnum;
        int max_vnum;

        argument = one_argument(argument, mm_buf);
        min_vnum = 0;
        max_vnum = 0;
        if (is_number(mm_buf))
            min_vnum = parse_long(mm_buf);

        if (min_vnum <= 0) {
            log_bug("mpoload - bad minimum number for random range %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
            return;
        }

        argument = one_argument(argument, mm_buf);
        if (is_number(mm_buf))
            max_vnum = parse_long(mm_buf);
        max_vnum = UMAX(min_vnum, max_vnum);

        vnum = number_range(min_vnum, max_vnum);
    } else {
        if (arg[0] == '\0' || !is_number(arg)) {
            log_bug("mpoload - bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
            return;
        }

        vnum = parse_long(arg);
    }

    argument = one_argument(argument, arg);
    /*
     * 2nd argument
     * 'R'     - load to room
     * 'W'     - load to mobile and force wear
     */
    to_room = false;
    to_wear = false;
    if (arg[0] == 'R' || arg[0] == 'r')
        to_room = true;
    else if (arg[0] == 'W' || arg[0] == 'w')
        to_wear = true;

    if ((objtemplate = objecttemplate_getbyvnum(vnum)) == NULL) {
        log_bug("mpoload - bad vnum arg from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
        return;
    }

    obj = create_object(objtemplate);
    if ((to_wear || !to_room) && CAN_WEAR(obj, ITEM_TAKE)) {
        obj_to_char(obj, ch);
        if (to_wear)
            wear_obj(ch, obj, true);
    } else {
        obj_to_room(obj, ch->in_room);
    }

    return;
}

/*
 * Lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room. The mobile cannot
 * purge itself for safety reasons.
 *
 * syntax mob purge {target}
 */
void do_mppurge(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    struct gameobject *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
	/* 'purge' */
	struct char_data *vnext;
	struct gameobject *obj_next;

	for (victim = ch->in_room->people; victim != NULL; victim = vnext) {
	    vnext = victim->next_in_room;
	    if (IS_NPC(victim) && victim != ch
		    && !IS_SET(victim->act, ACT_NOPURGE))
		extract_char(victim, true);
	}

	for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
	    obj_next = obj->next_content;
	    if (!IS_SET(obj->extra_flags, ITEM_NOPURGE))
		extract_obj(obj);
	}

	return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL) {
	if ((obj = get_obj_here(ch, arg)))
	    extract_obj(obj);
	else
	    log_bug("Mppurge - Bad argument from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (!IS_NPC(victim)) {
	log_bug("Mppurge - Purging a PC from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    extract_char(victim, true);
    return;
}


/*
 * Lets the mobile goto any location it wishes that is not private.
 *
 * Syntax: mob goto [location]
 */
void do_mpgoto(struct char_data *ch, const char *argument)
{
    struct roomtemplate *location;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0') {
	log_bug("Mpgoto - No argument from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (!str_cmp(arg, "random")) {
	if ((location = get_random_room(ch, NULL)) == NULL)
	    return;
    } else if ((location = find_location(ch, arg)) == NULL) {
	log_bug("Mpgoto - No such location from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (ch->fighting != NULL)
	stop_fighting(ch, true);

    char_from_room(ch);
    char_to_room(ch, location);

    return;
}

/*
 * Lets the mobile do a command at another location.
 *
 * Syntax: mob at [location] [commands]
 */
void do_mpat(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    struct roomtemplate *location;
    struct roomtemplate *original;
    struct char_data *wch;
    struct gameobject *on;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
	log_bug("Mpat - Bad argument from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if ((location = find_location(ch, arg)) == NULL) {
	log_bug("Mpat - No such location from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    original = ch->in_room;
    on = ch->on;
    char_from_room(ch);
    char_to_room(ch, location);
    interpret(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (wch = char_list; wch != NULL; wch = wch->next) {
	if (wch == ch) {
	    char_from_room(ch);
	    char_to_room(ch, original);
	    ch->on = on;
	    break;
	}
    }

    return;
}

/*
 * Lets the mobile transfer people.  The 'all' argument transfers
 *  everyone in the current room to the specified location
 *
 * Syntax: mob transfer [target|'all'] [location]
 */
void do_mptransfer(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct roomtemplate *location;
    struct char_data *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
	log_bug("Mptransfer - Bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (!str_cmp(arg1, "all")) {
	struct char_data *victim_next;

	for (victim = ch->in_room->people; victim != NULL; victim = victim_next) {
	    victim_next = victim->next_in_room;
	    if (!IS_NPC(victim)) {
		sprintf(buf, "%s %s", victim->name, arg2);
		do_mptransfer(ch, buf);
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0') {
	location = ch->in_room;
    } else {
	if ((location = find_location(ch, arg2)) == NULL) {
	    log_bug("Mptransfer - No such location from vnum %ld.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	    return;
	}

	if (room_is_private(location))
	    return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
	return;

    if (victim->in_room == NULL)
	return;

    if (victim->fighting != NULL)
	stop_fighting(victim, true);
    char_from_room(victim);
    char_to_room(victim, location);
    do_look(victim, "auto");

    return;
}

/*
 * Lets the mobile transfer all chars in same group as the victim.
 *
 * Syntax: mob gtransfer [victim] [location]
 */
void do_mpgtransfer(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct char_data *who, *victim, *victim_next;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
	log_bug("Mpgtransfer - Bad syntax from vnum %l.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if ((who = get_char_room(ch, arg1)) == NULL)
	return;

    for (victim = ch->in_room->people; victim; victim = victim_next) {
	victim_next = victim->next_in_room;
	if (is_same_group(who, victim)) {
	    sprintf(buf, "%s %s", victim->name, arg2);
	    do_mptransfer(ch, buf);
	}
    }
    return;
}

/*
 * Lets the mobile force someone to do something. Must be mortal level
 * and the all argument only affects those in the room with the mobile.
 *
 * Syntax: mob force [victim] [commands]
 */
void do_mpforce(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
	log_bug("Mpforce - Bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (!str_cmp(arg, "all")) {
	struct char_data *vch;
	struct char_data *vch_next;

	for (vch = char_list; vch != NULL; vch = vch_next) {
	    vch_next = vch->next;

	    if (vch->in_room == ch->in_room
		    && get_trust(vch) < get_trust(ch)
		    && can_see(ch, vch))
		interpret(vch, argument);
	}
    } else {
	struct char_data *victim;

	if ((victim = get_char_room(ch, arg)) == NULL)
	    return;

	if (victim == ch)
	    return;

	interpret(victim, argument);
    }

    return;
}

/*
 * Lets the mobile force a group something. Must be mortal level.
 *
 * Syntax: mob gforce [victim] [commands]
 */
void do_mpgforce(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim, *vch, *vch_next;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
	log_bug("MpGforce - Bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
	return;

    if (victim == ch)
	return;

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
	vch_next = vch->next_in_room;

	if (is_same_group(victim, vch))
	    interpret(vch, argument);
    }
    return;
}

/*
 * Forces all mobiles of certain vnum to do something(except ch)
 *
 * Syntax: mob vforce [vnum] [commands]
 */
void do_mpvforce(struct char_data *ch, const char *argument)
{
    struct char_data *victim, *victim_next;
    char arg[MAX_INPUT_LENGTH];
    long vnum;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
	log_bug("MpVforce - Bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if (!is_number(arg)) {
	log_bug("MpVforce - Non-number argument vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    vnum = parse_long(arg);

    for (victim = char_list; victim; victim = victim_next) {
	victim_next = victim->next;
	if (IS_NPC(victim) && victim->mob_idx->vnum == vnum
		&& ch != victim && victim->fighting == NULL)
	    interpret(victim, argument);
    }
    return;
}


/*
 * Lets the mobile cast spells --
 * Beware: this does only crude checking on the target validity
 * and does not account for mana etc., so you should do all the
 * necessary checking in your mob program before issuing this cmd!
 *
 * Syntax: mob cast [spell] {target}
 */
void do_mpcast(struct char_data *ch, const char *argument)
{
    struct char_data *vch;
    struct gameobject *obj;
    struct dynamic_skill *skill;
    void *victim = NULL;
    char spell[MAX_INPUT_LENGTH];
    char target[MAX_INPUT_LENGTH];

    argument = one_argument(argument, spell);
    one_argument(argument, target);

    if (spell[0] == '\0') {
	log_bug("MpCast - Bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    if ((skill = skill_lookup(spell)) == NULL) {
	log_bug("MpCast - No such spell from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    vch = get_char_room(ch, target);
    obj = get_obj_here(ch, target);

    switch (skill->target) {
	default:
	    return;
	case TAR_IGNORE:
	    break;
	case TAR_CHAR_OFFENSIVE:
	    if (vch == NULL || vch == ch)
		return;

	    victim = (void *)vch;
	    break;
	case TAR_CHAR_DEFENSIVE:
	    victim = (vch == NULL) ? (void *)ch : (void *)vch;
	    break;
	case TAR_CHAR_SELF:
	    victim = (void *)ch;
	    break;
	case TAR_OBJ_CHAR_DEF:
	case TAR_OBJ_CHAR_OFF:
	case TAR_OBJ_INV:
	    if (obj == NULL)
		return;
	    victim = (void *)obj;
    }

    cast_spell(ch, skill, ch->level, victim, TARGET_CHAR, argument);
    return;
}

/*
 * Lets mob cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: mob damage [victim] [min] [max] {kill}
 */
void do_mpdamage(struct char_data *ch, const char *argument)
{
    struct char_data *victim = NULL, *victim_next;
    char target[MAX_INPUT_LENGTH],
	 min[MAX_INPUT_LENGTH],
	 max[MAX_INPUT_LENGTH];
    int low, high;
    bool fAll = false, fKill = false;

    argument = one_argument(argument, target);
    argument = one_argument(argument, min);
    argument = one_argument(argument, max);

    if (target[0] == '\0') {
	log_bug("MpDamage - Bad syntax from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    if (!str_cmp(target, "all"))
	fAll = true;
    else if ((victim = get_char_room(ch, target)) == NULL)
	return;

    if (is_number(min)) {
	low = parse_int(min);
    } else {
	log_bug("MpDamage - Bad damage min vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    if (is_number(max)) {
	high = parse_int(max);
    } else {
	log_bug("MpDamage - Bad damage max vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    one_argument(argument, target);

    /*
     * If kill parameter is omitted, this command is "safe" and will not
     * kill the victim.
     */

    if (target[0] != '\0')
	fKill = true;
    if (fAll) {
	for (victim = ch->in_room->people; victim; victim = victim_next) {
	    victim_next = victim->next_in_room;
	    if (victim != ch) {
		damage(victim, victim,
			fKill ?
			number_range(low, high) : UMIN(victim->hit, number_range(low, high)),
			TYPE_UNDEFINED, DAM_NONE, false);
	    }
	}
    } else {
	damage(victim, victim,
		fKill ?
		number_range(low, high) : UMIN(victim->hit, number_range(low, high)),
		TYPE_UNDEFINED, DAM_NONE, false);
    }
    return;
}

/*
 * Lets the mobile to remember a target. The target can be referred to
 * with $q and $Q codes in MOBprograms. See also "mob forget".
 *
 * Syntax: mob remember [victim]
 */
void do_mpremember(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] != '\0')
	ch->mprog_target = get_char_world(ch, arg);
    else
	log_bug("MpRemember: missing argument from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
}

/*
 * Reverse of "mob remember".
 *
 * Syntax: mob forget
 */
void do_mpforget(struct char_data *ch, const char *argument)
{
    ch->mprog_target = NULL;
}

/*
 * Sets a delay for MOBprogram execution. When the delay time expires,
 * the mobile is checked for a MObprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_MOBILE
 *
 * Syntax: mob delay [pulses]
 */
void do_mpdelay(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (!is_number(arg)) {
	log_bug("MpDelay: invalid arg from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    ch->mprog_delay = parse_int(arg);
}

/*
 * Reverse of "mob delay", deactivates the timer.
 *
 * Syntax: mob cancel
 */
void do_mpcancel(struct char_data *ch, const char *argument)
{
    ch->mprog_delay = -1;
}
/*
 * Lets the mobile to call another MOBprogram withing a MOBprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the same room with the
 * mobile.
 *
 * Syntax: mob call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
void do_mpcall(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vch;
    struct gameobject *obj1, *obj2;
    struct mprog_code *prg;
    extern void program_flow(long, char *, struct char_data *, struct char_data *, const void *, const void *);

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
	log_bug("MpCall: missing arguments from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    if ((prg = get_mprog_index(parse_long(arg))) == NULL) {
	log_bug("MpCall: invalid prog from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    vch = NULL;
    obj1 = obj2 = NULL;
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	vch = get_char_room(ch, arg);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj1 = get_obj_here(ch, arg);
    argument = one_argument(argument, arg);
    if (arg[0] != '\0')
	obj2 = get_obj_here(ch, arg);
    program_flow(prg->vnum, prg->code, ch, vch, (void *)obj1, (void *)obj2);
}

/*
 * Forces the mobile to flee.
 *
 * Syntax: mob flee
 *
 */
void do_mpflee(struct char_data *ch, const char *argument)
{
    struct roomtemplate *was_in;
    struct exit_data *pexit;
    int door, attempt, dir;

    if (ch->fighting == NULL)
	return;

    if ((was_in = ch->in_room) == NULL)
	return;

    attempt = 0;
    door = -1;
    for (dir = 0; dir < 6; dir++) {
	if ((pexit = was_in->exit[dir]) == 0
		|| pexit->to_room == NULL
		|| IS_SET(pexit->exit_info, EX_CLOSED)
		|| (IS_NPC(ch)
		    && IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB))) {
	    continue;
	} else {
	    if (number_range(0, attempt) == 0)
		door = dir;
	    attempt++;
	}
    }

    if (door >= 0) {
	stop_fighting(ch, true);
	act("$n has fled!", ch, NULL, NULL, TO_ROOM);
	move_char(ch, door, false);
	if (ch->in_room != was_in)
	    return;
    }
}

/*
 * Lets the mobile to transfer an object. The object must be in the same
 * room with the mobile.
 *
 * Syntax: mob otransfer [item name] [location]
 */
void do_mpotransfer(struct char_data *ch, const char *argument)
{
    struct gameobject *obj;
    struct roomtemplate *location;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
	log_bug("MpOTransfer - Missing argument from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }

    one_argument(argument, buf);
    if ((location = find_location(ch, buf)) == NULL) {
	log_bug("MpOTransfer - No such location from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    }
    if ((obj = get_obj_here(ch, arg)) == NULL)
	return;

    if (obj->carried_by == NULL) {
	obj_from_room(obj);
    } else {
	if (obj->wear_loc != WEAR_NONE)
	    unequip_char(ch, obj);
	obj_from_char(obj);
    }
    obj_to_room(obj, location);
}

/*
 * Lets the mobile to strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: mob remove [victim] [object vnum|'all']
 */
void do_mpremove(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj, *obj_next;
    long vnum = 0;
    bool fAll = false;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);
    if ((victim = get_char_room(ch, arg)) == NULL)
	return;

    one_argument(argument, arg);
    if (!str_cmp(arg, "all")) {
	fAll = true;
    } else if (!is_number(arg)) {
	log_bug("MpRemove: Invalid object from vnum %d.", IS_NPC(ch) ? ch->mob_idx->vnum : 0);
	return;
    } else {
	vnum = parse_long(arg);
    }

    for (obj = victim->carrying; obj; obj = obj_next) {
	obj_next = obj->next_content;
	if (fAll || obj->objtemplate->vnum == vnum) {
	    unequip_char(ch, obj);
	    obj_from_char(obj);
	    extract_obj(obj);
	}
    }
}
