#define MAX_RETURN              200


/***************************************************************************
 *	utility functions
 ***************************************************************************/
bool cmp_fn_number(long val, const char *arg);
bool cmp_fn_string(const char *name, const char *arg);
bool cmp_fn_flag(long bit, const char *arg, const struct flag_type *table, struct buf_type * buf);
bool cmp_fn_index(long bit, const char *arg, const struct flag_type *table, struct buf_type * buf);


/***************************************************************************
 *	where functions - owhere mwhere
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/
typedef bool OBJ_CMP_FN (struct gameobject *obj, const char *arg, struct buf_type *buf);

struct cmp_vars_gameobject {
    char *		var;
    OBJ_CMP_FN *	fn;
};


bool obj_cmp_vnum(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_name(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_short(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_long(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_type(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_extra(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_wear(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_location(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_weight(struct gameobject * obj, const char *arg, struct buf_type * buf);
bool obj_cmp_cost(struct gameobject * obj, const char *arg, struct buf_type * buf);


/***************************************************************************
 *	characters
 ***************************************************************************/
typedef bool CHAR_CMP_FN (struct char_data *vch, const char *arg, struct buf_type *buf);

struct cmp_vars_char_data {
    char *		var;
    CHAR_CMP_FN *	fn;
};

bool char_cmp_vnum(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_name(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_short(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_long(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_race(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_level(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_sex(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_hit(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_mana(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_move(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_max_hit(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_max_mana(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_max_move(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_gold(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_silver(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_offense(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_form(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_act(struct char_data * vch, const char *arg, struct buf_type * buf);
bool char_cmp_player(struct char_data * vch, const char *arg, struct buf_type * buf);



/***************************************************************************
 *	vnum lookups - ovnum mvnum
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/
typedef bool OBJ_IDX_CMP_FN (struct objectprototype *obj, const char *arg, struct buf_type *buf);

struct cmp_vars_obj_index_data {
    char *		var;
    OBJ_IDX_CMP_FN *fn;
};

bool objprototype_cmp_name(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_short(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_long(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_type(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_extra(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_wear(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_weight(struct objectprototype * obj, const char *arg, struct buf_type * buf);
bool objprototype_cmp_cost(struct objectprototype * obj, const char *arg, struct buf_type * buf);


/***************************************************************************
 *	mobs
 ***************************************************************************/
typedef bool MOB_IDX_CMP_FN (const struct mob_index_data *vch, const char *arg, struct buf_type *buf);

struct cmp_vars_mob_index_data {
    char *		var;
    MOB_IDX_CMP_FN *fn;
};

bool mob_idx_cmp_name(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_short(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_long(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_race(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_level(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_sex(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_wealth(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_offense(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_form(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
bool mob_idx_cmp_act(const struct mob_index_data * vch, const char *arg, struct buf_type * buf);
