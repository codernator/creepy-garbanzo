#include "merc.h"
#include "character.h"

bool character_is_blind(CHAR_DATA *ch)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return false;

	return (IS_AFFECTED(ch, AFF_BLIND));
}

bool character_toggle_comm(CHAR_DATA *ch, long commflag, int location) 
{
    switch(location) {
        case 2:
            if (IS_SET(ch->comm2, commflag)) {
                REMOVE_BIT(ch->comm2, commflag);
                return true;
            } else {
                SET_BIT(ch->comm2, commflag);
                return false;
            }
            break;
        default:
            if (IS_SET(ch->comm, commflag)) {
                REMOVE_BIT(ch->comm, commflag);
                return true;
            } else {
                SET_BIT(ch->comm, commflag);
                return false;
            }
            break;
    }
}

bool character_has_comm(CHAR_DATA *ch, long commflag, int location) 
{
    switch(location) {
        case 2:
            return (IS_SET(ch->comm2, commflag)); 
        default:
            return (IS_SET(ch->comm, commflag)); 
    }
}


