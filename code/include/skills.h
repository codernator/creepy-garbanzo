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
*                   thought has gone into this software and you are          *
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


#if !defined(__SKILLS_H)
#define __SKILLS_H

#include "utility.h"

/***************************************************************************
*	persistence definitions
***************************************************************************/
#define SKILL_FILE      "./db/skill_file.txt"
#define GROUP_FILE      "./db/group_file.txt"


/***************************************************************************
*	learned type definitions
***************************************************************************/
#define LEARNED_TYPE_SKILL              1
#define LEARNED_TYPE_GROUP              2

/***************************************************************************
*	typdefs
***************************************************************************/
typedef struct level_info LEVEL_INFO;
typedef struct learned_info LEARNED;
typedef struct dynamic_skill SKILL;
typedef struct dynamic_skill_list SKILL_LIST;
typedef struct dynamic_group GROUP;
typedef struct spell_list SPELL_LIST;
typedef struct affect_list AFFECT_LIST;
typedef struct argument_type ARGUMENT;

typedef void SPELL_FUN args ((SKILL * skill, int level, CHAR_DATA * ch, void *vo, int target, char *argument));
typedef void AFFECT_FUN args ((SKILL * skill, void *target, int type, AFFECT_DATA * paf));


/***************************************************************************
*	argument_type
*
*	a list of semi-static arguments for a spell
***************************************************************************/
struct argument_type {
	ARGUMENT *	next;
	char *		key;
	VARIANT *	data;
	bool		valid;
};

/***************************************************************************
*	level_info
*
*	dynamic level information for a skill
***************************************************************************/
struct level_info {
	LEVEL_INFO *	next;
	int		class;
	int		level;
	int		difficulty;
	bool		valid;
	/*
	 *      this is for future use
	 * LEVEL_INFO *	prerequisites;
	 */
};

/***************************************************************************
*	dynamic_skill
*
*	these are presisted to a file and contain all of the
*	information for a spell - persisting to a file allows
*	this data to be easily expanded upon or edited
***************************************************************************/
struct dynamic_skill {
	SKILL *		next;
	int		sn;
	char *		name;
	int		target;
	int		min_pos;
	int		min_mana;
	int		wait;
	char *		dam_noun;
	char *		msg;
	char *		msg_obj;
	char *		msg_others;
	long		flags;
	int		difficulty;
	bool		valid;
	SPELL_LIST *	spells;
	AFFECT_LIST *	affects;
	LEVEL_INFO *	levels;
	HELP_DATA *	help;
	ARGUMENT *	args;
	char *		help_keyword;
	/* mob specific information */
	long		act_flag;
	long		off_flag;
	int		percent;
};

/***************************************************************************
*	dynamic_skill_list
*
*	a list of linked skills
***************************************************************************/
struct dynamic_skill_list {
	SKILL_LIST *	next;
	SKILL_LIST *	prev;
	SKILL *		skill;
	bool		valid;
};

/***************************************************************************
*	dynamic_group
*
*	a list of spell groups
***************************************************************************/
struct dynamic_group {
	GROUP *		next;
	int		gn;
	char *		name;
	SKILL_LIST *	skills;
	LEVEL_INFO *	levels;
	bool		valid;
	HELP_DATA *	help;
	char *		help_keyword;
};


/***************************************************************************
*	learned_info
*
*	linked list of learned percentage for skills
*	tied to a character
***************************************************************************/
struct learned_info {
	LEARNED *	next;
	LEARNED *	prev;
	SKILL *		skill;
	GROUP *		group;
	int		percent;
	int		type;
	bool		removable;
	bool		valid;
};




/***************************************************************************
*	spell_list
*
*	a list of spell functions - used for spell chaining
***************************************************************************/
struct spell_list {
	SPELL_LIST *	next;
	SPELL_FUN *	spell_fn;
	bool		valid;
};


/***************************************************************************
*	affect_list
*
*	a list of affect functions - used for affect chaining
***************************************************************************/
struct affect_list {
	AFFECT_LIST *	next;
	AFFECT_FUN *	affect_fn;
	bool		valid;
};

/***************************************************************************
*	memory recycling
***************************************************************************/
LEVEL_INFO *level_info_free;
LEARNED *learned_free;
SKILL_LIST *skill_list_free;
SPELL_LIST *spell_list_free;
AFFECT_LIST *affect_list_free;
ARGUMENT *argument_free;
GROUP *group_free;
GROUP *group_list;
SKILL *skill_free;
SKILL *skill_list;


LEVEL_INFO *new_level_info             args((void));
void free_level_info                    args((LEVEL_INFO * li));

LEARNED *new_learned                     args((void));
void free_learned                               args((LEARNED * learned));

SKILL_LIST *new_skill_list             args((void));
void free_skill_list                    args((SKILL_LIST * list));

SKILL *new_skill                               args((void));
void free_skill                                 args((SKILL * skill));

GROUP *new_group                               args((void));
void free_group                                 args((GROUP * group));

SPELL_LIST *new_spell_list             args((void));
void free_spell_list                    args((SPELL_LIST * spells));

AFFECT_LIST *new_affect_list   args((void));
void free_affect_list                   args((AFFECT_LIST * affects));

ARGUMENT *new_argument                 args((void));
void free_argument                              args((ARGUMENT * argument));

/***************************************************************************
*	lookup data
***************************************************************************/
struct spell_lookup_type {
	char *		name;
	SPELL_FUN *	fn;
};

struct affect_lookup_type {
	char *		name;
	AFFECT_FUN *	fn;
};

extern const struct spell_lookup_type spell_lookup_table[];
extern const struct affect_lookup_type affect_lookup_table[];


/***************************************************************************
*	save functions
***************************************************************************/
void load_skills                args((void));
void save_skills                args((void));
void load_groups                args((void));
void save_groups                args((void));
void assign_skill_helps args((void));


SKILL *skill_lookup                    args((char *name));
GROUP *group_lookup                    args((char *name));

SKILL *resolve_skill_sn                args((int sn));
SKILL *resolve_skill_affect    args((AFFECT_DATA * paf));
int get_skill_number                    args((char *name));

void add_learned                                args((CHAR_DATA * ch, LEARNED * learned));
void add_learned_group                  args((CHAR_DATA * ch, LEARNED * learned));
void add_learned_skill                  args((CHAR_DATA * ch, LEARNED * learned));
void remove_learned                             args((CHAR_DATA * ch, LEARNED * learned));
void add_skill_level                    args((SKILL * skill, LEVEL_INFO * level));
void add_group_level                    args((GROUP * group, LEVEL_INFO * level));
void add_group_skill                    args((GROUP * group, SKILL * skill));

LEARNED *create_learned_skill  args((char *name, int percent));
LEARNED *create_learned_group  args((char *name));

LEVEL_INFO *get_skill_level    args((CHAR_DATA * ch, SKILL * skill));
LEVEL_INFO *get_group_level    args((GROUP * group, int cls));


LEARNED *get_learned                   args((CHAR_DATA * ch, char *name));
LEARNED *get_learned_group             args((CHAR_DATA * ch, GROUP * group));
LEARNED *get_learned_skill             args((CHAR_DATA * ch, SKILL * skill));

int get_learned_percent                     args((CHAR_DATA * ch, SKILL * skill));
bool check_affected                             args((CHAR_DATA * ch, char *name));


SPELL_FUN *spell_fn_lookup         args((char *name));
char *spell_fn_name           args((SPELL_FUN * fn));

AFFECT_FUN *affect_fn_lookup        args((char *name));
char *affect_fn_name          args((AFFECT_FUN * fn));


/* functions used for spell/affect chaining */
void add_spell                                  args((SKILL * skill, SPELL_FUN * spell));
void add_affect                                 args((SKILL * skill, AFFECT_FUN * affect));
void add_argument                               args((SKILL * skill, ARGUMENT * argument));

VARIANT *find_argument                 args((ARGUMENT * argument, char *key));

/* functions used for global caching of spells */
void resolve_global_skills      args((void));
void resolve_global_hash        args((void));


#endif
