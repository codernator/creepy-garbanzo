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

void fill_combat_roll(COMBAT_ROLL_BOX *crb, bool defense, int bonus_die_skill);
