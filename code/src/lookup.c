#include "merc.h"
#include "tables.h"
#include "sysinternals.h"

long flag_lookup(const char *name, const struct flag_type *flag_table)
{
	int flag;

	for (flag = 0; flag_table[flag].name != NULL; flag++) {
		if (LOWER(name[0]) == LOWER(flag_table[flag].name[0])
		    && !str_prefix(name, flag_table[flag].name)
		    && flag_table[flag].settable)
			return flag_table[flag].bit;
	}

	return NO_FLAG;
}

int position_lookup(const char *name)
{
	int pos;

	for (pos = 0; position_table[pos].name != NULL; pos++) {
		if (LOWER(name[0]) == LOWER(position_table[pos].name[0])
		    && !str_prefix(name, position_table[pos].name))
			return pos;
	}

	return -1;
}

int sex_lookup(const char *name)
{
	int sex;

	for (sex = 0; sex_table[sex].name != NULL; sex++) {
		if (LOWER(name[0]) == LOWER(sex_table[sex].name[0])
		    && !str_prefix(name, sex_table[sex].name))
			return sex;
	}

	return -1;
}

int size_lookup(const char *name)
{
	int size;

	for (size = 0; size_table[size].name != NULL; size++) {
		if (LOWER(name[0]) == LOWER(size_table[size].name[0])
		    && !str_prefix(name, size_table[size].name))
			return size;
	}

	return -1;
}

/* returns race number */
int race_lookup(const char *name)
{
	int race;

	for (race = 0; race_table[race].name != NULL; race++) {
		if (LOWER(name[0]) == LOWER(race_table[race].name[0])
		    && !str_prefix(name, race_table[race].name))
			return race;
	}

	return 0;
}

int item_lookup(const char *name)
{
	int type;

	for (type = 0; item_table[type].name != NULL; type++) {
		if (LOWER(name[0]) == LOWER(item_table[type].name[0])
		    && !str_prefix(name, item_table[type].name))
			return item_table[type].type;
	}

	return -1;
}

int liq_lookup(const char *name)
{
	int liq;

	for (liq = 0; liq_table[liq].liq_name != NULL; liq++) {
		if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
		    && !str_prefix(name, liq_table[liq].liq_name))
			return liq;
	}

	return -1;
}

HELP_DATA *help_lookup(char *keyword)
{
	HELP_DATA *help;

	for (help = help_first; help != NULL; help = help->next) {
		if (!str_infix(keyword, help->keyword)
		    || (keyword[0] == help->keyword[0] && !str_cmp(keyword, help->keyword)))
			return help;
	}

	return NULL;
}

HELP_AREA *had_lookup(char *arg)
{
	extern HELP_AREA *had_list;
	HELP_AREA *temp;

	for (temp = had_list; temp; temp = temp->next)
		if (!str_cmp(arg, temp->filename))
			return temp;

	return NULL;
}
