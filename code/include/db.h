extern bool db_loading;
extern int newmobs;
extern int newobjs;
extern struct room_index_data *room_index_hash[MAX_KEY_HASH];
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern long top_mob_index;
extern int top_affect;
extern int top_ed;


/* macro for flag swapping */
#define GET_UNSET(flag1, flag2)  (~(flag1) & ((flag1) | (flag2)))

/* Magic number for memory allocation */
#define MAGIC_NUM 52571214

/* func from db.c */
extern void assign_area_vnum(long vnum);                     /* OLC */
