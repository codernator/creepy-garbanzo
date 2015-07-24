/****************************************************************************
*  Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,          *
*  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.    *
*                                                                          *
*  Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael            *
*  Chastain, Michael Quan, and Mitchell Tse.                               *
*                                                                          *
*  In order to use any part of this Merc Diku Mud, you must comply with    *
*  both the original Diku license in 'license.doc' as well the Merc        *
*  license in 'license.txt'.  In particular, you may not remove either of  *
*  these copyright notices.                                                *
*                                                                          *
*  Much time and thought has gone into this software and you are           *
*  benefitting.  We hope that you share your changes too.  What goes       *
*  around, comes around.                                                   *
****************************************************************************/

/****************************************************************************
 *       ROM 2.4 is copyright 1993-1995 Russ Taylor                          *
 *       ROM has been brought to you by the ROM consortium                   *
 *           Russ Taylor(rtaylor@pacinfo.com)                                *
 *           Gabrielle Taylor(gtaylor@pacinfo.com)                           *
 *           Brian Moore(rom@rom.efn.org)                                    *
 *       By using this code, you have agreed to follow the terms of the      *
 *       ROM license, in the file Rom24/doc/rom.license                      *
 *****************************************************************************/

/*
 * Careful with this value.  24 dice in a pool is pretty ridiculous.
 * Increase this too much and you'll have a bubble-sort out of control
 * on your hands... (See fill_combat_roll in combat_roll.c)
 */
#define MAX_COMBAT_DICE_POOL 24

typedef struct combat_roll_box {
	SKILL * special_attack;
	int	weapon_sn;
	int	weapon_skill;
	int	combatant_level;
	int	combat_rating;
	int	significant_dice_count;
	int	dice_pool;
	int	die_type;
	int	rolls[MAX_COMBAT_DICE_POOL];
	int	botch_count;
	int	total_roll;
	bool	bonus_die_success;
} COMBAT_ROLL_BOX;
