#include "merc.h"
#include "magic.h"
#include "interp.h"

extern SKILL *gsp_sword;
extern SKILL *gsp_mace;
extern SKILL *gsp_dagger;
extern SKILL *gsp_axe;
extern SKILL *gsp_spear;
extern SKILL *gsp_flail;
extern SKILL *gsp_whip;
extern SKILL *gsp_polearm;

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
	{ "unique", false, 0, 0, 0, 0, 0, 0, 0, 0 },

	{
		"human", true,
		0, 0, 0,
		0, 0, 0,
		A | H | M | V, A | B | C | D | E | F | G | H | I | J | K
	},

	{ NULL, false, 0, 0, 0, 0, 0, 0, 0, 0 }
};

const struct pc_race_type pc_race_table[] =
{
	{
		"", "", -1,
		{ -1,		   -1,		   -1,		      -1 },
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
		{ 100,		   100,		   100,		      100 },
		{ "" },
		{ 13,		   13,		   13,		      13,  13, 9},
		{ 18,		   22,		   22,		      18,  21, 25},
		{ -1,		   -1,		   -1,		      -1,  -1, 25},
		SIZE_MEDIUM
	},

	{
		"", "", -1,
		{ -1,		   -1,		   -1,		      -1 },
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
		true,
		"Mage basics", "maMagege default",
		0.8f, 18.0f, 0.9f, 17.0f,
		true, "Caster, Fast casting, destructive and defensive", 0
	},
	{
		"Cleric", "Cle", STAT_WIS, OBJ_VNUM_SCHOOL_MACE,
		{ 3003, 9619 },
		75, 20, 2,
		7, 22, 10, 30,
		true,
		"Cleric basics", "cleric default",
		0.9f, 12.0f, 0.9f, 15.0f,
		true, "Holy god worshipers, beneficial, healing", 0
	},
	{
		"Thief", "Thi", STAT_DEX, OBJ_VNUM_SCHOOL_DAGGER,
		{ 3028, 9639 },
		75, 20, -4,
		9, 30, 5, 15,
		false,
		"thief basics", "thief default",
		0.8f, 15.0f, 0.9f, 12.0f,
		true, "Melee-based attacks, deceptive and quick", 0
	},
	{
		"Warrior", "War", STAT_STR, OBJ_VNUM_SCHOOL_SWORD,
		{ 3022, 9633 },
		75, 20, -10,
		11, 32, 5, 13,
		false,
		"warrior basics", "warrior default",
		1.0f, 10.0f, 1.0f, 12.0f,
		true, "Aggressive and tough, melee and defensive", 0
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
