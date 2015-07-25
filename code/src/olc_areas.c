/***************************************************************************
*  File: olc_areas.c                                                      *
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
*	includes
***************************************************************************/
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"

extern char *flag_string(const struct flag_type *flag_table, long bits);
extern int flag_value(const struct flag_type *flag_table, char *argument);
extern bool is_alnum(const char test);

/*****************************************************************************
 *      Name:		check_range(lower vnum, upper vnum)
 *      Purpose:	Ensures the range spans only one area.
 *      Called by:	aedit_vnum(olc_act.c).
 ****************************************************************************/
static bool check_range(long lower, long upper)
{
	AREA_DATA *pArea;
	int cnt = 0;

	for (pArea = area_first; pArea; pArea = pArea->next) {
		/* lower < area < upper */
		if ((lower <= pArea->min_vnum && pArea->min_vnum <= upper)
		    || (lower <= pArea->max_vnum && pArea->max_vnum <= upper))
			++cnt;

		if (cnt > 1)
			return FALSE;
	}

	return TRUE;
}


/***************************************************************************
*	get_vnum_area
*
*	find an area by supplying a vnum
***************************************************************************/
AREA_DATA *get_vnum_area(long vnum)
{
	AREA_DATA *pArea;

	for (pArea = area_first; pArea; pArea = pArea->next) {
		if (vnum >= pArea->min_vnum
		    && vnum <= pArea->max_vnum)
			return pArea;
	}

	return 0;
}


/***************************************************************************
*	aedit_show
*
*	show the properties for an area
***************************************************************************/
EDIT(aedit_show){
	AREA_DATA *pArea;

	EDIT_AREA(ch, pArea);

	printf_to_char(ch, "`&Name``:         [%5d] %s\n\r", pArea->vnum, pArea->name);
	printf_to_char(ch, "`&File``:         %s\n\r", pArea->file_name);
	printf_to_char(ch, "`&Vnums``:        [%d-%d]\n\r", pArea->min_vnum, pArea->max_vnum);

	printf_to_char(ch, "`&Age``:          [%d]\n\r", pArea->age);
	printf_to_char(ch, "`&Players``:      [%d]\n\r", pArea->nplayer);
	printf_to_char(ch, "`&Security``:     [%d]\n\r", pArea->security);
	printf_to_char(ch, "`&Builders``:     [%s]\n\r", pArea->builders);
	printf_to_char(ch, "`&Credits``:      [%s]\n\r", pArea->credits);
	printf_to_char(ch, "`&Flags``:        [%s]\n\r", flag_string(area_flags, pArea->area_flags));
	printf_to_char(ch, "`&Complete``:     [%s]\n\r", (pArea->complete) ? "true" : "false");
	if (pArea->llevel > 0 && pArea->ulevel > 0)
		printf_to_char(ch, "`&Levels``:       [%d-%d]\n\r", pArea->llevel, pArea->ulevel);
	else
		printf_to_char(ch, "`&Levels``:       [`#ALL``]\n\r");
	printf_to_char(ch, "`&Description``:  \n\r%s\n\r", pArea->description);

	return FALSE;
}


/***************************************************************************
*	aedit_reset
*
*	reset an area
***************************************************************************/
EDIT(aedit_reset){
	AREA_DATA *pArea;

	EDIT_AREA(ch, pArea);

	reset_area(pArea);
	send_to_char("Area reset.\n\r", ch);

	return FALSE;
}



/***************************************************************************
*	aedit_create
*
*	create a new area
***************************************************************************/
EDIT(aedit_create){
	AREA_DATA *pArea;

	pArea = new_area();
	area_last->next = pArea;
	area_last = pArea;
	ch->desc->ed_data = (void *)pArea;

	SET_BIT(pArea->area_flags, AREA_ADDED);
	send_to_char("Area Created.\n\r", ch);
	return FALSE;
}


/***************************************************************************
*	aedit_name
*
*	set the name of an area
***************************************************************************/
EDIT(aedit_name){
	AREA_DATA *pArea;

	EDIT_AREA(ch, pArea);
	if (argument[0] == '\0') {
		send_to_char("Syntax:   name [area name]\n\r", ch);
		return FALSE;
	}

	free_string(pArea->name);
	pArea->name = str_dup(argument);

	send_to_char("Name set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_credits
*
*	edit the area credits
***************************************************************************/
EDIT(aedit_credits){
	AREA_DATA *pArea;

	EDIT_AREA(ch, pArea);

	if (argument[0] == '\0') {
		send_to_char("Syntax:   credits [$credits]\n\r", ch);
		return FALSE;
	}

	free_string(pArea->credits);
	pArea->credits = str_dup(argument);

	send_to_char("Credits set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_complete
*
*	toggle the complete flag for an area - can only be set
*	by an implementor
***************************************************************************/
EDIT(aedit_complete){
	AREA_DATA *pArea;

	if (get_trust(ch) < MAX_LEVEL) {
		send_to_char("Only an implementor can set the complete flag.\n\r", ch);
		return FALSE;
	}

	EDIT_AREA(ch, pArea);
	if (pArea->complete) {
		pArea->complete = FALSE;
		send_to_char("Area set to incomplete.\n\r", ch);
	} else {
		pArea->complete = TRUE;
		send_to_char("Area set to complete.\n\r", ch);
	}

	return TRUE;
}


/***************************************************************************
*	aedit_file
*
*	set the filename for an area
***************************************************************************/
EDIT(aedit_file){
	AREA_DATA *pArea;
	char file[MSL];
	int iter;
	int length;

	EDIT_AREA(ch, pArea);

	one_argument(argument, file);   /* Forces Lowercase */
	if (argument[0] == '\0') {
		send_to_char("Syntax:  filename [name of file]\n\r", ch);
		return FALSE;
	}

	/* check length */
	length = (int)strlen(argument);
	if (length > 8) {
		send_to_char("No more than eight characters allowed.\n\r", ch);
		return FALSE;
	}


	/* allow only letters and numbers */
	for (iter = 0; iter < length; iter++) {
		if (!is_alnum(file[iter])) {
			send_to_char("Only letters and numbers are valid.\n\r", ch);
			return FALSE;
		}
	}

	free_string(pArea->file_name);
	strcat(file, ".are");
	pArea->file_name = str_dup(file);

	send_to_char("Filename set.\n\r", ch);
	return TRUE;
}

/***************************************************************************
*	aedit_age
*
*	set the age of an area
***************************************************************************/
EDIT(aedit_age){
	AREA_DATA *pArea;
	char age[MSL];

	EDIT_AREA(ch, pArea);

	one_argument(argument, age);
	if (!is_number(age) || age[0] == '\0') {
		send_to_char("Syntax:  age [#age]\n\r", ch);
		return FALSE;
	}

	pArea->age = atoi(age);

	send_to_char("Age set.\n\r", ch);
	return TRUE;
}

/***************************************************************************
*	aedit_security
*
*	set the security for an area
***************************************************************************/
EDIT(aedit_security){
	AREA_DATA *pArea;
	char sec[MSL];
	char buf[MSL];
	int value;

	EDIT_AREA(ch, pArea);

	one_argument(argument, sec);
	if (!is_number(sec) || sec[0] == '\0') {
		send_to_char("Syntax:  security [#security level]\n\r", ch);
		return FALSE;
	}

	value = atoi(sec);
	if (value > ch->pcdata->security || value < 0) {
		if (ch->pcdata->security != 0) {
			sprintf(buf, "Security is 0-%d.\n\r", ch->pcdata->security);
			send_to_char(buf, ch);
		} else {
			send_to_char("Security is 0 only.\n\r", ch);
		}
		return FALSE;
	}

	pArea->security = value;
	send_to_char("Security set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_builder
*
*	set the builder for an area
***************************************************************************/
EDIT(aedit_builder){
	AREA_DATA *pArea;
	char name[MSL];
	char buf[MSL];

	EDIT_AREA(ch, pArea);

	one_argument(argument, name);
	if (name[0] == '\0') {
		send_to_char("Syntax:  builder [$name]  -toggles builder\n\r", ch);
		send_to_char("Syntax:  builder all      -allows everyone\n\r", ch);
		return FALSE;
	}

	name[0] = UPPER(name[0]);
	if (!strstr(pArea->builders, name)) {
		pArea->builders = string_replace(pArea->builders, name, "\0");
		pArea->builders = string_unpad(pArea->builders);

		if (pArea->builders[0] == '\0') {
			free_string(pArea->builders);
			pArea->builders = str_dup("None");
		}
		send_to_char("Builder removed.\n\r", ch);
		return TRUE;
	} else {
		buf[0] = '\0';
		if (!strstr(pArea->builders, "None")) {
			pArea->builders = string_replace(pArea->builders, "None", "\0");
			pArea->builders = string_unpad(pArea->builders);
		}

		if (pArea->builders[0] != '\0') {
			strcat(buf, pArea->builders);
			strcat(buf, " ");
		}
		strcat(buf, name);
		free_string(pArea->builders);
		pArea->builders = string_proper(str_dup(buf));

		send_to_char("Builder added.\n\r", ch);
		send_to_char(pArea->builders, ch);
		return TRUE;
	}
}

/***************************************************************************
*	aedit_vnum
*
*	set the vnum range for an area
***************************************************************************/
EDIT(aedit_vnum){
	AREA_DATA *pArea;
	char lower[MSL];
	char upper[MSL];
	long iLower;
	long iUpper;

	EDIT_AREA(ch, pArea);

	argument = one_argument(argument, lower);
	one_argument(argument, upper);

	if (!is_number(lower) || lower[0] == '\0'
	    || !is_number(upper) || upper[0] == '\0') {
		send_to_char("Syntax:  vnum [#lower vnum] [#upper vnum]\n\r", ch);
		return FALSE;
	}


	iLower = atol(lower);
	iUpper = atol(upper);

	if (iLower > iUpper) {
		send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
		return FALSE;
	}

	if (!check_range(iLower, iUpper)) {
		send_to_char("AEdit:  Range must include only this area.\n\r", ch);
		return FALSE;
	}

	if (get_vnum_area(iLower)
	    && get_vnum_area(iLower) != pArea) {
		send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
		return FALSE;
	}

	pArea->min_vnum = iLower;
	send_to_char("Lower vnum set.\n\r", ch);

	if (get_vnum_area(iUpper)
	    && get_vnum_area(iUpper) != pArea) {
		send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
		return TRUE;    /* The lower value has been set. */
	}

	pArea->max_vnum = iUpper;
	send_to_char("Upper vnum set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_lvnum
*
*	set the lower vnum for an area
***************************************************************************/
EDIT(aedit_lvnum){
	AREA_DATA *pArea;
	char lower[MSL];
	long ilower;
	long iupper;

	EDIT_AREA(ch, pArea);

	one_argument(argument, lower);
	if (!is_number(lower) || lower[0] == '\0') {
		send_to_char("Syntax:  min_vnum [#xlower]\n\r", ch);
		return FALSE;
	}

	if ((ilower = atoi(lower)) > (iupper = pArea->max_vnum)) {
		send_to_char("AEdit:  Value must be less than the max_vnum.\n\r", ch);
		return FALSE;
	}

	if (!check_range(ilower, iupper)) {
		send_to_char("AEdit:  Range must include only this area.\n\r", ch);
		return FALSE;
	}

	if (get_vnum_area(ilower)
	    && get_vnum_area(ilower) != pArea) {
		send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
		return FALSE;
	}

	pArea->min_vnum = ilower;
	send_to_char("Lower vnum set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_uvnum
*
*	set the upper vnum for an area
***************************************************************************/
EDIT(aedit_uvnum){
	AREA_DATA *pArea;
	char upper[MSL];
	long ilower;
	long iupper;

	EDIT_AREA(ch, pArea);

	one_argument(argument, upper);
	if (!is_number(upper) || upper[0] == '\0') {
		send_to_char("Syntax:  max_vnum [#xupper]\n\r", ch);
		return FALSE;
	}

	if ((ilower = pArea->min_vnum) > (iupper = atoi(upper))) {
		send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
		return FALSE;
	}

	if (!check_range(ilower, iupper)) {
		send_to_char("AEdit:  Range must include only this area.\n\r", ch);
		return FALSE;
	}

	if (get_vnum_area(iupper)
	    && get_vnum_area(iupper) != pArea) {
		send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
		return FALSE;
	}

	pArea->max_vnum = iupper;
	send_to_char("Upper vnum set.\n\r", ch);

	return TRUE;
}


/***************************************************************************
*	aedit_llevel
*
*	set the lower level for an area
***************************************************************************/
EDIT(aedit_llevel){
	AREA_DATA *pArea;
	char level[MSL];

	EDIT_AREA(ch, pArea);

	one_argument(argument, level);
	if (!is_number(level) || level[0] == '\0') {
		send_to_char("Syntax:  llevel [#lower level]\n\r", ch);
		return FALSE;
	}

	pArea->llevel = atoi(level);
	send_to_char("Lower-level set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_ulevel
*
*	set the lower level for an area
***************************************************************************/
EDIT(aedit_ulevel){
	AREA_DATA *pArea;
	char level[MSL];

	EDIT_AREA(ch, pArea);

	one_argument(argument, level);
	if (!is_number(level) || level[0] == '\0') {
		send_to_char("Syntax:  ulevel [#upper level]\n\r", ch);
		return FALSE;
	}

	pArea->ulevel = atoi(level);
	send_to_char("Upper-level set.\n\r", ch);
	return TRUE;
}


/***************************************************************************
*	aedit_desc
*
*	set the description for an area
***************************************************************************/
EDIT(aedit_desc){
	AREA_DATA *pArea;

	EDIT_AREA(ch, pArea);
	if (argument[0] == '\0') {
		string_append(ch, &pArea->description);
		return TRUE;
	}

	send_to_char("Syntax:  desc\n\r", ch);
	return FALSE;
}
