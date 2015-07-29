#include "merc.h"
#include "channels.h"

#include <stdio.h>
#include <string.h>

extern void mp_act_trigger(char *argument, CHAR_DATA * mob, CHAR_DATA * sender, const void *arg1, const void *arg2, int type);
extern bool add_buf(BUFFER *buffer, char *string);


static bool can_talk(CHAR_DATA *sender);
static bool is_ignoring(/*@partial@*/CHAR_DATA *sender, /*@partial@*/CHAR_DATA *receiver);
static bool send_tell(/*@partial@*/CHAR_DATA *sender, /*@partial@*/CHAR_DATA *whom, char* argument);
static char *emote_parse(char *argument);


void broadcast_auctalk(CHAR_DATA *sender, char *argument)
{
	DESCRIPTOR_DATA *d;

    if (!can_talk(sender)) {
        return;
    }

    REMOVE_BIT(sender->comm, COMM_NOAUCTION);

    printf_to_char(sender, "`3You `#(`3AucTalk`#)`3 '`#%s`3'``\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *receiver;

        receiver = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING
            && d->character != sender
            && !IS_SET(receiver->comm, COMM_NOAUCTION)
            && !IS_SET(receiver->comm, COMM_QUIET)) {
            act_new("`3$n `#(`3AucTalks`#)`3 '`#$t`3'``", sender, argument, d->character, TO_VICT, POS_DEAD, false);
        }
    }
    return;
}

void broadcast_immtalk(/*@partial@*/CHAR_DATA *sender, char *argument)
{
	static char buf[2*MIL];
	DESCRIPTOR_DATA *d;

	REMOVE_BIT(sender->comm, COMM_NOWIZ);

	(void)snprintf(buf, 2 * MIL, "```!$n `8: ```7%s``", argument);
	act_new("```!$n `8: ```7$t``", sender, argument, NULL, TO_CHAR, POS_DEAD, false);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected == CON_PLAYING &&
		    IS_IMMORTAL(d->character) &&
		    !IS_SET(d->character->comm, COMM_NOWIZ))
			act_new("```!$n `8: ```7$t``", sender, argument, d->character, TO_VICT, POS_DEAD, false);
	}
    return;
}

void broadcast_imptalk(CHAR_DATA *sender, char *argument)
{
	static char buf[2*MIL];
	DESCRIPTOR_DATA *d;

	REMOVE_BIT(sender->comm2, COMM2_IMPTALK);

	(void)snprintf(buf, 2 * MIL, "``$n `2I`8M`2P`8:`` %s``", argument);
	act_new("$n `2I`8M`2P`8:`` $t``", sender, argument, NULL, TO_CHAR, POS_DEAD, false);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if ((d->connected == CON_PLAYING) &&
		    (d->character->level == IMPLEMENTOR) &&
		    (!IS_SET(d->character->comm2, COMM2_IMPTALK)))
/*           && (!IS_SET(d->character->comm2, COMM2_IMPTALKM)))*/
			act_new("``$n `2I`8M`2P`8:`` $t", sender, argument, d->character, TO_VICT, POS_DEAD, false);
	}


	for (d = descriptor_list; d != NULL; d = d->next) {
		if ((d->connected == CON_PLAYING) &&
		    (IS_SET(d->character->comm2, COMM2_IMPTALKM)))
			act_new("``$n `2I`8M`2P`8:`` $t", sender, argument, d->character, TO_VICT, POS_DEAD, false);
	}
    return;
}

void broadcast_say(CHAR_DATA *sender, char *argument)
{
	static char buf[2*MIL];

	if (can_talk(sender)) {
		return;
	}

    (void)snprintf(buf, 2 * MIL, "`7You %s '`s$T`7'``",
               argument[strlen(argument) - 1] == '!' ? "exclaim" :
               argument[strlen(argument) - 1] == '?' ? "ask" : "say");
    act(buf, sender, NULL, argument, TO_CHAR);

    (void)snprintf(buf, 2 * MIL, "`7$n %s '`s$T`7'``",
               argument[strlen(argument) - 1] == '!' ? "exclaims" :
               argument[strlen(argument) - 1] == '?' ? "asks" : "says");
    act(buf, sender, NULL, argument, TO_ROOM);

    if (!IS_NPC(sender)) {
        CHAR_DATA *mob, *mob_next;
        for (mob = sender->in_room->people; mob != NULL; mob = mob_next) {
            mob_next = mob->next_in_room;
            if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
                && mob->position == mob->mob_idx->default_pos)
                mp_act_trigger(argument, mob, sender, NULL, NULL, TRIG_SPEECH);
        }
    }
    return;
}

void broadcast_shout(CHAR_DATA *sender, char *argument)
{
	DESCRIPTOR_DATA *d;
    
    if (!can_talk(sender)) {
        return;
    }

	if (IS_SET(sender->comm, COMM_NOSHOUT)) {
		send_to_char("You can't ```1shout``.\n\r", sender);
		return;
	}

	REMOVE_BIT(sender->comm, COMM_SHOUTSOFF);

	act("`1You shout '`!$T`1'``", sender, NULL, argument, TO_CHAR);
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *receiver;

		receiver = CH(d);

		if (d->connected == CON_PLAYING
		    && d->character != sender
		    && !IS_SET(receiver->comm, COMM_SHOUTSOFF)
		    && !IS_SET(receiver->comm, COMM_QUIET)) {
				act("`1$n shouts '`!$t`1'``", sender, argument, d->character, TO_VICT);
		}
	}
	return;
}

void broadcast_info(CHAR_DATA *sender, char *argument)
{
	DESCRIPTOR_DATA *d;

	SET_BIT(sender->comm2, COMM2_INFO);
	act("``You `![Info]:`` `&$T``'", sender, NULL, argument, TO_CHAR);
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *receiver;

		receiver = CH(d);

		if (d->connected == CON_PLAYING
		    && d->character != sender
		    && IS_SET(receiver->comm2, COMM2_INFO)
		    && !IS_SET(receiver->comm, COMM_QUIET))
			act("`![Info]:`& $t``", sender, argument, d->character, TO_VICT);

	}
    return;
}

void broadcast_tell(CHAR_DATA *sender, CHAR_DATA *whom, char *argument)
{
    if (send_tell(sender, whom, argument)) {
        whom->reply = sender;
        if (!IS_NPC(sender) && IS_NPC(whom) && HAS_TRIGGER(whom, TRIG_SPEECH)) {
            mp_act_trigger(argument, whom, sender, NULL, NULL, TRIG_SPEECH);
        }
    }
	return;
}

void broadcast_reply(CHAR_DATA *sender, char *argument)
{
	CHAR_DATA *whom;

	if ((whom = sender->reply) == NULL) {
		send_to_char("They aren't here.\n\r", sender);
		return;
	}

    if (send_tell(sender, whom, argument)) {
        whom->reply = sender;
    }

	return;
}

void broadcast_yell(CHAR_DATA *sender, char *argument)
{
	DESCRIPTOR_DATA *d;

    if (!can_talk(sender)) {
        return;
    }

	if (IS_SET(sender->comm, COMM_NOSHOUT)) {
		send_to_char("You can't ```1yell``.\n\r", sender);
		return;
	}

	act("`1You yell '`!$t`1'``", sender, argument, NULL, TO_CHAR);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character != sender
		    && d->character->in_room != NULL
		    && d->character->in_room->area == sender->in_room->area
		    && !IS_SET(d->character->comm, COMM_QUIET)) {
				act("`1$n yells '`!$t`1'``", sender, argument, d->character, TO_VICT);
		}
	}

	return;
}

void broadcast_emote(CHAR_DATA *sender, char *argument)
{
	if (!IS_NPC(sender) && IS_SET(sender->comm2, COMM2_NOEMOTE)) {
		send_to_char("You can't show your emotions.\n\r", sender);
		return;
	}
	act_new("$n $T", sender, NULL, emote_parse(argument), TO_ROOM, POS_RESTING, false);
	act_new("$n $T", sender, NULL, emote_parse(argument), TO_CHAR, POS_RESTING, false);
    return;
}

void broadcast_pmote(CHAR_DATA *sender, char *argument)
{
	CHAR_DATA *vch;
	char *letter;
	char *name;
	char last[MIL];
	char temp[MSL];
	int matches = 0;

	if (!IS_NPC(sender) && IS_SET(sender->comm2, COMM2_NOEMOTE)) {
		send_to_char("You can't show your emotions.\n\r", sender);
		return;
	}

	act("$n $t", sender, argument, NULL, TO_CHAR);

	for (vch = sender->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (vch->desc == NULL || vch == sender)
			continue;

		if ((letter = strstr(argument, vch->name)) == NULL) {
			act_new("$N $t", vch, argument, sender, TO_CHAR, POS_RESTING, false);
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

		act_new("$N $t", vch, temp, sender, TO_CHAR, POS_RESTING, false);
	}
    return;
}



bool is_ignoring(CHAR_DATA *sender, CHAR_DATA *receiver) 
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

bool can_talk(CHAR_DATA *sender)
{
	if (IS_SET(sender->comm, COMM_NOCHANNELS)) {
		send_to_char("The gods have revoked your channel priviliges.\n\r", sender);
		return false;
	}

	if (IS_SET(sender->comm, COMM_QUIET)) {
		send_to_char("You must turn off quiet mode first.\n\r", sender);
		return false;
	}

	return true;
}

bool send_tell(CHAR_DATA *sender, CHAR_DATA *whom, char* argument)
{
	static char buf[MSL];

	if (IS_SET(sender->comm, COMM_NOCHANNELS)) {
		send_to_char("The gods have revoked your channel priviliges.\n\r", sender);
		return false;
	}

	if (IS_SET(sender->comm, COMM_NOTELL)) {
		send_to_char("Your message didn't get through.\n\r", sender);
		return false;
	}

	if (!IS_IMMORTAL(whom) && !IS_AWAKE(sender)) {
		send_to_char("In your dreams, or what?\n\r", sender);
		return false;
	}

	if (is_ignoring(sender, whom)) {
		act("$N seems to be ignoring you.", sender, NULL, whom, TO_CHAR);
		return false;
	}

	if (!(IS_IMMORTAL(sender)) && !IS_AWAKE(whom)) {
		act("$E can't hear you.", sender, 0, whom, TO_CHAR);
		return false;
	}

	if (IS_SET(whom->comm, COMM_DEAF)) {
		act("$E is not receiving ```@tells``.", sender, 0, whom, TO_CHAR);
		return false;
	}

	if (IS_SET(whom->comm2, COMM2_AFK)) {
		if (IS_NPC(whom)) {
			act("$E is ```!A```@F```OK``, and not receiving tells.", sender, NULL, whom, TO_CHAR);
			return false;
		}

		act("$E is ```!A```@F```OK``, but your tell will go through when $E returns.", sender, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "```@%s tells you '`t%s```@'``\n\r", PERS(sender, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return true;
	}

    if (whom->desc == NULL && !IS_NPC(whom)) {
        act("$N seems to have misplaced $S link...try again later.", sender, NULL, whom, TO_CHAR);
        (void)snprintf(buf, 2 * MIL, "```@%s tells you '`t%s```@'``\n\r", PERS(sender, whom), argument);
        buf[0] = UPPER(buf[0]);
        add_buf(whom->pcdata->buffer, buf);
        return true;
    }

	if (IS_SET(whom->comm2, COMM2_BUSY)) {
		if (IS_NPC(whom)) {
			act("$E is Busy, and not receiving tells.", sender, NULL, whom, TO_CHAR);
			return false;
		}

		act("$E is `1Busy``, but your tell will go through when $E returns.", sender, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "```@%s tells you '`t%s```@'``\n\r", PERS(sender, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return true;
	}

	if (IS_SET(whom->comm2, COMM2_CODING)) {
		if (IS_NPC(whom)) {
			act("$E is coding, and not receiving tells.\n\r", sender, NULL, whom, TO_CHAR);
			return false;
		}

		act("$E is coding, but your tell will go through when $E returns.\n\r", sender, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "%s tells you '%s'\n\r", PERS(sender, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return true;
	}

	if (IS_SET(whom->comm2, COMM2_BUILD)) {
		if (IS_NPC(whom)) {
			act("$E is building, and not receiving tells.\n\r", sender, NULL, whom, TO_CHAR);
			return false;
		}

		act("$E is building, but your tell will go through when $E returns.\n\r", sender, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "%s tells you '%s'\n\r", PERS(sender, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return true;
	}



	act("`@You tell $N '`r$t`@'``", sender, argument, whom, TO_CHAR);
	act_new("`@$n tells you '`r$t`@'``", sender, argument, whom, TO_VICT, POS_DEAD, false);
    return true;
}

char *emote_parse(char *argument)
{
	static char target[MSL];
	char buf[MSL];
	int i = 0, arg_len = 0;
	int flag = false;

	/* Reset target each time before use */
	memset(target, 0x00, sizeof(target));
	arg_len = (int)strlen(argument);
	for (i = 0; i < arg_len; i++) {
		if (argument[i] == '"' && flag == false) {
			flag = true;
			(void)snprintf(buf, MSL, "%c", argument[i]);
			strcat(target, buf);
			strcat(target, "`P");
			continue;
		} else if (argument[i] == '"' && flag == true) {
			flag = false;
			strcat(target, "``");
			(void)snprintf(buf, MSL, "%c", argument[i]);
			strcat(target, buf);
			continue;
		} else {
			(void)snprintf(buf, MSL, "%c", argument[i]);
			strcat(target, buf);
		}
	}
	if (flag) {
		strcat(target, "``");
		(void)snprintf(buf, MSL, "%c", '"');
		strcat(target, buf);
	}
	return target;
}

