#define MAX_RETURN              200


/***************************************************************************
 *	utility functions
 ***************************************************************************/
bool cmp_fn_number(long val, const char *arg);
bool cmp_fn_string(const char *name, const char *arg);
bool cmp_fn_flag(long bit, const char *arg, const struct flag_type *table, BUFFER * buf);
bool cmp_fn_index(long bit, const char *arg, const struct flag_type *table, BUFFER * buf);


/***************************************************************************
 *	where functions - owhere mwhere
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/
typedef bool OBJ_CMP_FN (const GAMEOBJECT *obj, const char *arg, BUFFER *buf);

struct cmp_vars_gameobject {
    char *		var;
    OBJ_CMP_FN *	fn;
};


bool obj_cmp_vnum(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_name(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_short(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_long(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_type(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_extra(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_wear(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_location(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_weight(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_cost(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);
bool obj_cmp_level(const GAMEOBJECT * obj, const char *arg, BUFFER * buf);


/***************************************************************************
 *	characters
 ***************************************************************************/
typedef bool CHAR_CMP_FN (const CHAR_DATA *vch, const char *arg, BUFFER *buf);

struct cmp_vars_char_data {
    char *		var;
    CHAR_CMP_FN *	fn;
};

bool char_cmp_vnum(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_name(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_short(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_long(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_race(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_level(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_sex(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_hit(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_mana(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_move(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_max_hit(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_max_mana(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_max_move(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_gold(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_silver(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_offense(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_form(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_act(const CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_player(const CHAR_DATA * vch, const char *arg, BUFFER * buf);



/***************************************************************************
 *	vnum lookups - ovnum mvnum
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/
typedef bool OBJ_IDX_CMP_FN (const OBJECTPROTOTYPE *obj, const char *arg, BUFFER *buf);

struct cmp_vars_obj_index_data {
    char *		var;
    OBJ_IDX_CMP_FN *fn;
};

bool objprototype_cmp_name(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_short(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_long(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_type(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_extra(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_wear(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_weight(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_cost(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_level(const OBJECTPROTOTYPE * obj, const char *arg, BUFFER * buf);


/***************************************************************************
 *	mobs
 ***************************************************************************/
typedef bool MOB_IDX_CMP_FN (const MOB_INDEX_DATA *vch, const char *arg, BUFFER *buf);

struct cmp_vars_mob_index_data {
    char *		var;
    MOB_IDX_CMP_FN *fn;
};

bool mob_idx_cmp_name(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_short(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_long(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_race(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_level(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_sex(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_wealth(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_offense(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_form(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_act(const MOB_INDEX_DATA * vch, const char *arg, BUFFER * buf);
