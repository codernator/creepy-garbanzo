#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"




/***************************************************************************
*	local functions
***************************************************************************/
static void string_room(CHAR_DATA * ch, char *arg);
static void string_char(CHAR_DATA * ch, char *arg);
static void string_obj(CHAR_DATA * ch, char *arg);
static void realloc_string(char **src, char *str);

extern ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);



/***************************************************************************
*	do_string
***************************************************************************/
void do_string(CHAR_DATA *ch, char *argument)
{
	char type[MIL];


	smash_tilde(argument);
	argument = one_argument(argument, type);

	if (type[0] == '\0' || argument[0] == '\0') {
		do_help(ch, "string_general");
		return;
	}

	if (!str_prefix(type, "room")) {
		string_room(ch, argument);
		return;
	}


	if (!str_prefix(type, "character") || !str_prefix(type, "mobile")) {
		string_char(ch, argument);
		return;
	}

	if (!str_prefix(type, "object")) {
		string_obj(ch, argument);
		return;
	}

	do_string(ch, "");
}


/***************************************************************************
*	string_char
***************************************************************************/
static void string_char(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *victim;
	char arg1[MIL];
	char arg2[MIL];

	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}


	if (!str_prefix(arg2, "name")) {
		if (!IS_NPC(victim)) {
			send_to_char("Not on PC's. Try using 'rename'.\n\r", ch);
			return;
		}

		realloc_string(&victim->name, arg);
		return;
	}

	if (!str_prefix(arg2, "description")) {
		realloc_string(&victim->description, arg);
		return;
	}

	if (!str_prefix(arg2, "short")) {
		realloc_string(&victim->short_descr, arg);
		return;
	}

	if (!str_prefix(arg2, "long")) {
		char arg3[MIL];

		strcpy(arg3, arg);
		strcat(arg3, "\n\r");
		realloc_string(&victim->long_descr, arg3);
		return;
	}

	if (!str_prefix(arg2, "title")) {
		char arg3[MIL];

		if (IS_NPC(victim)) {
			send_to_char("Not on NPC's.\n\r", ch);
			return;
		}

		if (ch->level < victim->level) {
			send_to_char("`1Don't cross people who can hurt you.``\n\r", ch);
			return;
		}

		if (arg[0] != '.' && arg[0] != ',' && arg[0] != '!' && arg[0] != '?' && arg[0] != ';' && arg[0] != '+') {
			arg3[0] = ' ';
			strcpy(arg3 + 1, arg);
		} else {
			strcpy(arg3, arg);
		}

		realloc_string(&victim->pcdata->title, arg3);
		return;
	}

	if (!str_prefix(arg2, "who") && victim->level >= LEVEL_IMMORTAL) {
		if (IS_NPC(victim)) {
			send_to_char("Not on NPC's.\n\r", ch);
			return;
		}

		if (!str_cmp(arg, "reset")) {
			free_string(ch->pcdata->who_thing);
			ch->pcdata->who_thing = str_empty;
			return;
		}

		realloc_string(&victim->pcdata->who_thing, arg);
		return;
	}
}


/***************************************************************************
*	string_obj
***************************************************************************/
static void string_obj(CHAR_DATA *ch, char *arg)
{
	OBJ_DATA *obj;
	char arg1[MIL];
	char arg2[MIL];

	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);

	if ((obj = get_obj_carry(ch, arg1)) == NULL
	    && (obj = get_obj_here(ch, arg1)) == NULL
	    && (obj = get_obj_world(ch, arg1)) == NULL) {
		send_to_char("Nothing like that in heaven or earth.\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "name")) {
		realloc_string(&obj->name, arg);
		return;
	}

	if (!str_prefix(arg2, "short")) {
		realloc_string(&obj->short_descr, arg);
		return;
	}

	if (!str_prefix(arg2, "long")) {
		realloc_string(&obj->description, arg);
		return;
	}

	if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended")) {
		EXTRA_DESCR_DATA *ed;
		char arg3[MIL];

		arg = one_argument(arg, arg3);
		if (arg == NULL) {
			send_to_char("Syntax: string obj <object> ed <keyword> <string>\n\r", ch);
			return;
		}

		strcat(arg, "\n\r");
		ed = new_extra_descr();
		ed->keyword = str_dup(arg3);
		ed->description = str_dup(arg);
		ed->next = obj->extra_descr;
		obj->extra_descr = ed;
		return;
	}
}


/***************************************************************************
*	string_room
***************************************************************************/
static void string_room(CHAR_DATA *ch, char *arg)
{
	ROOM_INDEX_DATA *location;
	char arg1[MIL];
	char arg2[MIL];

	if (!IS_TRUSTED(ch, IMPLEMENTOR)) {
		send_to_char("You're not high enough to do that.\n\r", ch);
		return;
	}

	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);

	if ((location = find_location(ch, arg1)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, location)
	    && ch->in_room != location
	    && room_is_private(location)
	    && !IS_TRUSTED(ch, IMPLEMENTOR)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "name")) {
		realloc_string(&location->name, arg);
		return;
	}

	if (!str_prefix(arg2, "owner")) {
		realloc_string(&location->owner, arg);
		return;
	}

	if (!str_prefix(arg2, "extended")) {
		EXTRA_DESCR_DATA *ed;
		char arg3[MIL];

		arg = one_argument(arg, arg3);

		if (arg == NULL) {
			send_to_char("Syntax: string room <vnum> ed <keyword> <string>\n\r", ch);
			return;
		}

		strcat(arg, "\n\r");

		ed = new_extra_descr();
		ed->keyword = str_dup(arg3);
		ed->description = str_dup(arg);
		ed->next = location->extra_descr;
		location->extra_descr = ed;
		return;
	}
}


/***************************************************************************
*	realloc_string
***************************************************************************/
static void realloc_string(char **src, char *str)
{
	char buf[MIL];
	char *pt;

	pt = str;
	while (is_space(*pt))
		pt++;

	if (*pt == '+') {
		int len;

		while (is_space(*str))
			str++;

		if (*str == '+')
			str++;
		len = (int)strlen(*src) + (int)strlen(str);

		strcpy(buf, *src);
		if (len >= MIL)
			printf_bug("realloc_string: new string too long. %d bytes", len);
		else
			strcat(buf, str);

		if (*src)
			free_string(*src);
		*src = str_dup(buf);
	} else if (*pt == '-') {
		int len;

		while (is_space(*str))
			str++;

		if (*str == '-')
			str++;

		len = (int)strlen(*src) + (int)strlen(str);
		if (len >= MIL) {
			printf_bug("realloc_string: new string too long. %d bytes", len);
			strcpy(buf, *src);
		} else {
			strcpy(buf, str);
			strcat(buf, *src);
		}

		if (*src)
			free_string(*src);
		*src = str_dup(buf);
	} else {
		if (*src)
			free_string(*src);
		*src = str_dup(str);
	}
}



char *uncolor_str2(char *txt);

/***************************************************************************
*       do_restring
*
*       restring show                   - shows the current restring buffers
*       restring name  <string> - sets the name for the restring
*       restring short <string> - sets the short name for the restring
*       restring long  <string> - sets the long description for the restring
*       restring set   <object> - performs the restring on the object using
*                                                         the specified buffers
*
*       outstanding:
*               create help for "mortal_restring"
***************************************************************************/
void do_restring(CHAR_DATA *ch, char *argument)
{
	char cmd[MSL];

	if (IS_NPC(ch)) {
		send_to_char("Mobs cannot use this command.\n\r", ch);
		return;
	}

	argument = one_argument(argument, cmd);
	if (cmd[0] == '\0' || cmd[0] == '?' || !str_cmp(cmd, "help")) {
		do_help(ch, "mortal_restring");
		return;
	}

	if (!str_prefix(cmd, "show")) {
		CHAR_DATA *vch;

		if (argument[0] != '\0') {
			if ((vch = get_char_room(ch, argument)) == NULL)
				vch = ch;

			if (IS_NPC(vch))
				vch = ch;
		} else {
			vch = ch;
		}

		if (vch != ch)
			act("You show your restring to $N.", ch, vch, vch, TO_CHAR);

		printf_to_char(vch, "`@Name``:  %s\n\r", (ch->pcdata->restring_name[0] != '\0') ? ch->pcdata->restring_name : "none.");
		printf_to_char(vch, "`@Short``: %s\n\r", (ch->pcdata->restring_short[0] != '\0') ? ch->pcdata->restring_short : "none.");
		printf_to_char(vch, "`@Long``:  %s\n\r", (ch->pcdata->restring_long[0] != '\0') ? ch->pcdata->restring_long : "none.");

		return;
	}


	/* show the actual color codes */
	if (!str_prefix(cmd, "codes")) {
		CHAR_DATA *vch;
		char color_str[MIL];

		if (argument[0] != '\0') {
			if ((vch = get_char_room(ch, argument)) == NULL)
				vch = ch;

			if (IS_NPC(vch))
				vch = ch;
		} else {
			vch = ch;
		}

		if (vch != ch)
			act("You show your restring to $N.", ch, vch, vch, TO_CHAR);

		sprintf(color_str, "%s\n\r", (ch->pcdata->restring_name[0] != '\0') ? ch->pcdata->restring_name : "none.");
		send_to_char("`@Name``:  ", vch);
		send_to_char_ascii(color_str, vch);

		sprintf(color_str, "%s\n\r", (ch->pcdata->restring_short[0] != '\0') ? ch->pcdata->restring_short : "none.");
		send_to_char("`@Short``: ", vch);
		send_to_char_ascii(color_str, vch);

		sprintf(color_str, "%s\n\r", (ch->pcdata->restring_long[0] != '\0') ? ch->pcdata->restring_long : "none.");
		send_to_char("`@Long``:  ", vch);
		send_to_char_ascii(color_str, vch);


		return;
	}



	if (!str_prefix(cmd, "clear")) {
		free_string(ch->pcdata->restring_name);
		free_string(ch->pcdata->restring_short);
		free_string(ch->pcdata->restring_long);

		ch->pcdata->restring_name = str_dup("");
		ch->pcdata->restring_short = str_dup("");
		ch->pcdata->restring_long = str_dup("");

		return;
	}

	if (!str_prefix(cmd, "clone")) {
		OBJ_DATA *obj;
		CHAR_DATA *mob;

		if ((obj = get_obj_carry(ch, argument)) != NULL) {
			realloc_string(&ch->pcdata->restring_name, obj->name);
			realloc_string(&ch->pcdata->restring_short, obj->short_descr);
			realloc_string(&ch->pcdata->restring_long, obj->description);

			send_to_char("Ok.\n\r", ch);
		} else if (IS_IMMORTAL(ch) && ((mob = get_char_world(ch, argument)) != NULL)) {
			realloc_string(&ch->pcdata->restring_name, mob->name);
			realloc_string(&ch->pcdata->restring_short, mob->short_descr);
			realloc_string(&ch->pcdata->restring_long, mob->long_descr);

			send_to_char("Ok.\n\r", ch);
		} else {
			send_to_char("You are not carrying that item.\n\r", ch);
		}
		return;
	}


	if (!str_prefix(cmd, "name")) {
		realloc_string(&ch->pcdata->restring_name, argument);
		send_to_char("Restring Name set.\n\r", ch);
		return;
	}

	if (!str_prefix(cmd, "short")) {
		realloc_string(&ch->pcdata->restring_short, argument);
		send_to_char("Restring Short Description set.\n\r", ch);
		return;
	}

	if (!str_prefix(cmd, "long")) {
		realloc_string(&ch->pcdata->restring_long, argument);
		send_to_char("Restring Long Description set.\n\r", ch);
		return;
	}

	do_restring(ch, "");
}
