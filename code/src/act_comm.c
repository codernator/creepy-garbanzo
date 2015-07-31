#include "merc.h"
#include "channels.h"
#include "character.h"



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

void do_noauction(CHAR_DATA *ch, /*@unused@*/char *argument)
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

void do_gtell(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("Tell your group what?\n\r", ch);
	} else {
        broadcast_gtell(ch, argument);
	}
}

void do_sayto(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];

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

    broadcast_sayto(ch, victim, argument);
}

