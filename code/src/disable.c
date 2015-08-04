#include <stdio.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "sysinternals.h"
#include "libfile.h"

extern int parse_int(char *test);

/***************************************************************************
*  merc.h:
*  typedef struct disabled_data	DISABLED_DATA;
*
*  struct disabled_data
*  {
*   DISABLED_DATA *		next;
*   char *				command;
*   char *				disabled_by;
*   int				level;
*   int				type;
*  };
*
*  extern DISABLED_DATA *	disabled_first;
*
* #define DISABLED_FILE	"disabled.txt"
***************************************************************************/

/***************************************************************************
*	function prototypes
***************************************************************************/
/*
 * DISABLED_DATA *	new_disabled());
 * void			free_disabled(DISABLED_DATA * disabled));
 *
 * void disable_cmd(CHAR_DATA * ch, char * argument, DISABLED_DATA * disabled_list);
 * void disable_spell(CHAR_DATA * ch, char * argument, DISABLED_DATA * disabled_list);
 * void disable_all(CHAR_DATA * ch, char * argument, DISABLED_DATA * disabled_list);
 * void disable_show(CHAR_DATA * ch, DISABLED_DATA * list);
 */

extern bool battlefield_check_disabled(CHAR_DATA * ch, int type, char *name);

/***************************************************************************
*	memory functions
***************************************************************************/
static DISABLED_DATA *disabled_free;

/***************************************************************************
*	new_disabled()
***************************************************************************/
DISABLED_DATA *new_disabled()
{
	DISABLED_DATA *disabled;

	if (disabled_free == NULL) {
		disabled = alloc_perm((unsigned int)sizeof(*disabled));
	} else {
		disabled = disabled_free;
		disabled_free = disabled_free->next;
	}

	VALIDATE(disabled);

	return disabled;
}

/***************************************************************************
*	free_disabled
***************************************************************************/
void free_disabled(DISABLED_DATA *disabled)
{
	if (!IS_VALID(disabled))
		return;

	free_string(disabled->command);
	free_string(disabled->disabled_by);
	INVALIDATE(disabled);

	disabled->next = disabled_free;
	disabled_free = disabled;
}

/***************************************************************************
*	do_disable
***************************************************************************/
void do_disable(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MSL];

	if (IS_NPC(ch)) {
		send_to_char("RETURN first - stupid switched mofo.\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '?' || !str_cmp(arg, "help")) {
		do_help(ch, "disable");
		return;
	}

	if (arg[0] == '\0' || !str_cmp(arg, "show")) {
		disable_show(ch, disabled_first);
		return;
	}


	if ((victim = get_char_world(ch, arg)) != NULL
	    && !IS_NPC(victim)) {
		/* disable a character cmd or spell*/
		argument = one_argument(argument, arg);

		if (get_trust(victim) >= get_trust(ch)) {
			send_to_char("That is a good way to get spanked.\n\r", ch);
			return;
		}

		if (!str_prefix(arg, "spell")) {
			argument = one_argument(argument, arg);
			disable_spell(ch, argument, &victim->disabled);
		} else if (!str_prefix(arg, "cmd") || !str_prefix(arg, "command")) {
			argument = one_argument(argument, arg);
			disable_cmd(ch, argument, &victim->disabled);
		} else {
			disable_all(ch, argument, &victim->disabled);
		}
	} else if (!str_prefix(arg, "spell")) {
		argument = one_argument(argument, arg);
		disable_spell(ch, argument, &disabled_first);
		save_disabled();
	} else if (!str_prefix(arg, "cmd") || !str_prefix(arg, "command")) {
		argument = one_argument(argument, arg);
		disable_cmd(ch, argument, &disabled_first);
		save_disabled();
	} else {
		disable_all(ch, argument, &disabled_first);
		save_disabled();
	}

	return;
}

/***************************************************************************
*	disable_cmd
***************************************************************************/
void disable_cmd(CHAR_DATA *ch, char *argument, DISABLED_DATA **disabled_list)
{
	DISABLED_DATA *disabled;
	char cmd[MSL];

	one_argument(argument, cmd);
	for (disabled = *disabled_list; disabled != NULL; disabled = disabled->next) {
		if (disabled->type == DISABLED_CMD
		    && !str_prefix(argument, disabled->command))
			break;
	}

	if (disabled != NULL) {
		if (get_trust(ch) < disabled->level) {
			send_to_char("This command was disabled by a higher power.\n\r", ch);
			return;
		}

		if (*disabled_list == disabled) {
			*disabled_list = disabled->next;
		} else {
			DISABLED_DATA *list;

			for (list = *disabled_list; list->next != disabled; list = list->next) {
			}
			list->next = disabled->next;
		}

		free_disabled(disabled);
		send_to_char("Command enabled.\n\r", ch);
	} else {
		char arg[MSL];
		int iter;
		int level;

		argument = one_argument(argument, arg);

		if (!str_cmp(arg, "disable")) {
			send_to_char("You cannot disable the disable command.\n\r", ch);
			return;
		}

		for (iter = 0; cmd_table[iter].name[0] != '\0'; iter++)
			if (!str_cmp(cmd_table[iter].name, arg))
				break;

		if (cmd_table[iter].name[0] == '\0') {
			send_to_char("No such command.\n\r", ch);
			return;
		}

		if (cmd_table[iter].level > get_trust(ch)) {
			send_to_char("You dot have access to that command; you cannot disable it.\n\r", ch);
			return;
		}

		level = parse_int(argument);
		if (level <= LEVEL_IMMORTAL || level > MAX_LEVEL)
			level = get_trust(ch);

		disabled = new_disabled();
		disabled->command = str_dup(cmd_table[iter].name);
		disabled->disabled_by = str_dup(ch->name);
		disabled->level = (int)level;
		disabled->type = DISABLED_CMD;
		disabled->next = *disabled_list;
		*disabled_list = disabled;                      /* add before the current first element */

		send_to_char("Command disabled.\n\r", ch);
		save_disabled();
	}
}


/***************************************************************************
*	disable_spell
***************************************************************************/
void disable_spell(CHAR_DATA *ch, char *argument, DISABLED_DATA **disabled_list)
{
	DISABLED_DATA *disabled;
	char cmd[MSL];

	one_argument(argument, cmd);

	for (disabled = *disabled_list; disabled != NULL; disabled = disabled->next) {
		if (disabled->type == DISABLED_SPELL
		    && !str_prefix(argument, disabled->command))
			break;
	}

	if (disabled != NULL) {
		if (get_trust(ch) < disabled->level) {
			send_to_char("This spell was disabled by a higher power.\n\r", ch);
			return;
		}

		if (*disabled_list == disabled) {
			*disabled_list = disabled->next;
		} else {
			DISABLED_DATA *list;

			for (list = *disabled_list; list->next != disabled; list = list->next) {
			}
			list->next = disabled->next;
		}

		free_disabled(disabled);
		save_disabled();
		send_to_char("Spell enabled.\n\r", ch);
	} else {
		char arg[MSL];
		SKILL *skill;
		int level;

		argument = one_argument(argument, arg);

		if ((skill = skill_lookup(arg)) == NULL
		    || skill->spells == NULL) {
			send_to_char("No such spell.\n\r", ch);
			return;
		}

		level = parse_int(argument);
		if (level <= LEVEL_IMMORTAL || level > MAX_LEVEL)
			level = get_trust(ch);

		disabled = new_disabled();
		disabled->command = str_dup(skill->name);
		disabled->disabled_by = str_dup(ch->name);
		disabled->level = (int)level;
		disabled->type = DISABLED_SPELL;
		disabled->next = *disabled_list;
		*disabled_list = disabled;                      /* add before the current first element */

		send_to_char("Spell disabled.\n\r", ch);
		save_disabled();
	}
}


/***************************************************************************
*	disable_cmd
***************************************************************************/
void disable_all(CHAR_DATA *ch, char *argument, DISABLED_DATA **disabled_list)
{
	DISABLED_DATA *disabled;

	for (disabled = *disabled_list; disabled != NULL; disabled = disabled->next)
		if (!str_prefix(argument, disabled->command))
			break;

	if (disabled != NULL) {
		int type = disabled->type;

		if (get_trust(ch) < disabled->level) {
			send_to_char("This command was disabled by a higher power.\n\r", ch);
			return;
		}

		if (*disabled_list == disabled) {
			*disabled_list = disabled->next;
		} else {
			DISABLED_DATA *list;

			for (list = *disabled_list; list->next != disabled; list = list->next) {
			}
			list->next = disabled->next;
		}

		free_disabled(disabled);

		if (type == DISABLED_SPELL)
			send_to_char("Spell enabled.\n\r", ch);
		else
			send_to_char("Command enabled.\n\r", ch);
	} else {
		SKILL *skill;
		char arg[MSL];
		int iter;

		one_argument(argument, arg);

		for (iter = 0; cmd_table[iter].name[0] != '\0'; iter++)
			if (!str_cmp(cmd_table[iter].name, arg))
				break;

		if (cmd_table[iter].name[0] != '\0') {
			disable_cmd(ch, argument, disabled_list);
			return;
		}

		if ((skill = skill_lookup(arg)) != NULL
		    && skill->spells != NULL) {
			disable_spell(ch, argument, disabled_list);
			return;
		}

		send_to_char("No such command or spell.\n\r", ch);
	}
}

/***************************************************************************
*	disable_show
***************************************************************************/
void disable_show(CHAR_DATA *ch, DISABLED_DATA *list)
{
	DISABLED_DATA *disabled;

	if (list == NULL) {
		send_to_char("There are no commands disabled.\n\r", ch);
		return;
	}

	send_to_char("\n\rDisabled commands:\n\r"
		     "Command            Level   Disabled by    Type\n\r", ch);

	for (disabled = list; disabled != NULL; disabled = disabled->next) {
		printf_to_char(ch, "%-18.17s %3d     %-14s %-12s\n\r",
			       disabled->command,
			       disabled->level,
			       disabled->disabled_by,
			       (disabled->type == DISABLED_CMD) ? "command" : "spell");
	}
}

/***************************************************************************
*	load_disabled
***************************************************************************/
void load_disabled()
{
	DISABLED_DATA *disabled;
	SKILL *skill;
	FILE *fp;
	char *name;

	disabled_first = NULL;

	fp = fopen(DISABLED_FILE, "r");

	if (fp == NULL)
		return;

	name = fread_word(fp);

	while (str_cmp(name, END_MARKER) != 0) {
		int iter;
		int type;
		bool found;

		type = fread_number(fp);

		found = FALSE;
		switch (type) {
		case DISABLED_CMD:
			for (iter = 0; cmd_table[iter].name[0] != '\0'; iter++) {
				if (!str_cmp(cmd_table[iter].name, name)) {
					found = TRUE;
					break;
				}
			}
			break;
		case DISABLED_SPELL:
			if ((skill = skill_lookup(name)) != NULL)
				found = TRUE;
			break;
		}

		if (!found) {
			bug("Skipping uknown command in " DISABLED_FILE " file.", 0);
			fread_number(fp);       /* level */
			fread_word(fp);         /* disabled_by */
		} else {
			disabled = new_disabled();
			disabled->command = str_dup(name);
			disabled->type = (int)type;
			disabled->level = (int)fread_number(fp);
			disabled->disabled_by = str_dup(fread_word(fp));
			disabled->next = disabled_first;
			disabled_first = disabled;
		}

		name = fread_word(fp);
	}

	fclose(fp);
}

/***************************************************************************
*	save_disabled
***************************************************************************/
void save_disabled()
{
	DISABLED_DATA *disabled;
	FILE *fp;

	if (disabled_first == NULL) {
		unlink(DISABLED_FILE);
		return;
	}

	fp = fopen(DISABLED_FILE, "w");

	if (fp == NULL) {
		bug("Could not open " DISABLED_FILE " for writing", 0);
		return;
	}

	for (disabled = disabled_first; disabled != NULL; disabled = disabled->next) {
		fprintf(fp, "'%s' %d %d %s\n",
			disabled->command,
			disabled->type,
			disabled->level,
			disabled->disabled_by);
	}

	fprintf(fp, "%s\n", END_MARKER);

	fclose(fp);
}





/***************************************************************************
*	player_check_disabled
*
*	checks to see if a command or spell is disabled for a player
***************************************************************************/
static bool player_check_disabled(CHAR_DATA *ch, int type, char *name)
{
	DISABLED_DATA *disabled;
	bool is_disabled = FALSE;

	for (disabled = ch->disabled; disabled != NULL; disabled = disabled->next) {
		if (disabled->type == (int)type
		    && !str_cmp(name, disabled->command)) {
			is_disabled = TRUE;
			break;
		}
	}

	return is_disabled;
}

/***************************************************************************
*	check_disabled
***************************************************************************/
bool check_disabled(CHAR_DATA *ch, int type, char *name)
{
	DISABLED_DATA *disabled;
	bool is_disabled = FALSE;

	for (disabled = disabled_first; disabled != NULL; disabled = disabled->next) {
		if (disabled->type == (int)type
		    && !str_cmp(name, disabled->command)) {
			is_disabled = TRUE;
			break;
		}
	}

	if (!is_disabled)
		is_disabled = battlefield_check_disabled(ch, (int)type, name);

	if (!is_disabled)
		is_disabled = player_check_disabled(ch, type, name);

	return is_disabled;
}
