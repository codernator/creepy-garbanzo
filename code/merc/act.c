#include "merc.h"

/** imports */
extern void mp_act_trigger(const char *argument, struct char_data * mob, struct char_data * ch, const void *arg1, const void *arg2, int type);


void act(const char *format, struct char_data *ch, const void *arg1, const void *arg2, int type)
{
    /* to be compatible with older code */
    act_new(format, ch, arg1, arg2, type, POS_RESTING, true);
}

void act_new(const char *format, struct char_data *ch, const void *arg1, const void *arg2, int type, int min_pos, bool mob_trigger)
{
    static char *const he_she[] = { "it", "he", "she" };
    static char *const him_her[] = { "it", "him", "her" };
    static char *const his_her[] = { "its", "his", "her" };

    struct char_data *to;
    struct char_data *vch = (struct char_data *)arg2;
    struct gameobject *obj1 = (struct gameobject *)arg1;
    struct gameobject *obj2 = (struct gameobject *)arg2;
    char buf[MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    const char *str;
    char *i = NULL;
    char *point;

    /*
     * Discard null and zero-length messages.
     */
    if (format == NULL || format[0] == '\0')
	return;


    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
	return;

    to = ch->in_room->people;
    if (type == TO_VICT) {
	if (vch == NULL) {
	    log_bug("Act: null vch with TO_VICT.");
	    return;
	}

	if (vch->in_room == NULL)
	    return;

	to = vch->in_room->people;
    }

    for (; to != NULL; to = to->next_in_room) {
	if (to->position < min_pos)
	    continue;

	if (to->desc == NULL
		&& (!IS_NPC(to) || !HAS_TRIGGER(to, TRIG_ACT)))
	    continue;

	if ((type == TO_CHAR) && to != ch)
	    continue;

	if (type == TO_VICT && (to != vch || to == ch))
	    continue;

	if (type == TO_ROOM && to == ch)
	    continue;

	if (type == TO_NOTVICT && (to == ch || to == vch))
	    continue;

	point = buf;
	str = format;
	while (*str != '\0') {
	    if (*str != '$') {
		*point++ = *str++;
		continue;
	    }
	    ++str;

	    if (arg2 == NULL && *str >= 'A' && *str <= 'Z') {
		log_bug("Act: missing arg2 for code %d.", (int)*str);
		i = " <@@@> ";
	    } else {
		switch (*str) {
		    default:
			log_bug("Act: bad code %d.", (int)*str);
			i = " <@@@> ";
			break;
			/* Thx alex for 't' idea */
		    case 't':
			if (arg1) i = (char *)arg1;
			else log_bug("Act: bad code $t for 'arg1'");
			break;
		    case 'T':
			if (arg2) i = (char *)arg2;
			else log_bug("Act: bad code $T for 'arg2'");
			break;
		    case 'n':
			if (ch && to) i = PERS(ch, to);
			else log_bug("Act: bad code $n for 'ch' or 'to'");
			break;
		    case 'N':
			if (vch && to) i = PERS(vch, to);
			else log_bug("Act: bad code $N for 'vch' or 'to'");
			break;
		    case 'e':
			if (ch) i = he_she[URANGE(0, ch->sex, 2)];
			else log_bug("Act: bad code $e for 'ch'");
			break;
		    case 'E':
			if (vch) i = he_she[URANGE(0, vch->sex, 2)];
			else log_bug("Act: bad code $E for 'vch'");
			break;
		    case 'm':
			if (ch) i = him_her[URANGE(0, ch->sex, 2)];
			else log_bug("Act: bad code $m for 'ch'");
			break;
		    case 'M':
			if (vch) i = him_her[URANGE(0, vch->sex, 2)];
			else log_bug("Act: bad code $M for 'vch'");
			break;
		    case 's':
			if (ch) i = his_her[URANGE(0, ch->sex, 2)];
			else log_bug("Act: bad code $s for 'ch'");
			break;
		    case 'S':
			if (vch) i = his_her[URANGE(0, vch->sex, 2)];
			else log_bug("Act: bad code $S for 'vch'");
			break;
		    case 'p':
			i = can_see_obj(to, obj1) ? OBJECT_SHORT(obj1) : "something";
			break;
		    case 'P':
			i = can_see_obj(to, obj2) ? OBJECT_SHORT(obj2) : "something";
			break;

		    case 'd':
			if (arg2 == NULL || ((char *)arg2)[0] == '\0') {
			    i = "door";
			} else {
			    one_argument((char *)arg2, fname);
			    i = fname;
			}
			break;
		}
	    }

	    ++str;
	    while ((*point = *i) != '\0')
		++point, ++i;
	}

	*point++ = '\n';
	*point++ = '\r';
	*point = '\0';
	buf[0] = UPPER(buf[0]);

	if (to->desc) {
	    send_to_char(buf, to);
	} else {
	    if (mob_trigger)
		mp_act_trigger(buf, to, ch, arg1, arg2, TRIG_ACT);
	}
    }
    return;
}

