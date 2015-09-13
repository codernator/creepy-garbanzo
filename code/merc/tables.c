#include "merc.h"
#include "tables.h"


const struct bit_type bitvector_type [] =
{
    { affect_flags,        "affect" },
    { apply_flags,        "apply"  },
    { imm_flags,        "imm"    },
    { res_flags,        "res"    },
    { vuln_flags,        "vuln"   },
    { weapon_flag_type, "weapon" }
};


/***************************************************************************
*    flags used by characters (players/mobs)
***************************************************************************/

const struct position_type position_table[] =
{
    { "dead",          "dead"  },
    { "mortally wounded", "mort"  },
    { "incapacitated",    "incap" },
    { "stunned",          "stun"  },
    { "sleeping",          "sleep" },
    { "resting",          "rest"  },
    { "sitting",          "sit"   },
    { "fighting",          "fight" },
    { "standing",          "stand" },
    { NULL,              NULL    }
};

const struct sex_type sex_table[] =
{
    { "none"   },
    { "male"   },
    { "female" },
    { "either" },
    { NULL       }
};

const struct size_type size_table[] =
{
    { "tiny"   },
    { "small"  },
    { "medium" },
    { "large"  },
    { "huge"   },
    { "giant"  },
    { NULL       }
};

const struct flag_type act_flags[] =
{
    { "npc",       A,         false },
    { "sentinel",       B,         true  },
    { "scavenger",       C,         true  },
    { "aggressive",       F,         true  },
    { "stay_area",       G,         true  },
    { "wimpy",       H,         true  },
    { "pet",       I,         true  },
    { "train",       J,         true  },
    { "practice",       K,         true  },
    { "undead",       O,         true  },
    { "cleric",       Q,         true  },
    { "mage",       R,         true  },
    { "thief",       S,         true  },
    { "warrior",       T,         true  },
    { "nopurge",       V,         true  },
    { "outdoors",       W,         true  },
    { "indoors",       Y,         true  },
    { "exchanger",       Z,         true  },
    { "healer",       aa,         true  },
    { "gain",       bb,         true  },
    { "update_always", cc,         true  },
    { "changer",       dd,         true  },
    { "blacksmith",       ee,         true  },
    { NULL,           0,         false }
};

const struct flag_type plr_flags[] =
{
    { "npc",    A,  false },
    { "autoassist", C,  false },
    { "autoeq",    V,  false },
    { "autoexit",    D,  false },
    { "autoloot",    E,  false },
    { "autosac",    F,  false },
    { "autogold",    G,  false },
    { "autosplit",    H,  false },
    { "holylight",    N,  false },
    { "can_loot",    P,  false },
    { "nosummon",    Q,  false },
    { "nofollow",    R,  false },
    { "permit",    U,  true  },
    { "log",    W,  false },
    { "deny",    X,  false },
    { "freeze",    Y,  false },
    { "thief",    Z,  false },
    { "killer",    aa, false },
    { "peaceful",    dd, false },
    { NULL,        0,  0      }
};

const struct flag_type size_flags[] =
{
    { "tiny",   SIZE_TINY,     true },
    { "small",  SIZE_SMALL,     true },
    { "medium", SIZE_MEDIUM, true },
    { "large",  SIZE_LARGE,     true },
    { "huge",   SIZE_HUGE,     true },
    { "giant",  SIZE_GIANT,     true },
    { NULL,        0,         0    },
};

const struct flag_type position_flags[] =
{
    { "dead",     POS_DEAD,        false },
    { "mortal",   POS_MORTAL,   false },
    { "incap",    POS_INCAP,    false },
    { "stunned",  POS_STUNNED,  false },
    { "sleeping", POS_SLEEPING, true  },
    { "resting",  POS_RESTING,  true  },
    { "sitting",  POS_SITTING,  true  },
    { "fighting", POS_FIGHTING, true  },
    { "standing", POS_STANDING, true  },
    { NULL,          0,        0      }
};

const struct flag_type affect_flags[] =
{
    { "pollen",      A,  true },
    { "invisible",      B,  true },
    { "detect_invis", D,  true },
    { "detect_magic", E,  true },
    { "druid_call",      F,  true },
    { "sanctuary",      H,  true },
    { "faerie_fire",  I,  true },
    { "infrared",      J,  true },
    { "curse",      K,  true },
    { "blind",      L,  true },
    { "poison",      M,  true },
    { "sneak",      P,  true },
    { "hide",      Q,  true },
    { "sleep",      R,  true },
    { "charm",      S,  true },
    { "flying",      T,  true },
    { "pass_door",      U,  true },
    { "haste",      V,  true },
    { "calm",      W,  true },
    { "weaken",      Y,  true },
    { "dark_vision",  Z,  true },
    { "berserk",      aa, true },
    { "slow",      cc, true },
    { "banzai",      ee, true },
    { "calloused",      bb, true },
    { NULL,          0,  0       }
};

const struct flag_type off_flags[] =
{
    { "area_attack",    A, true },
    { "bash",        C, true },
    { "berserk",        D, true },
    { "disarm",        E, true },
    { "dodge",        F, true },
    { "fade",        G, true },
    { "fast",        H, true },
    { "kick",        I, true },
    { "dirt_kick",        J, true },
    { "parry",        K, true },
    { "rescue",        L, true },
    { "tail",        M, true },
    { "trip",        N, true },
    { "crush",        O, true },
    { "assist_all",        P, true },
    { "assist_race",    R, true },
    { "assist_players", S, true },
    { "assist_guard",   T, true },
    { "assist_vnum",    U, true },
    { NULL,            0, 0    }
};

const struct flag_type imm_flags[] = {
    { "summon",    A,  true },
    { "charm",     B,  true },
    { "magic",     C,  true },
    { "weapon",    D,  true },
    { "bash",      E,  true },
    { "pierce",    F,  true },
    { "slash",     G,  true },
    { "fire",      H,  true },
    { "cold",      I,  true },
    { "lightning", J,  true },
    { "acid",      K,  true },
    { "poison",    L,  true },
    { "negative",  M,  true },
    { "holy",      N,  true },
    { "energy",    O,  true },
    { "mental",    P,  true },
    { "drowning",  R,  true },
    { "light",     S,  true },
    { "sound",     T,  true },
    { "wood",      X,  true },
    { "silver",    Y,  true },
    { "iron",      Z,  true },
    { "illusion",  aa, true },
    { NULL, 0,  0 }
};

const struct flag_type form_flags[] = {
    { "edible",        FORM_EDIBLE,        true },
    { "poison",        FORM_POISON,        true },
    { "magical",       FORM_MAGICAL,       true },
    { "instant_decay", FORM_INSTANT_DECAY, true },
    { "other",         FORM_OTHER,         true },
    { "animal",        FORM_ANIMAL,        true },
    { "sentient",      FORM_SENTIENT,      true },
    { "undead",        FORM_UNDEAD,        true },
    { "construct",     FORM_CONSTRUCT,     true },
    { "mist",          FORM_MIST,          true },
    { "intangible",    FORM_INTANGIBLE,    true },
    { "biped",         FORM_BIPED,         true },
    { "centaur",       FORM_CENTAUR,       true },
    { "insect",        FORM_INSECT,        true },
    { "spider",        FORM_SPIDER,        true },
    { "crustacean",    FORM_CRUSTACEAN,    true },
    { "worm",          FORM_WORM,          true },
    { "blob",          FORM_BLOB,          true },
    { "mammal",        FORM_MAMMAL,        true },
    { "bird",          FORM_BIRD,          true },
    { "reptile",       FORM_REPTILE,       true },
    { "snake",         FORM_SNAKE,         true },
    { "dragon",        FORM_DRAGON,        true },
    { "amphibian",     FORM_AMPHIBIAN,     true },
    { "fish",          FORM_FISH,          true },
    { "cold_blood",    FORM_COLD_BLOOD,    true },
    { NULL, 0, 0  }
};

const struct flag_type part_flags[] = {
    { "head",        PART_HEAD,        true },
    { "arms",        PART_ARMS,        true },
    { "legs",        PART_LEGS,        true },
    { "heart",       PART_HEART,       true },
    { "brains",      PART_BRAINS,      true },
    { "guts",        PART_GUTS,        true },
    { "hands",       PART_HANDS,       true },
    { "feet",        PART_FEET,        true },
    { "fingers",     PART_FINGERS,     true },
    { "ear",         PART_EAR,         true },
    { "eye",         PART_EYE,         true },
    { "long_tongue", PART_LONG_TONGUE, true },
    { "eyestalks",   PART_EYESTALKS,   true },
    { "tentacles",   PART_TENTACLES,   true },
    { "fins",        PART_FINS,        true },
    { "wings",       PART_WINGS,       true },
    { "tail",        PART_TAIL,        true },
    { "claws",       PART_CLAWS,       true },
    { "fangs",       PART_FANGS,       true },
    { "horns",       PART_HORNS,       true },
    { "scales",      PART_SCALES,      true },
    { "tusks",       PART_TUSKS,       true },
    { NULL, 0, 0 }
};

const struct flag_type comm_flags[] = {
    { "compact",      COMM_COMPACT,      true },
    { "brief",        COMM_BRIEF,        true },
    { "prompt",       COMM_PROMPT,       true },
    { "combine",      COMM_COMBINE,      true },
    { "show_affects", COMM_SHOW_AFFECTS, true },
    { "afk",          COMM_AFK,          true },
    { "busy",         COMM_BUSY,         true },
    { "coding",       COMM_CODING,       true },
    { "building",     COMM_BUILD,        true },
    { NULL, 0, 0 }
};

const struct flag_type sex_flags[] = {
    { "male",    SEX_MALE,    true },
    { "female",  SEX_FEMALE,  true },
    { "neutral", SEX_NEUTRAL, true },
    { "random",  3,           true },
    { "none",    SEX_NEUTRAL, true },
    { NULL, 0, 0 }
};

const struct flag_type res_flags[] = {
    { "summon",    RES_SUMMON,    true },
    { "charm",     RES_CHARM,     true },
    { "magic",     RES_MAGIC,     true },
    { "weapon",    RES_WEAPON,    true },
    { "bash",      RES_BASH,      true },
    { "pierce",    RES_PIERCE,    true },
    { "slash",     RES_SLASH,     true },
    { "fire",      RES_FIRE,      true },
    { "cold",      RES_COLD,      true },
    { "lightning", RES_LIGHTNING, true },
    { "acid",      RES_ACID,      true },
    { "poison",    RES_POISON,    true },
    { "negative",  RES_NEGATIVE,  true },
    { "holy",      RES_HOLY,      true },
    { "energy",    RES_ENERGY,    true },
    { "mental",    RES_MENTAL,    true },
    { "drowning",  RES_DROWNING,  true },
    { "light",     RES_LIGHT,     true },
    { "sound",     RES_SOUND,     true },
    { "wood",      RES_WOOD,      true },
    { "silver",    RES_SILVER,    true },
    { "iron",      RES_IRON,      true },
    { NULL, 0, 0 }
};

const struct flag_type vuln_flags[] = {
    { "summon",    VULN_SUMMON,    true },
    { "charm",     VULN_CHARM,     true },
    { "magic",     VULN_MAGIC,     true },
    { "weapon",    VULN_WEAPON,    true },
    { "bash",      VULN_BASH,      true },
    { "pierce",    VULN_PIERCE,    true },
    { "slash",     VULN_SLASH,     true },
    { "fire",      VULN_FIRE,      true },
    { "cold",      VULN_COLD,      true },
    { "lightning", VULN_LIGHTNING, true },
    { "acid",      VULN_ACID,      true },
    { "poison",    VULN_POISON,    true },
    { "negative",  VULN_NEGATIVE,  true },
    { "holy",      VULN_HOLY,      true },
    { "energy",    VULN_ENERGY,    true },
    { "mental",    VULN_MENTAL,    true },
    { "drowning",  VULN_DROWNING,  true },
    { "light",     VULN_LIGHT,     true },
    { "sound",     VULN_SOUND,     true },
    { "wood",      VULN_WOOD,      true },
    { "silver",    VULN_SILVER,    true },
    { "iron",      VULN_IRON,      true },
    { NULL, 0, 0 }
};

const struct flag_type mprog_flags[] = {
    { "act",    TRIG_ACT,    true },
    { "bribe",  TRIG_BRIBE,  true },
    { "death",  TRIG_DEATH,  true },
    { "entry",  TRIG_ENTRY,  true },
    { "fight",  TRIG_FIGHT,  true },
    { "give",   TRIG_GIVE,   true },
    { "greet",  TRIG_GREET,  true },
    { "grall",  TRIG_GRALL,  true },
    { "kill",   TRIG_KILL,   true },
    { "hpcnt",  TRIG_HPCNT,  true },
    { "random", TRIG_RANDOM, true },
    { "speech", TRIG_SPEECH, true },
    { "exit",   TRIG_EXIT,   true },
    { "exall",  TRIG_EXALL,  true },
    { "delay",  TRIG_DELAY,  true },
    { "surr",   TRIG_SURR,   true },
    { NULL, 0, true }
};


/***************************************************************************
*    flags used by areas/rooms
***************************************************************************/
const struct flag_type area_flags[] = {
    { "none",    AREA_NONE,    false },
    { "changed", AREA_CHANGED, true     },
    { "added",   AREA_ADDED,   true     },
    { "loading", AREA_LOADING, false },
    { NULL, 0, 0 }
};

const struct flag_type exit_flags[] = {
    { "door",        EX_ISDOOR,      true },
    { "closed",      EX_CLOSED,      true },
    { "locked",      EX_LOCKED,      true },
    { "pickproof",   EX_PICKPROOF,   true },
    { "nopass",      EX_NOPASS,      true },
    { "easy",        EX_EASY,        true },
    { "hard",        EX_HARD,        true },
    { "infuriating", EX_INFURIATING, true },
    { "noclose",     EX_NOCLOSE,     true },
    { "nolock",      EX_NOLOCK,      true },
    { NULL, 0, 0 }
};

const struct flag_type door_resets[] = {
    { "open and unlocked",   0, true },
    { "closed and unlocked", 1, true },
    { "closed and locked",   2, true },
    { NULL, 0, 0 }
};

const struct flag_type room_flags[] = {
    { "bank",         ROOM_BANK,            true },
    { "dark",         ROOM_DARK,            true },
    { "indoors",      ROOM_INDOORS,         true },
    { "no_pushdrag",  ROOM_NO_PUSH_NO_DRAG, true },
    { "nogate",       ROOM_NOGATE,          true },
    { "noportal",     ROOM_NOPORTAL,        true },
    { "nosummon",     ROOM_NOSUMMON,        true },
    { "noteleport",   ROOM_NOTELEPORT,      true },
    { "notransport",  ROOM_NOTRANSPORT,     true },
    { "portalonly",   ROOM_PORTALONLY,      true },
    { "nowhere",      ROOM_NOWHERE,         true },
    { "pet_shop",     ROOM_PET_SHOP,        true },
    { "private",      ROOM_PRIVATE,         true },
    { "safe",         ROOM_SAFE,            true },
    { "solitary",     ROOM_SOLITARY,        true },
    { "norandom",     ROOM_NORANDOM,        true },
    { "imp_only",     ROOM_IMP_ONLY,        true },
    { "gods_only",    ROOM_GODS_ONLY,       true },
    { "heroes_only",  ROOM_HEROES_ONLY,     true },
    { "lev300",       ROOM_HIGHEST_ONLY,    true },
    { "lev150",       ROOM_HIGHER_ONLY,     true },
    { "newbies_only", ROOM_NEWBIES_ONLY,    true },
    { "no_mob",       ROOM_NO_MOB,          true },
    { NULL, 0, 0 }
};

const struct flag_type sector_flags[] = {
    { "inside",     SECT_INSIDE,       true },
    { "city",       SECT_CITY,         true },
    { "field",      SECT_FIELD,        true },
    { "forest",     SECT_FOREST,       true },
    { "hills",      SECT_HILLS,        true },
    { "mountain",   SECT_MOUNTAIN,     true },
    { "swim",       SECT_WATER_SWIM,   true },
    { "noswim",     SECT_WATER_NOSWIM, true },
    { "underwater", SECT_UNDERWATER,   true },
    { "air",        SECT_AIR,          true },
    { "desert",     SECT_DESERT,       true },
    { NULL, 0, 0 }
};


/***************************************************************************
*    flags used by objects
***************************************************************************/
const struct flag_type type_flags[] =
{
    { "light",        ITEM_LIGHT,         true  },
    { "scroll",        ITEM_SCROLL,     true  },
    { "wand",        ITEM_WAND,         true  },
    { "staff",        ITEM_STAFF,         true  },
    { "weapon",        ITEM_WEAPON,     true  },
    { "treasure",        ITEM_TREASURE,   true  },
    { "armor",        ITEM_ARMOR,         true  },
    { "potion",        ITEM_POTION,     true  },
    { "clothing",        ITEM_CLOTHING,   true  },
    { "furniture",        ITEM_FURNITURE,  true  },
    { "trash",        ITEM_TRASH,         true  },
    { "container",        ITEM_CONTAINER,  true  },
    { "drinkcontainer", ITEM_DRINK_CON,  true  },
    { "key",        ITEM_KEY,         true  },
    { "food",        ITEM_FOOD,         true  },
    { "money",        ITEM_MONEY,         true  },
    { "boat",        ITEM_BOAT,         true  },
    { "npccorpse",        ITEM_CORPSE_NPC, true  },
    { "pc corpse",        ITEM_CORPSE_PC,  false },
    { "fountain",        ITEM_FOUNTAIN,   true  },
    { "pill",        ITEM_PILL,         true  },
    { "map",        ITEM_MAP,         true  },
    { "portal",        ITEM_PORTAL,     true  },
    { "warpstone",        ITEM_WARP_STONE, true  },
    { "gem",        ITEM_GEM,         true  },
    { "jewelry",        ITEM_JEWELRY,    true  },
    { "teleporter",        ITEM_TELEPORT,   true  },
    { "atm",        ITEM_ATM,         true  },
    { "invitation",        ITEM_INVITATION, true  },
    { "faeriefog",        ITEM_FAERIE_FOG, false },
    { "dust",        ITEM_DUST,         false },
    { "doll",        ITEM_DOLL,         true  },
    { "settable",        ITEM_SOCKETS,    true  },
    { "dice",        ITEM_DICE,         true  },
    { NULL,            0,             0       }
};

const struct flag_type extra_flags[] =
{
    { "glow",     ITEM_GLOW,        true  },
    { "hum",     ITEM_HUM,        true  },
    { "dark",     ITEM_DARK,        true  },
    { "lock",     ITEM_LOCK,        true  },
    { "invis",     ITEM_INVIS,        true  },
    { "magic",     ITEM_MAGIC,        true  },
    { "nodrop",     ITEM_NODROP,        true  },
    { "bless",     ITEM_BLESS,        true  },
    { "noremove",     ITEM_NOREMOVE,        true  },
    { "inventory",     ITEM_INVENTORY,    true  },
    { "nopurge",     ITEM_NOPURGE,        true  },
    { "rotdeath",     ITEM_ROT_DEATH,    true  },
    { "visdeath",     ITEM_VIS_DEATH,    true  },
    { "nonmetal",     ITEM_NONMETAL,        true  },
    { "meltdrop",     ITEM_MELT_DROP,    true  },
    { "hadtimer",     ITEM_HAD_TIMER,    true  },
    { "sellextract", ITEM_SELL_EXTRACT, true  },
    { "burnproof",     ITEM_BURN_PROOF,   true  },
    { "nouncurse",     ITEM_NOUNCURSE,    true  },
    { "noauction",     ITEM_NOAUC,        true  },
    { "nolocate",     ITEM_NOLOCATE,        true  },
    { "unique",     ITEM_UNIQUE,        false },
    { "gemmed",     ITEM_INLAY1,        false },
    { "bejewelled",     (long)ITEM_INLAY2, false },
    { "deathdrop",     ITEM_DEATH_DROP,   true  },
    { NULL,         0,            0      }
};

const struct flag_type wear_flags[] =
{
    { "take",      ITEM_TAKE,     true },
    { "finger",    ITEM_WEAR_FINGER, true },
    { "neck",      ITEM_WEAR_NECK,     true },
    { "body",      ITEM_WEAR_BODY,     true },
    { "head",      ITEM_WEAR_HEAD,     true },
    { "face",      ITEM_WEAR_FACE,     true },
    { "ear",       ITEM_WEAR_EAR,     true },
    { "legs",      ITEM_WEAR_LEGS,     true },
    { "feet",      ITEM_WEAR_FEET,     true },
    { "hands",     ITEM_WEAR_HANDS,     true },
    { "arms",      ITEM_WEAR_ARMS,     true },
    { "shield",    ITEM_WEAR_SHIELD, true },
    { "about",     ITEM_WEAR_ABOUT,     true },
    { "waist",     ITEM_WEAR_WAIST,     true },
    { "wrist",     ITEM_WEAR_WRIST,     true },
    { "wield",     ITEM_WIELD,     true },
    { "hold",      ITEM_HOLD,     true },
    { "tattoo",    ITEM_WEAR_TATTOO, true },
    { "nosac",     ITEM_NO_SAC,     true },
    { "wearfloat", ITEM_WEAR_FLOAT,     true },
/*  { "twohands",        ITEM_TWO_HANDS,         true    }, */
    { NULL,           0,         0    }
};

const struct flag_type apply_flags[] =
{
    { "none",      APPLY_NONE,           true  },
    { "strength",      APPLY_STR,           true  },
    { "dexterity",      APPLY_DEX,           true  },
    { "intelligence", APPLY_INT,           true  },
    { "wisdom",      APPLY_WIS,           true  },
    { "constitution", APPLY_CON,           true  },
    { "sex",      APPLY_SEX,           true  },
    { "class",      APPLY_CLASS,           true  },
    { "level",      APPLY_LEVEL,           true  },
    { "age",      APPLY_AGE,           true  },
    { "height",      APPLY_HEIGHT,           true  },
    { "weight",      APPLY_WEIGHT,           true  },
    { "mana",      APPLY_MANA,           true  },
    { "hp",          APPLY_HIT,           true  },
    { "move",      APPLY_MOVE,           true  },
    { "gold",      APPLY_GOLD,           true  },
    { "experience",      APPLY_EXP,           true  },
    { "ac",          APPLY_AC,           true  },
    { "hitroll",      APPLY_HITROLL,       true  },
    { "damroll",      APPLY_DAMROLL,       true  },
    { "saves",      APPLY_SAVES,           true  },
    { "savingpara",      APPLY_SAVING_PARA,   true  },
    { "savingrod",      APPLY_SAVING_ROD,    true  },
    { "savingpetri",  APPLY_SAVING_PETRI,  true  },
    { "savingbreath", APPLY_SAVING_BREATH, true  },
    { "savingspell",  APPLY_SAVING_SPELL,  true  },
    { "spellaffect",  APPLY_SPELL_AFFECT,  false },
    { "my_lag",      APPLY_MLAG,           false },
    { "their_lag",      APPLY_TLAG,           false },
    { NULL,          0,               0     }
};

const struct flag_type wear_loc_strings[] =
{
    { "in the inventory",     WEAR_NONE,    true },
    { "as a light",         WEAR_LIGHT,    true },
    { "on the left finger",     WEAR_FINGER_L, true },
    { "on the right finger", WEAR_FINGER_R, true },
    { "around the neck (1)", WEAR_NECK_1,    true },
    { "around the neck (2)", WEAR_NECK_2,    true },
    { "on the body",     WEAR_BODY,    true },
    { "over the head",     WEAR_HEAD,    true },
    { "on the face",     WEAR_FACE,    true },
    { "as a tattoo",     WEAR_TATTOO,    true },
    { "on the legs",     WEAR_LEGS,    true },
    { "on the feet",     WEAR_FEET,    true },
    { "on the hands",     WEAR_HANDS,    true },
    { "on the arms",     WEAR_ARMS,    true },
    { "as a shield",     WEAR_SHIELD,    true },
    { "about the shoulders", WEAR_ABOUT,    true },
    { "around the waist",     WEAR_WAIST,    true },
    { "on the left wrist",     WEAR_WRIST_L,    true },
    { "on the right wrist",     WEAR_WRIST_R,    true },
    { "wielded",         WEAR_WIELD,    true },
    { "held in the hands",     WEAR_HOLD,    true },
    { "floating nearby",     WEAR_FLOAT,    true },
    { NULL,             0,        0    },
};

const struct flag_type wear_loc_flags[] =
{
    { "none",     WEAR_NONE,     true },
    { "light",    WEAR_LIGHT,    true },
    { "lfinger",  WEAR_FINGER_L, true },
    { "rfinger",  WEAR_FINGER_R, true },
    { "neck1",    WEAR_NECK_1,   true },
    { "neck2",    WEAR_NECK_2,   true },
    { "body",     WEAR_BODY,     true },
    { "head",     WEAR_HEAD,     true },
    { "face",     WEAR_FACE,     true },
    { "tattoo",   WEAR_TATTOO,   true },
    { "lear",     WEAR_EAR_L,    true },
    { "rear",     WEAR_EAR_R,    true },
    { "legs",     WEAR_LEGS,     true },
    { "feet",     WEAR_FEET,     true },
    { "hands",    WEAR_HANDS,    true },
    { "arms",     WEAR_ARMS,     true },
    { "shield",   WEAR_SHIELD,   true },
    { "about",    WEAR_ABOUT,    true },
    { "waist",    WEAR_WAIST,    true },
    { "lwrist",   WEAR_WRIST_L,  true },
    { "rwrist",   WEAR_WRIST_R,  true },
    { "wielded",  WEAR_WIELD,    true },
    { "hold",     WEAR_HOLD,     true },
    { "floating", WEAR_FLOAT,    true },
    { NULL,          0,         0      }
};

const struct flag_type container_flags[] =
{
    { "closeable", 1,  true },
    { "pickproof", 2,  true },
    { "closed",    4,  true },
    { "locked",    8,  true },
    { "puton",     16, true },
    { NULL,           0,  0    }
};

const struct flag_type ac_type[] =
{
    { "pierce", AC_PIERCE, true },
    { "bash",   AC_BASH,   true },
    { "slash",  AC_SLASH,  true },
    { "exotic", AC_EXOTIC, true },
    { NULL,        0,           0    }
};

const struct flag_type weapon_class[] =
{
    { "exotic",  WEAPON_EXOTIC,  true },
    { "sword",   WEAPON_SWORD,   true },
    { "dagger",  WEAPON_DAGGER,  true },
    { "spear",   WEAPON_SPEAR,   true },
    { "mace",    WEAPON_MACE,    true },
    { "axe",     WEAPON_AXE,     true },
    { "flail",   WEAPON_FLAIL,   true },
    { "whip",    WEAPON_WHIP,    true },
    { "polearm", WEAPON_POLEARM, true },
    { NULL,         0,             0      }
};

const struct flag_type weapon_flag_type[] =
{
    { "flaming",  WEAPON_FLAMING,    true },
    { "frost",    WEAPON_FROST,    true },
    { "vampiric", WEAPON_VAMPIRIC,    true },
    { "sharp",    WEAPON_SHARP,    true },
    { "vorpal",   WEAPON_VORPAL,    true },
    { "twohands", WEAPON_TWO_HANDS, true },
    { "shocking", WEAPON_SHOCKING,    true },
    { "poison",   WEAPON_POISON,    true },
    { "acidic",   WEAPON_ACIDIC,    true },
    { NULL,          0,        0    }
};

/***************************************************************************
*    **THIS TABLE IS ALL WRONG***
***************************************************************************/
const struct flag_type socket_flags[] =
{
    { "sapphire", SOC_SAPPHIRE, true },
    { "ruby",     SOC_RUBY,        true },
    { "emerald",  SOC_EMERALD,  true },
    { "diamond",  SOC_DIAMOND,  true },
    { "topaz",    SOC_TOPAZ,    true },
    { "skull",    SOC_SKULL,    true },
    { "",          0,        0     }
};

/***************************************************************************
*    **THIS TABLE IS ALL WRONG***
***************************************************************************/
const struct flag_type socket_values[] =
{
    { "chip",     GEM_CHIPPED,  true },
    { "flawed",   GEM_FLAWED,   true },
    { "flawless", GEM_FLAWLESS, true },
    { "perfect",  GEM_PERFECT,  true },
    { "",          0,        0     }
};

const struct flag_type portal_flags[] =
{
    { "normal_exit", GATE_NORMAL_EXIT, true },
    { "no_curse",     GATE_NOCURSE,       true },
    { "go_with",     GATE_GOWITH,       true },
    { "buggy",     GATE_BUGGY,       true },
    { "random",     GATE_RANDOM,       true },
    { NULL,         0,           0    }
};

const struct flag_type furniture_flags[] =
{
    { "stand_at",    STAND_AT,   true },
    { "stand_on",    STAND_ON,   true },
    { "stand_in",    STAND_IN,   true },
    { "sit_at",    SIT_AT,        true },
    { "sit_on",    SIT_ON,        true },
    { "sit_in",    SIT_IN,        true },
    { "rest_at",    REST_AT,    true },
    { "rest_on",    REST_ON,    true },
    { "rest_in",    REST_IN,    true },
    { "sleep_at",    SLEEP_AT,   true },
    { "sleep_on",    SLEEP_ON,   true },
    { "sleep_in",    SLEEP_IN,   true },
    { "put_at",    PUT_AT,        true },
    { "put_on",    PUT_ON,        true },
    { "put_in",    PUT_IN,        true },
    { "put_inside", PUT_INSIDE, true },
    { NULL,        0,        0     }
};

const struct  flag_type apply_types     [] =
{
    { "affects", TO_AFFECTS, true },
    { "object",  TO_OBJECT,     true },
    { "immune",  TO_IMMUNE,     true },
    { "resist",  TO_RESIST,     true },
    { "vuln",    TO_VULN,     true },
    { "weapon",  TO_WEAPON,     true },
    { NULL,         0,         true }
};

const struct flag_type damage_flags[] =
{
    { "slice",        1,  true  },
    { "stab",        2,  true  },
    { "slash",        3,  true  },
    { "whip",        4,  true  },
    { "claw",        5,  true  },                                            /*  5 */
    { "blast",        6,  true  },
    { "pound",        7,  true  },
    { "crush",        8,  true  },
    { "grep",        9,  true  },
    { "bite",        10, true  },                                    /* 10 */
    { "pierce",        11, true  },
    { "suction",        12, true  },
    { "beating",        13, true  },
    { "digestion",        14, true  },
    { "charge",        15, true  },                                            /* 15 */
    { "slap",        16, true  },
    { "punch",        17, true  },
    { "magic",        19, true  },
    { "divine power",    20, true  },                                    /* 20 */
    { "cleave",        21, true  },
    { "scratch",        22, true  },
    { "peck",        23, true  },
    { "peck",        24, true  },
    { "chop",        25, true  },                                            /* 25 */
    { "sting",        26, true  },
    { "smash",        27, true  },
    { "shocking bite",    28, true  },
    { "flaming bite",    29, true  },
    { "freezing bite",    30, true  },                                    /* 30 */
    { "acidic_bite",    31, true  },
    { "chomp",        32, true  },
    { "life drain",        33, true  },
    { "thrust",        34, true  },
    { "slime",        35, true  },                                            /* 35 */
    { "shock",        36, true  },
    { "thwack",        37, true  },
    { "flame",        38, true  },
    { "chill",        39, true  },
    { "tracers",        40, true  },                                            /* 40 */
    { "splinters",        42, true  },
    { "penstroke",        43, true  },
    { "vile caress",    45, true  },                                    /* 45 */
    { "withered touch",    46, true  },
    { "presence",        47, true  },
    { "passive resistance", 48, true  },
    { "darkness",        49, true  },
    { "singing",        50, true  },                                            /* 50 */
    { "judgement",        51, true  },
    { "stare",        52, true  },
    { "deathstroke",    53, true  },
    { "curse",        54, true  },
    { "shockwave",        55, true  },                                            /* 55 */
    { "gush",        57, true  },
    { NULL,            0,  false }
};

const struct flag_type target_flags[] =
{
    { "ignore",        TAR_IGNORE,         true },
    { "offensive",     TAR_CHAR_OFFENSIVE, true },
    { "defensive",     TAR_CHAR_DEFENSIVE, true },
    { "self",          TAR_CHAR_SELF,      true },
    { "inventory",     TAR_OBJ_INV,        true },
    { "obj_defensive", TAR_OBJ_CHAR_DEF,   true },
    { "obj_offensive", TAR_OBJ_CHAR_OFF,   true },
    { "room",          TAR_ROOM,           true },
    { NULL, 0, 0 }
};

const struct flag_type skill_flags[] =
{
    { "noscribe",     SPELL_NOSCRIBE,    true },
    { "nobrew",     SPELL_NOBREW,        true },
    { "dispellable", SPELL_DISPELLABLE, true },
    { "cancelable",     SPELL_CANCELABLE,  true },
    { "affstrip",     SPELL_AFFSTRIP,    true },
    { NULL,         0,            0     }
};

const struct flag_type extra2_flags[] =
{
    { "graft",    ITEM2_GRAFT,      true },
    { "ethereal",    ITEM2_ETHEREAL,      true },
    { "noscan",    ITEM2_NOSCAN,      true },
    { "relic",    ITEM2_RELIC,      true },
    { "nodonate",    ITEM2_NODONATE,      true },
    { "norestring", ITEM2_NORESTRING, true },
    { NULL,        0,          0    }
};
