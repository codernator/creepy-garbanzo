/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                          *
*   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*                                                                          *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc       *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                   thought has gone into this software and you are        *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

#include <time.h>
#include <stdbool.h>


#if !defined(__MERC_H)
#define __MERC_H

#define DECLARE_DO_FUN(fun)             DO_FUN fun
#define DECLARE_SPEC_FUN(fun)           SPEC_FUN fun
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
#if !defined(FALSE)
#define FALSE    0
#endif

#if !defined(TRUE)
#define TRUE     (!FALSE)
#endif

typedef int sh_int;
typedef unsigned char byte;

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
typedef struct nickname_data NICKNAME_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;
typedef struct war_data WAR_DATA;
typedef struct disabled_data DISABLED_DATA;
typedef struct auction_data AUCTION_DATA;

typedef struct mprog_list MPROG_LIST;
typedef struct mprog_code MPROG_CODE;
typedef struct help_area_data HELP_AREA;
typedef struct social_type SOCIAL;
typedef struct lyric_type LYRIC;
typedef struct song_type SONG;


/* required for new skill system */
#include "skills.h"

/* Function types. */
typedef void DO_FUN(/*@partial@*/CHAR_DATA * ch, char *argument);
typedef bool SPEC_FUN(/*@partial@*/CHAR_DATA * ch);


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
#define MAX_IN_GROUP            15
#define MAX_ALIAS               200
#define MAX_CLASS               15
#define MAX_PC_RACE             15
#define MAX_LEVEL               610
#define MAX_GET                 30
#define MAX_IGNORE              5
#define MAX_RANK                11
#define MAX_FORGET              10
#define MIN_POINTS              76
#define VERSION                 3
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
#define PULSE_BATTLEFIELD       (40 * PULSE_PER_SECOND)
#define PULSE_UNDERWATER        (5 * PULSE_PER_SECOND)

#define IMPLEMENTOR             MAX_LEVEL
#define CREATOR                 (MAX_LEVEL - 1)
#define SUPREME                 (MAX_LEVEL - 2)
#define DEITY                   (MAX_LEVEL - 3)
#define GOD                     (MAX_LEVEL - 4)
#define IMMORTAL                (MAX_LEVEL - 5)
#define DEMI                    (MAX_LEVEL - 6)
#define ANGEL                   (MAX_LEVEL - 7)
#define AVATAR                  (MAX_LEVEL - 8)
#define HERO                    LEVEL_HERO
#define MAX_VNUMS               2147483647l


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


/* Structure for a social in the socials table. */
struct social_type {
    SOCIAL *next;
    char * name;
    char * char_no_arg;
    char * others_no_arg;
    char * char_found;
    char * others_found;
    char * vict_found;
    char * char_auto;
    char * others_auto;
    bool valid;
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
#define CON_GET_ALIGNMENT       9
#define CON_DEFAULT_CHOICE      10
#define CON_GEN_GROUPS          11
#define CON_PICK_WEAPON         12
#define CON_READ_IMOTD          13
#define CON_READ_MOTD           14
#define CON_BREAK_CONNECT       15
#define CON_GET_ANSI            16
#define CON_COPYOVER_RECOVER    17
#define CON_PKILL_CHOICE        18

struct nickname_data {
    NICKNAME_DATA * next;
    char *  name;
    char *  nickname;
};


/* Descriptor (channel) structure. */
struct descriptor_data {
    DESCRIPTOR_DATA *next;
    DESCRIPTOR_DATA *previous;
    DESCRIPTOR_DATA *snoop_by;
    CHAR_DATA *character;
    CHAR_DATA *original;
    bool valid;
    char *host;
    SOCKET descriptor;
    int connected;
    bool fcommand;
    char inbuf[4 * MIL];
    char incomm[MIL];
    char inlast[MIL];
    int repeat;
    char *outbuf;
    int outsize;
    int outtop;
    char *showstr_head;
    char *showstr_point;
    int ifd;
    pid_t ipid;
    char *ident;
    int port;
    int ip;
    int auth_inc;
    int auth_state;
    char abuf[256];
    int auth_fd;
    char *user;
    int atimes;
    void *ed_data;
    char **ed_string;
    int editor;
    int idle;
};


#define DISABLED_CMD    1
#define DISABLED_SPELL  2

struct disabled_data {
 DISABLED_DATA * next;
 char *command;
 char *disabled_by;
 int level;
 int type;
 bool valid;
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
 char * name;                   /* the full name of the class */
 char who_name[4];            /* three-letter name for 'who */
 int attr_prime;             /* prime attribute */
 long weapon;                 /* first weapon */
 long guild[MAX_GUILD];       /* vnum of guild rooms */
 int skill_adept;            /* maximum skill level */
 int thac0_00;               /* thac0 for level  0 */
 int thac0_32;               /* thac0 for level 32 */
 int hp_min;                 /* min hp gained on leveling */
 int hp_max;                 /* max hp gained on leveling */
 int mana_min;               /* min mana gained on leveling */
 int mana_max;               /* max mana gained on leveling */
 bool fMana;                  /* class gains mana on level */
 char * base_group;             /* base skills gained */
 char * default_group;          /* default skills gained */

 /* New Melee Combat Math */
 float attack_rating;
 float ar_improve;
 float defense_rating;
 float dr_improve;

 /* New Structure Parts, Added by Monrick 3/31/2008 */
 bool canCreate;      /* class can be chosen at creation */
 char * shortDesc;      /* description for help file */
 int dAlign;         /* default alignment */
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



struct spec_type {
 char *  name;                   /* special function name */
 SPEC_FUN * function;               /* the function */
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

/*auction_data structure*/
struct auction_data {
 OBJ_DATA * item;
 CHAR_DATA * seller;
 CHAR_DATA * buyer;
 unsigned int bet;
 int  going;
 int  pulse;
 unsigned int reserve;
 int  type;
};

/* auction type information*/
struct auction_types {
 char *  name;
 char *  display;
 int  type;
 unsigned int min_reserve;
 unsigned int max_reserve;
 bool  is_coins;
};

extern const struct auction_types auction_type_table[];

/* message type information*/
struct message_types {
 int  type;
 char *  name;
 char *  display;
 char *  desc;
 char *  file_name;
 int  post_level;
 time_t  retention;
 NOTE_DATA ** thread;
};

extern const struct message_types message_type_table[];


/* affect_data*/

struct affect_data {
 /*@out@*/AFFECT_DATA * next;
 bool valid;
 /*@shared@*/SKILL *skill;
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


/*  For Ident ..    */
#define FLAG_WRAUTH             1
#define FLAG_AUTH               2


/* BUILDING CODE STARTS HERE
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO           3090l
#define MOB_VNUM_CITYGUARD      3060l
#define MOB_VNUM_VAMPIRE        3404l
#define MOB_VNUM_PIG            3167l
#define MOB_VNUM_FAMILIAR       4l
#define MOB_VNUM_PATROLMAN      2106l
#define GROUP_VNUM_TROLLS       2100l
#define GROUP_VNUM_OGRES        2101l


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
#define ACT_NOALIGN             (U)
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
#define DAM_DISEASE             13
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
#define ASSIST_ALIGN            (Q)
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
#define IMM_DISEASE             (Q)
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
#define RES_DISEASE             (Q)
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
#define VULN_DISEASE            (Q)
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
#define AFF_POLLEN                  (A)
#define AFF_INVISIBLE             (B)
#define AFF_DETECT_EVIL         (C)
#define AFF_DETECT_INVIS        (D)
#define AFF_DETECT_MAGIC        (E)
#define AFF_DRUID_CALL          (F)
#define AFF_DETECT_GOOD         (G)
#define AFF_SANCTUARY             (H)
#define AFF_FAERIE_FIRE         (I)
#define AFF_INFRARED              (J)
#define AFF_CURSE                     (K)
#define AFF_BLIND                     (L)
#define AFF_POISON                  (M)
#define AFF_PROTECT_EVIL        (N)
#define AFF_PROTECT_GOOD        (O)
#define AFF_SNEAK                     (P)
#define AFF_HIDE                      (Q)
#define AFF_SLEEP                     (R)
#define AFF_CHARM                     (S)
#define AFF_FLYING                  (T)
#define AFF_PASS_DOOR             (U)
#define AFF_HASTE                     (V)
#define AFF_CALM                      (W)
#define AFF_PLAGUE                  (X)
#define AFF_WEAKEN                  (Y)
#define AFF_DARK_VISION         (Z)
#define AFF_BERSERK                 (aa)
#define AFF_CALLOUSED             (bb)
#define AFF_SLOW                      (cc)
#define AFF_BURNING       (ff)
#define AFF_PHASED        (hh)
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
#define OBJ_VNUM_SCRATCH                7l
#define OBJ_VNUM_QTOKEN_ONE             26l
#define OBJ_VNUM_QTOKEN_SOME            27l

#define OBJ_VNUM_CORPSE_NPC             10l
#define OBJ_VNUM_CORPSE_PC              11l
#define OBJ_VNUM_SEVERED_HEAD           12l
#define OBJ_VNUM_TORN_HEART             13l
#define OBJ_VNUM_SLICED_ARM             14l
#define OBJ_VNUM_SLICED_LEG             15l
#define OBJ_VNUM_GUTS                   16l
#define OBJ_VNUM_BRAINS                 17l
#define OBJ_VNUM_MUSHROOM               20l
#define OBJ_VNUM_LIGHT_BALL             21l
#define OBJ_VNUM_SPRING                 22l
#define OBJ_VNUM_DISC                   23l
#define OBJ_VNUM_PORTAL                 25l
#define OBJ_VNUM_WEED                   472l
#define OBJ_VNUM_ROSE                   1001l
#define OBJ_VNUM_PIT                    3010l
#define OBJ_VNUM_SCROLL                 7817l
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
#define OBJ_VNUM_WHISTLE                2116l
#define OBJ_VNUM_RECEIPT                29l
#define OBJ_VNUM_FIREBLADE              30l
#define OBJ_VNUM_FAERIE_FOG             1201l
#define OBJ_VNUM_BLANK_PILL             14241l          /* Anonplis.are */
#define OBJ_VNUM_SHIT 8117l

/* Item types. * Used in #OBJECTS. */
#define ITEM_LIGHT                      1
#define ITEM_SCROLL                     2
#define ITEM_WAND                         3
#define ITEM_STAFF                      4
#define ITEM_WEAPON                     5
#define ITEM_SCRATCHOFF 6
#define ITEM_TREASURE           8
#define ITEM_ARMOR                      9
#define ITEM_POTION                     10
#define ITEM_CLOTHING           11
#define ITEM_FURNITURE  12
#define ITEM_TRASH                      13
#define ITEM_CONTAINER  15
#define ITEM_DRINK_CON  17
#define ITEM_KEY                          18
#define ITEM_FOOD                         19
#define ITEM_MONEY                      20
#define ITEM_BOAT                         22
#define ITEM_CORPSE_NPC 23
#define ITEM_CORPSE_PC  24
#define ITEM_FOUNTAIN           25
#define ITEM_PILL                         26
#define ITEM_PROTECT            27
#define ITEM_MAP                          28
#define ITEM_PORTAL                     29
#define ITEM_WARP_STONE 30
#define ITEM_ROOM_KEY           31
#define ITEM_GEM                          32
#define ITEM_JEWELRY            33
#define ITEM_QTOKEN                     35
#define ITEM_TELEPORT           36
#define ITEM_ATM                          37
#define ITEM_INVITATION 38
#define ITEM_SCARAB                     39
#define ITEM_FAERIE_FOG 42
#define ITEM_DUST                         43
#define ITEM_DOLL                         44
#define ITEM_QTOKEN2            45
#define ITEM_SOCKETS            46
#define ITEM_DICE                         47
#define ITEM_RELIC      48

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
#define ITEM_EVIL               (E)
#define ITEM_INVIS              (F)
#define ITEM_MAGIC              (G)
#define ITEM_NODROP             (H)
#define ITEM_BLESS              (I)
#define ITEM_ANTI_GOOD          (J)
#define ITEM_ANTI_EVIL          (K)
#define ITEM_ANTI_NEUTRAL       (L)
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
#define ITEM2_GRAFT     (A)
#define ITEM2_ETHEREAL  (B)
#define ITEM2_NOSCAN    (C)
#define ITEM2_RELIC     (D)
#define ITEM2_NODONATE  (E)
#define ITEM2_NORESTRING        (F)

/* Wear flags. * Used in #OBJECTS. */
#define ITEM_TAKE                     (A)
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
#define ITEM_WIELD        (N)
#define ITEM_HOLD               (O)
#define ITEM_NO_SAC           (P)
#define ITEM_WEAR_FLOAT         (Q)
#define ITEM_WEAR_EAR       (R)
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
#define ROOM_VNUM_BFS           822l
#define ROOM_VNUM_BF_START      800l /* battlefield */
#define ROOM_VNUM_BF_END        891l
#define ROOM_VNUM_WARPREP       899l

/* Room flags. * Used in #ROOMS. */
#define ROOM_DARK               (A)
#define ROOM_NORANDOM           (B)
#define ROOM_NO_MOB             (C)
#define ROOM_INDOORS            (D)

#define ROOM_NO_PUSH_NO_DRAG    (F)
#define ROOM_NODREAM            (G)
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
#define ROOM_BFIELD             (Z)
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
#define PLR_IS_NPC              (A)     /* Don't EVER set.        */

/* RT auto flags */
#define PLR_KILLER              (B)
#define PLR_AUTOASSIST          (C)
#define PLR_AUTOEXIT            (D)
#define PLR_AUTOLOOT            (E)
#define PLR_AUTOSAC             (F)
#define PLR_AUTOGOLD            (G)
#define PLR_AUTOSPLIT           (H)
#define PLR_PUNISHMENT          (J)
#define PLR_LINKDEAD            (L)
#define PLR_BATTLE              (M)

/* RT personal flags */
#define PLR_HOLYLIGHT           (N)
#define PLR_IT                  (O)
#define PLR_CANLOOT             (P)
#define PLR_NOSUMMON            (Q)
#define PLR_NOFOLLOW            (R)
#define PLR_AUTOEQ              (V)

/* penalty flags */
#define PLR_PERMIT              (U)
#define PLR_LOG                 (W)
#define PLR_DENY                (X)
#define PLR_FREEZE              (Y)
#define PLR_THIEF               (Z)
#define PLR_CHALLENGER          (aa)
#define PLR_PENDING             (bb)
#define PLR_DUELIST             (cc)
#define PLR_ESP                 (dd)
#define PLR_NORESTORE           (ee)

/* RT comm flags -- may be used n both mobs and chars */
#define COMM_QUIET              (A)
#define COMM_DEAF               (B)
#define COMM_NOWIZ              (C)
#define COMM_NOAUCTION          (D)
#define COMM_NOQUOTE            (I)
#define COMM_SHOUTSOFF          (J)
#define COMM_NOOOC              (K)
#define COMM_NOWISH             (M)
#define COMM_TRUE_TRUST         (N)
#define COMM_COMPACT            (O)
#define COMM_BRIEF              (P)
#define COMM_PROMPT             (Q)
#define COMM_COMBINE            (R)
#define COMM_TELNET_GA          (S)
#define COMM_SHOW_AFFECTS       (T)
#define COMM_NOSHOUT            (W)
#define COMM_NOTELL             (X)
#define COMM_NOCHANNELS         (Y)
#define COMM_SNOOP_PROOF        (Z)
#define COMM_NOQUIT             (aa)
#define COMM_CLEADER            (bb)
#define COMM_NOBATTLEFIELD      (cc)
#define COMM2_AFK               (A)
#define COMM2_INFO              (B)
#define COMM2_NOEMOTE           (C)
#define COMM2_SECRETAGENT       (D)
#define COMM2_IMPTALK           (E)
#define COMM2_TICKS             (F)
#define COMM2_PRIV              (G)
#define COMM2_THIRDARM          (H)
#define COMM2_AUTOTOKEN         (J)
#define COMM2_ENABLE            (K)
#define COMM2_TARGET            (L)
#define COMM2_LOCKREPLY         (M)
#define COMM2_IMPTALKM          (N)
#define COMM2_RP                (O)
#define COMM2_CODING            (P)
#define COMM2_BUILD             (Q)
#define COMM2_NOQCHAT           (R)
#define COMM2_OOC               (S)
#define COMM2_BUSY              (T)


/* WIZnet flags */
#define WIZ_ON                    (A)
#define WIZ_TICKS                 (B)
#define WIZ_LOGINS        (C)
#define WIZ_SITES                 (D)
#define WIZ_LINKS                 (E)
#define WIZ_DEATHS        (F)
#define WIZ_FLAGS                 (I)
#define WIZ_PENALTIES   (J)
#define WIZ_SACCING             (K)
#define WIZ_LEVELS              (L)
#define WIZ_SECURE              (M)
#define WIZ_SWITCHES    (N)
#define WIZ_SNOOPS              (O)
#define WIZ_RESTORE             (P)
#define WIZ_LOAD                  (Q)
#define WIZ_NEWBIE              (R)
#define WIZ_PREFIX              (S)
#define WIZ_SPAM                  (T)
#define WIZ_PLOG                  (U)
#define WIZ_ALOG                  (V)
#define WIZ_ROLEPLAY    (X)

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
 SPEC_FUN * spec_fun;
 SHOP_DATA * shop;
 MPROG_LIST * mprogs;
 AREA_DATA * area;
 long  vnum;
 long  group;
 bool  new_format;
 int  count;
 int  killed;
 char *  player_name;
 char *  short_descr;
 char *  long_descr;
 char *  description;
 long  act;
 long  affected_by;
 int  alignment;
 int  level;
 int  hitroll;
 int  hit[3];
 int  mana[3];
 int  damage[3];
 long  ac[4];
 int  dam_type;
 long  off_flags;
 long  imm_flags;
 long  res_flags;
 long  vuln_flags;
 int  start_pos;
 int  default_pos;
 int  sex;
 int  race;
 unsigned int wealth;
 long  form;
 long  parts;
 int  size;
 int  drained;
 char *  material;
 long  mprog_flags;
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
    /*@shared@*/CHAR_DATA *next;
    /*@shared@*/CHAR_DATA *next_in_room;
    /*@shared@*/CHAR_DATA *master;
    /*@shared@*/CHAR_DATA *leader;
    /*@shared@*/CHAR_DATA *fighting;
    /*@shared@*/CHAR_DATA *target;
    /*@shared@*/CHAR_DATA *linked;
    /*@shared@*/CHAR_DATA *reply;
    /*@shared@*/CHAR_DATA *pet;
    /*@shared@*/CHAR_DATA *mob_wuss;
    /*@shared@*/CHAR_DATA *mobmem;
    /*@shared@*/CHAR_DATA *dream;
    /*@shared@*/CHAR_DATA *mprog_target;
    /*@shared@*/CHAR_DATA *symbiosis;
    /*@shared@*/MEM_DATA *memory;
    /*@shared@*/SPEC_FUN *spec_fun;
    /*@shared@*//*@null@*/MOB_INDEX_DATA *mob_idx;
    /*@shared@*/DESCRIPTOR_DATA *desc;
    /*@shared@*/AFFECT_DATA *affected;
    /*@shared@*/NOTE_DATA *pnote;
    /*@shared@*/OBJ_DATA *carrying;
    /*@shared@*/OBJ_DATA *on;
    /*@shared@*/ROOM_INDEX_DATA *in_room;
    /*@shared@*/ROOM_INDEX_DATA *was_in_room;
    /*@shared@*/AREA_DATA *zone;
    /*@shared@*/PC_DATA *pcdata;
    /*@shared@*/NICKNAME_DATA *nicknames;
    /*@shared@*/DISABLED_DATA *disabled;
    bool valid;
    bool phased;
    /*@shared@*/char * name;
    long id;
    int version;
    int  vernew;
    /*@shared@*/char *short_descr;
    /*@shared@*/char *long_descr;
    /*@shared@*/char *description;
    /*@shared@*/char *inote;
    /*@shared@*/char *prompt;
    long group;
    int sex;
    int race;
    int class;
    int level;
    int trust;
    int played;
    int drained;
    int lines;                          /* for the pager */
    time_t logon;
    time_t llogoff;
    time_t last_fight;
    int ticks_since_last_fight;
    int timer;
    int wait;
    int daze;
    int hit;
    int max_hit;
    int mana;
    int max_mana;
    int move;
    int max_move;
    unsigned int gold;
    unsigned int silver;
    int exp;
    time_t duel_start;
    int duelwin;
    int duelloss;
    int benter;
    int bkills;
    int bloss;
    int pk_timer;
    int safe_timer;
    long act;
    int color;
    long comm;   /* RT added to pad the vector */
    long comm2;  /* RT added to pad the vector */
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
    int alignment;
    int hitroll;
    int damroll;
    long armor[4];
    long rpoint[11];
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
    int countdown;
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
    /*@shared@*/PC_DATA * next;
    /*@shared@*/CHAR_DATA * tagged_by;
    /*@shared@*/BUFFER * buffer;
    bool  valid;
    /*@shared@*/char *  pwd;
    /*@shared@*/char *  bamfin;
    /*@shared@*/char *  bamfout;
    /*@shared@*/char *  title;
    /*@shared@*/char *  grestore_string;
    /*@shared@*/char *  rrestore_string;
    /*@shared@*/char *  immkiss_string;
    time_t  last_note;
    time_t  last_idea;
    time_t  last_penalty;
    time_t  last_news;
    time_t  last_changes;
    time_t  last_rpnote;
    time_t  last_aucnote;
    time_t  last_build;
    time_t  last_read[NOTE_MAX];
    /*@shared@*/char *  who_thing;
    /*@shared@*/char *  filter[MAX_FORGET];
    int  perm_hit;
    int  perm_mana;
    int  perm_move;
    int  true_sex;
    int  last_level;
    int  tag_ticks;
    long  condition[COND_MAX];
    /*@shared@*/LEARNED * skills;
    int  points;
    bool  confirm_delete;
    bool  confirm_suicide;
    bool  confirm_pkills;
    /*@shared@*/char *  alias[MAX_ALIAS];
    /*@shared@*/char *  alias_sub[MAX_ALIAS];
    /*@shared@*/char *  ignore[MAX_IGNORE];
    byte  color_combat_s;
    byte  color_combat_condition_s;
    byte  color_combat_condition_o;
    byte  color_invis;
    byte  color_wizi;
    byte  color_hp;
    byte  color_combat_o;
    byte  color_hidden;
    byte  color_charmed;
    byte  color_mana;
    byte  color_move;
    byte  color_say;
    byte  color_tell;
    byte  color_reply;
    unsigned int silver_in_bank;
    unsigned int gold_in_bank;
    long  wiznet;
    long  impnet;
    /*@shared@*/char *  prefix;
    long  pkills;
    long  pdeaths;
    long  mobkills;
    long  mobdeaths;
    /*@shared@*/char *  deathcry;
    int  practice;
    int  train;
    time_t  killer_time;
    time_t  thief_time;
    /*@shared@*/char *  afk_message;
    time_t  last_bank;
    int  security;
    int  rank;
    unsigned int twohundred;
    unsigned int armorclass;
    unsigned int fireproof;
    unsigned int weaponflag;
    unsigned int restring;
    unsigned int damnoun;
    unsigned int immhidden;
    unsigned int immwild;
    unsigned int imp;
    unsigned int bounty;
    unsigned int skillset;
    unsigned int rp;
    unsigned int extendedlevel;
    long  extendedexp;
    bool  confirm_pk;
    /*@shared@*/char *  restring_name;
    /*@shared@*/char *  restring_short;
    /*@shared@*/char *  restring_long;
    /*@shared@*/char *  history;
};


struct extra_descr_data {
 EXTRA_DESCR_DATA * next;
 bool   valid;
 char *   keyword;        /* Keyword in look/examine */
 char *   description;    /* What to see    */
};


/***************************************************************************
* object specific structures
***************************************************************************/
struct obj_index_data {
 OBJ_INDEX_DATA * next;
 EXTRA_DESCR_DATA * extra_descr;
 AFFECT_DATA *  affected;
 AREA_DATA *  area;
 bool   new_format;
 char *   name;
 char *   short_descr;
 char *   description;
 long   vnum;
 long   reset_num;
 char *   material;
 int   item_type;
 long   extra_flags;
 long   extra2_flags;
 long   wear_flags;
 int   level;
 int   condition;
 int   timer;
 int   count;
 int   weight;
 int   plevel;
 int   xp_tolevel;
 int   exp;
 unsigned int  cost;
 long   value[5];
};


/***************************************************************************
* object_data* a single instance of an object
***************************************************************************/
struct obj_data {
    OBJ_DATA *  next;
    OBJ_DATA *  next_content;
    OBJ_DATA *  contains;
    OBJ_DATA *  in_obj;
    OBJ_DATA *  on;
    CHAR_DATA *  carried_by;
    CHAR_DATA *  target;
    EXTRA_DESCR_DATA * extra_descr;
    AFFECT_DATA *  affected;
    OBJ_INDEX_DATA * obj_idx;
    ROOM_INDEX_DATA * in_room;
    bool   valid;
    bool   enchanted;
    char *   owner;
    char *   name;
    char *   short_descr;
    char *   description;
    char *   inote;
    int   plevel;
    int   xp_tolevel;
    int   exp;
    int   item_type;
    long   extra_flags;
    long   extra2_flags;
    long   wear_flags;
    int   wear_loc;
    int   weight;
    unsigned int  cost;
    int   level;
    int   condition;
    char *   material;
    int   timer;
    long   value[5];
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
 OBJ_DATA *  contents;
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
#define VALIDATE(data)           ((data)->valid = TRUE)
#define INVALIDATE(data)         ((data)->valid = FALSE)
#define UMIN(a, b)               ((a) < (b) ? (a) : (b))
#define UABS(a)                  ((a) < 0 ? -(a) : (a))
#define UMAX(a, b)               ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)                 ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)                 ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define IS_SET(flag, bit)        ((((flag) & (bit)) == (bit)))
#define SET_BIT(var, bit)        ((var) |= (bit))
#define REMOVE_BIT(var, bit)     ((var) &= ~(bit))
#define IN_RANGE(min, num, max)  (((min) < (num)) && ((num) < (max)))
#define CHECK_POS(a, b, c) \
 { \
  (a) = (b); \
  if ((a) < 0) \
  { \
   bug("CHECK_POS : " c " == %d < 0", (int)a); \
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

#define GET_AGE(ch)             ((int)(17 + ((ch)->played + current_time - (ch)->logon) / 72000))
#define IS_GOOD(ch)             (ch->alignment >= 350)
#define IS_EVIL(ch)             (ch->alignment <= -350)
#define IS_NEUTRAL(ch)          (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_LINK_DEAD(ch)        ((!IS_NPC(ch)) && ch->desc == NULL)
#define IS_AWAKE(ch)            (ch->position > POS_SLEEPING)

/* stat max */
#define GET_HITROLL(ch)         ((ch)->hitroll + (get_curr_stat(ch, STAT_STR) / 2) + get_curr_stat(ch, STAT_DEX))
#define GET_DAMROLL(ch)         ((ch)->damroll + (get_curr_stat(ch, STAT_STR) * 3) / 2)
#define GET_AC(ch, type)         ((ch)->armor[type] + (IS_AWAKE(ch) ? get_curr_stat(ch, STAT_DEX) * -6 : 0))
#define IS_OUTSIDE(ch)          (!IS_SET((ch)->in_room->room_flags, ROOM_INDOORS))


#define WAIT_STATE(ch, npulse)  (set_wait((ch), (npulse)))
#define DAZE_STATE(ch, npulse)  (set_daze((ch), (npulse)))

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
extern const struct     spec_type spec_table[];
extern const struct     liq_type liq_table[];

/* max skill number */
extern int gn_max_skill_sn;
extern int gn_max_group_sn;

/* skills for weapons */
extern SKILL *gsp_axe;
extern SKILL *gsp_dagger;
extern SKILL *gsp_flail;
extern SKILL *gsp_mace;
extern SKILL *gsp_polearm;
extern SKILL *gsp_shield_block;
extern SKILL *gsp_spear;
extern SKILL *gsp_sword;
extern SKILL *gsp_whip;

/* pointers to frequently used skills */
extern SKILL *gsp_hand_to_hand;
extern SKILL *gsp_aggressive_parry;
extern SKILL *gsp_second_attack;
extern SKILL *gsp_third_attack;
extern SKILL *gsp_fourth_attack;
extern SKILL *gsp_fifth_attack;
extern SKILL *gsp_parry;
extern SKILL *gsp_dodge;
extern SKILL *gsp_evade;
extern SKILL *gsp_poison;
extern SKILL *gsp_web;
extern SKILL *gsp_banzai;
extern SKILL *gsp_enhanced_damage;
extern SKILL *gsp_flanking;
extern SKILL *gsp_whirlwind;
extern SKILL *gsp_darkness;
extern SKILL *gsp_invisibility;
extern SKILL *gsp_mass_invisibility;
extern SKILL *gsp_sleep;
extern SKILL *gsp_voodoo;
extern SKILL *gsp_hide;
extern SKILL *gsp_sneak;
extern SKILL *gsp_nexus;
extern SKILL *gsp_portal;
extern SKILL *gsp_gate;
extern SKILL *gsp_blindness;
extern SKILL *gsp_fear;
extern SKILL *gsp_evade;
extern SKILL *gsp_dodge;
extern SKILL *gsp_parry;
extern SKILL *gsp_plague;
extern SKILL *gsp_deft;
extern SKILL *gsp_dash;
extern SKILL *gsp_black_mantle;
extern SKILL *gsp_fast_healing;
extern SKILL *gsp_meditation;
extern SKILL *gsp_black_plague;
extern SKILL *gsp_burning_flames;
extern SKILL *gsp_peek;
extern SKILL *gsp_detect_magic;
extern SKILL *gsp_faerie_fog;
extern SKILL *gsp_haste;
extern SKILL *gsp_haggle;
extern SKILL *gsp_bless;
extern SKILL *gsp_obless;
extern SKILL *gsp_curse;
extern SKILL *gsp_frenzy;


/* race skills */
extern SKILL *gsp_supernatural_speed;
extern SKILL *gsp_tooth_and_claw;
extern SKILL *gsp_veil;
extern SKILL *gsp_anti_magic_aura;
extern SKILL *gsp_dream;


/* room affects */
extern SKILL *gsp_haven;
extern SKILL *gsp_mana_vortex;


struct battlefield_data {
 DISABLED_DATA * disabled;
 int  lroom;
 int  uroom;
 int  llevel;
 int  ulevel;
 char  opened_by[64];
 bool  special;
 bool  dirty;
 bool  open;
 int  participants;
 bool  affected;
 int  open_ticks;
 int  running_ticks;
 int  immortal_opens;
 int  mortal_opens;
};



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

/* Global Constants */
extern char *const dir_name        [];
extern const int rev_dir         [];                 /* int - ROM OLC */
extern const struct  spec_type spec_table      [];


/* Global variables. */
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;
extern SHOP_DATA *shop_last;
extern HELP_DATA *help_first;
extern SHOP_DATA *shop_first;
extern AUCTION_DATA *auction;
extern CHAR_DATA *char_list;
extern DESCRIPTOR_DATA *descriptor_list;
extern OBJ_DATA *object_list;
extern SOCIAL *social_list;


extern MPROG_CODE *mprog_list;
extern DISABLED_DATA *disabled_first;


extern char bug_buf[];
extern time_t current_time;
extern bool log_all;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern int reboot_tick_counter;
extern int copyover_tick_counter;
extern char last_command[MSL];
extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_help;
extern int top_reset;
extern int top_shop;
extern long top_vnum_mob;
extern long top_vnum_obj;
extern long top_vnum_room;
extern char str_empty       [1];
extern bool wizlock;
extern bool newlock;
extern bool debugging;
extern MOB_INDEX_DATA *mob_index_hash  [MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash  [MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash [MAX_KEY_HASH];


//char *crypt(const char *key, const char *salt);

#define crypt(value, salt)   (value)


/***************************************************************************
* data files used by the server.
*
* AREA_LIST contains a list of areas to boot.
* all files are read in completely at bootup.
* most output files (bug, idea, typo, shutdown) are append-only.
*
* the NULL_FILE is held open so that we have a stream handle in reserve,
* so players can go ahead and telnet to all the other descriptors.
* then we close it whenever we need to open a file (e.g. a save file).
***************************************************************************/


#define LAST_COMMAND     "./db/last_command.txt"  /* Tracking commands */
#define PLAYER_DIR       "./db/player/"           /* Player files */
#define GOD_DIR          "./db/gods/"             /* list of gods */
#define TEMP_FILE        "./db/player/romtmp"
#define NULL_FILE        "/dev/null"              /* To reserve one stream */
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
#define DISABLED_FILE    "./db/disabled.txt"      /* disabled commands */
#define HEADLINE_FILE    "./db/headline.txt"
#define HELP_FILE        "./db/area/help.are"
#define SOCIAL_FILE      "./db/SOCIALS.TXT"
#define NOTE_FILE        "./db/notes.not"         /* note thread */
#define SHUTDOWN_FILE    "./shutdown.txt"         /* For 'shutdown' */
#define COPYOVER_FILE    "./copyover.txt"

#define END_MARKER       "END"                    /* for load_disabled() and save_disabled() */


/***************************************************************************
* global function declarations
***************************************************************************/


/***************************************************************************
* act_comm.c
***************************************************************************/
void add_follower(CHAR_DATA * ch, CHAR_DATA * master);
void stop_follower(CHAR_DATA * ch);
void nuke_pets(CHAR_DATA * ch);
void die_follower(CHAR_DATA * ch);
bool is_same_group(CHAR_DATA * ach, CHAR_DATA * bch);
int get_item_apply_val(char *name);
void set_wait(CHAR_DATA * ch, int pulse);
void set_daze(CHAR_DATA * ch, int pulse);
void set_bash(CHAR_DATA * ch, int pulse);


/* act_enter.c */
ROOM_INDEX_DATA *get_random_room(CHAR_DATA * ch, AREA_DATA * area);

/* act_info.c */
void set_title(CHAR_DATA * ch, char *title);
void set_seek(CHAR_DATA * ch, char *seek);
void show_list_to_char(OBJ_DATA * list, CHAR_DATA * ch, bool fShort, bool fShowNothing);
void show_damage_display(CHAR_DATA * ch, CHAR_DATA * victim);

/* act_move.c */
void move_char(CHAR_DATA * ch, int door, bool follow);
void push_char(CHAR_DATA * ch, CHAR_DATA * vch, int door, bool follow);
void drag_char(CHAR_DATA * ch, CHAR_DATA * victim, int door, bool follow);
int find_door(CHAR_DATA * ch, char *arg);
int find_exit(CHAR_DATA * ch, char *arg);

/* act_obj.c */
bool can_loot(CHAR_DATA * ch, OBJ_DATA * obj);
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);

/* act_wiz.c */
void wiznet(char *string, /*@null@*/ CHAR_DATA * ch, /*@null@*/ OBJ_DATA * obj, long flag, long flag_skip, int min_level);
void impnet(char *string, CHAR_DATA * ch, OBJ_DATA * obj, long flag, long flag_skip, int min_level);
void copyover_recover(void);

/* alias.c */
void substitute_alias(DESCRIPTOR_DATA * d, char *input);


/* ban.c */
bool check_ban(char *site, int type);

/* comm.c */
void show_string(struct descriptor_data *d, char *input);
void close_socket(DESCRIPTOR_DATA * dclose);
void write_to_buffer(DESCRIPTOR_DATA * d, const char *txt, int length);
void send_to_char(char *txt, /*@partial@*/CHAR_DATA * ch);
void send_to_char_ascii(char *txt, /*@partial@*/CHAR_DATA * ch);
void page_to_char(char *txt, /*@partial@*/CHAR_DATA * ch);
void act(const char *format, /*@partial@*/CHAR_DATA * ch, /*@null@*/const void *arg1, /*@null@*/const void *arg2, int type);
void act_new(const char *format, /*@partial@*/CHAR_DATA * ch, /*@null@*/const void *arg1, /*@null@*/const void *arg2, int type, int min_pos, bool mob_trigger);

/* nanny.c */
void nanny(DESCRIPTOR_DATA * d, char *argument);
bool check_parse_name(char *name);
bool check_reconnect(DESCRIPTOR_DATA * d, char *name, bool fConn);
bool check_playing(DESCRIPTOR_DATA * d, char *name);
void stop_idling(CHAR_DATA * ch);

/* db.c */
char *print_flags(long flag);
void boot_db(void);
void area_update(void);

/* creation/cloning */
CHAR_DATA *create_mobile(MOB_INDEX_DATA * mob_idx);
void clone_mobile(CHAR_DATA * parent, CHAR_DATA * clone);
OBJ_DATA *create_object(OBJ_INDEX_DATA * obj_idx, int level);
void clone_object(OBJ_DATA * parent, OBJ_DATA * clone);
void clear_char(CHAR_DATA * ch);

/* find functions  */
/*@shared@*/char *get_extra_descr(const char *name, EXTRA_DESCR_DATA * ed);
/*@shared@*/MOB_INDEX_DATA *get_mob_index(long vnum);
/*@shared@*/OBJ_INDEX_DATA *get_obj_index(long vnum);
/*@shared@*/ROOM_INDEX_DATA *get_room_index(long vnum);


/* memory management */
/*@shared@*/void *alloc_mem(unsigned int sMem);
/*@only@*/void *alloc_perm(unsigned int sMem);
void free_mem(void *pMem, unsigned int sMem);
/*@shared@*//*@only@*/char *str_dup(const char *str);
void free_string(char *pstr);

/* number manipulation */
int number_fuzzy(int number);
long number_fuzzy_long(long number);
int number_range(int from, int to);
int number_percent(void);
int number_door(void);
int number_bits(unsigned int width);
long number_mm(void);
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
void smash_tilde(char *str);

/* string manipulation */
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
char *str_replace(char *orig, char *find, char *replace);

/* misc utility func. */
char *capitalize(const char *str);
void append_file(CHAR_DATA * ch, char *file, char *str);
void bug(const char *str, int param);
void log_string(const char *str);
void log_new(const char *log, const char *str, char *username);
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
bool vorpal_effect(CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * wield);

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
ONE_ATTACK_RESULT one_attack(CHAR_DATA *ch, CHAR_DATA *victim, int dt, /*@null@*/OBJ_DATA *wield);


/* handler.c */
AFFECT_DATA *affect_find(AFFECT_DATA * paf, SKILL * skill);
void affect_check(CHAR_DATA * ch, int where, long vector);
int count_users(OBJ_DATA * obj);
void deduct_cost(CHAR_DATA * ch, unsigned int cost);
void affect_enchant(OBJ_DATA * obj);
int check_immune(CHAR_DATA * ch, int dam_type);
int material_lookup(const char *name);
int weapon_lookup(const char *name);
int weapon_type(const char *name);
char *weapon_name(int weapon_Type);
char *item_name(int item_type);
int attack_lookup(const char *name);
long wiznet_lookup(const char *name);
long impnet_lookup(const char *name);
int class_lookup(const char *name);
int get_weapon_sn(CHAR_DATA * ch, /*@null@*/OBJ_DATA * wield);
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
bool is_name(char *str, char *namelist);
void char_from_room(CHAR_DATA * ch);
void char_to_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
void obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch);
void obj_from_char(OBJ_DATA * obj);
long apply_ac(OBJ_DATA * obj, int iWear, int type);
/*@shared@*/OBJ_DATA *get_eq_char(CHAR_DATA * ch, int iWear);
void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int iWear);
void unequip_char(CHAR_DATA * ch, OBJ_DATA * obj);
int count_obj_list(OBJ_INDEX_DATA * obj, OBJ_DATA * list);
void obj_from_room(OBJ_DATA * obj);
void obj_to_room(OBJ_DATA * obj, ROOM_INDEX_DATA * pRoomIndex);
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to);
void obj_from_obj(OBJ_DATA * obj);
void extract_obj(OBJ_DATA * obj);
void extract_char(CHAR_DATA * ch, bool fPull);
/*@shared@*/CHAR_DATA *get_char_room(CHAR_DATA * ch, char *argument);
/*@shared@*/CHAR_DATA *get_char_world(CHAR_DATA * ch, char *argument);
/*@shared@*/OBJ_DATA *get_obj_type(OBJ_INDEX_DATA * obj_idxData);
/*@shared@*/OBJ_DATA *get_obj_list(CHAR_DATA * ch, char *argument, OBJ_DATA * list);
/*@shared@*/OBJ_DATA *get_obj_carry(CHAR_DATA * ch, char *argument);
/*@shared@*/OBJ_DATA *get_obj_wear(CHAR_DATA * ch, char *argument);
/*@shared@*/OBJ_DATA *get_obj_here(CHAR_DATA * ch, char *argument);
/*@shared@*/OBJ_DATA *get_obj_world(CHAR_DATA * ch, char *argument);
/*@shared@*/OBJ_DATA *create_money(unsigned int gold, unsigned int silver);
int get_obj_number(OBJ_DATA * obj);
int get_obj_weight(OBJ_DATA * obj);
int get_true_weight(OBJ_DATA * obj);
bool room_is_dark(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
bool is_room_owner(CHAR_DATA * ch, ROOM_INDEX_DATA * room);
bool room_is_private(ROOM_INDEX_DATA * pRoomIndex);
bool room_is_bfield(ROOM_INDEX_DATA * pRoomIndex);
bool can_see(CHAR_DATA * ch, CHAR_DATA * victim);
bool can_see_obj(CHAR_DATA * ch, OBJ_DATA * obj);
bool can_see_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex);
bool can_drop_obj(CHAR_DATA * ch, OBJ_DATA * obj);
char *item_type_name(OBJ_DATA * obj);
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
char *first_arg(char *argument, char *arg_first, bool fCase);
char *room_flag_bit_name(ROOM_INDEX_DATA * room);
char *uncolor_str(char *txt);
void identify_item(CHAR_DATA * ch, OBJ_DATA * obj);
bool is_help(char *argument);
void furniture_check(CHAR_DATA * ch);
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);
ROOM_INDEX_DATA *get_death_room(CHAR_DATA * ch);

/* affects.c */
void affect_to_char(CHAR_DATA * ch, /*@partial@*/AFFECT_DATA * paf);
void affect_to_obj(OBJ_DATA * obj, AFFECT_DATA * paf);
void affect_to_room(ROOM_INDEX_DATA * room, AFFECT_DATA * paf);
void affect_remove(CHAR_DATA * ch, AFFECT_DATA * paf);
void affect_remove_obj(OBJ_DATA * obj, AFFECT_DATA * paf);
void affect_remove_room(ROOM_INDEX_DATA * room, AFFECT_DATA * paf);
void affect_strip(CHAR_DATA * ch, SKILL * skill);
void affect_strip_room(ROOM_INDEX_DATA * room, int sn);
void affect_join(CHAR_DATA * ch, AFFECT_DATA * paf);
bool is_affected(CHAR_DATA * ch, SKILL * skill);
bool is_affected_room(ROOM_INDEX_DATA * room, SKILL * skill);

/* rooms.c */
char *room_affect(AFFECT_DATA * paf);

/* interp.c */
void interpret(CHAR_DATA * ch, char *argument);
int number_argument(char *argument, char *arg);
int mult_argument(char *argument, char *arg);
/*@shared@*/char *one_argument(char *argument, /*@out@*/ char *arg_first);
/*@shared@*/char *one_line(char *base, char *buf);

/* magic.c */
void remove_all_affects(CHAR_DATA * victim);
int find_spell(CHAR_DATA * ch, const char *name);
int mana_cost(CHAR_DATA * ch, int min_mana, int level);
bool saves_spell(int level, CHAR_DATA * victim, int dam_type);
void obj_cast_spell(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj);
bool can_trans_room(CHAR_DATA * ch, CHAR_DATA * victim, int sn);

/* nickname.c */
void add_to_nicknames(CHAR_DATA * ch, char *nickname, char *name);
void free_nicknames(CHAR_DATA * ch);
char *check_nickname(CHAR_DATA * ch, char *nickname);

/* save.c */
void save_char_obj(CHAR_DATA * ch);
bool load_char_obj(DESCRIPTOR_DATA * d, char *name);
bool load_char_obj_2(CHAR_DATA * tempch, char *name);


/* skills.c */
bool parse_gen_groups(CHAR_DATA * ch, char *argument);
void list_group_costs(CHAR_DATA * ch);
void list_group_known(CHAR_DATA * ch);
int exp_per_level(CHAR_DATA * ch, int points);
void check_improve(CHAR_DATA * ch, SKILL * skill, bool success, int multiplier);



/* special.c */
SPEC_FUN *spec_lookup(const char *name);
char *spec_name(SPEC_FUN * function);

/* teleport.c */
ROOM_INDEX_DATA *room_by_name(char *target, int level, bool error);

/* update.c */
void advance_level(CHAR_DATA * ch, int level);
void advance_level2(CHAR_DATA * ch);
void gain_exp(CHAR_DATA * ch, int gain);
void gain_object_exp(CHAR_DATA * ch, OBJ_DATA * obj, int gain);
void gain_condition(CHAR_DATA * ch, int condition, long value);
void update_handler(void);
void restore_char(CHAR_DATA * ch);
void strip_negative_affects(CHAR_DATA * ch);





/* disable.c */
DISABLED_DATA *new_disabled(void);
void free_disabled(DISABLED_DATA * disabled);
void disable_show(CHAR_DATA * ch, DISABLED_DATA * list);
void disable_cmd(CHAR_DATA * ch, char *argument, DISABLED_DATA * *disabled_list);
void disable_spell(CHAR_DATA * ch, char *argument, DISABLED_DATA * *disabled_list);
void disable_all(CHAR_DATA * ch, char *argument, DISABLED_DATA * *disabled_list);
bool check_disabled(CHAR_DATA * ch, int type, char *cmd);
void load_disabled(void);
void save_disabled(void);


/* olc */
void printf_to_char(CHAR_DATA *, char *, ...);
void printf_bug(char *, ...);
void printf_log(char *fmt, ...);


/* lookup.c */
int race_lookup(const char *name);
int item_lookup(const char *name);
int liq_lookup(const char *name);

/* object_service.c */
bool is_situpon(/*@partial@*/OBJ_DATA *obj);
bool is_standupon(/*@partial@*/OBJ_DATA *obj);

#endif  /* __MERC_H */

