/*
 * Temporary code to help find unused room flags
 *			added by Monrick 2/2008
 */

#include <string.h>
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"
#include "olc.h"


extern char *flag_string(const struct flag_type *flag_table, long bits);
extern int flag_value(const struct flag_type *flag_table, char *argument);


void do_findflags(CHAR_DATA * ch, char *argument);
void do_worldflag(CHAR_DATA * ch, char *argument);
void do_spellflags(CHAR_DATA * ch, char *argument);

DECLARE_DO_FUN(do_findflags);
DECLARE_DO_FUN(do_worldflag);
DECLARE_DO_FUN(do_spellflags);

void do_findflags(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *room;
	BUFFER *buf;
	int flag_idx;
	long flag;
	int flag_counts[50];
	bool found;
	int hash_idx;
	char *unclr;
	int col = 0;
	char rFlag[MIL];

	/* Initialize values */
	argument = one_argument(argument, rFlag);
	found = FALSE;
	buf = new_buf();

	if (rFlag[0] != '\0') {
		flag = NO_FLAG;
		for (flag_idx = 0; room_flags[flag_idx].name != NULL; flag_idx++) {
			if (LOWER(rFlag[0]) == LOWER(room_flags[flag_idx].name[0])
			    && !str_prefix(rFlag, room_flags[flag_idx].name))
				flag = room_flags[flag_idx].bit;
		}

		if (flag == NO_FLAG) {
			send_to_char("No such flag.  Type '`^? room``' to see a list of available flags.\n\r", ch);
			return;
		}

		for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
			for (room = room_index_hash[hash_idx]; room != NULL; room = room->next) {
				if (IS_SET(room->room_flags, flag)) {
					found = TRUE;

					unclr = uncolor_str(capitalize(room->name));

					printf_buf(buf, "[`O%5d``] %17.16s", room->vnum, unclr);
					free_string(unclr);

					if (++col % 3 == 0)
						add_buf(buf, "\n\r");
				}
			}
		}

		if (!found) {
			printf_buf(buf, "No such rooms found.\n\r");
			send_to_char(buf_string(buf), ch);
			free_buf(buf);
			return;
		}

		if (col % 3 != 0)
			add_buf(buf, "\n\r");

		page_to_char(buf_string(buf), ch);
		free_buf(buf);
		return;
	} else {
		/* Set up flag_counts table */
		for (flag_idx = 0; room_flags[flag_idx].name != NULL; flag_idx++)
			flag_counts[flag_idx] = 0;

		for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) { /* All rooms*/
			for (room = room_index_hash[hash_idx]; room != NULL; room = room->next) {
				for (flag_idx = 0; room_flags[flag_idx].name != NULL; flag_idx++) { /* All room_flag types */
					if (((flag = flag_value(room_flags, room_flags[flag_idx].name)) != NO_FLAG)
					    && IS_SET(room->room_flags, flag))
						flag_counts[flag_idx]++;
				}
			}
		}

		for (flag_idx = 0; room_flags[flag_idx].name != NULL; flag_idx++)
			printf_buf(buf, "%30s: %-10d\n\r", room_flags[flag_idx].name, flag_counts[flag_idx]);

		page_to_char(buf_string(buf), ch);
		free_buf(buf);
		return;
	}
}

void do_worldflag(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *room;
	int nCount = 0;
	int fCount = 0;
	int flag_idx;
	long flag;
	bool found;
	int hash_idx;
	char rFlag[MIL];
	char rType[MIL];

	if (str_prefix(capitalize(ch->name), "Monrick")) {
		send_to_char("No way, jose.\n\r", ch);
		return;
	}

	/* Initialize values*/
	argument = one_argument(argument, rFlag);
	argument = one_argument(argument, rType);
	found = FALSE;
	flag = NO_FLAG;

	for (flag_idx = 0; room_flags[flag_idx].name != NULL; flag_idx++) {
		if (LOWER(rFlag[0]) == LOWER(room_flags[flag_idx].name[0])
		    && !str_prefix(rFlag, room_flags[flag_idx].name))
			flag = room_flags[flag_idx].bit;
	}

	if (flag == NO_FLAG) {
		send_to_char("No such flag.  Type '`^? room``' to see a list of available flags.\n\r", ch);
		return;
	}

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (room = room_index_hash[hash_idx]; room != NULL; room = room->next) {
			if (IS_SET(room->room_flags, flag)) {
				found = TRUE;

				if (!strcmp(rType, "on")) {
					SET_BIT(room->room_flags, flag);
					nCount++;
				} else if (!strcmp(rType, "off")) {
					REMOVE_BIT(room->room_flags, flag);
					fCount++;
				} else {
					TOGGLE_BIT(room->room_flags, flag);
					if (IS_SET(room->room_flags, flag))
						nCount++;
					else
						fCount++;
				}
			}
		}
	}

	if (!found) {
		send_to_char("No rooms!\n\r", ch);
		return;
	}

	printf_to_char(ch, "%d rooms turned `@ON``.\n\r%d rooms turned `!OFF``.\n\r", nCount, fCount);
	return;
}

void do_spellflags(CHAR_DATA *ch, char *argument)
{
	BUFFER *buf;
	SKILL *skill_idx;
	bool found;
	int flag;
	int col;

	/* Initialize values*/
	buf = new_buf();
	found = FALSE;
	col = 0;

	if ((flag = flag_value(skill_flags, argument)) == NO_FLAG) {
		send_to_char("Syntax: spellflags [flag]\n\rAvailable flags:\n\r", ch);
		for (flag = 0; skill_flags[flag].name != NULL; flag++) {
			printf_to_char(ch, "%-20s", skill_flags[flag].name);
			if (++col % 4 == 0)
				send_to_char("\n\r", ch);
		}
		if (col % 4 != 0)
			send_to_char("\n\r", ch);
		return;
	}


	printf_buf(buf, "Skills flagged: `^%s``\n\r\n\r", flag_string(skill_flags, flag));

	for (skill_idx = skill_list; skill_idx != NULL; skill_idx = skill_idx->next) {
		if (IS_SET(skill_idx->flags, flag)) {
			found = TRUE;
			printf_buf(buf, "%-20s", skill_idx->name);

			if (++col % 4 == 0)
				add_buf(buf, "\n\r");
		}
	}

	if (found) {
		if (col % 4 != 0)
			add_buf(buf, "\n\r");
	} else {
		add_buf(buf, "None.\n\r");
	}


	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return;
}
