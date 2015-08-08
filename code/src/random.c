#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "unique.h"
#include "tables.h"
#include "olc.h"


/*
 *      NON-RELATED TODO:
 *              scavenger flag
 *
 */


/***************************************************************************
*	random functions
***************************************************************************/
typedef void RANDOM_FN(long vnum);

static RANDOM_FN random_drop_item;
static RANDOM_FN random_drop_gold;
static RANDOM_FN random_drop_unique;

static ROOM_INDEX_DATA *random_drop_room(void);
CHAR_DATA *random_unique_mob(void);
void format_obj(OBJ_DATA * obj);
void name_obj(CHAR_DATA * mob, OBJ_DATA * obj);
void apply_random_affect(OBJ_DATA * obj, bool positive);

/***************************************************************************
*	random drop table
***************************************************************************/
static const struct random_drop {
	int		vnum;
	int		start_range;
	int		end_range;
	RANDOM_FN *	fn;
}
random_drop_table[] =
{
	{ 30140, 1,    9,    random_drop_item	},                      /* A [1] */
	{ 30141, 10,   11,   random_drop_item	},                      /* B [3] */
	{ 30142, 12,   13,   random_drop_item	},                      /* C [3] */
	{ 30143, 14,   17,   random_drop_item	},                      /* D [2] */
	{ 30144, 18,   29,   random_drop_item	},                      /* E [1] */
	{ 30145, 30,   31,   random_drop_item	},                      /* F [4] */
	{ 30146, 32,   34,   random_drop_item	},                      /* G [2] */
	{ 30147, 35,   36,   random_drop_item	},                      /* H [4] */
	{ 30148, 37,   45,   random_drop_item	},                      /* I [1] */
	{ 30149, 46,   46,   random_drop_item	},                      /* J [8] */
	{ 30150, 47,   47,   random_drop_item	},                      /* K [5] */
	{ 30151, 48,   51,   random_drop_item	},                      /* L [1] */
	{ 30152, 52,   53,   random_drop_item	},                      /* M [3] */
	{ 30153, 54,   59,   random_drop_item	},                      /* N [1] */
	{ 30154, 60,   67,   random_drop_item	},                      /* O [1] */
	{ 30155, 68,   69,   random_drop_item	},                      /* P [3] */
	{ 30156, 70,   70,   random_drop_item	},                      /* Q [10] */
	{ 30157, 71,   76,   random_drop_item	},                      /* R [1] */
	{ 30158, 77,   80,   random_drop_item	},                      /* S [1] */
	{ 30159, 81,   86,   random_drop_item	},                      /* T [1] */
	{ 30160, 87,   90,   random_drop_item	},                      /* U [1] */
	{ 30161, 91,   92,   random_drop_item	},                      /* V [4] */
	{ 30162, 93,   94,   random_drop_item	},                      /* W [4] */
	{ 30163, 95,   95,   random_drop_item	},                      /* X [8] */
	{ 30164, 96,   97,   random_drop_item	},                      /* Y [4] */
	{ 30165, 98,   98,   random_drop_item	},                      /* Z [10] */
	{ 30166, 99,   99,   random_drop_item	},                      /* Triple Word Score */
	{ 30167, 100,  101,  random_drop_item	},                      /* Double Word Score */
	{ 30168, 102,  103,  random_drop_item	},                      /* Triple Letter Score */
	{ 30169, 104,  105,  random_drop_item	},                      /* Double Letter Score */
	{ 30170, 106,  107,  random_drop_item	},                      /* Blank [0] */
	{ 1,	 108,  1500, random_drop_gold	},                      /* GOLD */
	{ 30171, 1501, 1900, random_drop_item	},                      /* Hidden Token */
	{ 30172, 1901, 2700, random_drop_item	},                      /* 25 QP Token */
	{ 30173, 2701, 2800, random_drop_item	},                      /* Restring */
	{ 30174, 2801, 3001, random_drop_item	},                      /* Wild */
	{ 30175, 3001, 3100, random_drop_item	},                      /* Train */
	{ 30176, 3101, 3200, random_drop_item	},                      /* IMP */
	{ 1,	 3201, 8500, random_drop_unique },                      /* Unique */
	{ -1,	 -1,   -1,   NULL		}
};


/***************************************************************************
*	random_drop
*
*	pick a random item from the list and drop it
***************************************************************************/
void random_drop()
{
	int rand_num;
	int upper;
	int idx;

	/* 1 in 10 chance to drop a random item */
	rand_num = number_range(1, 5);
	if (rand_num > 1)
		return;

	upper = 0;
	/* dynamically caclulate the upper range - allows me to
	 * just add items to the table without changing this function */
	for (idx = 0; random_drop_table[idx].vnum > 0; idx++)
		if (random_drop_table[idx].end_range > upper)
			upper = random_drop_table[idx].end_range;

	rand_num = number_range(1, upper);

	/* now find the random item to drop */
	for (idx = 0; random_drop_table[idx].vnum > 0; idx++) {
		if (random_drop_table[idx].start_range <= rand_num
		    && random_drop_table[idx].end_range >= rand_num) {
			/* we have our random item - call the function passing
			 * in the vnum */
			(*random_drop_table[idx].fn)(random_drop_table[idx].vnum);
			break;
		}
	}
}


/***************************************************************************
*	random_drop_room
*
*	find a random room to drop an item in
*	differs from get_random_room in that it
*		a) doesnt require a character
*		b) doesnt do any kind of player-based sanity checks
***************************************************************************/
static ROOM_INDEX_DATA *random_drop_room()
{
	ROOM_INDEX_DATA *to_room;

	to_room = NULL;
	while (to_room == NULL) {
		/* loop through here until we have a room */
		if ((to_room = get_room_index(number_range(0, 65535))) != NULL)
			break;
	}

	return to_room;
}

/***************************************************************************
*	random_drop_item
*
*	drop a random item in the world
***************************************************************************/
static void random_drop_item(long vnum)
{
	ROOM_INDEX_DATA *to_room;

	to_room = random_drop_room();
	if (to_room != NULL) {
		OBJ_INDEX_DATA *obj_idx;
		OBJ_DATA *new_obj;

		if ((obj_idx = get_obj_index(vnum)) != NULL) {
			new_obj = create_object(obj_idx, obj_idx->level);

			obj_to_room(new_obj, to_room);
		}
	}
}

/***************************************************************************
*	random_drop_gold
*
*	drop some random gold
***************************************************************************/
static void random_drop_gold(long vnum)
{
	ROOM_INDEX_DATA *to_room;
	OBJ_DATA *coins;
	unsigned int gold;
	unsigned int silver;

	/* get a random gold and silver range */
	silver = (unsigned int)number_range(30000, 100000);
	gold = (unsigned int)number_range(30000, 100000);

	/* create some money */
	coins = create_money(gold, silver);

	/* find a room */
	to_room = random_drop_room();

	/* drop the coins */
	if (to_room != NULL && coins != NULL)
		obj_to_room(coins, to_room);
}


/***************************************************************************
*	random_drop_unique
*
*	drop a random unique
***************************************************************************/
static void random_drop_unique(long vnum)
{
	OBJ_DATA *unique;
	CHAR_DATA *to_mob;
	int idx;
	int num_affects;
	int chance;

	if ((unique = create_object(get_obj_index(OBJ_UNIQUE_DUMMY), 0)) == NULL)
		return;

	if ((number_range(1, 100) > 25))
		return;

	/*Clean up the object*/
	free_string(unique->short_descr);
	free_string(unique->description);

	to_mob = NULL;
	/*	get a random mob - select 3, take the highest level mob */
	for (idx = 0; idx < 3; idx++) {
		CHAR_DATA *tmp_mob;

		tmp_mob = random_unique_mob();

		if (tmp_mob != NULL) {
			if (to_mob == NULL
			    || (to_mob != NULL && to_mob->level < tmp_mob->level))
				to_mob = tmp_mob;
		}
	}

	if (to_mob == NULL || to_mob->level < 1)
		return;

	/* make sure the item is ITEM_TAKE */
	SET_BIT(unique->wear_flags, ITEM_TAKE);

	/* set the object level */
	unique->level = URANGE(100, to_mob->level / 2, 250);

	/* format the object */
	format_obj(unique);

	/* name the object */
	name_obj(to_mob, unique);

	/* these have a few extra affects on them */
	num_affects = ((to_mob->level) / 60);
	if (num_affects == 0)
		num_affects++;

	/* apply positive affects */
	for (idx = 0; idx < num_affects; idx++)
		apply_random_affect(unique, true);


	/* apply negative affects */
	chance = number_percent();

	if (to_mob->level > 125 && chance <= 92) {
		num_affects = (to_mob->level - 20) / 100;

		if (num_affects == 0)
			num_affects++;

		for (idx = 0; idx < num_affects; idx++)
			apply_random_affect(unique, false);
	}

	/* give the object to the mob */
	obj_to_char(unique, to_mob);
	do_wear(to_mob, unique->name);
}
