#include <time.h>
#include <stdbool.h>
#include "sysinternals.h"
#include <signal.h>

#if !defined(__MERC_H)
#define __MERC_H


#ifdef S_SPLINT_S
#define _Exit exit
#endif


#define ABORT \
{ \
    if (raise(SIGABRT) != 0) { \
	_Exit(EXIT_FAILURE); \
    } \
    return; \
}

#define RABORT(rv) \
{ \
    if (raise(SIGABRT) != 0) { \
	_Exit(EXIT_FAILURE); \
    } \
    return rv; \
}



#define DECLARE_DO_FUN(fun)             DO_FUN fun
#define DECLARE_SPELL_FUN(fun)          SPELL_FUN fun
#define DECLARE_AFFECT_FUN(fun)         AFFECT_FUN fun

/* system calls */
int unlink();
int system();

typedef int SOCKET;
/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if !defined(false)
#define false    0
#endif

#if !defined(true)
#define true     (!false)
#endif



typedef int sh_int;

typedef enum e_one_attack_result {
    oar_error = -1,
    oar_miss,
    oar_fumble,
    oar_hit,
    oar_critical_hit
} ONE_ATTACK_RESULT;


/*Structure types. */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct buf_type BUFFER;
typedef struct char_data CHAR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct kill_data KILL_DATA;
typedef struct mem_data MEM_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct note_data NOTE_DATA;
typedef struct gameobject GAMEOBJECT;
typedef struct objectprototype OBJECTPROTOTYPE;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;

typedef struct mprog_list MPROG_LIST;
typedef struct mprog_code MPROG_CODE;
typedef struct help_area_data HELP_AREA;
typedef struct system_state SYSTEM_STATE;
typedef struct game_state GAME_STATE;


/* required for new skill system */
#include "skills.h"

/* Function types. */
typedef void DO_FUN(/*@partial@*/CHAR_DATA * ch, const char *argument);


/*String and memory management parameters. */
#define MAX_KEY_HASH            65534
#define MAX_STRING_LENGTH       65534
#define MAX_INPUT_LENGTH        512
#define LOG_BUF_LENGTH          2*MAX_INPUT_LENGTH
#define PAGELEN                 22
#define MAX_TITLE_LENGTH        90
#define MAX_NAME_LENGTH         12

#define MSL             MAX_STRING_LENGTH
#define MIL             MAX_INPUT_LENGTH


/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SKILL               285
#define MAX_GROUP               35
#define MAX_ALIAS               200
#define MAX_CLASS               4
#define MAX_PC_RACE             15
#define MAX_LEVEL               610
#define MAX_GET                 30
#define MAX_IGNORE              5
#define MAX_FORGET              10
#define MIN_POINTS              76
#define LEVEL_HERO              (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL          (MAX_LEVEL - 8)
#define LEVEL_NEWBIE            11

#define PULSE_PER_SECOND        4
#define PULSE_VIOLENCE          (3 * PULSE_PER_SECOND)
#define PULSE_MOBILE            (4 * PULSE_PER_SECOND)
#define PULSE_TICK              (40 * PULSE_PER_SECOND)
#define PULSE_AREA              (120 * PULSE_PER_SECOND)
#define PULSE_ROOM              (20 * PULSE_PER_SECOND)
#define PULSE_RESTORE           (3200 * PULSE_PER_SECOND)
#define PULSE_AUCTION           (10 * PULSE_PER_SECOND)
#define PULSE_UNDERWATER        (5 * PULSE_PER_SECOND)

#define IMPLEMENTOR             MAX_LEVEL
#define GOD                     (MAX_LEVEL - 4)
#define IMMORTAL                (MAX_LEVEL - 5)
#define DEMI                    (MAX_LEVEL - 6)
#define ANGEL                   (MAX_LEVEL - 7)
#define AVATAR                  (MAX_LEVEL - 8)
#define HERO                    LEVEL_HERO

struct system_state {
    time_t current_time;            /* Time of this pulse. */
    bool tickset;                   /* Force a tick? whaat? --Eo */
    bool merc_down;                 /* Shutdown */
    char boot_time[MIL];

    bool wizlock;                   /* Game is wizlocked. */
    bool newlock;                   /* Game is newlocked. */
    bool log_all;
    char last_command[MSL];	    /* In case of failure, log this. */

    int copyover_tick_counter;      /* schedule copyover */
    int reboot_tick_counter;        /* schedule reboot */
};


struct game_state {
    WEATHER_DATA *weather;
    TIME_INFO_DATA *gametime;
};


/* Site ban structure. */

#define BAN_SUFFIX      (int)A
#define BAN_PREFIX      (int)B
#define BAN_NEWBIES     (int)C
#define BAN_ALL         (int)D
#define BAN_PERMIT      (int)E
#define BAN_PERMANENT   (int)F

struct ban_data {
    BAN_DATA * next;
    bool  valid;
    int  ban_flags;
    int  level;
    char *  name;
};

struct buf_type {
    BUFFER *next;
    bool valid;
    int state;                  /* error state of the buffer */
    long size;                   /* size in k */
    char * string;                 /* buffer's string */
};





/* Time and weather stuff. */
#define SUN_DARK        0
#define SUN_RISE        1
#define SUN_LIGHT       2
#define SUN_SET         3

#define SKY_CLOUDLESS   0
#define SKY_CLOUDY      1
#define SKY_RAINING     2
#define SKY_LIGHTNING   3

struct time_info_data {
    int hour;
    int day;
    int month;
    int year;
};

struct weather_data {
    int mmhg;
    int change;
    int sky;
    int sunlight;
};



/* Connected state for a channel. */
#define CON_PLAYING             0
#define CON_GET_NAME            1
#define CON_GET_OLD_PASSWORD    2
#define CON_CONFIRM_NEW_NAME    3
#define CON_GET_NEW_PASSWORD    4
#define CON_CONFIRM_NEW_PASSWORD        5
#define CON_GET_NEW_RACE        6
#define CON_GET_NEW_SEX         7
#define CON_GET_NEW_CLASS       8
#define CON_DEFAULT_CHOICE      10
#define CON_GEN_GROUPS          11
#define CON_PICK_WEAPON         12
#define CON_READ_IMOTD          13
#define CON_READ_MOTD           14
#define CON_BREAK_CONNECT       15
#define CON_GET_ANSI            16
#define CON_COPYOVER_RECOVER    17

/* Descriptor (channel) structure. */
struct descriptor_data {
    /*@owned@*//*@partial@*//*@null@*/DESCRIPTOR_DATA *next;
    /*@dependent@*//*@partial@*//*@null@*/DESCRIPTOR_DATA *prev;

    /*@dependent@*//*@null@*/DESCRIPTOR_DATA *snoop_by;
    /*@dependent@*//*@null@*/CHAR_DATA *character;
    /*@dependent@*//*@null@*/CHAR_DATA *original;
    bool pending_delete;
    bool pending_input;
    /*@shared@*//*@null@*/char *host;
    SOCKET descriptor;
    int connected;
    bool fcommand;
    char inbuf[4 * MIL];
    char incomm[MIL];
    char inlast[MIL];
    int repeat;
    /*@owned@*//*@null@*/char *outbuf;
    size_t outsize;
    int outtop;
    /*@shared@*//*@null@*/char *showstr_head;
    /*@shared@*//*@null@*/char *showstr_point;
    /*@shared@*//*@null@*/void *ed_data;
    /*@shared@*//*@null@*/char **ed_string;
    int editor;
    int idle;
};


/* TO types for act. */
#define TO_ROOM         0
#define TO_NOTVICT      1
#define TO_VICT         2
#define TO_CHAR         3
#define TO_ALL          4

/* Help table types. */
struct help_data {
    HELP_DATA * next;
    HELP_DATA * next_area;
    int level;
    char * keyword;
    char * text;
};

struct help_area_data {
    HELP_AREA * next;
    HELP_DATA * first;
    HELP_DATA * last;
    AREA_DATA * area;
    char * filename;
    bool changed;
};


/*
 * Shop types.
 */
#define MAX_TRADE       5

struct shop_data {
    SHOP_DATA * next;                   /* Next shop in list            */
    long keeper;                        /* Vnum of shop keeper mob      */
    int buy_type[MAX_TRADE];            /* Item types shop will buy     */
    int profit_buy;                     /* Cost multiplier for buying   */
    int profit_sell;                    /* Cost multiplier for selling  */
    int open_hour;                      /* First opening hour           */
    int close_hour;                     /* First closing hour           */
};



/*
 * Per-class stuff.
 */

#define MAX_GUILD               2
#define MAX_STATS               6
#define STAT_STR                0
#define STAT_INT                1
#define STAT_WIS                2
#define STAT_DEX                3
#define STAT_CON                4
#define STAT_LUCK               5

struct class_type {
    char * name;                /* the full name of the class */
    char who_name[4];           /* three-letter name for 'who */
    int attr_prime;             /* prime attribute */
    long weapon;                /* first weapon */
    long guild[MAX_GUILD];      /* vnum of guild rooms */
    int skill_adept;            /* maximum skill level */
    int thac0_00;               /* thac0 for level  0 */
    int thac0_32;               /* thac0 for level 32 */
    int hp_min;                 /* min hp gained on leveling */
    int hp_max;                 /* max hp gained on leveling */
    int mana_min;               /* min mana gained on leveling */
    int mana_max;               /* max mana gained on leveling */
    bool fMana;                 /* class gains mana on level */
    char * base_group;          /* base skills gained */
    char * default_group;       /* default skills gained */

    /* New Melee Combat Math */
    float attack_rating;
    float ar_improve;
    float defense_rating;
    float dr_improve;

    /* New Structure Parts, Added by Monrick 3/31/2008 */
    bool canCreate;     /* class can be chosen at creation */
    char * shortDesc;   /* description for help file */
};

struct item_type {
    int type;
    char * name;
    char * help_keyword;
};

struct weapon_type {
    char * name;
    long vnum;
    int type;
    SKILL **gsp;
};

struct wiznet_type {
    char * name;
    long flag;
    int level;
};

struct impnet_type {
    char * name;
    long flag;
    int level;
};

struct attack_type {
    char * name;   /* name */
    char * noun;   /* message */
    int damage; /* damage class */
};

struct race_type {
    char * name;           /* call name of the race */
    bool pc_race;        /* can be chosen by pcs */
    long act;            /* act bits for the race */
    long aff;            /* aff bits for the race */
    long off;            /* off bits for the race */
    long imm;            /* imm bits for the race */
    long res;            /* res bits for the race */
    long vuln;           /* vuln bits for the race */
    long form;           /* default form flag for the race */
    long parts;          /* default parts for the race */
};

/* additional data for pc races */
struct pc_race_type {
    char * name;                           /* MUST be in race_type */
    char who_name[6];
    int points;                         /* cost in points of the race */
    int class_mult[MAX_CLASS];          /* exp multiplier for class, * 100 */
    char * skills[5];                      /* bonus skills for the race */
    int stats[MAX_STATS];               /* starting stats */
    int max_train_stats[MAX_STATS];     /* maximum a stat can be trained to */
    int max_stats[MAX_STATS];           /* maximum stats */
    int size;                           /* aff bits for the race */
};


/* Data structure for notes. */

#define NOTE_NOTE       0
#define NOTE_PENALTY    1
#define NOTE_NEWS       2
#define NOTE_CHANGES    3
#define NOTE_RPNOTE     4
#define NOTE_AUCNOTE    5
#define NOTE_CONTEST    7
#define NOTE_IDEA       8
#define NOTE_BUILD      9

#define NOTE_MAX        10

struct note_data {
    NOTE_DATA * next;
    bool  valid;
    int  type;
    char *  sender;
    char *  date;
    char *  to_list;
    char *  subject;
    char *  text;
    time_t  date_stamp;
};

/* message type information*/
struct message_types {
    int type;
    char *name;
    char *display;
    char *desc;
    char *file_name;
    int post_level;
    time_t retention;
    NOTE_DATA **thread;
};

extern const struct message_types message_type_table[];


struct affect_data {
    AFFECT_DATA * next;
    bool valid;
    SKILL *skill;
    int where;
    int type;
    int level;
    int duration;
    int location;
    long modifier;
    long bitvector;
};

/* where definitions */
#define TO_AFFECTS      0
#define TO_OBJECT       1
#define TO_IMMUNE       2
#define TO_RESIST       3
#define TO_VULN         4
#define TO_WEAPON       5
#define TO_ACT_FLAG     6


/* A kill structure (indexed by level). */
struct kill_data {
    int number;
    int killed;
};



/* BUILDING CODE STARTS HERE
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO           3090l
#define MOB_VNUM_PIG            3167l
#define MOB_VNUM_FAMILIAR       4l


/* RT ASCII conversions -- used so we can have letters in this file */
#define A       1l
#define B       2l
#define C       4l
#define D       8l
#define E       16l
#define F       32l
#define G       64l
#define H       128l
#define I       256l
#define J       512l
#define K       1024l
#define L       2048l
#define M       4096l
#define N       8192l
#define O       16384l
#define P       32768l
#define Q       65536l
#define R       131072l
#define S       262144l
#define T       524288l
#define U       1048576l
#define V       2097152l
#define W       4194304l
#define X       8388608l
#define Y       16777216l
#define Z       33554432l
#define aa      67108864l       /* doubled due to conflicts */
#define bb      134217728l
#define cc      268435456l
#define dd      536870912l
#define ee      1073741824l
#define ff      2147483648ul
#define gg      4294967296ul
#define hh      8589934592ul

/* ACT bits for mobs. * Used in #MOBILES. */
#define ACT_IS_NPC              (A)     /* Auto set for mobs      */
#define ACT_SENTINEL            (B)     /* Stays in one room      */
#define ACT_SCAVENGER           (C)     /* Picks up objects       */
#define ACT_AGGRESSIVE          (F)     /* Attacks PC's           */
#define ACT_STAY_AREA           (G)     /* Won't leave area       */
#define ACT_WIMPY               (H)
#define ACT_PET                 (I)     /* Auto set for pets      */
#define ACT_TRAIN               (J)     /* Can train PC's */
#define ACT_PRACTICE            (K)     /* Can practice PC's      */
#define ACT_UNDEAD              (O)
#define ACT_CLERIC              (Q)
#define ACT_MAGE                (R)
#define ACT_THIEF               (S)
#define ACT_WARRIOR             (T)
#define ACT_NOPURGE             (V)
#define ACT_OUTDOORS            (W)
#define ACT_INDOORS             (Y)
#define ACT_IS_EXCHANGER        (Z)
#define ACT_IS_HEALER           (aa)
#define ACT_GAIN                (bb)
#define ACT_UPDATE_ALWAYS       (cc)
#define ACT_IS_CHANGER          (dd)
#define ACT_IS_BLACKSMITH       (ee)

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_MENTAL              12
#define DAM_DROWNING            14
#define DAM_LIGHT               15
#define DAM_OTHER               16
#define DAM_HARM                17
#define DAM_CHARM               18
#define DAM_SOUND               19
#define DAM_ILLUSION            20
#define DAM_WOOD                21

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH               (O)
#define ASSIST_ALL              (P)
#define ASSIST_RACE             (R)
#define ASSIST_PLAYERS          (S)
#define ASSIST_GUARD            (T)
#define ASSIST_VNUM             (U)

/* return values for check_imm */
#define IS_NORMAL               0
#define IS_IMMUNE               1
#define IS_RESISTANT            2
#define IS_VULNERABLE           3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DROWNING            (R)
#define IMM_LIGHT               (S)
#define IMM_SOUND               (T)
#define IMM_WOOD                (X)
#define IMM_SILVER              (Y)
#define IMM_IRON                (Z)
#define IMM_ILLUSION            (aa)

/* RES bits for mobs */
#define RES_SUMMON              (A)
#define RES_CHARM               (B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DROWNING            (R)
#define RES_LIGHT               (S)
#define RES_SOUND               (T)
#define RES_WOOD                (X)
#define RES_SILVER              (Y)
#define RES_IRON                (Z)
#define RES_ILLUSION            (aa)

/* VULN bits for mobs */
#define VULN_SUMMON             (A)
#define VULN_CHARM              (B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DROWNING           (R)
#define VULN_LIGHT              (S)
#define VULN_SOUND              (T)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON               (Z)
#define VULN_ILLUSION           (aa)

/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)     /* defined by material bit */

/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)
#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB               (S)
#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD         (cc)

/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE                (K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS              (Y)


/* Bits for 'affected_by'. * Used in #MOBILES. */
#define AFF_POLLEN              (A)
#define AFF_INVISIBLE           (B)
#define AFF_DETECT_INVIS        (D)
#define AFF_DETECT_MAGIC        (E)
#define AFF_DRUID_CALL          (F)
#define AFF_SANCTUARY           (H)
#define AFF_FAERIE_FIRE         (I)
#define AFF_INFRARED            (J)
#define AFF_CURSE               (K)
#define AFF_BLIND               (L)
#define AFF_POISON              (M)
#define AFF_SNEAK               (P)
#define AFF_HIDE                (Q)
#define AFF_SLEEP               (R)
#define AFF_CHARM               (S)
#define AFF_FLYING              (T)
#define AFF_PASS_DOOR           (U)
#define AFF_HASTE               (V)
#define AFF_CALM                (W)
#define AFF_WEAKEN              (Y)
#define AFF_DARK_VISION         (Z)
#define AFF_BERSERK             (aa)
#define AFF_CALLOUSED           (bb)
#define AFF_SLOW                (cc)
#define AFF_BURNING             (ff)
#define AFF_PHASED              (hh)
#define AFF_STONE_SKIN          (gg)

/* Sex. * Used in #MOBILES. */
#define SEX_NEUTRAL             0
#define SEX_MALE                1
#define SEX_FEMALE              2

/* AC types */
#define AC_PIERCE               0
#define AC_BASH                 1
#define AC_SLASH                2
#define AC_EXOTIC               3

/* dice */
#define DICE_NUMBER             0
#define DICE_TYPE               1
#define DICE_BONUS              2

/* size */
#define SIZE_TINY               0
#define SIZE_SMALL              1
#define SIZE_MEDIUM             2
#define SIZE_LARGE              3
#define SIZE_HUGE               4
#define SIZE_GIANT              5



/* Well known object virtual numbers. * Defined in #OBJECTS. */
#define OBJ_VNUM_SILVER_ONE             1l
#define OBJ_VNUM_GOLD_ONE               2l
#define OBJ_VNUM_GOLD_SOME              3l
#define OBJ_VNUM_SILVER_SOME            4l
#define OBJ_VNUM_COINS                  5l

#define OBJ_VNUM_CORPSE_NPC             10l
#define OBJ_VNUM_CORPSE_PC              11l
#define OBJ_VNUM_SEVERED_HEAD           12l
#define OBJ_VNUM_TORN_HEART             13l
#define OBJ_VNUM_SLICED_ARM             14l
#define OBJ_VNUM_SLICED_LEG             15l
#define OBJ_VNUM_GUTS                   16l
#define OBJ_VNUM_BRAINS                 17l
#define OBJ_VNUM_LIGHT_BALL             21l
#define OBJ_VNUM_DISC                   23l
#define OBJ_VNUM_PORTAL                 25l
#define OBJ_VNUM_PIT                    3010l
#define OBJ_VNUM_SCHOOL_MACE            3700l
#define OBJ_VNUM_SCHOOL_DAGGER          3701l
#define OBJ_VNUM_SCHOOL_SWORD           3702l
#define OBJ_VNUM_SCHOOL_SPEAR           3717l
#define OBJ_VNUM_SCHOOL_STAFF           3718l
#define OBJ_VNUM_SCHOOL_AXE             3719l
#define OBJ_VNUM_SCHOOL_FLAIL           3720l
#define OBJ_VNUM_SCHOOL_WHIP            3721l
#define OBJ_VNUM_SCHOOL_POLEARM         3722l
#define OBJ_VNUM_SCHOOL_VEST            3703l
#define OBJ_VNUM_SCHOOL_SHIELD          3704l
#define OBJ_VNUM_SCHOOL_BANNER          3716l
#define OBJ_VNUM_MAP                    3162l
#define OBJ_VNUM_RECEIPT                29l
#define OBJ_VNUM_FIREBLADE              30l
#define OBJ_VNUM_BLANK_PILL             14241l          /* Anonplis.are */

/* Item types. * Used in #OBJECTS. */
#define ITEM_LIGHT                      1
#define ITEM_SCROLL                     2
#define ITEM_WAND                       3
#define ITEM_STAFF                      4
#define ITEM_WEAPON                     5
#define ITEM_TREASURE                   8
#define ITEM_ARMOR                      9
#define ITEM_POTION                     10
#define ITEM_CLOTHING                   11
#define ITEM_FURNITURE                  12
#define ITEM_TRASH                      13
#define ITEM_CONTAINER                  15
#define ITEM_DRINK_CON                  17
#define ITEM_KEY                        18
#define ITEM_FOOD                       19
#define ITEM_MONEY                      20
#define ITEM_BOAT                       22
#define ITEM_CORPSE_NPC                 23
#define ITEM_CORPSE_PC                  24
#define ITEM_FOUNTAIN                   25
#define ITEM_PILL                       26
#define ITEM_PROTECT                    27
#define ITEM_MAP                        28
#define ITEM_PORTAL                     29
#define ITEM_WARP_STONE                 30
#define ITEM_ROOM_KEY                   31
#define ITEM_GEM                        32
#define ITEM_JEWELRY                    33
#define ITEM_TELEPORT                   36
#define ITEM_ATM                        37
#define ITEM_INVITATION                 38
#define ITEM_SCARAB                     39
#define ITEM_FAERIE_FOG                 42
#define ITEM_DUST                       43
#define ITEM_DOLL                       44
#define ITEM_SOCKETS                    46
#define ITEM_DICE                       47
#define ITEM_RELIC                      48

/* Values for socketing items (value[0]). * Used in #OBJECTS. */
#define SOC_SAPPHIRE    1
#define SOC_RUBY        2
#define SOC_EMERALD     3
#define SOC_DIAMOND     4
#define SOC_TOPAZ       5
#define SOC_SKULL       6

/*Values for socketing items (value[1]).  * used in #OBJECTS  */
#define GEM_CHIPPED     0
#define GEM_FLAWED      1
#define GEM_FLAWLESS    2
#define GEM_PERFECT     3


/* Extra flags. * Used in #OBJECTS. */
#define ITEM_GLOW               (A)
#define ITEM_HUM                (B)
#define ITEM_DARK               (C)
#define ITEM_LOCK               (D)
#define ITEM_INVIS              (F)
#define ITEM_MAGIC              (G)
#define ITEM_NODROP             (H)
#define ITEM_BLESS              (I)
#define ITEM_NOREMOVE           (M)
#define ITEM_INVENTORY          (N)
#define ITEM_NOPURGE            (O)
#define ITEM_ROT_DEATH          (P)
#define ITEM_VIS_DEATH          (Q)
#define ITEM_NOSAC              (R)
#define ITEM_NONMETAL           (S)
#define ITEM_NOLOCATE           (T)
#define ITEM_MELT_DROP          (U)
#define ITEM_HAD_TIMER          (V)
#define ITEM_SELL_EXTRACT       (W)
#define ITEM_NOAUC              (X)
#define ITEM_BURN_PROOF         (Y)
#define ITEM_NOUNCURSE          (Z)
#define ITEM_CACHE              (aa)
#define ITEM_SAFE_CORPSE        (bb)
#define ITEM_UNIQUE             (cc)
#define ITEM_INLAY1             (dd)
#define ITEM_DEATH_DROP         (ee)
#define ITEM_INLAY2             (ff)

/* extra2 bits */
#define ITEM2_GRAFT             (A)
#define ITEM2_ETHEREAL          (B)
#define ITEM2_NOSCAN            (C)
#define ITEM2_RELIC             (D)
#define ITEM2_NODONATE          (E)
#define ITEM2_NORESTRING        (F)

/* Wear flags. * Used in #OBJECTS. */
#define ITEM_TAKE               (A)
#define ITEM_WEAR_FINGER        (B)
#define ITEM_WEAR_NECK          (C)
#define ITEM_WEAR_BODY          (D)
#define ITEM_WEAR_HEAD          (E)
#define ITEM_WEAR_LEGS          (F)
#define ITEM_WEAR_FEET          (G)
#define ITEM_WEAR_HANDS         (H)
#define ITEM_WEAR_ARMS          (I)
#define ITEM_WEAR_SHIELD        (J)
#define ITEM_WEAR_ABOUT         (K)
#define ITEM_WEAR_WAIST         (L)
#define ITEM_WEAR_WRIST         (M)
#define ITEM_WIELD              (N)
#define ITEM_HOLD               (O)
#define ITEM_NO_SAC             (P)
#define ITEM_WEAR_FLOAT         (Q)
#define ITEM_WEAR_EAR           (R)
#define ITEM_WEAR_FACE          (S)
#define ITEM_WEAR_TATTOO        (T)


/* weapon class */
#define WEAPON_EXOTIC           0
#define WEAPON_SWORD            1
#define WEAPON_DAGGER           2
#define WEAPON_SPEAR            3
#define WEAPON_MACE             4
#define WEAPON_AXE              5
#define WEAPON_FLAIL            6
#define WEAPON_WHIP             7
#define WEAPON_POLEARM          8

/* weapon types */
#define WEAPON_FLAMING          (A)
#define WEAPON_FROST            (B)
#define WEAPON_VAMPIRIC         (C)
#define WEAPON_SHARP            (D)
#define WEAPON_VORPAL           (E)
#define WEAPON_TWO_HANDS        (F)
#define WEAPON_SHOCKING         (G)
#define WEAPON_POISON           (H)
#define WEAPON_ACIDIC           (I)     /* Silly rabbi, kicks are for trids */

/* gate flags */
#define GATE_NORMAL_EXIT        (A)
#define GATE_NOCURSE            (B)
#define GATE_GOWITH             (C)
#define GATE_BUGGY              (D)
#define GATE_RANDOM             (E)
#define GATE_BARRED             (F)

/* furniture flags */
#define STAND_AT                (A)
#define STAND_ON                (B)
#define STAND_IN                (C)
#define SIT_AT                  (D)
#define SIT_ON                  (E)
#define SIT_IN                  (F)
#define REST_AT                 (G)
#define REST_ON                 (H)
#define REST_IN                 (I)
#define SLEEP_AT                (J)
#define SLEEP_ON                (K)
#define SLEEP_IN                (L)
#define PUT_AT                  (M)
#define PUT_ON                  (N)
#define PUT_IN                  (O)
#define PUT_INSIDE              (P)

/* Apply types (for affects). * Used in #OBJECTS. */
#define APPLY_NONE              0
#define APPLY_STR               1
#define APPLY_DEX               2
#define APPLY_INT               3
#define APPLY_WIS               4
#define APPLY_CON               5
#define APPLY_SEX               6
#define APPLY_CLASS             7
#define APPLY_LEVEL             8
#define APPLY_AGE               9
#define APPLY_HEIGHT            10
#define APPLY_WEIGHT            11
#define APPLY_MANA              12
#define APPLY_HIT               13
#define APPLY_MOVE              14
#define APPLY_GOLD              15
#define APPLY_EXP               16
#define APPLY_AC                17
#define APPLY_HITROLL           18
#define APPLY_DAMROLL           19
#define APPLY_SAVES             20
#define APPLY_SAVING_PARA       20
#define APPLY_SAVING_ROD        21
#define APPLY_SAVING_PETRI      22
#define APPLY_SAVING_BREATH     23
#define APPLY_SAVING_SPELL      24
#define APPLY_SPELL_AFFECT      25
#define APPLY_LUCK              26
#define APPLY_MLAG              27
#define APPLY_TLAG              28

/* Values for containers (value[1]). * Used in #OBJECTS. */
#define CONT_CLOSEABLE          1
#define CONT_PICKPROOF          2
#define CONT_CLOSED             4
#define CONT_LOCKED             8
#define CONT_PUT_ON             16
#define TOKEN_QP                1
#define TOKEN_TRAIN             2
#define TOKEN_200               4
#define TOKEN_10                8
#define TOKEN_WEAPONFLAG        16
#define TOKEN_FIREPROOF         32
#define TOKEN_DAMAGENOUN        64
#define TOKEN_RESTRING          128
#define TOKEN_SKILLSET          256
#define TOKEN_IMMHIDDEN         2048
#define TOKEN_IMMWILD           4096
#define TOKEN_IMP               8192
#define TOKEN_AC                16384

/* Well known room virtual numbers. * Defined in #ROOMS. */
#define ROOM_VNUM_LIMBO         2l
#define ROOM_VNUM_DUEL          3l
#define ROOM_VNUM_CHAT          1200l
#define ROOM_VNUM_TEMPLE        3001l
#define ROOM_VNUM_ALTAR         3054l
#define ROOM_VNUM_SCHOOL        3700l
#define ROOM_VNUM_DONATION      3360l

/* Room flags. * Used in #ROOMS. */
#define ROOM_DARK               (A)
#define ROOM_NORANDOM           (B)
#define ROOM_NO_MOB             (C)
#define ROOM_INDOORS            (D)

#define ROOM_NO_PUSH_NO_DRAG    (F)
#define ROOM_PORTALONLY         (H)

#define ROOM_PRIVATE            (J)
#define ROOM_SAFE               (K)
#define ROOM_SOLITARY           (L)
#define ROOM_PET_SHOP           (M)
#define ROOM_IMP_ONLY           (O)
#define ROOM_GODS_ONLY          (P)
#define ROOM_HEROES_ONLY        (Q)
#define ROOM_NEWBIES_ONLY       (R)
#define ROOM_LAW                (S)
#define ROOM_NOWHERE            (T)
#define ROOM_BANK               (U)

#define ROOM_HIGHEST_ONLY       (X)
#define ROOM_HIGHER_ONLY        (Y)
#define ROOM_NOGATE             (aa)
#define ROOM_NOSUMMON           (bb)
#define ROOM_NOPORTAL           (dd)
#define ROOM_NOTRANSPORT        (ee)
#define ROOM_NOTELEPORT         (ff)

/* AREA Flags*/


/* Directions. * Used in #ROOMS. */
#define DIR_NORTH               0
#define DIR_EAST                1
#define DIR_SOUTH               2
#define DIR_WEST                3
#define DIR_UP                  4
#define DIR_DOWN                5

/* Exit flags. * Used in #ROOMS. */
#define EX_ISDOOR               (int)(A)
#define EX_CLOSED               (int)(B)
#define EX_LOCKED               (int)(C)
#define EX_PICKPROOF            (int)(F)
#define EX_NOPASS               (int)(G)
#define EX_EASY                 (int)(H)
#define EX_HARD                 (int)(I)
#define EX_INFURIATING          (int)(J)
#define EX_NOCLOSE              (int)(K)
#define EX_NOLOCK               (int)(L)


/* Sector types. * Used in #ROOMS. */
#define SECT_INSIDE             0
#define SECT_CITY               1
#define SECT_FIELD              2
#define SECT_FOREST             3
#define SECT_HILLS              4
#define SECT_MOUNTAIN           5
#define SECT_WATER_SWIM         6
#define SECT_WATER_NOSWIM       7
#define SECT_UNDERWATER         8
#define SECT_AIR                9
#define SECT_DESERT             10
#define SECT_MAX                11


/* Equpiment wear locations. * Used in #RESETS. */
#define WEAR_NONE               -1
#define WEAR_LIGHT              0
#define WEAR_FINGER_L           1
#define WEAR_FINGER_R           2
#define WEAR_NECK_1             3
#define WEAR_NECK_2             4
#define WEAR_BODY               5
#define WEAR_HEAD               6
#define WEAR_LEGS               7
#define WEAR_FEET               8
#define WEAR_HANDS              9
#define WEAR_ARMS               10
#define WEAR_SHIELD             11
#define WEAR_ABOUT              12
#define WEAR_WAIST              13
#define WEAR_WRIST_L            14
#define WEAR_WRIST_R            15
#define WEAR_WIELD              16
#define WEAR_HOLD               17
#define WEAR_FLOAT              18
#define WEAR_SECONDARY          19
#define WEAR_THIRD              20
#define WEAR_FINGER_L2          21
#define WEAR_FINGER_R2          22
#define WEAR_EAR_L              23
#define WEAR_EAR_R              24
#define WEAR_FACE               25
#define WEAR_TATTOO             26
#define MAX_WEAR                27




/* Conditions. */
#define COND_FULL               1
#define COND_THIRST             2
#define COND_HUNGER             3
#define COND_FEED               4
#define COND_DRAINED            5
#define COND_MAX                6

/** Positions. */
#define POS_DEAD                0
#define POS_MORTAL              1
#define POS_INCAP               2
#define POS_STUNNED             3
#define POS_SLEEPING            4
#define POS_RESTING             5
#define POS_SITTING             6
#define POS_FIGHTING            7
#define POS_STANDING            8

/* ACT bits for players. */
/* RT auto flags */
#define PLR_KILLER              (B)
#define PLR_AUTOASSIST          (C)
#define PLR_AUTOEXIT            (D)
#define PLR_AUTOLOOT            (E)
#define PLR_AUTOSAC             (F)
#define PLR_AUTOGOLD            (G)
#define PLR_AUTOSPLIT           (H)
#define PLR_LINKDEAD            (L)

/* RT personal flags */
#define PLR_HOLYLIGHT           (N)
#define PLR_CANLOOT             (P)
#define PLR_NOSUMMON            (Q)
#define PLR_NOFOLLOW            (R)
#define PLR_AUTOEQ              (V)

/* penalty flags */
#define PLR_PERMIT              (U)
#define PLR_LOG                 (W)
#define PLR_DENY                (X)
#define PLR_THIEF               (Z)

/* RT comm flags -- may be used n both mobs and chars */
#define COMM_COMPACT            (A)
#define COMM_BRIEF              (B)
#define COMM_PROMPT             (C)
#define COMM_COMBINE            (D)
#define COMM_SHOW_AFFECTS       (E)
#define COMM_AFK                (F)
#define COMM_TICKS              (G)
#define COMM_CODING             (H)
#define COMM_BUILD              (I)
#define COMM_BUSY               (J)


/* WIZnet flags */
#define WIZ_ON                  (A)
#define WIZ_TICKS               (B)
#define WIZ_LOGINS              (C)
#define WIZ_SITES               (D)
#define WIZ_LINKS               (E)
#define WIZ_DEATHS              (F)
#define WIZ_FLAGS               (I)
#define WIZ_PENALTIES           (J)
#define WIZ_SACCING             (K)
#define WIZ_LEVELS              (L)
#define WIZ_SECURE              (M)
#define WIZ_SWITCHES            (N)
#define WIZ_SNOOPS              (O)
#define WIZ_RESTORE             (P)
#define WIZ_LOAD                (Q)
#define WIZ_NEWBIE              (R)
#define WIZ_PREFIX              (S)
#define WIZ_SPAM                (T)
#define WIZ_PLOG                (U)
#define WIZ_ALOG                (V)
#define WIZ_ROLEPLAY            (X)

/*  IMPNet Flags  */
#define IMN_ON                  (A)
#define IMN_MOBDEATHS           (B)
#define IMN_PREFIX              (C)
#define IMN_RESETS              (D)
#define IMN_SAVES               (E)
#define IMN_TICKS               (F)
#define IMN_UPDATES             (G)
#define IMN_AUTO                (H)

#define LIQ_WATER               0


struct liq_type {
    char * liq_name;
    char * liq_color;
    int liq_affect[5];
};



/***************************************************************************
 * mob/character structures
 * mob_index_data
 * the database prototype for a mob
 ***************************************************************************/
struct mob_index_data {
    MOB_INDEX_DATA *next;
    SHOP_DATA *shop;
    MPROG_LIST *mprogs;
    AREA_DATA *area;
    long vnum;
    long group;
    bool new_format;
    int count;
    int killed;
    char *player_name;
    char *short_descr;
    char *long_descr;
    char *description;
    long act;
    long affected_by;
    int level;
    int hitroll;
    int hit[3];
    int mana[3];
    int damage[3];
    long ac[4];
    int dam_type;
    long off_flags;
    long imm_flags;
    long res_flags;
    long vuln_flags;
    int start_pos;
    int default_pos;
    int sex;
    int race;
    unsigned int wealth;
    long form;
    long parts;
    int size;
    char *material;
    long mprog_flags;
};


/***************************************************************************
 * mob_memory
 ***************************************************************************/
#define MEM_CUSTOMER    A
#define MEM_SELLER      B
#define MEM_HOSTILE     C
#define MEM_AFRAID      D

/***************************************************************************
 * mem_data
 ***************************************************************************/
struct mem_data {
    MEM_DATA * next;
    bool  valid;
    int  id;
    int  reaction;
    time_t  when;
};


/***************************************************************************
 * char_data
 *
 * the instance structure for a character - either mobile or
 * player
 ***************************************************************************/
struct char_data {
    CHAR_DATA *next;
    CHAR_DATA *next_in_room;
    CHAR_DATA *master;
    CHAR_DATA *leader;
    CHAR_DATA *fighting;
    CHAR_DATA *linked;
    CHAR_DATA *reply;
    CHAR_DATA *pet;
    CHAR_DATA *mob_wuss;
    CHAR_DATA *mobmem;
    CHAR_DATA *mprog_target;
    /*@null@*/MOB_INDEX_DATA *mob_idx;
    DESCRIPTOR_DATA *desc;
    AFFECT_DATA *affected;
    NOTE_DATA *pnote;
    GAMEOBJECT *carrying;
    GAMEOBJECT *on;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *was_in_room;
    AREA_DATA *zone;
    PC_DATA *pcdata;
    bool valid;
    char *name;
    long id;
    int version;
    char *short_descr;
    char *long_descr;
    char *description;
    char *prompt;
    long group;
    int sex;
    int race;
    int class;
    int level;
    int trust;
    int played;
    int lines;                          /* for the pager */
    time_t logon;
    time_t llogoff;
    int timer;
    int wait;
    int hit;
    int max_hit;
    int mana;
    int max_mana;
    int move;
    int max_move;
    unsigned int gold;
    unsigned int silver;
    int exp;
    long act;
    bool use_ansi_color;
    unsigned long channels_enabled;
    unsigned long channels_denied;
    long comm;   /* RT added to pad the vector */
    long imm_flags;
    long res_flags;
    long vuln_flags;
    int invis_level;
    int incog_level;
    long affected_by;
    int position;
    int carry_weight;
    int carry_number;
    int saving_throw;
    int hitroll;
    int damroll;
    long armor[4];
    int wimpy;
    int perm_stat[MAX_STATS];
    int mod_stat[MAX_STATS];
    long form;
    long parts;
    int size;
    char *material;
    long off_flags;
    int damage[3];
    int dam_type;
    int start_pos;
    int default_pos;
    long deathroom;
    int mprog_delay;
    int mLag;                   /* added by Monrick */
    int tLag;                   /*    May 2008      */
};


/***************************************************************************
 * pc_data
 *
 * data specific to a player
 ***************************************************************************/
struct pc_data {
    /*@null@*/PC_DATA *next;
    BUFFER *buffer;
    bool valid;
    char *pwd;
    char *bamfin;
    char *bamfout;
    char *title;
    char *grestore_string;
    char *rrestore_string;
    time_t last_note;
    time_t last_idea;
    time_t last_penalty;
    time_t last_news;
    time_t last_changes;
    time_t last_rpnote;
    time_t last_aucnote;
    time_t last_build;
    time_t last_read[NOTE_MAX];
    char *who_thing;
    char *filter[MAX_FORGET];
    int perm_hit;
    int perm_mana;
    int perm_move;
    int true_sex;
    int last_level;
    long condition[COND_MAX];
    LEARNED *skills;
    int  points;
    bool confirm_delete;
    bool confirm_suicide;
    char *alias[MAX_ALIAS];
    char *alias_sub[MAX_ALIAS];
    char *ignore[MAX_IGNORE];
    byte color_combat_s;
    byte color_combat_condition_s;
    byte color_combat_condition_o;
    byte color_invis;
    byte color_wizi;
    byte color_hp;
    byte color_combat_o;
    byte color_hidden;
    byte color_charmed;
    byte color_mana;
    byte color_move;
    byte color_say;
    byte color_tell;
    byte color_reply;
    unsigned int silver_in_bank;
    unsigned int gold_in_bank;
    long wiznet;
    long impnet;
    char *prefix;
    long pkills;
    long pdeaths;
    long mobkills;
    long mobdeaths;
    char *deathcry;
    int practice;
    int train;
    time_t killer_time;
    time_t thief_time;
    char *  afk_message;
    time_t last_bank;
    int security;
    int rank;
    unsigned int extendedlevel;
    long extendedexp;
    char *restring_name;
    char *restring_short;
    char *restring_long;
    char *history;
};


struct extra_descr_data {
    /*@owned@*//*@null@*/EXTRA_DESCR_DATA *next;
    bool valid;
    /*@shared@*/char *keyword;        /* Keyword in look/examine */
    /*@shared@*/char *description;    /* What to see    */
};


/***************************************************************************
 * object specific structures
 ***************************************************************************/
/*@abstract@*/struct objectprototype {
    /*@owned@*//*@null@*//*@partial@*/OBJECTPROTOTYPE *next;
    /*@dependent@*//*@null@*//*@partial@*/OBJECTPROTOTYPE *prev;

    long vnum;
    /*@owned@*//*@null@*/EXTRA_DESCR_DATA *extra_descr;
    /*@dependent@*//*@null@*/AFFECT_DATA *affected;
    /*@dependent@*//*@null@*/AREA_DATA *area;
    /*@shared@*/char *name;
    /*@shared@*//*@null@*/char *short_descr;
    /*@shared@*//*@null@*/char *description;
    /*@shared@*//*@null@*/char *material;
    long reset_num;
    int item_type;
    long extra_flags;
    long extra2_flags;
    long wear_flags;
    int level;
    int init_timer;
    int condition;
    int count;
    int weight;
    int plevel;
    int xp_tolevel;
    int exp;
    unsigned int cost;
    long value[5];
};


/***************************************************************************
 * object_data* a single instance of an object
 ***************************************************************************/
/*@abstract@*/struct gameobject {
    /*@owned@*//*@null@*//*@partial@*/GAMEOBJECT *next;
    /*@dependent@*//*@null@*//*@partial@*/GAMEOBJECT *prev;
    /*@dependent@*//*@null@*/GAMEOBJECT *next_content;
    /*@dependent@*//*@null@*/GAMEOBJECT *contains;
    /*@dependent@*//*@null@*/GAMEOBJECT *in_obj;
    /*@dependent@*//*@null@*/GAMEOBJECT *on;
    /*@dependent@*//*@null@*/CHAR_DATA *carried_by;
    /*@dependent@*//*@null@*/CHAR_DATA *target;
    /*@owned@*//*@null@*/EXTRA_DESCR_DATA *extra_descr;
    /*@dependent@*//*@null@*/AFFECT_DATA *affected;
    /*@dependent@*/OBJECTPROTOTYPE *objprototype;
    /*@dependent@*//*@null@*/ROOM_INDEX_DATA *in_room;
    bool enchanted;
    /*@shared@*//*@null@*/char *owner_name;
    /*@shared@*//*@null@*/char *override_name;
    /*@shared@*//*@null@*/char *short_descr;
    /*@shared@*//*@null@*/char *description;
    int plevel;
    int xp_tolevel;
    int exp;
    int item_type;
    long extra_flags;
    long extra2_flags;
    long wear_flags;
    int wear_loc;
    int weight;
    unsigned int cost;
    int level;
    int condition;
    /*@shared@*//*@null@*/char *material;
    int timer;
    long value[5];
};


/***************************************************************************
 * room specific structures
 ***************************************************************************/
struct exit_data {
    union {
	ROOM_INDEX_DATA * to_room;
	long   vnum;
    } u1;
    int  exit_info;
    int  level;
    long  key;
    char *  keyword;
    char *  description;
    EXIT_DATA * next;
    int  rs_flags;
    int  orig_door;
};



/***************************************************************************
 * reset_data
 *
 * reset commands:
 *  '*': comment
 *  'M': read a mobile
 *  'O': read an object
 *  'P': put object in object
 *  'G': give object to mobile
 *  'E': equip object to mobile
 *  'D': set state of door
 *  'R': randomize room exits
 *  'S': stop (end of list)
 ***************************************************************************/
struct reset_data {
    RESET_DATA * next;
    char  command;
    long  arg1;
    int  arg2;
    long  arg3;
    int  arg4;
};


/***************************************************************************
 * area_data* definition of an area
 ***************************************************************************/
struct area_data {
    AREA_DATA * next;
    RESET_DATA * reset_first;
    RESET_DATA * reset_last;
    HELP_AREA * helps;
    char *  file_name;
    char *  name;
    char *  credits;
    int  age;
    int  nplayer;
    int  llevel;
    int  ulevel;
    long  min_vnum;
    long  max_vnum;
    long  vnum;
    bool  empty;
    char *  builders;               /* Listing of */
    int  area_flags;
    int  security;               /* Value 1-9  */
    bool  complete;
    char *  description;
};


/***************************************************************************
 * room_index_data
 *
 * definition of a room
 ***************************************************************************/
struct room_index_data {
    ROOM_INDEX_DATA * next;
    CHAR_DATA *  people;
    GAMEOBJECT *  contents;
    EXTRA_DESCR_DATA * extra_descr;
    AREA_DATA *  area;
    EXIT_DATA *  exit[6];
    RESET_DATA *  reset_first;
    RESET_DATA *  reset_last;
    AFFECT_DATA *  affected;
    char *   name;
    char *   description;
    char *   owner;
    long   vnum;
    long   room_flags;
    int   light;
    int   sector_type;
    int   heal_rate;
    int   mana_rate;
    int   timer;
};



/***************************************************************************
 * other defines
 ***************************************************************************/

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,but may be arbitrary beyond that.
 */

#define TYPE_UNDEFINED          -1
#define TYPE_HIT                1000
/*  Target types. */
#define TAR_IGNORE              0
#define TAR_CHAR_OFFENSIVE      1
#define TAR_CHAR_DEFENSIVE      2
#define TAR_CHAR_SELF           3
#define TAR_OBJ_INV             4
#define TAR_OBJ_CHAR_DEF        5
#define TAR_OBJ_CHAR_OFF        6
#define TAR_ROOM                7
#define TARGET_CHAR             0
#define TARGET_OBJ              1
#define TARGET_ROOM             2
#define TARGET_NONE             3


/***************************************************************************
 * affect types
 ***************************************************************************/
#define AFFECT_TYPE_CHAR        1
#define AFFECT_TYPE_OBJ         2
#define AFFECT_TYPE_ROOM        3

/***************************************************************************
 * skills/spells
 ***************************************************************************/
struct skill_type {
    char *  name;                   /* Name of skill                */
    int  skill_level[MAX_CLASS]; /* Level needed by class        */
    int  rating[MAX_CLASS];      /* How hard it is to learn      */
    SPELL_FUN * spell_fun;              /* Spell pointer (for spells)   */
    int  target;                 /* Legal targets                */
    int  minimum_position;       /* Position for caster / user   */
    int *  pgsn;                   /* Pointer to associated gsn    */
    int  slot;                   /* Slot for #OBJECT loading     */
    int  min_mana;               /* Minimum mana used            */
    int  beats;                  /* Waiting time after use       */
    char *  noun_damage;            /* Damage message               */
    char *  msg_off;                /* Wear off message             */
    char *  msg_obj;                /* Wear off message for obects  */

    /*
     * flags and difficulty were really only added to make brewing
     * and scribing more balanced, but hopefully they will be flexible
     * enough that we can use them for other things to if the need ever
     * arises - one example might be SPELL_NOMOBCAST or something
     */
    long  flags;
    int  difficulty;
    AFFECT_FUN * aff_fun;
};

/* spell flags */
#define SPELL_NOSCRIBE          (A)
#define SPELL_NOBREW            (B)
#define SPELL_DISPELLABLE       (C)
#define SPELL_CANCELABLE        (D)
#define SPELL_AFFSTRIP          (E)

/* MOBprog definitions */
#define TRIG_ACT          (int)(A)
#define TRIG_BRIBE      (int)(B)
#define TRIG_DEATH      (int)(C)
#define TRIG_ENTRY      (int)(D)
#define TRIG_FIGHT      (int)(E)
#define TRIG_GIVE         (int)(F)
#define TRIG_GREET      (int)(G)
#define TRIG_GRALL      (int)(H)
#define TRIG_KILL         (int)(I)
#define TRIG_HPCNT      (int)(J)
#define TRIG_RANDOM     (int)(K)
#define TRIG_SPEECH     (int)(L)
#define TRIG_EXIT         (int)(M)
#define TRIG_EXALL      (int)(N)
#define TRIG_DELAY      (int)(O)
#define TRIG_SURR         (int)(P)

struct mprog_list {
    int  trig_type;
    char *  trig_phrase;
    long  vnum;
    char *  code;
    MPROG_LIST * next;
    bool  valid;
};

struct mprog_code {
    long  vnum;
    char *  comment;
    char *  code;
    MPROG_CODE * next;
};


enum e_harvey_proctor_is { hp_pissed_off, hp_irritated, hp_off_his_rocker, hp_agreeable };

/***************************************************************************
 * utility macros
 ***************************************************************************/
#define CHECK_NON_OWNER(ch)      (strncmp((ch)->name, "Brom", 5) && strncmp((ch)->name, "Araevin", 8))
#define IS_VALID(data)           ((data) != NULL && (data)->valid)
#define VALIDATE(data)           ((data)->valid = true)
#define INVALIDATE(data)         ((data)->valid = false)
#define IS_SET(flag, bit)        ((((flag) & (bit)) == (bit)))
#define SET_BIT(var, bit)        ((var) |= (bit))
#define REMOVE_BIT(var, bit)     ((var) &= ~(bit))
#define IN_RANGE(min, num, max)  (((min) < (num)) && ((num) < (max)))
#define CHECK_POS(a, b, c) \
{ \
    (a) = (b); \
    if ((a) < 0) \
    { \
	log_bug("CHECK_POS : " c " == %d < 0", (int)a); \
    } \
} \

#define DENY_NPC(ch) \
    if (ch == NULL) \
{ \
    return; \
} \
else \
if (IS_NPC(ch)) \
{ \
    send_to_char("Mobs can't use this command.\n\r", ch); \
    return; \
} \

#define IS_TRAINER(mob) \
    (IS_NPC((mob)) && (IS_SET((mob)->act, ACT_TRAIN))) \

#define IS_GUILDMASTER(mob) \
    (IS_NPC((mob)) && (IS_SET((mob)->act, ACT_PRACTICE))) \

#define IS_HEALER(mob) \
    (IS_NPC((mob)) && (IS_SET((mob)->act, ACT_IS_HEALER))) \

#define IS_CHANGER(mob) \
    (IS_NPC((mob)) && (IS_SET((mob)->act, ACT_IS_CHANGER))) \

#define IS_EXCHANGER(mob) \
    (IS_NPC((mob)) && (IS_SET((mob)->act, ACT_IS_EXCHANGER))) \

#define IS_SHOPKEEPER(mob) \
    (IS_NPC((mob)) && ((mob)->mob_idx != NULL) && ((mob)->mob_idx->shop != NULL)) \



#define CH(d) ((d)->original ? (d)->original : (d)->character)

/***************************************************************************
 * identd macros
 ***************************************************************************/
#define replace_string(pstr, nstr)    { free_string((pstr)); pstr = str_dup((nstr)); }
#define IS_NULLSTR(str)                 ((str) == NULL || (str)[0] == '\0')


/* character macros*/
#define IS_NPC(ch)              (IS_SET((ch)->act, ACT_IS_NPC) && (ch)->mob_idx != NULL)
#define IS_IMMORTAL(ch)         (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_IMP(ch)              (get_trust(ch) >= MAX_LEVEL)
#define IS_HERO(ch)             (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch, level)    (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)     (IS_SET((ch)->affected_by, (sn)))

#define GET_AGE(ch)             ((int)(17 + ((ch)->played + globalSystemState.current_time - (ch)->logon) / 72000))
#define IS_LINK_DEAD(ch)        ((!IS_NPC(ch)) && ch->desc == NULL)
#define IS_AWAKE(ch)            (ch->position > POS_SLEEPING)

/* stat max */
#define GET_HITROLL(ch)         ((ch)->hitroll + (get_curr_stat(ch, STAT_STR) / 2) + get_curr_stat(ch, STAT_DEX))
#define GET_DAMROLL(ch)         ((ch)->damroll + (get_curr_stat(ch, STAT_STR) * 3) / 2)
#define GET_AC(ch, type)         ((ch)->armor[type] + (IS_AWAKE(ch) ? get_curr_stat(ch, STAT_DEX) * -6 : 0))
#define IS_OUTSIDE(ch)          (!IS_SET((ch)->in_room->room_flags, ROOM_INDOORS))


#define WAIT_STATE(ch, npulse)  (set_wait((ch), (npulse)))

#define HAS_TRIGGER(ch, trig)    (IS_SET((ch)->mob_idx->mprog_flags, (trig)))
#define IS_SWITCHED(ch)         (ch->desc && ch->desc->original)
#define IS_BUILDER(ch, area)    (!IS_NPC(ch) && !IS_SWITCHED(ch) && (ch->pcdata->security >= area->security || strstr(area->builders, ch->name) || strstr(area->builders, "All")))

/** Object macros. */
#define CAN_WEAR(obj, part)     (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat)  (IS_SET((obj)->extra_flags, (stat)))
#define IS_OBJ_STAT2(obj, stat) (IS_SET((obj)->extra2_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat)(IS_SET((obj)->value[4], (stat)))
#define WEIGHT_MULT(obj)        ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : 100)
#define IS_OBJ2_STAT(obj, stat) (IS_SET((obj)->extra2_flags, (stat)))


/* Description macros. */
#define PERS(ch, looker)        (can_see(looker, (ch)) ? (IS_NPC(ch) ? (ch)->short_descr : (ch)->name) : "someone")


/* Global constants. */
extern const struct     class_type class_table[MAX_CLASS];
extern const struct     weapon_type weapon_table[];
extern const struct     item_type item_table[];
extern const struct     wiznet_type wiznet_table[];
extern const struct     impnet_type impnet_table[];
extern const struct     attack_type attack_table[];
extern const struct     race_type race_table[];
extern const struct     pc_race_type pc_race_table[];
extern const struct     liq_type liq_table[];

/* max skill number */
extern int gn_max_skill_sn;
extern int gn_max_group_sn;



/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/* Object defined in limbo.are * Used in save.c to load objects that don't exist. */
#define OBJ_VNUM_DUMMY  39l

/* Area flags. */
#define         AREA_NONE       0
#define         AREA_CHANGED    1       /* Area has been modified. */
#define         AREA_ADDED      2       /* Area has been added to. */
#define         AREA_LOADING    4       /* Used for counting in db.c */

#define MAX_DIR 6
#define NO_FLAG -99     /* Must not be used in flags or stats. */

/* Global variables. */
extern SYSTEM_STATE globalSystemState;
extern GAME_STATE globalGameState;

/* Global Constants */
extern char *const dir_name[];
extern const int rev_dir[];                 /* int - ROM OLC */
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;
extern SHOP_DATA *shop_last;
extern HELP_DATA *help_first;
extern SHOP_DATA *shop_first;
extern CHAR_DATA *char_list;
extern MPROG_CODE *mprog_list;
extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_help;
extern int top_reset;
extern int top_shop;
extern long top_vnum_mob;
extern long top_vnum_obj;
extern long top_vnum_room;
extern MOB_INDEX_DATA *mob_index_hash  [MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash [MAX_KEY_HASH];





/***************************************************************************
 * data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * all files are read in completely at bootup.
 * most output files (bug, idea, typo, shutdown) are append-only.
 ***************************************************************************/
#define LAST_COMMAND     "./db/last_command.txt"  /* Tracking commands */
#define PLAYER_DIR       "./db/player/"           /* Player files */
#define GOD_DIR          "./db/gods/"             /* list of gods */
#define TEMP_FILE        "./db/player/romtmp"
#define RDESC_DIR        "./db/rdesc/"
#define MEMLOG_FILE      "./log/"
#define EXE_FILE         "./badtrip"

#define BUG_FILE         "./log/bug.txt"  /* For 'bug' and bug() */
#define TYPO_FILE        "./log/typo.txt" /* For 'typo' */
#define LAST_COMMANDS    "./log/command/lastCMDs.txt"
#define LOG_ALWAYS_FILE  "./log/command/logAlways.txt"
#define LOG_ALL_CMDS_FILE "./log/command/logALLCommands.txt"
#define LOG_PLAYER_FILE  "./log/player/%s.txt"
#define AREA_FOLDER      "./db/area/"
#define AREA_LIST        "./db/area.lst"          /* List of areas */
#define BAN_FILE         "./db/ban.txt"
#define HEADLINE_FILE    "./db/headline.txt"
#define HELP_FILE        "./db/area/help.are"
#define NOTE_FILE        "./db/notes.not"         /* note thread */
#define SHUTDOWN_FILE    "./shutdown.txt"         /* For 'shutdown' */
#define COPYOVER_FILE    "./copyover.txt"

#define END_MARKER       "END"




/**
 * services/follower_service.c
 */
void add_follower(CHAR_DATA * ch, CHAR_DATA * master);
void stop_follower(CHAR_DATA * ch);
void nuke_pets(CHAR_DATA * ch);
void die_follower(CHAR_DATA * ch);
bool is_same_group(CHAR_DATA * ach, CHAR_DATA * bch);



/** somewhere */
int get_item_apply_val(char *name);
void set_wait(CHAR_DATA * ch, int pulse);
void set_bash(CHAR_DATA * ch, int pulse);


/* act_enter.c */
ROOM_INDEX_DATA *get_random_room(CHAR_DATA * ch, AREA_DATA * area);

/* act_info.c */
void set_title(CHAR_DATA * ch, char *title);
void set_seek(CHAR_DATA * ch, char *seek);
void show_list_to_char(GAMEOBJECT * list, CHAR_DATA * ch, bool fShort, bool fShowNothing);
void show_damage_display(CHAR_DATA * ch, CHAR_DATA * victim);

/* act_move.c */
void move_char(CHAR_DATA * ch, int door, bool follow);
void push_char(CHAR_DATA * ch, CHAR_DATA * vch, int door, bool follow);
void drag_char(CHAR_DATA * ch, CHAR_DATA * victim, int door, bool follow);
int find_door(CHAR_DATA * ch, char *arg);
int find_exit(CHAR_DATA * ch, char *arg);

/* act_obj.c */
bool can_loot(CHAR_DATA * ch, GAMEOBJECT * obj);
void wear_obj(CHAR_DATA * ch, GAMEOBJECT * obj, bool fReplace);

/* act_wiz.c */
void wiznet(char *string, /*@null@*/ CHAR_DATA * ch, /*@null@*/ GAMEOBJECT * obj, long flag, long flag_skip, int min_level);
void impnet(char *string, CHAR_DATA * ch, GAMEOBJECT * obj, long flag, long flag_skip, int min_level);

/* alias.c */
void substitute_alias(DESCRIPTOR_DATA * d, const char *input);


/* ban.c */
bool check_ban(const char *site, int type);

/* comm.c */
void show_string(struct descriptor_data *d, char *input);
void close_socket(DESCRIPTOR_DATA * dclose, bool withProcessOutput, bool withSaveChar);
void write_to_buffer(DESCRIPTOR_DATA * d, const char *txt, int length);
void send_to_char(char *txt, /*@partial@*/CHAR_DATA * ch);
void send_to_char_ascii(char *txt, /*@partial@*/CHAR_DATA * ch);
void page_to_char(const char *txt, /*@partial@*/const CHAR_DATA * ch);
void act(const char *format, /*@partial@*/CHAR_DATA * ch, /*@null@*/const void *arg1, /*@null@*/const void *arg2, int type);
void act_new(const char *format, /*@partial@*/CHAR_DATA * ch, /*@null@*/const void *arg1, /*@null@*/const void *arg2, int type, int min_pos, bool mob_trigger);
void printf_to_char(CHAR_DATA *, char *, ...);


/* nanny.c */
void nanny(DESCRIPTOR_DATA * d, const char *argument);
bool check_parse_name(const char *name);
bool check_reconnect(DESCRIPTOR_DATA * d, const char *name, bool fConn);
bool check_playing(DESCRIPTOR_DATA * d, const char *name);
void stop_idling(CHAR_DATA * ch);

/* db.c */
char *print_flags(long flag);
void boot_db(void);
void area_update(void);

/* creation/cloning */
CHAR_DATA *create_mobile(MOB_INDEX_DATA * mob_idx);
void clone_mobile(CHAR_DATA * parent, CHAR_DATA * clone);
GAMEOBJECT *create_object(OBJECTPROTOTYPE * objprototype, int level);
void clear_char(CHAR_DATA * ch);

/* find functions  */
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA * ed);
MOB_INDEX_DATA *get_mob_index(long vnum);
ROOM_INDEX_DATA *get_room_index(long vnum);


/* memory management */
void *alloc_mem(unsigned int sMem);
void *alloc_perm(unsigned int sMem);
void free_mem(void *pMem, unsigned int sMem);
/*@shared@*/char *str_dup(const char *str);
void free_string(char *pstr);

/* number manipulation */
int number_door(void);
int interpolate(int level, int value_00, int value_32);
void smash_tilde(char *str);


/* misc utility func. */
void tail_chain(void);

/* olc/mprogs */
MPROG_CODE *get_mprog_index(long vnum);
void reset_area(AREA_DATA * pArea);
void reset_room(ROOM_INDEX_DATA * pRoom);
void load_socials(void);

/* effects.c */
void acid_effect(void *vo, int level, int dam, int target);
void cold_effect(void *vo, int level, int dam, int target);
void fire_effect(void *vo, int level, int dam, int target);
void poison_effect(void *vo, int level, int dam, int target);
void shock_effect(void *vo, int level, int dam, int target);
bool vorpal_effect(CHAR_DATA * ch, CHAR_DATA * victim, GAMEOBJECT * wield);

/* fight.c */
bool is_safe(CHAR_DATA * ch, CHAR_DATA * victim);
bool is_safe_spell(CHAR_DATA * ch, CHAR_DATA * victim, bool area);
void violence_update(void);
void multi_hit(CHAR_DATA * ch, CHAR_DATA * victim, int dt);
int damage(CHAR_DATA * ch, CHAR_DATA * victim, int dam, int dt, int class, bool show);
void update_pos(CHAR_DATA * victim);
void stop_fighting(CHAR_DATA * ch, bool fBoth);
void check_killer(CHAR_DATA * ch, CHAR_DATA * victim);
void raw_kill(CHAR_DATA * victim, /*@null@*/CHAR_DATA * killer);
ONE_ATTACK_RESULT one_attack(CHAR_DATA *ch, CHAR_DATA *victim, int dt, /*@null@*/GAMEOBJECT *wield);


/* handler.c */
void cancel_snoops(DESCRIPTOR_DATA *snooper);
AFFECT_DATA *affect_find(AFFECT_DATA * paf, SKILL * skill);
void affect_check(CHAR_DATA * ch, int where, long vector);
int count_users(GAMEOBJECT * obj);
void deduct_cost(CHAR_DATA * ch, unsigned int cost);
void affect_enchant(GAMEOBJECT * obj);
int check_immune(CHAR_DATA * ch, int dam_type);
int weapon_lookup(const char *name);
int weapon_type(const char *name);
char *weapon_name(int weapon_Type);
char *item_name(int item_type);
int attack_lookup(const char *name);
long wiznet_lookup(const char *name);
long impnet_lookup(const char *name);
int class_lookup(const char *name);
int get_weapon_sn(CHAR_DATA * ch, /*@null@*/GAMEOBJECT * wield);
int get_weapon_skill(CHAR_DATA * ch, int sn);
int get_age(CHAR_DATA * ch);
void reset_char(CHAR_DATA * ch);
int get_trust(CHAR_DATA * ch);
int get_hours_played(CHAR_DATA * ch);
int get_minutes_played(CHAR_DATA * ch);
int get_seconds_played(CHAR_DATA * ch);
int get_session_hours(CHAR_DATA * ch);
int get_session_minutes(CHAR_DATA * ch);
int get_session_seconds(CHAR_DATA * ch);
int get_curr_stat(CHAR_DATA * ch, int stat);
int get_max_train(CHAR_DATA * ch, int stat);
int can_carry_n(CHAR_DATA * ch);
int can_carry_w(CHAR_DATA * ch);
int get_wield_weight(CHAR_DATA * ch);
bool is_name(const char *str, const char *namelist);
void char_from_room(CHAR_DATA * ch);
void char_to_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
void obj_to_char(GAMEOBJECT * obj, CHAR_DATA * ch);
void obj_from_char(GAMEOBJECT * obj);
long apply_ac(GAMEOBJECT * obj, int iWear, int type);
GAMEOBJECT *get_eq_char(CHAR_DATA * ch, int iWear);
void equip_char(CHAR_DATA * ch, GAMEOBJECT * obj, int iWear);
void unequip_char(CHAR_DATA * ch, GAMEOBJECT * obj);
int count_obj_list(OBJECTPROTOTYPE * obj, GAMEOBJECT * list);
void obj_from_room(GAMEOBJECT * obj);
void obj_to_room(GAMEOBJECT * obj, ROOM_INDEX_DATA * pRoomIndex);
void obj_to_obj(GAMEOBJECT * obj, GAMEOBJECT * obj_to);
void obj_from_obj(GAMEOBJECT * obj);
void extract_obj(GAMEOBJECT * obj);
void extract_char(CHAR_DATA * ch, bool fPull);
CHAR_DATA *get_char_room(CHAR_DATA * ch, const char *argument);
CHAR_DATA *get_char_world(CHAR_DATA * ch, const char *argument);
GAMEOBJECT *get_obj_type(OBJECTPROTOTYPE * objprototypeData);
GAMEOBJECT *get_obj_list(CHAR_DATA * ch, const char *argument, GAMEOBJECT * list);
GAMEOBJECT *get_obj_carry(CHAR_DATA * ch, const char *argument);
GAMEOBJECT *get_obj_wear(CHAR_DATA * ch, const char *argument);
GAMEOBJECT *get_obj_here(CHAR_DATA * ch, const char *argument);
GAMEOBJECT *get_obj_world(CHAR_DATA * ch, const char *argument);
GAMEOBJECT *create_money(unsigned int gold, unsigned int silver);
int get_obj_number(GAMEOBJECT * obj);
int get_obj_weight(GAMEOBJECT * obj);
int get_true_weight(GAMEOBJECT * obj);
bool room_is_dark(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
bool is_room_owner(CHAR_DATA * ch, ROOM_INDEX_DATA * room);
bool room_is_private(ROOM_INDEX_DATA * pRoomIndex);
bool can_see(CHAR_DATA * ch, CHAR_DATA * victim);
bool can_see_obj(CHAR_DATA * ch, GAMEOBJECT * obj);
bool can_see_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
bool can_drop_obj(CHAR_DATA * ch, GAMEOBJECT * obj);
char *item_type_name(GAMEOBJECT * obj);
char *affect_loc_name(long location);
char *affect_bit_name(long vector);
char *exit_bit_name(long vector);
char *extra_bit_name(long extra_flags);
char *extra2_bit_name(long extra2_flags);
char *wear_bit_name(long wear_flags);
char *act_bit_name(long act_flags);
char *off_bit_name(long off_flags);
char *imm_bit_name(long imm_flags);
char *form_bit_name(long form_flags);
char *part_bit_name(long part_flags);
char *weapon_bit_name(long weapon_flags);
char *comm_bit_name(long comm_flags);
char *cont_bit_name(long cont_flags);
char *token_bit_name(long token_flags);
const char *first_arg(const char *argument, char *arg_first, bool fCase);
char *room_flag_bit_name(ROOM_INDEX_DATA * room);
char *uncolor_str(char *txt);
void identify_item(CHAR_DATA * ch, GAMEOBJECT * obj);
bool is_help(const char *argument);
void furniture_check(CHAR_DATA * ch);
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, const char *arg);
ROOM_INDEX_DATA *get_death_room(CHAR_DATA * ch);

/* affects.c */
void affect_to_char(CHAR_DATA * ch, /*@partial@*/AFFECT_DATA * paf);
void affect_to_obj(GAMEOBJECT * obj, AFFECT_DATA * paf);
void affect_to_room(ROOM_INDEX_DATA * room, AFFECT_DATA * paf);
void affect_remove(CHAR_DATA * ch, AFFECT_DATA * paf);
void affect_remove_obj(GAMEOBJECT * obj, AFFECT_DATA * paf);
void affect_remove_room(ROOM_INDEX_DATA * room, AFFECT_DATA * paf);
void affect_strip(CHAR_DATA * ch, SKILL * skill);
void affect_strip_room(ROOM_INDEX_DATA * room, int sn);
void affect_join(CHAR_DATA * ch, AFFECT_DATA * paf);
bool is_affected(CHAR_DATA * ch, SKILL * skill);
bool is_affected_room(ROOM_INDEX_DATA * room, SKILL * skill);

/* rooms.c */
char *room_affect(AFFECT_DATA * paf);

/* interp.c */
void interpret(CHAR_DATA * ch, const char *argument);
int number_argument(const char *argument, char *arg);
int mult_argument(const char *argument, char *arg);
const char *one_argument(const char *argument, /*@out@*/ char *arg_first);
char *one_line(char *base, char *buf);

/* magic.c */
void remove_all_affects(CHAR_DATA * victim);
int find_spell(CHAR_DATA * ch, const char *name);
int mana_cost(CHAR_DATA * ch, int min_mana, int level);
bool saves_spell(int level, CHAR_DATA * victim, int dam_type);
void obj_cast_spell(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, GAMEOBJECT * obj);
bool can_trans_room(CHAR_DATA * ch, CHAR_DATA * victim, int sn);

/* save.c */
void save_char_obj(CHAR_DATA * ch);
bool load_char_obj(DESCRIPTOR_DATA * d, char *name);
bool load_char_obj_2(CHAR_DATA * tempch, char *name);


/* skills.c */
bool parse_gen_groups(CHAR_DATA * ch, const char *argument);
void list_group_costs(CHAR_DATA * ch);
void list_group_known(CHAR_DATA * ch);
int exp_per_level(CHAR_DATA * ch, int points);
void check_improve(CHAR_DATA * ch, SKILL * skill, bool success, int multiplier);

/* teleport.c */
ROOM_INDEX_DATA *room_by_name(char *target, int level, bool error);

/* update.c */
void advance_level(CHAR_DATA * ch, int level);
void advance_level2(CHAR_DATA * ch);
void gain_exp(CHAR_DATA * ch, int gain);
void gain_object_exp(CHAR_DATA * ch, GAMEOBJECT * obj, int gain);
void gain_condition(CHAR_DATA * ch, int condition, long value);
void update_handler(void);
void restore_char(CHAR_DATA * ch);
void strip_negative_affects(CHAR_DATA * ch);

/* lookup.c */
int race_lookup(const char *name);
int item_lookup(const char *name);
int liq_lookup(const char *name);


/* logging.c */
#define LOG_SINK_ALWAYS 1
#define LOG_SINK_ALL 2
#define LOG_SINK_PLAYER 3
#define LOG_SINK_LASTCMD 4

void append_file(CHAR_DATA *ch, const char *file, const char *str);
void log_to(int log, char username[], const char *fmt, ...);
void log_bug(const char *fmt, ...);
void log_string(const char *fmt, ...);
/* ~logging.c */


/* Needs a home */
char *capitalize(const char *str);

/* descriptor.c */
typedef struct descriptor_iterator_filter DESCRIPTOR_ITERATOR_FILTER;
struct descriptor_iterator_filter {
    bool all;
    bool must_playing;
    /*@null@*/CHAR_DATA *skip_character;
    SOCKET descriptor;
};
extern const DESCRIPTOR_ITERATOR_FILTER descriptor_empty_filter;

/*@dependent@*/DESCRIPTOR_DATA * descriptor_new(SOCKET descriptor);
void descriptor_free(/*@owned@*/DESCRIPTOR_DATA * d);
int descriptor_list_count();
/*@dependent@*//*@null@*/DESCRIPTOR_DATA *descriptor_iterator_start(const DESCRIPTOR_ITERATOR_FILTER *filter);
/*@dependent@*//*@null@*/DESCRIPTOR_DATA *descriptor_iterator(DESCRIPTOR_DATA *current, const DESCRIPTOR_ITERATOR_FILTER *filter);
/* ~descriptor.c */


/* objectprototype.c */
typedef struct objectprototype_filter OBJECTPROTOTYPE_FILTER;
struct objectprototype_filter {
    /*@null@*/const char *name;
};
extern const OBJECTPROTOTYPE_FILTER objectprototype_empty_filter;

/*@dependent@*/OBJECTPROTOTYPE *objectprototype_new(long vnum);
void objectprototype_free(/*@owned@*/OBJECTPROTOTYPE *templatedata);
int objectprototype_list_count();
/*@dependent@*//*@null@*/OBJECTPROTOTYPE *objectprototype_iterator_start(const OBJECTPROTOTYPE_FILTER *filter);
/*@dependent@*//*@null@*/OBJECTPROTOTYPE *objectprototype_iterator(OBJECTPROTOTYPE *current, const OBJECTPROTOTYPE_FILTER *filter);
/*@dependent@*//*@null@*/OBJECTPROTOTYPE *objectprototype_getbyvnum(long vnum);
/*@only@*/KEYVALUEPAIR_ARRAY *objectprototype_serialize(const OBJECTPROTOTYPE *obj);
/*@dependent@*/OBJECTPROTOTYPE *objectprototype_deserialize(const KEYVALUEPAIR_ARRAY *data);
/* ~objectprototype.c */

#endif  /* __MERC_H */
