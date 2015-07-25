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

/***************************************************************************
*	db.c
***************************************************************************/
extern bool db_loading;
extern int newmobs;
extern int newobjs;
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern long top_mob_index;
extern long top_obj_index;
extern int top_affect;
extern int top_ed;
extern AREA_DATA *area_first;


/* macro for flag swapping */
#define GET_UNSET(flag1, flag2)  (~(flag1) & ((flag1) | (flag2)))

/* Magic number for memory allocation */
#define MAGIC_NUM 52571214

/* func from db.c */
extern void assign_area_vnum(long vnum);                     /* OLC */
