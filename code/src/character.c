#include "merc.h"
#include "character.h"

bool character_is_blind(CHAR_DATA *ch)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return false;

	return (IS_AFFECTED(ch, AFF_BLIND));
}

bool character_toggle_comm(CHAR_DATA *ch, long commflag)
{
    if (IS_SET(ch->comm, commflag)) {
        REMOVE_BIT(ch->comm, commflag);
        return true;
    } else {
        SET_BIT(ch->comm, commflag);
        return false;
    }
}

bool character_has_comm(CHAR_DATA *ch, long commflag)
{
    return (IS_SET(ch->comm, commflag));
}


