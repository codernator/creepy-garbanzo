#include "merc.h"
#include "olc.h"



/***************************************************************************
* PUBLIC INTERFACE
***************************************************************************/
void mob_auto_hit_dice(MOB_INDEX_DATA *mix, enum medit_auto_config_type auto_config_type);



/***************************************************************************
* IMPLEMENTATION
***************************************************************************/
void mob_auto_hit_dice(MOB_INDEX_DATA *mix, enum medit_auto_config_type auto_config_type)
{
	switch (auto_config_type) {
	case mact_easy:
		mix->hit[DICE_NUMBER] = (int)(3 * mix->level / 5);
		mix->hit[DICE_TYPE] = (int)(mix->level / 10);
		mix->hit[DICE_BONUS] = (int)(mix->level);
		break;
	case mact_normal:
		mix->hit[DICE_NUMBER] = (int)(4 * mix->level / 5);
		mix->hit[DICE_TYPE] = (int)(mix->level / 10) + 5;
		mix->hit[DICE_BONUS] = (int)(5 * mix->level);
		break;
	case mact_hard:
		mix->hit[DICE_NUMBER] = (int)(6 * mix->level / 5);
		mix->hit[DICE_TYPE] = (int)(mix->level / 25) + 10;
		mix->hit[DICE_BONUS] = (int)(8 * mix->level);
		break;
	case mact_insane:
		mix->hit[DICE_NUMBER] = (int)(9 * mix->level / 5);
		mix->hit[DICE_TYPE] = (int)(mix->level / 25);
		mix->hit[DICE_BONUS] = (int)(15 * mix->level);
		break;
	}
}
