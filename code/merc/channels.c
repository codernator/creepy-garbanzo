#include "merc.h"
#include "channels.h"
#include "tables.h"
#include "recycle.h"


#include <stdio.h>
#include <string.h>


#define DECLARE_BROADCASTER(fun)              BROADCAST_FUNCTION fun
#define DECLARE_TARGETED_BROADCASTER(fun)     BROADCAST_TARGET_FUNCTION fun
#define CHAN_ENABLED(ch, channel_flag)        (((ch)->channels_enabled & (channel_flag)) == (channel_flag))
#define ENABLE_CHAN(ch, channel_flag)         ((ch)->channels_enabled |= (channel_flag))
#define DISABLE_CHAN(ch, channel_flag)        ((ch)->channels_enabled &= ~(channel_flag))

#define CHAN_DENIED(ch, channel_flag)         (((ch)->channels_denied & (channel_flag)) == (channel_flag))
#define DENY_CHAN(ch, channel_flag)           ((ch)->channels_denied |= (channel_flag))
#define GRANT_CHAN(ch, channel_flag)          ((ch)->channels_denied &= ~(channel_flag))


/** imports */
extern void mp_act_trigger(const char *argument, struct char_data * mob, struct char_data * sender, const void *arg1, const void *arg2, int type);


/** locals */
static bool is_ignoring(/*@partial@*/struct char_data *sender, /*@partial@*/struct char_data *receiver);
static bool send_tell(/*@partial@*/struct char_data *sender, /*@partial@*/struct char_data *whom, const char* argument);
static char *emote_parse(const char *argument);

static DECLARE_BROADCASTER(broadcast_global);
static DECLARE_BROADCASTER(broadcast_shout);
static DECLARE_BROADCASTER(broadcast_emote);
static DECLARE_BROADCASTER(broadcast_pmote);
static DECLARE_BROADCASTER(broadcast_say);
static DECLARE_BROADCASTER(broadcast_reply);
static DECLARE_BROADCASTER(broadcast_gtell);
static DECLARE_TARGETED_BROADCASTER(broadcast_tell);
static DECLARE_TARGETED_BROADCASTER(broadcast_sayto);


static const CHANNEL_DEFINITION const channels[] =
{
    { CHANNEL_AUCTION, "auction", "`3Auction``", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_global, NULL },
    { CHANNEL_SHOUT, "shout", "Shout", true, POS_RESTING, CHANNEL_TARGET_NONE, broadcast_shout, NULL },
    { CHANNEL_TELL, "tell", "Tell", false, POS_DEAD, CHANNEL_TARGET_REQUIRED, NULL, broadcast_tell },
    { CHANNEL_TELL, "reply", "Tell", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_reply, NULL },
    { CHANNEL_TELL, "gtell", "Tell", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_gtell, NULL },
    { CHANNEL_CODENET, "codenet", "Code-net", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_global, NULL },
    { CHANNEL_ADVOCATENET, "advocatenet", "Advocate-net", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_global, NULL },
    { CHANNEL_BUILDNET, "buildnet", "Build-net", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_global, NULL },
    { CHANNEL_EMOTE, "emote", "Emote", true, POS_RESTING, CHANNEL_TARGET_NONE, broadcast_emote, NULL },
    { CHANNEL_EMOTE, "pmote", "Emote", true, POS_RESTING, CHANNEL_TARGET_NONE, broadcast_pmote, NULL },
    { CHANNEL_INFO, "infonet", "`![Info]``", false, POS_DEAD, CHANNEL_TARGET_NONE, broadcast_global, NULL },
    { CHANNEL_SAY, "say", "Say", true, POS_RESTING, CHANNEL_TARGET_OPTIONAL, broadcast_say, broadcast_sayto },
    { 0, "", "", false, 0, 0, NULL }
};


/***
 * Interface
 */

void channels_toggle(struct char_data *sender, const CHANNEL_DEFINITION const *channel)
{
    bool now_on;

    if (CHAN_DENIED(sender, channel->flag)) {
	printf_to_char(sender, "You may not access the %s channel.", channel->name);
	return;
    }

    if (CHAN_ENABLED(sender, channel->flag)) {
	now_on = false;
	DISABLE_CHAN(sender, channel->flag);
    } else {
	now_on = true;
	ENABLE_CHAN(sender, channel->flag);
    }

    printf_to_char(sender, "%s is now %s.\n\r", channel->name, now_on ? "ON" : "OFF");
}

void channels_show(struct char_data *sender)
{
    int i;
    bool enabled;
    bool denied;
    unsigned long flag;


    printf_to_char(sender, "%-14s%-7s\n\r", "CHANNEL", "STATUS");
    send_to_char("```&---------------------``\n\r", sender);

    for (i = 0; (flag = channels[i].flag) != 0; i++) {
	denied = (IS_SET(sender->channels_denied, flag));
	enabled = (IS_SET(sender->channels_enabled, flag));
	printf_to_char(sender, "%-14s%-7s\n\r", channels[i].name, denied ? "--" : (enabled ? "`#ON" : "`1OFF"));
    }
}

void channels_permission(struct char_data *grantor, struct char_data *grantee, bool granted, const CHANNEL_DEFINITION const *channel)
{
    static char buf[MAX_INPUT_LENGTH];

    DENY_NPC(grantor);

    if (IS_NPC(grantee)) {
	send_to_char("Not on NPC's.\n\r", grantor);
	return;
    }

    if (get_trust(grantee) >= get_trust(grantor)) {
	send_to_char("You failed.\n\r", grantor);
	return;
    }


    if (granted) {
	if (!CHAN_DENIED(grantee, channel->flag)) {
	    printf_to_char(grantor, "%s is already granted channel %s.\n\r", capitalize(grantee->name), channel->name);
	    return;
	}
	GRANT_CHAN(grantee, channel->flag);
    } else {
	if (CHAN_DENIED(grantee, channel->flag)) {
	    printf_to_char(grantor, "%s is already denied channel %s.\n\r", capitalize(grantee->name), channel->name);
	    return;
	}
	DENY_CHAN(grantee, channel->flag);
    }

    printf_to_char(grantee, "You have been %s access to channel %s.\n\r", granted ? "granted": "denied", channel->name);
    printf_to_char(grantor, "Channel access %s to %s.", granted ? "granted": "denied", capitalize(grantee->name));
    (void)snprintf(buf, MAX_INPUT_LENGTH, "$N %s %s to %s.", granted ? "grants" : "denies", channel->name, grantee->name);
    wiznet(buf, grantor, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
}

void broadcast_channel(struct char_data *sender, const CHANNEL_DEFINITION const *channel, struct char_data *target, const char *argument)
{
    if (sender != NULL) {
	if (CHAN_DENIED(sender, channel->flag)) {
	    send_to_char("You may not use that channel.", sender);
	    return;
	}

	ENABLE_CHAN(sender, channel->flag);
    }

    if (channel->broadcaster == NULL) {
	if (sender != NULL) {
	    send_to_char("Bug logged.", sender);
	}
	log_bug("No broadcaster for channel %s (%ul).", channel->name, channel->flag);
	return;
    }

    switch (channel->target_requirement)
    {
	case CHANNEL_TARGET_NONE:
	    {
		if (target != NULL) {
		    log_bug("Ignored target provided for channel %s (%ul).", channel->name, channel->flag);
		}
		channel->broadcaster(channel, sender, argument);
		return;
	    }
	case CHANNEL_TARGET_OPTIONAL:
	    {
		if (target == NULL) {
		    channel->broadcaster(channel, sender, argument);
		} else {
		    channel->targeted_broadcaster(channel, sender, target, argument);
		}
		return;
	    }
	case CHANNEL_TARGET_REQUIRED:
	    {
		if (target == NULL) {
		    if (sender != NULL) {
			send_to_char("Bug logged.", sender);
		    }
		    log_bug("Target is required and not provided for channel %s (%ul).", channel->name, channel->flag);
		    return;
		}

		if (is_ignoring(sender, target)) {
		    act_new("$N seems to be ignoring you.", sender, NULL, target, TO_CHAR, POS_DEAD, false);
		    return;
		}

		if (!(IS_IMMORTAL(sender) && sender->level > LEVEL_IMMORTAL) && !IS_AWAKE(target)) {
		    act_new("$E can't hear you.", sender, 0, target, TO_CHAR, POS_DEAD, false);
		    return;
		}

		channel->targeted_broadcaster(channel, sender, target, argument);
		return;
	    }
    }
}

const CHANNEL_DEFINITION const *channels_parse(const char *argument)
{
    int i;
    for (i = 0; channels[i].flag != 0; i++) {
	if (!strncmp(argument, channels[i].name, MAX_INPUT_LENGTH)) {
	    return &channels[i];
	}
    }
    return NULL;
}

const CHANNEL_DEFINITION const *channels_find(CHANNEL_FLAG_TYPE channel_flag)
{
    int i;
    for (i = 0; channels[i].flag != 0; i++) {
	if (channels[i].flag == channel_flag) {
	    return &channels[i];
	}
    }
    return NULL;
}


/***
 * Broadcasters
 */

void broadcast_shout(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = sender };
    struct descriptor_data *d;
    struct descriptor_data *dpending;
    struct char_data *actual;
    struct char_data *receiver;

    act_new("`1You shout '`!$T`1'``", sender, NULL, argument, TO_CHAR, POS_DEAD, channel->mob_trigger);
    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	actual = CH(d);
	receiver = d->character;

	if (receiver->in_room != NULL && receiver->in_room->area == sender->in_room->area && CHAN_ENABLED(actual, channel->flag) && !CHAN_DENIED(actual, channel->flag)) {
	    act_new("`1$n shouts '`!$t`1'``", sender, argument, receiver, TO_VICT, channel->receiver_position, false);
	}
    }
}

void broadcast_global(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    static char buf[2*MAX_INPUT_LENGTH];
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = sender };
    struct descriptor_data *dpending;
    struct descriptor_data *d;
    struct char_data *actual;

    (void)snprintf(buf, 2 * MAX_INPUT_LENGTH, "``$n %s: %s``", channel->print_name, argument);
    if (sender != NULL) {
	act_new("$n `2I`8M`2P`8:`` $t``", sender, argument, NULL, TO_CHAR, POS_DEAD, channel->mob_trigger);
    }
    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	actual = CH(d);

	(void)snprintf(buf, 2 * MAX_INPUT_LENGTH, "``$t %s: %s``", channel->print_name, argument);
	if (CHAN_ENABLED(actual, channel->flag) && !CHAN_DENIED(actual, channel->flag)) {
	    act_new(buf, sender, argument, d->character, TO_VICT, channel->receiver_position, false);
	}
    }
}

void broadcast_say(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
#define VERBSIZE 20
    static char buf[2*MAX_INPUT_LENGTH];
    static char verb[VERBSIZE];
    int len = strlen(argument);
    int puncpos = len - 1;
    int puncpos2 = len - 2;

    switch (puncpos)
    {
	case '!':
	    {
		if (puncpos2 >= 0) {
		    switch (argument[puncpos2])
		    {
			case '!': snprintf(verb, VERBSIZE, "%s", "shout"); break;
			case '?': snprintf(verb, VERBSIZE, "%s", "angrily demand"); break;
			default: snprintf(verb, VERBSIZE, "%s", "exclaim"); break;
		    }
		} else {
		    snprintf(verb, VERBSIZE, "%s", "exclaim");
		}
		break;
	    }
	case '?':
	    {
		if (puncpos2 >= 0) {
		    switch (argument[puncpos2])
		    {
			case '?': snprintf(verb, VERBSIZE, "%s", "in confusion, ask"); break;
			case '!': snprintf(verb, VERBSIZE, "%s", "angrily demand"); break;
			default: snprintf(verb, VERBSIZE, "%s", "ask"); break;
		    }
		} else {
		    snprintf(verb, VERBSIZE, "%s", "ask");
		}
		break;
	    }
	default: snprintf(verb, VERBSIZE, "%s", "say"); break;
    }

    (void)snprintf(buf, 2 * MAX_INPUT_LENGTH, "`7You %s '`s$T`7'``", verb);
    act_new(buf, sender, NULL, argument, TO_CHAR, POS_RESTING, channel->mob_trigger);

    (void)snprintf(buf, 2 * MAX_INPUT_LENGTH, "`7$n %ss '`s$T`7'``", verb);
    act_new(buf, sender, NULL, argument, TO_ROOM, channel->receiver_position, false);
}

void broadcast_emote(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    act_new("$n $T", sender, NULL, emote_parse(argument), TO_CHAR, POS_RESTING, channel->mob_trigger);
    act_new("$n $T", sender, NULL, emote_parse(argument), TO_ROOM, channel->receiver_position, false);
}

void broadcast_pmote(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    struct char_data *vch;
    char *letter;
    char *name;
    char last[MAX_INPUT_LENGTH];
    char temp[MAX_STRING_LENGTH];
    int matches = 0;

    act_new("$n $t", sender, argument, NULL, TO_CHAR, POS_DEAD, channel->mob_trigger);

    for (vch = sender->in_room->people; vch != NULL; vch = vch->next_in_room) {
	if (vch->desc == NULL || vch == sender)
	    continue;

	if ((letter = strstr(argument, vch->name)) == NULL) {
	    act_new("$N $t", vch, argument, sender, TO_CHAR, channel->receiver_position, false);
	    continue;
	}

	strcpy(temp, argument);
	temp[strlen(argument) - strlen(letter)] = '\0';
	last[0] = '\0';
	name = vch->name;

	for (; *letter != '\0'; letter++) {
	    if (*letter == '\'' && matches == (int)strlen(vch->name)) {
		strcat(temp, "r");
		continue;
	    }

	    if (*letter == 's' && matches == (int)strlen(vch->name)) {
		matches = 0;
		continue;
	    }

	    if (matches == (int)strlen(vch->name))
		matches = 0;

	    if (*letter == *name) {
		matches++;
		name++;
		if (matches == (int)strlen(vch->name)) {
		    strcat(temp, "you");
		    last[0] = '\0';
		    name = vch->name;
		    continue;
		}
		strncat(last, letter, 1);
		continue;
	    }

	    matches = 0;
	    strcat(temp, last);
	    strncat(temp, letter, 1);
	    last[0] = '\0';
	    name = vch->name;
	}

	act_new("$N $t", vch, temp, sender, TO_CHAR, channel->receiver_position, false);
    }
}

void broadcast_smote(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    struct char_data *vch;
    char *letter;
    char *name;
    char last[MAX_INPUT_LENGTH];
    char temp[MAX_STRING_LENGTH];
    int matches = 0;

    if (strstr(argument, sender->name) == NULL) {
	send_to_char("You must include your name in an smote.\n\r", sender);
	return;
    }

    printf_to_char(sender, "%s\n\r", argument);

    for (vch = sender->in_room->people; vch != NULL; vch = vch->next_in_room) {
	if (vch->desc == NULL || vch == sender)
	    continue;

	if ((letter = strstr(argument, vch->name)) == NULL) {
	    printf_to_char(vch, "%s\n\r", argument);
	    continue;
	}

	strcpy(temp, argument);
	temp[strlen(argument) - strlen(letter)] = '\0';
	last[0] = '\0';
	name = vch->name;

	for (; *letter != '\0'; letter++) {
	    if (*letter == '\'' && matches == (int)strlen(vch->name)) {
		strcat(temp, "r");
		continue;
	    }

	    if (*letter == 's' && matches == (int)strlen(vch->name)) {
		matches = 0;
		continue;
	    }

	    if (matches == (int)strlen(vch->name))
		matches = 0;

	    if (*letter == *name) {
		matches++;
		name++;

		if (matches == (int)strlen(vch->name)) {
		    strcat(temp, "you");
		    last[0] = '\0';
		    name = vch->name;
		    continue;
		}

		strncat(last, letter, 1);
		continue;
	    }

	    matches = 0;
	    strcat(temp, last);
	    strncat(temp, letter, 1);
	    last[0] = '\0';
	    name = vch->name;
	}

	printf_to_char(vch, "%s\n\r", temp);
    }
}

void broadcast_reply(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    struct char_data *whom;

    if ((whom = sender->reply) == NULL) {
	send_to_char("They aren't here.\n\r", sender);
	return;
    }

    if (send_tell(sender, whom, argument)) {
	whom->reply = sender;
    }

    return;
}

void broadcast_gtell(const CHANNEL_DEFINITION const *channel, struct char_data *sender, const char *argument)
{
    struct char_data *gch;

    for (gch = char_list; gch != NULL; gch = gch->next) {
	if (is_same_group(gch, sender)) {
	    printf_to_char(gch, "```5%s tells the group '```P%s```5'``\n\r", sender->name, argument);
	}
    }
}


/***
 * Targetted Broadcasters
 */

void broadcast_sayto(const CHANNEL_DEFINITION const *channel, struct char_data *sender, struct char_data *whom, const char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = sender };
    struct descriptor_data *d;
    struct descriptor_data *dpending;

    if (IS_SET(whom->comm, COMM_AFK) && IS_NPC(whom)) {
	act("$E is `!A`@F`OK``, and is unable to pay attention.", sender, NULL, whom, TO_CHAR);
	return;
    }

    printf_to_char(sender, "``You say to %s '`P%s``'\n\r", whom->name, argument);
    printf_to_char(whom, "``%s says to you '`P%s``'\n\r", sender->name, argument);

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	if (d->character->in_room == sender->in_room
		&& d->character->position != POS_SLEEPING
		&& d->character != whom) {
	    printf_to_char(d->character, "%s says to %s, '`P%s``'.\n\r", PERS(sender, d->character), PERS(whom, d->character), argument);
	}
    }
}

void broadcast_tell(const CHANNEL_DEFINITION const *channel, struct char_data *sender, struct char_data *whom, const char *argument)
{
    if (send_tell(sender, whom, argument)) {
	whom->reply = sender;
	if (!IS_NPC(sender) && IS_NPC(whom) && HAS_TRIGGER(whom, TRIG_SPEECH)) {
	    mp_act_trigger(argument, whom, sender, NULL, NULL, TRIG_SPEECH);
	}
    }
}


/***
 * Util
 */

bool is_ignoring(struct char_data *sender, struct char_data *receiver)
{
    int pos;
    bool found = false;

    if (IS_NPC(receiver)) {
	return false;
    }

    for (pos = 0; pos < MAX_IGNORE; pos++) {
	if (receiver->pcdata->ignore[pos] == NULL) {
	    break;
	}
	if (!str_cmp(sender->name, receiver->pcdata->ignore[pos])) {
	    found = true;
	    break;
	}
    }
    return found;
}

bool send_tell(struct char_data *sender, struct char_data *whom, const char* argument)
{
    static char buf[MAX_STRING_LENGTH];

    if (!IS_IMMORTAL(whom) && !IS_AWAKE(sender)) {
	send_to_char("In your dreams, or what?\n\r", sender);
	return false;
    }

    if (IS_SET(whom->comm, COMM_AFK)) {
	if (IS_NPC(whom)) {
	    act("$E is ```!A```@F```OK``, and not receiving tells.", sender, NULL, whom, TO_CHAR);
	    return false;
	}

	act("$E is ```!A```@F```OK``, but your tell will go through when $E returns.", sender, NULL, whom, TO_CHAR);
	(void)snprintf(buf, 2 * MAX_INPUT_LENGTH, "```@%s tells you '`t%s```@'``\n\r", PERS(sender, whom), argument);
	buf[0] = UPPER(buf[0]);
	add_buf(whom->pcdata->buffer, buf);
	return true;
    }

    if (whom->desc == NULL && !IS_NPC(whom)) {
	act("$N seems to have misplaced $S link...try again later.", sender, NULL, whom, TO_CHAR);
	(void)snprintf(buf, 2 * MAX_INPUT_LENGTH, "```@%s tells you '`t%s```@'``\n\r", PERS(sender, whom), argument);
	buf[0] = UPPER(buf[0]);
	add_buf(whom->pcdata->buffer, buf);
	return true;
    }


    act_new("`@You tell $N '`r$t`@'``", sender, argument, whom, TO_CHAR, POS_DEAD, false);
    act_new("`@$n tells you '`r$t`@'``", sender, argument, whom, TO_VICT, POS_DEAD, false);
    return true;
}

char *emote_parse(const char *argument)
{
    static char target[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int i = 0, arg_len = 0;
    int flag = false;

    /* Reset target each time before use */
    memset(target, 0x00, sizeof(target));
    arg_len = (int)strlen(argument);
    for (i = 0; i < arg_len; i++) {
	if (argument[i] == '"' && flag == false) {
	    flag = true;
	    (void)snprintf(buf, MAX_STRING_LENGTH, "%c", argument[i]);
	    strcat(target, buf);
	    strcat(target, "`P");
	    continue;
	} else if (argument[i] == '"' && flag == true) {
	    flag = false;
	    strcat(target, "``");
	    (void)snprintf(buf, MAX_STRING_LENGTH, "%c", argument[i]);
	    strcat(target, buf);
	    continue;
	} else {
	    (void)snprintf(buf, MAX_STRING_LENGTH, "%c", argument[i]);
	    strcat(target, buf);
	}
    }
    if (flag) {
	strcat(target, "``");
	(void)snprintf(buf, MAX_STRING_LENGTH, "%c", '"');
	strcat(target, buf);
    }
    return target;
}

