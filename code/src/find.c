#include <stdio.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "db.h"
#include "find.h"


const struct cmp_vars_obj_index_data objprototype_flags[] =
{
    { "name",   &objprototype_cmp_name	},
    { "short",  &objprototype_cmp_short	},
    { "long",   &objprototype_cmp_long	},
    { "type",   &objprototype_cmp_type	},
    { "extra",  &objprototype_cmp_extra	},
    { "wear",   &objprototype_cmp_wear	},
    { "weight", &objprototype_cmp_weight },
    { "cost",   &objprototype_cmp_cost	},
    { "level",  &objprototype_cmp_level	},
    { "",	    NULL		}
};

const struct cmp_vars_mob_index_data mob_idx_flags[] =
{
    { "name",     &mob_idx_cmp_name	   },
    { "short",    &mob_idx_cmp_short   },
    { "long",     &mob_idx_cmp_long	   },
    { "race",     &mob_idx_cmp_race	   },
    { "level",    &mob_idx_cmp_level   },
    { "sex",      &mob_idx_cmp_sex	   },
    { "gold",     &mob_idx_cmp_wealth  },
    { "offense",  &mob_idx_cmp_offense },
    { "form",     &mob_idx_cmp_form	   },
    { "act",      &mob_idx_cmp_act	   },
    { "",	      NULL		   }
};


static void help_ovnum_properties(CHAR_DATA *ch);

extern void ovnum_find_empty(CHAR_DATA *ch, char *arg, BUFFER *out_buffer);
extern void mvnum_find_empty(CHAR_DATA *ch, char *arg, BUFFER *out_buffer);
extern AREA_DATA *grok_area(CHAR_DATA *ch, char *arg, BUFFER *out_buffer);


GAMEOBJECT *get_object_by_itemtype_and_room(int item_type, ROOM_INDEX_DATA *room, CHAR_DATA *ch)
{
    GAMEOBJECT *instance = NULL;

    for (instance = room->contents; instance != NULL; instance = instance->next_content)
	if (instance->item_type == item_type && (ch == NULL || can_see_obj(ch, instance)))
	    break;

    return instance;
}




/***************************************************************************
 *	where functions - owhere mwhere
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/
const struct cmp_vars_gameobject obj_flags[] =
{
    { "vnum",     &obj_cmp_vnum	},
    { "name",     &obj_cmp_name	},
    { "short",    &obj_cmp_short	},
    { "long",     &obj_cmp_long	},
    { "type",     &obj_cmp_type	},
    { "extra",    &obj_cmp_extra	},
    { "wear",     &obj_cmp_wear	},
    { "location", &obj_cmp_location },
    { "weight",   &obj_cmp_weight	},
    { "cost",     &obj_cmp_cost	},
    { "level",    &obj_cmp_level	},
    { "",	      NULL		}
};


/**
 * owhere <var> <value>
 */
void do_owhere(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    OBJ_CMP_FN *cmp_fn;
    char buf[MIL];
    char arg[MIL];
    int iter;

    if (ch == NULL || IS_NPC(ch))
	return;

    if (argument[0] == '\0') {
	send_to_char("\n\rFind what?\n\r", ch);
	return;
    }

    cmp_fn = NULL;
    one_argument(argument, arg);

    if (argument[0] == '?' || !str_prefix(argument, "help")) {
	do_help(ch, "owhere");
	return;
    }

    buffer = new_buf();
    if (!str_prefix(argument, "list")) {
	add_buf(buffer, "owhere: searchable property list\n\r");
	for (iter = 0; obj_flags[iter].var[0] != '\0'; iter++) {
	    sprintf(buf, "%-18.17s", obj_flags[iter].var);
	    add_buf(buffer, buf);
	    if ((iter % 2) == 1)
		add_buf(buffer, "\n\r");
	}
	add_buf(buffer, "\n\r");
	page_to_char(buf_string(buffer), ch);
	return;
    }

    sprintf(buf, "`#QUERY``: owhere %s\n\r\n\r", argument);
    add_buf(buffer, buf);

    for (iter = 0; obj_flags[iter].var[0] != '\0'; iter++) {
	if (!str_prefix(arg, obj_flags[iter].var)) {
	    cmp_fn = (OBJ_CMP_FN *)obj_flags[iter].fn;
	    argument = one_argument(argument, arg);
	    break;
	}
    }

    if (cmp_fn == NULL)
	cmp_fn = obj_cmp_name;


    if (argument[0] == '?' || argument[0] == '\0') {
	clear_buf(buffer);
	sprintf(buf, "`#SYNTAX``:\n\r    owhere %s <value>:\n\r\n\r", arg);
	add_buf(buffer, buf);

	(*cmp_fn)(object_iterator_start(&object_empty_filter), argument, buffer);
	page_to_char(buf_string(buffer), ch);
    } else {
	GAMEOBJECT *obj, *opending;
	GAMEOBJECT *in_obj;
	char *clr1;
	char *clr2;
	int number;

	number = 0;

	opending = object_iterator_start(&object_empty_filter);
	while ((obj = opending) != NULL) {
	    opending = object_iterator(obj, &object_empty_filter);

	    if (can_see_obj(ch, obj)
		    && (*cmp_fn)(obj, argument, NULL)) {
		number++;
		if (number == 1) {
		    sprintf(buf, "#   vnum   name                        where                      room\n\r");
		    add_buf(buffer, buf);
		    sprintf(buf, "=== ====== =========================== ========================== =====\n\r");

		    add_buf(buffer, buf);
		}

		for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj)
		    continue;

		if (in_obj->carried_by != NULL
			&& can_see(ch, in_obj->carried_by)
			&& in_obj->carried_by->in_room != NULL) {
		    clr1 = uncolor_str(obj->short_descr);
		    clr2 = uncolor_str(PERS(in_obj->carried_by, ch));
		    sprintf(buf, "%-3d %-7ld  %-26.26s  %-25.25s  %-7ld\n\r",
			    number,
			    obj->objprototype->vnum,
			    clr1,
			    clr2,
			    in_obj->carried_by->in_room->vnum);
		    free_string(clr1);
		    free_string(clr2);
		} else if (in_obj->in_room != NULL && can_see_room(ch, in_obj->in_room)) {
		    clr1 = uncolor_str(obj->short_descr);
		    clr2 = uncolor_str(in_obj->in_room->name);

		    sprintf(buf, "%-3d %-7ld  %-26.26s  %-25.25s  %-7ld\n\r",
			    number,
			    obj->objprototype->vnum,
			    clr1,
			    clr2,
			    in_obj->in_room->vnum);
		    free_string(clr1);
		    free_string(clr2);
		} else {
		    clr1 = uncolor_str(obj->short_descr);

		    sprintf(buf, "%-3d %-7ld  %-26.26s\n\r",
			    number,
			    obj->objprototype->vnum,
			    clr1);
		    free_string(clr1);
		}

		buf[0] = UPPER(buf[0]);
		add_buf(buffer, buf);

		if (number >= MAX_RETURN)
		    break;
	    }
	}


	if (number == 0)
	    send_to_char("Nothing like that in heaven or earth.\n\r", ch);
	else
	    page_to_char(buf_string(buffer), ch);
    }

    free_buf(buffer);
}


/***************************************************************************
 *	obj_cmp_vnum
 ***************************************************************************/
bool obj_cmp_vnum(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's vnum.\n\r");
    return cmp_fn_number(((obj->objprototype != NULL) ? obj->objprototype->vnum : 0), arg);
}

/***************************************************************************
 *	obj_cmp_name
 ***************************************************************************/
bool obj_cmp_name(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's name.\n\r");
    return cmp_fn_string(obj->name, arg);
}

/***************************************************************************
 *	obj_cmp_short
 ***************************************************************************/
bool obj_cmp_short(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's short description.\n\r");
    return cmp_fn_string(obj->short_descr, arg);
}

/***************************************************************************
 *	obj_cmp_long
 ***************************************************************************/
bool obj_cmp_long(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's long description.\n\r");
    return cmp_fn_string(obj->description, arg);
}

/***************************************************************************
 *	obj_cmp_type
 ***************************************************************************/
bool obj_cmp_type(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's type.\n\r");
	add_buf(buf, "available type flags:\n\r");
    }
    return cmp_fn_index((long)obj->item_type, arg, type_flags, buf);
}

/***************************************************************************
 *	obj_cmp_location
 ***************************************************************************/
bool obj_cmp_location(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's wear location.\n\r");
	add_buf(buf, "available wear flags:\n\r");
    }
    return cmp_fn_index((long)obj->wear_loc, arg, wear_loc_flags, buf);
}

/***************************************************************************
 *	obj_cmp_weight
 ***************************************************************************/
bool obj_cmp_weight(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's weight.\n\r");
    return cmp_fn_number((obj->weight / 10), arg);
}

/***************************************************************************
 *	obj_cmp_cost
 ***************************************************************************/
bool obj_cmp_cost(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's cost.\n\r");
    return cmp_fn_number((long)obj->cost, arg);
}

/***************************************************************************
 *	obj_cmp_level
 ***************************************************************************/
bool obj_cmp_level(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's level.\n\r");
    return cmp_fn_number(obj->level, arg);
}

/***************************************************************************
 *	obj_cmp_extra
 ***************************************************************************/
bool obj_cmp_extra(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's wear location.\n\r");
	add_buf(buf, "available extra flags:\n\r");
    }
    return cmp_fn_flag((long)obj->extra_flags, arg, extra_flags, buf);
}

/***************************************************************************
 *	obj_cmp_wear
 ***************************************************************************/
bool obj_cmp_wear(GAMEOBJECT *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's wear location.\n\r");
	add_buf(buf, "available wear flags:\n\r");
    }
    return cmp_fn_flag((long)obj->wear_flags, arg, wear_flags, buf);
}



/***************************************************************************
 *	characters
 ***************************************************************************/
const struct cmp_vars_char_data char_flags[] =
{
    { "vnum",     &char_cmp_vnum	 },
    { "name",     &char_cmp_name	 },
    { "short",    &char_cmp_short	 },
    { "long",     &char_cmp_long	 },
    { "race",     &char_cmp_race	 },
    { "level",    &char_cmp_level	 },
    { "sex",      &char_cmp_sex	 },
    { "hit",      &char_cmp_hit	 },
    { "mana",     &char_cmp_mana	 },
    { "move",     &char_cmp_move	 },
    { "max_hit",  &char_cmp_max_hit	 },
    { "max_mana", &char_cmp_max_mana },
    { "max_move", &char_cmp_max_move },
    { "gold",     &char_cmp_gold	 },
    { "silver",   &char_cmp_silver	 },
    { "offense",  &char_cmp_offense	 },
    { "form",     &char_cmp_form	 },
    { "act",      &char_cmp_act	 },
    { "player",   &char_cmp_player	 },
    { "",	      NULL		 }
};

/***************************************************************************
 *	do_mwhere
 *
 *	new syntax:
 *		mwhere <var> <value>
 *
 ***************************************************************************/
void do_mwhere(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    CHAR_CMP_FN *cmp_fn;
    char buf[MIL];
    char arg[MIL];
    int iter;

    if (ch == NULL || IS_NPC(ch))
	return;

    if (argument[0] == '\0') {
	send_to_char("\n\rFind what?\n\r", ch);
	return;
    }


    if (argument[0] == '?' || !str_prefix(argument, "help")) {
	do_help(ch, "mwhere");
	return;
    }

    buffer = new_buf();
    if (!str_prefix(argument, "list")) {
	add_buf(buffer, "mwhere: searchable property list\n\r");
	for (iter = 0; char_flags[iter].var[0] != '\0'; iter++) {
	    sprintf(buf, "%-18.17s", char_flags[iter].var);
	    add_buf(buffer, buf);
	    if ((iter % 2) == 1)
		add_buf(buffer, "\n\r");
	}
	add_buf(buffer, "\n\r");
	page_to_char(buf_string(buffer), ch);
	return;
    }

    sprintf(buf, "`#QUERY``: mwhere %s\n\r\n\r", argument);
    add_buf(buffer, buf);

    if (argument[0] == '\0') {
	cmp_fn = char_cmp_name;
    } else {
	cmp_fn = NULL;
	one_argument(argument, arg);

	for (iter = 0; char_flags[iter].var[0] != '\0'; iter++) {
	    if (!str_prefix(arg, char_flags[iter].var)) {
		cmp_fn = (CHAR_CMP_FN *)char_flags[iter].fn;
		argument = one_argument(argument, arg);
		break;
	    }
	}

	if (cmp_fn == NULL)
	    cmp_fn = char_cmp_name;
    }

    if (argument[0] == '?' || argument[0] == '\0') {
	clear_buf(buffer);
	sprintf(buf, "`#SYNTAX``:\n\r"
		"       mwhere %s <value>:\n\r\n\r", arg);
	add_buf(buffer, buf);

	(*cmp_fn)(char_list, argument, buffer);
	page_to_char(buf_string(buffer), ch);
    } else {
	CHAR_DATA *vch;
	char *clr1;
	char *clr2;
	int number;

	number = 0;
	for (vch = char_list; vch != NULL; vch = vch->next) {
	    if (can_see(ch, vch)
		    && vch->in_room != NULL
		    && (*cmp_fn)(vch, argument, NULL)) {
		number++;

		if (number == 1) {
		    sprintf(buf, "#   vnum   name                        where                      room\n\r");
		    add_buf(buffer, buf);
		    sprintf(buf, "=== ====== =========================== ========================== =====\n\r");

		    add_buf(buffer, buf);
		}

		clr1 = uncolor_str(IS_NPC(vch) ? vch->short_descr : vch->name);
		clr2 = uncolor_str(vch->in_room->name);
		sprintf(buf, "%-3d %-7ld  %-26.26s  %-25.25s  %-7ld\n\r",
			number,
			IS_NPC(vch) ? vch->mob_idx->vnum : 0,
			clr1,
			clr2,
			vch->in_room->vnum);
		free_string(clr1);
		free_string(clr2);

		buf[0] = UPPER(buf[0]);
		add_buf(buffer, buf);

		if (number >= MAX_RETURN)
		    break;
	    }
	}


	if (number == 0)
	    send_to_char("Nothing like that in heaven or earth.\n\r", ch);
	else
	    page_to_char(buf_string(buffer), ch);
    }

    free_buf(buffer);
}


/***************************************************************************
 *	char_cmp_vnum
 ***************************************************************************/
bool char_cmp_vnum(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's vnum.\n\r");
    return cmp_fn_number((IS_NPC(vch) ? vch->mob_idx->vnum : 0), arg);
}


/***************************************************************************
 *	char_cmp_name
 ***************************************************************************/
bool char_cmp_name(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by characters name.\n\r");
    return cmp_fn_string(vch->name, arg);
}


/***************************************************************************
 *	char_cmp_short
 ***************************************************************************/
bool char_cmp_short(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's short description.\n\r");
    return IS_NPC(vch) ? cmp_fn_string(vch->short_descr, arg) : false;
}

/***************************************************************************
 *	char_cmp_long
 ***************************************************************************/
bool char_cmp_long(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's long description.\n\r");
    return IS_NPC(vch) ? cmp_fn_string(vch->long_descr, arg) : false;
}

/***************************************************************************
 *	char_cmp_race
 ***************************************************************************/
bool char_cmp_race(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	char flag[MSL];
	int col = 0;
	int iter;

	add_buf(buf, "search by a mob's race.\n\r");
	add_buf(buf, "available races:\n\r");

	add_buf(buf, "\n\r     ");
	for (iter = 0; race_table[iter].name != NULL; iter++) {
	    sprintf(flag, "%-19.18s", race_table[iter].name);
	    add_buf(buf, flag);
	    if (++col % 3 == 0)
		add_buf(buf, "\n\r     ");
	}

	if (col % 3 != 0)
	    add_buf(buf, "\n\r");

	return false;
    }

    while (is_space(*arg) || arg[0] == '=')
	arg++;
    return vch->race == race_lookup(arg);
}

/***************************************************************************
 *	char_cmp_sex
 ***************************************************************************/
bool char_cmp_sex(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	char flag[MSL];
	int col = 0;
	int iter;

	add_buf(buf, "search by a mob's gender.\n\r");
	add_buf(buf, "available sexs:\n\r");

	add_buf(buf, "\n\r     ");
	for (iter = 0; sex_table[iter].name != NULL; iter++) {
	    sprintf(flag, "%-19.18s", sex_table[iter].name);
	    add_buf(buf, flag);
	    if (++col % 3 == 0)
		add_buf(buf, "\n\r     ");
	}

	if (col % 3 != 0)
	    add_buf(buf, "\n\r");

	return false;
    }

    while (is_space(*arg) || arg[0] == '=')
	arg++;
    return vch->sex == sex_lookup(arg);
}


/***************************************************************************
 *	char_cmp_level
 ***************************************************************************/
bool char_cmp_level(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's level.\n\r");
    return cmp_fn_number(vch->level, arg);
}

/***************************************************************************
 *	char_cmp_hit
 ***************************************************************************/
bool char_cmp_hit(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's current hit points.\n\r");
    return cmp_fn_number(vch->hit, arg);
}


/***************************************************************************
 *	char_cmp_mana
 ***************************************************************************/
bool char_cmp_mana(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's current mana.\n\r");
    return cmp_fn_number(vch->mana, arg);
}

/***************************************************************************
 *	char_cmp_move
 ***************************************************************************/
bool char_cmp_move(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's current movement.\n\r");
    return cmp_fn_number(vch->move, arg);
}

/***************************************************************************
 *	char_cmp_max_hit
 ***************************************************************************/
bool char_cmp_max_hit(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's max hit points.\n\r");
    return cmp_fn_number(vch->max_hit, arg);
}

/***************************************************************************
 *	char_cmp_max_mana
 ***************************************************************************/
bool char_cmp_max_mana(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's max mana.\n\r");
    return cmp_fn_number(vch->max_mana, arg);
}

/***************************************************************************
 *	char_cmp_max_move
 ***************************************************************************/
bool char_cmp_max_move(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's max movement.\n\r");
    return cmp_fn_number(vch->max_move, arg);
}

/***************************************************************************
 *	char_cmp_gold
 ***************************************************************************/
bool char_cmp_gold(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's gold.\n\r");
    return cmp_fn_number((long)vch->gold, arg);
}

/***************************************************************************
 *	char_cmp_silver
 ***************************************************************************/
bool char_cmp_silver(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's silver.\n\r");
    return cmp_fn_number((long)vch->silver, arg);
}

/***************************************************************************
 *	char_cmp_off_flags
 ***************************************************************************/
bool char_cmp_offense(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a mob's offensive flags.\n\r");
	add_buf(buf, "available offense flags:\n\r");
    }
    return cmp_fn_flag((IS_NPC(vch) ? vch->off_flags : 0), arg, off_flags, buf);
}

/***************************************************************************
 *	char_cmp_form
 ***************************************************************************/
bool char_cmp_form(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a mob's form flags.\n\r");
	add_buf(buf, "available form flags:\n\r");
    }
    return cmp_fn_flag(vch->form, arg, form_flags, buf);
}

/***************************************************************************
 *	char_cmp_act
 ***************************************************************************/
bool char_cmp_act(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a mob's act flags.\n\r");
	add_buf(buf, "available act flags:\n\r");
    }
    return cmp_fn_flag((IS_NPC(vch) ? vch->act : 0), arg, act_flags, buf);
}

/***************************************************************************
 *	char_cmp_plr
 ***************************************************************************/
bool char_cmp_player(CHAR_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a character's player flags.\n\r");
	add_buf(buf, "available player flags:\n\r");
	if (IS_NPC(vch))
	    cmp_fn_flag(0, arg, plr_flags, buf);
    }
    return cmp_fn_flag((IS_NPC(vch) ? vch->act : 0), arg, plr_flags, buf);
}


static void help_ovnum_properties(CHAR_DATA *ch)
{
    BUFFER *buffer = new_buf();
    int iter;
    char buf[MIL];

    add_buf(buffer, "`#QUERY``: ovnum: searchable property list\n\r");

    for (iter = 0; objprototype_flags[iter].var[0] != '\0'; iter++) {
	snprintf(buf, MIL, "%-18.17s", objprototype_flags[iter].var);
	add_buf(buffer, buf);
	if ((iter % 2) == 1)
	    add_buf(buffer, "\n\r");
    }
    add_buf(buffer, "\n\r");
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
}

static void help_mvnum_properties(CHAR_DATA *ch)
{
    BUFFER *buffer = new_buf();
    char buf[MIL];
    int iter;

    add_buf(buffer, "`#QUERY``: mvnum: searchable property list\n\r");
    for (iter = 0; mob_idx_flags[iter].var[0] != '\0'; iter++) {
	sprintf(buf, "%-18.17s", mob_idx_flags[iter].var);
	add_buf(buffer, buf);
	if ((iter % 2) == 1)
	    add_buf(buffer, "\n\r");
    }
    add_buf(buffer, "\n\r");
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
}

static char *get_search_vnum_range(CHAR_DATA *ch, char *argument, char *arg, BUFFER *buffer,
	long *out_high_vnum, long *out_low_vnum)
{
    AREA_DATA *ad = NULL;
    char buf[MIL];

    if (is_number(arg) || (arg[0] == '?' && is_digit(arg[1])))
	ad = grok_area(ch, arg, buffer);

    if (ad == NULL) {
	*out_low_vnum = 0;
	*out_high_vnum = 1000000000; /* TODO - duh find MAX_LONG macro (thought was in stdint.h) */
    } else {
	argument = one_argument(argument, arg);
	snprintf(buf, MIL, "In area %s (%ld) [%ld - %ld]\n\r\n\r", ad->name, ad->vnum, ad->min_vnum, ad->max_vnum);
	add_buf(buffer, buf);
	*out_low_vnum = ad->min_vnum;
	*out_high_vnum = ad->max_vnum;
    }

    return argument;
}



static char *prep_find_entity_vnum(CHAR_DATA *ch, char *argument, char *arg, const char *entity,
	void (*help_entity_properties_fn)(CHAR_DATA *ch),
	void (*entity_empty_fn)(CHAR_DATA *ch, char *arg, BUFFER *buffer))
{
    if (ch == NULL || IS_NPC(ch))
	return NULL;

    if (argument[0] == '\0') {
	char buf[200];
	snprintf(buf, 200, "\n\r%s: Find what?\n\r", entity);
	send_to_char(buf, ch);
	return NULL;
    }

    argument = one_argument(argument, arg);
    if (arg[0] == '?' || !str_prefix(arg, "help")) {
	do_help(ch, (char *)entity);
	return NULL;
    }

    if (!str_prefix(arg, "list")) {
	(*help_entity_properties_fn)(ch);
	return NULL;
    } else
	if (!str_prefix(arg, "empty")) {
	    BUFFER *buffer = new_buf();
	    argument = one_argument(argument, arg);
	    (*entity_empty_fn)(ch, arg, buffer);
	    page_to_char(buf_string(buffer), ch);
	    free_buf(buffer);
	    return NULL;
	}

    return argument;
}


/***************************************************************************
 *	vnum lookups - ovnum mvnum
 ***************************************************************************/
/***************************************************************************
 *	objects
 ***************************************************************************/


/***************************************************************************
 *	do_ovnum
 *
 *	new syntax:
 *		ovnum <var> <value>
 *
 ***************************************************************************/
void do_ovnum(CHAR_DATA *ch, char *argument)
{
    char *original_argument = argument;
    char arg[MIL];

    argument = prep_find_entity_vnum(ch, argument, arg, "ovnum",
	    help_ovnum_properties,
	    ovnum_find_empty);
    if (argument != NULL) {
	BUFFER *buffer = new_buf();
	OBJ_IDX_CMP_FN *cmp_fn = NULL;
	long low_vnum, high_vnum;
	char buf[MIL];
	long iter;

	sprintf(buf, "`#QUERY``: ovnum %s\n\r\n\r", original_argument);
	add_buf(buffer, buf);

	for (iter = 0; objprototype_flags[iter].var[0] != '\0'; iter++) {
	    if (!str_prefix(arg, objprototype_flags[iter].var)) {
		cmp_fn = (OBJ_IDX_CMP_FN *)objprototype_flags[iter].fn;
		argument = one_argument(argument, arg);
		break;
	    }
	}

	if (cmp_fn == NULL)
	    cmp_fn = objprototype_cmp_name;

	argument = get_search_vnum_range(ch, argument, arg, buffer, &high_vnum, &low_vnum);

	if (arg[0] == '?' || arg[0] == '\0') {
	    clear_buf(buffer);
	    sprintf(buf, "`#SYNTAX``:\n\r       ovnum %s <value>:\n\r\n\r", arg);
	    add_buf(buffer, buf);

	    (*cmp_fn)(objectprototype_getbyvnum(OBJ_VNUM_MAP), arg, buffer);
	    page_to_char(buf_string(buffer), ch);
	} else {
	    OBJECTPROTOTYPE *current;
	    OBJECTPROTOTYPE *pending;
	    char *clr1;
	    int number = 0;
	    long count = 0;

	    pending = objectprototype_iterator_start(&objectprototype_empty_filter);
	    while ((current = pending) != NULL) {
		pending = objectprototype_iterator(current, &objectprototype_empty_filter);

		count++;

		if ((*cmp_fn)(current, arg, NULL)) {
		    number++;

		    if (number == 1) {
			sprintf(buf, "#   vnum   name\n\r");
			add_buf(buffer, buf);
			sprintf(buf, "=== ====== =======================================\n\r");

			add_buf(buffer, buf);
		    }

		    clr1 = uncolor_str(current->short_descr);
		    sprintf(buf, "%-3d %-7ld  %-38.38s\n\r", number, current->vnum, clr1);
		    free_string(clr1);

		    add_buf(buffer, buf);

		    if (number >= MAX_RETURN)
			break;
		}
	    }

	    if (number == 0)
		send_to_char("Nothing like that in heaven or earth.\n\r", ch);
	    else
		page_to_char(buf_string(buffer), ch);
	}

	free_buf(buffer);
    }
}


/***************************************************************************
 *	objprototype_cmp_name
 ***************************************************************************/
bool objprototype_cmp_name(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's name.\n\r");
    return cmp_fn_string(obj->name, arg);
}

/***************************************************************************
 *	objprototype_cmp_short
 ***************************************************************************/
bool objprototype_cmp_short(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's short description.\n\r");
    return cmp_fn_string(obj->short_descr, arg);
}

/***************************************************************************
 *	objprototype_cmp_long
 ***************************************************************************/
bool objprototype_cmp_long(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's long description.\n\r");
    return cmp_fn_string(obj->description, arg);
}

/***************************************************************************
 *	objprototype_cmp_type
 ***************************************************************************/
bool objprototype_cmp_type(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's type.\n\r");
	add_buf(buf, "available type flags:\n\r");
    }
    return cmp_fn_index((long)obj->item_type, arg, type_flags, buf);
}

/***************************************************************************
 *	objprototype_cmp_weight
 ***************************************************************************/
bool objprototype_cmp_weight(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's weight.\n\r");
    return cmp_fn_number(obj->weight, arg);
}

/***************************************************************************
 *	objprototype_cmp_cost
 ***************************************************************************/
bool objprototype_cmp_cost(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's cost.\n\r");
    return cmp_fn_number((long)obj->cost, arg);
}

/***************************************************************************
 *	objprototype_cmp_level
 ***************************************************************************/
bool objprototype_cmp_level(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by an object's level.\n\r");
    return cmp_fn_number(obj->level, arg);
}

/***************************************************************************
 *	objprototype_cmp_extra
 ***************************************************************************/
bool objprototype_cmp_extra(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's extra flags.\n\r");
	add_buf(buf, "available extra flags:\n\r");
    }
    return cmp_fn_flag((long)obj->extra_flags, arg, extra_flags, buf);
}

/***************************************************************************
 *	objprototype_cmp_wear
 ***************************************************************************/
bool objprototype_cmp_wear(OBJECTPROTOTYPE *obj, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by an object's wear flags.\n\r");
	add_buf(buf, "available wear flags:\n\r");
    }
    return cmp_fn_flag((long)obj->wear_flags, arg, wear_flags, buf);
}




/***************************************************************************
 *	mobs
 ***************************************************************************/


/***************************************************************************
 *	do_mvnum
 *
 *	new syntax:
 *		mvnum <var> <value>
 *
 ***************************************************************************/
void do_mvnum(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];
    char *original_argument = argument;

    argument = prep_find_entity_vnum(ch, argument, arg, "mvnum",
	    &help_mvnum_properties,
	    &mvnum_find_empty);
    if (argument != NULL) {
	{
	    BUFFER *buffer;
	    MOB_IDX_CMP_FN *cmp_fn = NULL;
	    char buf[MIL];
	    long low_vnum, high_vnum;
	    long iter;

	    buffer = new_buf();
	    sprintf(buf, "`#QUERY``: mvnum %s\n\r\n\r", original_argument);
	    add_buf(buffer, buf);

	    for (iter = 0; mob_idx_flags[iter].var[0] != '\0'; iter++) {
		if (!str_prefix(arg, mob_idx_flags[iter].var)) {
		    cmp_fn = (MOB_IDX_CMP_FN *)mob_idx_flags[iter].fn;
		    argument = one_argument(argument, arg);
		    break;
		}
	    }

	    if (cmp_fn == NULL)
		cmp_fn = mob_idx_cmp_name;

	    argument = get_search_vnum_range(ch, argument, arg, buffer, &high_vnum, &low_vnum);

	    if (arg[0] == '?' || arg[0] == '\0') {
		clear_buf(buffer);
		sprintf(buf, "`#SYNTAX``:\n\r       mvnum %s <value>:\n\r\n\r", arg);
		add_buf(buffer, buf);

		(*cmp_fn)(get_mob_index(MOB_VNUM_PIG), argument, buffer);
		page_to_char(buf_string(buffer), ch);
	    } else {
		MOB_INDEX_DATA *mob;
		char *clr1;
		int number = 0;
		long count = 0;

		for (iter = low_vnum; iter <= high_vnum && count < top_mob_index; iter++) {
		    if ((mob = get_mob_index(iter)) != NULL) {
			count++;

			if ((*cmp_fn)(mob, arg, NULL)) {
			    number++;

			    if (number == 1) {
				add_buf(buffer, "#   vnum   name\n\r");
				add_buf(buffer, "=== ====== =======================================\n\r");
			    }

			    clr1 = uncolor_str(mob->short_descr);
			    sprintf(buf, "%-3d %-7ld  %-38.38s\n\r",
				    number,
				    mob->vnum,
				    clr1);
			    free_string(clr1);

			    add_buf(buffer, buf);

			    if (number >= MAX_RETURN)
				break;
			}
		    }
		}

		if (number == 0)
		    send_to_char("Nothing like that in heaven or earth.\n\r", ch);
		else
		    page_to_char(buf_string(buffer), ch);
	    }

	    free_buf(buffer);
	}
    }
}

bool mob_idx_cmp_name(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's name.\n\r");
    return cmp_fn_string(vch->player_name, arg);
}

bool mob_idx_cmp_short(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's short description.\n\r");
    return cmp_fn_string(vch->short_descr, arg);
}

bool mob_idx_cmp_long(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's long description.\n\r");
    return cmp_fn_string(vch->long_descr, arg);
}

bool mob_idx_cmp_race(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	char flag[MSL];
	int col = 0;
	int iter;

	add_buf(buf, "search by a mob's race.\n\r");
	add_buf(buf, "available races:\n\r");

	add_buf(buf, "\n\r     ");
	for (iter = 0; race_table[iter].name != NULL; iter++) {
	    sprintf(flag, "%-19.18s", race_table[iter].name);
	    add_buf(buf, flag);
	    if (++col % 3 == 0)
		add_buf(buf, "\n\r     ");
	}

	if (col % 3 != 0)
	    add_buf(buf, "\n\r");

	return false;
    }

    while (is_space(*arg) || arg[0] == '=')
	arg++;
    return vch->race == race_lookup(arg);
}

/***************************************************************************
 *	mob_idx_cmp_sex
 ***************************************************************************/
bool mob_idx_cmp_sex(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	char flag[MSL];
	int col = 0;
	int iter;

	add_buf(buf, "search by a mob's gender.\n\r");
	add_buf(buf, "available sexs:\n\r");

	add_buf(buf, "\n\r     ");
	for (iter = 0; sex_table[iter].name != NULL; iter++) {
	    sprintf(flag, "%-19.18s", sex_table[iter].name);
	    add_buf(buf, flag);
	    if (++col % 3 == 0)
		add_buf(buf, "\n\r     ");
	}

	if (col % 3 != 0)
	    add_buf(buf, "\n\r");

	return false;
    }


    while (is_space(*arg) || arg[0] == '=')
	arg++;
    return vch->sex == sex_lookup(arg);
}


/***************************************************************************
 *	mob_idx_cmp_level
 ***************************************************************************/
bool mob_idx_cmp_level(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's level.\n\r");
    return cmp_fn_number(vch->level, arg);
}


/***************************************************************************
 *	mob_idx_cmp_gold
 ***************************************************************************/
bool mob_idx_cmp_wealth(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL)
	add_buf(buf, "search by a mob's wealth.\n\r");
    return cmp_fn_number((long)vch->wealth, arg);
}


/***************************************************************************
 *	mob_idx_cmp_off_flags
 ***************************************************************************/
bool mob_idx_cmp_offense(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a mob's offense flags.\n\r");
	add_buf(buf, "available offense flags:\n\r");
    }
    return cmp_fn_flag(vch->off_flags, arg, off_flags, buf);
}

/***************************************************************************
 *	mob_idx_cmp_form
 ***************************************************************************/
bool mob_idx_cmp_form(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a mob's form flags.\n\r");
	add_buf(buf, "available form flags:\n\r");
    }
    return cmp_fn_flag(vch->form, arg, form_flags, buf);
}

/***************************************************************************
 *	mob_idx_cmp_act
 ***************************************************************************/
bool mob_idx_cmp_act(MOB_INDEX_DATA *vch, char *arg, BUFFER *buf)
{
    if (buf != NULL) {
	add_buf(buf, "search by a mob's act flags.\n\r");
	add_buf(buf, "available act flags:\n\r");
    }
    return cmp_fn_flag(vch->act, arg, act_flags, buf);
}





/***************************************************************************
 *	general compare functions
 ***************************************************************************/
/***************************************************************************
 *	cmp_fn_string
 ***************************************************************************/
bool cmp_fn_string(char *name, char *arg)
{
    while (is_space(*arg) || arg[0] == '=')
	arg++;

    return is_name(arg, name);
}

/***************************************************************************
 *	cmd_fn_number
 ***************************************************************************/
bool cmp_fn_number(long val, char *arg)
{
    if (arg[0] == '>') {
	while (is_space(*arg) || arg[0] == '>')
	    arg++;

	if (is_number(arg))
	    return val > parse_int(arg);
    } else if (arg[0] == '<') {
	while (is_space(*arg) || arg[0] == '<')
	    arg++;

	if (is_number(arg))
	    return val < parse_int(arg);
    } else {
	while (is_space(*arg) || arg[0] == '=')
	    arg++;

	if (is_number(arg))
	    return val == parse_int(arg);
    }

    return false;
}


/***************************************************************************
 *	cmp_fn_flag
 ***************************************************************************/
bool cmp_fn_flag(long				bit,
	char *				arg,
	const struct flag_type *	table,
	BUFFER *			buf)
{
    int iter;

    if (buf != NULL) {
	char flag[MSL];
	int col = 0;

	add_buf(buf, "\n\r     ");
	for (iter = 0; table[iter].name != NULL; iter++) {
	    sprintf(flag, "%-19.18s", table[iter].name);
	    add_buf(buf, flag);
	    if (++col % 3 == 0)
		add_buf(buf, "\n\r     ");
	}

	if (col % 3 != 0)
	    add_buf(buf, "\n\r");
    } else {
	while (is_space(*arg) || arg[0] == '=')
	    arg++;

	if (is_number(arg)) {
	    return IS_SET(bit, parse_long(arg)) > 0;
	} else {
	    for (iter = 0; table[iter].name != NULL; iter++)
		if (!str_prefix(arg, table[iter].name))
		    return IS_SET(bit, (long)table[iter].bit) > 0;
	}
    }

    return false;
}

/***************************************************************************
 *	cmp_fn_index
 ***************************************************************************/
bool cmp_fn_index(long				bit,
	char *			arg,
	const struct flag_type *	table,
	BUFFER *			buf)
{
    int iter;

    if (buf != NULL) {
	char flag[MSL];
	int col = 0;

	add_buf(buf, "\n\r     ");
	for (iter = 0; table[iter].name != NULL; iter++) {
	    sprintf(flag, "%-19.18s", table[iter].name);
	    add_buf(buf, flag);
	    if (++col % 3 == 0)
		add_buf(buf, "\n\r     ");
	}

	if (col % 3 != 0)
	    add_buf(buf, "\n\r");
    } else {
	while (is_space(*arg) || arg[0] == '=')
	    arg++;

	if (is_number(arg)) {
	    return IS_SET(bit, parse_long(arg)) > 0;
	} else {
	    for (iter = 0; table[iter].name != NULL; iter++)
		if (!str_prefix(arg, table[iter].name))
		    return bit == table[iter].bit;
	}
    }

    return false;
}
