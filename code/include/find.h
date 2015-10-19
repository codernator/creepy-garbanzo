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
typedef bool OBJ_CMP_FN (struct gameobject *obj, const char *arg, BUFFER *buf);

struct cmp_vars_gameobject {
    char *		var;
    OBJ_CMP_FN *	fn;
};


bool obj_cmp_vnum(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_name(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_short(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_long(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_type(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_extra(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_wear(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_location(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_weight(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_cost(struct gameobject * obj, const char *arg, BUFFER * buf);
bool obj_cmp_level(struct gameobject * obj, const char *arg, BUFFER * buf);


/***************************************************************************
 *	characters
 ***************************************************************************/
typedef bool CHAR_CMP_FN (CHAR_DATA *vch, const char *arg, BUFFER *buf);

struct cmp_vars_char_data {
    char *		var;
    CHAR_CMP_FN *	fn;
};

bool char_cmp_vnum(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_name(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_short(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_long(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_race(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_level(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_sex(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_hit(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_mana(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_move(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_max_hit(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_max_mana(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_max_move(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_gold(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_silver(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_offense(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_form(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_act(CHAR_DATA * vch, const char *arg, BUFFER * buf);
bool char_cmp_player(CHAR_DATA * vch, const char *arg, BUFFER * buf);



/***************************************************************************
 *	vnum lookups - ovnum mvnum
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/
typedef bool OBJ_IDX_CMP_FN (struct objectprototype *obj, const char *arg, BUFFER *buf);

struct cmp_vars_obj_index_data {
    char *		var;
    OBJ_IDX_CMP_FN *fn;
};

bool objprototype_cmp_name(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_short(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_long(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_type(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_extra(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_wear(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_weight(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_cost(struct objectprototype * obj, const char *arg, BUFFER * buf);
bool objprototype_cmp_level(struct objectprototype * obj, const char *arg, BUFFER * buf);


/***************************************************************************
 *	mobs
 ***************************************************************************/
typedef bool MOB_IDX_CMP_FN (const struct mob_index_data *vch, const char *arg, BUFFER *buf);

struct cmp_vars_mob_index_data {
    char *		var;
    MOB_IDX_CMP_FN *fn;
};

bool mob_idx_cmp_name(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_short(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_long(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_race(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_level(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_sex(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_wealth(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_offense(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_form(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
bool mob_idx_cmp_act(const struct mob_index_data * vch, const char *arg, BUFFER * buf);
