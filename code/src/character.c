#include "merc.h"
#include "character.h"

bool character_is_blind(CHAR_DATA *ch)
{
	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
		return false;

	return (IS_AFFECTED(ch, AFF_BLIND));
}
