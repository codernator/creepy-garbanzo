#include "merc.h"
#include "character.h"
#include "channels.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "libstring.h"

#include <stdio.h>
#include <string.h>



void do_channels(CHAR_DATA *ch, /*@unused@*/char *argument)
{
    DENY_NPC(ch);
    show_channels(ch);
}

void do_deaf(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    DENY_NPC(ch);
    toggle_comm(ch, COMM_DEAF);
}

void do_quiet(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    DENY_NPC(ch);
    toggle_comm(ch, COMM_QUIET);
}

void do_afk(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch);
    toggle_afk(ch, argument);
}

void do_replay(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    DENY_NPC(ch);
    replay(ch);
}

void do_auctalk(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch);
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_NOAUCTION);
	} else {
        broadcast_auctalk(ch, argument);
	}
}

void do_noauction(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch);
    toggle_comm(ch, COMM_NOAUCTION);
}

void do_immtalk(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_NOWIZ);
	} else {
        broadcast_immtalk(ch, argument);
    }
}

void do_imptalk(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		toggle_comm(ch, COMM2_IMPTALK);
	} else {
        broadcast_imptalk(ch, argument);
    }
}

void do_say(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("```&Say`` what?\n\r", ch);
	} else {
        broadcast_say(ch, argument);
    }
}

void do_shout(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_SHOUTSOFF);
	} else {
        broadcast_shout(ch, argument);
    }
}

void do_info(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch);
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM2_INFO);
	} else {
        if (!IS_IMMORTAL(ch)) {
            send_to_char("Use '`#info``' to turn the info channel on or off.", ch);
        } else {
            broadcast_info(ch, argument);
        }
    }
}

void do_tell(CHAR_DATA *ch, char *argument)
{
	static char arg[MIL];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("```@Tell`` whom what?\n\r", ch);
	} else {
        /* Can tell to PC's anywhere, but NPC's only in same room.
         *    -- Furey
         */
        if ((victim = get_char_world(ch, arg)) == NULL
               || (IS_NPC(victim) && victim->in_room != ch->in_room)) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        broadcast_tell(ch, victim, argument);
    }
}

void do_reply(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("```@Reply`` what?\n\r", ch);
    } else {
        broadcast_reply(ch, argument);
    }
}

void do_yell(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("```1Yell`` what?\n\r", ch);
	} else {
        broadcast_yell(ch, argument);
    }
}

void do_emote(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("Emote what?\n\r", ch);
	} else {
        broadcast_emote(ch, argument);
    }
}

void do_pmote(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("Emote what?\n\r", ch);
	} else {
        broadcast_pmote(ch, argument);
    }
}

void do_order(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	CHAR_DATA *och;
	CHAR_DATA *och_next;
	char buf[MSL];
	char arg[MIL];
	char arg2[MIL];
	bool found;
	bool fAll;

	argument = one_argument(argument, arg);
	(void)one_argument(argument, arg2);

	if (!str_cmp(arg2, "suicide")) {
		send_to_char("That will not be done..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "bash")) {
		send_to_char("Sorry, no way.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "mob")) {
		send_to_char("That will NOT be done.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "quit")) {
		send_to_char("That will NOT be done.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "brew")) {
		send_to_char("Sorry pal, ain't gonna happen.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "scribe")) {
		send_to_char("Mobiles are not literate enough.\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "cast")) {
		send_to_char("Survey says:  No!\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "push")) {
		send_to_char("Survey says:  No!\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "pk")) {
		send_to_char("Huh? .. Kill what?\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "eat")) {
		send_to_char("Your friend is not hungry..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "drink")) {
		send_to_char("Your friend is not thirsty..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "enter")) {
		send_to_char("I'm sorry Dave, but I'm afraid I can't do that.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "drop")) {
		send_to_char("No.\n\r", ch);
		return;
	}


	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("Order whom to do what?\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		send_to_char("You feel like taking, not giving, orders.\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		fAll = TRUE;
		victim = NULL;
	} else {
		fAll = FALSE;
		if ((victim = get_char_room(ch, arg)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		if (victim == ch) {
			send_to_char("Aye aye, right away!\n\r", ch);
			return;
		}

		if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
		    || (IS_IMMORTAL(victim) && victim->trust >= ch->trust)) {
			send_to_char("Do it yourself!\n\r", ch);
			return;
		}
	}

	found = FALSE;
	for (och = ch->in_room->people; och != NULL; och = och_next) {
		och_next = och->next_in_room;

		if (IS_AFFECTED(och, AFF_CHARM)
		    && och->master == ch
		    && (fAll || och == victim)) {
			found = TRUE;

			if (!str_infix(arg2, "rem")) {
				send_to_char("One at a time...\n\r", ch);
				return;
			}

			(void)snprintf(buf, 2 * MIL, "$n orders you to '%s'.", argument);
			act(buf, ch, NULL, och, TO_VICT);
			interpret(och, argument);
		}
	}

	if (found) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		send_to_char("Ok.\n\r", ch);
	} else {
		send_to_char("You have no followers here.\n\r", ch);
	}
}

void do_gtell(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	CHAR_DATA *gch;

	if (argument[0] == '\0') {
		send_to_char("Tell your group what?\n\r", ch);
		return;
	}

	if (IS_SET(ch->comm, COMM_NOTELL)) {
		send_to_char("Your message didn't get through!\n\r", ch);
		return;
	}

    /*
     * Note use of send_to_char, so gtell works on sleepers.
     */
	(void)snprintf(buf, 2 * MIL, "```5%s tells the group '```P%s```5'``\n\r", ch->name,
		       argument);
	for (gch = char_list; gch != NULL; gch = gch->next)
		if (is_same_group(gch, ch))
			send_to_char(buf, gch);

	return;
}

void do_ignor(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	send_to_char("You must enter the full command to ignore someone.\n\r", ch);
}

void do_ignore(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;
	char arg[MIL], buf[MSL];
	DESCRIPTOR_DATA *d;
	int pos;
	bool found = FALSE;

	if (ch->desc == NULL)
		rch = ch;
	else
		rch = ch->desc->original ? ch->desc->original : ch;

	if (IS_NPC(rch))
		return;

	smash_tilde(argument);

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		if (rch->pcdata->ignore[0] == NULL) {
			send_to_char("You are not ignoring anyone.\n\r", ch);
			return;
		}
		send_to_char("You are currently ignoring:\n\r", ch);

		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (rch->pcdata->ignore[pos] == NULL)
				break;

			(void)snprintf(buf, 2 * MIL, "    %s\n\r", rch->pcdata->ignore[pos]);
			send_to_char(buf, ch);
		}
		return;
	}

	for (pos = 0; pos < MAX_IGNORE; pos++) {
		if (rch->pcdata->ignore[pos] == NULL)
			break;

		if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
			send_to_char("You are already ignoring that person.\n\r", ch);
			return;
		}
	}

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch, wch))
			continue;

		if (!str_cmp(arg, wch->name)) {
			found = TRUE;
			if (wch == ch) {
				send_to_char("You try to ignore yourself .. and fail.\n\r", ch);
				return;
			}
			if (wch->level >= LEVEL_IMMORTAL) {
				send_to_char("That person is very hard to ignore.\n\r", ch);
				return;
			}
		}
	}

	if (!found) {
		send_to_char("No one by that name is playing.\n\r", ch);
		return;
	}

	for (pos = 0; pos < MAX_IGNORE; pos++)
		if (rch->pcdata->ignore[pos] == NULL)
			break;

	if (pos >= MAX_IGNORE) {
		send_to_char("Stop being so antisocial!\n\r", ch);
		return;
	}

	rch->pcdata->ignore[pos] = str_dup(arg);
	(void)snprintf(buf, 2 * MIL, "You are now deaf to %s.\n\r", arg);
	send_to_char(buf, ch);
}

void do_unignore(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;
	char arg[MIL], buf[MSL];
	int pos;
	bool found = FALSE;

	if (ch->desc == NULL)
		rch = ch;
	else
		rch = ch->desc->original ? ch->desc->original : ch;

	if (IS_NPC(rch))
		return;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		if (rch->pcdata->ignore[0] == NULL) {
			send_to_char("You are not ignoring anyone.\n\r", ch);
			return;
		}
		send_to_char("You are currently ignoring:\n\r", ch);

		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (rch->pcdata->ignore[pos] == NULL)
				break;

			(void)snprintf(buf, 2 * MIL, "    %s\n\r", rch->pcdata->ignore[pos]);
			send_to_char(buf, ch);
		}
		return;
	}

	for (pos = 0; pos < MAX_IGNORE; pos++) {
		if (rch->pcdata->ignore[pos] == NULL)
			break;

		if (found) {
			rch->pcdata->ignore[pos - 1] = rch->pcdata->ignore[pos];
			rch->pcdata->ignore[pos] = NULL;
			continue;
		}

		if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
			send_to_char("Ignore removed.\n\r", ch);
			free_string(rch->pcdata->ignore[pos]);
			rch->pcdata->ignore[pos] = NULL;
			found = TRUE;
		}
	}

	if (!found)
		send_to_char("You aren't ignoring anyone by that name!\n\r", ch);
}

void do_sayto(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	int pos;
	bool found = FALSE;
	DESCRIPTOR_DATA *d;

	argument = one_argument(argument, arg);
	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("`8Say`` what to whom?\n\r", ch);
		return;
	}
	if ((victim = get_char_world(ch, arg)) == NULL
	    || victim->in_room != ch->in_room) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}
	if (victim->desc == NULL && !IS_NPC(victim)) {
		act("$N seems to have misplaced $S link...try again later.", ch, NULL, victim, TO_CHAR);
		return;
	}
	if (!IS_NPC(victim)) {
		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (victim->pcdata->ignore[pos] == NULL)
				break;
			if (!str_cmp(ch->name, victim->pcdata->ignore[pos]))
				found = TRUE;
		}
	}
	if (found) {
		if (!IS_IMMORTAL(ch)) {
			act("$N is not paying attention to you right now.", ch, NULL, victim, TO_CHAR);
			return;
		}
	}
	if (!(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim)) {
		act("$E can't hear you.", ch, 0, victim, TO_CHAR);
		return;
	}
	if (IS_SET(victim->comm2, COMM2_AFK)) {
		if (IS_NPC(victim)) {
			act("$E is `!A`@F`OK``, and is unable to pay attention.", ch, NULL, victim, TO_CHAR);
			return;
		}
		return;
	}
	printf_to_char(ch, "``You say to %s '`P%s``'\n\r",
		       victim->name, argument);

	printf_to_char(victim, "``%s says to you '`P%s``'\n\r",
		       ch->name,
		       argument);

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character->in_room == ch->in_room
		    && d->character != ch
		    && d->character->position != POS_SLEEPING
		    && d->character != victim) {
			if (!IS_NPC(victim)) {
				printf_to_char(d->character, "%s says to %s, '`P%s``'.\n\r",
					       PERS(ch, d->character),
					       PERS(victim, d->character), argument);
			} else {
				printf_to_char(d->character, "%s says to %s, '`P%s``'.\n\r",
					       PERS(ch, d->character),
					       PERS(victim, d->character), argument);
			}
		}
	}
}

