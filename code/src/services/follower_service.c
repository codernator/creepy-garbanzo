#include "merc.h"


void add_follower(CHAR_DATA *ch, CHAR_DATA *master)
{
	if (ch->master != NULL) {
		bug("Add_follower: non-null master.", 0);
		return;
	}

	ch->master = master;
	ch->leader = NULL;

	if (can_see(master, ch))
		act("$n now follows you.", ch, NULL, master, TO_VICT);

	act("You now follow $N.", ch, NULL, master, TO_CHAR);
}

void die_follower(CHAR_DATA *ch)
{
	CHAR_DATA *fch;

	if (ch->master != NULL) {
		if (ch->master->pet == ch)
			ch->master->pet = NULL;
		stop_follower(ch);
	}

	ch->leader = NULL;

	for (fch = char_list; fch != NULL; fch = fch->next) {
		if (fch->master == ch)
			stop_follower(fch);
		if (fch->leader == ch)
			fch->leader = fch;
	}
}

/*
 * It is very important that this be an equivalence relation:
 *(1) A ~ A
 *(2) if A ~ B then B ~ A
 *(3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch)
{
	if (ach == NULL || bch == NULL)
		return false;

	if (IS_NPC(ach) && IS_NPC(bch)) {
		if (ach->group == bch->group)
			return true;
	}

	if (ach->leader != NULL)
		ach = ach->leader;
	if (bch->leader != NULL)
		bch = bch->leader;

	return ach == bch;
}

// nukes charmed monsters and pets
void nuke_pets(CHAR_DATA *ch)
{
	CHAR_DATA *pet;

	if ((pet = ch->pet) != NULL) {
		stop_follower(pet);
		if (pet->in_room != NULL) {
			act("$N disappears in a puff of faerie dust.", ch, NULL, pet, TO_NOTVICT);
        }

		extract_char(pet, true);
	}
	ch->pet = NULL;
}

void stop_follower(CHAR_DATA *ch)
{
	if (ch->master == NULL) {
		bug("Stop_follower: null master.", 0);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		REMOVE_BIT(ch->affected_by, AFF_CHARM);
		affect_strip(ch, skill_lookup("charm person"));
	}

	if (can_see(ch->master, ch) && ch->in_room != NULL) {
		act("$n stops following you.", ch, NULL, ch->master, TO_VICT);
		act("You stop following $N.", ch, NULL, ch->master, TO_CHAR);
	}

	if (ch->master->pet == ch)
		ch->master->pet = NULL;

	ch->master = NULL;
	ch->leader = NULL;
}
