#include "merc.h"
#include "tables.h"
#include "magic.h"
#include "lookup.h"
#include "interp.h"
#include "recycle.h"


#include <stdio.h>


void do_raceinfo(CHAR_DATA *ch, const char *argument)
{
	BUFFER *buf;
	int race_idx;
	int skill_idx;
	int race = 0;
	bool first;
	bool all;

	all = (argument[0] == '\0' || !str_prefix(argument, "all"));
	if (!all) {
		race = race_lookup(argument);
		if (race <= 0) {
			send_to_char("That is an invalid race.\n\r", ch);
			return;
		}
	}
	buf = new_buf();
	for (race_idx = 1; pc_race_table[race_idx].name[0] != '\0'; race_idx++) {
		if (all || race == race_idx) {
			add_buf(buf, "\n\r`2-----------------------------------------------------------------------``\n\r");
			printf_buf(buf, "`#Race`3:``             %s\n\r", capitalize(pc_race_table[race_idx].name));

			/* display skills */
			first = false;
			for (skill_idx = 0; skill_idx < 5; skill_idx++) {
				if (pc_race_table[race_idx].skills[skill_idx] == NULL)
					break;

				if (!first) {
					add_buf(buf, "`#Default skills`3:``   ");
					first = true;
				} else if (skill_idx % 3 == 0) {
					add_buf(buf, "\n\r                  ");

/*printf_buf(buf, "                 %s\n\r", capitalize(pc_race_table[race_idx].skills[skill_idx]));*/
				}
				printf_buf(buf, "%-18.18s", capitalize(pc_race_table[race_idx].skills[skill_idx]));
			}

			if (skill_idx % 3 == 0)
				add_buf(buf, "\n\r");

			/* display the stat header */
			add_buf(buf, "\n\r\n\r`@                  STR     INT     WIS     DEX     CON     LUCK\n\r");
			add_buf(buf, "`2-----------------------------------------------------------------------``\n\r");
			printf_buf(buf, "`#Starting stats`3:``   %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``   %3d\n\r",
				   pc_race_table[race_idx].stats[STAT_STR],
				   pc_race_table[race_idx].stats[STAT_INT],
				   pc_race_table[race_idx].stats[STAT_WIS],
				   pc_race_table[race_idx].stats[STAT_DEX],
				   pc_race_table[race_idx].stats[STAT_CON],
				   pc_race_table[race_idx].stats[STAT_LUCK]);

			printf_buf(buf, "`#Trainable stats`3:``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``   %3d\n\r",
				   pc_race_table[race_idx].max_train_stats[STAT_STR],
				   pc_race_table[race_idx].max_train_stats[STAT_INT],
				   pc_race_table[race_idx].max_train_stats[STAT_WIS],
				   pc_race_table[race_idx].max_train_stats[STAT_DEX],
				   pc_race_table[race_idx].max_train_stats[STAT_CON],
				   pc_race_table[race_idx].max_train_stats[STAT_LUCK]);

			printf_buf(buf, "`#Maximum stats`3:``    %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``  %3d  `2|``   %3d\n\r",
				   (pc_race_table[race_idx].max_stats[STAT_STR] > 0) ? pc_race_table[race_idx].max_stats[STAT_STR] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_INT] > 0) ? pc_race_table[race_idx].max_stats[STAT_INT] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_WIS] > 0) ? pc_race_table[race_idx].max_stats[STAT_WIS] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_DEX] > 0) ? pc_race_table[race_idx].max_stats[STAT_DEX] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_CON] > 0) ? pc_race_table[race_idx].max_stats[STAT_CON] : 0,
				   (pc_race_table[race_idx].max_stats[STAT_LUCK] > 0) ? pc_race_table[race_idx].max_stats[STAT_LUCK] : 0);

			add_buf(buf, "`2-----------------------------------------------------------------------``\n\r");
			add_buf(buf, "\n\r");
		}
	}

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
}


