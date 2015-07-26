/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                          *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                          *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor(rtaylor@hypercube.org)                                *
 *       Gabrielle Taylor(gtaylor@hypercube.org)                           *
 *       Brian Moore(zump@rom.org)                                         *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 ***************************************************************************/

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
