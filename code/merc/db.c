#include <sys/resource.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libfile.h"
#include "merc.h"
#include "object.h"
#include "db.h"
#include "recycle.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"
#include "olc.h"
#include "skills.h"
#include "channels.h"
#include "help.h"
#include <stdarg.h>
#ifndef S_SPLINT_S
#include <ctype.h>
#endif



/** exports */
char str_empty[1];
struct char_data *char_list;
struct note_data *note_list;
struct mprog_code *mprog_list;
struct shop_data *shop_first;
struct shop_data *shop_last;
struct note_data *note_free;
/*@observer@*/const char *help_greeting;
struct mob_index_data *mob_index_hash[MAX_KEY_HASH];
struct roomtemplate *room_index_hash[MAX_KEY_HASH];
int top_affect;
int top_ed;
long top_exit;
long top_mob_index;
int top_reset;
long top_room;
int top_shop;
long top_vnum_room;
long top_vnum_mob;
long top_vnum_obj;
long top_mprog_index;
int mobile_count = 0;


/** imports */
extern long flag_lookup(const char *word, const struct flag_type *flag_table);
extern int _filbuf(FILE *);
extern void init_mm(void);
extern unsigned int fread_uint(FILE *fp);
extern long fread_long(FILE *fp);
extern struct gameobject *obj_free;
extern struct char_data *char_free;
extern struct pc_data *pcdata_free;
extern struct affect_data *affect_free;

extern void load_threads(void);
extern void load_bans(void);
extern void resolve_global_skills(void);


/** locals */
static char *string_hash[MAX_KEY_HASH];
static char *string_space;
static char *top_string;
static void bug(FILE *fparea, const char *fmt, ...);


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
static char area_file_path[MAX_INPUT_LENGTH];

/***************************************************************************
 *	local functions used in boot process
 ***************************************************************************/
static /*@shared@*/struct area_data *load_area(/*@observer@*/const struct database_controller *db, const char *filename);
static void load_template(/*@observer@*/const struct database_controller *db, /*@shared@*/struct area_data *area);

static void load_helps(const char const *filepath);
static void load_mobiles(FILE * fp, /*@shared@*/struct area_data *area);
static void load_room(const struct database_controller *db, /*@shared@*/struct area_data *area);
static void load_shops(FILE * fp, /*@shared@*/struct area_data *area);
static void load_mobprogs(FILE * fp, /*@shared@*/struct area_data *area);


static void fix_exits(void);
static void fix_mobprogs(void);
static void reset_areas(void);
static void assign_area_vnum(long vnum, struct area_data *area);


/* RT max open files fix */
static void maxfilelimit()
{
    struct rlimit r;

    getrlimit(RLIMIT_NOFILE, &r);
    r.rlim_cur = r.rlim_max;
    setrlimit(RLIMIT_NOFILE, &r);
}




/** initialize the global time structures */
static void init_game_time()
{
    long lhour;
    long lday;
    long lmonth;

    lhour = (long)((globalSystemState.current_time - 650336715l) / (PULSE_TICK / PULSE_PER_SECOND));
    globalGameState.gametime->hour = (int)(lhour % 24l);
    lday = (int)(lhour / 24);
    globalGameState.gametime->day = (int)(lday % 35l);
    lmonth = lday / 35l;
    globalGameState.gametime->month = (int)(lmonth % 17l);
    globalGameState.gametime->year = (int)(lmonth / 17l);

    if (globalGameState.gametime->hour < 5)
        globalGameState.weather->sunlight = SUN_DARK;
    else if (globalGameState.gametime->hour < 6)
        globalGameState.weather->sunlight = SUN_RISE;
    else if (globalGameState.gametime->hour < 19)
        globalGameState.weather->sunlight = SUN_LIGHT;
    else if (globalGameState.gametime->hour < 20)
        globalGameState.weather->sunlight = SUN_SET;
    else
        globalGameState.weather->sunlight = SUN_DARK;

    globalGameState.weather->change = 0;
    globalGameState.weather->mmhg = 960;

    if (globalGameState.gametime->month >= 7 && globalGameState.gametime->month <= 12)
        globalGameState.weather->mmhg += number_range(1, 50);
    else
        globalGameState.weather->mmhg += number_range(1, 80);

    if (globalGameState.weather->mmhg <= 980)
        globalGameState.weather->sky = SKY_LIGHTNING;
    else if (globalGameState.weather->mmhg <= 1000)
        globalGameState.weather->sky = SKY_RAINING;
    else if (globalGameState.weather->mmhg <= 1020)
        globalGameState.weather->sky = SKY_CLOUDY;
    else
        globalGameState.weather->sky = SKY_CLOUDLESS;
}


/** load the area information */
static void init_areas()
{
    FILE *fpList;
    char *word;
    char area_file_name[MAX_INPUT_LENGTH];
    struct database_controller *db;
    struct area_data *area_loading;

    log_string("Opening area file.");
    fpList = fopen(AREA_LIST, "r");
    if (fpList == NULL) {
        perror(AREA_LIST);
        log_bug("Unable to open area list (%s)", AREA_LIST);
        ABORT;
    }


    for (;;) {
        word = fread_word(fpList);
        if (word[0] == '$') {
            /** End of Area List. */
            break;
        }
        (void)snprintf(area_file_path, MAX_INPUT_LENGTH, "%s%s", AREA_FOLDER, word);
        (void)snprintf(area_file_name, MAX_INPUT_LENGTH, "%s", word);

        db = database_open(area_file_path, true);
        if (db == NULL) {
            perror(area_file_path);
            log_bug("Unable to open area file (%s)", area_file_path);
            ABORT;
        }

        for (;; ) {
            char token;

            token = fread_letter(db->_cfptr);
            if (token != '#') {
                bug(db->_cfptr, "Boot_db: Found %c instead of expected header token # in area_file_path %s.", token, area_file_path);
                ABORT;
            }

            word = fread_word(db->_cfptr);

            if (word[0] == '$') {
                /** End of Area File definition. */
                break;
            } else if (!str_cmp(word, "AREADATA")) {
                area_loading = load_area(db, area_file_name);
            } else {
                if (area_loading == NULL) {
                    bug(db->_cfptr, "Load %s: no #AREADATA seen yet.", word);
                    ABORT;
                }

                if (!str_cmp(word, "MOBILES")) {
                    load_mobiles(db->_cfptr, area_loading);
                } else if (!str_cmp(word, "PROGRAMS")) {
                    load_mobprogs(db->_cfptr, area_loading);
                } else if (!str_cmp(word, "ROOM")) {
                    load_room(db, area_loading);
                } else if (!str_cmp(word, "OBJECT")) {
                    load_template(db, area_loading);
                } else if (!str_cmp(word, "SHOPS")) {
                    load_shops(db->_cfptr, area_loading);
                } else {
                    bug(db->_cfptr, "Boot_db: bad section name (%s)", word);
                    ABORT;
                }
            }
        }

        database_close(db);
        db = NULL;
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
        bug(NULL, "Boot_db: can't alloc %d string space.", MAX_STRING);
        ABORT;
    }

    top_string = string_space;
    db_loading = true;

    /* init random number generator */
    log_string("Initializing environment..");
    init_mm();
    init_game_time();


    /* load skills */
    log_string("Loading Skills..");
    load_skills();
    load_groups();
    resolve_global_skills();

    log_string("Loading Areas..");
    init_areas();


    log_string("Loading helps.");
    load_helps(HELP_FILE);

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
    log_string("Loading Bans..");
    load_bans();

    log_string("BootDB: Done..");

    if (!help_greeting) {            /* Spacey */
        log_bug("boot_db: No help_greeting read.");
        help_greeting = "By what name do you wish to be known ? ";
    }

    return;
}




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



struct area_data *load_area(const struct database_controller *db, const char *filename)
{
    struct array_list *data;
    char *dbstream;
    struct area_data *area;

    dbstream = database_read_stream(db);
    data = database_parse_stream(dbstream);
    free(dbstream);
    area = area_deserialize(data, filename);
    kvp_free_array(data);

    return area;
}

void load_template(const struct database_controller *db, struct area_data *area)
{
    struct array_list *data;
    char *dbstream;
    struct objecttemplate *template;
    long vnum;

    dbstream = database_read_stream(db);
    data = database_parse_stream(dbstream);
    free(dbstream);
    template = objecttemplate_deserialize(data);
    kvp_free_array(data);

    template->area = area;

    vnum = template->vnum;
    top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
    assign_area_vnum(vnum, area);
}

/*
 * Snarf a room section.
 */
void load_room(const struct database_controller *db, struct area_data *area)
{
    struct array_list *data;
    char *dbstream;
    struct roomtemplate *template;
    long vnum;

    dbstream = database_read_stream(db);
    data = database_parse_stream(dbstream);
    free(dbstream);
    template = roomtemplate_deserialize(data);
    kvp_free_array(data);

    template->area = area;
    vnum = template->vnum;

    top_room++;
    top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
    assign_area_vnum(vnum, area);

    return;
}


/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum(long vnum, struct area_data *area)
{
    if (area->min_vnum == 0 || area->max_vnum == 0)
        area->min_vnum = area->max_vnum = vnum;

    if (vnum != URANGE(area->min_vnum, vnum, area->max_vnum)) {
        if (vnum < area->min_vnum)
            area->min_vnum = vnum;
        else
            area->max_vnum = vnum;
    }

    return;
}

void load_helps(const char const *filepath)
{
    struct array_list *data;
    struct database_controller *db;

    db = database_open(filepath, true);

    if (db == NULL) {
        log_bug("Unable to open database at %s.", filepath);
        ABORT;
        return;
    }

    while (true) {
        struct help_data *snarfed;
        char *dbstream;

        dbstream = database_read_stream(db);
        data = database_parse_stream(dbstream);
        free (dbstream);
        if (!array_list_any(data)) {
            kvp_free_array(data);
            break;
        }

        snarfed = helpdata_deserialize(data);
        kvp_free_array(data);

        if (!str_cmp(snarfed->keyword, "greeting")) {
            help_greeting = snarfed->text;
        }
    }

    database_close(db);
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
void load_shops(FILE *fp, struct area_data *area)
{
    struct shop_data *shop;

    for (;; ) {
        struct mob_index_data *mob_idx;
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
void load_mobiles(FILE *fp, struct area_data *area)
{
    struct mob_index_data *mob_idx;

    for (;; ) {
        long vnum;
        char letter;
        long hash_idx;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug(fp, "Load_mobiles: # not found.");
            ABORT;
        }

        vnum = (long)fread_number(fp);
        if (vnum == 0)
            break;

        db_loading = false;
        if (get_mob_index(vnum) != NULL) {
            bug(fp, "Load_mobiles: vnum %ld duplicated.", vnum);
            ABORT;
        }
        db_loading = true;

        mob_idx = alloc_perm((unsigned int)sizeof(*mob_idx));
        mob_idx->vnum = vnum;
        mob_idx->area = area;

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
                    bug(fp, "Flag remove: flag not found.");
                    ABORT;
                }
            } else if (letter == 'M') {
                struct mprog_list *mprog;
                char *word;
                int trigger = 0;

                mprog = new_mprog();
                word = fread_word(fp);

                if ((trigger = (int)flag_lookup(word, mprog_flags)) == NO_FLAG) {
                    bug(fp, "MOBprogs: invalid trigger.");
                    ABORT;
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
        assign_area_vnum(vnum, area);
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
    struct roomtemplate *room_idx;
    struct roomtemplate *to_room;
    struct exit_data *pexit;
    struct exit_data *pexit_rev;
    struct reset_data *reset;
    struct roomtemplate *iLastRoom;
    struct roomtemplate *ilast_obj;
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
                      log_bug("fix_exits : room %d with reset cmd %c", room_idx->vnum, reset->command);
                      ABORT;

                  case 'M':
                      get_mob_index(reset->arg1);
                      iLastRoom = get_room_index(reset->arg3);
                      break;

                  case 'O':
                      if (objecttemplate_getbyvnum(reset->arg1) == NULL) {
                          log_bug("Load room reset: bad vnum %ld.", reset->arg1);
                      }
                      ilast_obj = get_room_index(reset->arg3);
                      break;

                  case 'P':
                      if (objecttemplate_getbyvnum(reset->arg1) == NULL) {
                          log_bug("Load room reset: bad vnum %ld.", reset->arg1);
                      }
                      if (ilast_obj == NULL) {
                          log_bug("fix_exits : reset in room %d con ilast_obj NULL", room_idx->vnum);
                          ABORT;
                      }
                      break;

                  case 'G':
                  case 'E':
                      if (objecttemplate_getbyvnum(reset->arg1) == NULL) {
                          log_bug("Load room reset: bad vnum %ld.", reset->arg1);
                      }
                      if (iLastRoom == NULL) {
                          log_bug("fix_exits : reset in room %d with iLastRoom NULL", room_idx->vnum);
                          ABORT;
                      }
                      ilast_obj = iLastRoom;
                      break;

                  case 'D':
                      break;

                  case 'R':
                      get_room_index(reset->arg1);
                      if (reset->arg2 < 0 || reset->arg2 > MAX_DIR) {
                          log_bug("fix_exits : reset in room %d with arg2 %d >= MAX_DIR", room_idx->vnum, reset->arg2);
                          ABORT;
                      }
                      break;
                }       /* switch */
            }               /* for */

            fexit = false;
            for (door = 0; door <= 5; door++) {
                if ((pexit = room_idx->exit[door]) != NULL) {
                    if (pexit->vnum <= 0
                        || get_room_index(pexit->vnum) == NULL) {
                        pexit->to_room = NULL;
                    } else {
                        fexit = true;
                        pexit->to_room = get_room_index(pexit->vnum);
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
                    && (to_room = pexit->to_room) != NULL
                    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
                    && pexit_rev->to_room != room_idx
                    && (room_idx->vnum < 1200 || room_idx->vnum > 1299)
                    && (room_idx->vnum < 5700 && room_idx->vnum > 5799)) {
                    log_bug("Fix_exits: %d:%d -> %d:%d -> %d.",
                        room_idx->vnum, door,
                        to_room->vnum, rev_dir[door],
                        (pexit_rev->to_room == NULL) ? 0 : pexit_rev->to_room->vnum);
                }
            }
        }
    }

    return;
}



void load_mobprogs(FILE *fp, struct area_data *area)
{
    struct mprog_code *mprog;

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
                bug(fp, "load_mobprogs: vnum %d duplicated.", vnum);
                ABORT;
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
    struct mob_index_data *mob_idx;
    struct mprog_list *list;
    struct mprog_code *prog;
    int hash_idx;

    for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
        for (mob_idx = mob_index_hash[hash_idx];
             mob_idx != NULL;
             mob_idx = mob_idx->next) {
            for (list = mob_idx->mprogs; list != NULL; list = list->next) {
                if ((prog = get_mprog_index(list->vnum)) != NULL) {
                    list->code = prog->code;
                } else {
                    log_bug("fix_mobprogs: code vnum %ld not found.", list->vnum);
                    ABORT;
                }
            }
        }
    }
}

void reset_areas()
{
    struct area_data *iterator;

    iterator = area_iterator_start(NULL);
    while (iterator != NULL) {
        reset_area(iterator);
        iterator = area_iterator(iterator, NULL);
    }
}

/*
 * Repopulate areas periodically.
 */
void area_update(void)
{
    struct area_data *area;
    char buf[MAX_STRING_LENGTH];

    area = area_iterator_start(NULL);
    while (area != NULL) {
        if (++area->age > 3) {
            /*
             * Check age and reset.
             * Note: Mud School resets every 3 minutes(not 15).
             */
            if ((!area->empty && (area->nplayer == 0 || area->age >= 15)) || area->age >= 31) {
                struct roomtemplate *room_idx;

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

        area = area_iterator(area, NULL);
    }
}


/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(struct roomtemplate *room)
{
    struct reset_data *reset;
    struct char_data *mob_it;
    struct char_data *mob = NULL;
    struct gameobject *obj = NULL;
    struct char_data *last_mob = NULL;
    struct gameobject *last_obj = NULL;
    int exit_dir;
    bool last;

    if (!room)
        return;

    mob = NULL;
    last = false;
    for (exit_dir = 0; exit_dir < MAX_DIR; exit_dir++) {
        struct exit_data *exit;
        if ((exit = room->exit[exit_dir])) {
            exit->exit_info = (int)exit->rs_flags;

            if ((exit->to_room != NULL)
                && ((exit = exit->to_room->exit[rev_dir[exit_dir]])))
                exit->exit_info = (int)exit->rs_flags;
        }
    }

    for (reset = room->reset_first; reset != NULL; reset = reset->next) {
        struct mob_index_data *mob_idx;
        struct objecttemplate *objtemplate;
        struct objecttemplate *obj_to_idx;
        struct roomtemplate *room_idx;
        int count;
        int limit = 0;

        switch (reset->command) {
          default:
              log_bug("Reset_room: bad command %c.", (int)reset->command);
              break;

          case 'M':
              if (!(mob_idx = get_mob_index(reset->arg1))) {
                  log_bug("Reset_room: 'M': bad vnum %ld.", reset->arg1);
                  continue;
              }

              if ((room_idx = get_room_index(reset->arg3)) == NULL) {
                  log_bug("Reset_area: 'R': bad vnum %ld.", reset->arg3);
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
                  struct roomtemplate *room_idxPrev;

                  room_idxPrev = get_room_index(room->vnum - 1);
                  if (room_idxPrev && IS_SET(room_idxPrev->room_flags, ROOM_PET_SHOP))
                      SET_BIT(mob->act, ACT_PET);
              }

              char_to_room(mob, room);

              last_mob = mob;
              last = true;
              break;

          case 'O':
              if (!(objtemplate = objecttemplate_getbyvnum(reset->arg1))) {
                  log_bug("Reset_room: 'O' 1 : bad vnum %ld %d %ld %d", reset->arg1, reset->arg2, reset->arg3, reset->arg4);
                  continue;
              }

              if (!(room_idx = get_room_index(reset->arg3))) {
                  log_bug("Reset_room: 'O' 3 : bad vnum %ld %d %ld %d", reset->arg1, reset->arg2, reset->arg3, reset->arg4);
                  continue;
              }

              /*
               * this was causing problems for the 'repop' command
               * and the builders trying to use it to test their
               * resets -- i just took out the testing for players
               * in the area because i dont really care about that...
               */
              /*if(room->area->nplayer > 0
                || count_obj_list(objtemplate, room->contents) > 0)*/
              if (count_obj_list(objtemplate, room->contents) > 0) {
                  last = false;
                  break;
              }

              obj = create_object(objtemplate);
              obj->cost = 0;
              obj_to_room(obj, room);
              last = true;
              break;

          case 'P':
              if (!(objtemplate = objecttemplate_getbyvnum(reset->arg1))) {
                  log_bug("Reset_room: 'P': bad vnum %ld.", reset->arg1);
                  continue;
              }

              if (!(obj_to_idx = objecttemplate_getbyvnum(reset->arg3))) {
                  log_bug("Reset_room: 'P': bad vnum %ld.", reset->arg3);
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
                  || (objtemplate->count >= limit && number_range(0, 2) != 0)
                  || (count = count_obj_list(objtemplate, last_obj->contains)) > reset->arg4) {
                  last = false;
                  break;
              }

              while (count < reset->arg4) {
                  obj = create_object(objtemplate);
                  obj_to_obj(obj, last_obj);
                  count++;
                  if (objtemplate->count >= (int)limit)
                      break;
              }

              /* fix object lock state! */
              last_obj->value[1] = last_obj->objtemplate->value[1];
              last = true;
              break;

          case 'G':
          case 'E':
              if (!(objtemplate = objecttemplate_getbyvnum(reset->arg1))) {
                  log_bug("Reset_room: 'E' or 'G': bad vnum %ld.", reset->arg1);
                  continue;
              }

              if (!last)
                  break;

              if (!last_mob) {
                  log_bug("Reset_room: 'E' or 'G': null mob for vnum %ld.", reset->arg1);
                  last = false;
                  break;
              }

              if (IS_SHOPKEEPER(last_mob)) { /* Shop-keeper? */
                  obj = create_object(objtemplate);
                  SET_BIT(obj->extra_flags, ITEM_INVENTORY);  /* ROM OLC */
              } else { /* ROM OLC else version */
                  int limit;
                  if (reset->arg2 > 50)                           /* old format */
                      limit = (int)999;
                  else if (reset->arg2 == -1 || reset->arg2 == 0) /* no limit */
                      limit = (int)999;
                  else
                      limit = (int)reset->arg2;

                  if (objtemplate->count < limit || number_range(0, 4) == 0)
                      obj = create_object(objtemplate);

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
                  log_bug("Reset_room: 'R': bad vnum %ld.", reset->arg1);
                  continue;
              }

              {
                  struct exit_data *exit;
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
void reset_area(struct area_data *area)
{
    struct roomtemplate *room;
    long vnum;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++)
        if ((room = get_room_index(vnum)))
            reset_room(room);

    return;
}


/*
 * Create an instance of a mobile.
 */
struct char_data *create_mobile(struct mob_index_data *mob_idx)
{
    struct char_data *mob;
    struct dynamic_skill *skill;
    int idx;
    struct affect_data af;

    mobile_count++;

    if (mob_idx == NULL) {
        log_bug("Create_mobile: NULL mob_idx.");
        RABORT(NULL);
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

    /* read from template */
    mob->group = mob_idx->group;
    mob->act = mob_idx->act;

    mob->channels_denied = CHANNEL_SHOUT;
    mob->affected_by = mob_idx->affected_by;
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

    mob->position = mob->start_pos;


    /* link the mob to the world list */
    mob->next = char_list;
    char_list = mob;
    mob_idx->count++;

    return mob;
}


/* duplicate a mobile exactly -- except inventory */
void clone_mobile(struct char_data *parent, struct char_data *clone)
{
    int i;
    struct affect_data *paf;

    if (parent == NULL || clone == NULL || !IS_NPC(parent))
        return;

    /* start fixing values */
    clone->name = str_dup(parent->name);
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
struct gameobject *create_object(struct objecttemplate *objtemplate)
{
    struct affect_data *paf;
    struct gameobject *obj;

    if (objtemplate == NULL) {
        log_bug("Create_object: NULL objtemplate.");
        RABORT(NULL);
    }

    obj = object_new(objtemplate);

    obj->in_room = NULL;

    obj->wear_loc = -1;

    object_name_set(obj, str_dup(objtemplate->name));
    obj->timer = objtemplate->init_timer;
    obj->extra_flags = objtemplate->extra_flags;
    obj->value[0] = objtemplate->value[0];
    obj->value[1] = objtemplate->value[1];
    obj->value[2] = objtemplate->value[2];
    obj->value[3] = objtemplate->value[3];
    obj->value[4] = objtemplate->value[4];
    obj->weight = objtemplate->weight;
    obj->cost = objtemplate->cost;

    /*
     * Mess with object properties.
     */
    switch (OBJECT_TYPE(obj)) {
      default:
          log_bug("Read_object: vnum %ld bad type.", objtemplate->vnum);
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
      case ITEM_FOOD:
      case ITEM_BOAT:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
      case ITEM_FOUNTAIN:
      case ITEM_MAP:
      case ITEM_CLOTHING:
      case ITEM_PORTAL:
      case ITEM_ATM:
      case ITEM_TELEPORT:
      case ITEM_TREASURE:
      case ITEM_WARP_STONE:
      case ITEM_GEM:
      case ITEM_JEWELRY:
      case ITEM_DOLL:
      case ITEM_DICE:
      case ITEM_SCROLL:
      case ITEM_WAND:
      case ITEM_STAFF:
      case ITEM_WEAPON:
      case ITEM_ARMOR:
      case ITEM_POTION:
      case ITEM_PILL:
      case ITEM_MONEY:
          break;
    }

    for (paf = objtemplate->affected; paf != NULL; paf = paf->next)
        if (paf->location == APPLY_SPELL_AFFECT)
            affect_to_obj(obj, paf);

    objtemplate->count++;

    return obj;
}




/*
 * Clear a new character.
 */
void clear_char(struct char_data *ch)
{
    static struct char_data ch_zero;
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
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
struct mob_index_data *get_mob_index(long vnum)
{
    struct mob_index_data *mob_idx;

    for (mob_idx = mob_index_hash[vnum % MAX_KEY_HASH];
         mob_idx != NULL;
         mob_idx = mob_idx->next)
        if (mob_idx->vnum == vnum)
            return mob_idx;

    if (db_loading) {
        log_bug("Get_mob_index: bad vnum %ld.", vnum);
        RABORT(NULL);
    }

    return NULL;
}





/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
struct roomtemplate *get_room_index(long vnum)
{
    struct roomtemplate *room_idx;

    for (room_idx = room_index_hash[vnum % MAX_KEY_HASH];
         room_idx != NULL;
         room_idx = room_idx->next)
        if (room_idx->vnum == vnum)
            return room_idx;

    if (db_loading) {
        log_bug("Get_room_index: bad vnum %ld.", vnum);
        RABORT(NULL);
    }

    return NULL;
}

struct mprog_code *get_mprog_index(long vnum)
{
    struct mprog_code *prg;

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
        bug(fp, "Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        RABORT(NULL);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
        c = (char)getc(fp);
    while (isspace((int)c));

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
              bug(fp, "Fread_string: EOF");
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
        bug(fp, "Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        RABORT(NULL);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
        c = (char)getc(fp);
    while (isspace((int)c));

    if ((*plast++ = c) == '\n')
        return &str_empty[0];

    for (;; ) {
        if (!char_special[(int)(*plast++ = (char)getc(fp)) - EOF])
            continue;

        switch (plast[-1]) {
          default:
              break;

          case EOF:
              bug(fp, "Fread_string_eol  EOF");
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
        log_bug("Alloc_mem: size %ld too large.", (long)sMem);
        RABORT(NULL);
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
        log_bug("Attempt to recyle invalid memory of size %d (structure size %d).", (long)sMem, (char *)pMem + sizeof(*magic));
        ABORT;
    }

    *magic = 0;
    sMem += sizeof(*magic);

    for (iList = 0; iList < MAX_MEM_LIST; iList++)
        if (sMem <= rgSizeList[iList])
            break;

    if (iList == MAX_MEM_LIST) {
        log_bug("Free_mem: size %d too large.", (long)sMem);
        ABORT;
    }

    *((void **)pMem) = rgFreeList[iList];
    rgFreeList[iList] = pMem;

    return;
}


/*
 * Allocate some permanent memory.
 * Permanent memory is never freed, pointers into it may be copied safely.
 */
void *alloc_perm(unsigned int sMem)
{
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;

    while (sMem % sizeof(long) != 0)
        sMem++;

    if (sMem > MAX_PERM_BLOCK) {
        log_bug("Alloc_perm: %ld too large.", (long)sMem);
        RABORT(NULL);
    }

    if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK) {
        iMemPerm = 0;
        if ((pMemPerm = calloc(1, MAX_PERM_BLOCK)) == NULL) {
            perror("Alloc_perm");
            RABORT(NULL);
        }
    }

    pMem = pMemPerm + iMemPerm;
    iMemPerm += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

/** Duplicate a string into memory. Fread_strings are read-only and shared. */
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




void do_memory(struct char_data *ch, const char *argument)
{
    printf_to_char(ch, "Affects %5d\n\r", top_affect);
    printf_to_char(ch, "ExDes   %5d\n\r", top_ed);
    printf_to_char(ch, "Exits   %5d\n\r", top_exit);
    printf_to_char(ch, "Helps   %5d\n\r", count_helps());
    printf_to_char(ch, "Mobs    %5d\n\r", top_mob_index);
    printf_to_char(ch, "(in use)%5d\n\r", mobile_count);
    printf_to_char(ch, "Objs    %5d\n\r", objecttemplate_list_count());
    printf_to_char(ch, "Resets  %5d\n\r", top_reset);
    printf_to_char(ch, "Rooms   %5d\n\r", top_room);
    printf_to_char(ch, "Shops   %5d\n\r", top_shop);

    printf_to_char(ch, "Strings %5d strings of %7d bytes(max %d).\n\r", nAllocString, sAllocString, MAX_STRING); 
    printf_to_char(ch, "Perms   %5d blocks  of %7d bytes.\n\r", nAllocPerm, sAllocPerm);
}

void do_dump(struct char_data *ch, const char *argument)
{
    struct char_data *fch;
    struct mob_index_data *mob_idx;
    struct pc_data *pc;
    struct gameobject *obj;
    struct descriptor_data *d;
    struct affect_data *af;
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

    /* mobile templates */
    fprintf(fp, "MobProt	%10ld(%12ld bytes)\n", top_mob_index, top_mob_index * (long)(sizeof(*mob_idx)));

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
    fprintf(fp, "Mobs	%10ld(%12ld bytes), %10ld free(%ld bytes)\n", count, (long)count * (long)(sizeof(*fch)), count_free, (long)count_free * (long)(sizeof(*fch)));

    /* pcdata */
    count = 0;
    for (pc = pcdata_free; pc != NULL; pc = pc->next)
        count++;
    fprintf(fp, "Pcdata	%10ld(%12ld bytes), %10ld free(%12ld bytes)\n", num_pcs, (long)num_pcs * (long)(sizeof(*pc)), count, (long)count * (long)(sizeof(*pc)));

    /* descriptors */
    count = descriptor_list_count();
    fprintf(fp, "Descs	%10ld(%12ld bytes)\n", count, (long)count * (long)(sizeof(*d)));

    /* objecttemplates */
    count = objecttemplate_list_count();
    fprintf(fp, "ObjProt	%10ld(%12ld bytes)\n", count, count * (long)(sizeof(struct objecttemplate)));

    /* objects */
    count = object_list_count();
    fprintf(fp, "Objs	%10ld(%12ld bytes)\n", count, (long)count * (long)(sizeof(*obj)));

    /* affects */
    count = 0;
    for (af = affect_free; af != NULL; af = af->next)
        count++;

    /* affects on object templates */
    {
        struct objecttemplate *current;
        struct objecttemplate *pending;

        pending = objecttemplate_iterator_start(&objecttemplate_empty_filter);
        while ((current = pending) != NULL) {
            pending = objecttemplate_iterator(current, &objecttemplate_empty_filter);
            for (af = current->affected; af != NULL; af = af->next)
                aff_count++;
        }
    }

    fprintf(fp, "Affects	%10ld(%12ld bytes), %10ld free(%12ld bytes)\n", aff_count, (long)aff_count * (long)(sizeof(*af)), count, (long)count * (long)(sizeof(*af)));

    /* rooms */
    fprintf(fp, "Rooms	%10ld(%12ld bytes)\n", top_room, top_room * (long)(sizeof(struct roomtemplate *)));

    /* exits */
    fprintf(fp, "Exits	%10ld(%12ld bytes)\n", top_exit, top_exit * (long)(sizeof(struct exit_data *)));

    fclose(fp);
    /* end memory dump */


    /* start mob dump */
    fp = fopen("mob.dmp", "w");
    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    for (vnum = 0; match < top_mob_index; vnum++) {
        if ((mob_idx = get_mob_index(vnum)) != NULL) {
            fprintf(fp, "#%-10ld %3d active %3d killed     %s\n", mob_idx->vnum, mob_idx->count, mob_idx->killed, mob_idx->short_descr);
        }
    }
    fclose(fp);
    /* end of mob dump */


    /* start object dump */
    {
        struct objecttemplate *current;
        struct objecttemplate *pending;

        fp = fopen("obj.dmp", "w");
        fprintf(fp, "\nObject Analysis\n");
        fprintf(fp, "---------------\n");

        pending = objecttemplate_iterator_start(&objecttemplate_empty_filter);
        while ((current = pending) != NULL) {
            pending = objecttemplate_iterator(current, &objecttemplate_empty_filter);

            fprintf(fp, "#%-10ld %3d active   %s\n", current->vnum, current->count, current->short_descr);
        }
        fclose(fp);
    }
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
 * Reports a bug.
 */
void bug(FILE *fparea, const char *fmt, ...)
{
    va_list args;
    static char buf[MAX_STRING_LENGTH];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    if (fparea == NULL) {
        log_bug("%s", buf);
    } else {
        int iLine;
        long iChar;

        iChar = ftell(fparea);
        fseek(fparea, 0, 0);
        for (iLine = 0; ftell(fparea) < iChar; iLine++)
            while ((char)getc(fparea) != '\n');
        fseek(fparea, iChar, 0);

        log_bug("[*****] BUG IN FILE: %s LINE: %d\n%s", area_file_path, iLine, buf);
    }
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
    while (isspace((int)c));

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
              bug(fp, "Fread_norm_String: EOF");
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
    bug(fp, "String overflow: Fread_Norm_String");
    return NULL;
}
