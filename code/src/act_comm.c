#include "merc.h"
#include "interp.h"
#include "channels.h"
#include "character.h"



void command_channel(CHAR_DATA *ch, char *argument) {
    static char arg1[MIL];
    static char arg2[MIL];
    static char arg3[MIL];
    const CHANNEL_DEFINITION const *channel;

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
            CHAR_DATA *target = NULL;

            if (argument[0] == '\0') {
                /** TODO - automatic "current channel" */
                send_to_char("Please specify a channel.", ch);
                return;
            }

            argument = one_argument(argument, arg2);
            channel = channels_parse(arg2);
            if (channel == NULL) {
                printf_to_char(ch, "There is no such channel: %s.", arg2);
                return;
            }

            switch (channel->target_requirement) {
                case CHANNEL_TARGET_NONE:
                    break;
                case CHANNEL_TARGET_OPTIONAL:
                {
                    if (argument[0] == '-') {
                        argument = one_argument(argument, arg3);
                        if ((target = get_char_world(ch, arg3+1)) == NULL) {
                            printf_to_char(ch, "There is no such victim: %s.", arg3);
                            return;
                        }
                    }
                    break;
                }
                case CHANNEL_TARGET_REQUIRED:
                {
                    if (argument[0] == '-') {
                        argument = one_argument(argument, arg3);
                        if ((target = get_char_world(ch, arg3+1)) == NULL) {
                            printf_to_char(ch, "There is no such victim: %s.", arg3);
                            return;
                        }
                    } else {
                        printf_to_char(ch, "You must specify a receiver for channel: %s.", channel->name);
                        return;
                    }
                    break;
                }
            }

            broadcast_channel(ch, channel, target, argument);
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
