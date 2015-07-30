/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
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

/***************************************************************************
*	includes
***************************************************************************/
#include "merc.h"
#include "magic.h"
#include "interp.h"


/* item type list */
const struct item_type item_table[] =
{
	{ ITEM_LIGHT,	   "light",	    "ITEM_LIGHT"      },
	{ ITEM_SCROLL,	   "scroll",	    "ITEM_SCROLL"     },
	{ ITEM_WAND,	   "wand",	    "ITEM_WAND"	      },
	{ ITEM_STAFF,	   "staff",	    "ITEM_STAFF"      },
	{ ITEM_WEAPON,	   "weapon",	    "ITEM_WEAPON"     },
	{ ITEM_TREASURE,   "treasure",	    "ITEM_TREASURE"   },
	{ ITEM_ARMOR,	   "armor",	    "ITEM_ARMOR"      },
	{ ITEM_POTION,	   "potion",	    "ITEM_POTION"     },
	{ ITEM_CLOTHING,   "clothing",	    "ITEM_CLOTHING"   },
	{ ITEM_FURNITURE,  "furniture",	    "ITEM_FURNITURE"  },
	{ ITEM_TRASH,	   "trash",	    ""		      },
	{ ITEM_CONTAINER,  "container",	    "ITEM_CONTAINER"  },
	{ ITEM_DRINK_CON,  "drink",	    "ITEM_DRINK"      },
	{ ITEM_KEY,	   "key",	    "ITEM_KEY"	      },
	{ ITEM_FOOD,	   "food",	    "ITEM_FOOD"	      },
	{ ITEM_MONEY,	   "money",	    "ITEM_MONEY"      },
	{ ITEM_BOAT,	   "boat",	    "ITEM_BOAT"	      },
	{ ITEM_CORPSE_NPC, "npc_corpse",    "ITEM_CORPSE_NPC" },
	{ ITEM_CORPSE_PC,  "pc_corpse",	    "ITEM_CORPSE_PC"  },
	{ ITEM_FOUNTAIN,   "fountain",	    "ITEM_FOUNTAIN"   },
	{ ITEM_PILL,	   "pill",	    "ITEM_PILL"	      },
	{ ITEM_PROTECT,	   "protect",	    ""		      },
	{ ITEM_MAP,	   "map",	    "ITEM_MAP"	      },
	{ ITEM_PORTAL,	   "portal",	    "ITEM_PORTAL"     },
	{ ITEM_WARP_STONE, "warp_stone",    "ITEM_WARP_STONE" },
	{ ITEM_GEM,	   "gem",	    "ITEM_GEM"	      },
	{ ITEM_JEWELRY,	   "jewelry",	    "ITEM_JEWELRY"    },
	{ ITEM_TELEPORT,   "teleporter",    "ITEM_TELEPORTER" },
	{ ITEM_ATM,	   "atm_machine",   "ITEM_ATM"	      },
	{ ITEM_SCARAB,	   "scarab",	    "ITEM_SCARAB"     },
	{ ITEM_DOLL,	   "doll",	    "ITEM_DOLL"	      },
	{ ITEM_INVITATION, "invitation",    "ITEM_INVITATION" },
	{ ITEM_SOCKETS,	   "settable",	    "ITEM_SETTABLE"   },
	{ ITEM_DICE,	   "dice",	    "ITEM_DICE"	      },
	{ 0,		   NULL,	    ""		      }
};


/* weapon selection table */
const struct weapon_type weapon_table[] =
{
	{ "sword",   OBJ_VNUM_SCHOOL_SWORD,   WEAPON_SWORD,   &gsp_sword   },
	{ "mace",    OBJ_VNUM_SCHOOL_MACE,    WEAPON_MACE,    &gsp_mace	   },
	{ "dagger",  OBJ_VNUM_SCHOOL_DAGGER,  WEAPON_DAGGER,  &gsp_dagger  },
	{ "axe",     OBJ_VNUM_SCHOOL_AXE,     WEAPON_AXE,     &gsp_axe	   },
	{ "staff",   OBJ_VNUM_SCHOOL_STAFF,   WEAPON_SPEAR,   &gsp_spear   },
	{ "flail",   OBJ_VNUM_SCHOOL_FLAIL,   WEAPON_FLAIL,   &gsp_flail   },
	{ "whip",    OBJ_VNUM_SCHOOL_WHIP,    WEAPON_WHIP,    &gsp_whip	   },
	{ "polearm", OBJ_VNUM_SCHOOL_POLEARM, WEAPON_POLEARM, &gsp_polearm },
	{ NULL,	     0,			      0,	      NULL	   }
};



/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] =
{
	{ "on",	       WIZ_ON,	      IM },
	{ "prefix",    WIZ_PREFIX,    IM },
	{ "ticks",     WIZ_TICKS,     IM },
	{ "logins",    WIZ_LOGINS,    IM },
	{ "sites",     WIZ_SITES,     L4 },
	{ "links",     WIZ_LINKS,     L7 },
	{ "newbies",   WIZ_NEWBIE,    IM },
	{ "spam",      WIZ_SPAM,      L5 },
	{ "deaths",    WIZ_DEATHS,    IM },
	{ "flags",     WIZ_FLAGS,     L5 },
	{ "penalties", WIZ_PENALTIES, L5 },
	{ "saccing",   WIZ_SACCING,   L5 },
	{ "levels",    WIZ_LEVELS,    IM },
	{ "load",      WIZ_LOAD,      L2 },
	{ "restore",   WIZ_RESTORE,   L2 },
	{ "snoops",    WIZ_SNOOPS,    L2 },
	{ "switches",  WIZ_SWITCHES,  L2 },
	{ "secure",    WIZ_SECURE,    L1 },
	{ "alog",      WIZ_PLOG,      L1 },
	{ "plog",      WIZ_ALOG,      L1 },
	{ "rp",	       WIZ_ROLEPLAY,  L4 },
	{ NULL,	       0,	      0	 }
};

/* impnet table and prototype for future flag setting */
const struct impnet_type impnet_table[] =
{
	{ "on",		IMN_ON,	       L1 },
	{ "prefix",	IMN_PREFIX,    L1 },
	{ "ticks",	IMN_TICKS,     L1 },
	{ "resets",	IMN_RESETS,    L1 },
	{ "mobdeaths",	IMN_MOBDEATHS, L1 },
	{ "saves",	IMN_SAVES,     L1 },
	{ "updates",	IMN_UPDATES,   L1 },
	{ "automation", IMN_AUTO,      L1 },
	{ NULL,		0,	       0  }
};

/* attack table  -- not very organized :( */
const struct attack_type attack_table[] =
{
	{ "none",	 "hit",		       -1	     },                                                 /*  0 */
	{ "slice",	 "slice",	       DAM_SLASH     },
	{ "stab",	 "stab",	       DAM_PIERCE    },
	{ "slash",	 "slash",	       DAM_SLASH     },
	{ "whip",	 "whip",	       DAM_SLASH     },
	{ "claw",	 "claw",	       DAM_SLASH     },                                         /*  5 */
	{ "blast",	 "blast",	       DAM_BASH	     },
	{ "pound",	 "pound",	       DAM_BASH	     },
	{ "crush",	 "crush",	       DAM_BASH	     },
	{ "grep",	 "grep",	       DAM_SLASH     },
	{ "bite",	 "bite",	       DAM_PIERCE    },                                         /* 10 */
	{ "pierce",	 "pierce",	       DAM_PIERCE    },
	{ "suction",	 "suction",	       DAM_BASH	     },
	{ "beating",	 "beating",	       DAM_BASH	     },
	{ "digestion",	 "digestion",	       DAM_ACID	     },
	{ "charge",	 "charge",	       DAM_BASH	     },                                         /* 15 */
	{ "slap",	 "slap",	       DAM_BASH	     },
	{ "punch",	 "punch",	       DAM_BASH	     },
	{ "magic",	 "magic",	       DAM_ENERGY    },
	{ "divine",	 "divine power",       DAM_HOLY	     },                                 /* 20 */
	{ "cleave",	 "cleave",	       DAM_SLASH     },
	{ "scratch",	 "scratch",	       DAM_PIERCE    },
	{ "peck",	 "peck",	       DAM_PIERCE    },
	{ "peckb",	 "peck",	       DAM_BASH	     },
	{ "chop",	 "chop",	       DAM_SLASH     },                                         /* 25 */
	{ "sting",	 "sting",	       DAM_PIERCE    },
	{ "smash",	 "smash",	       DAM_BASH	     },
	{ "shbite",	 "shocking bite",      DAM_LIGHTNING },
	{ "flbite",	 "flaming bite",       DAM_FIRE	     },
	{ "frbite",	 "freezing bite",      DAM_COLD	     },                                 /* 30 */
	{ "acbite",	 "acidic bite",	       DAM_ACID	     },
	{ "chomp",	 "chomp",	       DAM_PIERCE    },
	{ "drain",	 "life drain",	       DAM_NEGATIVE  },
	{ "thrust",	 "thrust",	       DAM_PIERCE    },
	{ "slime",	 "slime",	       DAM_ACID	     },                                         /* 35 */
	{ "shock",	 "shock",	       DAM_LIGHTNING },
	{ "thwack",	 "thwack",	       DAM_BASH	     },
	{ "flame",	 "flame",	       DAM_FIRE	     },
	{ "chill",	 "chill",	       DAM_COLD	     },
	{ "tracers",	 "tracers",	       DAM_LIGHT     },                                         /* 40 */
	{ "splinters",	 "splinters",	       DAM_WOOD	     },
	{ "penstroke",	 "penstroke",	       DAM_PIERCE    },
	{ "evil stroke", "evil stroke",	       DAM_NEGATIVE  },
	{ "vile caress", "vile caress",	       DAM_COLD	     },                         /* 45 */
	{ "wtouch",	 "withered touch",     DAM_ENERGY    },
	{ "presence",	 "presence",	       DAM_NEGATIVE  },
	{ "presist",	 "passive resistance", DAM_HOLY	     },
	{ "darkness",	 "darkness",	       DAM_NEGATIVE  },
	{ "singing",	 "singing",	       DAM_PIERCE    },                                         /* 50 */
	{ "judgement",	 "judgement",	       DAM_HOLY	     },
	{ "stare",	 "stare",	       DAM_PIERCE    },
	{ "deathstroke", "deathstroke",	       DAM_PIERCE    },
	{ "curse",	 "curse",	       DAM_NEGATIVE  },
	{ "shockwave",	 "shockwave",	       DAM_SOUND     },                                 /* 55 */
	{ "gush",	 "gush",	       DAM_DROWNING  },
	{ NULL,		 NULL,		       0	     }
};

/* race table */
const struct race_type race_table[] =
{
/*
 * {
 *         name,                pc_race?,
 *         act bits,    aff_by bits,    off bits,
 *         imm,         res,            vuln,
 *         form,                parts
 * },
 */
	{ "unique", FALSE, 0, 0, 0, 0, 0, 0, 0, 0 },

	{
		"human", TRUE,
		0, 0, 0,
		0, 0, 0,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"elf", TRUE,
		0, AFF_INFRARED, 0,
		0, RES_CHARM, VULN_IRON,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"dwarf", TRUE,
		0, AFF_INFRARED, 0,
		IMM_ACID, RES_POISON | RES_DISEASE, VULN_DROWNING,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},


	{
		"giant", TRUE,
		0, 0, 0,
		IMM_WOOD | IMM_SOUND, RES_FIRE | RES_COLD, VULN_MENTAL | VULN_LIGHTNING,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},


	{
		"feline", TRUE,
		0, AFF_DARK_VISION | AFF_DETECT_INVIS | AFF_SNEAK, 0,
		0, RES_BASH | RES_DISEASE, VULN_DROWNING,
		A | H | V, A | C | D | E | F | H | I | J | K | Q | U | V
	},

	{
		"aiel", TRUE,
		0, AFF_SNEAK | AFF_DETECT_INVIS, 0,
		0, RES_WEAPON | RES_BASH, VULN_DROWNING,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"mutant", TRUE,
		0, AFF_SNEAK | AFF_DARK_VISION, 0,
		IMM_LIGHTNING, RES_MENTAL | RES_DISEASE, VULN_COLD,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"dragon", TRUE,
		0, AFF_DETECT_INVIS | AFF_FLYING, 0,
		IMM_FIRE | IMM_COLD | IMM_LIGHTNING, RES_WEAPON, VULN_ACID,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"darkelf", TRUE,
		0, AFF_DARK_VISION, 0,
		0, RES_CHARM, VULN_IRON | VULN_DROWNING,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"werebeast", TRUE,
		0, AFF_SNEAK | AFF_DETECT_INVIS, 0,
		IMM_DISEASE, RES_IRON, VULN_SILVER | VULN_HOLY,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"vampire", TRUE,
		0, AFF_DARK_VISION | AFF_DETECT_INVIS | AFF_SNEAK, 0,
		IMM_DISEASE, RES_IRON, VULN_WOOD | VULN_HOLY,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"sprite", TRUE,
		0, AFF_FLYING | AFF_INVISIBLE | AFF_DARK_VISION | AFF_DETECT_INVIS, 0,
		IMM_DISEASE, RES_NEGATIVE | RES_WEAPON | RES_MAGIC | RES_CHARM, VULN_POISON | VULN_ACID,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"centaur", TRUE,
		0, 0, 0,
		IMM_DROWNING, RES_MAGIC, VULN_SOUND,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"bat", FALSE,
		0, AFF_FLYING | AFF_DARK_VISION, OFF_DODGE | OFF_FAST,
		0, 0, VULN_LIGHT,
		A | G | V, A | C | D | E | F | H | J | K | P
	},

	{
		"bear", FALSE,
		0, 0, OFF_CRUSH | OFF_DISARM | OFF_BERSERK,
		0, RES_BASH | RES_COLD, 0,
		A | G | V, A | B | C | D | E | F | H | J | K | U | V
	},

	{
		"cat", FALSE,
		0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
		0, 0, 0,
		A | G | V, A | C | D | E | F | H | J | K | Q | U | V
	},

	{
		"centipede", FALSE,
		0, AFF_DARK_VISION, 0,
		0, RES_PIERCE | RES_COLD, VULN_BASH,
		A | B | G | O, A | C | K
	},

	{
		"dog", FALSE,
		0, 0, OFF_FAST,
		0, 0, 0,
		A | G | V, A | C | D | E | F | H | J | K | U | V
	},

	{
		"doll", FALSE,
		0, 0, 0,
		IMM_COLD | IMM_POISON | IMM_HOLY | IMM_NEGATIVE | IMM_MENTAL | IMM_DISEASE
		| IMM_DROWNING, RES_BASH | RES_LIGHT,
		VULN_SLASH | VULN_FIRE | VULN_ACID | VULN_LIGHTNING | VULN_ENERGY,
		E | J | M | cc, A | B | C | G | H | K
	},

	{
		"dragon", FALSE,
		0, AFF_INFRARED | AFF_FLYING, 0,
		0, RES_FIRE | RES_BASH | RES_CHARM,
		VULN_PIERCE | VULN_COLD,
		A | H | Z, A | C | D | E | F | G | H | I | J | K | P | Q | U | V | X
	},

	{
		"fido", FALSE,
		0, 0, OFF_DODGE | ASSIST_RACE,
		0, 0, VULN_MAGIC,
		A | B | G | V, A | C | D | E | F | H | J | K | Q | V
	},

	{
		"fox", FALSE,
		0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
		0, 0, 0,
		A | G | V, A | C | D | E | F | H | J | K | Q | V
	},

	{
		"goblin", FALSE,
		0, AFF_INFRARED, 0,
		0, RES_DISEASE, VULN_MAGIC,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"hobgoblin", FALSE,
		0, AFF_INFRARED, 0,
		0, RES_DISEASE | RES_POISON, 0,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Y
	},

	{
		"kobold", FALSE,
		0, AFF_INFRARED, 0,
		0, RES_POISON, VULN_MAGIC,
		A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Q
	},

	{
		"lizard", FALSE,
		0, 0, 0,
		0, RES_POISON, VULN_COLD,
		A | G | X | cc, A | C | D | E | F | H | K | Q | V
	},

	{
		"modron", FALSE,
		0, AFF_INFRARED, ASSIST_RACE | ASSIST_ALIGN,
		IMM_CHARM | IMM_DISEASE | IMM_MENTAL | IMM_HOLY | IMM_NEGATIVE,
		RES_FIRE | RES_COLD | RES_ACID, 0,
		H, A | B | C | G | H | J | K
	},

	{
		"orc", FALSE,
		0, AFF_INFRARED, 0,
		0, RES_DISEASE, VULN_LIGHT,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{
		"pig", FALSE,
		0, 0, 0,
		0, 0, 0,
		A | G | V, A | C | D | E | F | H | J | K
	},

	{
		"rabbit", FALSE,
		0, 0, OFF_DODGE | OFF_FAST,
		0, 0, 0,
		A | G | V, A | C | D | E | F | H | J | K
	},

	{
		"school monster", FALSE,
		ACT_NOALIGN, 0, 0,
		IMM_CHARM | IMM_SUMMON, 0, VULN_MAGIC,
		A | M | V, A | B | C | D | E | F | H | J | K | Q | U
	},

	{
		"snake", FALSE,
		0, 0, 0,
		0, RES_POISON, VULN_COLD,
		A | G | X | Y | cc, A | D | E | F | K | L | Q | V | X
	},

	{
		"song bird", FALSE,
		0, AFF_FLYING, OFF_FAST | OFF_DODGE,
		0, 0, 0,
		A | G | W, A | C | D | E | F | H | K | P
	},

	{
		"troll", FALSE,
		0, AFF_INFRARED, OFF_BERSERK,
		0, RES_CHARM | RES_BASH, VULN_FIRE | VULN_ACID,
		A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | U | V
	},

	{
		"water fowl", FALSE,
		0, AFF_FLYING, 0,
		0, RES_DROWNING, 0,
		A | G | W, A | C | D | E | F | H | K | P
	},

	{
		"wolf", FALSE,
		0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
		0, 0, 0,
		A | G | V, A | C | D | E | F | J | K | Q | V
	},

	{
		"wyvern", FALSE,
		0, AFF_FLYING | AFF_DETECT_INVIS, OFF_BASH | OFF_FAST | OFF_DODGE,
		IMM_POISON, 0, VULN_LIGHT,
		A | B | G | Z, A | C | D | E | F | H | J | K | Q | V | X
	},

	{
		"unique", FALSE,
		0, 0, 0,
		0, 0, 0,
		0, 0
	},


	{
		NULL, FALSE, 0, 0, 0, 0, 0, 0, 0, 0
	}
};

const struct pc_race_type pc_race_table[] =
{
	{
		"", "", -1,
		{ -1,		   -1,		   -1,		      -1,  -1  },
		{ "" },
		{ -1,		   -1,		   -1,		      -1,  -1, -1},
		{ -1,		   -1,		   -1,		      -1,  -1, -1},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		-1
	},

	/*
	 * {
	 *             "race name",        short name,     points,
	 *             { class multipliers },
	 *             { bonus skills },
	 *             { base stats },
	 *             { max stats },          size
	 * },
	 */
	{
		"Human", "Human", 0,
		{ 100,		   100,		   100,		      100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
		{ "dragonslay" },
		{ 13,		   13,		   13,		      13,  13, 9},
		{ 18,		   22,		   22,		      18,  21, 25},
		{ -1,		   -1,		   -1,		      -1,  -1, 25},
		SIZE_MEDIUM
	},

	{
		"Elf", "Elf  ", 5,
		{ 125,		   150,		   125,		      145, 125, 125, 150, 125, 145, 125, 125, 150, 125, 145, 125},
		{ "sneak",	   "hide" },
		{ 12,		   14,		   13,		      15,  11, 9},
		{ 16,		   28,		   24,		      28,  15, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_SMALL
	},

	{
		"Dwarf", "Dwarf", 8,
		{ 290,		   140,		   165,		      140, 240, 290, 140, 165, 140, 240, 290, 140, 165, 140, 240},
		{ "berserk",	   "kneecap" },
		{ 14,		   14,		   14,		      10,  15, 9},
		{ 28,		   18,		   23,		      19,  33, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_SMALL
	},

	{
		"Giant", "Giant", 6,
		{ 100,		   100,		   100,		      100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
		{ "bash",	   "fast healing", "crush",	      "throw" },
		{ 16,		   11,		   13,		      11,  14, 9},
		{ 40,		   15,		   18,		      19,  23, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_HUGE
	},

	{
		"Feline", "Felin", 0,
		{ 125,		   125,		   125,		      125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125},
		{ "hide",	   "dodge",	   "hiss",	      "rake" },
		{ 13,		   15,		   16,		      18,  13, 9},
		{ 17,		   23,		   22,		      30,  17, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_SMALL
	},

	{
		"Aiel", "Aiel ", 6,
		{ 210,		   210,		   230,		      180, 210, 210, 210, 230, 180, 210, 210, 210, 230, 180, 210},
		{ "veil",	   "hand to hand", "dodge",	      "disarm" },
		{ 15,		   15,		   14,		      16,  17, 9},
		{ 25,		   22,		   19,		      25,  30, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_MEDIUM
	},

	{
		"Mutant", "Mutnt", 5,
		{ 150,		   150,		   150,		      150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150},
		{ "fast healing",  "meditation",   "lore" },
		{ 12,		   12,		   12,		      12,  12, 9},
		{ 25,		   23,		   22,		      20,  25, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_MEDIUM
	},

	{
		"Dragon", "Drago", 14,
		{ 300,		   325,		   310,		      300, 320, 300, 325, 310, 300, 320, 300, 325, 310, 300, 320},
		{ "parry",	   "disarm",	   "meditation",      "tooth and claw" },
		{ 15,		   16,		   16,		      15,  17, 9},
		{ 30,		   30,		   35,		      25,  30, 25},
		{ -1,		   -1,		   -1,		      -1,  -1, 25},
		SIZE_HUGE
	},

	{
		"DarkElf", "D.Elf", 7,
		{ 125,		   150,		   125,		      145, 130, 125, 150, 125, 145, 130, 125, 150, 125, 145, 130},
		{ "sneak",	   "fast healing", "second attack" },
		{ 12,		   13,		   13,		      15,  12, 9},
		{ 24,		   20,		   18,		      24,  25, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_SMALL
	},

	{
		"WereBeast", "Beast", 8,
		{ 150,		   150,		   150,		      150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150},
		{ "fourth attack", "fast healing", "enhanced damage", "bite" },
		{ 17,		   17,		   9,		      15,  15, 5},
		{ 28,		   21,		   18,		      25,  28, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_LARGE
	},

	{
		"Vampire", "Vampr", 8,
		{ 150,		   150,		   150,		      150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150},
		{ "fourth attack", "fast healing", "enhanced damage", "bite" },
		{ 17,		   17,		   9,		      15,  15, 5},
		{ 28,		   21,		   18,		      25,  28, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_LARGE
	},
	{
		"Sprite", "Sprit", 6,
		{ 150,		   150,		   150,		      150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150},
		{ "sneak",	   "hide",	   "fast healing",    "dust" },
		{ 12,		   16,		   14,		      15,  11, 9},
		{ 21,		   34,		   33,		      28,  20, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_TINY
	},

	{
		"Centaur", "Centr", 5,
		{ 150,		   150,		   150,		      150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150},
		{ "kick",	   "bash",	   "second attack",   "anti-magic aura" },
		{ 18,		   17,		   16,		      16,  17, 9},
		{ 30,		   23,		   22,		      28,  28, 18},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		SIZE_LARGE
	},

	{
		"", "", -1,
		{ -1,		   -1,		   -1,		      -1,  -1  },
		{ "" },
		{ -1,		   -1,		   -1,		      -1,  -1, -1},
		{ -1,		   -1,		   -1,		      -1,  -1, -1},
		{ -1,		   -1,		   -1,		      -1,  -1, 18},
		-1
	}
};


/*
 * Class table.
 */
const struct class_type class_table[MAX_CLASS] =
{
	{
		"Mage", "Mag", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
		{ 3018, 9618 },
		75, 20, 6,
		6, 20, 10, 32,
		TRUE,
		"Mage basics", "maMagege default",
		0.8f, 18.0f, 0.9f, 17.0f,
		TRUE, "Caster, Fast casting, destructive and defensive", 0
	},
	{
		"Cleric", "Cle", STAT_WIS, OBJ_VNUM_SCHOOL_MACE,
		{ 3003, 9619 },
		75, 20, 2,
		7, 22, 10, 30,
		TRUE,
		"Cleric basics", "cleric default",
		0.9f, 12.0f, 0.9f, 15.0f,
		TRUE, "Holy god worshipers, beneficial, healing", 0
	},
	{
		"Thief", "Thi", STAT_DEX, OBJ_VNUM_SCHOOL_DAGGER,
		{ 3028, 9639 },
		75, 20, -4,
		9, 30, 5, 15,
		FALSE,
		"thief basics", "thief default",
		0.8f, 15.0f, 0.9f, 12.0f,
		TRUE, "Melee-based attacks, deceptive and quick", 0
	},
	{
		"Warrior", "War", STAT_STR, OBJ_VNUM_SCHOOL_SWORD,
		{ 3022, 9633 },
		75, 20, -10,
		11, 32, 5, 13,
		FALSE,
		"warrior basics", "warrior default",
		1.0f, 10.0f, 1.0f, 12.0f,
		TRUE, "Aggressive and tough, melee and defensive", 0
	},
	{
		"Witch", "Wit", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
		{ 3362, 1    },
		75, 20, 6,
		8, 24, 10, 20,
		TRUE,
		"witch basics", "witch default",
		0.9f, 15.0f, 0.9f, 16.0f,
		TRUE, "Caster, poisons and unbeneficial spells", 0
	},
	{
		"WarMage", "Wmg", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
		{ 400,	400  },
		75, 20, 6,
		6, 20, 10, 32,
		TRUE,
		"WarMage basics", "WarMage default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Caster, fast-casting, all destructive", 750
	},
	{
		"Priest", "Pri", STAT_WIS, OBJ_VNUM_SCHOOL_MACE,
		{ 400,	400  },
		75, 20, 2,
		7, 22, 10, 30,
		TRUE,
		"Priest basics", "Priest default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Holy crusader, major beneficial, minor healing/destructive", 750
	},
	{
		"Merchant", "Mer", STAT_DEX, OBJ_VNUM_SCHOOL_DAGGER,
		{ 400,	400  },
		75, 20, -4,
		9, 30, 5, 15,
		FALSE,
		"Merchant basics", "Merchant default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Spells/skills for item creation and design", 750
	},
	{
		"Paladin", "Pal", STAT_STR, OBJ_VNUM_SCHOOL_SWORD,
		{ 400,	400  },
		75, 20, -10,
		11, 32, 5, 13,
		FALSE,
		"Paladin basics", "Paladin default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Melee-based attacks, w/ some healing & beneficial", 750
	},
	{
		"Seer", "See", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
		{ 400,	400  },
		75, 20, 6,
		8, 24, 10, 20,
		TRUE,
		"Seer basics", "Seer default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Beneficial and creation spells, minor melee", 750
	},
	{
		"Necro", "Nec", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
		{ 400,	400  },
		75, 20, 6,
		6, 20, 10, 32,
		TRUE,
		"Necro basics", "Necro default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "All destructive and stat-based attacks, summoning", -750
	},
	{
		"Demonic", "Dmn", STAT_WIS, OBJ_VNUM_SCHOOL_MACE,
		{ 400,	400  },
		75, 20, 2,
		7, 22, 10, 30,
		TRUE,
		"Demonic basics", "Demonic default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Aggressive and destructive, no beneficial", -750
	},
	{
		"Assassin", "Asn", STAT_DEX, OBJ_VNUM_SCHOOL_DAGGER,
		{ 400,	400  },
		75, 20, -4,
		9, 30, 5, 15,
		FALSE,
		"Assassin basics", "Assassin default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Melee attacks with a mix of spells", -750
	},
	{
		"Mercenery", "Mrc", STAT_STR, OBJ_VNUM_SCHOOL_SWORD,
		{ 400,	400  },
		75, 20, -10,
		11, 32, 5, 13,
		FALSE,
		"Mercenary basics", "Mercenary default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "Power melee, fewer attacks but stronger skills", -750
	},
	{
		"Shaman", "Sha", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
		{ 400,	400  },
		75, 20, 6,
		8, 24, 10, 20,
		TRUE,
		"Shaman basics", "Shaman default",
		0.9f, 15.0f, 0.9f, 16.0f,
		FALSE, "All attack spells, minor unbeneficial spells", -750
	}
};



/*
 * Liquid properties.
 * Used in world.obj.
 */
const struct liq_type liq_table[] =
{
/*
 *      {
 *              name, color
 *              proof, full, thirst, food, size
 *      }
 */
	{
		"water", "clear",
		{ 0,   1,  10, 0,  16 }
	},

	{
		"`@M`2ountain `@D`2ew``", "greenish",
		{ 0,   5,  10, 2,  44 }
	},

	{
		"Mountain Dew", "greenish",
		{ 0,   5,  10, 2,  44 }
	},


	{
		"beer", "amber",
		{ 12,  1,  8,  1,  12 }
	},

	{
		"liquor", "colorful",
		{ 12,  1,  8,  1,  12 }
	},

	{
		"red wine", "burgundy",
		{ 30,  1,  8,  1,  5  }
	},

	{
		"ale", "brown",
		{ 15,  1,  8,  1,  12 }
	},

	{
		"dark ale", "dark",
		{ 16,  1,  8,  1,  12 }
	},

	{
		"whisky", "golden",
		{ 120, 1,  5,  0,  2  }
	},

	{
		"lemonade", "pink",
		{ 0,   1,  9,  2,  12 }
	},

	{
		"firebreather", "boiling",
		{ 190, 0,  4,  0,  2  }
	},

	{
		"local specialty", "clear",
		{ 151, 1,  3,  0,  2  }
	},

	{
		"slime mold juice", "green",
		{ 0,   2,  -8, 1,  2  }
	},

	{
		"slush", "colorful",
		{ 0,   1,  10, 0,  16 }
	},

	{
		"milk", "white",
		{ 0,   2,  9,  3,  12 }
	},

	{
		"tea", "tan",
		{ 0,   1,  8,  0,  6  }
	},

	{
		"coffee", "black",
		{ 0,   1,  8,  0,  6  }
	},

	{
		"blood", "red",
		{ 0,   2,  -1, 2,  6  }
	},

	{
		"salt water", "clear",
		{ 0,   1,  -2, 0,  1  }
	},

	{
		"coke", "brown",
		{ 0,   2,  9,  2,  12 }
	},

	{
		"root beer", "brown",
		{ 0,   2,  9,  2,  12 }
	},

	{
		"elvish wine", "green",
		{ 35,  2,  8,  1,  5  }
	},

	{
		"white wine", "golden",
		{ 28,  1,  8,  1,  5  }
	},

	{
		"champagne", "golden",
		{ 32,  1,  8,  1,  5  }
	},

	{
		"mead", "honey-colored",
		{ 34,  2,  8,  2,  12 }
	},

	{
		"rose wine", "pink",
		{ 26,  1,  8,  1,  5  }
	},

	{
		"benedictine wine", "burgundy",
		{ 40,  1,  8,  1,  5  }
	},

	{
		"vodka", "clear",
		{ 130, 1,  5,  0,  2  }
	},

	{
		"cranberry juice", "red",
		{ 0,   1,  9,  2,  12 }
	},

	{
		"orange juice", "orange",
		{ 0,   2,  9,  3,  12 }
	},

	{
		"absinthe", "green",
		{ 200, 1,  4,  0,  2  }
	},

	{
		"brandy", "golden",
		{ 80,  1,  5,  0,  4  }
	},

	{
		"aquavit", "clear",
		{ 140, 1,  5,  0,  2  }
	},

	{
		"schnapps", "clear",
		{ 90,  1,  5,  0,  2  }
	},

	{
		"icewine", "purple",
		{ 50,  2,  6,  1,  5  }
	},

	{
		"wine", "red",
		{ 50,  2,  6,  1,  5  }
	},

	{
		"amontillado", "burgundy",
		{ 35,  2,  8,  1,  5  }
	},

	{
		"sherry", "red",
		{ 38,  2,  7,  1,  5  }
	},

	{
		"framboise", "red",
		{ 50,  1,  7,  1,  5  }
	},

	{
		"rum", "amber",
		{ 151, 1,  4,  0,  2  }
	},

	{
		"`6w`^a`6t`^e`6r `3t`#a`3c`#o``", "brown",
		{ 1,   40, 40, 40, 2  }
	},

	{
		"cordial", "clear",
		{ 100, 1,  5,  0,  2  }
	},

	{
		NULL, NULL,
		{ 0,   0,  0,  0,  0  }
	}
};
