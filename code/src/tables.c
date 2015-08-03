#include "merc.h"
#include "tables.h"


const struct bit_type bitvector_type [] =
{
	{ affect_flags,	    "affect" },
	{ apply_flags,	    "apply"  },
	{ imm_flags,	    "imm"    },
	{ res_flags,	    "res"    },
	{ vuln_flags,	    "vuln"   },
	{ weapon_flag_type, "weapon" }
};


/***************************************************************************
*	flags used by characters (players/mobs)
***************************************************************************/

const struct position_type position_table[] =
{
	{ "dead",	      "dead"  },
	{ "mortally wounded", "mort"  },
	{ "incapacitated",    "incap" },
	{ "stunned",	      "stun"  },
	{ "sleeping",	      "sleep" },
	{ "resting",	      "rest"  },
	{ "sitting",	      "sit"   },
	{ "fighting",	      "fight" },
	{ "standing",	      "stand" },
	{ NULL,		      NULL    }
};

const struct sex_type sex_table[] =
{
	{ "none"   },
	{ "male"   },
	{ "female" },
	{ "either" },
	{ NULL	   }
};

const struct size_type size_table[] =
{
	{ "tiny"   },
	{ "small"  },
	{ "medium" },
	{ "large"  },
	{ "huge"   },
	{ "giant"  },
	{ NULL	   }
};

const struct flag_type act_flags[] =
{
	{ "npc",	   A,	     FALSE },
	{ "sentinel",	   B,	     TRUE  },
	{ "scavenger",	   C,	     TRUE  },
	{ "aggressive",	   F,	     TRUE  },
	{ "stay_area",	   G,	     TRUE  },
	{ "wimpy",	   H,	     TRUE  },
	{ "pet",	   I,	     TRUE  },
	{ "train",	   J,	     TRUE  },
	{ "practice",	   K,	     TRUE  },
	{ "undead",	   O,	     TRUE  },
	{ "cleric",	   Q,	     TRUE  },
	{ "mage",	   R,	     TRUE  },
	{ "thief",	   S,	     TRUE  },
	{ "warrior",	   T,	     TRUE  },
	{ "noalign",	   U,	     TRUE  },
	{ "nopurge",	   V,	     TRUE  },
	{ "outdoors",	   W,	     TRUE  },
	{ "indoors",	   Y,	     TRUE  },
	{ "exchanger",	   Z,	     TRUE  },
	{ "healer",	   aa,	     TRUE  },
	{ "gain",	   bb,	     TRUE  },
	{ "update_always", cc,	     TRUE  },
	{ "changer",	   dd,	     TRUE  },
	{ "blacksmith",	   ee,	     TRUE  },
	{ NULL,		   0,	     FALSE }
};

const struct flag_type plr_flags[] =
{
	{ "npc",	A,  FALSE },
	{ "autoassist", C,  FALSE },
	{ "autoeq",	V,  FALSE },
	{ "autoexit",	D,  FALSE },
	{ "autoloot",	E,  FALSE },
	{ "autosac",	F,  FALSE },
	{ "autogold",	G,  FALSE },
	{ "autosplit",	H,  FALSE },
	{ "holylight",	N,  FALSE },
	{ "can_loot",	P,  FALSE },
	{ "nosummon",	Q,  FALSE },
	{ "nofollow",	R,  FALSE },
	{ "permit",	U,  TRUE  },
	{ "log",	W,  FALSE },
	{ "deny",	X,  FALSE },
	{ "freeze",	Y,  FALSE },
	{ "thief",	Z,  FALSE },
	{ "killer",	aa, FALSE },
	{ "peaceful",	dd, FALSE },
	{ NULL,		0,  0	  }
};

const struct flag_type size_flags[] =
{
	{ "tiny",   SIZE_TINY,	 TRUE },
	{ "small",  SIZE_SMALL,	 TRUE },
	{ "medium", SIZE_MEDIUM, TRUE },
	{ "large",  SIZE_LARGE,	 TRUE },
	{ "huge",   SIZE_HUGE,	 TRUE },
	{ "giant",  SIZE_GIANT,	 TRUE },
	{ NULL,	    0,		 0    },
};

const struct flag_type position_flags[] =
{
	{ "dead",     POS_DEAD,	    FALSE },
	{ "mortal",   POS_MORTAL,   FALSE },
	{ "incap",    POS_INCAP,    FALSE },
	{ "stunned",  POS_STUNNED,  FALSE },
	{ "sleeping", POS_SLEEPING, TRUE  },
	{ "resting",  POS_RESTING,  TRUE  },
	{ "sitting",  POS_SITTING,  TRUE  },
	{ "fighting", POS_FIGHTING, TRUE  },
	{ "standing", POS_STANDING, TRUE  },
	{ NULL,	      0,	    0	  }
};

const struct flag_type affect_flags[] =
{
	{ "pollen",	  A,  TRUE },
	{ "invisible",	  B,  TRUE },
	{ "detect_evil",  C,  TRUE },
	{ "detect_invis", D,  TRUE },
	{ "detect_magic", E,  TRUE },
	{ "druid_call",	  F,  TRUE },
	{ "detect_good",  G,  TRUE },
	{ "sanctuary",	  H,  TRUE },
	{ "faerie_fire",  I,  TRUE },
	{ "infrared",	  J,  TRUE },
	{ "curse",	  K,  TRUE },
	{ "blind",	  L,  TRUE },
	{ "poison",	  M,  TRUE },
	{ "protect_evil", N,  TRUE },
	{ "protect_good", O,  TRUE },
	{ "sneak",	  P,  TRUE },
	{ "hide",	  Q,  TRUE },
	{ "sleep",	  R,  TRUE },
	{ "charm",	  S,  TRUE },
	{ "flying",	  T,  TRUE },
	{ "pass_door",	  U,  TRUE },
	{ "haste",	  V,  TRUE },
	{ "calm",	  W,  TRUE },
	{ "plague",	  X,  TRUE },
	{ "weaken",	  Y,  TRUE },
	{ "dark_vision",  Z,  TRUE },
	{ "berserk",	  aa, TRUE },
	{ "slow",	  cc, TRUE },
	{ "banzai",	  ee, TRUE },
	{ "calloused",	  bb, TRUE },
	{ NULL,		  0,  0	   }
};

const struct flag_type off_flags[] =
{
	{ "area_attack",    A, TRUE },
	{ "bash",	    C, TRUE },
	{ "berserk",	    D, TRUE },
	{ "disarm",	    E, TRUE },
	{ "dodge",	    F, TRUE },
	{ "fade",	    G, TRUE },
	{ "fast",	    H, TRUE },
	{ "kick",	    I, TRUE },
	{ "dirt_kick",	    J, TRUE },
	{ "parry",	    K, TRUE },
	{ "rescue",	    L, TRUE },
	{ "tail",	    M, TRUE },
	{ "trip",	    N, TRUE },
	{ "crush",	    O, TRUE },
	{ "assist_all",	    P, TRUE },
	{ "assist_align",   Q, TRUE },
	{ "assist_race",    R, TRUE },
	{ "assist_players", S, TRUE },
	{ "assist_guard",   T, TRUE },
	{ "assist_vnum",    U, TRUE },
	{ NULL,		    0, 0    }
};

const struct flag_type imm_flags[] =
{
	{ "summon",    A,  TRUE },
	{ "charm",     B,  TRUE },
	{ "magic",     C,  TRUE },
	{ "weapon",    D,  TRUE },
	{ "bash",      E,  TRUE },
	{ "pierce",    F,  TRUE },
	{ "slash",     G,  TRUE },
	{ "fire",      H,  TRUE },
	{ "cold",      I,  TRUE },
	{ "lightning", J,  TRUE },
	{ "acid",      K,  TRUE },
	{ "poison",    L,  TRUE },
	{ "negative",  M,  TRUE },
	{ "holy",      N,  TRUE },
	{ "energy",    O,  TRUE },
	{ "mental",    P,  TRUE },
	{ "disease",   Q,  TRUE },
	{ "drowning",  R,  TRUE },
	{ "light",     S,  TRUE },
	{ "sound",     T,  TRUE },
	{ "wood",      X,  TRUE },
	{ "silver",    Y,  TRUE },
	{ "iron",      Z,  TRUE },
	{ "illusion",  aa, TRUE },
	{ NULL,	       0,  0	}
};

const struct flag_type form_flags[] =
{
	{ "edible",	   FORM_EDIBLE,	       TRUE },
	{ "poison",	   FORM_POISON,	       TRUE },
	{ "magical",	   FORM_MAGICAL,       TRUE },
	{ "instant_decay", FORM_INSTANT_DECAY, TRUE },
	{ "other",	   FORM_OTHER,	       TRUE },
	{ "animal",	   FORM_ANIMAL,	       TRUE },
	{ "sentient",	   FORM_SENTIENT,      TRUE },
	{ "undead",	   FORM_UNDEAD,	       TRUE },
	{ "construct",	   FORM_CONSTRUCT,     TRUE },
	{ "mist",	   FORM_MIST,	       TRUE },
	{ "intangible",	   FORM_INTANGIBLE,    TRUE },
	{ "biped",	   FORM_BIPED,	       TRUE },
	{ "centaur",	   FORM_CENTAUR,       TRUE },
	{ "insect",	   FORM_INSECT,	       TRUE },
	{ "spider",	   FORM_SPIDER,	       TRUE },
	{ "crustacean",	   FORM_CRUSTACEAN,    TRUE },
	{ "worm",	   FORM_WORM,	       TRUE },
	{ "blob",	   FORM_BLOB,	       TRUE },
	{ "mammal",	   FORM_MAMMAL,	       TRUE },
	{ "bird",	   FORM_BIRD,	       TRUE },
	{ "reptile",	   FORM_REPTILE,       TRUE },
	{ "snake",	   FORM_SNAKE,	       TRUE },
	{ "dragon",	   FORM_DRAGON,	       TRUE },
	{ "amphibian",	   FORM_AMPHIBIAN,     TRUE },
	{ "fish",	   FORM_FISH,	       TRUE },
	{ "cold_blood",	   FORM_COLD_BLOOD,    TRUE },
	{ NULL,		   0,		       0    }
};

const struct flag_type part_flags[] =
{
	{ "head",	 PART_HEAD,	   TRUE },
	{ "arms",	 PART_ARMS,	   TRUE },
	{ "legs",	 PART_LEGS,	   TRUE },
	{ "heart",	 PART_HEART,	   TRUE },
	{ "brains",	 PART_BRAINS,	   TRUE },
	{ "guts",	 PART_GUTS,	   TRUE },
	{ "hands",	 PART_HANDS,	   TRUE },
	{ "feet",	 PART_FEET,	   TRUE },
	{ "fingers",	 PART_FINGERS,	   TRUE },
	{ "ear",	 PART_EAR,	   TRUE },
	{ "eye",	 PART_EYE,	   TRUE },
	{ "long_tongue", PART_LONG_TONGUE, TRUE },
	{ "eyestalks",	 PART_EYESTALKS,   TRUE },
	{ "tentacles",	 PART_TENTACLES,   TRUE },
	{ "fins",	 PART_FINS,	   TRUE },
	{ "wings",	 PART_WINGS,	   TRUE },
	{ "tail",	 PART_TAIL,	   TRUE },
	{ "claws",	 PART_CLAWS,	   TRUE },
	{ "fangs",	 PART_FANGS,	   TRUE },
	{ "horns",	 PART_HORNS,	   TRUE },
	{ "scales",	 PART_SCALES,	   TRUE },
	{ "tusks",	 PART_TUSKS,	   TRUE },
	{ NULL,		 0,		   0	}
};

const struct flag_type comm_flags[] =
{
	{ "quiet",	  COMM_QUIET,	     TRUE  },
	{ "compact",	  COMM_COMPACT,	     TRUE  },
	{ "brief",	  COMM_BRIEF,	     TRUE  },
	{ "prompt",	  COMM_PROMPT,	     TRUE  },
	{ "combine",	  COMM_COMBINE,	     TRUE  },
	{ "telnet_ga",	  COMM_TELNET_GA,    TRUE  },
	{ "show_affects", COMM_SHOW_AFFECTS, TRUE  },
	{ "nochannels",	  COMM_NOCHANNELS,   FALSE },
	{ "afk",	  COMM_AFK,	     TRUE  },
	{ "info",	  COMM_INFO,	     TRUE  },
	{ "noemote",	  COMM_NOEMOTE,     FALSE },
	{ "busy",	  COMM_BUSY,	     TRUE  },
	{ "coding",	  COMM_CODING,	     TRUE  },
	{ "building",	  COMM_BUILD,	     TRUE  },
	{ NULL,		  0,		     0	   }
};

const struct flag_type sex_flags[] =
{
	{ "male",    SEX_MALE,	  TRUE },
	{ "female",  SEX_FEMALE,  TRUE },
	{ "neutral", SEX_NEUTRAL, TRUE },
	{ "random",  3,		  TRUE },
	{ "none",    SEX_NEUTRAL, TRUE },
	{ NULL,	     0,		  0    }
};

const struct flag_type res_flags[] =
{
	{ "summon",    RES_SUMMON,    TRUE },
	{ "charm",     RES_CHARM,     TRUE },
	{ "magic",     RES_MAGIC,     TRUE },
	{ "weapon",    RES_WEAPON,    TRUE },
	{ "bash",      RES_BASH,      TRUE },
	{ "pierce",    RES_PIERCE,    TRUE },
	{ "slash",     RES_SLASH,     TRUE },
	{ "fire",      RES_FIRE,      TRUE },
	{ "cold",      RES_COLD,      TRUE },
	{ "lightning", RES_LIGHTNING, TRUE },
	{ "acid",      RES_ACID,      TRUE },
	{ "poison",    RES_POISON,    TRUE },
	{ "negative",  RES_NEGATIVE,  TRUE },
	{ "holy",      RES_HOLY,      TRUE },
	{ "energy",    RES_ENERGY,    TRUE },
	{ "mental",    RES_MENTAL,    TRUE },
	{ "disease",   RES_DISEASE,   TRUE },
	{ "drowning",  RES_DROWNING,  TRUE },
	{ "light",     RES_LIGHT,     TRUE },
	{ "sound",     RES_SOUND,     TRUE },
	{ "wood",      RES_WOOD,      TRUE },
	{ "silver",    RES_SILVER,    TRUE },
	{ "iron",      RES_IRON,      TRUE },
	{ NULL,	       0,	      0	   }
};

const struct flag_type vuln_flags[] =
{
	{ "summon",    VULN_SUMMON,    TRUE },
	{ "charm",     VULN_CHARM,     TRUE },
	{ "magic",     VULN_MAGIC,     TRUE },
	{ "weapon",    VULN_WEAPON,    TRUE },
	{ "bash",      VULN_BASH,      TRUE },
	{ "pierce",    VULN_PIERCE,    TRUE },
	{ "slash",     VULN_SLASH,     TRUE },
	{ "fire",      VULN_FIRE,      TRUE },
	{ "cold",      VULN_COLD,      TRUE },
	{ "lightning", VULN_LIGHTNING, TRUE },
	{ "acid",      VULN_ACID,      TRUE },
	{ "poison",    VULN_POISON,    TRUE },
	{ "negative",  VULN_NEGATIVE,  TRUE },
	{ "holy",      VULN_HOLY,      TRUE },
	{ "energy",    VULN_ENERGY,    TRUE },
	{ "mental",    VULN_MENTAL,    TRUE },
	{ "disease",   VULN_DISEASE,   TRUE },
	{ "drowning",  VULN_DROWNING,  TRUE },
	{ "light",     VULN_LIGHT,     TRUE },
	{ "sound",     VULN_SOUND,     TRUE },
	{ "wood",      VULN_WOOD,      TRUE },
	{ "silver",    VULN_SILVER,    TRUE },
	{ "iron",      VULN_IRON,      TRUE },
	{ NULL,	       0,	       0    }
};

const struct flag_type mprog_flags[] =
{
	{ "act",    TRIG_ACT,	 TRUE },
	{ "bribe",  TRIG_BRIBE,	 TRUE },
	{ "death",  TRIG_DEATH,	 TRUE },
	{ "entry",  TRIG_ENTRY,	 TRUE },
	{ "fight",  TRIG_FIGHT,	 TRUE },
	{ "give",   TRIG_GIVE,	 TRUE },
	{ "greet",  TRIG_GREET,	 TRUE },
	{ "grall",  TRIG_GRALL,	 TRUE },
	{ "kill",   TRIG_KILL,	 TRUE },
	{ "hpcnt",  TRIG_HPCNT,	 TRUE },
	{ "random", TRIG_RANDOM, TRUE },
	{ "speech", TRIG_SPEECH, TRUE },
	{ "exit",   TRIG_EXIT,	 TRUE },
	{ "exall",  TRIG_EXALL,	 TRUE },
	{ "delay",  TRIG_DELAY,	 TRUE },
	{ "surr",   TRIG_SURR,	 TRUE },
	{ NULL,	    0,		 TRUE }
};


/***************************************************************************
*	flags used by areas/rooms
***************************************************************************/
const struct flag_type area_flags[] =
{
	{ "none",    AREA_NONE,	   FALSE },
	{ "changed", AREA_CHANGED, TRUE	 },
	{ "added",   AREA_ADDED,   TRUE	 },
	{ "loading", AREA_LOADING, FALSE },
	{ NULL,	     0,		   0	 }
};

const struct flag_type exit_flags[] =
{
	{ "door",	 EX_ISDOOR,	 TRUE },
	{ "closed",	 EX_CLOSED,	 TRUE },
	{ "locked",	 EX_LOCKED,	 TRUE },
	{ "pickproof",	 EX_PICKPROOF,	 TRUE },
	{ "nopass",	 EX_NOPASS,	 TRUE },
	{ "easy",	 EX_EASY,	 TRUE },
	{ "hard",	 EX_HARD,	 TRUE },
	{ "infuriating", EX_INFURIATING, TRUE },
	{ "noclose",	 EX_NOCLOSE,	 TRUE },
	{ "nolock",	 EX_NOLOCK,	 TRUE },
	{ NULL,		 0,		 0    }
};

const struct flag_type door_resets[] =
{
	{ "open and unlocked",	 0, TRUE },
	{ "closed and unlocked", 1, TRUE },
	{ "closed and locked",	 2, TRUE },
	{ NULL,			 0, 0	 }
};

const struct flag_type room_flags[] =
{
	{ "bank",	  ROOM_BANK,		 TRUE  },
	{ "battlefield",  ROOM_BFIELD,		 FALSE },
	{ "dark",	  ROOM_DARK,		 TRUE  },
	{ "indoors",	  ROOM_INDOORS,		 TRUE  },
	{ "law",	  ROOM_LAW,		 TRUE  },
	{ "no_pushdrag",  ROOM_NO_PUSH_NO_DRAG,	 TRUE  },
	{ "nodream",	  ROOM_NODREAM,		 TRUE  },
	{ "nogate",	  ROOM_NOGATE,		 TRUE  },
	{ "noportal",	  ROOM_NOPORTAL,	 TRUE  },
	{ "nosummon",	  ROOM_NOSUMMON,	 TRUE  },
	{ "noteleport",	  (long)ROOM_NOTELEPORT, TRUE  },
	{ "notransport",  ROOM_NOTRANSPORT,	 TRUE  },
	{ "portalonly",	  ROOM_PORTALONLY,	 TRUE  },
	{ "nowhere",	  ROOM_NOWHERE,		 TRUE  },
	{ "pet_shop",	  ROOM_PET_SHOP,	 TRUE  },
	{ "private",	  ROOM_PRIVATE,		 TRUE  },
	{ "safe",	  ROOM_SAFE,		 TRUE  },
	{ "solitary",	  ROOM_SOLITARY,	 TRUE  },
	{ "norandom",	  ROOM_NORANDOM,	 TRUE  },
	{ "imp_only",	  ROOM_IMP_ONLY,	 TRUE  },
	{ "gods_only",	  ROOM_GODS_ONLY,	 TRUE  },
	{ "heroes_only",  ROOM_HEROES_ONLY,	 TRUE  },
	{ "lev300",	  ROOM_HIGHEST_ONLY,	 TRUE  },
	{ "lev150",	  ROOM_HIGHER_ONLY,	 TRUE  },
	{ "newbies_only", ROOM_NEWBIES_ONLY,	 TRUE  },
	{ "no_mob",	  ROOM_NO_MOB,		 TRUE  },
	{ NULL,		  0,			 0     }
};

const struct flag_type sector_flags[] =
{
	{ "inside",	SECT_INSIDE,	   TRUE },
	{ "city",	SECT_CITY,	   TRUE },
	{ "field",	SECT_FIELD,	   TRUE },
	{ "forest",	SECT_FOREST,	   TRUE },
	{ "hills",	SECT_HILLS,	   TRUE },
	{ "mountain",	SECT_MOUNTAIN,	   TRUE },
	{ "swim",	SECT_WATER_SWIM,   TRUE },
	{ "noswim",	SECT_WATER_NOSWIM, TRUE },
	{ "underwater", SECT_UNDERWATER,   TRUE },
	{ "air",	SECT_AIR,	   TRUE },
	{ "desert",	SECT_DESERT,	   TRUE },
	{ NULL,		0,		   0	}
};


/***************************************************************************
*	flags used by objects
***************************************************************************/
const struct flag_type type_flags[] =
{
	{ "light",	    ITEM_LIGHT,	     TRUE  },
	{ "scroll",	    ITEM_SCROLL,     TRUE  },
	{ "wand",	    ITEM_WAND,	     TRUE  },
	{ "staff",	    ITEM_STAFF,	     TRUE  },
	{ "weapon",	    ITEM_WEAPON,     TRUE  },
	{ "treasure",	    ITEM_TREASURE,   TRUE  },
	{ "armor",	    ITEM_ARMOR,	     TRUE  },
	{ "potion",	    ITEM_POTION,     TRUE  },
	{ "clothing",	    ITEM_CLOTHING,   TRUE  },
	{ "furniture",	    ITEM_FURNITURE,  TRUE  },
	{ "trash",	    ITEM_TRASH,	     TRUE  },
	{ "container",	    ITEM_CONTAINER,  TRUE  },
	{ "drinkcontainer", ITEM_DRINK_CON,  TRUE  },
	{ "key",	    ITEM_KEY,	     TRUE  },
	{ "food",	    ITEM_FOOD,	     TRUE  },
	{ "money",	    ITEM_MONEY,	     TRUE  },
	{ "boat",	    ITEM_BOAT,	     TRUE  },
	{ "npccorpse",	    ITEM_CORPSE_NPC, TRUE  },
	{ "pc corpse",	    ITEM_CORPSE_PC,  FALSE },
	{ "fountain",	    ITEM_FOUNTAIN,   TRUE  },
	{ "pill",	    ITEM_PILL,	     TRUE  },
	{ "map",	    ITEM_MAP,	     TRUE  },
	{ "portal",	    ITEM_PORTAL,     TRUE  },
	{ "warpstone",	    ITEM_WARP_STONE, TRUE  },
	{ "gem",	    ITEM_GEM,	     TRUE  },
	{ "jewelry",	    ITEM_JEWELRY,    TRUE  },
	{ "teleporter",	    ITEM_TELEPORT,   TRUE  },
	{ "atm",	    ITEM_ATM,	     TRUE  },
	{ "invitation",	    ITEM_INVITATION, TRUE  },
	{ "faeriefog",	    ITEM_FAERIE_FOG, FALSE },
	{ "dust",	    ITEM_DUST,	     FALSE },
	{ "doll",	    ITEM_DOLL,	     TRUE  },
	{ "settable",	    ITEM_SOCKETS,    TRUE  },
	{ "dice",	    ITEM_DICE,	     TRUE  },
	{ NULL,		    0,		     0	   }
};

const struct flag_type extra_flags[] =
{
	{ "glow",	 ITEM_GLOW,	    TRUE  },
	{ "hum",	 ITEM_HUM,	    TRUE  },
	{ "dark",	 ITEM_DARK,	    TRUE  },
	{ "lock",	 ITEM_LOCK,	    TRUE  },
	{ "evil",	 ITEM_EVIL,	    TRUE  },
	{ "invis",	 ITEM_INVIS,	    TRUE  },
	{ "magic",	 ITEM_MAGIC,	    TRUE  },
	{ "nodrop",	 ITEM_NODROP,	    TRUE  },
	{ "bless",	 ITEM_BLESS,	    TRUE  },
	{ "antigood",	 ITEM_ANTI_GOOD,    TRUE  },
	{ "antievil",	 ITEM_ANTI_EVIL,    TRUE  },
	{ "antineutral", ITEM_ANTI_NEUTRAL, TRUE  },
	{ "noremove",	 ITEM_NOREMOVE,	    TRUE  },
	{ "inventory",	 ITEM_INVENTORY,    TRUE  },
	{ "nopurge",	 ITEM_NOPURGE,	    TRUE  },
	{ "rotdeath",	 ITEM_ROT_DEATH,    TRUE  },
	{ "visdeath",	 ITEM_VIS_DEATH,    TRUE  },
	{ "nonmetal",	 ITEM_NONMETAL,	    TRUE  },
	{ "meltdrop",	 ITEM_MELT_DROP,    TRUE  },
	{ "hadtimer",	 ITEM_HAD_TIMER,    TRUE  },
	{ "sellextract", ITEM_SELL_EXTRACT, TRUE  },
	{ "burnproof",	 ITEM_BURN_PROOF,   TRUE  },
	{ "nouncurse",	 ITEM_NOUNCURSE,    TRUE  },
	{ "noauction",	 ITEM_NOAUC,	    TRUE  },
	{ "nolocate",	 ITEM_NOLOCATE,	    TRUE  },
	{ "unique",	 ITEM_UNIQUE,	    FALSE },
	{ "gemmed",	 ITEM_INLAY1,	    FALSE },
	{ "bejewelled",	 (long)ITEM_INLAY2, FALSE },
	{ "deathdrop",	 ITEM_DEATH_DROP,   TRUE  },
	{ NULL,		 0,		    0	  }
};

const struct flag_type wear_flags[] =
{
	{ "take",      ITEM_TAKE,	 TRUE },
	{ "finger",    ITEM_WEAR_FINGER, TRUE },
	{ "neck",      ITEM_WEAR_NECK,	 TRUE },
	{ "body",      ITEM_WEAR_BODY,	 TRUE },
	{ "head",      ITEM_WEAR_HEAD,	 TRUE },
	{ "face",      ITEM_WEAR_FACE,	 TRUE },
	{ "ear",       ITEM_WEAR_EAR,	 TRUE },
	{ "legs",      ITEM_WEAR_LEGS,	 TRUE },
	{ "feet",      ITEM_WEAR_FEET,	 TRUE },
	{ "hands",     ITEM_WEAR_HANDS,	 TRUE },
	{ "arms",      ITEM_WEAR_ARMS,	 TRUE },
	{ "shield",    ITEM_WEAR_SHIELD, TRUE },
	{ "about",     ITEM_WEAR_ABOUT,	 TRUE },
	{ "waist",     ITEM_WEAR_WAIST,	 TRUE },
	{ "wrist",     ITEM_WEAR_WRIST,	 TRUE },
	{ "wield",     ITEM_WIELD,	 TRUE },
	{ "hold",      ITEM_HOLD,	 TRUE },
	{ "tattoo",    ITEM_WEAR_TATTOO, TRUE },
	{ "nosac",     ITEM_NO_SAC,	 TRUE },
	{ "wearfloat", ITEM_WEAR_FLOAT,	 TRUE },
/*  { "twohands",		ITEM_TWO_HANDS,         TRUE    }, */
	{ NULL,	       0,		 0    }
};

const struct flag_type apply_flags[] =
{
	{ "none",	  APPLY_NONE,	       TRUE  },
	{ "strength",	  APPLY_STR,	       TRUE  },
	{ "dexterity",	  APPLY_DEX,	       TRUE  },
	{ "intelligence", APPLY_INT,	       TRUE  },
	{ "wisdom",	  APPLY_WIS,	       TRUE  },
	{ "constitution", APPLY_CON,	       TRUE  },
	{ "sex",	  APPLY_SEX,	       TRUE  },
	{ "class",	  APPLY_CLASS,	       TRUE  },
	{ "level",	  APPLY_LEVEL,	       TRUE  },
	{ "age",	  APPLY_AGE,	       TRUE  },
	{ "height",	  APPLY_HEIGHT,	       TRUE  },
	{ "weight",	  APPLY_WEIGHT,	       TRUE  },
	{ "mana",	  APPLY_MANA,	       TRUE  },
	{ "hp",		  APPLY_HIT,	       TRUE  },
	{ "move",	  APPLY_MOVE,	       TRUE  },
	{ "gold",	  APPLY_GOLD,	       TRUE  },
	{ "experience",	  APPLY_EXP,	       TRUE  },
	{ "ac",		  APPLY_AC,	       TRUE  },
	{ "hitroll",	  APPLY_HITROLL,       TRUE  },
	{ "damroll",	  APPLY_DAMROLL,       TRUE  },
	{ "saves",	  APPLY_SAVES,	       TRUE  },
	{ "savingpara",	  APPLY_SAVING_PARA,   TRUE  },
	{ "savingrod",	  APPLY_SAVING_ROD,    TRUE  },
	{ "savingpetri",  APPLY_SAVING_PETRI,  TRUE  },
	{ "savingbreath", APPLY_SAVING_BREATH, TRUE  },
	{ "savingspell",  APPLY_SAVING_SPELL,  TRUE  },
	{ "spellaffect",  APPLY_SPELL_AFFECT,  FALSE },
	{ "my_lag",	  APPLY_MLAG,	       FALSE },
	{ "their_lag",	  APPLY_TLAG,	       FALSE },
	{ NULL,		  0,		       0     }
};

const struct flag_type wear_loc_strings[] =
{
	{ "in the inventory",	 WEAR_NONE,	TRUE },
	{ "as a light",		 WEAR_LIGHT,	TRUE },
	{ "on the left finger",	 WEAR_FINGER_L, TRUE },
	{ "on the right finger", WEAR_FINGER_R, TRUE },
	{ "around the neck (1)", WEAR_NECK_1,	TRUE },
	{ "around the neck (2)", WEAR_NECK_2,	TRUE },
	{ "on the body",	 WEAR_BODY,	TRUE },
	{ "over the head",	 WEAR_HEAD,	TRUE },
	{ "on the face",	 WEAR_FACE,	TRUE },
	{ "as a tattoo",	 WEAR_TATTOO,	TRUE },
	{ "on the legs",	 WEAR_LEGS,	TRUE },
	{ "on the feet",	 WEAR_FEET,	TRUE },
	{ "on the hands",	 WEAR_HANDS,	TRUE },
	{ "on the arms",	 WEAR_ARMS,	TRUE },
	{ "as a shield",	 WEAR_SHIELD,	TRUE },
	{ "about the shoulders", WEAR_ABOUT,	TRUE },
	{ "around the waist",	 WEAR_WAIST,	TRUE },
	{ "on the left wrist",	 WEAR_WRIST_L,	TRUE },
	{ "on the right wrist",	 WEAR_WRIST_R,	TRUE },
	{ "wielded",		 WEAR_WIELD,	TRUE },
	{ "held in the hands",	 WEAR_HOLD,	TRUE },
	{ "floating nearby",	 WEAR_FLOAT,	TRUE },
	{ NULL,			 0,		0    },
};

const struct flag_type wear_loc_flags[] =
{
	{ "none",     WEAR_NONE,     TRUE },
	{ "light",    WEAR_LIGHT,    TRUE },
	{ "lfinger",  WEAR_FINGER_L, TRUE },
	{ "rfinger",  WEAR_FINGER_R, TRUE },
	{ "neck1",    WEAR_NECK_1,   TRUE },
	{ "neck2",    WEAR_NECK_2,   TRUE },
	{ "body",     WEAR_BODY,     TRUE },
	{ "head",     WEAR_HEAD,     TRUE },
	{ "face",     WEAR_FACE,     TRUE },
	{ "tattoo",   WEAR_TATTOO,   TRUE },
	{ "lear",     WEAR_EAR_L,    TRUE },
	{ "rear",     WEAR_EAR_R,    TRUE },
	{ "legs",     WEAR_LEGS,     TRUE },
	{ "feet",     WEAR_FEET,     TRUE },
	{ "hands",    WEAR_HANDS,    TRUE },
	{ "arms",     WEAR_ARMS,     TRUE },
	{ "shield",   WEAR_SHIELD,   TRUE },
	{ "about",    WEAR_ABOUT,    TRUE },
	{ "waist",    WEAR_WAIST,    TRUE },
	{ "lwrist",   WEAR_WRIST_L,  TRUE },
	{ "rwrist",   WEAR_WRIST_R,  TRUE },
	{ "wielded",  WEAR_WIELD,    TRUE },
	{ "hold",     WEAR_HOLD,     TRUE },
	{ "floating", WEAR_FLOAT,    TRUE },
	{ NULL,	      0,	     0	  }
};

const struct flag_type container_flags[] =
{
	{ "closeable", 1,  TRUE },
	{ "pickproof", 2,  TRUE },
	{ "closed",    4,  TRUE },
	{ "locked",    8,  TRUE },
	{ "puton",     16, TRUE },
	{ NULL,	       0,  0	}
};

const struct flag_type ac_type[] =
{
	{ "pierce", AC_PIERCE, TRUE },
	{ "bash",   AC_BASH,   TRUE },
	{ "slash",  AC_SLASH,  TRUE },
	{ "exotic", AC_EXOTIC, TRUE },
	{ NULL,	    0,	       0    }
};

const struct flag_type weapon_class[] =
{
	{ "exotic",  WEAPON_EXOTIC,  TRUE },
	{ "sword",   WEAPON_SWORD,   TRUE },
	{ "dagger",  WEAPON_DAGGER,  TRUE },
	{ "spear",   WEAPON_SPEAR,   TRUE },
	{ "mace",    WEAPON_MACE,    TRUE },
	{ "axe",     WEAPON_AXE,     TRUE },
	{ "flail",   WEAPON_FLAIL,   TRUE },
	{ "whip",    WEAPON_WHIP,    TRUE },
	{ "polearm", WEAPON_POLEARM, TRUE },
	{ NULL,	     0,		     0	  }
};

const struct flag_type weapon_flag_type[] =
{
	{ "flaming",  WEAPON_FLAMING,	TRUE },
	{ "frost",    WEAPON_FROST,	TRUE },
	{ "vampiric", WEAPON_VAMPIRIC,	TRUE },
	{ "sharp",    WEAPON_SHARP,	TRUE },
	{ "vorpal",   WEAPON_VORPAL,	TRUE },
	{ "twohands", WEAPON_TWO_HANDS, TRUE },
	{ "shocking", WEAPON_SHOCKING,	TRUE },
	{ "poison",   WEAPON_POISON,	TRUE },
	{ "acidic",   WEAPON_ACIDIC,	TRUE },
	{ NULL,	      0,		0    }
};

/***************************************************************************
*	**THIS TABLE IS ALL WRONG***
***************************************************************************/
const struct flag_type socket_flags[] =
{
	{ "sapphire", SOC_SAPPHIRE, TRUE },
	{ "ruby",     SOC_RUBY,	    TRUE },
	{ "emerald",  SOC_EMERALD,  TRUE },
	{ "diamond",  SOC_DIAMOND,  TRUE },
	{ "topaz",    SOC_TOPAZ,    TRUE },
	{ "skull",    SOC_SKULL,    TRUE },
	{ "",	      0,	    0	 }
};

/***************************************************************************
*	**THIS TABLE IS ALL WRONG***
***************************************************************************/
const struct flag_type socket_values[] =
{
	{ "chip",     GEM_CHIPPED,  TRUE },
	{ "flawed",   GEM_FLAWED,   TRUE },
	{ "flawless", GEM_FLAWLESS, TRUE },
	{ "perfect",  GEM_PERFECT,  TRUE },
	{ "",	      0,	    0	 }
};

const struct flag_type portal_flags[] =
{
	{ "normal_exit", GATE_NORMAL_EXIT, TRUE },
	{ "no_curse",	 GATE_NOCURSE,	   TRUE },
	{ "go_with",	 GATE_GOWITH,	   TRUE },
	{ "buggy",	 GATE_BUGGY,	   TRUE },
	{ "random",	 GATE_RANDOM,	   TRUE },
	{ NULL,		 0,		   0	}
};

const struct flag_type furniture_flags[] =
{
	{ "stand_at",	STAND_AT,   TRUE },
	{ "stand_on",	STAND_ON,   TRUE },
	{ "stand_in",	STAND_IN,   TRUE },
	{ "sit_at",	SIT_AT,	    TRUE },
	{ "sit_on",	SIT_ON,	    TRUE },
	{ "sit_in",	SIT_IN,	    TRUE },
	{ "rest_at",	REST_AT,    TRUE },
	{ "rest_on",	REST_ON,    TRUE },
	{ "rest_in",	REST_IN,    TRUE },
	{ "sleep_at",	SLEEP_AT,   TRUE },
	{ "sleep_on",	SLEEP_ON,   TRUE },
	{ "sleep_in",	SLEEP_IN,   TRUE },
	{ "put_at",	PUT_AT,	    TRUE },
	{ "put_on",	PUT_ON,	    TRUE },
	{ "put_in",	PUT_IN,	    TRUE },
	{ "put_inside", PUT_INSIDE, TRUE },
	{ NULL,		0,	    0	 }
};

const struct  flag_type apply_types     [] =
{
	{ "affects", TO_AFFECTS, TRUE },
	{ "object",  TO_OBJECT,	 TRUE },
	{ "immune",  TO_IMMUNE,	 TRUE },
	{ "resist",  TO_RESIST,	 TRUE },
	{ "vuln",    TO_VULN,	 TRUE },
	{ "weapon",  TO_WEAPON,	 TRUE },
	{ NULL,	     0,		 TRUE }
};

const struct flag_type damage_flags[] =
{
	{ "slice",		1,  TRUE  },
	{ "stab",		2,  TRUE  },
	{ "slash",		3,  TRUE  },
	{ "whip",		4,  TRUE  },
	{ "claw",		5,  TRUE  },                                            /*  5 */
	{ "blast",		6,  TRUE  },
	{ "pound",		7,  TRUE  },
	{ "crush",		8,  TRUE  },
	{ "grep",		9,  TRUE  },
	{ "bite",		10, TRUE  },                                    /* 10 */
	{ "pierce",		11, TRUE  },
	{ "suction",		12, TRUE  },
	{ "beating",		13, TRUE  },
	{ "digestion",		14, TRUE  },
	{ "charge",		15, TRUE  },                                            /* 15 */
	{ "slap",		16, TRUE  },
	{ "punch",		17, TRUE  },
	{ "magic",		19, TRUE  },
	{ "divine power",	20, TRUE  },                                    /* 20 */
	{ "cleave",		21, TRUE  },
	{ "scratch",		22, TRUE  },
	{ "peck",		23, TRUE  },
	{ "peck",		24, TRUE  },
	{ "chop",		25, TRUE  },                                            /* 25 */
	{ "sting",		26, TRUE  },
	{ "smash",		27, TRUE  },
	{ "shocking bite",	28, TRUE  },
	{ "flaming bite",	29, TRUE  },
	{ "freezing bite",	30, TRUE  },                                    /* 30 */
	{ "acidic_bite",	31, TRUE  },
	{ "chomp",		32, TRUE  },
	{ "life drain",		33, TRUE  },
	{ "thrust",		34, TRUE  },
	{ "slime",		35, TRUE  },                                            /* 35 */
	{ "shock",		36, TRUE  },
	{ "thwack",		37, TRUE  },
	{ "flame",		38, TRUE  },
	{ "chill",		39, TRUE  },
	{ "tracers",		40, TRUE  },                                            /* 40 */
	{ "splinters",		42, TRUE  },
	{ "penstroke",		43, TRUE  },
	{ "evil stroke",	44, TRUE  },
	{ "vile caress",	45, TRUE  },                                    /* 45 */
	{ "withered touch",	46, TRUE  },
	{ "presence",		47, TRUE  },
	{ "passive resistance", 48, TRUE  },
	{ "darkness",		49, TRUE  },
	{ "singing",		50, TRUE  },                                            /* 50 */
	{ "judgement",		51, TRUE  },
	{ "stare",		52, TRUE  },
	{ "deathstroke",	53, TRUE  },
	{ "curse",		54, TRUE  },
	{ "shockwave",		55, TRUE  },                                            /* 55 */
	{ "gush",		57, TRUE  },
	{ NULL,			0,  FALSE }
};

const struct flag_type target_flags[] =
{
	{ "ignore",	   TAR_IGNORE,	       TRUE },
	{ "offensive",	   TAR_CHAR_OFFENSIVE, TRUE },
	{ "defensive",	   TAR_CHAR_DEFENSIVE, TRUE },
	{ "self",	   TAR_CHAR_SELF,      TRUE },
	{ "inventory",	   TAR_OBJ_INV,	       TRUE },
	{ "obj_defensive", TAR_OBJ_CHAR_DEF,   TRUE },
	{ "obj_offensive", TAR_OBJ_CHAR_OFF,   TRUE },
	{ "room",	   TAR_ROOM,	       TRUE },
	{ NULL,		   0,		       0    }
};

const struct flag_type skill_flags[] =
{
	{ "noscribe",	 SPELL_NOSCRIBE,    TRUE },
	{ "nobrew",	 SPELL_NOBREW,	    TRUE },
	{ "dispellable", SPELL_DISPELLABLE, TRUE },
	{ "cancelable",	 SPELL_CANCELABLE,  TRUE },
	{ "affstrip",	 SPELL_AFFSTRIP,    TRUE },
	{ NULL,		 0,		    0	 }
};

const struct flag_type extra2_flags[] =
{
	{ "graft",	ITEM2_GRAFT,	  TRUE },
	{ "ethereal",	ITEM2_ETHEREAL,	  TRUE },
	{ "noscan",	ITEM2_NOSCAN,	  TRUE },
	{ "relic",	ITEM2_RELIC,	  TRUE },
	{ "nodonate",	ITEM2_NODONATE,	  TRUE },
	{ "norestring", ITEM2_NORESTRING, TRUE },
	{ NULL,		0,		  0    }
};
