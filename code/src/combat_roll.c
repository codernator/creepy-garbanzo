#include "merc.h"
#include "combat_roll.h"
#include <string.h>


void fill_combat_roll(COMBAT_ROLL_BOX *crb, bool defense, int bonus_die_skill)
{
	int roller, roll, temp;

	memset(crb->rolls, 0, MAX_COMBAT_DICE_POOL * sizeof(int));

	temp = (int)(UMIN(MAX_LEVEL, crb->combatant_level) / 50) + 1;
	crb->significant_dice_count = UMIN(MAX_COMBAT_DICE_POOL, temp);

	temp = crb->significant_dice_count + (crb->significant_dice_count / 2) + (defense ? 2 : 0);
	crb->dice_pool = UMIN(MAX_COMBAT_DICE_POOL, temp);

	crb->die_type = defense ? 115 : 125;

	/* Roll the pool of dice.  Any roll over the weapon skill counts as a "botch". */
	crb->botch_count = 0;
	for (roller = 0; roller < crb->dice_pool; roller++) {
		roll = number_range(1, crb->die_type);
		crb->rolls[roller] = (roll <= crb->weapon_skill) ? roll : 0;
		if (roll >= 105 && roll > crb->weapon_skill)
			crb->botch_count++;
	}

	/* Don't count the botches in the insignificant group of dice. */
	/* Accept the possibility of negative count. */
	crb->botch_count -= (crb->dice_pool - crb->significant_dice_count);

	/* Bubble sort is ok here because we are talking about only a few rolls. */
	i_bubble_sort(crb->rolls, crb->dice_pool);

	/* If bonus skill, roll the dice pool (limited to significant count) and allow the
	 * first success greater than the least roll in the significant pool to replace
	 * the least roll.
	 */
	if (bonus_die_skill > 0) {
		for (roller = 0; roller < crb->significant_dice_count; roller++) {
			roll = number_range(1, crb->die_type);
			if (roll <= bonus_die_skill && roll > crb->rolls[crb->significant_dice_count - 1]) {
				crb->rolls[crb->significant_dice_count - 1] = roll;
				crb->bonus_die_success = TRUE;
				break;
			}
		}
	}

	/* Sum up all the significant rolls. */
	crb->total_roll = 0;
	for (roller = 0; roller < crb->significant_dice_count; roller++)
		crb->total_roll += crb->rolls[roller];

	if (crb->combat_rating > 10000)
		crb->total_roll *= 10;
}
