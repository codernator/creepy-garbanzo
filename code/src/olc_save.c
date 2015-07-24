/**************************************************************************
 *  File: olc_save.c                                                       *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/
/* OLC_SAVE.C
 * This takes care of saving all the .are information.
 * Notes:
 * -If a good syntax checker is used for setting vnum ranges of areas
 *  then it would become possible to just cycle through vnums instead
 *  of using the hash_idx stuff and checking that the room or reset or
 *  mob etc is part of that area.
 */


/***************************************************************************
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"

#define DIF(a, b)(~((~a) | (b)))

/*
 *  Verbose writes reset data in plain english into the comments
 *  section of the resets.  It makes areas considerably larger but
 *  may aid in debugging.
 */

extern int max_social;
extern void bug_long(const char *str, long param);
static void save_area           args((AREA_DATA * area));

/***************************************************************************
*	fix_string
*
*	returns a string without \r and ~
***************************************************************************/
static char *fix_string(const char *str)
{
	static char strfix[MSL * 2];
	int idx;
	int pos;

	if (str == NULL) {
		strfix[0] = '\0';
		return strfix;
	}

	for (pos = idx = 0; str[idx + pos] != '\0'; idx++) {
		if (str[idx + pos] == '\r' || str[idx + pos] == '~')
			pos++;
		strfix[idx] = str[idx + pos];
	}

	strfix[idx] = '\0';
	return strfix;
}



/***************************************************************************
*	save_area_list
*
*	save the list of areas to the startup list
***************************************************************************/
static void save_area_list()
{
	extern HELP_AREA *had_list;
	FILE *fp;
	AREA_DATA *area;
	HELP_AREA *ha;

	if ((fp = fopen(AREA_LIST, "w")) == NULL) {
		bug("Save_area_list: fopen", 0);
		perror("area.lst");
	} else {
		/*
		 * Add any help files that need to be loaded at
		 * startup to this section.
		 */
		for (ha = had_list; ha; ha = ha->next)
			if (ha->area == NULL)
				fprintf(fp, "%s\n", ha->filename);

		for (area = area_first; area; area = area->next)
			fprintf(fp, "%s\n", area->file_name);

		fprintf(fp, "$\n");
		fclose(fp);
	}

	return;
}


/***************************************************************************
*	fwrite_flag
*
*	writes flags in the format fread_flag reads
***************************************************************************/
static char *fwrite_flag(long flags, char buf[])
{
	unsigned int offset;
	char *cp;

	buf[0] = '\0';
	if (flags == 0) {
		strcpy(buf, "0");
		return buf;
	}

	/* 32 -- number of bits in a long */
	for (offset = 0, cp = buf; offset < 32; offset++) {
		if (flags & (1u << offset)) {
			if (offset <= (unsigned int)('Z' - 'A'))
				*(cp++) = 'A' + (char)offset;
			else
				*(cp++) = 'a' + (char)(offset - ((unsigned int)('Z' - 'A') + 1));
		}
	}

	*cp = '\0';

	return buf;
}


/***************************************************************************
*	save_mobprogs
*
*	save mob programs for an area
***************************************************************************/
static void save_mobprogs(FILE *fp, AREA_DATA *area)
{
	MPROG_CODE *mprog;
	long iter;

	fprintf(fp, "#PROGRAMS\n");

	for (iter = area->min_vnum; iter <= area->max_vnum; iter++) {
		if ((mprog = get_mprog_index(iter)) != NULL) {
			fprintf(fp, "#%ld\n", iter);
			if (mprog->comment[0] != '\0')
				fprintf(fp, "Comment %s~\n", mprog->comment);
			fprintf(fp, "Code\n%s~\n\n", fix_string(mprog->code));
		}
	}

	fprintf(fp, "#0\n\n");
	return;
}


/***************************************************************************
*	save_mobile
*
*	save a single mobile
***************************************************************************/
static void save_mobile(FILE *fp, MOB_INDEX_DATA *mob_idx)
{
	MPROG_LIST *mprog;
	int race = mob_idx->race;
	char buf[MSL];
	long temp;

	fprintf(fp, "#%ld\n", mob_idx->vnum);
	fprintf(fp, "%s~\n", mob_idx->player_name);
	fprintf(fp, "%s~\n", mob_idx->short_descr);
	fprintf(fp, "%s~\n", fix_string(mob_idx->long_descr));
	fprintf(fp, "%s~\n", fix_string(mob_idx->description));
	fprintf(fp, "%s~\n", race_table[race].name);
	fprintf(fp, "%s ", fwrite_flag(mob_idx->act, buf));
	fprintf(fp, "%s ", fwrite_flag(mob_idx->affected_by, buf));
	fprintf(fp, "%d %ld\n", mob_idx->alignment, mob_idx->group);
	fprintf(fp, "%d ", mob_idx->level);
	fprintf(fp, "%d ", mob_idx->hitroll);
	fprintf(fp, "%dd%d+%d ", mob_idx->hit[DICE_NUMBER],
		mob_idx->hit[DICE_TYPE],
		mob_idx->hit[DICE_BONUS]);
	fprintf(fp, "%dd%d+%d ", mob_idx->mana[DICE_NUMBER],
		mob_idx->mana[DICE_TYPE],
		mob_idx->mana[DICE_BONUS]);
	fprintf(fp, "%dd%d+%d ", mob_idx->damage[DICE_NUMBER],
		mob_idx->damage[DICE_TYPE],
		mob_idx->damage[DICE_BONUS]);
	fprintf(fp, "'%s'\n", attack_table[mob_idx->dam_type].name);
	fprintf(fp, "%ld %ld %ld %ld\n",
		mob_idx->ac[AC_PIERCE] / 10,
		mob_idx->ac[AC_BASH] / 10,
		mob_idx->ac[AC_SLASH] / 10,
		mob_idx->ac[AC_EXOTIC] / 10);
	fprintf(fp, "%s ", fwrite_flag(mob_idx->off_flags, buf));
	fprintf(fp, "%s ", fwrite_flag(mob_idx->imm_flags, buf));
	fprintf(fp, "%s ", fwrite_flag(mob_idx->res_flags, buf));
	fprintf(fp, "%s\n", fwrite_flag(mob_idx->vuln_flags, buf));
	fprintf(fp, "%s %s %s %u\n",
		position_table[mob_idx->start_pos].short_name,
		position_table[mob_idx->default_pos].short_name,
		(mob_idx->sex > 0 && sex_table[mob_idx->sex].name != NULL) ? sex_table[mob_idx->sex].name : "either",
		mob_idx->wealth);
	fprintf(fp, "%s ", fwrite_flag(mob_idx->form, buf));
	fprintf(fp, "%s ", fwrite_flag(mob_idx->parts, buf));

	fprintf(fp, "%s ", size_table[mob_idx->size].name);
	fprintf(fp, "%s\n", IS_NULLSTR(mob_idx->material) ? mob_idx->material : "unknown");

	if ((temp = DIF(race_table[race].act, mob_idx->act)))
		fprintf(fp, "F act %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].aff, mob_idx->affected_by)))
		fprintf(fp, "F aff %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].off, mob_idx->off_flags)))
		fprintf(fp, "F off %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].imm, mob_idx->imm_flags)))
		fprintf(fp, "F imm %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].res, mob_idx->res_flags)))
		fprintf(fp, "F res %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].vuln, mob_idx->vuln_flags)))
		fprintf(fp, "F vul %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].form, mob_idx->form)))
		fprintf(fp, "F for %s\n", fwrite_flag(temp, buf));

	if ((temp = DIF(race_table[race].parts, mob_idx->parts)))
		fprintf(fp, "F par %s\n", fwrite_flag(temp, buf));

	for (mprog = mob_idx->mprogs; mprog; mprog = mprog->next) {
		fprintf(fp, "M %s %ld %s~\n",
			mprog_type_to_name(mprog->trig_type),
			mprog->vnum,
			mprog->trig_phrase);
	}

	return;
}



/***************************************************************************
*	save_mobiles
*
*	save all of the mobiles in an area
***************************************************************************/
static void save_mobiles(FILE *fp, AREA_DATA *area)
{
	MOB_INDEX_DATA *pMob;
	long iter;

	fprintf(fp, "#MOBILES\n");

	for (iter = area->min_vnum; iter <= area->max_vnum; iter++)
		if ((pMob = get_mob_index(iter)))
			save_mobile(fp, pMob);

	fprintf(fp, "#0\n\n\n\n");
	return;
}





/***************************************************************************
*	save_object
*
*	save a single object
***************************************************************************/
static void save_object(FILE *fp, OBJ_INDEX_DATA *pObjIndex)
{
	AFFECT_DATA *pAf;
	EXTRA_DESCR_DATA *extra;
	char letter;
	char buf[MSL];

	fprintf(fp, "#%ld\n", pObjIndex->vnum);
	fprintf(fp, "%s~\n", pObjIndex->name);
	fprintf(fp, "%s~\n", pObjIndex->short_descr);
	fprintf(fp, "%s~\n", fix_string(pObjIndex->description));
	fprintf(fp, "%s~\n", pObjIndex->material);
	fprintf(fp, "%s ", fwrite_flag(pObjIndex->extra2_flags, buf));
	fprintf(fp, "%s ", item_name(pObjIndex->item_type));
	fprintf(fp, "%s ", fwrite_flag(pObjIndex->extra_flags, buf));
	fprintf(fp, "%s\n", fwrite_flag(pObjIndex->wear_flags, buf));

	/*
	 *  Using fwrite_flag to write most values gives a strange
	 *  looking area file, consider making a case for each
	 *  item type later.
	 */

	switch (pObjIndex->item_type) {
	default:
		fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[0], buf));
		fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[1], buf));
		fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[2], buf));
		fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[3], buf));
		fprintf(fp, "%s\n", fwrite_flag(pObjIndex->value[4], buf));
		break;

	case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
		fprintf(fp, "%ld %ld '%s' %ld %ld\n",
			pObjIndex->value[0],
			pObjIndex->value[1],
			liq_table[pObjIndex->value[2]].liq_name,
			pObjIndex->value[3],
			pObjIndex->value[4]);
		break;

	case ITEM_CONTAINER:
		fprintf(fp, "%ld %s %ld %ld %ld\n",
			pObjIndex->value[0],
			fwrite_flag(pObjIndex->value[1], buf),
			pObjIndex->value[2],
			pObjIndex->value[3],
			pObjIndex->value[4]);
		break;

	case ITEM_WEAPON:
		fprintf(fp, "%s %ld %ld '%s' %s\n",
			weapon_name((int)pObjIndex->value[0]),
			pObjIndex->value[1],
			pObjIndex->value[2],
			attack_table[pObjIndex->value[3]].name,
			fwrite_flag(pObjIndex->value[4], buf));
		break;

	case ITEM_PILL:
	case ITEM_POTION:
	case ITEM_SCROLL:
	{
		SKILL *skills[4];
		int idx;

		for (idx = 1; idx <= 4; idx++)
			skills[idx - 1] = resolve_skill_sn((int)pObjIndex->value[idx]);

		fprintf(fp, "%ld '%s' '%s' '%s' '%s'\n",
			pObjIndex->value[0],
			(skills[0] != NULL) ? skills[0]->name : "",
			(skills[1] != NULL) ? skills[1]->name : "",
			(skills[2] != NULL) ? skills[2]->name : "",
			(skills[3] != NULL) ? skills[3]->name : "");
		break;
	}

	case ITEM_STAFF:
	case ITEM_WAND:
	{
		SKILL *skill;

		skill = resolve_skill_sn((int)pObjIndex->value[3]);

		fprintf(fp, "%ld %ld %ld '%s' %ld\n",
			pObjIndex->value[0],
			pObjIndex->value[1],
			pObjIndex->value[2],
			(skill != NULL) ? skill->name : "",
			pObjIndex->value[4]);
		break;
	}
	}

	fprintf(fp, "%d ", pObjIndex->level);
	fprintf(fp, "%d ", pObjIndex->weight);
	fprintf(fp, "%u ", pObjIndex->cost);
	fprintf(fp, "%d ", pObjIndex->timer);

	if (pObjIndex->condition > 90) letter = 'P';
	else if (pObjIndex->condition > 75) letter = 'G';
	else if (pObjIndex->condition > 50) letter = 'A';
	else if (pObjIndex->condition > 25) letter = 'W';
	else if (pObjIndex->condition > 10) letter = 'D';
	else if (pObjIndex->condition > 0) letter = 'B';
	else                                                                    letter = 'R';

	fprintf(fp, "%c\n", letter);
	for (pAf = pObjIndex->affected; pAf; pAf = pAf->next) {
		if (pAf->where == TO_OBJECT || pAf->bitvector == 0) {
			fprintf(fp, "A\n%d %ld\n", pAf->location, pAf->modifier);
		} else {
			fprintf(fp, "F\n");

			switch (pAf->where) {
			case TO_AFFECTS:
				fprintf(fp, "A ");
				break;
			case TO_IMMUNE:
				fprintf(fp, "I ");
				break;
			case TO_RESIST:
				fprintf(fp, "R ");
				break;
			case TO_VULN:
				fprintf(fp, "V ");
				break;
			default:
				bug("olc_save: Invalid Affect->where", 0);
				break;
			}

			fprintf(fp, "%d %ld %s\n", pAf->location, pAf->modifier,
				fwrite_flag(pAf->bitvector, buf));
		}
	}

	for (extra = pObjIndex->extra_descr; extra; extra = extra->next)
		fprintf(fp, "E\n%s~\n%s~\n", extra->keyword, fix_string(extra->description));
	return;
}




/***************************************************************************
*	save_objects
*
*	save all of the objects in an area
***************************************************************************/
static void save_objects(FILE *fp, AREA_DATA *area)
{
	OBJ_INDEX_DATA *pObj;
	long iter;

	fprintf(fp, "#OBJECTS\n");

	for (iter = area->min_vnum; iter <= area->max_vnum; iter++)
		if ((pObj = get_obj_index(iter)))
			save_object(fp, pObj);

	fprintf(fp, "#0\n\n\n\n");
	return;
}





/***************************************************************************
*	save_rooms
*
*	save all of the room data in an area
***************************************************************************/
static void save_rooms(FILE *fp, AREA_DATA *area)
{
	ROOM_INDEX_DATA *room;
	EXTRA_DESCR_DATA *extra;
	EXIT_DATA *exit;
	AFFECT_DATA *paf;
	SKILL *skill;
	int hash_idx;
	int door;

	fprintf(fp, "#ROOMS\n");
	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (room = room_index_hash[hash_idx];
		     room != NULL;
		     room = room->next) {
			if (room->area == area) {
				fprintf(fp, "#%ld\n", room->vnum);
				fprintf(fp, "%s~\n", room->name);
				fprintf(fp, "%s~\n", fix_string(room->description));
				fprintf(fp, "0 ");
				fprintf(fp, "%ld ", room->room_flags);
				fprintf(fp, "%d\n", room->sector_type);

				for (extra = room->extra_descr; extra; extra = extra->next) {
					fprintf(fp, "E\n%s~\n%s~\n",
						extra->keyword,
						fix_string(extra->description));
				}

				for (door = 0; door < MAX_DIR; door++) { /* I hate this! */
					if ((exit = room->exit[door])
					    && exit->u1.to_room) {
						int locks = 0;

						/* HACK : TO PREVENT EX_LOCKED etc without EX_ISDOOR
						 * to stop booting the mud */
						if (IS_SET(exit->rs_flags, EX_CLOSED)
						    || IS_SET(exit->rs_flags, EX_LOCKED)
						    || IS_SET(exit->rs_flags, EX_PICKPROOF)
						    || IS_SET(exit->rs_flags, EX_NOPASS)
						    || IS_SET(exit->rs_flags, EX_EASY)
						    || IS_SET(exit->rs_flags, EX_HARD)
						    || IS_SET(exit->rs_flags, EX_INFURIATING)
						    || IS_SET(exit->rs_flags, EX_NOCLOSE)
						    || IS_SET(exit->rs_flags, EX_NOLOCK))
							SET_BIT(exit->rs_flags, EX_ISDOOR);
						else
							REMOVE_BIT(exit->rs_flags, EX_ISDOOR);

						/* THIS SUCKS but it's backwards compatible */
						/* NOTE THAT EX_NOCLOSE NOLOCK etc aren't being saved */
						if (IS_SET(exit->rs_flags, EX_ISDOOR)
						    && (!IS_SET(exit->rs_flags, EX_PICKPROOF))
						    && (!IS_SET(exit->rs_flags, EX_NOPASS)))
							locks = 1;

						if (IS_SET(exit->rs_flags, EX_ISDOOR)
						    && (IS_SET(exit->rs_flags, EX_PICKPROOF))
						    && (!IS_SET(exit->rs_flags, EX_NOPASS)))
							locks = 2;

						if (IS_SET(exit->rs_flags, EX_ISDOOR)
						    && (!IS_SET(exit->rs_flags, EX_PICKPROOF))
						    && (IS_SET(exit->rs_flags, EX_NOPASS)))
							locks = 3;

						if (IS_SET(exit->rs_flags, EX_ISDOOR)
						    && (IS_SET(exit->rs_flags, EX_PICKPROOF))
						    && (IS_SET(exit->rs_flags, EX_NOPASS)))
							locks = 4;

						fprintf(fp, "D%d\n", exit->orig_door);
						fprintf(fp, "%s~\n", fix_string(exit->description));
						fprintf(fp, "%s~\n", exit->keyword);
						fprintf(fp, "%d %ld %ld\n",
							locks,
							exit->key,
							exit->u1.to_room->vnum);
					}
				}

				if (room->mana_rate != 100 || room->heal_rate != 100) {
					fprintf(fp, "M %d H %d\n",
						room->mana_rate,
						room->heal_rate);
				}

				if (!IS_NULLSTR(room->owner))
					fprintf(fp, "O %s~\n", room->owner);

				for (paf = room->affected; paf != NULL; paf = paf->next)
					if (paf->duration < 0 && (skill = resolve_skill_sn(paf->type)) != NULL)
						fprintf(fp, "A '%s' %d\n", skill->name, paf->level);
				fprintf(fp, "S\n");
			}
		}
	}

	fprintf(fp, "#0\n\n\n\n");
	return;
}



/***************************************************************************
*	save_specials
*
*	save all of the spec funs to an area
***************************************************************************/
static void save_specials(FILE *fp, AREA_DATA *area)
{
	MOB_INDEX_DATA *mob_idx;
	int hash_idx;

	fprintf(fp, "#SPECIALS\n");
	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (mob_idx = mob_index_hash[hash_idx]; mob_idx; mob_idx = mob_idx->next)
			if (mob_idx && mob_idx->area == area && mob_idx->spec_fun)
				fprintf(fp, "M %ld %s\n", mob_idx->vnum, spec_name(mob_idx->spec_fun));
	}

	fprintf(fp, "S\n\n\n\n");
	return;
}



/***************************************************************************
*	save_door_resets
*
*	save the reset state of a door
***************************************************************************/
static void save_door_resets(FILE *fp, AREA_DATA *area)
{
	ROOM_INDEX_DATA *room;
	EXIT_DATA *exit;
	int hash_idx;
	int door;

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (room = room_index_hash[hash_idx]; room; room = room->next) {
			if (room->area == area) {
				for (door = 0; door < MAX_DIR; door++) {
					if ((exit = room->exit[door])
					    && exit->u1.to_room
					    && (IS_SET(exit->rs_flags, EX_CLOSED) || IS_SET(exit->rs_flags, EX_LOCKED))) {
						fprintf(fp, "D 0 %ld %d %d\n",
							room->vnum,
							exit->orig_door,
							IS_SET(exit->rs_flags, EX_LOCKED) ? 2 : 1);
					}
				}
			}
		}
	}

	return;
}




/***************************************************************************
*	save_resets
*
*	save all of the resets for an area
***************************************************************************/
static void save_resets(FILE *fp, AREA_DATA *area)
{
	RESET_DATA *pReset;
	MOB_INDEX_DATA *pLastMob = NULL;
	ROOM_INDEX_DATA *pRoom;
	int hash_idx;

	fprintf(fp, "#RESETS\n");

	save_door_resets(fp, area);

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (pRoom = room_index_hash[hash_idx]; pRoom; pRoom = pRoom->next) {
			if (pRoom->area == area) {
				for (pReset = pRoom->reset_first; pReset; pReset = pReset->next) {
					switch (pReset->command) {
					default:
						bug("Save_resets: bad command %c.", (int)pReset->command);
						break;

					case 'M':
						pLastMob = get_mob_index(pReset->arg1);
						fprintf(fp, "M 0 %ld %d %ld %d\n",
							pReset->arg1,
							pReset->arg2,
							pReset->arg3,
							pReset->arg4);
						break;

					case 'O':
						pRoom = get_room_index(pReset->arg3);
						fprintf(fp, "O 0 %ld 0 %ld\n",
							pReset->arg1,
							pReset->arg3);
						break;

					case 'P':
						fprintf(fp, "P 0 %ld %d %ld %d\n",
							pReset->arg1,
							pReset->arg2,
							pReset->arg3,
							pReset->arg4);
						break;

					case 'G':
						fprintf(fp, "G 0 %ld 0\n", pReset->arg1);
						if (!pLastMob)
							printf_log("Save_resets: !NO_MOB! in [%s]", area->file_name);
						break;
					case 'E':
						fprintf(fp, "E 0 %ld 0 %ld\n",
							pReset->arg1,
							pReset->arg3);
						if (!pLastMob)
							printf_log("Save_resets: !NO_MOB! in [%s]", area->file_name);
						break;

					case 'D':
						break;

					case 'R':
						pRoom = get_room_index(pReset->arg1);
						fprintf(fp, "R 0 %ld %d\n",
							pReset->arg1,
							pReset->arg2);
						break;
					}
				}
			}       /* End if correct area */
		}               /* End for pRoom */
	} /* End for hash_idx */

	fprintf(fp, "S\n\n\n\n");
	return;
}



/***************************************************************************
*	save_shops
*
*	save the shop data for an area
***************************************************************************/
static void save_shops(FILE *fp, AREA_DATA *area)
{
	SHOP_DATA *shopIndex;
	MOB_INDEX_DATA *mob_idx;
	int iTrade;
	int hash_idx;

	fprintf(fp, "#SHOPS\n");

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (mob_idx = mob_index_hash[hash_idx]; mob_idx; mob_idx = mob_idx->next) {
			if (mob_idx && mob_idx->area == area && mob_idx->shop) {
				shopIndex = mob_idx->shop;

				fprintf(fp, "%ld ", shopIndex->keeper);
				for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
					if (shopIndex->buy_type[iTrade] != 0)
						fprintf(fp, "%d ", shopIndex->buy_type[iTrade]);
					else
						fprintf(fp, "0 ");
				}
				fprintf(fp, "%d %d ", shopIndex->profit_buy, shopIndex->profit_sell);
				fprintf(fp, "%d %d\n", shopIndex->open_hour, shopIndex->close_hour);
			}
		}
	}

	fprintf(fp, "0\n\n\n\n");
	return;
}



/***************************************************************************
*	save_helps
*
*	save all of the helps for a help area
***************************************************************************/
static void save_helps(FILE *fp, HELP_AREA *ha)
{
	HELP_DATA *help = ha->first;

	fprintf(fp, "#HELPS\n");

	for (; help; help = help->next_area) {
		fprintf(fp, "%d %s~\n", help->level, help->keyword);
		fprintf(fp, "%s~\n\n", fix_string(help->text));
	}

	fprintf(fp, "-1 $~\n\n");

	ha->changed = FALSE;

	return;
}



/***************************************************************************
*	save_changed_helps
*
*	save all of the helps for a help area
***************************************************************************/
static void save_changed_helps()
{
	extern HELP_AREA *had_list;
	HELP_AREA *ha;
	AREA_DATA *area;
	FILE *fp;

	for (ha = had_list; ha; ha = ha->next) {
		if (ha->changed == TRUE) {
			for (area = area_first; area != NULL; area = area->next) {
				if (area->helps != NULL && area->helps == ha) {
					save_area(area);
					ha->changed = FALSE;
					break;
				}
			}

			if (area == NULL) {
				char haf[MIL];
				snprintf(haf, MIL, "%s%s", AREA_FOLDER, ha->filename);
				fp = fopen(haf, "w");

				if (!fp) {
					perror(haf);
					return;
				}

				save_helps(fp, ha);

				fprintf(fp, "#$\n");
				fclose(fp);

				ha->changed = FALSE;
			}
		}
	}

	return;
}

/***************************************************************************
*	save_area
*
*	save a single area
***************************************************************************/
static void save_area(AREA_DATA *area)
{
	FILE *fp;
	char haf[MIL];

	snprintf(haf, MIL, "%s%s", AREA_FOLDER, area->file_name);

	fclose(fpReserve);
	if (!(fp = fopen(haf, "w"))) {
		bug("Open_area: fopen", 0);
		perror(haf);
	}

	fprintf(fp, "#AREADATA\n");
	fprintf(fp, "Name %s~\n", area->name);
	fprintf(fp, "Builders %s~\n", fix_string(area->builders));
	fprintf(fp, "VNUMs %ld %ld\n", area->min_vnum, area->max_vnum);
	fprintf(fp, "Credits %s~\n", area->credits);
	fprintf(fp, "Security %d\n", area->security);
	fprintf(fp, "Complete %d\n", (int)area->complete);
	fprintf(fp, "Llevel %d\n", area->llevel);
	fprintf(fp, "Ulevel %d\n", area->ulevel);
	fprintf(fp, "Description %s~\n", area->description);
	fprintf(fp, "End\n\n\n\n");

	save_mobiles(fp, area);
	save_objects(fp, area);
	save_rooms(fp, area);
	save_specials(fp, area);
	save_mobprogs(fp, area);
	save_resets(fp, area);
	save_shops(fp, area);

	if (area->helps && area->helps->first)
		save_helps(fp, area->helps);

	fprintf(fp, "#$\n");

	fclose(fp);
	fpReserve = fopen(NULL_FILE, "r");
	return;
}


/***************************************************************************
*	save_socials
*
*	save the social list
***************************************************************************/
void save_socials()
{
	FILE *fp;
	SOCIAL *social;

	fp = fopen(SOCIAL_FILE, "w");

	if (!fp) {
		bug("Could not open " SOCIAL_FILE " for writing.", 0);
		return;
	}

	fprintf(fp, "%d\n", max_social);

	for (social = social_list; social != NULL; social = social->next) {
		fprintf(fp, "%s~\n", (social->name != NULL) ? social->name : "");
		fprintf(fp, "%s~\n", (social->char_no_arg != NULL) ? social->char_no_arg : "");
		fprintf(fp, "%s~\n", (social->others_no_arg != NULL) ? social->others_no_arg : "");
		fprintf(fp, "%s~\n", (social->char_found != NULL) ? social->char_found : "");
		fprintf(fp, "%s~\n", (social->others_found != NULL) ? social->others_found : "");
		fprintf(fp, "%s~\n", (social->vict_found != NULL) ? social->vict_found : "");
		fprintf(fp, "%s~\n", (social->char_auto != NULL) ? social->char_auto : "");
		fprintf(fp, "%s~\n", (social->others_auto != NULL) ? social->others_auto : "");
	}

	fclose(fp);
}

/***************************************************************************
*	do_asave
*
*	initiate an OLC save function
***************************************************************************/
void do_asave(CHAR_DATA *ch, char *argument)
{
	AREA_DATA *area;
	char arg[MIL];
	int value;

	smash_tilde(argument);
	argument = one_argument(argument, arg);
	if (is_help(arg)) {
		if (ch != NULL) {
			send_to_char("`#Syntax`3:``\n\r", ch);
			send_to_char("  `!asave `1<`!vnum`1>``   - saves a particular area\n\r", ch);
			send_to_char("  `!asave list``     - saves the area.lst file\n\r", ch);
			send_to_char("  `!asave area``     - saves the area being edited\n\r", ch);
			send_to_char("  `!asave changed``  - saves all changed zones\n\r", ch);
			send_to_char("  `!asave helps``    - saves all helps *not* linked to areas\n\r", ch);
			send_to_char("  `!asave skills``   - saves the skill list\n\r", ch);
			send_to_char("  `!asave world``    - saves EVERYTHING\n\r", ch);
			send_to_char("\n\r", ch);
		}

		return;
	}

	/*
	 * see if we have a numeric arugment - if we do, then
	 * save the area it corresponds to
	 */
	if (is_number(arg)) {
		value = atoi(arg);

		if ((area = get_area_data(value)) == NULL) {
			if (ch != NULL)
				send_to_char("That area does not exist.\n\r", ch);
			return;
		}

		/* save the area */
		if (area != NULL) {
			if (ch && !IS_BUILDER(ch, area))
				send_to_char("You are not a builder for this area.\n\r", ch);

			save_area_list();
			save_area(area);
		}
		return;
	}

	/* save everything */
	if (!str_prefix(arg, "world")) {
		save_area_list();
		save_changed_helps();

		for (area = area_first; area; area = area->next) {
			if (ch != NULL && !IS_BUILDER(ch, area))
				continue;

			save_area(area);
			REMOVE_BIT(area->area_flags, AREA_CHANGED);
		}
		if (ch != NULL)
			send_to_char("You saved the world.\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "helps")) {
		save_changed_helps();
		if (ch != NULL)
			send_to_char("All changed helps have been saved.\n\r", ch);

		return;
	}

	/* Save changed areas, only authorized areas. */
	/* ------------------------------------------ */
	if (!str_prefix(arg, "changed")) {
		char buf[MIL];
		bool saved;

		save_area_list();

		if (ch != NULL)
			send_to_char("Saved areas:\n\r", ch);
		else
			log_string("Saved areas:");

		saved = FALSE;
		for (area = area_first; area; area = area->next) {
			/* Builder must be assigned this area. */
			if (ch != NULL && !IS_BUILDER(ch, area))
				continue;

			/* Save changed areas. */
			if (IS_SET(area->area_flags, AREA_CHANGED)) {
				save_area(area);
				saved = TRUE;
				sprintf(buf, "%24s - '%s'", area->name, area->file_name);
				if (ch != NULL) {
					send_to_char(buf, ch);
					send_to_char("\n\r", ch);
				} else {
					log_string(buf);
				}

				REMOVE_BIT(area->area_flags, AREA_CHANGED);
			}
		}

		save_changed_helps();

		if (!saved) {
			if (ch != NULL)
				send_to_char("None.", ch);
			else
				log_string("None.");
		}
		return;
	}

	/* save the area list */
	if (!str_prefix(arg, "list")) {
		save_area_list();
		return;
	}

	/* save the area that is currently being edited */
	if (!str_prefix(arg, "area")) {
		if (ch == NULL
		    || ch->desc == NULL)
			return;

		if (ch->desc->editor == ED_NONE) {
			send_to_char("You are not editing an area, therefore an area vnum is required.\n\r", ch);
			return;
		}

		switch (ch->desc->editor) {
		case ED_AREA:
			area = (AREA_DATA *)ch->desc->ed_data;
			break;
		case ED_ROOM:
			area = ch->in_room->area;
			break;
		case ED_OBJECT:
			area = ((OBJ_INDEX_DATA *)ch->desc->ed_data)->area;
			break;
		case ED_MOBILE:
			area = ((MOB_INDEX_DATA *)ch->desc->ed_data)->area;
			break;
		case ED_HELP:
			send_to_char("Saving helps.", ch);
			save_changed_helps();
			return;
		default:
			area = ch->in_room->area;
			break;
		}

		if (area == NULL
		    || !IS_BUILDER(ch, area)) {
			send_to_char("You are not a builder for this area.\n\r", ch);
			return;
		}

		save_area_list();
		save_area(area);
		REMOVE_BIT(area->area_flags, AREA_CHANGED);
		send_to_char("Area saved.\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "skills")) {
		save_skills();
		save_groups();
		send_to_char("Skills saved.\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "socials")) {
		save_socials();
		send_to_char("Socials saved.\n\r", ch);
		return;
	}

	/* display help */
	do_asave(ch, "");
	return;
}
