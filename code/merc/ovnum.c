/***************************************************************************
 *   BAD TRIP                                                               *
 *   ovnum.c  By Brandon Griffin (2009-09-28)                               *
 *   Presented under the GNU Public License :)                              *
 ***************************************************************************/
#include <stdio.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "db.h"
#include "find.h"


AREA_DATA *grok_area(CHAR_DATA *ch, const char *arg, BUFFER *out_buffer);



AREA_DATA *grok_area(CHAR_DATA *ch, const char *arg, BUFFER *out_buffer)
{
    AREA_DATA *ad = NULL;

    if (arg[0] != '\0') {
        if (arg[0] == '?') {
            arg = arg + 1;
            if (is_number(arg)) {
                ad = area_getbycontainingvnum(parse_long(arg));
                if (ad == NULL)
                    add_buf(out_buffer, "No area found containing that vnum.\n\r");
            } else {
                add_buf(out_buffer, "Invalid argument for area.  Follow ? with vnum of room contained by area.\n\r");
            }
        } else {
            if (is_number(arg)) {
                ad = area_getbyvnum(parse_long(arg));
                if (ad == NULL)
                    add_buf(out_buffer, "No area found with that vnum.\n\r");
            } else {
                add_buf(out_buffer, "Invalid argument for area.  Supply the vnum of an area or assume the area you are in.\n\r");
            }
        }
    } else {
        if (ch->in_room == NULL)
            add_buf(out_buffer, "You aren't in a room???\n\r");
        else
            ad = ch->in_room->area;
    }

    return ad;
}

typedef bool CHECK_EXISTS_FN (long vnum);

static bool check_exists_object(long vnum)
{
    return objectprototype_getbyvnum(vnum) != NULL;
}

static bool check_exists_mob(long vnum)
{
    return get_mob_index(vnum) != NULL;
}

static CHECK_EXISTS_FN *get_check_exists_function(char type)
{
    CHECK_EXISTS_FN *fn = NULL;

    switch (type) {
      case 'o':
          fn = &check_exists_object;
          break;

      case 'm':
          fn = &check_exists_mob;
          break;

      default:
          break;
    }

    return fn;
}

static void find_empty_vnums(char type, CHAR_DATA *ch, const char *arg, BUFFER *out_buffer)
{
    AREA_DATA *ad;
    long vnum, low_empty_range = 0, high_empty_range = 0;
    char buf[MIL];
    CHECK_EXISTS_FN *check_exists_fn;

    ad = grok_area(ch, arg, out_buffer);
    if (ad == NULL)
        return;

    check_exists_fn = get_check_exists_function(type);
    if (check_exists_fn == NULL)
        return;

    snprintf(buf, MIL, "  AREA: %s\n\r\n\r", ad->name);
    add_buf(out_buffer, buf);

    for (vnum = ad->min_vnum; vnum <= ad->max_vnum; vnum++) {
        if (!(*check_exists_fn)(vnum)) {
            if (low_empty_range == 0) {
                /* first time range. */
                low_empty_range = vnum;
                high_empty_range = vnum;
            } else
                if (vnum == (high_empty_range + 1)) {
                    high_empty_range = vnum;
                } else {
                    /* finish old range.  start new range.   */
                    if (high_empty_range == low_empty_range)
                        snprintf(buf, MIL, "    %ld\n\r", low_empty_range);
                    else
                        snprintf(buf, MIL, "    %ld - %ld\n\r", low_empty_range, high_empty_range);

                    add_buf(out_buffer, buf);
                    low_empty_range = vnum;
                    high_empty_range = vnum;
                }
        }
    }

    if (low_empty_range > 0) {
        /* Last range could not have been written to output, so write now. */
        if (high_empty_range == low_empty_range)
            snprintf(buf, MIL, "    %ld\n\r\n\r", low_empty_range);
        else
            snprintf(buf, MIL, "    %ld - %ld\n\r\n\r", low_empty_range, high_empty_range);

        add_buf(out_buffer, buf);
    } else {
        add_buf(out_buffer, "    No empty vnums.\n\r\n\r");
    }
}


void ovnum_find_empty(CHAR_DATA *ch, const char *arg, BUFFER *out_buffer)
{
    add_buf(out_buffer, "`#QUERY``: ovnum find empty vnums in area.\n\r\n\r");

    find_empty_vnums('o', ch, arg, out_buffer);
}

void mvnum_find_empty(CHAR_DATA *ch, const char *arg, BUFFER *out_buffer)
{
    add_buf(out_buffer, "`#QUERY``: mvnum find empty vnums in area.\n\r\n\r");

    find_empty_vnums('m', ch, arg, out_buffer);
}
