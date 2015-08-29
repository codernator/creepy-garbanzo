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
typedef bool OBJ_CMP_FN (GAMEOBJECT *obj, char *arg, BUFFER *buf);

struct cmp_vars_gameobject {
    char *		var;
    OBJ_CMP_FN *	fn;
};


bool obj_cmp_vnum(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_name(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_short(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_long(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_type(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_extra(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_wear(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_location(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_weight(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_cost(GAMEOBJECT * obj, char *arg, BUFFER * buf);
bool obj_cmp_level(GAMEOBJECT * obj, char *arg, BUFFER * buf);


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
typedef bool OBJ_IDX_CMP_FN (OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf);

struct cmp_vars_obj_index_data {
    char *		var;
    OBJ_IDX_CMP_FN *fn;
};

bool objprototype_cmp_name(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_short(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_long(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_type(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_extra(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_wear(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_weight(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_cost(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);
bool objprototype_cmp_level(OBJECTPROTOTYPE * obj, char *arg, BUFFER * buf);


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
