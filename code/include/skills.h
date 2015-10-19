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
typedef struct argument_type ARGUMENT;

typedef struct dynamic_skill DYNAMIC_SKILL;
typedef void SPELL_FUN(DYNAMIC_SKILL * skill, int level, CHAR_DATA * ch, void *vo, int target, const char *argument);
typedef void AFFECT_FUN(DYNAMIC_SKILL * skill, void *target, int type, AFFECT_DATA *paf);


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
	struct dynamic_skill *next;
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
	struct spell_list *spells;
	struct affect_list *affects;
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
	struct dynamic_skill_list *next;
	struct dynamic_skill_list *prev;
	struct dynamic_skill *	skill;
	bool valid;
};

/***************************************************************************
*	dynamic_group
*
*	a list of spell groups
***************************************************************************/
struct dynamic_group {
	struct dynamic_group *next;
	int	 gn;
	char *name;
	struct dynamic_skill_list *skills;
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
	struct dynamic_skill *		skill;
	struct dynamic_group *		group;
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
	struct spell_list *	next;
	SPELL_FUN *	spell_fn;
	bool		valid;
};


/***************************************************************************
*	affect_list
*
*	a list of affect functions - used for affect chaining
***************************************************************************/
struct affect_list {
	struct affect_list *	next;
	AFFECT_FUN *	affect_fn;
	bool		valid;
};

/***************************************************************************
*	memory recycling
***************************************************************************/
struct level_info *level_info_free;
struct learned_info *learned_free;
struct dynamic_skill_list *skill_list_free;
struct spell_list *spell_list_free;
struct affect_list *affect_list_free;
ARGUMENT *argument_free;
struct dynamic_group *group_free;
struct dynamic_group *group_list;
struct dynamic_skill *skill_free;
struct dynamic_skill *skill_list;


struct level_info *new_level_info(void);
void free_level_info(struct level_info * li);

struct learned_info *new_learned(void);
void free_learned(struct learned_info * learned);

struct dynamic_skill_list *new_skill_list(void);
void free_skill_list(struct dynamic_skill_list * list);

struct dynamic_skill *new_skill(void);
void free_skill(struct dynamic_skill * skill);

struct dynamic_group *new_group(void);
void free_group(struct dynamic_group * group);

struct spell_list *new_spell_list(void);
void free_spell_list(struct spell_list * spells);

struct affect_list *new_affect_list(void);
void free_affect_list(struct affect_list * affects);

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


/*@shared@*/struct dynamic_skill *skill_lookup(const char *name);
struct dynamic_group *group_lookup(const char *name);

struct dynamic_skill *resolve_skill_sn(int sn);
struct dynamic_skill *resolve_skill_affect(AFFECT_DATA *paf);
int get_skill_number(char *name);

void add_learned(CHAR_DATA * ch, struct learned_info * learned);
void add_learned_group(CHAR_DATA * ch, struct learned_info * learned);
void add_learned_skill(CHAR_DATA * ch, struct learned_info * learned);
void remove_learned(CHAR_DATA * ch, struct learned_info * learned);
void add_skill_level(struct dynamic_skill * skill, struct level_info * level);
void add_group_level(struct dynamic_group * group, struct level_info * level);
void add_group_skill(struct dynamic_group * group, struct dynamic_skill * skill);

struct learned_info *create_learned_skill(char *name, int percent);
struct learned_info *create_learned_group(char *name);

struct level_info *get_skill_level(CHAR_DATA * ch, struct dynamic_skill * skill);
struct level_info *get_group_level(struct dynamic_group * group, int cls);


struct learned_info *get_learned(CHAR_DATA * ch, char *name);
struct learned_info *get_learned_group(CHAR_DATA * ch, struct dynamic_group * group);
struct learned_info *get_learned_skill(CHAR_DATA * ch, struct dynamic_skill * skill);

int get_learned_percent(CHAR_DATA * ch, struct dynamic_skill * skill);
bool check_affected(CHAR_DATA * ch, char *name);


SPELL_FUN *spell_fn_lookup(const char *name);
char *spell_fn_name(SPELL_FUN * fn);

AFFECT_FUN *affect_fn_lookup(const char *name);
char *affect_fn_name(AFFECT_FUN * fn);


/* functions used for spell/affect chaining */
void add_spell(struct dynamic_skill * skill, SPELL_FUN * spell);
void add_affect(struct dynamic_skill * skill, AFFECT_FUN * affect);
void add_argument(struct dynamic_skill * skill, ARGUMENT * argument);

VARIANT *find_argument(ARGUMENT * argument, char *key);

/* functions used for global caching of spells */
void resolve_global_skills(void);
void resolve_global_hash(void);


#endif
