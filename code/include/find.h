/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/



#define MAX_RETURN              200


/***************************************************************************
*	utility functions
***************************************************************************/
bool cmp_fn_number(long val, char *arg);
bool cmp_fn_string(char *name, char *arg);
bool cmp_fn_flag(long bit, char *arg, const struct flag_type *table, BUFFER * buf);
bool cmp_fn_index(long bit, char *arg, const struct flag_type *table, BUFFER * buf);


/***************************************************************************
*	where functions - owhere mwhere
***************************************************************************/
/***************************************************************************
*	objects
***************************************************************************/
typedef bool OBJ_CMP_FN (OBJ_DATA *obj, char *arg, BUFFER *buf);

struct cmp_vars_obj_data {
	char *		var;
	OBJ_CMP_FN *	fn;
};


bool obj_cmp_vnum(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_name(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_short(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_long(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_type(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_extra(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_wear(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_location(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_weight(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_cost(OBJ_DATA * obj, char *arg, BUFFER * buf);
bool obj_cmp_level(OBJ_DATA * obj, char *arg, BUFFER * buf);


/***************************************************************************
*	characters
***************************************************************************/
typedef bool CHAR_CMP_FN (CHAR_DATA *vch, char *arg, BUFFER *buf);

struct cmp_vars_char_data {
	char *		var;
	CHAR_CMP_FN *	fn;
};

bool char_cmp_vnum(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_name(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_short(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_long(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_race(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_level(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_sex(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_hit(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_mana(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_move(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_max_hit(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_max_mana(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_max_move(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_gold(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_silver(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_offense(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_form(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_act(CHAR_DATA * vch, char *arg, BUFFER * buf);
bool char_cmp_player(CHAR_DATA * vch, char *arg, BUFFER * buf);



/***************************************************************************
*	vnum lookups - ovnum mvnum
***************************************************************************/
/***************************************************************************
*	objects
***************************************************************************/
typedef bool OBJ_IDX_CMP_FN (OBJ_INDEX_DATA *obj, char *arg, BUFFER *buf);

struct cmp_vars_obj_index_data {
	char *		var;
	OBJ_IDX_CMP_FN *fn;
};

bool obj_idx_cmp_name(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_short(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_long(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_type(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_extra(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_wear(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_weight(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_cost(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);
bool obj_idx_cmp_level(OBJ_INDEX_DATA * obj, char *arg, BUFFER * buf);


/***************************************************************************
*	mobs
***************************************************************************/
typedef bool MOB_IDX_CMP_FN (MOB_INDEX_DATA *vch, char *arg, BUFFER *buf);

struct cmp_vars_mob_index_data {
	char *		var;
	MOB_IDX_CMP_FN *fn;
};

bool mob_idx_cmp_name(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_short(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_long(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_race(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_level(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_sex(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_wealth(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_offense(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_form(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
bool mob_idx_cmp_act(MOB_INDEX_DATA * vch, char *arg, BUFFER * buf);
