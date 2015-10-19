#include "merc.h"
#include "character.h"

bool character_is_blind(struct char_data *ch)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return false;

	return (IS_AFFECTED(ch, AFF_BLIND));
}

bool character_toggle_comm(struct char_data *ch, long commflag)
{
    if (IS_SET(ch->comm, commflag)) {
        REMOVE_BIT(ch->comm, commflag);
        return true;
    } else {
        SET_BIT(ch->comm, commflag);
        return false;
    }
}

bool character_has_comm(struct char_data *ch, long commflag)
{
    return (IS_SET(ch->comm, commflag));
}


