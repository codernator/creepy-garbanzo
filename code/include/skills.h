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
typedef struct dynamic_skill SKILL;
typedef struct dynamic_skill_list SKILL_LIST;
typedef struct dynamic_group GROUP;
typedef struct spell_list SPELL_LIST;
typedef struct affect_list AFFECT_LIST;
typedef struct argument_type ARGUMENT;

typedef void SPELL_FUN(SKILL * skill, int level, CHAR_DATA * ch, void *vo, int target, const char *argument);
typedef void AFFECT_FUN(SKILL * skill, void *target, int type, AFFECT_DATA *paf);


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
	struct level_info *	next;
	int		class;
	int		level;
	int		difficulty;
	bool		valid;
	/*
	 *      this is for future use
	 * struct level_info *	prerequisites;
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
	SKILL *next;
	int sn;
	char *name;
	int target;
	int min_pos;
	int min_mana;
	int wait;
	char *dam_noun;
	char *msg;
	char *msg_obj;
	char *msg_others;
	long flags;
	int	difficulty;
	bool valid;
	SPELL_LIST *spells;
	AFFECT_LIST *affects;
	struct level_info *levels;
	ARGUMENT *args;
	char *help_keyword;
	/* mob specific information */
	long act_flag;
	long off_flag;
	int	 percent;
};

/***************************************************************************
*	dynamic_skill_list
*
*	a list of linked skills
***************************************************************************/
struct dynamic_skill_list {
	SKILL_LIST *next;
	SKILL_LIST *prev;
	SKILL *	skill;
	bool valid;
};

/***************************************************************************
*	dynamic_group
*
*	a list of spell groups
***************************************************************************/
struct dynamic_group {
	GROUP *next;
	int	 gn;
	char *name;
	SKILL_LIST *skills;
	struct level_info *levels;
	bool valid;
	char *help_keyword;
};


/***************************************************************************
*	learned_info
*
*	linked list of learned percentage for skills
*	tied to a character
***************************************************************************/
struct learned_info {
	struct learned_info *	next;
	struct learned_info *	prev;
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
struct level_info *level_info_free;
struct learned_info *learned_free;
SKILL_LIST *skill_list_free;
SPELL_LIST *spell_list_free;
AFFECT_LIST *affect_list_free;
ARGUMENT *argument_free;
GROUP *group_free;
GROUP *group_list;
SKILL *skill_free;
SKILL *skill_list;


struct level_info *new_level_info(void);
void free_level_info(struct level_info * li);

struct learned_info *new_learned(void);
void free_learned(struct learned_info * learned);

SKILL_LIST *new_skill_list(void);
void free_skill_list(SKILL_LIST * list);

SKILL *new_skill(void);
void free_skill(SKILL * skill);

GROUP *new_group(void);
void free_group(GROUP * group);

SPELL_LIST *new_spell_list(void);
void free_spell_list(SPELL_LIST * spells);

AFFECT_LIST *new_affect_list(void);
void free_affect_list(AFFECT_LIST * affects);

ARGUMENT *new_argument(void);
void free_argument(ARGUMENT * argument);

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
void load_skills(void);
void save_skills(void);
void load_groups(void);
void save_groups(void);
void assign_skill_helps(void);


/*@shared@*/SKILL *skill_lookup(const char *name);
GROUP *group_lookup(const char *name);

SKILL *resolve_skill_sn(int sn);
SKILL *resolve_skill_affect(AFFECT_DATA *paf);
int get_skill_number(char *name);

void add_learned(CHAR_DATA * ch, struct learned_info * learned);
void add_learned_group(CHAR_DATA * ch, struct learned_info * learned);
void add_learned_skill(CHAR_DATA * ch, struct learned_info * learned);
void remove_learned(CHAR_DATA * ch, struct learned_info * learned);
void add_skill_level(SKILL * skill, struct level_info * level);
void add_group_level(GROUP * group, struct level_info * level);
void add_group_skill(GROUP * group, SKILL * skill);

struct learned_info *create_learned_skill(char *name, int percent);
struct learned_info *create_learned_group(char *name);

struct level_info *get_skill_level(CHAR_DATA * ch, SKILL * skill);
struct level_info *get_group_level(GROUP * group, int cls);


struct learned_info *get_learned(CHAR_DATA * ch, char *name);
struct learned_info *get_learned_group(CHAR_DATA * ch, GROUP * group);
struct learned_info *get_learned_skill(CHAR_DATA * ch, SKILL * skill);

int get_learned_percent(CHAR_DATA * ch, SKILL * skill);
bool check_affected(CHAR_DATA * ch, char *name);


SPELL_FUN *spell_fn_lookup(const char *name);
char *spell_fn_name(SPELL_FUN * fn);

AFFECT_FUN *affect_fn_lookup(const char *name);
char *affect_fn_name(AFFECT_FUN * fn);


/* functions used for spell/affect chaining */
void add_spell(SKILL * skill, SPELL_FUN * spell);
void add_affect(SKILL * skill, AFFECT_FUN * affect);
void add_argument(SKILL * skill, ARGUMENT * argument);

VARIANT *find_argument(ARGUMENT * argument, char *key);

/* functions used for global caching of spells */
void resolve_global_skills(void);
void resolve_global_hash(void);


#endif
