/***********************************************************************
 * Unique Object Generator v1.0
 *
 * This file and its contents are copyright 1999 Ward Fisher (except where
 * otherwise noted).  The most recent version can be had by contacting
 * Ward Fisher (Ferric) either through email (ferric#uwyo,edu), or by contact
 * on his current project, MelmothMUD (melmoth.uwyo.edu 9000).
 *
 * If you decide to use this snippet, *please* email me at the above email
 * address.  It goes a long ways towards encouraging future snippets, trust
 * me ;).
 **************************************************************************/

/*
 * Synopsis:  This (and its sister file, unique.h) allow for the on-the-fly
 * creation of a random piece of Unique EQ.  It happens in roughly the
 * following steps.  The object is created, a random mob is selected,
 * the object type is randomly assigned; if weapon, the type of weapon
 * is then assigned; if armor, the type of armor is assigned (via random
 * wear-loc assignment).  Random affects are assigned, strength and number
 * set by the objects level, which is in turn set by the mobile it's being
 * loaded to's level.  The object is named, incorporating several tables
 * of descriptive words, as well as the name of the mobile it's being
 * loaded to.
 *
 * This file also includes a "tally" function, which tallies the number of
 * unique objects in the world currently.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "tables.h"
#include "olc.h"
#include "unique.h"

/***************************************************************************
*	defines
***************************************************************************/
#define MAX_ARMOR_SUFFIX 27
#define MAX_WEAPON_SUFFIX 15
#define MAX_PREFIX 18


extern DECLARE_DO_FUN(do_wear);


CHAR_DATA *random_unique_mob                       args((void));
void format_obj                      args((OBJ_DATA * obj));
extern int wear_bit                        args((int loc));
void name_obj                        args((CHAR_DATA * mob, OBJ_DATA * obj));
void apply_random_affect     args((OBJ_DATA * obj, bool positive));


/***************************************************************************
*	local function declarations
***************************************************************************/
static void format_obj_weapon args((OBJ_DATA * obj));
static void format_obj_armor args((OBJ_DATA * obj));
static char *weapon_type_name args((OBJ_DATA * obj));
static char *armor_type_name args((OBJ_DATA * obj));
static int random_location args((void));



/***************************************************************************
*	overkill stuff for making names
***************************************************************************/
static struct unique_attrib_table unique_table_armor_suffix[] =
{
	{ ""			       },
	{ "distinction"		       },
	{ "protection"		       },
	{ "value"		       },
	{ "power"		       },
	{ "`8dark `1p`!o`1w`!e`1r``"   },
	{ "`#h`7o`&l`#i`7n`&e`#s`7s``" },
	{ "desecration"		       },
	{ "`#h`*o`#p`*e``"	       },
	{ "hopelessness"	       },
	{ "adoration"		       },
	{ "invulnerability"	       },
	{ "Ferric"		       },
	{ "`2A`8n`3a`2r`3c`8h``"       },
	{ "`!A`@F`OK`#Wh`3o`#r`3e``"   },
	{ "Mota"		       },
	{ "`OT`&y`Ol`&o`Or``"	       },
	{ "`6S`^e`&t`^h``"	       },
	{ "`3M`#o`7n`8r`7i`#c`3k``"    },
	{ "`&P`7u`8c`&k``"	       },
	{ "Veles"		       },
	{ "Smoothness"		       },
	{ "`!f`1e`!a`1r``"	       },
	{ "the `!Fa`8ll`1en``"	       },
	{ "the gods"		       },
	{ "Evility"		       },
	{ "`5m`7y`5s`7t`5e`7r`5y``"    },
};

static struct unique_attrib_table unique_table_weapon_suffix[] =
{
	{ ""				  },
	{ "destruction"			  },
	{ "sharpness"			  },
	{ "`^p`6o`*w`6e`^r``"		  },
	{ "`1m`!ai`1m`!i`1ng``"		  },
	{ "`8vi`!o`1l`!e`8nce``"	  },
	{ "`!slaying``"			  },
	{ "havok"			  },
	{ "crushing"			  },
	{ "cutting"			  },
	{ "`!f`1e`!a`1r``"		  },
	{ "pillage"			  },
	{ "wanton destruction"		  },
	{ "`^a`#n`@n`^o`#y`@a`^n`#c`@e``" },
	{ "striking"			  },
};

static struct unique_attrib_table unique_table_prefix[] =
{
	{ ""		      },
	{ "flashing"	      },
	{ "dull"	      },
	{ "well crafted"      },
	{ "finely crafted"    },
	{ "`^s`*h`^i`*n`^y``" },
	{ "fine"	      },
	{ "fantastic"	      },
	{ "well-oiled"	      },
	{ "brilliant"	      },
	{ "hewn"	      },
	{ "scratched"	      },
	{ "dark"	      },
	{ "intense"	      },
	{ "battered"	      },
	{ "polished"	      },
	{ "`&f`Pa`5d`8e`7d``" },
	{ "enigmatic"	      },
};


/***************************************************************************
*	do_cuo
*
*	create a number of unique arguments and randomly put them
*	out in the world
***************************************************************************/
void do_cuo(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	CHAR_DATA *mob;
	int num_affects;
	int number_objects;
	int chance;
	int iter;
	int idx;

	if (argument[0] == '\0' || argument[0] == '?' || !str_prefix(argument, "help")) {
		send_to_char("Syntax: `!CUO `&<`#number of items`&>``\n\r", ch);
		return;
	}

	number_objects = UMAX(1, atoi(argument));
	for (idx = 0; idx < number_objects; idx++) {
		if ((obj = create_object(get_obj_index(OBJ_UNIQUE_DUMMY), 0)) == NULL) {
			send_to_char("Bug with OBJ_UNIQUE_DUMMY\n\r", ch);
			return;
		}

		/*Clean up the object*/
		free_string(obj->short_descr);
		free_string(obj->description);

		mob = random_unique_mob();
		if (mob == NULL || mob->level < 1)
			continue;

		/*All objects should be TAKE*/
		SET_BIT(obj->weight, 70);
		SET_BIT(obj->wear_flags, ITEM_TAKE);

		/*Object level should be mob level.*/
		obj->level = URANGE(100, mob->level / 2, 250);

		/*Lets decide what type of object it is*/
		format_obj(obj);

		/*Now, lets name the obj*/
		name_obj(mob, obj);

		/*Lets apply a good affects*/
		num_affects = ((mob->level) / 80);
		if (num_affects == 0)
			num_affects++;

		for (iter = 0; iter < num_affects; iter++)
			apply_random_affect(obj, TRUE);

		chance = number_percent();


		/*Lets apply bad affects*/
		if (mob->level > 50 && chance <= 95) {
			num_affects = (mob->level - 20) / 100;

			if (num_affects == 0)
				num_affects++;

			for (iter = 0; iter < num_affects; iter++)
				apply_random_affect(obj, FALSE);
		}

		/*Lets give the obj to the mob*/
		obj_to_char(obj, mob);
		do_wear(mob, obj->name);
	}

	printf_to_char(ch, "`#%d`` unique objects randomly distributed.\n\r", number_objects);
	return;
}

/***************************************************************************
*	something else entirely
***************************************************************************
***************************************************************************
*	do_tally
*
*	get a count of unique items in the world still carried by a mob
***************************************************************************/
void do_tally(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	int unique_tally = 0;
	int hidden_tally = 0;
	int imp_tally = 0;
	int odd_tally = 0;

	for (obj = object_list; obj != NULL; obj = obj->next) {
		CHAR_DATA *vch;

		/* if it is carried by nobody or it is carried by a mob */
		if (((vch = obj->carried_by) != NULL && IS_NPC(vch))
		    || (obj->carried_by == NULL && obj->in_obj == NULL)) {
			/* if the item is a unique and it is carried by someone */
			if (IS_SET(obj->extra_flags, ITEM_UNIQUE) && obj->carried_by != NULL) {
				unique_tally++;
				continue;
			}

			if (obj->item_type == ITEM_QTOKEN2) {
				/* we have a hidden token? */
				if (IS_SET(obj->value[0], TOKEN_IMMHIDDEN)) {
					hidden_tally++;
					continue;
				}
				/* we have an imp token? */
				if (IS_SET(obj->value[0], TOKEN_IMP)) {
					imp_tally++;
					continue;
				}
				/* we have some other form of token */
				odd_tally++;
				continue;
			}

			if (obj->item_type == ITEM_QTOKEN)
				odd_tally++;
		}
	}

	/* need to fix colors here */
	printf_to_char(ch, "`gUnique Items:  %d currently in the wilderness.``\n\r", unique_tally);
	printf_to_char(ch, "`gIMP Tokens:    %d currently in the wilderness.``\n\r", imp_tally);
	printf_to_char(ch, "`gHidden Tokens: %d currently in the wilderness.``\n\r", hidden_tally);
	printf_to_char(ch, "`gOther Tokens:  %d currently in the wilderness.``\n\r", odd_tally);
	return;
}



/***************************************************************************
*	do_untally
*
*	remove all of the uniques from the world
***************************************************************************/
void do_untally(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int tally = 0;

	for (obj = object_list; obj != NULL; obj = obj_next) {
		CHAR_DATA *vch;

		obj_next = obj->next;
		if ((vch = obj->carried_by) == NULL)
			continue;

		if (!IS_NPC(vch))
			continue;

		if (IS_SET(obj->extra_flags, ITEM_UNIQUE)) {
			extract_obj(obj);
			tally++;
		}
	}

	printf_to_char(ch, "`gThere were %d unique items extracted.`x\n\r", tally);
	return;
}



/***************************************************************************
*	apply_random_affect
*
*	apply a random affect to an item
***************************************************************************/
void apply_random_affect(OBJ_DATA *obj, bool positive)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_find;
	int location;
	int mult;
	int value;
	int max;
	int min;
	int chance;
	int modifier;

	location = random_location();

	switch (location) {
	default:
		mult = 1;
		break;
	case (APPLY_DAMROLL):
		mult = 2;
		break;
	case (APPLY_HITROLL):
		mult = 2;
		break;
	case (APPLY_HIT):
		mult = 40;
		break;
	case (APPLY_MOVE):
		mult = 40;
		break;
	case (APPLY_MANA):
		mult = 40;
		break;
	case (APPLY_AC):
		mult = 80;
		break;
	}

	chance = number_percent();

	value = (1 + (obj->level / 25)) * mult;
	if (chance > 95)
		value *= 2;

	max = value;
	min = value / 2;

	if (min == 0)
		min++;

	paf = NULL;
	modifier = number_range(min, max);
	/* saving spell and ac are positive affects if the
	 * value is negative */
	if (location == APPLY_SAVING_SPELL || location == APPLY_AC)
		modifier *= -1;

	/* reverse the modifier if the value is negative */
	if (!positive)
		modifier *= -1;

	for (paf_find = obj->affected; paf_find != NULL; paf_find = paf_find->next) {
		if (paf_find->location == location
		    && paf_find->where == TO_OBJECT
		    && paf_find->duration == -1
		    && ((paf_find->modifier > 0 && modifier > 0)
			|| (paf_find->modifier < 0 && modifier < 0))) {
			paf = paf_find;
			break;
		}
	}

	if (paf != NULL) {
		paf->modifier += modifier;
	} else {
		paf = new_affect();

		paf->location = location;
		paf->modifier = modifier;
		paf->where = TO_OBJECT;
		paf->type = location;
		paf->duration = -1;
		paf->bitvector = 0;
		paf->level = obj->level;

		affect_to_obj(obj, paf);
	}
}


/***************************************************************************
*	name_obj
*
*	randomly generate a name for an object
***************************************************************************/
void name_obj(CHAR_DATA *mob, OBJ_DATA *obj)
{
	char buf[MSL];
	char *uncolor_buf;
	int prefix = number_range(1, MAX_PREFIX - 1);
	int weapon_suffix = number_range(1, MAX_WEAPON_SUFFIX - 1);
	int armor_suffix = number_range(1, MAX_ARMOR_SUFFIX - 1);
	char *name;

	name = obj->item_type == ITEM_WEAPON ? weapon_type_name(obj) : armor_type_name(obj);
	/* i dont like color codes in object names */
	sprintf(buf, "unique %s %s %s of %s",
		mob->short_descr,
		unique_table_prefix[prefix].descriptive,
		name,
		obj->item_type == ITEM_WEAPON ?
		unique_table_weapon_suffix[weapon_suffix].descriptive :
		unique_table_armor_suffix[armor_suffix].descriptive);

	/* get rid of the colors */
	uncolor_buf = uncolor_str(buf);

	free_string(obj->name);
	obj->name = str_dup(uncolor_buf);
	free_string(uncolor_buf);

	/* create a short description */
	sprintf(buf, "`8(`&U`7n`8i`7q`&ue`8)`` %s's %s %s of %s",
		mob->short_descr,
		unique_table_prefix[prefix].descriptive,
		name,
		obj->item_type == ITEM_WEAPON ?
		unique_table_weapon_suffix[weapon_suffix].descriptive :
		unique_table_armor_suffix[armor_suffix].descriptive);


	obj->short_descr = str_dup(buf);

	strcat(buf, " is laying on the ground.");
	obj->description = str_dup(buf);
}



/***************************************************************************
*	format_obj_weapon
*
*	format a weapon
***************************************************************************/
static void format_obj_weapon(OBJ_DATA *obj)
{
	int size;
	int dice;
	int avg;


	if (obj->level > 0) {
		obj->value[0] = number_range(0, 8);


		SET_BIT(obj->wear_flags, ITEM_WIELD);


		avg = obj->level + 30;
		dice = (obj->level / 10 + 1);
		size = dice / 2;

		for (size = dice / 2; dice * (size + 2) / 2 < avg; size++) {
		}

		dice = UMAX(1, dice);
		size = UMAX(2, size);

		obj->value[1] = dice;
		obj->value[2] = size;
	}
}



/***************************************************************************
*	weapon_type_name
*
*	get a name for a weapon
***************************************************************************/
static char *weapon_type_name(OBJ_DATA *obj)
{
	int idx;

	for (idx = 0; weapon_class[idx].name != NULL; idx++)
		if (weapon_class[idx].bit == obj->value[0])
			return weapon_class[idx].name;

	return "Exotic";
}


/***************************************************************************
*	named_armor_types
*
*	a list of amor types used during the string of the object
***************************************************************************/
static const struct armor_type_name_definition {
	int	type;
	char *	names[3];
} named_armor_types[] =
{
	{ WEAR_LIGHT,	  { "light",	  "light",	  "light"	} },
	{ WEAR_FINGER_L,  { "ring",	  "ring",	  "ring"	} },
	{ WEAR_FINGER_R,  { "ring",	  "ring",	  "ring"	} },
	{ WEAR_FINGER_L2, { "ring",	  "ring",	  "ring"	} },
	{ WEAR_FINGER_R2, { "ring",	  "ring",	  "ring"	} },
	{ WEAR_EAR_L,	  { "earring",	  "earring",	  "earring"	} },
	{ WEAR_EAR_R,	  { "earring",	  "earring",	  "earring"	} },
	{ WEAR_FACE,	  { "veil",	  "veil",	  "veil"	} },
	{ WEAR_NECK_1,	  { "necklace",	  "pendant",	  "neck guard"	} },
	{ WEAR_NECK_2,	  { "necklace",	  "pendant",	  "neck guard"	} },
	{ WEAR_BODY,	  { "armor",	  "breast plate", "breastplate" } },
	{ WEAR_HEAD,	  { "skullcap",	  "helmet",	  "helm",	} },
	{ WEAR_LEGS,	  { "leggings",	  "leg plates",	  "pants"	} },
	{ WEAR_FEET,	  { "sandals",	  "boots",	  "clogs"	} },
	{ WEAR_HANDS,	  { "gloves",	  "gauntlets",	  "sap gloves"	} },
	{ WEAR_ARMS,	  { "arm plates", "arm plates",	  "arm plates"	} },
	{ WEAR_SHIELD,	  { "shield",	  "shield",	  "shield"	} },
	{ WEAR_ABOUT,	  { "cloak",	  "cape",	  "coat"	} },
	{ WEAR_WAIST,	  { "belt",	  "girdle",	  "chain"	} },
	{ WEAR_WRIST_L,	  { "bracelet",	  "wrist band",	  "band"	} },
	{ WEAR_WRIST_R,	  { "bracelet",	  "wrist band",	  "band"	} },
	{ WEAR_FLOAT,	  { "float",	  "aura",	  "feelings"	} },
	{ WEAR_TATTOO,	  { "tattoo",	  "tattoo",	  "tattoo"	} },
	{ -1,		  { "",		  "",		  ""		} }
};

/***************************************************************************
*	armor_type_name
*
*	get a random armor name from the above ugly chart
***************************************************************************/
static char *armor_type_name(OBJ_DATA *obj)
{
	int idx;

	for (idx = 0; named_armor_types[idx].type != -1; idx++) {
		if (obj->wear_loc == named_armor_types[idx].type) {
			int random = number_range(0, 2);

			if (named_armor_types[idx].names[random]
			    && named_armor_types[idx].names[random][0] != '\0')
				return named_armor_types[idx].names[random];
		}
	}

	return "Unknown";
}


/***************************************************************************
*	format_obj_armor
*
*	format an armor object
***************************************************************************/
static void format_obj_armor(OBJ_DATA *obj)
{
	int wear_loc;
	bool set;

	set = FALSE;
	while (!set) {
		wear_loc = number_range(0, MAX_WEAR);

		switch (wear_loc) {
		case WEAR_LIGHT:
			obj->item_type = ITEM_LIGHT;
			obj->value[2] = -1;
			set = TRUE;
			break;
		case WEAR_FINGER_L:
		case WEAR_FINGER_R:
		case WEAR_FINGER_L2:
		case WEAR_FINGER_R2:
			SET_BIT(obj->wear_flags, ITEM_WEAR_FINGER);
			set = TRUE;
			break;
		case WEAR_NECK_1:
		case WEAR_NECK_2:
			SET_BIT(obj->wear_flags, ITEM_WEAR_NECK);
			set = TRUE;
			break;
		case WEAR_BODY:
			SET_BIT(obj->wear_flags, ITEM_WEAR_BODY);
			set = TRUE;
			break;
		case WEAR_HEAD:
			SET_BIT(obj->wear_flags, ITEM_WEAR_HEAD);
			set = TRUE;
			break;
		case WEAR_FACE:
			SET_BIT(obj->wear_flags, ITEM_WEAR_FACE);
			set = TRUE;
			break;
		case WEAR_TATTOO:
			SET_BIT(obj->wear_flags, ITEM_WEAR_TATTOO);
			set = TRUE;
			break;
		case WEAR_EAR_L:
		case WEAR_EAR_R:
			SET_BIT(obj->wear_flags, ITEM_WEAR_EAR);
			set = TRUE;
			break;
		case WEAR_LEGS:
			SET_BIT(obj->wear_flags, ITEM_WEAR_LEGS);
			set = TRUE;
			break;
		case WEAR_FEET:
			SET_BIT(obj->wear_flags, ITEM_WEAR_FEET);
			set = TRUE;
			break;
		case WEAR_HANDS:
			SET_BIT(obj->wear_flags, ITEM_WEAR_HANDS);
			set = TRUE;
			break;
		case WEAR_ARMS:
			SET_BIT(obj->wear_flags, ITEM_WEAR_ARMS);
			set = TRUE;
			break;
		case WEAR_SHIELD:
			SET_BIT(obj->wear_flags, ITEM_WEAR_SHIELD);
			set = TRUE;
			break;
		case WEAR_ABOUT:
			SET_BIT(obj->wear_flags, ITEM_WEAR_ABOUT);
			set = TRUE;
			break;
		case WEAR_WAIST:
			SET_BIT(obj->wear_flags, ITEM_WEAR_WAIST);
			set = TRUE;
			break;
		case WEAR_WRIST_R:
		case WEAR_WRIST_L:
			SET_BIT(obj->wear_flags, ITEM_WEAR_WRIST);
			set = TRUE;
			break;
		case WEAR_FLOAT:
			SET_BIT(obj->wear_flags, ITEM_WEAR_FLOAT);
			wear_loc = WEAR_FLOAT;
			set = TRUE;
			break;
		}

		if (set)
			obj->wear_loc = wear_loc;
	}

	/* set the armor values */
	if (obj->item_type != ITEM_LIGHT) {
		obj->value[0] = number_range((obj->level * 2) - 1, (obj->level * 2) + 2);
		obj->value[1] = number_range((obj->level * 2) - 1, (obj->level * 2) + 2);
		obj->value[2] = number_range((obj->level * 2) - 1, (obj->level * 2) + 2);
		obj->value[3] = number_range((obj->level * 2) - 1, (obj->level * 2) + 2);
	}
}


/***************************************************************************
*	format_obj
*
*	choose a random type for the object and format it
*	appropriately
***************************************************************************/
void format_obj(OBJ_DATA *obj)
{
	int random;

	random = number_range(0, 9);
	switch (random) {
	case 0:
		obj->item_type = ITEM_WEAPON;
		format_obj_weapon(obj);
		break;
	default:
		obj->item_type = ITEM_ARMOR;
		format_obj_armor(obj);
		break;
	}
}

/***************************************************************************
*	random_unique_mob
*
*	pick a random mob from the world
***************************************************************************/
CHAR_DATA *random_unique_mob()
{
	CHAR_DATA *mob = NULL;
	CHAR_DATA *vch;
	int chance;
	int num_found;

	num_found = 0;
	for (vch = char_list; vch != NULL; vch = vch->next) {
		if (!IS_NPC(vch)
		    || vch->name == NULL
		    || vch->short_descr == NULL
		    || vch->in_room == NULL
		    || IS_SHOPKEEPER(vch)
		    || IS_TRAINER(vch)
		    || IS_HEALER(vch)
		    || vch->level < 100)
			continue;


		chance = number_range(0, num_found);
		if (chance == 0
		    || (mob != NULL
			&& num_found > 4
			&& chance < UMIN(10, (num_found / 4))
			&& mob->level < vch->level))
			mob = vch;

		if (++num_found >= 5000)
			break;
	}

	return mob;
}


/***************************************************************************
*	random_location
*
*	pick a random location
***************************************************************************/
static int random_location()
{
	int random;
	int location = APPLY_NONE;

	random = number_range(1, 14);
	switch (random) {
	case 1:
	case 2:
	case 3:
		random = number_range(1, 5);
		switch (random) {
		case 1:
			location = APPLY_STR;
			break;
		case 2:
			location = APPLY_DEX;
			break;
		case 3:
			location = APPLY_INT;
			break;
		case 4:
			location = APPLY_WIS;
			break;
		case 5:
			location = APPLY_CON;
			break;
		}
		break;
	case 4:
	case 5:
		location = APPLY_MANA;
		break;
	case 6:
	case 7:
		location = APPLY_HIT;
		break;
	case 8:
		location = APPLY_MOVE;
		break;
	case 9:
	case 10:
		location = APPLY_HITROLL;
		break;
	case 11:
	case 12:
		location = APPLY_DAMROLL;
		break;
	case 13:
		location = APPLY_SAVING_SPELL;
		break;
	case 14:
		location = APPLY_AC;
		break;
	}
	return location;
}
