#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "lookup.h"
#include "recycle.h"
#include "skills.h"
#include "libfile.h"


/***************************************************************************
 *	local defines
 ***************************************************************************/
const struct olc_cmd_type skedit_table[] =
{
    /*	{	command			function			}, */

    { "commands",	    show_commands	  },
    { "show",	    skedit_show		  },
    { "?",		    show_help		  },
    { "help",	    show_help		  },
    { "new",	    skedit_new		  },
    /*{	"delete",			skedit_delete			},*/
    { "level",	    skedit_level	  },
    { "mana",	    skedit_min_mana	  },
    { "wait",	    skedit_wait		  },
    { "position",	    skedit_min_pos	  },
    { "damage",	    skedit_damage_msg	  },
    { "target",	    skedit_target	  },
    { "message",	    skedit_message	  },
    { "obj_message",    skedit_obj_message	  },
    { "others_message", skedit_others_message },
    { "helpfile",	    skedit_help		  },
    { "spell",	    skedit_spell	  },
    { "affect",	    skedit_affect	  },
    { "argument",	    skedit_argument	  },
    { "flags",	    skedit_flags	  },
    { "difficulty",	    skedit_difficulty	  },

    /* mob stuff */
    { "percent",	    skedit_percent	  },
    { "act_flag",	    skedit_act_flag	  },
    { "off_flag",	    skedit_off_flag	  },

    { NULL,		    0			  }
};





/***************************************************************************
 *	skedit
 *
 *	command interpreter for the skill editor
 ***************************************************************************/
void skedit(CHAR_DATA *ch, const char *argument)
{
    char arg[MIL];
    const char *parg;
    char command[MIL];
    int cmd;

    strcpy(arg, argument);
    smash_tilde(arg);

    parg = one_argument(arg, command);

    if (ch->pcdata->security < 9) {
	send_to_char("SKEdit: Insuficient security to edit skills.\n\r", ch);
	edit_done(ch);
	return;
    }

    if (command[0] == '\0') {
	skedit_show(ch, parg);
	return;
    }

    if (!str_cmp(command, "done")) {
	edit_done(ch);
	return;
    }

    for (cmd = 0; skedit_table[cmd].name != NULL; cmd++) {
	if (!str_prefix(command, skedit_table[cmd].name)) {
	    (*skedit_table[cmd].olc_fn)(ch, parg);
	    return;
	}
    }

    interpret(ch, arg);
}


/***************************************************************************
 *	do_skedit
 *
 *	entry point for the skill editor
 ***************************************************************************/
void do_skedit(CHAR_DATA *ch, const char *argument)
{
    SKILL *skill;
    char arg[MSL];

    if (IS_NPC(ch))
	return;

    one_argument(argument, arg);
    if (!str_prefix(arg, "new")) {
	argument = one_argument(argument, arg);
	skedit_new(ch, argument);
    }

    if ((skill = skill_lookup(argument)) == NULL) {
	send_to_char("SKEdit : Skill does not exist.\n\r", ch);
	return;
    }

    ch->desc->ed_data = (void *)skill;
    ch->desc->editor = ED_SKILL;

    return;
}



/***************************************************************************
 *	general commands
 ***************************************************************************/
/***************************************************************************
 *	skedit_delete
 *
 *	edit the levels of the skill
 ***************************************************************************/
EDIT(skedit_delete){
    SKILL *skill;
    SKILL *skill_idx;
    SKILL *skill_prev = NULL;

    EDIT_SKILL(ch, skill);

    /* unlink the list */
    if (skill == skill_list) {
	skill_list = skill->next;
    } else {
	for (skill_idx = skill_list;
		skill_idx != NULL;
		skill_idx = skill_idx->next) {
	    if (skill_idx == skill)
		break;
	    skill_prev = skill_idx;
	}

	if (skill_prev != NULL && skill_idx != NULL)
	    skill_prev->next = skill->next;
    }

    free_skill(skill);
    edit_done(ch);

    send_to_char("Ok.\n\r", ch);
    return true;
}



/***************************************************************************
 *	skedit_new
 *
 *	create a new skill
 ***************************************************************************/
EDIT(skedit_new){
    SKILL *skill;

    if (is_help(argument)) {
	send_to_char("Syntax   : new [name]\n\r", ch);
	return false;
    }

    if ((skill = skill_lookup(argument)) != NULL) {
	send_to_char("SKedit : skill already exists.\n\r", ch);
	return false;
    }

    skill = new_skill();
    skill->name = str_dup(argument);
    skill->sn = (++gn_max_skill_sn);

    skill->next = skill_list;
    skill_list = skill;

    resolve_global_hash();

    ch->desc->ed_data = (SKILL *)skill;
    ch->desc->editor = ED_SKILL;

    send_to_char("Ok.\n\r", ch);
    return false;
}


/***************************************************************************
 *	skedit_list
 *
 *	show the list of helps
 ***************************************************************************/
EDIT(skedit_list){
    SKILL *skill;
    SKILL *list;
    BUFFER *buffer;
    int cnt = 0;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax : list [partial name]\n\r", ch);
	return false;
    }

    buffer = new_buf();

    for (list = skill_list; list != NULL; list = list->next) {
	if ((argument[0] == '\0') || !str_infix(argument, skill->name)) {
	    printf_buf(buffer, "%3d. %-14.14s%s", cnt, skill->name, (cnt % 4 == 3) ? "\n\r" : " ");
	    cnt++;
	}
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);

    return false;
}

/***************************************************************************
 *	skedit_show
 *
 *	show the properties for the skill
 ***************************************************************************/
EDIT(skedit_show){
    SKILL *skill;
    char buf[MSL];

    EDIT_SKILL(ch, skill);

    /* general information */
    printf_to_char(ch, "\n\r`&Name``:        [%s]\n\r", skill->name);
    printf_to_char(ch, "`&Slot``:        [%d]\n\r", skill->sn);
    printf_to_char(ch, "`&Type``:        [%s]\n\r", (skill->spells == NULL) ? "skill" : "spell");
    printf_to_char(ch, "`&Target``:      [%s]\n\r", flag_string(target_flags, skill->target));
    printf_to_char(ch, "`&Wait``:        [%d]\n\r", skill->wait);
    printf_to_char(ch, "`&Min Pos``:     [%s]\n\r", flag_string(position_flags, skill->min_pos));
    printf_to_char(ch, "`&Min Mana``:    [%d]\n\r", skill->min_mana);
    printf_to_char(ch, "`&Damage Msg``:  [%s]\n\r", (skill->dam_noun != NULL) ? skill->dam_noun : "none");

    /* spell/affects */
    if (skill->spells != NULL) {
	SPELL_LIST *spells;
	bool first;

	first = true;
	for (spells = skill->spells; spells != NULL; spells = spells->next) {
	    if (spells->spell_fn != NULL) {
		if (first) {
		    printf_to_char(ch, "`&Spell Fn``:    [%s]\n\r", spell_fn_name(spells->spell_fn));
		    first = false;
		} else {
		    printf_to_char(ch, "             [%s]\n\r", spell_fn_name(spells->spell_fn));
		}
	    }
	}
    } else {
	send_to_char("`&Spell Fn``:    [none]\n\r", ch);
    }

    if (skill->affects != NULL) {
	AFFECT_LIST *affects;
	bool first;

	first = true;
	for (affects = skill->affects; affects != NULL; affects = affects->next) {
	    if (affects->affect_fn != NULL) {
		if (first) {
		    printf_to_char(ch, "`&Affect Fn``:   [%s]\n\r", affect_fn_name(affects->affect_fn));
		    first = false;
		} else {
		    printf_to_char(ch, "             [%s]\n\r", affect_fn_name(affects->affect_fn));
		}
	    }
	}
    } else {
	send_to_char("`&Affect Fn``:   [none]\n\r", ch);
    }

    /* messages */
    printf_to_char(ch, "`&Strip Msg``:   [%s]\n\r", (skill->msg != NULL) ? skill->msg : "none");
    printf_to_char(ch, "`&Others Msg``:  [%s]\n\r", (skill->msg_others != NULL) ? skill->msg_others : "none");
    printf_to_char(ch, "`&Object Msg``:  [%s]\n\r", (skill->msg_obj != NULL) ? skill->msg_obj : "none");
    printf_to_char(ch, "`&Help``:        [%s]\n\r", (skill->help != NULL) ? skill->help->keyword : "none");

    if (skill->args != NULL) {
	ARGUMENT *args;
	send_to_char("\n\r`!OPTIONAL ARGUMENTS``\n\r", ch);
	send_to_char("`1================================================\n\r", ch);

	for (args = skill->args; args != NULL; args = args->next)
	    if (args->data != NULL && VALIDATE_VARIANT(args->data, VARIANT_STRING))
		printf_to_char(ch, "`&%s``:   [%s]\n\r", args->key, (char *)args->data->data);

    }



    /* flags - mostly for brew/scribe */
    send_to_char("\n\r`!FLAGS``\n\r", ch);
    send_to_char("`1================================================\n\r", ch);
    printf_to_char(ch, "`&Difficulty``:  [%d]\n\r", skill->difficulty);
    printf_to_char(ch, "`&Flags``:       [%s]\n\r", flag_string(skill_flags, skill->flags));

    /* mob information */
    send_to_char("\n\r`!MOB DATA``\n\r", ch);
    send_to_char("`1================================================\n\r", ch);
    printf_to_char(ch, "`&Act Flag``:    [%s]\n\r", flag_string(act_flags, skill->act_flag));
    printf_to_char(ch, "`&Off Flag``:    [%s]\n\r", flag_string(off_flags, skill->off_flag));
    printf_to_char(ch, "`&Percent``:     [%d]\n\r", skill->percent);

    send_to_char("\n\r`!LEVELS``\n\r", ch);
    send_to_char("`1================================================\n\r", ch);

    send_to_char("`!Class        Level           Cost\n\r", ch);
    send_to_char("`1------------------------------------------------\n\r", ch);
    if (skill->levels != NULL) {
	LEVEL_INFO *levels;

	for (levels = skill->levels; levels != NULL; levels = levels->next) {
	    sprintf(buf, "%s``:", capitalize(class_table[levels->class].name));
	    printf_to_char(ch, "`&%-11.11s    [%3d]           [%3d]\n\r", buf, levels->level, levels->difficulty);
	}
    }

    return false;
}



/***************************************************************************
 *	skedit_level
 *
 *	edit the levels of the skill
 ***************************************************************************/
EDIT(skedit_level){
    SKILL *skill;
    LEVEL_INFO *level_info;
    LEVEL_INFO *level_idx;
    char arg[MSL];
    int cls;
    int level;
    int difficulty;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax:  level <class> <level|none> [difficulty]\n\r", ch);
	return false;
    }

    argument = one_argument(argument, arg);
    if ((cls = class_lookup(arg)) <= -1) {
	send_to_char("The chosen class is invalid.  Please check the name and try again.\n\r", ch);
	return false;
    }

    if (!str_prefix(argument, "none")) {
	level = -1;
	difficulty = -1;
    } else {
	argument = one_argument(argument, arg);

	level = -1;
	difficulty = -1;
	if (is_number(arg)) {
	    level = parse_int(arg);
	} else {
	    send_to_char("The level must be a number or 'none'.\n\r", ch);
	    return false;
	}

	if (is_number(argument))
	    difficulty = parse_int(argument);
    }

    /*
     * we have a valid class and a level - see if a
     * level for that class already exists
     */
    level_info = NULL;
    for (level_idx = skill->levels;
	    level_idx != NULL;
	    level_idx = level_idx->next) {
	if (level_idx->class == cls) {
	    level_info = level_idx;
	    break;
	}
    }

    if (level > -1) {
	/*
	 * we did not find an existing class specifier
	 * for the skill, so create a new one
	 */
	if (level_info == NULL) {
	    level_info = new_level_info();
	    level_info->class = cls;
	}

	if (level_info == NULL) {
	    send_to_char("Error creating LEVEL_INFO structure.\n\r", ch);
	    return false;
	}

	level_info->level = level;
	level_info->difficulty = difficulty;

	add_skill_level(skill, level_info);
    } else {
	LEVEL_INFO *level_prev = NULL;

	if (level_info == NULL) {
	    send_to_char("That level information does not exist.\n\r", ch);
	    return false;
	}

	/* unlink the level info structure */
	if (level_info == skill->levels) {
	    skill->levels = level_info->next;
	} else {
	    for (level_idx = skill->levels; level_idx != NULL; level_idx = level_idx->next) {
		if (level_idx == level_info)
		    break;
		level_prev = level_idx;
	    }

	    if (level_prev != NULL)
		level_prev->next = level_info->next;
	}

	/* it is no longer in the list, recycle the memory */
	free_level_info(level_info);
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}




/***************************************************************************
 *	skedit_min_mana
 *
 *	edit the min_mana of the skill
 ***************************************************************************/
EDIT(skedit_min_mana){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument) || !is_number(argument)) {
	send_to_char("Syntax:  mana [number]\n\r", ch);
	return false;
    }

    skill->min_mana = parse_int(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}



/***************************************************************************
 *	skedit_wait
 *
 *	edit the wait of the skill
 ***************************************************************************/
EDIT(skedit_wait){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument) || !is_number(argument)) {
	send_to_char("Syntax:  wait [number]\n\r", ch);
	return false;
    }

    skill->wait = parse_int(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

/***************************************************************************
 *	skedit_min_pos
 *
 *	edit the wait of the skill
 ***************************************************************************/
EDIT(skedit_min_pos){
    SKILL *skill;
    int value;

    EDIT_SKILL(ch, skill);


    if (is_help(argument)
	    || (value = flag_value(position_flags, argument)) == NO_FLAG) {
	send_to_char("Syntax:  position [position]\n\r"
		"Type '? position' for a list of positions.\n\r", ch);
	return false;
    }

    skill->min_pos = value;

    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	skedit_target
 *
 *	edit the wait of the skill
 ***************************************************************************/
EDIT(skedit_target){
    SKILL *skill;
    int value;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)
	    || (value = flag_value(target_flags, argument)) == NO_FLAG) {
	send_to_char("Syntax:  target [target]\n\r"
		"Type '? target' for a list of skill targets.\n\r", ch);
	return false;
    }

    skill->target = value;

    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	skedit_damage_msg
 *
 *	edit the wait of the skill
 ***************************************************************************/
EDIT(skedit_damage_msg){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax:  damage [damage message]\n\r", ch);
	return false;
    }

    if (skill->dam_noun != NULL)
	free_string(skill->dam_noun);

    skill->dam_noun = str_dup(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

/***************************************************************************
 *	skedit_message
 *
 *	edit the wear-off message of the skill
 ***************************************************************************/
EDIT(skedit_message){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax:  message [wear-off message]\n\r", ch);
	return false;
    }

    if (skill->msg != NULL)
	free_string(skill->msg);

    skill->msg = str_dup(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

/***************************************************************************
 *	skedit_obj_message
 *
 *	edit the wear-off message of the skill
 ***************************************************************************/
EDIT(skedit_obj_message){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax:  obj_message [object wear-off message]\n\r", ch);
	return false;
    }

    if (skill->msg_obj != NULL)
	free_string(skill->msg_obj);

    skill->msg_obj = str_dup(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	skedit_others_message
 *
 *	edit the wear-off message displayed to the room
 *	of the skill
 ***************************************************************************/
EDIT(skedit_others_message){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax:  others_message [wear-off message to room]\n\r", ch);
	return false;
    }

    if (skill->msg_others != NULL)
	free_string(skill->msg_others);

    skill->msg_others = str_dup(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

/***************************************************************************
 *	skedit_spell
 *
 *	edit the spell list for the skill
 ***************************************************************************/
EDIT(skedit_spell){
    SKILL *skill;
    SPELL_FUN *spell;
    SPELL_LIST *spells;
    char cmd[MIL];

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("\n\r", ch);
	send_to_char("Syntax:  spell <add|remove> <spell name>\n\r", ch);
	send_to_char("         spell <list> [all|partiall spell name]\n\r\n\r", ch);
	return false;
    }

    argument = one_argument(argument, cmd);
    if (!str_prefix(cmd, "add")) {
	if ((spell = spell_fn_lookup(argument)) == NULL) {
	    send_to_char("That spell does not exist.  Type 'spell list' for a list of spells.\n\r", ch);
	    return false;
	}

	add_spell(skill, spell);
    } else if (!str_prefix(cmd, "remove")) {
	if (skill->spells == NULL) {
	    send_to_char("That skill does not have any spells associated with it.\n\r", ch);
	    return false;
	}

	if (!str_prefix(argument, "all")) {
	    SPELL_LIST *spells_next;

	    for (spells = skill->spells; spells != NULL; spells = spells_next) {
		spells_next = spells->next;
		free_spell_list(spells);
	    }
	    skill->spells = NULL;
	} else {
	    if ((spell = spell_fn_lookup(argument)) == NULL) {
		send_to_char("That spell does not exist.  Type 'spell list' for a list of spells.\n\r", ch);
		return false;
	    }

	    if (skill->spells->spell_fn == spell) {
		spells = skill->spells;
		skill->spells = spells->next;

		free_spell_list(spells);
	    } else {
		SPELL_LIST *spells_prev = NULL;

		for (spells = skill->spells; spells != NULL; spells = spells->next) {
		    if (spells->spell_fn == spell)
			break;
		    spells_prev = spells;
		}

		if (spells == NULL) {
		    printf_to_char(ch, "The spell is not set on this skill. Spell: %s\n\r", spell_fn_name(spell));
		    return false;
		}

		if (spells_prev != NULL) {
		    spells_prev->next = spells->next;
		    free_spell_list(spells);
		}
	    }
	}
    } else if (!str_prefix(cmd, "list")) {
	int col;
	int idx;
	bool show_all;

	col = 0;
	show_all = (argument[0] == '\0' || !str_prefix(argument, "all"));
	for (idx = 0; spell_lookup_table[idx].name[0] != '\0'; idx++) {
	    if (show_all || !str_prefix(argument, spell_lookup_table[idx].name)) {
		printf_to_char(ch, "%-18.18s", spell_lookup_table[idx].name);
		if (++col % 3 == 0)
		    send_to_char("\n\r", ch);
	    }
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);
    } else {
	return skedit_spell(ch, "");
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}



/***************************************************************************
 *	skedit_affect
 *
 *	edit the affect list for the skill
 ***************************************************************************/
EDIT(skedit_affect){
    SKILL *skill;
    AFFECT_FUN *affect;
    AFFECT_LIST *affects;
    char cmd[MIL];

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("\n\r", ch);
	send_to_char("Syntax:  affect <add|remove> <affect function name>\n\r", ch);
	send_to_char("         affect <list> [all|partial affect name]\n\r\n\r", ch);
	return false;
    }

    argument = one_argument(argument, cmd);
    if (!str_prefix(cmd, "add")) {
	if ((affect = affect_fn_lookup(argument)) == NULL) {
	    send_to_char("That affect does not exist.  Type 'affect list' for a list of affects.\n\r", ch);
	    return false;
	}

	add_affect(skill, affect);
    } else if (!str_prefix(cmd, "remove")) {
	if (skill->affects == NULL) {
	    send_to_char("That skill does not have any affects associated with it.\n\r", ch);
	    return false;
	}

	if (!str_prefix(argument, "all")) {
	    AFFECT_LIST *affects_next;

	    for (affects = skill->affects; affects != NULL; affects = affects_next) {
		affects_next = affects->next;
		free_affect_list(affects);
	    }
	    skill->affects = NULL;
	} else {
	    if ((affect = affect_fn_lookup(argument)) == NULL) {
		send_to_char("That affect does not exist.  Type 'affect list' for a list of affects.\n\r", ch);
		return false;
	    }

	    if (skill->affects->affect_fn == affect) {
		affects = skill->affects;
		skill->affects = affects->next;

		free_affect_list(affects);
	    } else {
		AFFECT_LIST *affects_prev = NULL;

		for (affects = skill->affects; affects != NULL; affects = affects->next) {
		    if (affects->affect_fn == affect)
			break;
		    affects_prev = affects;
		}

		if (affects == NULL) {
		    printf_to_char(ch, "The affect is not set on this skill. Affect: %s\n\r", affect_fn_name(affect));
		    return false;
		}

		if (affects_prev != NULL) {
		    affects_prev->next = affects->next;
		    free_affect_list(affects);
		}
	    }
	}
    } else if (!str_prefix(cmd, "list")) {
	int idx;
	int col;
	bool show_all;

	col = 0;
	show_all = (argument[0] == '\0' || !str_prefix(argument, "all"));
	for (idx = 0; affect_lookup_table[idx].name[0] != '\0'; idx++) {
	    if (show_all || !str_prefix(argument, affect_lookup_table[idx].name)) {
		printf_to_char(ch, "%-18.18s", affect_lookup_table[idx].name);
		if (++col % 3 == 0)
		    send_to_char("\n\r", ch);
	    }
	}

	if (col % 3 != 0)
	    send_to_char("\n\r", ch);

	return false;
    } else {
	return skedit_affect(ch, "");
    }


    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	skedit_argument
 *
 *	edit the argument list for the skill
 ***************************************************************************/
EDIT(skedit_argument){
    SKILL *skill;
    ARGUMENT *arg;
    char cmd[MIL];

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("\n\rSyntax:  argument [add] [argument name] [value]\n\r", ch);
	send_to_char("Syntax:  argument [remove] [argument name]\n\r\n\r", ch);
	return false;
    }

    argument = one_argument(argument, cmd);
    if (!str_prefix(cmd, "add")) {
	char key[MIL];

	argument = one_argument(argument, key);
	if (argument[0] == '\0') {
	    send_to_char("You must provide a value.\n\r", ch);
	    return false;
	}

	arg = new_argument();
	arg->key = str_dup(key);
	arg->data = new_variant();
	set_variant(arg->data, VARIANT_STRING, str_dup(argument));

	add_argument(skill, arg);
    } else if (!str_prefix(cmd, "remove")) {
	if (skill->args == NULL) {
	    send_to_char("That skill does not have any arguments associated with it.\n\r", ch);
	    return false;
	}

	if (!str_prefix(argument, "all")) {
	    ARGUMENT *arg_next;

	    for (arg = skill->args; arg != NULL; arg = arg_next) {
		arg_next = arg->next;
		free_argument(arg);
	    }
	    skill->args = NULL;
	} else {
	    if (argument[0] == '\0') {
		send_to_char("You must supply a key name.\n\r", ch);
		return false;
	    }

	    if (!str_prefix(argument, skill->args->key)) {
		arg = skill->args;
		skill->args = arg->next;

		free_argument(arg);
	    } else {
		ARGUMENT *arg_prev = NULL;

		for (arg = skill->args; arg != NULL; arg = arg->next) {
		    if (!str_prefix(argument, arg->key))
			break;

		    arg_prev = arg;
		}
		if (arg == NULL) {
		    printf_to_char(ch, "The argument is not set on this skill. Argument: %s\n\r", argument);
		    return false;
		}

		if (arg_prev != NULL) {
		    arg_prev->next = arg->next;
		    free_argument(arg);
		}
	    }
	}
    } else {
	return skedit_argument(ch, "");
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}




/***************************************************************************
 *	skedit_help
 *
 *	edit the help file for the skill
 ***************************************************************************/
EDIT(skedit_help){
    SKILL *skill;
    HELP_DATA *help;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)) {
	send_to_char("Syntax:  helpfile [keyword of help]\n\r", ch);
	return false;
    }

    if ((help = help_lookup(argument)) == NULL) {
	send_to_char("That help does not exist.\n\r", ch);
	return false;
    }

    if (skill->help_keyword != NULL)
	free_string(skill->help_keyword);
    skill->help_keyword = str_dup(help->keyword);
    skill->help = help;

    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	brew/scribe stuff - could be used for a number of things
 ***************************************************************************/
/***************************************************************************
 *	skedit_flags
 *
 *	set the flags for a skill - currently, it only supports
 *	nobrew and noscribe but it could be used for a lot more
 ***************************************************************************/
EDIT(skedit_flags){
    SKILL *skill;
    int value;

    EDIT_SKILL(ch, skill);

    if (is_help(argument)
	    || (value = flag_value(skill_flags, argument)) == NO_FLAG) {
	send_to_char("Syntax:  flag [flag]\n\r"
		"Type '? skill-flags' for a list of skill flags.\n\r", ch);
	return false;
    }

    skill->flags ^= value;

    send_to_char("Ok.\n\r", ch);
    return true;
}

/***************************************************************************
 *	skedit_difficulty
 *
 *	used by brew/scribe to help determine how hard it is
 *	to brew/scribe a spell...but could be used in several
 *	other places
 ***************************************************************************/
EDIT(skedit_difficulty){
    SKILL *skill;
    int value;

    EDIT_SKILL(ch, skill);
    if (is_help(argument) || !is_number(argument)) {
	send_to_char("Syntax:  difficulty [number]\n\rReflects how hard a skill is to use.", ch);
	return false;
    }

    value = parse_int(argument);
    if (value < 1 || value > 10) {
	send_to_char("The number must be between 1 and 10.\n\r", ch);
	return false;
    }

    skill->difficulty = value;

    send_to_char("Ok.\n\r", ch);
    return true;
}




/***************************************************************************
 *	mob specific attributes
 ***************************************************************************/
/***************************************************************************
 *	skedit_percent
 *
 *	sets the mob's base percentage for using a skill
 ***************************************************************************/
EDIT(skedit_percent){
    SKILL *skill;

    EDIT_SKILL(ch, skill);

    if (is_help(argument) || !is_number(argument)) {
	send_to_char("Syntax:  percent [number]\n\r\n\rSets a mob's base percentage for using a skill.\n\r", ch);
	return false;
    }

    skill->percent = parse_int(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}



/***************************************************************************
 *	skedit_act_flag
 *
 *	set the act flag necessary for a mob to use a skill
 ***************************************************************************/
EDIT(skedit_act_flag){
    SKILL *skill;
    int value;

    EDIT_SKILL(ch, skill);


    if (is_help(argument)
	    || (value = flag_value(act_flags, argument)) == NO_FLAG) {
	send_to_char("Syntax:  act_flag [flag]\n\r"
		"Type '? act' for a list of act flags.\n\r", ch);
	return false;
    }

    skill->act_flag = value;

    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	skedit_off_flag
 *
 *	set the off flag necessary for a mob to use a skill
 ***************************************************************************/
EDIT(skedit_off_flag){
    SKILL *skill;
    int value;

    EDIT_SKILL(ch, skill);


    if (is_help(argument)
	    || (value = flag_value(off_flags, argument)) == NO_FLAG) {
	send_to_char("Syntax:  off_flag [flag]\n\r"
		"Type '? off' for a list of off flags.\n\r", ch);
	return false;
    }

    skill->off_flag = value;

    send_to_char("Ok.\n\r", ch);
    return true;
}





/***************************************************************************
 *	load/save skills
 ***************************************************************************/
#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)              \
    if (!str_cmp(word, literal))                     \
{                                                                       \
    field = value;                                \
    found = true;                                 \
}


/***************************************************************************
 *	file functions
 ***************************************************************************/
/***************************************************************************
 *	load_skills
 *
 *	load the skill table from a file
 ***************************************************************************/
void load_skills()
{
    FILE *fp;
    SKILL *skill = NULL;
    char *word;
    int sn;
    bool found;

    gn_max_skill_sn = 0;

    if ((fp = fopen(SKILL_FILE, "r")) == NULL) {
	perror("load_skills: could not open skill file.");
	return;
    }

    word = fread_word(fp);
    found = false;

    while (!feof(fp) && str_cmp(word, END_MARKER)) {
	if (!str_cmp(word, "sn")) {
	    sn = fread_number(fp);

	    skill = new_skill();
	    skill->next = skill_list;
	    skill_list = skill;

	    skill->sn = sn;
	    gn_max_skill_sn = UMAX(gn_max_skill_sn, sn);
	    found = true;
	} else {
	    if (skill == NULL) {
		log_bug("load_skills: No skill loaded - invalid file syntax. %s", word);
		raise(SIGABRT);
	    }

	    KEY("Name", skill->name, fread_string(fp));
	    KEY("Target", skill->target, fread_number(fp));
	    KEY("Pos", skill->min_pos, fread_number(fp));
	    KEY("Mana", skill->min_mana, fread_number(fp));
	    KEY("Wait", skill->wait, fread_number(fp));
	    KEY("Flags", skill->flags, fread_flag(fp));
	    KEY("Diff", skill->difficulty, fread_number(fp));
	    KEY("Dam", skill->dam_noun, fread_string(fp));
	    KEY("Msg", skill->msg, fread_string(fp));
	    KEY("MobAct", skill->act_flag, fread_flag(fp));
	    KEY("MobOff", skill->off_flag, fread_flag(fp));
	    KEY("Percent", skill->percent, fread_number(fp));
	    KEY("MsgObj", skill->msg_obj, fread_string(fp));
	    KEY("MsgOther", skill->msg_others, fread_string(fp));

	    if (!str_cmp(word, "SpFn")) {
		SPELL_FUN *spell;

		spell = spell_fn_lookup(fread_string(fp));
		if (spell != NULL)
		    add_spell(skill, spell);

		found = true;
	    }

	    if (!str_cmp(word, "AfFn")) {
		AFFECT_FUN *affect;

		affect = affect_fn_lookup(fread_string(fp));
		if (affect != NULL)
		    add_affect(skill, affect);

		found = true;
	    }


	    if (!str_cmp(word, "Arg")) {
		ARGUMENT *arg;
		char *data;

		arg = new_argument();
		arg->key = str_dup(fread_word(fp));
		data = fread_string(fp);
		arg->data = new_variant();

		set_variant(arg->data, VARIANT_STRING, data);

		add_argument(skill, arg);
		found = true;
	    }



	    if (!str_cmp(word, "Lvl")) {
		LEVEL_INFO *level;

		level = new_level_info();

		level->class = fread_number(fp);
		level->level = fread_number(fp);
		level->difficulty = fread_number(fp);

		add_skill_level(skill, level);

		found = true;
	    }
	}

	if (!str_cmp(word, "Help")) {
	    skill->help_keyword = fread_string(fp);
	    skill->help = help_lookup(skill->help_keyword);
	    found = true;
	}

	if (!found) {
	    log_bug("load_skills: No skill loaded - invalid file syntax. %s", word);
	    raise(SIGABRT);
	}

	word = fread_word(fp);
	found = false;
    }
}


/***************************************************************************
 *	save_skills
 *
 *	save the skill table to a file
 ***************************************************************************/
void save_skills()
{
    FILE *fp;
    SKILL *skill;
    char *tmp;

    if ((fp = fopen(SKILL_FILE, "w")) == NULL) {
	log_bug("save_skills: fopen");
	return;
    }

    for (skill = skill_list; skill != NULL; skill = skill->next) {
	fprintf(fp, "Sn %d\n", skill->sn);
	fprintf(fp, "Name %s~\n", skill->name);

	if (skill->spells != NULL) {
	    SPELL_LIST *spells;

	    for (spells = skill->spells; spells != NULL; spells = spells->next) {
		tmp = spell_fn_name(spells->spell_fn);
		if (tmp[0] != '\0')
		    fprintf(fp, "SpFn %s~\n", tmp);
	    }
	}

	if (skill->affects != NULL) {
	    AFFECT_LIST *affects;

	    for (affects = skill->affects; affects != NULL; affects = affects->next) {
		tmp = affect_fn_name(affects->affect_fn);
		if (tmp[0] != '\0')
		    fprintf(fp, "AfFn %s~\n", tmp);
	    }
	}

	if (skill->levels != NULL) {
	    LEVEL_INFO *level;

	    for (level = skill->levels; level != NULL; level = level->next)
		fprintf(fp, "Lvl %d %d %d\n", level->class, level->level, level->difficulty);
	}

	if (skill->args != NULL) {
	    ARGUMENT *arg;
	    for (arg = skill->args; arg != NULL; arg = arg->next)
		if (arg->data != NULL && VALIDATE_VARIANT(arg->data, VARIANT_STRING))
		    fprintf(fp, "Arg '%s' %s~\n", arg->key, (char *)arg->data->data);
	}
	fprintf(fp, "Target %d\n", skill->target);
	fprintf(fp, "Pos %d\n", skill->min_pos);
	fprintf(fp, "Mana %d\n", skill->min_mana);
	fprintf(fp, "Wait %d\n", skill->wait);

	if (skill->dam_noun != NULL
		&& skill->dam_noun[0] != '\0')
	    fprintf(fp, "Dam %s~\n", skill->dam_noun);

	if (skill->msg != NULL
		&& skill->msg[0] != '\0')
	    fprintf(fp, "Msg %s~\n", skill->msg);

	if (skill->msg_obj != NULL
		&& skill->msg_obj[0] != '\0')
	    /* i dont think there are many of these - break here and see where it applies*/
	    fprintf(fp, "MsgObj %s~\n", skill->msg_obj);

	if (skill->msg_others != NULL
		&& skill->msg_others[0] != '\0')
	    /* i dont think there are many of these - break here and see where it applies*/
	    fprintf(fp, "MsgOther %s~\n", skill->msg_others);

	if (skill->help != NULL)
	    fprintf(fp, "Help %s~\n", skill->help_keyword);

	fprintf(fp, "Flags %s\n", print_flags(skill->flags));
	fprintf(fp, "Diff %d\n", skill->difficulty);
	fprintf(fp, "MobAct %s\n", print_flags(skill->act_flag));
	fprintf(fp, "MobOff %s\n", print_flags(skill->off_flag));
	fprintf(fp, "Percent %d\n", skill->percent);
    }

    fprintf(fp, "END\n");
    fclose(fp);
}




/***************************************************************************
 *	assign_skill_helps
 *
 *	the skills have helps attached to them but the skills are
 *	loaded before the helps are so we have to retro-actively go
 *	back and assign the pointers
 ***************************************************************************/
void assign_skill_helps()
{
    SKILL *skill;
    GROUP *group;

    /* assign helps for skills */
    for (skill = skill_list; skill != NULL; skill = skill->next)
	if (skill->help == NULL)
	    skill->help = (skill->help_keyword != NULL) ? help_lookup(skill->help_keyword) : help_lookup(skill->name);

    /* assign helps for groups */
    for (group = group_list; group != NULL; group = group->next)
	if (group->help == NULL)
	    group->help = (group->help_keyword != NULL) ? help_lookup(group->help_keyword) : help_lookup(group->name);
}
