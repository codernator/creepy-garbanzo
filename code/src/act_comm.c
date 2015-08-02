#include "merc.h"
#include "interp.h"
#include "channels.h"
#include "character.h"



void command_channel(CHAR_DATA *ch, char *argument) {
    static char arg1[MIL];
    static char arg2[MIL];
    static char arg3[MIL];
    CHANNEL_DEFINITION *channel;

    if (argument[0] == '\0') {
        channels_show(ch);
        return;
    }

    argument = one_argument(argument, arg1);

    switch (arg1[0]) {
        case '~': /* toggly-woggly */
        case '-': /* deny */
        case '+': /* grant */
        {
            char *chanName;
            CHAR_DATA *victim;

            if (arg1[1] != '\0') {
                chanName = arg1+1;
            } else {
                if (argument[0] == '\0') {
                    send_to_char("Please specify a channel.", ch);
                    return;
                }
                argument = one_argument(argument, arg2);
                chanName = arg2;
            }

            channel = channels_parse(chanName);
            if (channel == NULL) {
                printf_to_char(ch, "There is no such channel: %s.", chanName);
                return;
            }

            switch (arg1[0]) {
            case '~':
                channels_toggle(ch, channel);
                break;
            case '-':
            case '+':
                (void)one_argument(argument, arg3);
                if ((victim = get_char_world(ch, arg3)) == NULL) {
                    printf_to_char(ch, "There is no such victim: %s.", arg3);
                    return;
                }
                channels_permission(ch, victim, arg1[0] == '+', channel);
                break;
            }
            return;
        }
        default:
        {
            if (argument[0] == '\0') {
                /** TODO - automatic "current channel"  */
                send_to_char("Please specify a channel.", ch);
                return;
            }

            argument = one_argument(argument, arg2);
            channel = channels_parse(arg2);
            if (channel == NULL) {
                printf_to_char(ch, "There is no such channel: %s.", arg2);
                return;
            }

            broadcast_channel(ch, channel, argument);
            break;
        }
    }
}


void do_quiet(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    DENY_NPC(ch);
    toggle_quiet(ch);
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

