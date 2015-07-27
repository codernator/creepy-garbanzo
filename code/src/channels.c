#include "merc.h"
#include "channels.h"

#include <stdio.h>

extern bool can_talk(CHAR_DATA *ch);


void broadcast_auctalk(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;

    if (!can_talk(ch))
        return;
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
}

void broadcast_wish(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	char buf[2*MIL];


	if (!can_talk(ch))
		return;

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

}


bool can_talk(CHAR_DATA *ch)
{
	if (IS_SET(ch->comm, COMM_QUIET)) {
		send_to_char("You must turn off quiet mode first.\n\r", ch);
		return FALSE;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
		send_to_char("The gods have revoked your channel priviliges.\n\r", ch);
		return FALSE;
	}

	if ((ch->in_room->vnum > 20924)
	    && (ch->in_room->vnum < 20930)
	    && (ch->level < LEVEL_IMMORTAL)) {
		send_to_char("Not in jail....\n\r", ch);
		return FALSE;
	}


	return TRUE;
}

