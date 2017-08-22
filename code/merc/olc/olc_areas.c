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

#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"

#include <stdio.h>
#include <string.h>
#ifndef S_SPLINT_S
#include <ctype.h>
#endif


extern char *string_replace(char *orig, char *old, char *new);
extern char *string_unpad(char *argument);
extern char *string_proper(char *argument);

static bool check_range(long lower, long upper);


EDIT(aedit_show)
{
    struct area_data *pArea;

    EDIT_AREA(ch, pArea);

    printf_to_char(ch, "`&Name``:         [%5d] %s\n\r", pArea->vnum, pArea->name);
    printf_to_char(ch, "`&File``:         %s\n\r", pArea->file_name);
    printf_to_char(ch, "`&Vnums``:        [%d-%d]\n\r", pArea->min_vnum, pArea->max_vnum);

    printf_to_char(ch, "`&Age``:          [%d]\n\r", pArea->age);
    printf_to_char(ch, "`&Players``:      [%d]\n\r", pArea->nplayer);
    printf_to_char(ch, "`&Security``:     [%d]\n\r", pArea->security);
    printf_to_char(ch, "`&Flags``:        [%s]\n\r", flag_string(area_flags, pArea->area_flags));
    if (pArea->llevel > 0 && pArea->ulevel > 0)
        printf_to_char(ch, "`&Levels``:       [%d-%d]\n\r", pArea->llevel, pArea->ulevel);
    else
        printf_to_char(ch, "`&Levels``:       [`#ALL``]\n\r");
    printf_to_char(ch, "`&Description``:  \n\r%s\n\r", pArea->description);

    return false;
}

EDIT(aedit_reset)
{
    struct area_data *pArea;

    EDIT_AREA(ch, pArea);

    reset_area(pArea);
    send_to_char("Area reset.\n\r", ch);

    return false;
}

EDIT(aedit_create)
{
    struct area_data *pArea;

    pArea = area_new(0);
    ch->desc->ed_data = (void *)pArea;

    SET_BIT(pArea->area_flags, AREA_ADDED);
    send_to_char("Area Created.\n\r", ch);
    return false;
}

EDIT(aedit_name)
{
    struct area_data *pArea;

    EDIT_AREA(ch, pArea);
    if (argument[0] == '\0') {
        send_to_char("Syntax:   name [area name]\n\r", ch);
        return false;
    }

    free_string(pArea->name);
    pArea->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return true;
}

EDIT(aedit_file)
{
    struct area_data *pArea;
    char file[MAX_STRING_LENGTH];
    int iter;
    int length;

    EDIT_AREA(ch, pArea);

    one_argument(argument, file);   /* Forces Lowercase */
    if (argument[0] == '\0') {
        send_to_char("Syntax:  filename [name of file]\n\r", ch);
        return false;
    }

    /* check length */
    length = (int)strlen(argument);
    if (length > 8) {
        send_to_char("No more than eight characters allowed.\n\r", ch);
        return false;
    }


    /* allow only letters and numbers */
    for (iter = 0; iter < length; iter++) {
        if (!isalnum((int)(file[iter]))) {
            send_to_char("Only letters and numbers are valid.\n\r", ch);
            return false;
        }
    }

    free_string(pArea->file_name);
    strcat(file, ".are");
    pArea->file_name = str_dup(file);

    send_to_char("Filename set.\n\r", ch);
    return true;
}

EDIT(aedit_age)
{
    struct area_data *pArea;
    char age[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, age);
    if (!is_number(age) || age[0] == '\0') {
        send_to_char("Syntax:  age [#age]\n\r", ch);
        return false;
    }

    pArea->age = parse_int(age);

    send_to_char("Age set.\n\r", ch);
    return true;
}

EDIT(aedit_security)
{
    struct area_data *pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int value;

    EDIT_AREA(ch, pArea);

    one_argument(argument, sec);
    if (!is_number(sec) || sec[0] == '\0') {
        send_to_char("Syntax:  security [#security level]\n\r", ch);
        return false;
    }

    value = parse_int(sec);
    if (value > ch->pcdata->security || value < 0) {
        if (ch->pcdata->security != 0) {
            sprintf(buf, "Security is 0-%d.\n\r", ch->pcdata->security);
            send_to_char(buf, ch);
        } else {
            send_to_char("Security is 0 only.\n\r", ch);
        }
        return false;
    }

    pArea->security = value;
    send_to_char("Security set.\n\r", ch);
    return true;
}

EDIT(aedit_vnum)
{
    struct area_data *pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    long iLower;
    long iUpper;

    EDIT_AREA(ch, pArea);

    argument = one_argument(argument, lower);
    one_argument(argument, upper);

    if (!is_number(lower) || lower[0] == '\0'
        || !is_number(upper) || upper[0] == '\0') {
        send_to_char("Syntax:  vnum [#lower vnum] [#upper vnum]\n\r", ch);
        return false;
    }


    iLower = parse_long(lower);
    iUpper = parse_long(upper);

    if (iLower > iUpper) {
        send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
        return false;
    }

    if (!check_range(iLower, iUpper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (area_getbycontainingvnum(iLower) != pArea) {
        send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
        return false;
    }

    pArea->min_vnum = iLower;
    send_to_char("Lower vnum set.\n\r", ch);

    if (area_getbycontainingvnum(iUpper) != pArea) {
        send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
        return true;    /* The lower value has been set. */
    }

    pArea->max_vnum = iUpper;
    send_to_char("Upper vnum set.\n\r", ch);
    return true;
}

EDIT(aedit_lvnum)
{
    struct area_data *pArea;
    char lower[MAX_STRING_LENGTH];
    long ilower;
    long iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, lower);
    if (!is_number(lower) || lower[0] == '\0') {
        send_to_char("Syntax:  min_vnum [#xlower]\n\r", ch);
        return false;
    }

    if ((ilower = parse_int(lower)) > (iupper = pArea->max_vnum)) {
        send_to_char("AEdit:  Value must be less than the max_vnum.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (area_getbycontainingvnum(ilower) != pArea) {
        send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
        return false;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\n\r", ch);
    return true;
}

EDIT(aedit_uvnum)
{
    struct area_data *pArea;
    char upper[MAX_STRING_LENGTH];
    long ilower;
    long iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, upper);
    if (!is_number(upper) || upper[0] == '\0') {
        send_to_char("Syntax:  max_vnum [#xupper]\n\r", ch);
        return false;
    }

    if ((ilower = pArea->min_vnum) > (iupper = parse_int(upper))) {
        send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (area_getbycontainingvnum(iupper) != pArea) {
        send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
        return false;
    }

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\n\r", ch);

    return true;
}

EDIT(aedit_llevel)
{
    struct area_data *pArea;
    char level[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, level);
    if (!is_number(level) || level[0] == '\0') {
        send_to_char("Syntax:  llevel [#lower level]\n\r", ch);
        return false;
    }

    pArea->llevel = parse_int(level);
    send_to_char("Lower-level set.\n\r", ch);
    return true;
}

EDIT(aedit_ulevel)
{
    struct area_data *pArea;
    char level[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, level);
    if (!is_number(level) || level[0] == '\0') {
        send_to_char("Syntax:  ulevel [#upper level]\n\r", ch);
        return false;
    }

    pArea->ulevel = parse_int(level);
    send_to_char("Upper-level set.\n\r", ch);
    return true;
}

EDIT(aedit_desc)
{
    struct area_data *pArea;

    EDIT_AREA(ch, pArea);
    if (argument[0] != '\0') {
        olc_area_setdescription(pArea, argument);
        send_to_char("Description set.\n\r", ch);
        return true;
    }

    olc_start_string_editor(ch, pArea, olc_area_getdescription, olc_area_setdescription);
    return true;
}


static bool check_range(long lower, long upper)
{
    struct area_data *pArea;
    int cnt = 0;

    pArea = area_iterator_start(NULL);
    while (pArea != NULL) {
        if ((lower <= pArea->min_vnum && pArea->min_vnum <= upper)
            || (lower <= pArea->max_vnum && pArea->max_vnum <= upper))
            ++cnt;

        // TODO - would love to short circuit this because I don't really
        // care how many overlaps there are, but iterator needs to be freed
        // first.
        pArea = area_iterator(pArea, NULL);
    }

    return cnt == 0;
}
