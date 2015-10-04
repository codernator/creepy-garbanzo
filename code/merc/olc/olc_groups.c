#include <stdlib.h>
#include <stdio.h>
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
const struct olc_cmd_type gredit_table[] =
{
    /*	{	command			function			}, */
    { "commands", show_commands },
    { "show",     gredit_show   },
    { "?",	      show_olc_help },
    { "help",     show_olc_help },
    { "new",      gredit_new    },
    { "delete",   gredit_delete },
    { "helpfile", gredit_help   },
    { "skills",   gredit_skills },
    { "cost",     gredit_cost   },
    { NULL,	      0		    }
};




/***************************************************************************
 *	gredit
 *
 *	command interpreter for the group editor
 ***************************************************************************/
void gredit(CHAR_DATA *ch, const char *argument)
{
    char arg[MIL];
    const char *parg;
    char command[MIL];
    int cmd;

    strcpy(arg, argument);
    smash_tilde(arg);

    parg = one_argument(arg, command);

    if (ch->pcdata->security < 9) {
        send_to_char("GRedit: insuficient security to edit groups.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        gredit_show(ch, parg);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    for (cmd = 0; gredit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, gredit_table[cmd].name)) {
            (*gredit_table[cmd].olc_fn)(ch, parg);
            return;
        }
    }

    interpret(ch, arg);
    return;
}


/***************************************************************************
 *	entry point for the skill editor
 ***************************************************************************/
void do_gredit(CHAR_DATA *ch, const char *argument)
{
    GROUP *group;
    char arg[MSL];

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);
    if (!str_prefix(arg, "new")) {
        argument = one_argument(argument, arg);
        gredit_new(ch, argument);
    }

    if ((group = group_lookup(argument)) == NULL) {
        send_to_char("GRedit : group does not exist.\n\r", ch);
        return;
    }

    ch->desc->ed_data = (void *)group;
    ch->desc->editor = ED_GROUP;

    return;
}


/***************************************************************************
 *	general commands
 ***************************************************************************/
/***************************************************************************
 *	gredit_delete
 *
 *	delete the group
 ***************************************************************************/
EDIT(gredit_delete){
    GROUP *group;

    EDIT_GROUP(ch, group);

    /* unlink the list */
    if (group == group_list) {
        group_list = group->next;
    } else {
        GROUP *group_idx;
        GROUP *group_prev = NULL;

        for (group_idx = group_list;
             group_idx != NULL;
             group_idx = group_idx->next) {
            if (group_idx == group)
                break;
            group_prev = group_idx;
        }

        if (group_prev != NULL && group_idx != NULL)
            group_prev->next = group->next;
    }

    free_group(group);
    edit_done(ch);

    send_to_char("Ok.\n\r", ch);
    return true;
}




/***************************************************************************
 *	gredit_show
 *
 *	show the properties for the group
 ***************************************************************************/
EDIT(gredit_show){
    GROUP *group;
    char buf[MSL];

    EDIT_GROUP(ch, group);

    /* general information */
    printf_to_char(ch, "\n\r`&Name``:        [%s]\n\r", group->name);
    printf_to_char(ch, "`&Number``:      [%d]\n\r", group->gn);
    printf_to_char(ch, "`&Help``:        [%s]\n\r", (group->help != NULL) ? group->help->keyword : "none");

    /* spell/affects */
    if (group->skills != NULL && group->skills->skill != NULL) {
        SKILL_LIST *skills;
        bool first;

        first = true;
        for (skills = group->skills; skills != NULL; skills = skills->next) {
            if (skills->skill != NULL) {
                if (first) {
                    printf_to_char(ch, "`&Skills``:      [%s]\n\r", skills->skill->name);
                    first = false;
                } else {
                    printf_to_char(ch, "             [%s]\n\r", skills->skill->name);
                }
            }
        }
    } else {
        send_to_char("`&Skills``:      [none]\n\r", ch);
    }

    send_to_char("\n\r`!COSTS``\n\r", ch);
    send_to_char("`1================================================\n\r", ch);

    if (group->levels != NULL) {
        LEVEL_INFO *levels;

        for (levels = group->levels; levels != NULL; levels = levels->next) {
            sprintf(buf, "%s``:", capitalize(class_table[levels->class].name));
            printf_to_char(ch, "`&%-11.11s    [%d]\n\r", buf, levels->difficulty);
        }
    }

    return false;
}



/***************************************************************************
 *	gredit_new
 *
 *	create a new group
 ***************************************************************************/
EDIT(gredit_new){
    GROUP *group;

    if (is_help(argument)) {
        send_to_char("Syntax   : new [name]\n\r", ch);
        return false;
    }

    if ((group = group_lookup(argument)) != NULL) {
        send_to_char("GRedit : group already exists.\n\r", ch);
        return false;
    }

    group = new_group();
    group->name = str_dup(argument);
    group->gn = (++gn_max_group_sn);

    group->next = group_list;
    group_list = group;
    ch->desc->ed_data = (GROUP *)group;
    ch->desc->editor = ED_GROUP;

    send_to_char("Ok.\n\r", ch);
    return false;
}


/***************************************************************************
 *	gredit_help
 *
 *	edit the help file for the group
 ***************************************************************************/
EDIT(gredit_help){
    GROUP *group;
    HELP_DATA *help;

    EDIT_GROUP(ch, group);

    if (is_help(argument)) {
        send_to_char("Syntax:  helpfile [keyword of help]\n\r", ch);
        return false;
    }

    if ((help = help_lookup(argument)) == NULL) {
        send_to_char("That help does not exist.\n\r", ch);
        return false;
    }

    if (group->help_keyword != NULL)
        free_string(group->help_keyword);
    group->help_keyword = str_dup(help->keyword);
    group->help = help;

    send_to_char("Ok.\n\r", ch);
    return true;
}



/***************************************************************************
 *	gredit_skills
 *
 *	add or remove a skill from the group list
 ***************************************************************************/
EDIT(gredit_skills){
    GROUP *group;
    SKILL *skill;
    SKILL_LIST *list;
    char cmd[MIL];

    EDIT_GROUP(ch, group);

    if (is_help(argument)) {
        send_to_char("\n\r", ch);
        send_to_char("Syntax:  skill <add|remove> <skill name>\n\r", ch);
        send_to_char("         skill <list> [all|partial skill name]\n\r\n\r", ch);
        return false;
    }

    argument = one_argument(argument, cmd);
    if (!str_prefix(cmd, "add")) {
        if ((skill = skill_lookup(argument)) == NULL) {
            send_to_char("That skill does not exist.  Type 'skill list' for a list of spells.\n\r", ch);
            return false;
        }

        add_group_skill(group, skill);
    } else if (!str_prefix(cmd, "remove")) {
        if (group->skills == NULL) {
            send_to_char("That group does not have any skills associated with it.\n\r", ch);
            return false;
        }

        if (!str_prefix(argument, "all")) {
            SKILL_LIST *list_next;

            for (list = group->skills; list != NULL; list = list_next) {
                list_next = list->next;
                free_skill_list(list);
            }
            group->skills = NULL;
        } else {
            if ((skill = skill_lookup(argument)) == NULL) {
                send_to_char("That skill does not exist.  Type 'skill list' for a list of spells.\n\r", ch);
                return false;
            }

            if (group->skills->skill == skill) {
                list = group->skills;
                group->skills = list->next;
                free_skill_list(list);
            } else {
                SKILL_LIST *list_prev = NULL;

                for (list = group->skills; list != NULL; list = list->next) {
                    if (list->skill == skill)
                        break;
                    list_prev = list;
                }

                if (list == NULL) {
                    printf_to_char(ch, "The skill is not set on this group. Skill: %s\n\r", skill->name);
                    return false;
                }

                if (list_prev != NULL) {
                    list_prev->next = list->next;
                    free_skill_list(list);
                }
            }
        }
    } else if (!str_prefix(cmd, "list")) {
        int col;
        bool show_all;

        col = 0;
        show_all = (argument[0] == '\0' || !str_prefix(argument, "all"));

        for (skill = skill_list; skill != NULL; skill = skill->next) {
            if (show_all || !str_prefix(argument, skill->name)) {
                printf_to_char(ch, "%-18.18s", skill->name);
                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }

        if (col % 3 != 0)
            send_to_char("\n\r", ch);
        return false;
    } else {
        return skedit_spell(ch, "");
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}


/***************************************************************************
 *	gredit_cost
 *
 *	edit the costs of the group
 ***************************************************************************/
EDIT(gredit_cost){
    GROUP *group;
    LEVEL_INFO *level_info;
    LEVEL_INFO *level_idx;
    char arg[MSL];
    int cls;
    int cost;

    EDIT_GROUP(ch, group);
    if (is_help(argument)) {
        send_to_char("Syntax:  cost <class> <cost|none>\n\r", ch);
        return false;
    }

    argument = one_argument(argument, arg);
    if ((cls = class_lookup(arg)) <= -1) {
        send_to_char("The chosen class is invalid.  Please check the name and try again.\n\r", ch);
        return false;
    }

    cost = -1;

    if (is_number(argument))
        cost = parse_int(argument);

    /*
     * we have a valid class and a level - see if a
     * level for that class already exists
     */
    level_info = NULL;
    for (level_idx = group->levels;
         level_idx != NULL;
         level_idx = level_idx->next) {
        if (level_idx->class == cls) {
            level_info = level_idx;
            break;
        }
    }

    if (cost > -1) {
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

        level_info->level = 1;
        level_info->difficulty = cost;

        add_group_level(group, level_info);
    } else {
        LEVEL_INFO *level_prev = NULL;

        if (level_info == NULL) {
            send_to_char("That level information does not exist.\n\r", ch);
            return false;
        }

        /* unlink the level info structure */
        if (level_info == group->levels) {
            group->levels = level_info->next;
        } else {
            for (level_idx = group->levels; level_idx != NULL; level_idx = level_idx->next) {
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
 *	load/save groups
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
 *	load_groups
 *
 *	load the groups table from a file
 ***************************************************************************/
void load_groups()
{
    FILE *fp;
    GROUP *group = NULL;
    SKILL *skill;
    char *word;
    int gn;
    bool found;

    gn_max_group_sn = 0;

    if ((fp = fopen(GROUP_FILE, "r")) == NULL) {
        perror("load_groups: could not open group file.");
        return;
    }

    word = fread_word(fp);
    found = false;

    while (!feof(fp) && str_cmp(word, END_MARKER)) {
        if (!str_cmp(word, "gn")) {
            gn = fread_number(fp);

            group = new_group();
            group->next = group_list;
            group_list = group;

            group->gn = gn;
            gn_max_group_sn = UMAX(gn_max_group_sn, gn);
            found = true;
        } else {
            if (group == NULL) {
                log_bug("load_groups: No group loaded - invalid file syntax. %s", word);
                raise(SIGABRT);
            }

            KEY("Name", group->name, fread_string(fp));

            if (!str_cmp(word, "Lvl")) {
                LEVEL_INFO *level;

                level = new_level_info();

                level->class = fread_number(fp);
                level->level = fread_number(fp);
                level->difficulty = fread_number(fp);

                add_group_level(group, level);
                found = true;
            }

            if (!str_cmp(word, "Sk")) {
                skill = skill_lookup(fread_string(fp));
                if (skill != NULL)
                    add_group_skill(group, skill);
                else
                    log_bug("load_groups: invalid group skill.\ngroup: %s\nskill: %s\n",
                            group->name, word);

                found = true;
            }

            if (!str_cmp(word, "Help")) {
                group->help_keyword = fread_string(fp);

                /* temporary fix */
                if (!str_cmp(group->help_keyword, "(null)")) {
                    free_string(group->help_keyword);
                    group->help_keyword = NULL;
                }

                group->help = help_lookup(group->help_keyword);
                found = true;
            }
        }

        if (!found) {
            log_bug("load_groups: No group loaded - invalid file syntax. %s", word);
            raise(SIGABRT);
        }

        word = fread_word(fp);
        found = false;
    }
}


/***************************************************************************
 *	save_groups
 *
 *	save the group table to a file
 ***************************************************************************/
void save_groups()
{
    FILE *fp;
    GROUP *group;
    SKILL_LIST *skills;
    LEVEL_INFO *level;

    if ((fp = fopen(GROUP_FILE, "w")) == NULL) {
        log_bug("save_groups: fopen");
        return;
    }

    for (group = group_list; group != NULL; group = group->next) {
        fprintf(fp, "Gn %d\n", group->gn);
        fprintf(fp, "Name %s~\n", group->name);

        for (level = group->levels; level != NULL; level = level->next)
            fprintf(fp, "Lvl %d %d %d\n", level->class, level->level, level->difficulty);

        for (skills = group->skills; skills != NULL; skills = skills->next)
            if (skills->skill != NULL)
                fprintf(fp, "Sk %s~\n", skills->skill->name);

        if (group->help != NULL)
            fprintf(fp, "Help %s~\n", group->help->keyword);

        fprintf(fp, "\n");
    }

    fprintf(fp, "END\n");
    fclose(fp);
}
