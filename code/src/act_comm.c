#include "merc.h"
#include "interp.h"
#include "channels.h"
#include "character.h"



void command_channel(CHAR_DATA *ch, char *argument) {
    static char arg1[MIL];
    static char arg2[MIL];
    const CHANNEL_DEFINITION const *channel;

    if (argument[0] == '\0') {
        channels_show(ch);
        return;
    }


    switch (argument[0]) {
        case '~': /* toggly-woggly */
        case '-': /* deny */
        case '+': /* grant */
        {
            /** EX1: channel ~ say */
            /** EX2: channel ~say */
            /** EX3: channel -say foobar */
            
            char operation = argument[0];
            char *chanName;
            CHAR_DATA *victim;

            if (argument[1] != '\0') {
                /** EX2, chanName = "say" */
                chanName = argument+1;
            } else {
                /** EX1, chanName = arg1 = "say", argument = "\0" */
                argument = one_argument(argument, arg1);
                if (arg1[0] == '\0') {
                    send_to_char("Please specify a channel.", ch);
                    return;
                }
                chanName = arg1;
            }

            channel = channels_parse(chanName);
            if (channel == NULL) {
                printf_to_char(ch, "There is no such channel: %s.", chanName);
                return;
            }

            switch (operation) {
            case '~':
                channels_toggle(ch, channel);
                break;
            case '-':
            case '+':
                (void)one_argument(argument, arg2);
                if (arg2[0] == '\0') {
                    send_to_char("Please specify a grantee.", ch);
                    return;
                }
                if ((victim = get_char_world(ch, arg2)) == NULL) {
                    printf_to_char(ch, "There is no such grantee: %s.", arg2);
                    return;
                }
                channels_permission(ch, victim, operation == '+', channel);
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

            argument = one_argument(argument, arg1);
            channel = channels_parse(arg1);
            if (channel == NULL) {
                printf_to_char(ch, "There is no such channel: %s.", arg1);
                return;
            }

            switch (channel->target_requirement) {
                case CHANNEL_TARGET_NONE:
                    break;
                case CHANNEL_TARGET_OPTIONAL:
                {
                    if (argument[0] == '-') {
                        argument = one_argument(argument, arg2);
                        if ((target = get_char_world(ch, arg2+1)) == NULL) {
                            printf_to_char(ch, "There is no such character: %s.", arg2);
                            return;
                        }
                    }
                    break;
                }
                case CHANNEL_TARGET_REQUIRED:
                {
                    if (argument[0] == '-') {
                        argument = one_argument(argument, arg2);
                        if ((target = get_char_world(ch, arg2+1)) == NULL) {
                            printf_to_char(ch, "There is no such character: %s.", arg2);
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
