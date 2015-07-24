/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael          *
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
*       Russ Taylor(rtaylor@hypercube.org)                                *
*       Gabrielle Taylor(gtaylor@hypercube.org)                           *
*       Brian Moore(zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#if defined(WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "lookup.h"
#include "recycle.h"
#include "skills.h"



static SOCIAL *social_free;
SOCIAL *social_list;

int max_social;

extern void save_socials        args((void));

/***************************************************************************
*	new_social
*
*	allocate the memory for a new social
***************************************************************************/
static SOCIAL *new_social()
{
	static SOCIAL social_z;
	SOCIAL *social;

	if (social_free == NULL) {
		social = alloc_perm((unsigned int)sizeof(*social));
	} else {
		social = social_free;
		social_free = social->next;
	}

	*social = social_z;

	VALIDATE(social);

	return social;
}


/***************************************************************************
*	free_social
*
*	free a social structure
***************************************************************************/
static void free_social(SOCIAL *social)
{
	if (!IS_VALID(social))
		return;

	free_string(social->char_auto);
	free_string(social->char_found);
	free_string(social->char_no_arg);
	free_string(social->name);
	free_string(social->others_auto);
	free_string(social->others_found);
	free_string(social->others_no_arg);
	free_string(social->vict_found);

	INVALIDATE(social);
}



/***************************************************************************
*	load_socials
*
*	load the entire social table
***************************************************************************/
void load_socials()
{
	FILE *fp;
	SOCIAL *social;
	int idx;

	fp = fopen(SOCIAL_FILE, "r");
	if (!fp) {
		log_string("Could not open " SOCIAL_FILE " for reading.");
		exit(1);
	}

	if (fscanf(fp, "%d\n", &max_social) == EOF) {
		log_string("Unexpected EOF reading socials.");
        exit(1);
    }

	for (idx = 0; idx < max_social; idx++) {
		social = new_social();

		social->name = fread_string(fp);
		social->char_no_arg = fread_string(fp);
		social->others_no_arg = fread_string(fp);
		social->char_found = fread_string(fp);
		social->others_found = fread_string(fp);
		social->vict_found = fread_string(fp);
		social->char_auto = fread_string(fp);
		social->others_auto = fread_string(fp);

		social->next = social_list;
		social_list = social;
	}

	fclose(fp);
}





/***************************************************************************
*	social_lookup
*
*	look up a social by name
***************************************************************************/
SOCIAL *social_lookup(const char *name)
{
	SOCIAL *social;
	SOCIAL *social_tmp;

	social_tmp = NULL;
	for (social = social_list; social != NULL; social = social->next) {
		/* short-circuit if we are obviously not interested */
		if (name[0] != social->name[0])
			continue;

		/* check if we have an exact match */
		if (!str_cmp(name, social->name))
			return social;

		/* we have to do some poo if we have a partial */
		if (!str_prefix(name, social->name)) {
			if (social_tmp == NULL) {
				social_tmp = social;
			} else {
				/* get the shortest match */
				if (strlen(social_tmp->name) > strlen(social->name))
					social_tmp = social;
			}
		}
	}

	return social_tmp;
}

/***************************************************************************
*	local defines
***************************************************************************/
const struct olc_cmd_type scedit_table[] =
{
/*	{	command		function					}, */

	{ "new",      scedit_new	   },
	{ "delete",   scedit_delete	   },
	{ "commands", show_commands	   },
	{ "show",     scedit_show	   },
	{ "?",	      show_help		   },
	{ "help",     show_help		   },

	{ "cself",    scedit_char_auto	   },
	{ "cfound",   scedit_char_found	   },
	{ "cnoarg",   scedit_char_no_arg   },

	{ "oself",    scedit_others_auto   },
	{ "ofound",   scedit_others_found  },
	{ "onoarg",   scedit_others_no_arg },

	{ "vfound",   scedit_vict_found	   },

	{ NULL,	      0			   }
};





/***************************************************************************
*	scedit
*
*	command interpreter for the skill editor
***************************************************************************/
void scedit(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char command[MIL];
	int cmd;

	smash_tilde(argument);
	strcpy(arg, argument);

	argument = one_argument(argument, command);

	if (ch->pcdata->security < 3) {
		send_to_char("SCEdit: Insuficient security to edit socials.\n\r", ch);
		edit_done(ch);
		return;
	}

	if (command[0] == '\0') {
		scedit_show(ch, argument);
		return;
	}

	if (!str_cmp(command, "done")) {
		edit_done(ch);
		return;
	}

	for (cmd = 0; scedit_table[cmd].name != NULL; cmd++) {
		if (!str_prefix(command, scedit_table[cmd].name)) {
			(*scedit_table[cmd].olc_fn)(ch, argument);
			return;
		}
	}

	interpret(ch, arg);
	return;
}


/***************************************************************************
*	do_scedit
*
*	entry point for the skill editor
***************************************************************************/
void do_scedit(CHAR_DATA *ch, char *argument)
{
	SOCIAL *social;
	char arg[MSL];

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);
	if (!str_prefix(arg, "new")) {
		argument = one_argument(argument, arg);
		scedit_new(ch, argument);
	}

	if ((social = social_lookup(argument)) == NULL) {
		send_to_char("SCEdit : Social does not exist.\n\r", ch);
		return;
	}

	ch->desc->ed_data = (void *)social;
	ch->desc->editor = ED_SOCIAL;

	return;
}


/***************************************************************************
*	general functions
***************************************************************************/
/***************************************************************************
*	scedit_new
*
*	show the properties for the skill
***************************************************************************/
EDIT(scedit_new){
	SOCIAL *social;
	char arg[MSL];

	argument = one_argument(argument, arg);
	if ((social = social_lookup(arg)) != NULL) {
		send_to_char("That social already exists.\n\r", ch);
		return FALSE;
	}

	social = new_social();

	/* set the name */
	social->name = str_dup(arg);

	/* set the other arguments to empty strings */
	social->char_auto = str_dup("");
	social->char_found = str_dup("");
	social->char_no_arg = str_dup("");
	social->others_auto = str_dup("");
	social->others_found = str_dup("");
	social->others_no_arg = str_dup("");
	social->vict_found = str_dup("");

	/* link it */
	social->next = social_list;
	social_list = social;

	max_social++;

	send_to_char("Ok.\n\r", ch);

	return TRUE;
}

/***************************************************************************
*	scedit_show
*
*	show the properties for the skill
***************************************************************************/
EDIT(scedit_delete){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (social == social_list) {
		social_list = social->next;
	} else {
		SOCIAL *social_idx;
		SOCIAL *social_prev = NULL;

		for (social_idx = social_list; social_idx != NULL; social_idx = social_idx->next) {
			if (social_idx == social)
				break;

			social_prev = social_idx;
		}

		if (social_idx != NULL && social_prev != NULL)
			social_prev->next = social->next;
	}

	free_social(social);
	max_social--;
	edit_done(ch);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}



/***************************************************************************
*	scedit_show
*
*	show the properties for the skill
***************************************************************************/
EDIT(scedit_show){
	SOCIAL *social;


	/*
	 *      printf_to_char(ch, "Social: %s\n\r"
	 *                                         "(cnoarg) No argument given, character sees:\n\r"
	 *                                         "%s\n\r\n\r"
	 *                                         "(onoarg) No argument given, others see:\n\r"
	 *                                         "%s\n\r\n\r"
	 *                                         "(cfound) Target found, character sees:\n\r"
	 *                                         "%s\n\r\n\r"
	 *                                         "(ofound) Target found, others see:\n\r"
	 *                                         "%s\n\r\n\r"
	 *                                         "(vfound) Target found, victim sees:\n\r"
	 *                                         "%s\n\r\n\r"
	 *                                         "(cself) Target is character himself:\n\r"
	 *                                         "%s\n\r\n\r"
	 *                                         "(oself) Target is character himself, others see:\n\r"
	 *                                         "%s\n\r",
	 *                                         social->name,
	 *                                         social->char_no_arg,
	 *                                         social->others_no_arg,
	 *                                         social->char_found,
	 *                                         social->others_found,
	 *                                         social->vict_found,
	 *                                         social->char_auto,
	 *                                         social->others_auto);
	 */

	EDIT_SOCIAL(ch, social);

	printf_to_char(ch, "\n\r`&Social Name``:   %s\n\r", social->name);

	send_to_char("\n\rMessage with No Argument Given\n\r", ch);
	send_to_char("`1========================================================================``\n\r", ch);
	printf_to_char(ch, "`&Player sees `8(`7cnoarg`8)``: %s\n\r", social->char_no_arg);
	printf_to_char(ch, "`&Others see `8(`7onoarg`8)``:  %s\n\r", social->others_no_arg);


	send_to_char("\n\rMessage with Target Found\n\r", ch);
	send_to_char("`1========================================================================``\n\r", ch);
	printf_to_char(ch, "`&Player sees `8(`7cfound`8)``: %s\n\r", social->char_found);
	printf_to_char(ch, "`&Others see `8(`7ofound`8)``:  %s\n\r", social->others_found);
	printf_to_char(ch, "`&Victim sees `8(`7vfound`8)``: %s\n\r", social->vict_found);

	send_to_char("\n\rMessage when Target is Self\n\r", ch);
	send_to_char("`1========================================================================``\n\r", ch);
	printf_to_char(ch, "`&Player sees `8(`7cself`8)``:  %s\n\r", social->char_auto);
	printf_to_char(ch, "`&Others see `8(`7oself`8)``:   %s\n\r", social->others_auto);


	return FALSE;
}


/***************************************************************************
*	messages to characters
***************************************************************************/
/***************************************************************************
*	scedit_char_auto
*
*	edit the char_auto message
***************************************************************************/
EDIT(scedit_char_auto){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  char_auto [message]\n\r", ch);
		return FALSE;
	}

	if (social->char_auto != NULL)
		free_string(social->char_auto);

	social->char_auto = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	scedit_char_found
*
*	edit the char_found message
***************************************************************************/
EDIT(scedit_char_found){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  char_found [message]\n\r", ch);
		return FALSE;
	}

	if (social->char_found != NULL)
		free_string(social->char_found);

	social->char_found = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	scedit_char_no_arg
*
*	edit the char_no_arg message
***************************************************************************/
EDIT(scedit_char_no_arg){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  char_no_arg [message]\n\r", ch);
		return FALSE;
	}

	if (social->char_no_arg != NULL)
		free_string(social->char_no_arg);

	social->char_no_arg = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}




/***************************************************************************
*	messages to others
***************************************************************************/
/***************************************************************************
*	scedit_others_auto
*
*	edit the others_auto message
***************************************************************************/
EDIT(scedit_others_auto){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  others_auto [message]\n\r", ch);
		return FALSE;
	}

	if (social->others_auto != NULL)
		free_string(social->others_auto);

	social->others_auto = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	scedit_others_found
*
*	edit the others_found message
***************************************************************************/
EDIT(scedit_others_found){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  others_found [message]\n\r", ch);
		return FALSE;
	}

	if (social->others_found != NULL)
		free_string(social->others_found);

	social->others_found = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	scedit_others_no_arg
*
*	edit the others_no_arg message
***************************************************************************/
EDIT(scedit_others_no_arg){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  others_no_arg [message]\n\r", ch);
		return FALSE;
	}

	if (social->others_no_arg != NULL)
		free_string(social->others_no_arg);

	social->others_no_arg = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}



/***************************************************************************
*	messages to victims
***************************************************************************/
/***************************************************************************
*	scedit_vict_found
*
*	edit the vict_found message
***************************************************************************/
EDIT(scedit_vict_found){
	SOCIAL *social;

	EDIT_SOCIAL(ch, social);

	if (is_help(argument)) {
		send_to_char("Syntax:  vict_found [message]\n\r", ch);
		return FALSE;
	}

	if (social->vict_found != NULL)
		free_string(social->vict_found);

	social->vict_found = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
	return TRUE;
}




/***************************************************************************
*	do_sedit
*
*	legacy social editor - saved for those fuxors that are used
*	to using the clunky thing
***************************************************************************/
void do_sedit(CHAR_DATA *ch, char *argument)
{
	SOCIAL *social;
	char command[MIL];
	char name[MIL];
	char arg[MIL];
	bool changed;

	changed = FALSE;

	/* maintain some existing code */
	strcpy(arg, argument);
	argument = one_argument(argument, command);
	argument = one_argument(argument, name);

	if (command[0] == '\0') {
		send_to_char("Huh? Type HELP SEDIT to see syntax.\n\r", ch);
		return;
	}


	if (name[0] == '\0') {
		send_to_char("What social do you want to operate on?\n\r", ch);
		return;
	}

	if (ch->pcdata->security < 3) {
		send_to_char("You lack the security needed to edit Socials.\n\r", ch);
		return;
	}


	social = social_lookup(name);

	if (!str_prefix(command, "new")) { /* Create a new social */
		changed = scedit_new(ch, name);
		edit_done(ch);
	} else {
		int cmd;

		if (social == NULL) {
			send_to_char("No such social exists.\n\r", ch);
			return;
		}

		ch->desc->ed_data = (void *)social;
		ch->desc->editor = ED_SOCIAL;

		for (cmd = 0; scedit_table[cmd].name != NULL; cmd++)
			if (!str_prefix(command, scedit_table[cmd].name))
				changed = (*scedit_table[cmd].olc_fn)(ch, argument);

		edit_done(ch);
	}



	/* We have done something. update social table */
	if (changed)
		save_socials();
}
