#include "merc.h"
#include "channels.h"

#include <stdio.h>
#include <string.h>

extern void mp_act_trigger(char *argument, CHAR_DATA * mob, CHAR_DATA * ch, const void *arg1, const void *arg2, int type);
extern bool add_buf(BUFFER *buffer, char *string);


static bool can_talk(CHAR_DATA *ch);
static bool is_ignoring(CHAR_DATA *sender, CHAR_DATA *receiver);


void broadcast_auctalk(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;

    if (!can_talk(ch)) {
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOAUCTION);

    printf_to_char(ch, "`3You `#(`3AucTalk`#)`3 '`#%s`3'``\n\r", argument);

    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING
            && d->character != ch
            && !IS_SET(victim->comm, COMM_NOAUCTION)
            && !IS_SET(victim->comm, COMM_QUIET)) {
            act_new("`3$n `#(`3AucTalks`#)`3 '`#$t`3'``", ch, argument, d->character, TO_VICT, POS_DEAD, FALSE);
        }
    }
    return;
}

void broadcast_wish(CHAR_DATA *ch, char *argument)
{
	static char buf[2*MIL];
	DESCRIPTOR_DATA *d;

	if (!can_talk(ch)) {
		return;
    }

	if (IS_SET(ch->comm, COMM_NOWISH)) {
		send_to_char("The gods are deaf to your wishes.\n\r", ch);
		return;
	}

	(void)snprintf(buf, 2 * MIL, "`7$n `Owishes`7: ``%s", argument);
	act_new("`7$n `Owishes`7: $t", ch, argument, NULL, TO_CHAR, POS_DEAD, FALSE);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected == CON_PLAYING
		    && IS_IMMORTAL(d->character)
		    && !IS_SET(d->character->comm, COMM_NOWISH))
			act_new("`7$n `Owishes`7: $t", ch, argument, d->character, TO_VICT, POS_DEAD, FALSE);
	}
    return;
}

void broadcast_ooc(/*@partial@*/CHAR_DATA *ch, char *argument)
{
	static char buf[2*MIL];
	DESCRIPTOR_DATA *d;

    if (!can_talk(ch)) {
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOOOC);

    (void)snprintf(buf, 2 * MIL, "`7You `#OOC`7: '`#%s`7'\n\r", argument);
    send_to_char(buf, ch);
    (void)snprintf(buf, 2 * MIL, "\n\r`7$n `#OOC`7: '`#%s`7'", argument);
    for (d = descriptor_list; d != NULL; d = d->next) {
        CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            !IS_SET(victim->comm, COMM_NOOOC) &&
            !IS_SET(victim->comm, COMM_QUIET)) {
                act_new("`7$n `#OOC`7: '`#$t`7'", ch, argument, d->character, TO_VICT, POS_DEAD, FALSE);
        }
    }
    return;
}

void broadcast_immtalk(/*@partial@*/CHAR_DATA *ch, char *argument)
{
	static char buf[2*MIL];
	DESCRIPTOR_DATA *d;

	REMOVE_BIT(ch->comm, COMM_NOWIZ);

	(void)snprintf(buf, 2 * MIL, "```!$n `8: ```7%s``", argument);
	act_new("```!$n `8: ```7$t``", ch, argument, NULL, TO_CHAR, POS_DEAD, FALSE);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected == CON_PLAYING &&
		    IS_IMMORTAL(d->character) &&
		    !IS_SET(d->character->comm, COMM_NOWIZ))
			act_new("```!$n `8: ```7$t``", ch, argument, d->character, TO_VICT, POS_DEAD, FALSE);
	}
    return;
}

void broadcast_imptalk(CHAR_DATA *ch, char *argument)
{
	static char buf[2*MIL];
	DESCRIPTOR_DATA *d;

	REMOVE_BIT(ch->comm2, COMM2_IMPTALK);

	(void)snprintf(buf, 2 * MIL, "``$n `2I`8M`2P`8:`` %s``", argument);
	act_new("$n `2I`8M`2P`8:`` $t``", ch, argument, NULL, TO_CHAR, POS_DEAD, FALSE);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if ((d->connected == CON_PLAYING) &&
		    (d->character->level == IMPLEMENTOR) &&
		    (!IS_SET(d->character->comm2, COMM2_IMPTALK)))
/*           && (!IS_SET(d->character->comm2, COMM2_IMPTALKM)))*/
			act_new("``$n `2I`8M`2P`8:`` $t", ch, argument, d->character, TO_VICT, POS_DEAD, FALSE);
	}


	for (d = descriptor_list; d != NULL; d = d->next) {
		if ((d->connected == CON_PLAYING) &&
		    (IS_SET(d->character->comm2, COMM2_IMPTALKM)))
			act_new("``$n `2I`8M`2P`8:`` $t", ch, argument, d->character, TO_VICT, POS_DEAD, FALSE);
	}
    return;
}

void broadcast_say(CHAR_DATA *ch, char *argument)
{
	static char buf[2*MIL];

	if (can_talk(ch)) {
		return;
	}

    (void)snprintf(buf, 2 * MIL, "`7You %s '`s$T`7'``",
               argument[strlen(argument) - 1] == '!' ? "exclaim" :
               argument[strlen(argument) - 1] == '?' ? "ask" : "say");
    act(buf, ch, NULL, argument, TO_CHAR);

    (void)snprintf(buf, 2 * MIL, "`7$n %s '`s$T`7'``",
               argument[strlen(argument) - 1] == '!' ? "exclaims" :
               argument[strlen(argument) - 1] == '?' ? "asks" : "says");
    act(buf, ch, NULL, argument, TO_ROOM);

    if (!IS_NPC(ch)) {
        CHAR_DATA *mob, *mob_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next) {
            mob_next = mob->next_in_room;
            if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
                && mob->position == mob->mob_idx->default_pos)
                mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
        }
    }
    return;
}

void broadcast_osay(CHAR_DATA *ch, char *argument)
{
	act("`6$n says (`&ooc`6) '`7$T`6'`7", ch, NULL, argument, TO_ROOM);
	act("`6You say (`&ooc`6) '`7$T`6'`7", ch, NULL, argument, TO_CHAR);

	if (!IS_NPC(ch)) {
		CHAR_DATA *mob, *mob_next;
		for (mob = ch->in_room->people; mob != NULL; mob = mob_next) {
			mob_next = mob->next_in_room;
			if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
			    && mob->position == mob->mob_idx->default_pos)
				mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
		}
	}
    return;
}

void broadcast_shout(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
    
    if (!can_talk(ch)) {
        return;
    }

	if (IS_SET(ch->comm, COMM_NOSHOUT)) {
		send_to_char("You can't ```1shout``.\n\r", ch);
		return;
	}

	REMOVE_BIT(ch->comm, COMM_SHOUTSOFF);

	act("`1You shout '`!$T`1'``", ch, NULL, argument, TO_CHAR);
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *victim;

		victim = CH(d);

		if (d->connected == CON_PLAYING
		    && d->character != ch
		    && !IS_SET(victim->comm, COMM_SHOUTSOFF)
		    && !IS_SET(victim->comm, COMM_QUIET)) {
				act("`1$n shouts '`!$t`1'``", ch, argument, d->character, TO_VICT);
		}
	}
	return;
}

void broadcast_info(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;

	SET_BIT(ch->comm2, COMM2_INFO);
	act("``You `![Info]:`` `&$T``'", ch, NULL, argument, TO_CHAR);
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *victim;

		victim = CH(d);

		if (d->connected == CON_PLAYING
		    && d->character != ch
		    && IS_SET(victim->comm2, COMM2_INFO)
		    && !IS_SET(victim->comm, COMM_QUIET))
			act("`![Info]:`& $t``", ch, argument, d->character, TO_VICT);

	}
    return;
}

void broadcast_tell(CHAR_DATA *ch, CHAR_DATA *whom, char *argument)
{
	static char buf[MSL];

    if (!can_talk(ch)) {
        return;
    }

	if (IS_SET(ch->comm, COMM_NOTELL)) {
		send_to_char("Your message didn't get through.\n\r", ch);
		return;
	}

	if (is_ignoring(ch, whom)) {
		act("$N seems to be ignoring you.", ch, NULL, whom, TO_CHAR);
		return;
	}

	if (!(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(whom)) {
		act("$E can't hear you.", ch, 0, whom, TO_CHAR);
		return;
	}

	if ((IS_SET(whom->comm, COMM_QUIET) || IS_SET(whom->comm, COMM_DEAF))
	    && !IS_IMMORTAL(ch)) {
		act("$E is not receiving ```@tells``.", ch, 0, whom, TO_CHAR);
		return;
	}

	if (IS_SET(whom->comm2, COMM2_AFK)) {
		if (IS_NPC(whom)) {
			act("$E is ```!A```@F```OK``, and not receiving tells.", ch, NULL, whom, TO_CHAR);
			return;
		}

		act("$E is ```!A```@F```OK``, but your tell will go through when $E returns.", ch, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "```@%s tells you '`t%s```@'``\n\r", PERS(ch, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return;
	}
	act("`@You tell $N '`t$t`@'``", ch, argument, whom, TO_CHAR);
	act_new("`@$n tells you '`t$t`@'``", ch, argument, whom, TO_VICT, POS_DEAD, FALSE);
	whom->reply = ch;

	if (IS_SET(whom->comm2, COMM2_BUSY)) {
		if (IS_NPC(whom)) {
			act("$E is Busy, and not receiving tells.", ch, NULL, whom, TO_CHAR);
			return;
		}

		act("$E is `1Busy``, but your tell will go through when $E returns.", ch, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "```@%s tells you '`t%s```@'``\n\r", PERS(ch, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return;
	}

	if (IS_SET(whom->comm2, COMM2_CODING)) {
		if (IS_NPC(whom)) {
			act("$E is coding, and not receiving tells.\n\r", ch, NULL, whom, TO_CHAR);
			return;
		}

		act("$E is coding, but your tell will go through when $E returns.\n\r", ch, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "%s tells you '%s'\n\r", PERS(ch, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return;
	}

	if (IS_SET(whom->comm2, COMM2_BUILD)) {
		if (IS_NPC(whom)) {
			act("$E is building, and not receiving tells.\n\r", ch, NULL, whom, TO_CHAR);
			return;
		}

		act("$E is building, but your tell will go through when $E returns.\n\r", ch, NULL, whom, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "%s tells you '%s'\n\r", PERS(ch, whom), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(whom->pcdata->buffer, buf);
		return;
	}

	if (!IS_NPC(ch) && IS_NPC(whom) && HAS_TRIGGER(whom, TRIG_SPEECH)) {
		mp_act_trigger(argument, whom, ch, NULL, NULL, TRIG_SPEECH);
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

bool can_talk(CHAR_DATA *ch)
{
	if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
		send_to_char("The gods have revoked your channel priviliges.\n\r", ch);
		return FALSE;
	}

	if (IS_SET(ch->comm, COMM_QUIET)) {
		send_to_char("You must turn off quiet mode first.\n\r", ch);
		return FALSE;
	}

	return TRUE;
}

