/***************************************************************************
*  File: olc_medit.c                                                      *
*                                                                         *
*  Much time and thought has gone into this software and you are          *
*  benefitting.  We hope that you share your changes too.  What goes      *
*  around, comes around.                                                  *
*                                                                         *
*  This code was freely distributed with the The Isles 1.1 source code,   *
*  and has been used here for OLC - OLC would not be what it is without   *
*  all the previous coders who released their source code.                *
*                                                                         *
***************************************************************************/
/***************************************************************************
*	The following codes is based on ILAB OLC by Jason Dinkel
*	Mobprogram code by Lorrom from Nevermore Mud
***************************************************************************/



/***************************************************************************
*	includes
***************************************************************************/
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"



extern void string_append(CHAR_DATA * ch, char **string);

/***************************************************************************
*	local defines
***************************************************************************/
#define MPEDIT(fun)           bool fun(CHAR_DATA * ch, char *argument)

/* editor table */
const struct olc_cmd_type mpedit_table[] =
{
/*	{	command		function		}, */

	{ "commands", show_commands  },
	{ "create",   mpedit_create  },
	{ "clone",    mpedit_clone   },
	{ "comment",  mpedit_comment },
	{ "code",     mpedit_code    },
	{ "show",     mpedit_show    },
	{ "list",     mpedit_list    },
	{ "?",	      show_help	     },
	{ NULL,	      0		     }
};



/***************************************************************************
*	mpedit
*
*	run the mob program interpreter
***************************************************************************/
void mpedit(CHAR_DATA *ch, char *argument)
{
	MPROG_CODE *mpcode;
	AREA_DATA *ad;
	char arg[MIL];
	char command[MIL];
	int cmd;

	smash_tilde(argument);
	strcpy(arg, argument);
	argument = one_argument(argument, command);

	EDIT_MPCODE(ch, mpcode);
	if (mpcode) {
		ad = get_vnum_area(mpcode->vnum);
		if (ad == NULL) {
			edit_done(ch);
			return;
		}

		if (!IS_BUILDER(ch, ad)) {
			send_to_char("MPEdit: Insufficient security to modify code.\n\r", ch);
			edit_done(ch);
			return;
		}
	}

	if (command[0] == '\0') {
		mpedit_show(ch, argument);
		return;
	}

	if (!str_cmp(command, "done")) {
		edit_done(ch);
		return;
	}

	for (cmd = 0; mpedit_table[cmd].name != NULL; cmd++) {
		if (!str_prefix(command, mpedit_table[cmd].name)) {
			if ((*mpedit_table[cmd].olc_fn)(ch, argument) && mpcode) {
				if ((ad = get_vnum_area(mpcode->vnum)) != NULL)
					SET_BIT(ad->area_flags, AREA_CHANGED);
			}
			return;
		}
	}

	interpret(ch, arg);
	return;
}


/***************************************************************************
*	do_mpedit
*
*	entry point for editing mob programs
***************************************************************************/
void do_mpedit(CHAR_DATA *ch, const char *argument)
{
	MPROG_CODE *mpcode;
	char command[MIL];
	char arg[MIL];


	argument = one_argument(argument, command);
	if (is_number(command)) {
		int vnum = parse_int(command);
		AREA_DATA *ad;

		if ((mpcode = get_mprog_index(vnum)) == NULL) {
			send_to_char("MPEdit : That vnum does not exist.\n\r", ch);
			return;
		}

		ad = get_vnum_area(vnum);
		if (ad == NULL) {
			send_to_char("MPEdit : That vnum is not assigned to an area.\n\r", ch);
			return;
		}

		if (!IS_BUILDER(ch, ad)) {
			send_to_char("MPEdit : Insufficient security to edit that area.\n\r", ch);
			return;
		}

		ch->desc->ed_data = (void *)mpcode;
		ch->desc->editor = ED_MPCODE;
		return;
	}

	if (!str_cmp(command, "create")) {
		if (argument[0] == '\0') {
			send_to_char("Syntax:  mpedit create [vnum]\n\r", ch);
			return;
		}

		mpedit_create(ch, argument);
		return;
	}

	if (!str_cmp(command, "clone")) {
		if (argument[0] == '\0') {
			send_to_char("Syntax:  mpedit clone [new vnum] [existing vnum]\n\r", ch);
			return;
		}

		if (mpedit_create(ch, argument)) {
			argument = one_argument(argument, arg);
			medit_clone(ch, argument);
		}
		return;
	}

	send_to_char("Syntax:  mpedit [vnum]\n\r", ch);
	send_to_char("         mpedit create [vnum]\n\r", ch);
	send_to_char("         mpedit clone [new vnum] [existing vnum]\n\r", ch);
	return;
}



/***************************************************************************
*	mpedit_create
*
*	create a new mob program
***************************************************************************/
MPEDIT(mpedit_create){
	MPROG_CODE *mpcode;
	AREA_DATA *ad;
	int value = parse_int(argument);

	if (IS_NULLSTR(argument) || value < 1) {
		send_to_char("Syntax:  mpedit create [vnum]\n\r", ch);
		return false;
	}

	ad = get_vnum_area(value);
	if (ad == NULL) {
		send_to_char("MPEdit : That vnum is not assigned to an area.\n\r", ch);
		return false;
	}

	if (!IS_BUILDER(ch, ad)) {
		send_to_char("MPEdit : Insufficient security to create MobProgs.\n\r", ch);
		return false;
	}

	if (get_mprog_index(value)) {
		send_to_char("MPEdit: Code vnum already exists.\n\r", ch);
		return false;
	}

	mpcode = new_mpcode();
	mpcode->vnum = value;
	mpcode->next = mprog_list;
	mprog_list = mpcode;

	ch->desc->ed_data = (void *)mpcode;
	ch->desc->editor = ED_MPCODE;

	send_to_char("MobProgram Code Created.\n\r", ch);
	return true;
}


/***************************************************************************
*	mpedit_clone
*
*	clone one mprogs code from another
***************************************************************************/
MPEDIT(mpedit_clone){
	MPROG_CODE *mpcode;
	MPROG_CODE *pClone;
	int value;

	EDIT_MPCODE(ch, mpcode);
	value = parse_int(argument);
	if (argument[0] == '\0'
	    || value == 0) {
		send_to_char("Syntax:  clone [existing vnum]\n\r", ch);
		return false;
	}

	if ((pClone = get_mprog_index(value)) == NULL) {
		send_to_char("MEdit:  MobProgram to clone does not exist.\n\r", ch);
		return false;
	}

	free_string(mpcode->code);
	mpcode->code = str_dup(pClone->code);
	return true;
}


/***************************************************************************
*	mpedit_show
*
*	show the details of a mob program
***************************************************************************/
MPEDIT(mpedit_show){
	MPROG_CODE *mpcode;

	EDIT_MPCODE(ch, mpcode);
	printf_to_char(ch, "`&Vnum``:       [%d]\n\r"
		       "`&Comment``:    [%s]\n\r"
		       "`&Code``:\n\r%s\n\r",
		       mpcode->vnum,
		       (mpcode->comment[0] != '\0') ? mpcode->comment : "(none)",
		       mpcode->code);
	return false;
}


/***************************************************************************
*	mpedit_code
*
*	edit the code for a mob program
***************************************************************************/
MPEDIT(mpedit_code){
	MPROG_CODE *mpcode;

	EDIT_MPCODE(ch, mpcode);
	if (argument[0] == '\0') {
		string_append(ch, &mpcode->code);
		return true;
	}

	send_to_char("Syntax: code\n\r", ch);
	return false;
}


/***************************************************************************
*	mpedit_comment
*
*	edit the code for a mob program
***************************************************************************/
MPEDIT(mpedit_comment){
	MPROG_CODE *mpcode;

	EDIT_MPCODE(ch, mpcode);

	if (is_help(argument)) {
		send_to_char("Syntax: code\n\r", ch);
		return false;
	}

	free_string(mpcode->comment);
	mpcode->comment = str_dup(argument);
	return true;
}


/***************************************************************************
*	mpedit_list
*
*	show a list of mob programs
***************************************************************************/
MPEDIT(mpedit_list){
	MPROG_CODE *mprg;
	AREA_DATA *ad;
	int count = 1;
	bool show_all = !str_cmp(argument, "all");

	for (mprg = mprog_list; mprg != NULL; mprg = mprg->next) {
		if (show_all
		    || IN_RANGE(ch->in_room->area->min_vnum, mprg->vnum, ch->in_room->area->max_vnum)) {
			if (count == 1) {
				send_to_char("`7  #   Area                  Vnum       Comment\n\r", ch);
				send_to_char("`1======================================================``\n\r", ch);
			}

			ad = get_vnum_area(mprg->vnum);
			if (ad != NULL) {
				char *unclr;

				unclr = uncolor_str(ad->name);
				printf_to_char(ch, "[`&%3d``] %-16.16s     %5d        %s\n\r",
					       count,
					       unclr,
					       mprg->vnum,
					       (mprg->comment[0] != '\0') ? mprg->comment : "none");
				free_string(unclr);
			} else {
				printf_to_char(ch, "[`&%3d``] %-16.16s     %5d        %s\n\r",
					       count,
					       "unknown",
					       mprg->vnum,
					       (mprg->comment[0] != '\0') ? mprg->comment : "none");
			}
			count++;
		}
	}

	if (count == 1) {
		if (show_all)
			send_to_char("No existing mob programs.\n\r", ch);
		else
			send_to_char("There are no mob programs in this area.\n\r", ch);
	}

	return false;
}
