#include "merc.h"
#include "character.h"
#include <string.h>

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

void character_getdescription(struct char_data *owner, char *target, size_t maxlen)
{
    (void)strncpy(target, owner->description, maxlen);
    return;
}

void character_setdescription(struct char_data *owner, const char *text)
{
    free_string(owner->description);
    owner->description = str_dup(text);
    return;
}

void olc_character_getdescription(void *owner, char *target, size_t maxlen)
{
    character_getdescription((struct char_data *)owner, target, maxlen);
}

void olc_character_setdescription(void *owner, const char *text)
{
    character_setdescription((struct char_data *)owner, text);
}
