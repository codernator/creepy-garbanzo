#include <sys/resource.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "libfile.h"
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"
#include "olc.h"
#include "skills.h"
#include "channels.h"



extern long flag_lookup(const char *word, const struct flag_type *flag_table);
extern int _filbuf(FILE *);
extern void init_mm(void);
extern unsigned int fread_uint(FILE *fp);
extern long fread_long(FILE *fp);
extern bool is_space(const char test);



extern OBJ_DATA *obj_free;
extern CHAR_DATA *char_free;
extern PC_DATA *pcdata_free;
extern AFFECT_DATA *affect_free;

CHAR_DATA *char_list;
NOTE_DATA *note_list;

HELP_DATA *help_first;
static HELP_DATA *help_last;
HELP_AREA *had_list;

MPROG_CODE *mprog_list;

SHOP_DATA *shop_first;
SHOP_DATA *shop_last;

NOTE_DATA *note_free;
KILL_DATA kill_table[MAX_LEVEL];


char *help_greeting;
static char *ahelp_greeting;
static char *ahelp_greeting2;

int reboot_tick_counter = -1;
int copyover_tick_counter = -1;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;

AUCTION_DATA *auction;



/***************************************************************************
*	local vars
***************************************************************************/
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
static char *string_hash[MAX_KEY_HASH];

AREA_DATA *area_first;
AREA_DATA *area_last;
static AREA_DATA *current_area;

static char *string_space;
static char *top_string;
char str_empty[1];

int top_affect;
int top_area;
int top_ed;
long top_exit;
int top_help;
long top_mob_index;
long top_obj_index;
int top_reset;
long top_room;
int top_shop;

long top_vnum_room;
long top_vnum_mob;
long top_vnum_obj;
long top_mprog_index;

int mobile_count = 0;
int newmobs = 0;
int newobjs = 0;


static void bug_long(const char *str, long param);
static void bug(const char *str, int param);

/***************************************************************************
*	memory management
***************************************************************************/
#define MAX_STRING                      4000000
#define MAX_PERM_BLOCK          131072
#define MAX_MEM_LIST            11

static void *rgFreeList[MAX_MEM_LIST];
static const unsigned int rgSizeList[MAX_MEM_LIST] =
{
	16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768
};

static int nAllocString;
static int sAllocString;
static int nAllocPerm;
static int sAllocPerm;


bool db_loading;
static FILE *fp_area;
static char area_file[MIL];


/***************************************************************************
*	local functions used in boot process
***************************************************************************/
static void load_area(FILE * fp);
static void load_helps(FILE * fp, char *fname);
static void load_mobiles(FILE * fp);
static void load_objects(FILE * fp);
static void load_resets(FILE * fp);
static void load_rooms(FILE * fp);
static void load_shops(FILE * fp);
void load_threads(void);
extern void load_bans(void);
static void load_mobprogs(FILE * fp);
static void load_mobprogs_new(FILE * fp);
void resolve_global_skills(void);

static void fix_exits(void);
static void fix_mobprogs(void);
static void reset_areas(void);
void reset_area(AREA_DATA * area);


/* RT max open files fix */
static void maxfilelimit()
{
	struct rlimit r;

	getrlimit(RLIMIT_NOFILE, &r);
	r.rlim_cur = r.rlim_max;
	setrlimit(RLIMIT_NOFILE, &r);
}




/***************************************************************************
*	init_time
*
*	initialize the global time structures
***************************************************************************/
static void init_time()
{
	long lhour;
	long lday;
	long lmonth;

	lhour = (long)((globalSystemState.current_time - 650336715l) / (PULSE_TICK / PULSE_PER_SECOND));
	time_info.hour = (int)(lhour % 24l);
	lday = (int)(lhour / 24);
	time_info.day = (int)(lday % 35l);
	lmonth = lday / 35l;
	time_info.month = (int)(lmonth % 17l);
	time_info.year = (int)(lmonth / 17l);

	if (time_info.hour < 5)
		weather_info.sunlight = SUN_DARK;
	else if (time_info.hour < 6)
		weather_info.sunlight = SUN_RISE;
	else if (time_info.hour < 19)
		weather_info.sunlight = SUN_LIGHT;
	else if (time_info.hour < 20)
		weather_info.sunlight = SUN_SET;
	else
		weather_info.sunlight = SUN_DARK;

	weather_info.change = 0;
	weather_info.mmhg = 960;

	if (time_info.month >= 7 && time_info.month <= 12)
		weather_info.mmhg += number_range(1, 50);
	else
		weather_info.mmhg += number_range(1, 80);

	if (weather_info.mmhg <= 980)
		weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.mmhg <= 1000)
		weather_info.sky = SKY_RAINING;
	else if (weather_info.mmhg <= 1020)
		weather_info.sky = SKY_CLOUDY;
	else
		weather_info.sky = SKY_CLOUDLESS;
}


/** load the area information */
static void init_areas()
{
	FILE *fpList;
	char *buf;

    log_string("Opening area file.");
	if ((fpList = fopen(AREA_LIST, "r")) == NULL) {
		perror(AREA_LIST);
		raise(SIGABRT);
        return;
	}


	for (;;) {
		buf = fread_word(fpList);
		if (buf[0] == '$') {
            /** End of Area List. */
			break;
        }
		snprintf(area_file, MIL, "%s%s", AREA_FOLDER, buf);

        if ((fp_area = fopen(area_file, "r")) == NULL) {
            perror(area_file);
            raise(SIGABRT);
            return;
        }

		current_area = NULL;

		for (;; ) {
			char *word;
            char token;

            token = fread_letter(fp_area);
			if (token != '#') {
				log_bug("Boot_db: Found %c instead of expected header token # in area_file %s.", token, area_file);
				raise(SIGABRT);
                return;
			}

			word = fread_word(fp_area);

			if (word[0] == '$') {
                /** End of Area File definition. */
				break;
			} else if (!str_cmp(word, "AREADATA")) {
				load_area(fp_area);
			} else if (!str_cmp(word, "HELPS")) {
				load_helps(fp_area, area_file);
			} else if (!str_cmp(word, "MOBILES")) {
				load_mobiles(fp_area);
			} else if (!str_cmp(word, "MOBPROGS")) {
				load_mobprogs(fp_area);
			} else if (!str_cmp(word, "PROGRAMS")) {
				load_mobprogs_new(fp_area);
			} else if (!str_cmp(word, "OBJECTS")) {
				load_objects(fp_area);
			} else if (!str_cmp(word, "RESETS")) {
				load_resets(fp_area);
			} else if (!str_cmp(word, "ROOMS")) {
				load_rooms(fp_area);
			} else if (!str_cmp(word, "SHOPS")) {
				load_shops(fp_area);
			} else {
				bug("Boot_db: bad section name.", 0);
				raise(SIGABRT);
                return;
			}
		}

		if (fp_area != stdin)
			fclose(fp_area);

		fp_area = NULL;
	}

	fclose(fpList);
}



/***************************************************************************
*	boot_db
*
*	create the database
***************************************************************************/
void boot_db()
{
	/* open file fix */
	maxfilelimit();

	/* init string space */
	if ((string_space = calloc(1, MAX_STRING)) == NULL) {
		bug("Boot_db: can't alloc %d string space.", MAX_STRING);
		raise(SIGABRT);
        return;
	}

	top_string = string_space;
	db_loading = true;

/* init random number generator */
	log_string("Initializing environment..");
	init_mm();
	init_time();

	auction = (AUCTION_DATA *)malloc(sizeof(AUCTION_DATA));  /* DOH!!! */
	if (auction == NULL) {
		bug("Cannot allocate memory for the AUCTION_DATA structure - could note allocate %d bytes", (int)sizeof(AUCTION_DATA));
		raise(SIGABRT);
        return;
	}
	auction->item = NULL;   /* nothing is being sold */

	/* load skills */
	log_string("Loading Skills..");
	load_skills();
	load_groups();
	resolve_global_skills();

	log_string("Loading Areas..");
	init_areas();

	/*
	 * Fix up exits.
	 * Declare db booting over.
	 * Reset all areas once.
	 * Load up the songs, notes and ban files.
	 */

	log_string("Fixing exits..");
	fix_exits();
	fix_mobprogs();

	db_loading = false;

	log_string("Reseting Areas..");
	reset_areas();
	area_update();
	log_string("Loading Message Threads..");
	load_threads();
	log_string("Loading Disabled List..");
	load_disabled();
	log_string("Loading Bans..");
	load_bans();
	log_string("Loading Songs..");

	log_string("Assigning Skill Helpts..");
	assign_skill_helps();

	log_string("BootDB: Done..");

	if (!help_greeting) {            /* Spacey */
		bug("boot_db: No help_greeting read.", 0);
		help_greeting = "By what name do you wish to be known ? ";
	}

	return;
}




/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the new_load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)   \
	if (!str_cmp(word, literal))     \
	{                                \
		field = value;               \
		break;                       \
	}

#define SKEY(word, string, field)    \
	if (!str_cmp(word, string))      \
	{                                \
		free_string(field);          \
		field = fread_string(fp);    \
		break;                       \
	}



/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * End
 */
void load_area(FILE *fp)
{
	AREA_DATA *area;
	char *word;

	area = new_area();
	area->age = 15;
	area->nplayer = 0;
	area->file_name = str_dup(area_file);
	area->vnum = top_area;
	area->name = str_dup("New Area");
	area->builders = str_dup("");
	area->security = 9;                             /* 9 -- Hugin */
	area->min_vnum = 0;
	area->max_vnum = 0;
	area->area_flags = 0;
	area->complete = true;
	area->ulevel = 0;
	area->llevel = 0;
	area->description = str_dup("");

	for (;; ) {
		word = feof(fp) ? "End" : fread_word(fp);

		switch (UPPER(word[0])) {
		case 'B':
			SKEY(word, "Builders", area->builders);
			break;
		case 'C':
			KEY("Complete", area->complete, (bool)fread_number(fp));
			SKEY(word, "Credits", area->credits);
			break;
		case 'D':
			SKEY(word, "Description", area->description);
			break;
		case 'E':
			if (!str_cmp(word, "End")) {
				if (area_first == NULL)
					area_first = area;
				if (area_last != NULL)
					area_last->next = area;
				area_last = area;
				area->next = NULL;
				current_area = area;
				top_area++;

				return;
			}
			break;
		case 'L':
			KEY("Llevel", area->llevel, (int)fread_number(fp));
			break;
		case 'N':
			SKEY(word, "Name", area->name);
			break;
		case 'S':
			KEY("Security", area->security, fread_number(fp));
			break;
		case 'U':
			KEY("Ulevel", area->ulevel, (int)fread_number(fp));
			break;
		case 'V':
			if (!str_cmp(word, "VNUMs")) {
				area->min_vnum = fread_number(fp);
				area->max_vnum = fread_number(fp);
			}
			break;
		}
	}
}

/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum(long vnum)
{
	if (area_last->min_vnum == 0 || area_last->max_vnum == 0)
		area_last->min_vnum = area_last->max_vnum = vnum;

	if (vnum != URANGE(area_last->min_vnum, vnum, area_last->max_vnum)) {
		if (vnum < area_last->min_vnum)
			area_last->min_vnum = vnum;
		else
			area_last->max_vnum = vnum;
	}

	return;
}

/*
 * Snarf a help section.
 */
void load_helps(FILE *fp, char *fname)
{
	HELP_DATA *pHelp;
	int level;
	char *keyword;
	char *text;

	for (;; ) {
		HELP_AREA *had;

		level = fread_number(fp);
		keyword = fread_string(fp);

		if (keyword[0] == '$')
			break;

		if ((pHelp = help_lookup(keyword)) != NULL
		    && !str_cmp(pHelp->keyword, keyword)) {
			text = fread_string(fp);
			free_string(keyword);
			free_string(text);
			continue;
		}

		if (!had_list) {
			had = new_had();
			had->filename = str_dup(fname);
			had->area = current_area;

			if (current_area)
				current_area->helps = had;

			had_list = had;
		} else {
			if (str_cmp(fname, had_list->filename)) {
				had = new_had();
				had->filename = str_dup(fname);
				had->area = current_area;

				if (current_area)
					current_area->helps = had;

				had->next = had_list;
				had_list = had;
			} else {
				had = had_list;
			}
		}

		pHelp = new_help();
		pHelp->level = (int)level;
		pHelp->keyword = keyword;

		pHelp->text = fread_string(fp);

		if (!str_cmp(pHelp->keyword, "greeting"))
			help_greeting = pHelp->text;

		if (!str_cmp(pHelp->keyword, "agreeting"))
			ahelp_greeting = pHelp->text;

		if (!str_cmp(pHelp->keyword, "agreeting2"))
			ahelp_greeting2 = pHelp->text;

		if (help_first == NULL)
			help_first = pHelp;

		if (help_last != NULL)
			help_last->next = pHelp;

		help_last = pHelp;
		pHelp->next = NULL;

		if (!had->first)
			had->first = pHelp;

		if (!had->last)
			had->last = pHelp;

		had->last->next_area = pHelp;
		had->last = pHelp;
		pHelp->next_area = NULL;
		top_help++;
	}

	return;
}




/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
static void new_reset(ROOM_INDEX_DATA *pR, RESET_DATA *reset)
{
	RESET_DATA *pr;

	if (!pR)
		return;

	pr = pR->reset_last;

	if (!pr) {
		pR->reset_first = reset;
		pR->reset_last = reset;
	} else {
		pR->reset_last->next = reset;
		pR->reset_last = reset;
		pR->reset_last->next = NULL;
	}

	return;
}



/*
 * Snarf a reset section.
 */
void load_resets(FILE *fp)
{
	RESET_DATA *reset;
	EXIT_DATA *pexit;
	ROOM_INDEX_DATA *room_idx;
	long rVnum = -1;

	if (!area_last) {
		bug("Load_resets: no #AREA seen yet.", 0);
		raise(SIGABRT);
        return;
	}

	for (;; ) {
/*
 *      OLC
 *              ROOM_INDEX_DATA *	room_idx;
 *              EXIT_DATA *			pexit;
 *              OBJ_INDEX_DATA *	temp_index;
 */
		char letter;

		if ((letter = fread_letter(fp)) == 'S')
			break;

		if (letter == '*') {
			fread_to_eol(fp);
			continue;
		}

		reset = new_reset_data();
		reset->command = letter;

		/* if_flag */ fread_number(fp);

		reset->arg1 = fread_number(fp);
		reset->arg2 = fread_number(fp);
		reset->arg3 = (letter == 'G' || letter == 'R') ? 0 : fread_number(fp);
		reset->arg4 = (letter == 'P' || letter == 'M') ? fread_number(fp) : 0;
		fread_to_eol(fp);

/*
 * Validate parameters.
 * We're calling the index functions for the side effect.
 */
		switch (reset->command) {
		case 'M':
		case 'O':
			rVnum = reset->arg3;
			break;

		case 'P':
		case 'G':
		case 'E':
			break;

		case 'D':
			room_idx = get_room_index((rVnum = reset->arg1));
			if (reset->arg2 < 0
			    || reset->arg2 >= MAX_DIR
			    || !room_idx
			    || !(pexit = room_idx->exit[reset->arg2])
			    || !IS_SET(pexit->rs_flags, EX_ISDOOR)) {
				log_bug("Load_resets: 'D': exit %d, room %d not door.", reset->arg2, reset->arg1);
				raise(SIGABRT);
                return;
			}

			switch (reset->arg3) {
			default: bug_long("Load_resets: 'D': bad 'locks': %d.", reset->arg3); break;
			case 0: break;
			case 1: SET_BIT(pexit->rs_flags, EX_CLOSED);
				SET_BIT(pexit->exit_info, EX_CLOSED); break;
			case 2: SET_BIT(pexit->rs_flags, EX_CLOSED | EX_LOCKED);
				SET_BIT(pexit->exit_info, EX_CLOSED | EX_LOCKED); break;
			}
			break;

		case 'R':
			rVnum = reset->arg1;
			break;
		}

		if (rVnum == -1) {
			log_bug("load_resets : rVnum == -1");
			raise(SIGABRT);
            return;
		}

		if (reset->command != 'D')
			new_reset(get_room_index(rVnum), reset);
		else
			free_reset_data(reset);
	}

	return;
}

/*
 * Snarf a room section.
 */
void load_rooms(FILE *fp)
{
	ROOM_INDEX_DATA *room_idx;

	if (area_last == NULL) {
		bug("Load_rooms: no #AREA seen yet.", 0);
		raise(SIGABRT);
        return;
	}

	for (;; ) {
		long vnum;
		char letter;
		int door;
		int hash_idx;

		letter = fread_letter(fp);
		if (letter != '#') {
			bug("Load_rooms: # not found.", 0);
			raise(SIGABRT);
            return;
		}

		vnum = (long)fread_number(fp);
		if (vnum == 0)
			break;

		db_loading = false;
		if (get_room_index(vnum) != NULL) {
			bug_long("Load_rooms: vnum %d duplicated.", vnum);
			raise(SIGABRT);
            return;
		}
		db_loading = true;

		room_idx = alloc_perm((unsigned int)sizeof(*room_idx));
		room_idx->owner = str_dup("");
		room_idx->people = NULL;
		room_idx->contents = NULL;
		room_idx->extra_descr = NULL;
		room_idx->area = area_last;
		room_idx->vnum = vnum;
		room_idx->name = fread_string(fp);
		room_idx->description = fread_string(fp);
		/* Area number */ fread_number(fp);
		room_idx->room_flags = fread_flag(fp);

		/* horrible hack */
		if (3000 <= vnum && vnum < 3400)
			SET_BIT(room_idx->room_flags, ROOM_LAW);

		room_idx->sector_type = fread_number(fp);
		room_idx->light = 0;
		for (door = 0; door <= 5; door++)
			room_idx->exit[door] = NULL;

		/* defaults */
		room_idx->heal_rate = 100;
		room_idx->mana_rate = 100;

		for (;; ) {
			letter = fread_letter(fp);

			if (letter == 'S')
				break;

			if (letter == 'H') {     /* healing room */
				room_idx->heal_rate = fread_number(fp);
			} else if (letter == 'M') { /* mana room */
				room_idx->mana_rate = fread_number(fp);
			} else if (letter == 'D') {
				EXIT_DATA *pexit;
				int locks;

				door = fread_number(fp);
				if (door < 0 || door > 5) {
					bug_long("Fread_rooms: vnum %d has bad door number.", vnum);
					raise(SIGABRT);
                    return;
				}

				pexit = alloc_perm((unsigned int)sizeof(*pexit));
				pexit->description = fread_string(fp);
				pexit->keyword = fread_string(fp);
				pexit->exit_info = 0;
				pexit->rs_flags = 0;
				locks = fread_number(fp);
				pexit->key = fread_number(fp);
				pexit->u1.vnum = fread_number(fp);
				pexit->orig_door = door;

				switch (locks) {
				case 1:
					pexit->exit_info = EX_ISDOOR;
					pexit->rs_flags = EX_ISDOOR;
					break;
				case 2:
					pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
					pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF;
					break;
				case 3:
					pexit->exit_info = EX_ISDOOR | EX_NOPASS;
					pexit->rs_flags = EX_ISDOOR | EX_NOPASS;
					break;
				case 4:
					pexit->exit_info = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
					pexit->rs_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
					break;
				}

				room_idx->exit[door] = pexit;
				/*room_idx->old_exit[door] = pexit; */
				top_exit++;
			} else if (letter == 'E') {
				EXTRA_DESCR_DATA *ed;

				ed = new_extra_descr();
				ed->keyword = fread_string(fp);
				ed->description = fread_string(fp);
				ed->next = room_idx->extra_descr;
				room_idx->extra_descr = ed;
				top_ed++;
			} else if (letter == 'O') {
				if (room_idx->owner[0] != '\0') {
					bug("Load_rooms: duplicate owner.", 0);
					raise(SIGABRT);
                    return;
				}

				room_idx->owner = fread_string(fp);
			} else if (letter == 'A') {
				AFFECT_DATA af;
				SKILL *skill;

				if ((skill = skill_lookup(fread_word(fp))) != NULL) {
					af.where = TO_AFFECTS;
					af.type = skill->sn;
					af.skill = skill;
					af.level = (int)fread_number(fp);
					af.duration = -1;
					af.location = 0;
					af.modifier = 0;
					af.bitvector = 0;
					affect_to_room(room_idx, &af);
				}
			} else {
				bug_long("Load_rooms: vnum %d has flag not 'HMDEOS'.", vnum);
				raise(SIGABRT);
                return;
			}
		}

		hash_idx = (int)vnum % MAX_KEY_HASH;
		room_idx->next = room_index_hash[hash_idx];
		room_index_hash[hash_idx] = room_idx;
		top_room++;
		top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
		assign_area_vnum(vnum);
	}

	return;
}


/**
 * returns an initial-capped string.
 */
char *capitalize(const char *str)
{
	static char strcap[MAX_STRING_LENGTH];
	capitalize_into(str, strcap, MAX_STRING_LENGTH);
	return strcap;
}


/*
 * Snarf a shop section.
 */
void load_shops(FILE *fp)
{
	SHOP_DATA *shop;

	for (;; ) {
		MOB_INDEX_DATA *mob_idx;
		int iTrade;

		shop = new_shop();
		shop->keeper = fread_number(fp);
		if (shop->keeper == 0)
			break;
		for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
			shop->buy_type[iTrade] = (int)fread_number(fp);
		shop->profit_buy = (int)fread_number(fp);
		shop->profit_sell = (int)fread_number(fp);
		shop->open_hour = (int)fread_number(fp);
		shop->close_hour = (int)fread_number(fp);
		fread_to_eol(fp);

		mob_idx = get_mob_index(shop->keeper);
		mob_idx->shop = shop;

		if (shop_first == NULL)
			shop_first = shop;
		if (shop_last != NULL)
			shop_last->next = shop;

		shop_last = shop;
		shop->next = NULL;
		top_shop++;
	}

	return;
}


/*
 * Snarf a mob section.  new style
 */
void load_mobiles(FILE *fp)
{
	MOB_INDEX_DATA *mob_idx;

	if (!area_last) {
		bug("Load_mobiles: no #AREA seen yet.", 0);
		raise(SIGABRT);
        return;
	}

	for (;; ) {
		long vnum;
		char letter;
		long hash_idx;

		letter = fread_letter(fp);
		if (letter != '#') {
			bug("Load_mobiles: # not found.", 0);
			raise(SIGABRT);
            return;
		}

		vnum = (long)fread_number(fp);
		if (vnum == 0)
			break;

		db_loading = false;
		if (get_mob_index(vnum) != NULL) {
			bug_long("Load_mobiles: vnum %d duplicated.", vnum);
			raise(SIGABRT);
            return;
		}
		db_loading = true;

		mob_idx = alloc_perm((unsigned int)sizeof(*mob_idx));
		mob_idx->vnum = vnum;
		mob_idx->area = area_last;
		mob_idx->new_format = true;
		newmobs++;

		mob_idx->player_name = fread_string(fp);
		mob_idx->short_descr = fread_string(fp);
		mob_idx->long_descr = fread_string(fp);
		mob_idx->description = fread_string(fp);
		mob_idx->race = race_lookup(fread_string(fp));

		mob_idx->long_descr[0] = UPPER(mob_idx->long_descr[0]);
		mob_idx->description[0] = UPPER(mob_idx->description[0]);

		mob_idx->act = fread_flag(fp) | ACT_IS_NPC | race_table[mob_idx->race].act;
		mob_idx->affected_by = fread_flag(fp) | race_table[mob_idx->race].aff;
		mob_idx->shop = NULL;
		mob_idx->alignment = fread_number(fp);
		mob_idx->group = (int)fread_number(fp);

		mob_idx->level = (int)fread_number(fp);
		mob_idx->hitroll = (int)fread_number(fp);

		/* read hit dice */
		mob_idx->hit[DICE_NUMBER] = (int)fread_number(fp);
		/* 'd'          */ (void)fread_letter(fp);
		mob_idx->hit[DICE_TYPE] = (int)fread_number(fp);
		/* '+'          */ (void)fread_letter(fp);
		mob_idx->hit[DICE_BONUS] = (int)fread_number(fp);

		/* read mana dice */
		mob_idx->mana[DICE_NUMBER] = (int)fread_number(fp);
		(void)fread_letter(fp);
		mob_idx->mana[DICE_TYPE] = (int)fread_number(fp);
		(void)fread_letter(fp);
		mob_idx->mana[DICE_BONUS] = (int)fread_number(fp);

		/* read damage dice */
		mob_idx->damage[DICE_NUMBER] = (int)fread_number(fp);
		(void)fread_letter(fp);
		mob_idx->damage[DICE_TYPE] = (int)fread_number(fp);
		(void)fread_letter(fp);
		mob_idx->damage[DICE_BONUS] = (int)fread_number(fp);
		mob_idx->dam_type = (int)attack_lookup(fread_word(fp));

		/* read armor class */
		mob_idx->ac[AC_PIERCE] = fread_number(fp) * 10;
		mob_idx->ac[AC_BASH] = fread_number(fp) * 10;
		mob_idx->ac[AC_SLASH] = fread_number(fp) * 10;
		mob_idx->ac[AC_EXOTIC] = fread_number(fp) * 10;

		/* read flags and add in data from the race table */
		mob_idx->off_flags = fread_flag(fp) | race_table[mob_idx->race].off;
		mob_idx->imm_flags = fread_flag(fp) | race_table[mob_idx->race].imm;
		mob_idx->res_flags = fread_flag(fp) | race_table[mob_idx->race].res;
		mob_idx->vuln_flags = fread_flag(fp) | race_table[mob_idx->race].vuln;

		/* vital statistics */
		mob_idx->start_pos = position_lookup(fread_word(fp));
		mob_idx->default_pos = position_lookup(fread_word(fp));
		mob_idx->sex = sex_lookup(fread_word(fp));

		mob_idx->wealth = fread_uint(fp);

		mob_idx->form = fread_flag(fp) | race_table[mob_idx->race].form;
		mob_idx->parts = fread_flag(fp) | race_table[mob_idx->race].parts;

		/* size */
		CHECK_POS(mob_idx->size, (int)size_lookup(fread_word(fp)), "size");

		/*mob_idx->size = size_lookup(fread_word(fp));*/
		mob_idx->drained = mob_idx->level;
		mob_idx->material = str_dup(fread_word(fp));

		for (;; ) {
			letter = fread_letter(fp);

			if (letter == 'F') {
				char *word;
				long vector;

				word = fread_word(fp);
				vector = fread_flag(fp);

				if (!str_prefix(word, "act")) {
					REMOVE_BIT(mob_idx->act, vector);
				} else if (!str_prefix(word, "aff")) {
					REMOVE_BIT(mob_idx->affected_by, vector);
				} else if (!str_prefix(word, "off")) {
					REMOVE_BIT(mob_idx->off_flags, vector);
				} else if (!str_prefix(word, "imm")) {
					REMOVE_BIT(mob_idx->imm_flags, vector);
				} else if (!str_prefix(word, "res")) {
					REMOVE_BIT(mob_idx->res_flags, vector);
				} else if (!str_prefix(word, "vul")) {
					REMOVE_BIT(mob_idx->vuln_flags, vector);
				} else if (!str_prefix(word, "for")) {
					REMOVE_BIT(mob_idx->form, vector);
				} else if (!str_prefix(word, "par")) {
					REMOVE_BIT(mob_idx->parts, vector);
				} else {
					bug("Flag remove: flag not found.", 0);
					raise(SIGABRT);
                    return;
				}
			} else if (letter == 'M') {
				MPROG_LIST *mprog;
				char *word;
				int trigger = 0;

				mprog = new_mprog();
				word = fread_word(fp);

				if ((trigger = (int)flag_lookup(word, mprog_flags)) == NO_FLAG) {
					bug("MOBprogs: invalid trigger.", 0);
					raise(SIGABRT);
                    return;
				}

				SET_BIT(mob_idx->mprog_flags, trigger);

				mprog->trig_type = trigger;
				mprog->vnum = fread_number(fp);
				mprog->trig_phrase = fread_string(fp);
				mprog->next = mob_idx->mprogs;
				mob_idx->mprogs = mprog;
			} else {
				ungetc(letter, fp);
				break;
			}
		}

		hash_idx = vnum % MAX_KEY_HASH;
		mob_idx->next = mob_index_hash[hash_idx];
		mob_index_hash[hash_idx] = mob_idx;
		top_mob_index++;

		top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
		assign_area_vnum(vnum);

		kill_table[URANGE(0, mob_idx->level, MAX_LEVEL - 1)].number++;
	}

	return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects(FILE *fp)
{
	OBJ_INDEX_DATA *obj_idx;
	SKILL *skill;

	if (!area_last) {
		bug("Load_objects: no #AREA seen yet.", 0);
		raise(SIGABRT);
        return;
	}

	for (;; ) {
		long vnum;
		char letter;
		long hash_idx;

		letter = fread_letter(fp);
		if (letter != '#') {
			bug("Load_objects: # not found.", 0);
			raise(SIGABRT);
            return;
		}

		vnum = fread_number(fp);
		if (vnum == 0)
			break;

		db_loading = false;
		if (get_obj_index(vnum) != NULL) {
			bug_long("Load_objects: vnum %d duplicated.", vnum);
			raise(SIGABRT);
            return;
		}
		db_loading = true;

		obj_idx = alloc_perm((unsigned int)sizeof(*obj_idx));
		obj_idx->vnum = vnum;
		obj_idx->area = area_last;
		obj_idx->new_format = true;
		obj_idx->reset_num = 0;
		newobjs++;


		obj_idx->name = fread_string(fp);
		obj_idx->short_descr = fread_string(fp);
		obj_idx->description = fread_string(fp);
		obj_idx->material = fread_string(fp);
		obj_idx->extra2_flags = (int)fread_flag(fp);

		CHECK_POS(obj_idx->item_type, (int)item_lookup(fread_word(fp)), "item_type");

		/* obj_idx->item_type	= item_lookup(fread_word(fp)); */
		obj_idx->extra_flags = (int)fread_flag(fp);
		obj_idx->wear_flags = (int)fread_flag(fp);

		switch (obj_idx->item_type) {
		case ITEM_WEAPON:
			obj_idx->value[0] = weapon_type(fread_word(fp));
			obj_idx->value[1] = fread_number(fp);
			obj_idx->value[2] = fread_number(fp);
			obj_idx->value[3] = attack_lookup(fread_word(fp));
			obj_idx->value[4] = fread_flag(fp);
			break;
		case ITEM_CONTAINER:
			obj_idx->value[0] = fread_number(fp);
			obj_idx->value[1] = fread_flag(fp);
			obj_idx->value[2] = fread_number(fp);
			obj_idx->value[3] = fread_number(fp);
			obj_idx->value[4] = fread_number(fp);
			break;
		case ITEM_DRINK_CON:
		case ITEM_FOUNTAIN:
			obj_idx->value[0] = fread_number(fp);
			obj_idx->value[1] = fread_number(fp);
			/*obj_idx->value[2] = liq_lookup(fread_word(fp));*/

			CHECK_POS(obj_idx->value[2], (int)liq_lookup(fread_word(fp)), "liq_lookup");

			obj_idx->value[3] = fread_number(fp);
			obj_idx->value[4] = fread_number(fp);
			break;
		case ITEM_WAND:
		case ITEM_STAFF:

			obj_idx->value[0] = fread_number(fp);
			obj_idx->value[1] = fread_number(fp);
			obj_idx->value[2] = fread_number(fp);
			if ((skill = skill_lookup(fread_word(fp))) != NULL)
				obj_idx->value[3] = skill->sn;
			else
				obj_idx->value[3] = -1;
			obj_idx->value[4] = fread_number(fp);
			break;
		case ITEM_POTION:
		case ITEM_PILL:
		case ITEM_SCROLL:
			obj_idx->value[0] = fread_number(fp);
			if ((skill = skill_lookup(fread_word(fp))) != NULL)
				obj_idx->value[1] = skill->sn;
			if ((skill = skill_lookup(fread_word(fp))) != NULL)
				obj_idx->value[2] = skill->sn;
			if ((skill = skill_lookup(fread_word(fp))) != NULL)
				obj_idx->value[3] = skill->sn;
			if ((skill = skill_lookup(fread_word(fp))) != NULL)
				obj_idx->value[4] = skill->sn;
			break;
		default:
			obj_idx->value[0] = fread_flag(fp);
			obj_idx->value[1] = fread_flag(fp);
			obj_idx->value[2] = fread_flag(fp);
			obj_idx->value[3] = fread_flag(fp);
			obj_idx->value[4] = fread_flag(fp);
			break;
		}

		obj_idx->level = (int)fread_number(fp);
		obj_idx->weight = (int)fread_number(fp);
		obj_idx->cost = (unsigned int)fread_number(fp);
		obj_idx->timer = fread_number(fp);

		/* condition */
		letter = fread_letter(fp);
		switch (letter) {
		case ('P'):
			obj_idx->condition = 100;
			break;
		case ('G'):
			obj_idx->condition = 90;
			break;
		case ('A'):
			obj_idx->condition = 75;
			break;
		case ('W'):
			obj_idx->condition = 50;
			break;
		case ('D'):
			obj_idx->condition = 25;
			break;
		case ('B'):
			obj_idx->condition = 10;
			break;
		case ('R'):
			obj_idx->condition = 0;
			break;
		default:
			obj_idx->condition = 100;
			break;
		}

		for (;; ) {
			char letter;

			letter = fread_letter(fp);

			if (letter == 'A') {
				AFFECT_DATA *paf;

				paf = new_affect();
				paf->where = TO_OBJECT;
				paf->type = -1;
				paf->level = obj_idx->level;
				paf->duration = -1;
				paf->location = (int)fread_number(fp);
				paf->modifier = (int)fread_number(fp);
				paf->bitvector = 0;
				paf->next = obj_idx->affected;
				obj_idx->affected = paf;
				top_affect++;
			} else if (letter == 'F') {
				AFFECT_DATA *paf;

				paf = new_affect();
				letter = fread_letter(fp);
				switch (letter) {
				case 'A':
					paf->where = TO_AFFECTS;
					break;
				case 'C':
					paf->where = TO_ACT_FLAG;
					break;
				case 'I':
					paf->where = TO_IMMUNE;
					break;
				case 'R':
					paf->where = TO_RESIST;
					break;
				case 'V':
					paf->where = TO_VULN;
					break;
				default:
					bug("Load_objects: Bad where on flag set.", 0);
					raise(SIGABRT);
                    return;
				}
				paf->type = -1;
				paf->level = obj_idx->level;
				paf->duration = -1;
				paf->location = (int)fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->bitvector = fread_flag(fp);
				paf->next = obj_idx->affected;
				obj_idx->affected = paf;
				top_affect++;
			} else if (letter == 'E') {
				EXTRA_DESCR_DATA *ed;

				ed = new_extra_descr();
				ed->keyword = fread_string(fp);
				ed->description = fread_string(fp);
				ed->next = obj_idx->extra_descr;
				obj_idx->extra_descr = ed;
				top_ed++;
			} else {
				ungetc(letter, fp);
				break;
			}
		}

		hash_idx = vnum % MAX_KEY_HASH;
		obj_idx->next = obj_index_hash[hash_idx];
		obj_index_hash[hash_idx] = obj_idx;
		top_obj_index++;

		top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
		assign_area_vnum(vnum);
	}

	return;
}







/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits(void)
{
	extern const int rev_dir[];
	ROOM_INDEX_DATA *room_idx;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;
	RESET_DATA *reset;
	ROOM_INDEX_DATA *iLastRoom;
	ROOM_INDEX_DATA *ilast_obj;
	int hash_idx;
	int door;

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (room_idx = room_index_hash[hash_idx];
		     room_idx != NULL;
		     room_idx = room_idx->next) {
			bool fexit;

			iLastRoom = ilast_obj = NULL;

			/* OLC : nuevo chequeo de resets */
			for (reset = room_idx->reset_first; reset; reset = reset->next) {
				switch (reset->command) {
				default:
					log_string("fix_exits : room %d with reset cmd %c", room_idx->vnum, reset->command);
					raise(SIGABRT);
                    return;

				case 'M':
					get_mob_index(reset->arg1);
					iLastRoom = get_room_index(reset->arg3);
					break;

				case 'O':
					get_obj_index(reset->arg1);
					ilast_obj = get_room_index(reset->arg3);
					break;

				case 'P':
					get_obj_index(reset->arg1);
					if (ilast_obj == NULL) {
						log_string("fix_exits : reset in room %d con ilast_obj NULL", room_idx->vnum);
						raise(SIGABRT);
                        return;
					}
					break;

				case 'G':
				case 'E':
					get_obj_index(reset->arg1);
					if (iLastRoom == NULL) {
						log_bug("fix_exits : reset in room %d with iLastRoom NULL", room_idx->vnum);
						raise(SIGABRT);
                        return;
					}
					ilast_obj = iLastRoom;
					break;

				case 'D':
					break;

				case 'R':
					get_room_index(reset->arg1);
					if (reset->arg2 < 0 || reset->arg2 > MAX_DIR) {
						log_bug("fix_exits : reset in room %d with arg2 %d >= MAX_DIR", room_idx->vnum, reset->arg2);
						raise(SIGABRT);
                        return;
					}
					break;
				}       /* switch */
			}               /* for */

			fexit = false;
			for (door = 0; door <= 5; door++) {
				if ((pexit = room_idx->exit[door]) != NULL) {
					if (pexit->u1.vnum <= 0
					    || get_room_index(pexit->u1.vnum) == NULL) {
						pexit->u1.to_room = NULL;
					} else {
						fexit = true;
						pexit->u1.to_room = get_room_index(pexit->u1.vnum);
					}
				}
			}
			if (!fexit)
				SET_BIT(room_idx->room_flags, ROOM_NO_MOB);
		}
	}

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (room_idx = room_index_hash[hash_idx];
		     room_idx != NULL;
		     room_idx = room_idx->next) {
			for (door = 0; door <= 5; door++) {
				if ((pexit = room_idx->exit[door]) != NULL
				    && (to_room = pexit->u1.to_room) != NULL
				    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
				    && pexit_rev->u1.to_room != room_idx
				    && (room_idx->vnum < 1200 || room_idx->vnum > 1299)
				    && (room_idx->vnum < 5700 && room_idx->vnum > 5799)) {
					log_string("Fix_exits: %d:%d -> %d:%d -> %d.",
						   room_idx->vnum, door,
						   to_room->vnum, rev_dir[door],
						   (pexit_rev->u1.to_room == NULL) ? 0 : pexit_rev->u1.to_room->vnum);
				}
			}
		}
	}

	return;
}


/*
 * Load mobprogs section
 */
void load_mobprogs(FILE *fp)
{
	MPROG_CODE *pMprog;

	if (area_last == NULL) {
		bug("Load_mobprogs: no #AREA seen yet.", 0);
		raise(SIGABRT);
        return;
	}

	for (;; ) {
		long vnum;
		char letter;

		letter = fread_letter(fp);
		if (letter != '#') {
			bug("Load_mobprogs: # not found.", 0);
			raise(SIGABRT);
            return;
		}

		vnum = fread_number(fp);
		if (vnum == 0)
			break;

		db_loading = false;
		if (get_mprog_index(vnum) != NULL) {
			bug_long("Load_mobprogs: vnum %d duplicated.", vnum);
			raise(SIGABRT);
            return;
		}
		db_loading = true;

		pMprog = alloc_perm((unsigned int)sizeof(*pMprog));
		pMprog->vnum = vnum;
		pMprog->code = fread_string(fp);
		pMprog->comment = str_dup("");

		if (mprog_list == NULL) {
			mprog_list = pMprog;
		} else {
			pMprog->next = mprog_list;
			mprog_list = pMprog;
		}

		top_mprog_index++;
	}

	return;
}


void load_mobprogs_new(FILE *fp)
{
	MPROG_CODE *mprog;

	if (area_last == NULL) {
		bug("load_mobprogs: no #AREA seen yet.", 0);
		raise(SIGABRT);
        return;
	}

	mprog = NULL;
	for (;; ) {
		char *word;
		int vnum;

		word = fread_word(fp);

		if (word[0] == '#') {
			char *idx;

			idx = word;
			idx++;

			if (mprog != NULL) {
				if (!str_cmp(mprog->comment, "(null)")) {
					free_string(mprog->comment);
					mprog->comment = str_dup("");
				}

				if (mprog_list == NULL) {
					mprog_list = mprog;
				} else {
					mprog->next = mprog_list;
					mprog_list = mprog;
				}

				top_mprog_index++;
			}
			vnum = atoi(idx);

			if (vnum == 0)
				break;

			db_loading = false;
			if (get_mprog_index(vnum) != NULL) {
				bug("load_mobprogs: vnum %d duplicated.", vnum);
				raise(SIGABRT);
                return;
			}
			db_loading = true;

			mprog = new_mpcode();
			mprog->vnum = vnum;
		} else {
			if (mprog != NULL) {
				switch (UPPER(word[0])) {
				case 'C':
					SKEY(word, "comment", mprog->comment);
					SKEY(word, "code", mprog->code);
					break;
				}
			}
		}
	}

	return;
}


/*
 *  Translate mobprog vnums pointers to real code
 */
void fix_mobprogs(void)
{
	MOB_INDEX_DATA *mob_idx;
	MPROG_LIST *list;
	MPROG_CODE *prog;
	int hash_idx;

	for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
		for (mob_idx = mob_index_hash[hash_idx];
		     mob_idx != NULL;
		     mob_idx = mob_idx->next) {
			for (list = mob_idx->mprogs; list != NULL; list = list->next) {
				if ((prog = get_mprog_index(list->vnum)) != NULL) {
					list->code = prog->code;
				} else {
					bug_long("fix_mobprogs: code vnum %d not found.", list->vnum);
					raise(SIGABRT);
                    return;
				}
			}
		}
	}
}

void reset_areas()
{
	AREA_DATA *area;

	for (area = area_first; area != NULL; area = area->next)
		reset_area(area);
}

/*
 * Repopulate areas periodically.
 */
void area_update(void)
{
	AREA_DATA *area;
	char buf[MSL];

	for (area = area_first; area != NULL; area = area->next) {
		if (++area->age < 3)
			continue;

		/*
		 * Check age and reset.
		 * Note: Mud School resets every 3 minutes(not 15).
		 */
		if ((!area->empty && (area->nplayer == 0 || area->age >= 15))
		    || area->age >= 31) {
			ROOM_INDEX_DATA *room_idx;

			reset_area(area);
			sprintf(buf, "%s has just been reset.", area->name);
			impnet(buf, NULL, NULL, (long)IMN_RESETS, 0, 0);

			area->age = (int)number_range(0, 3);
			room_idx = get_room_index(ROOM_VNUM_SCHOOL);
			if (room_idx != NULL && area == room_idx->area)
				area->age = 15 - 2;
			else if (area->nplayer == 0)
				area->empty = true;
		}
	}

	return;
}


/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(ROOM_INDEX_DATA *room)
{
	RESET_DATA *reset;
	CHAR_DATA *mob_it;
	CHAR_DATA *mob = NULL;
	OBJ_DATA *obj = NULL;
	CHAR_DATA *last_mob = NULL;
	OBJ_DATA *last_obj = NULL;
	int exit_dir;
	bool last;

	if (!room)
		return;

	mob = NULL;
	last = false;
	for (exit_dir = 0; exit_dir < MAX_DIR; exit_dir++) {
		EXIT_DATA *exit;
		if ((exit = room->exit[exit_dir])) {
			exit->exit_info = (int)exit->rs_flags;

			if ((exit->u1.to_room != NULL)
			    && ((exit = exit->u1.to_room->exit[rev_dir[exit_dir]])))
				exit->exit_info = (int)exit->rs_flags;
		}
	}

	for (reset = room->reset_first; reset != NULL; reset = reset->next) {
		MOB_INDEX_DATA *mob_idx;
		OBJ_INDEX_DATA *obj_idx;
		OBJ_INDEX_DATA *obj_to_idx;
		ROOM_INDEX_DATA *room_idx;
		char buf[MSL];
		int count;
		int limit = 0;

		switch (reset->command) {
		default:
			bug("Reset_room: bad command %c.", (int)reset->command);
			break;

		case 'M':
			if (!(mob_idx = get_mob_index(reset->arg1))) {
				bug_long("Reset_room: 'M': bad vnum %d.", reset->arg1);
				continue;
			}

			if ((room_idx = get_room_index(reset->arg3)) == NULL) {
				bug_long("Reset_area: 'R': bad vnum %d.", reset->arg3);
				continue;
			}

			if (mob_idx->count >= (int)reset->arg2) {
				last = false;
				break;
			}

			count = 0;
			for (mob_it = room_idx->people; mob_it != NULL; mob_it = mob_it->next_in_room) {
				if (mob_it->mob_idx == mob_idx) {
					count++;
					if (count >= reset->arg4) {
						last = false;
						break;
					}
				}
			}

			if (count >= reset->arg4)
				break;

			mob = create_mobile(mob_idx);
			mob->zone = mob_idx->area;

			/*
			 * Some more hard coding.
			 */
			if (room_is_dark(NULL, room))
				SET_BIT(mob->affected_by, AFF_INFRARED);

			/*
			 * Pet shop mobiles get ACT_PET set.
			 */
			{
				ROOM_INDEX_DATA *room_idxPrev;

				room_idxPrev = get_room_index(room->vnum - 1);
				if (room_idxPrev && IS_SET(room_idxPrev->room_flags, ROOM_PET_SHOP))
					SET_BIT(mob->act, ACT_PET);
			}

			char_to_room(mob, room);

			last_mob = mob;
			last = true;
			break;

		case 'O':
			if (!(obj_idx = get_obj_index(reset->arg1))) {
				bug_long("Reset_room: 'O' 1 : bad vnum %d", reset->arg1);
				sprintf(buf, "%ld %d %ld %d", reset->arg1, reset->arg2, reset->arg3, reset->arg4);
				bug(buf, 1);
				continue;
			}

			if (!(room_idx = get_room_index(reset->arg3))) {
				bug_long("Reset_room: 'O' 2 : bad vnum %d.", reset->arg3);
				sprintf(buf, "%ld %d %ld %d", reset->arg1, reset->arg2, reset->arg3, reset->arg4);
				bug(buf, 1);
				continue;
			}

			/*
			 * this was causing problems for the 'repop' command
			 * and the builders trying to use it to test their
			 * resets -- i just took out the testing for players
			 * in the area because i dont really care about that...
			 */
			/*if(room->area->nplayer > 0
			|| count_obj_list(obj_idx, room->contents) > 0)*/
			if (count_obj_list(obj_idx, room->contents) > 0) {
				last = false;
				break;
			}

			obj = create_object(obj_idx, -1);
			obj->cost = 0;
			obj_to_room(obj, room);
			last = true;
			break;

		case 'P':
			if (!(obj_idx = get_obj_index(reset->arg1))) {
				bug_long("Reset_room: 'P': bad vnum %d.", reset->arg1);
				continue;
			}

			if (!(obj_to_idx = get_obj_index(reset->arg3))) {
				bug_long("Reset_room: 'P': bad vnum %d.", reset->arg3);
				continue;
			}

			if (reset->arg2 == 0)
				reset->arg2 = -1;

			if (reset->arg2 > 50)           /* old format */
				limit = (int)999;
			else if (reset->arg2 == -1)     /* no limit */
				limit = (int)999;
			else
				limit = (int)reset->arg2;

			if (room->area->nplayer > 0
			    || (last_obj = get_obj_type(obj_to_idx)) == NULL
			    || (last_obj->in_room == NULL && !last)
			    || (obj_idx->count >= limit && number_range(0, 2) != 0)
			    || (count = count_obj_list(obj_idx, last_obj->contains)) > reset->arg4) {
				last = false;
				break;
			}

			while (count < reset->arg4) {
				obj = create_object(obj_idx, -1);
				obj_to_obj(obj, last_obj);
				count++;
				if (obj_idx->count >= (int)limit)
					break;
			}

			/* fix object lock state! */
			last_obj->value[1] = last_obj->obj_idx->value[1];
			last = true;
			break;

		case 'G':
		case 'E':
			if (!(obj_idx = get_obj_index(reset->arg1))) {
				bug_long("Reset_room: 'E' or 'G': bad vnum %d.", reset->arg1);
				continue;
			}

			if (!last)
				break;

			if (!last_mob) {
				bug_long("Reset_room: 'E' or 'G': null mob for vnum %d.", reset->arg1);
				last = false;
				break;
			}

			if (IS_SHOPKEEPER(last_mob)) { /* Shop-keeper? */
				int olevel = 0;

				if (!obj_idx->new_format) {
					switch (obj_idx->item_type) {
					default:
						olevel = 0;
						break;
					case ITEM_PILL:
					case ITEM_POTION:
					case ITEM_SCROLL:
						olevel = 53;
						olevel = (int)UMAX(0, (olevel * 3 / 4) - 2);
						break;
					case ITEM_WAND:
						olevel = (int)number_range(10, 20);
						break;
					case ITEM_STAFF:
						olevel = (int)number_range(15, 25);
						break;
					case ITEM_ARMOR:
						olevel = (int)number_range(5, 15);
						break;
					case ITEM_WEAPON:
						olevel = (int)number_range(5, 15);
						break;
					case ITEM_TREASURE:
						olevel = (int)number_range(10, 20);
						break;
					}
				}

				obj = create_object(obj_idx, -1);
				SET_BIT(obj->extra_flags, ITEM_INVENTORY);  /* ROM OLC */
			} else { /* ROM OLC else version */
				int limit;
				if (reset->arg2 > 50)                           /* old format */
					limit = (int)999;
				else if (reset->arg2 == -1 || reset->arg2 == 0) /* no limit */
					limit = (int)999;
				else
					limit = (int)reset->arg2;

				if (obj_idx->count < limit || number_range(0, 4) == 0)
					obj = create_object(obj_idx, -1);

				else
					break;
			}

			if (obj != NULL) {
				obj_to_char(obj, last_mob);
				if (reset->command == 'E')
					equip_char(last_mob, obj, (int)reset->arg3);
				last = true;
			}
			break;

		case 'D':
			break;

		case 'R':
			if (!(room_idx = get_room_index(reset->arg1))) {
				bug_long("Reset_room: 'R': bad vnum %d.", reset->arg1);
				continue;
			}

			{
				EXIT_DATA *exit;
				int d0;
				int d1;

				for (d0 = 0; d0 < reset->arg2 - 1; d0++) {
					d1 = number_range(d0, reset->arg2 - 1);
					exit = room_idx->exit[d0];
					room_idx->exit[d0] = room_idx->exit[d1];
					room_idx->exit[d1] = exit;
				}
			}
			break;
		}
	}

	return;
}

/* OLC
 * Reset one area.
 */
void reset_area(AREA_DATA *area)
{
	ROOM_INDEX_DATA *room;
	long vnum;

	for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++)
		if ((room = get_room_index(vnum)))
			reset_room(room);

	return;
}


/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile(MOB_INDEX_DATA *mob_idx)
{
	CHAR_DATA *mob;
	SKILL *skill;
	int idx;
	AFFECT_DATA af;

	mobile_count++;

	if (mob_idx == NULL) {
		bug("Create_mobile: NULL mob_idx.", 0);
		raise(SIGABRT);
        return NULL;
	}

	mob = new_char();

	mob->mob_idx = mob_idx;
	mob->id = get_mob_id();

	mob->name = str_dup(mob_idx->player_name);
	mob->short_descr = str_dup(mob_idx->short_descr);
	mob->long_descr = str_dup(mob_idx->long_descr);
	mob->description = str_dup(mob_idx->description);

	mob->prompt = NULL;
	mob->mprog_target = NULL;

	if (mob_idx->wealth == 0) {
		mob->silver = 0;
		mob->gold = 0;
	} else {
		long wealth;

		wealth = number_range((int)(mob_idx->wealth / 2), (int)(3 * mob_idx->wealth / 2));
		mob->gold = (unsigned int)number_range((int)(wealth / 200), (int)(wealth / 100));
		mob->silver = (unsigned int)(wealth - (mob->gold * 100));
	}

	if (mob_idx->new_format) {
		/* read from prototype */
		mob->group = mob_idx->group;
		mob->act = mob_idx->act;

		mob->channels_denied = CHANNEL_SHOUT;
		mob->affected_by = mob_idx->affected_by;
		mob->alignment = mob_idx->alignment;
		mob->level = mob_idx->level;
		mob->hitroll = mob_idx->hitroll;
		mob->damroll = mob_idx->damage[DICE_BONUS];
		mob->max_hit = dice(mob_idx->hit[DICE_NUMBER],
				    mob_idx->hit[DICE_TYPE]) + mob_idx->hit[DICE_BONUS];
		mob->hit = mob->max_hit;
		mob->max_mana = dice(mob_idx->mana[DICE_NUMBER],
				     mob_idx->mana[DICE_TYPE]) + mob_idx->mana[DICE_BONUS];
		mob->mana = mob->max_mana;

		mob->damage[DICE_NUMBER] = mob_idx->damage[DICE_NUMBER];
		mob->damage[DICE_TYPE] = mob_idx->damage[DICE_TYPE];
		mob->dam_type = mob_idx->dam_type;

		if (mob->dam_type == 0) {
			switch (number_range(1, 3)) {
			case (1):
				mob->dam_type = 3;
				break;  /* slash */
			case (2):
				mob->dam_type = 7;
				break;  /* pound */
			case (3):
				mob->dam_type = 11;
				break;  /* pierce */
			}
		}

		for (idx = 0; idx < 4; idx++)
			mob->armor[idx] = mob_idx->ac[idx];

		mob->off_flags = mob_idx->off_flags;
		mob->imm_flags = mob_idx->imm_flags;
		mob->res_flags = mob_idx->res_flags;
		mob->vuln_flags = mob_idx->vuln_flags;
		mob->start_pos = mob_idx->start_pos;
		mob->default_pos = mob_idx->default_pos;
		mob->sex = mob_idx->sex;
		if (mob->sex == 3)       /* random sex */
			mob->sex = (int)number_range(1, 2);

		mob->race = mob_idx->race;
		mob->form = mob_idx->form;
		mob->parts = mob_idx->parts;
		mob->size = mob_idx->size;
		mob->material = str_dup(mob_idx->material);

		/* computed on the spot */
		for (idx = 0; idx < MAX_STATS; idx++)
			mob->perm_stat[idx] = URANGE(3, mob->level / 4, 50);

		if (IS_SET(mob->act, ACT_WARRIOR)) {
			mob->perm_stat[STAT_STR] += 3;
			mob->perm_stat[STAT_INT] -= 1;
			mob->perm_stat[STAT_CON] += 2;
		}

		if (IS_SET(mob->act, ACT_THIEF)) {
			mob->perm_stat[STAT_DEX] += 3;
			mob->perm_stat[STAT_INT] += 1;
			mob->perm_stat[STAT_WIS] -= 1;
		}

		if (IS_SET(mob->act, ACT_CLERIC)) {
			mob->perm_stat[STAT_WIS] += 3;
			mob->perm_stat[STAT_DEX] -= 1;
			mob->perm_stat[STAT_STR] += 1;
		}

		if (IS_SET(mob->act, ACT_MAGE)) {
			mob->perm_stat[STAT_INT] += 3;
			mob->perm_stat[STAT_STR] -= 1;
			mob->perm_stat[STAT_DEX] += 1;
		}

		if (IS_SET(mob->off_flags, OFF_FAST))
			mob->perm_stat[STAT_DEX] += 2;

		mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
		mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

		/* let's get some spell action */
		if (IS_AFFECTED(mob, AFF_SANCTUARY)) {
			if ((skill = skill_lookup("sanctuary")) != NULL) {
				af.where = TO_AFFECTS;
				af.type = skill->sn;
				af.skill = skill;
				af.level = mob->level;
				af.duration = -1;
				af.location = APPLY_NONE;
				af.modifier = 0;
				af.bitvector = AFF_SANCTUARY;
				affect_to_char(mob, &af);
			}
		}

		if (IS_AFFECTED(mob, AFF_DRUID_CALL)) {
			if ((skill = skill_lookup("druid call")) != NULL) {
				af.where = TO_AFFECTS;
				af.type = skill->sn;
				af.skill = skill;
				af.level = mob->level;
				af.duration = -1;
				af.location = APPLY_NONE;
				af.modifier = 0;
				af.bitvector = AFF_DRUID_CALL;
				affect_to_char(mob, &af);
			}
		}

		if (check_affected(mob, "fear")) {
			if ((skill = skill_lookup("fear")) != NULL) {
				af.where = TO_AFFECTS;
				af.type = skill->sn;
				af.skill = skill;
				af.level = mob->level;
				af.duration = -1;
				af.location = APPLY_NONE;
				af.modifier = 0;
				af.bitvector = 0;
				affect_to_char(mob, &af);
			}
		}

		if (IS_AFFECTED(mob, AFF_HASTE)) {
			if ((skill = skill_lookup("haste")) != NULL) {
				af.where = TO_AFFECTS;
				af.type = skill->sn;
				af.skill = skill;
				af.level = mob->level;
				af.duration = -1;
				af.location = APPLY_DEX;
				af.modifier = 1 + mob->level / 100;
				af.bitvector = AFF_HASTE;
				affect_to_char(mob, &af);
			}
		}

		if (IS_AFFECTED(mob, AFF_PROTECT_EVIL)) {
			if ((skill = skill_lookup("protection evil")) != NULL) {
				af.where = TO_AFFECTS;
				af.type = skill->sn;
				af.skill = skill;
				af.level = mob->level;
				af.duration = -1;
				af.location = APPLY_SAVES;
				af.modifier = -1;
				af.bitvector = AFF_PROTECT_EVIL;
				affect_to_char(mob, &af);
			}
		}

		if (IS_AFFECTED(mob, AFF_PROTECT_GOOD)) {
			if ((skill = skill_lookup("protection good")) != NULL) {
				af.where = TO_AFFECTS;
				af.type = skill->sn;
				af.skill = skill;
				af.level = mob->level;
				af.duration = -1;
				af.location = APPLY_SAVES;
				af.modifier = -1;
				af.bitvector = AFF_PROTECT_GOOD;
				affect_to_char(mob, &af);
			}
		}
	} else {
		/* read in old format and convert */
		mob->act = mob_idx->act;
		mob->affected_by = mob_idx->affected_by;
		mob->alignment = mob_idx->alignment;
		mob->level = mob_idx->level;
		mob->hitroll = mob_idx->hitroll;
		mob->damroll = 0;
		mob->max_hit = mob->level * 8 + number_range(mob->level * mob->level / 4, mob->level * mob->level);
		mob->max_hit /= 9;
		mob->hit = mob->max_hit;
		mob->max_mana = 100 + dice(mob->level, 10);
		mob->mana = mob->max_mana;

		switch (number_range(1, 3)) {
		case (1):
			mob->dam_type = 3;
			break;  /* slash */
		case (2):
			mob->dam_type = 7;
			break;  /* pound */
		case (3):
			mob->dam_type = 11;
			break;  /* pierce */
		}

		for (idx = 0; idx < 3; idx++)
			mob->armor[idx] = interpolate(mob->level, 100, -100);

		mob->armor[3] = interpolate(mob->level, 100, 0);
		mob->race = mob_idx->race;
		mob->off_flags = mob_idx->off_flags;
		mob->imm_flags = mob_idx->imm_flags;
		mob->res_flags = mob_idx->res_flags;
		mob->vuln_flags = mob_idx->vuln_flags;
		mob->start_pos = mob_idx->start_pos;
		mob->default_pos = mob_idx->default_pos;
		mob->sex = mob_idx->sex;
		mob->form = mob_idx->form;
		mob->parts = mob_idx->parts;
		mob->size = SIZE_MEDIUM;
		mob->material = "";

		for (idx = 0; idx < MAX_STATS; idx++)
			mob->perm_stat[idx] = 11 + mob->level / 4;
	}

	mob->position = mob->start_pos;


/* link the mob to the world list */
	mob->next = char_list;
	char_list = mob;
	mob_idx->count++;

	return mob;
}


/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone)
{
	int i;
	AFFECT_DATA *paf;

	if (parent == NULL || clone == NULL || !IS_NPC(parent))
		return;

/* start fixing values */
	clone->name = str_dup(parent->name);
	clone->version = parent->version;
	clone->short_descr = str_dup(parent->short_descr);
	clone->long_descr = str_dup(parent->long_descr);
	clone->description = str_dup(parent->description);
	clone->group = parent->group;
	clone->sex = parent->sex;
	clone->class = parent->class;
	clone->race = parent->race;
	clone->level = parent->level;
	clone->trust = 0;
	clone->timer = parent->timer;
	clone->wait = parent->wait;
	clone->hit = parent->hit;
	clone->max_hit = parent->max_hit;
	clone->mana = parent->mana;
	clone->max_mana = parent->max_mana;
	clone->move = parent->move;
	clone->max_move = parent->max_move;
	clone->gold = parent->gold;
	clone->silver = parent->silver;
	clone->exp = parent->exp;
	clone->act = parent->act;
	clone->comm = parent->comm;
	clone->imm_flags = parent->imm_flags;
	clone->res_flags = parent->res_flags;
	clone->vuln_flags = parent->vuln_flags;
	clone->invis_level = parent->invis_level;
	clone->affected_by = parent->affected_by;
	clone->position = parent->position;
/*clone->practice   = parent->practice; */
/*clone->train      = parent->train; */
	clone->saving_throw = parent->saving_throw;
	clone->alignment = parent->alignment;
	clone->hitroll = parent->hitroll;
	clone->damroll = parent->damroll;
	clone->wimpy = parent->wimpy;
	clone->form = parent->form;
	clone->parts = parent->parts;
	clone->size = parent->size;
	clone->material = str_dup(parent->material);
	clone->off_flags = parent->off_flags;
	clone->dam_type = parent->dam_type;
	clone->start_pos = parent->start_pos;
	clone->default_pos = parent->default_pos;

	for (i = 0; i < 4; i++)
		clone->armor[i] = parent->armor[i];

	for (i = 0; i < MAX_STATS; i++) {
		clone->perm_stat[i] = parent->perm_stat[i];
		clone->mod_stat[i] = parent->mod_stat[i];
	}

	for (i = 0; i < 3; i++)
		clone->damage[i] = parent->damage[i];

/* now add the affects */
	for (paf = parent->affected; paf != NULL; paf = paf->next)
		affect_to_char(clone, paf);
}




/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object(OBJ_INDEX_DATA *obj_idx, int level)
{
	AFFECT_DATA *paf;
	OBJ_DATA *obj;

	if (obj_idx == NULL) {
		bug("Create_object: NULL obj_idx.", 0);
		raise(SIGABRT);
        return NULL;
	}

	obj = new_object();

	obj->obj_idx = obj_idx;
	obj->in_room = NULL;
	obj->enchanted = false;

	if (level == -1) {
		if (obj_idx->new_format)
			obj->level = obj_idx->level;
		else
			obj->level = 0;
	} else {
		obj->level = level;
	}

	obj->wear_loc = -1;

	obj->name = str_dup(obj_idx->name);
	obj->short_descr = str_dup(obj_idx->short_descr);
	obj->description = str_dup(obj_idx->description);
	obj->material = str_dup(obj_idx->material);
	obj->timer = obj_idx->timer;
	obj->item_type = obj_idx->item_type;
	obj->extra_flags = obj_idx->extra_flags;
	obj->wear_flags = obj_idx->wear_flags;
	obj->value[0] = obj_idx->value[0];
	obj->value[1] = obj_idx->value[1];
	obj->value[2] = obj_idx->value[2];
	obj->value[3] = obj_idx->value[3];
	obj->value[4] = obj_idx->value[4];
	obj->weight = obj_idx->weight;
	if (IS_OBJ_STAT2(obj_idx, ITEM_RELIC))
		obj_idx->xp_tolevel = 1500;

	if (level == -1 || obj_idx->new_format)
		obj->cost = obj_idx->cost;
	else
		obj->cost = (unsigned int)(number_fuzzy(10) * number_fuzzy(level) * number_fuzzy(level));
/*
 * Mess with object properties.
 */
	switch (obj->item_type) {
	default:
		bug_long("Read_object: vnum %d bad type.", obj_idx->vnum);
		break;

	case ITEM_LIGHT:
		if (obj->value[2] == 999)
			obj->value[2] = -1;
		break;

	case ITEM_FURNITURE:
	case ITEM_TRASH:
	case ITEM_CONTAINER:
	case ITEM_DRINK_CON:
	case ITEM_KEY:
	case ITEM_SOCKETS:
	case ITEM_FOOD:
	case ITEM_BOAT:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	case ITEM_FOUNTAIN:
	case ITEM_MAP:
	case ITEM_CLOTHING:
	case ITEM_PORTAL:
		if (!obj_idx->new_format)
			obj->cost /= 5;
		break;

	case ITEM_ATM:
	case ITEM_TELEPORT:
	case ITEM_TREASURE:
	case ITEM_WARP_STONE:
	case ITEM_GEM:
	case ITEM_JEWELRY:
	case ITEM_DOLL:
	case ITEM_DICE:
		break;

	case ITEM_SCROLL:
		if (level != -1 && !obj_idx->new_format)
			obj->value[0] = number_fuzzy_long(obj->value[0]);
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		if (level != -1 && !obj_idx->new_format) {
			obj->value[0] = number_fuzzy_long(obj->value[0]);
			obj->value[1] = number_fuzzy_long(obj->value[1]);
			obj->value[2] = obj->value[1];
		}
		if (!obj_idx->new_format)
			obj->cost *= 2;
		break;

	case ITEM_WEAPON:
		if (level != -1 && !obj_idx->new_format) {
			obj->value[1] = number_fuzzy(number_fuzzy(1 * level / 4 + 2));
			obj->value[2] = number_fuzzy(number_fuzzy(3 * level / 4 + 6));
		}
		break;

	case ITEM_ARMOR:
		if (level != -1 && !obj_idx->new_format) {
			obj->value[0] = number_fuzzy(level / 5 + 3);
			obj->value[1] = number_fuzzy(level / 5 + 3);
			obj->value[2] = number_fuzzy(level / 5 + 3);
			obj->value[3] = number_fuzzy(level / 5 + 3);
		}
		break;

	case ITEM_POTION:
	case ITEM_PILL:
		if (level != -1 && !obj_idx->new_format)
			obj->value[0] = number_fuzzy_long(number_fuzzy_long(obj->value[0]));
		break;

	case ITEM_MONEY:
		if (!obj_idx->new_format)
			obj->value[0] = (long)obj->cost;
		break;
	}

	for (paf = obj_idx->affected; paf != NULL; paf = paf->next)
		if (paf->location == APPLY_SPELL_AFFECT)
			affect_to_obj(obj, paf);

	obj->next = globalSystemState.object_head;
	globalSystemState.object_head = obj;
	obj_idx->count++;

	return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone)
{
	int i;
	AFFECT_DATA *paf;
	EXTRA_DESCR_DATA *ed, *ed_new;

	if (parent == NULL || clone == NULL)
		return;

/* start fixing the object */
	clone->name = str_dup(parent->name);
	clone->short_descr = str_dup(parent->short_descr);
	clone->description = str_dup(parent->description);
	clone->item_type = parent->item_type;
	clone->extra_flags = parent->extra_flags;
	clone->wear_flags = parent->wear_flags;
	clone->weight = parent->weight;
	clone->cost = parent->cost;
	clone->level = parent->level;
	clone->condition = parent->condition;
	clone->material = str_dup(parent->material);
	clone->timer = parent->timer;

	for (i = 0; i < 5; i++)
		clone->value[i] = parent->value[i];

/* affects */
	clone->enchanted = parent->enchanted;

	for (paf = parent->affected; paf != NULL; paf = paf->next)
		affect_to_obj(clone, paf);

/* extended desc */
	for (ed = parent->extra_descr; ed != NULL; ed = ed->next) {
		ed_new = new_extra_descr();
		ed_new->keyword = str_dup(ed->keyword);
		ed_new->description = str_dup(ed->description);
		ed_new->next = clone->extra_descr;
		clone->extra_descr = ed_new;
	}
}



/*
 * Clear a new character.
 */
void clear_char(CHAR_DATA *ch)
{
	static CHAR_DATA ch_zero;
	int i;

	*ch = ch_zero;
	ch->name = &str_empty[0];
	ch->short_descr = &str_empty[0];
	ch->long_descr = &str_empty[0];
	ch->description = &str_empty[0];
	ch->prompt = &str_empty[0];
	ch->lines = PAGELEN;
	for (i = 0; i < 4; i++)
		ch->armor[i] = 100;
	ch->position = POS_STANDING;
	ch->hit = 20;
	ch->max_hit = 20;
	ch->mana = 100;
	ch->max_mana = 100;
	ch->move = 100;
	ch->max_move = 100;
	ch->on = NULL;
	for (i = 0; i < MAX_STATS; i++) {
		ch->perm_stat[i] = 13;
		ch->mod_stat[i] = 0;
	}
	return;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA *ed)
{
	for (; ed != NULL; ed = ed->next)
		if (is_name((char *)name, ed->keyword))
			return ed->description;
	return NULL;
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index(long vnum)
{
	MOB_INDEX_DATA *mob_idx;

	for (mob_idx = mob_index_hash[vnum % MAX_KEY_HASH];
	     mob_idx != NULL;
	     mob_idx = mob_idx->next)
		if (mob_idx->vnum == vnum)
			return mob_idx;

	if (db_loading) {
		bug_long("Get_mob_index: bad vnum %d.", vnum);
		raise(SIGABRT);
        return NULL;
	}

	return NULL;
}



/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index(long vnum)
{
	OBJ_INDEX_DATA *obj_idx;

	for (obj_idx = obj_index_hash[vnum % MAX_KEY_HASH];
	     obj_idx != NULL;
	     obj_idx = obj_idx->next)
		if (obj_idx->vnum == vnum)
			return obj_idx;

	if (db_loading) {
		bug_long("Get_obj_index: bad vnum %d.", vnum);
		raise(SIGABRT);
        return NULL;
	}

	return NULL;
}



/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index(long vnum)
{
	ROOM_INDEX_DATA *room_idx;

	for (room_idx = room_index_hash[vnum % MAX_KEY_HASH];
	     room_idx != NULL;
	     room_idx = room_idx->next)
		if (room_idx->vnum == vnum)
			return room_idx;

	if (db_loading) {
		bug_long("Get_room_index: bad vnum %d.", vnum);
		raise(SIGABRT);
        return NULL;
	}

	return NULL;
}

MPROG_CODE *get_mprog_index(long vnum)
{
	MPROG_CODE *prg;

	for (prg = mprog_list; prg; prg = prg->next)
		if (prg->vnum == vnum)
			return prg;
	return NULL;
}








/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *fread_string(FILE *fp)
{
	char *plast;
	char c;

	plast = top_string + sizeof(char *);

	if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
		bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
		raise(SIGABRT);
        return NULL;
	}

/*
 * Skip blanks.
 * Read first char.
 */
	do
		c = (char)getc(fp);
	while (is_space(c));

	if ((*plast++ = c) == '~')
		return &str_empty[0];

	for (;; ) {
		/*
		 * Back off the char type lookup,
		 *   it was too dirty for portability.
		 *   -- Furey
		 */

		switch (*plast = (char)getc(fp)) {
		default:
			plast++;
			break;

		case EOF:
			/* temp fix */
			bug("Fread_string: EOF", 0);
			return NULL;

		case '\n':
			plast++;
			*plast++ = '\r';
			break;

		case '\r':
			break;

		case '~':
			plast++;
			{
				union {
					char *	pc;
					char	rgc[sizeof(char *)];
				}
				u1;
				int ic;
				int hash_idx;
				char *pHash;
				char *pHashPrev;
				char *pString;

				plast[-1] = '\0';
				hash_idx = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
				for (pHash = string_hash[hash_idx]; pHash; pHash = pHashPrev) {
					for (ic = 0; ic < (int)sizeof(char *); ic++)

						u1.rgc[ic] = pHash[ic];
					pHashPrev = u1.pc;
					pHash += sizeof(char *);

					if (top_string[sizeof(char *)] == pHash[0]
					    && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
						return pHash;
				}

				if (db_loading) {
					pString = top_string;
					top_string = plast;
					u1.pc = string_hash[hash_idx];
					for (ic = 0; ic < (int)sizeof(char *); ic++)

						pString[ic] = u1.rgc[ic];
					string_hash[hash_idx] = pString;

					nAllocString += 1;
					sAllocString += top_string - pString;
					return pString + sizeof(char *);
				} else {
					return str_dup(top_string + sizeof(char *));
				}
			}
		}
	}
}

char *fread_string_eol(FILE *fp)
{
	static bool char_special[256 - EOF];
	char *plast;
	char c;

	if (char_special[EOF - EOF] != true) {
		char_special[EOF - EOF] = true;
		char_special[(int)'\n' - EOF] = true;
		char_special[(int)'\r' - EOF] = true;
	}

	plast = top_string + sizeof(char *);

	if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
		bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
		raise(SIGABRT);
        return NULL;
	}

/*
 * Skip blanks.
 * Read first char.
 */
	do
		c = (char)getc(fp);
	while (is_space(c));

	if ((*plast++ = c) == '\n')
		return &str_empty[0];

	for (;; ) {
		if (!char_special[(int)(*plast++ = (char)getc(fp)) - EOF])
			continue;

		switch (plast[-1]) {
		default:
			break;

		case EOF:
			bug("Fread_string_eol  EOF", 0);
			/* intentionally deref a null to generate core file. */
			*((char *)NULL) = 'a';
			break;

		case '\n':
		case '\r':
		{
			union {
				char *	pc;
				char	rgc[sizeof(char *)];
			}
			u1;
			int ic;
			int hash_idx;
			char *pHash;
			char *pHashPrev;
			char *pString;

			plast[-1] = '\0';
			hash_idx = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
			for (pHash = string_hash[hash_idx]; pHash; pHash = pHashPrev) {
				for (ic = 0; ic < (int)sizeof(char *); ic++)

					u1.rgc[ic] = pHash[ic];
				pHashPrev = u1.pc;
				pHash += sizeof(char *);

				if (top_string[sizeof(char *)] == pHash[0]
				    && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
					return pHash;
			}

			if (db_loading) {
				pString = top_string;
				top_string = plast;
				u1.pc = string_hash[hash_idx];
				for (ic = 0; ic < (int)sizeof(char *); ic++)

					pString[ic] = u1.rgc[ic];
				string_hash[hash_idx] = pString;

				nAllocString += 1;
				sAllocString += top_string - pString;
				return pString + sizeof(char *);
			} else {
				return str_dup(top_string + sizeof(char *));
			}
		}
		}
	}
}



/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *alloc_mem(unsigned int sMem)
{
	void *pMem;
	int iList;

	int *magic;

	sMem += sizeof(*magic);

	for (iList = 0; iList < MAX_MEM_LIST; iList++)
		if ((unsigned int)sMem <= rgSizeList[iList])
			break;

	if (iList == MAX_MEM_LIST) {
		bug_long("Alloc_mem: size %d too large.", (long)sMem);
		raise(SIGABRT);
        return NULL;
	}

	if (rgFreeList[iList] == NULL) {
		pMem = alloc_perm(rgSizeList[iList]);
	} else {
		pMem = rgFreeList[iList];
		rgFreeList[iList] = *((void **)rgFreeList[iList]);
	}

	magic = (int *)pMem;
	*magic = MAGIC_NUM;
	pMem += sizeof(*magic);

	return pMem;
}



/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem(void *pMem, unsigned int sMem)
{
	int iList;

	int *magic;

	pMem -= sizeof(*magic);
	magic = (int *)pMem;

	if (*magic != MAGIC_NUM) {
		bug_long("Attempt to recyle invalid memory of size %d.", (long)sMem);
		bug((char *)pMem + sizeof(*magic), 0);
		return;
	}

	*magic = 0;
	sMem += sizeof(*magic);

	for (iList = 0; iList < MAX_MEM_LIST; iList++)
		if (sMem <= rgSizeList[iList])
			break;

	if (iList == MAX_MEM_LIST) {
		bug_long("Free_mem: size %d too large.", (long)sMem);
		raise(SIGABRT);
        return;
	}

	*((void **)pMem) = rgFreeList[iList];
	rgFreeList[iList] = pMem;

	return;
}


/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm(unsigned int sMem)
{
	static char *pMemPerm;
	static int iMemPerm;
	void *pMem;

	while (sMem % sizeof(long) != 0)
		sMem++;

	if (sMem > MAX_PERM_BLOCK) {
		bug_long("Alloc_perm: %d too large.", (long)sMem);
		raise(SIGABRT);
        return NULL;
	}

	if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK) {
		iMemPerm = 0;
		if ((pMemPerm = calloc(1, MAX_PERM_BLOCK)) == NULL) {
			perror("Alloc_perm");
			raise(SIGABRT);
            return NULL;
		}
	}

	pMem = pMemPerm + iMemPerm;
	iMemPerm += sMem;
	nAllocPerm += 1;
	sAllocPerm += sMem;
	return pMem;
}



/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup(const char *str)
{
	char *str_new;

	if (str[0] == '\0')
		return &str_empty[0];

	if (str >= string_space && str < top_string)
		return (char *)str;

	str_new = alloc_mem((unsigned int)(strlen(str) + 1));
	strcpy(str_new, str);
	return str_new;
}



/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char *pstr)
{
	if (pstr == NULL
	    || pstr == &str_empty[0]
	    || (pstr >= string_space && pstr < top_string))
		return;

	free_mem(pstr, (unsigned int)(strlen(pstr) + 1));
	return;
}



void do_areas(CHAR_DATA *ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	AREA_DATA *area1;
	AREA_DATA *area2;
	int iArea;
	int iAreaHalf;

	if (argument[0] != '\0') {
		send_to_char("No argument is used with this command.\n\r", ch);
		return;
	}

	iAreaHalf = (top_area + 1) / 2;
	area1 = area_first;
	area2 = area_first;
	for (iArea = 0; iArea < iAreaHalf; iArea++)
		area2 = area2->next;

	for (iArea = 0; iArea < iAreaHalf; iArea++) {
		sprintf(buf, "%-39s%-39s\n\r",
			area1->credits, (area2 != NULL) ? area2->credits : "");
		send_to_char(buf, ch);
		area1 = area1->next;
		if (area2 != NULL)
			area2 = area2->next;
	}

	return;
}



void do_memory(CHAR_DATA *ch, char *argument)
{
	printf_to_char(ch, "Affects %5d\n\r", top_affect);
	printf_to_char(ch, "Areas   %5d\n\r", top_area);
	printf_to_char(ch, "ExDes   %5d\n\r", top_ed);
	printf_to_char(ch, "Exits   %5d\n\r", top_exit);
	printf_to_char(ch, "Helps   %5d\n\r", top_help);
	printf_to_char(ch, "Mobs    %5d(%d new format)\n\r", top_mob_index, newmobs);
	printf_to_char(ch, "(in use)%5d\n\r", mobile_count);
	printf_to_char(ch, "Objs    %5d(%d new format)\n\r", top_obj_index, newobjs);
	printf_to_char(ch, "Resets  %5d\n\r", top_reset);
	printf_to_char(ch, "Rooms   %5d\n\r", top_room);
	printf_to_char(ch, "Shops   %5d\n\r", top_shop);

	printf_to_char(ch, "Strings %5d strings of %7d bytes(max %d).\n\r",
		       nAllocString,
		       sAllocString,
		       MAX_STRING);

	printf_to_char(ch, "Perms   %5d blocks  of %7d bytes.\n\r",
		       nAllocPerm,
		       sAllocPerm);
}

void do_dump(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *fch;
	MOB_INDEX_DATA *mob_idx;
	PC_DATA *pc;
	OBJ_DATA *obj;
	OBJ_INDEX_DATA *obj_idx;
	DESCRIPTOR_DATA *d;
	AFFECT_DATA *af;
	FILE *fp;
	long count;
	long count_free;
	long num_pcs;
	long aff_count;
	long vnum;
	long match;

	/* start memory dump */
	fp = fopen("mem.dmp", "w");

	num_pcs = 0;
	aff_count = 0;
	match = 0;

	/* mobile prototypes */
	fprintf(fp, "MobProt	%10ld(%12ld bytes)\n",
		top_mob_index,
		top_mob_index * (long)(sizeof(*mob_idx)));

	/* mobs */
	count = 0;
	count_free = 0;
	for (fch = char_list; fch != NULL; fch = fch->next) {
		count++;
		if (fch->pcdata != NULL)
			num_pcs++;
		for (af = fch->affected; af != NULL; af = af->next)
			aff_count++;
	}
	for (fch = char_free; fch != NULL; fch = fch->next)
		count_free++;

	fprintf(fp, "Mobs	%10ld(%12ld bytes), %10ld free(%ld bytes)\n",
		count,
		(long)count * (long)(sizeof(*fch)),
		count_free,
		(long)count_free * (long)(sizeof(*fch)));

	/* pcdata */
	count = 0;
	for (pc = pcdata_free; pc != NULL; pc = pc->next)
		count++;

	fprintf(fp, "Pcdata	%10ld(%12ld bytes), %10ld free(%12ld bytes)\n",
		num_pcs,
		(long)num_pcs * (long)(sizeof(*pc)),
		count,
		(long)count * (long)(sizeof(*pc)));

	/* descriptors */
	count = descriptor_list_count();
    count_free = descriptor_recycle_count();

	fprintf(fp, "Descs	%10ld(%12ld bytes), %10ld free(%12ld bytes)\n",
		count,
		(long)count * (long)(sizeof(*d)),
		count_free,
		(long)count_free * (long)(sizeof(*d)));

	/* object prototypes */
	for (vnum = 0; match < top_obj_index; vnum++) {
		if ((obj_idx = get_obj_index(vnum)) != NULL) {
			for (af = obj_idx->affected; af != NULL; af = af->next)
				aff_count++;

			match++;
		}
	}

	fprintf(fp, "ObjProt	%10ld(%12ld bytes)\n", top_obj_index, top_obj_index * (long)(sizeof(*obj_idx)));

	/* objects */
	count = object_list_count();
	count_free = object_recycle_count();

	fprintf(fp, "Objs	%10ld(%12ld bytes), %10ld free(%12ld bytes)\n",
		count,
		(long)count * (long)(sizeof(*obj)),
		count_free,
		(long)count_free * (long)(sizeof(*obj)));

	/* affects */
	count = 0;
	for (af = affect_free; af != NULL; af = af->next)
		count++;

	fprintf(fp, "Affects	%10ld(%12ld bytes), %10ld free(%12ld bytes)\n",
		aff_count, (long)aff_count * (long)(sizeof(*af)), count, (long)count * (long)(sizeof(*af)));

	/* rooms */
	fprintf(fp, "Rooms	%10ld(%12ld bytes)\n", top_room, top_room * (long)(sizeof(ROOM_INDEX_DATA *)));

	/* exits */
	fprintf(fp, "Exits	%10ld(%12ld bytes)\n", top_exit, top_exit * (long)(sizeof(EXIT_DATA *)));

	fclose(fp);
	/* end memory dump */


	/* start mob dump */
	fp = fopen("mob.dmp", "w");
	fprintf(fp, "\nMobile Analysis\n");
	fprintf(fp, "---------------\n");
	match = 0;
	for (vnum = 0; match < top_mob_index; vnum++) {
		if ((mob_idx = get_mob_index(vnum)) != NULL) {
			match++;
			fprintf(fp, "#%-10ld %3d active %3d killed     %s\n",
				mob_idx->vnum,
				mob_idx->count,
				mob_idx->killed,
				mob_idx->short_descr);
		}
	}
	fclose(fp);
	/* end of mob dump */


	/* start object dump */
	fp = fopen("obj.dmp", "w");
	fprintf(fp, "\nObject Analysis\n");
	fprintf(fp, "---------------\n");
	match = 0;
	for (vnum = 0; match < top_obj_index; vnum++) {
		if ((obj_idx = get_obj_index(vnum)) != NULL) {
			match++;
			fprintf(fp, "#%-10ld %3d active %3ld reset      %s\n",
				obj_idx->vnum,
				obj_idx->count,
				obj_idx->reset_num,
				obj_idx->short_descr);
		}
	}
	fclose(fp);
	/* end of object dump */
}


/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32)
{
	return value_00 + level * (value_32 - value_00) / 32;
}


/*
 * Append a string to a file.
 */
void append_file(CHAR_DATA *ch, char *file, char *str)
{
	FILE *fp;

	if (IS_NPC(ch) || str[0] == '\0')
		return;

	if ((fp = fopen(file, "a")) == NULL) {
		perror(file);
		send_to_char("Could not open the file!\n\r", ch);
	} else {
		fprintf(fp, "[%5ld] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str);
		fclose(fp);
	}
}

/*
 * Reports a bug.
 */
void bug(const char *str, int param)
{
    int iLine;
    long iChar;

    if (fp_area == NULL)
        return;

    iChar = ftell(fp_area);
    fseek(fp_area, 0, 0);
    for (iLine = 0; ftell(fp_area) < iChar; iLine++)
        while ((char)getc(fp_area) != '\n');
    fseek(fp_area, iChar, 0);

    log_string("[*****] BUG IN FILE: %s LINE: %d", area_file, iLine);
    log_string(str, param);
}

void bug_long(const char *str, long param)
{
    int iLine;
    long iChar;

    if (fp_area == NULL)
        return;

    iChar = ftell(fp_area);
    fseek(fp_area, 0, 0);
    for (iLine = 0; ftell(fp_area) < iChar; iLine++) {
        while ((char)getc(fp_area) != '\n') {
        }
    }
    fseek(fp_area, iChar, 0);

    log_string("[*****] BUG IN FILE: %s LINE: %d", area_file, iLine);
    log_string(str, param);
}




/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain(void)
{
	return;
}

char *fread_norm_string(FILE *fp)
{
	char *strptr;
	char c;
	char strbuf[MAX_STRING_LENGTH + 50];
	int i = 0;

	do
		c = (char)getc(fp);
	while (is_space(c));

	if (c == '~')
		return NULL;

	strbuf[i++] = c;

	while (i < (MAX_STRING_LENGTH + 20)) {
		/*
		 * Back off the char type lookup,
		 *   it was too dirty for portability.
		 *   -- Furey
		 */

		switch (strbuf[i++] = (char)getc(fp)) {
		default:
			break;
		case EOF:
			/* temp fix */
			bug("Fread_norm_String: EOF", 0);
			strbuf[--i] = '\0';
			return NULL;
		case '\n':
			strbuf[i++] = '\r';
			break;
		case '\r':
			i--;
			break;
		case '~':
			strbuf[--i] = '\0';
			strptr = strdup(strbuf);
			return strptr;
		}
	}
	log_string("String overflow: Fread_Norm_String");
	return NULL;
}
