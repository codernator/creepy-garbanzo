#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"



char last_command[MSL];

DISABLED_DATA *disabled_first;

char *repeater(char *s, int i);


/* Command logging types.*/

#define LOG_NORMAL      0
#define LOG_ALWAYS      1
#define LOG_NEVER       2


/* Log-all switch. */

bool log_all = false;


/* Command table. */

const struct cmd_type cmd_table[] =
{
	/* Common movement commands */
	{ "north",	      do_north,	       POS_STANDING, 0,	 LOG_NEVER,  0 },
	{ "east",	      do_east,	       POS_STANDING, 0,	 LOG_NEVER,  0 },
	{ "south",	      do_south,	       POS_STANDING, 0,	 LOG_NEVER,  0 },
	{ "west",	      do_west,	       POS_STANDING, 0,	 LOG_NEVER,  0 },
	{ "up",		      do_up,	       POS_STANDING, 0,	 LOG_NEVER,  0 },
	{ "down",	      do_down,	       POS_STANDING, 0,	 LOG_NEVER,  0 },

	/* Common other commands so one and two letter abbreviations work. */
	{ "cast",	      do_cast,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "auction",	  do_auction,      POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "buy",	      do_buy,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "exits",	      do_exits,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "get",	      do_get,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "goto",	      do_goto,	       POS_DEAD,     L8, LOG_NORMAL, 1 },
	{ "gobstopper",	  do_gobstopper,   POS_DEAD,     L6, LOG_ALWAYS, 1 },
	{ "grant",	      do_grant,	       POS_SLEEPING, L6, LOG_NORMAL, 1 },
	{ "group",	      do_group,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
/*	{ "hit",		do_kill,		POS_FIGHTING,	0,		LOG_NORMAL,		0       }, */
	{ "history",	  do_history,      POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "viewhist",	  do_viewhist,     POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "unignore",	  do_unignore,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "inventory",	  do_inventory,    POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "ignore",	      do_ignore,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "kill",	      do_kill,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "look",	      do_look,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "glance",	      do_glance,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "laston",	      do_laston,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "lore",	      do_lore,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "findflags",	  do_findflags,    POS_DEAD,     ML, LOG_NORMAL, 0 },
	{ "worldflag",	  do_worldflag,    POS_DEAD,     ML, LOG_ALWAYS, 0 },
	{ "spellflags",	  do_spellflags,   POS_DEAD,     ML, LOG_NORMAL, 0 },
	{ "order",	      do_order,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "practice",	  do_practice,     POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "rest",	      do_rest,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "restring",	  do_restring,     POS_SLEEPING, 0,	 LOG_ALWAYS, 1 },
	{ "repair",	      do_repair,       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "raceinfo",	  do_raceinfo,     POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "sit",	      do_sit,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "sockets",	  do_sockets,      POS_DEAD,     L2, LOG_NORMAL, 1 },
	{ "stand",	      do_stand,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "veil",	      do_veil,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "finger",	      do_finger,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "unlock",	      do_unlock,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "wield",	      do_wear,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "wizhelp",	  do_wizhelp,      POS_DEAD,     HE, LOG_NORMAL, 1 },
	{ "wizcommands",  do_wizcommands,  POS_DEAD,     HE, LOG_NORMAL, 1 },

	/*  Informational commands.  */
	{ "affects",	  do_affects,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "areas",	      do_areas,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "breathe",	  do_breathe,      POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "bug",	      do_bug,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "changes",	  do_changes,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "commands",	  do_commands,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "compare",	  do_compare,      POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "consider",	  do_consider,     POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "credits",	  do_credits,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "equipment",	  do_equipment,    POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "examine",	  do_examine,      POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "help",	      do_help,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },

	{ "information",  do_groups,       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "motd",	      do_motd,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "news",	      do_news,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "read",	      do_look,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "report",	      do_report,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "rules",	      do_rules,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "score",	      do_score,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "scan",	      do_scan,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "skills",	      do_skills,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "show",	      do_show,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "spells",	      do_spells,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "story",	      do_story,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "tick",	      do_tick,	       POS_DEAD,     L8, LOG_NORMAL, 1 },
	{ "time",	      do_time,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "typo",	      do_typo,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "weather",	  do_weather,      POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "ewho",	      do_ewho,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "who",	      do_who,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "whois",	      do_whois,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "wizlist",	  do_wizlist,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "worth",	      do_worth,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },

	/*  Configuration commands.  */

	{ "alia",	      do_alia,	       POS_DEAD,     0,	 LOG_NORMAL, 0 },
	{ "alias",	      do_alias,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autolist",	  do_autolist,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autoassist",	  do_autoassist,   POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autoeq",	      do_autoeq,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autoexit",	  do_autoexit,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autogold",	  do_autogold,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autoloot",	  do_autoloot,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autosac",	  do_autosac,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autosplit",	  do_autosplit,    POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "autoticks",	  do_autoticks,    POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "brief",	      do_brief,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "combine",	  do_combine,      POS_DEAD,     ML, LOG_NORMAL, 1 },
	{ "compact",	  do_compact,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "description",  do_description,  POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "deathcry",	  do_deathcry,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "dice",	      do_dice,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "delet",	      do_delet,	       POS_DEAD,     0,	 LOG_ALWAYS, 0 },
	{ "delete",	      do_delete,       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "disable",	  do_disable,      POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "nickname",	  do_nickname,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "nofollow",	  do_nofollow,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "noloot",	      do_noloot,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "nosummon",	  do_nosummon,     POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "password",	  do_password,     POS_DEAD,     0,	 LOG_NEVER,  1 },
	{ "prompt",	      do_prompt,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "scroll",	      do_scroll,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "title",	      do_title,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "unalias",	  do_unalias,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "wimpy",	      do_wimpy,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },

	/*  Communication commands.  */

	{ "afk",	      do_afk,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "go_pk",	      do_gopk,	       POS_SLEEPING, 0,	 LOG_ALWAYS, 1 },
	{ "channel",      command_channel, POS_DEAD,     0,  LOG_NORMAL, 0 },

	{ "note",	      do_note,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "rpnote",	      do_rpnote,       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "aucnote",	  do_aucnote,      POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "contest",	  do_contest,      POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "idea",	      do_idea,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "build",	      do_build,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },

	{ "at",		      do_at,	       POS_DEAD,     L7, LOG_NORMAL, 1 },
	{ "replay",	      do_replay,       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "catchup",	  do_catchup,      POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "unread",	      do_unread,       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },

	/*  Object manipulation commands. */

	{ "brandish",	  do_brandish,     POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "close",	      do_close,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "donate",	      do_donate,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "drink",	      do_drink,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "drop",	      do_drop,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "eat",	      do_eat,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "envenom",	  do_envenom,      POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "evaluate",	  do_objident,     POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "fixscreen",	  do_fixscreen,    POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "fill",	      do_fill,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "give",	      do_give,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "heal",	      do_heal,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "hold",	      do_wear,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "list",	      do_list,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "lock",	      do_lock,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "open",	      do_open,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "pick",	      do_pick,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "pour",	      do_pour,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "put",	      do_put,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "quaff",	      do_quaff,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "recite",	      do_recite,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "remove",	      do_remove,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "sell",	      do_sell,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "second",	      do_second,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "take",	      do_get,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "sacrifice",	  do_sacrifice,    POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "junk",	      do_sacrifice,    POS_RESTING,  0,	 LOG_NORMAL, 0 },
	{ "value",	      do_value,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "wear",	      do_wear,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "zap",	      do_zap,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },

	/* Bank Commands     */

	{ "withdraw",	  do_atm_withdraw, POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "deposit",	  do_atm_deposit,  POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "balance",	  do_atm_balance,  POS_STANDING, 0,	 LOG_NORMAL, 1 },

	/* Combat commands.*/

	{ "bash",	      do_bash,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "bite",	      do_bite,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "feed",	      do_feed,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "crush",	      do_crush,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "disarm",	      do_disarm,       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "deft",	      do_deft,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "dash",	      do_dash,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "flee",	      do_flee,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "hiss",	      do_hiss,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "intimidate",	  do_intimidate,   POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "kick",	      do_kick,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "kneecap",	  do_kneecap,      POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "rake",	      do_rake,	       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },
	{ "dream",	      do_dream,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "rescue",	      do_rescue,       POS_FIGHTING, 0,	 LOG_NORMAL, 0 },
	{ "throw",	      do_throw,	       POS_FIGHTING, 0,	 LOG_NORMAL, 0 },
	{ "shriek",	      do_shriek,       POS_FIGHTING, 0,	 LOG_NORMAL, 1 },

/*  Mob command interpreter(placed here for faster scan...)  */

	{ "mob",	      do_mob,	       POS_DEAD,     0,	 LOG_NEVER,  0 },
	{ "mpstat",	      do_mpstat,       POS_RESTING,  BL, LOG_NORMAL, 1 },
	{ "mpdump",	      do_mpdump,       POS_RESTING,  BL, LOG_NORMAL, 1 },

/*  Miscellaneous commands.  */

	{ "enter",	      do_enter,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "follow",	      do_follow,       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "gain",	      do_gain,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "go",		      do_enter,	       POS_STANDING, 0,	 LOG_NORMAL, 0 },
	{ "groups",	      do_groups,       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "hide",	      do_hide,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "cls",	      do_clearscreen,  POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "clearscreen",  do_clearscreen,  POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "die",	      do_die,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "qui",	      do_qui,	       POS_DEAD,     0,	 LOG_NORMAL, 0 },
	{ "quit",	      do_quit,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "radio",	      do_radio,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "regenerate",	  do_regenerate,   POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "save",	      do_save,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "sleep",	      do_sleep,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "sneak",	      do_sneak,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "split",	      do_split,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "steal",	      do_steal,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "train",	      do_train,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "tally",	      do_tally,	       POS_RESTING,  1,	 LOG_NORMAL, 1 },
	{ "untally",	  do_untally,      POS_RESTING,  L1, LOG_NORMAL, 1 },
	{ "visible",	  do_visible,      POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "wake",	      do_wake,	       POS_SLEEPING, 0,	 LOG_NORMAL, 1 },
	{ "where",	      do_where,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "here",	      do_here,	       POS_RESTING,  0,	 LOG_NORMAL, 1 },
	{ "push",	      do_push,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "drag",	      do_drag,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "color",	      do_color,	       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "scribe",	      do_scribe,       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "brew",	      do_brew,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "dust",	      do_dust,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "sprinkle",	  do_sprinkle,     POS_STANDING, 0,	 LOG_NORMAL, 1 },

	/*  Blacksmith commands  */

	{ "estimate",	  do_estimate,     POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "engulf",	      do_engulf,       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "electrify",	  do_electrify,    POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "poison",	      do_poison,       POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "energize",	  do_energize,     POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "chill",	      do_chill,	       POS_STANDING, 0,	 LOG_NORMAL, 1 },

	/*  Immortal commands  */

	{ "addalias",	  do_addalias,     POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "advance",	  do_advance,      POS_DEAD,     L1, LOG_ALWAYS, 1 },
	{ "dump",	      do_dump,	       POS_DEAD,     ML, LOG_ALWAYS, 0 },
	{ "trust",	      do_trust,	       POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "extend",	      do_extend,       POS_DEAD,     IM, LOG_ALWAYS, 1 },
	{ "violate",	  do_violate,      POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "enchant",	  do_enchant,      POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "disenchant",	  do_disenchant,   POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "cuo",	      do_cuo,	       POS_DEAD,     L4, LOG_NORMAL, 1 },
	{ "allow",	      do_allow,	       POS_DEAD,     L2, LOG_ALWAYS, 1 },
	{ "ban",	      do_ban,	       POS_DEAD,     L2, LOG_ALWAYS, 1 },
	{ "deny",	      do_deny,	       POS_DEAD,     L2, LOG_ALWAYS, 1 },
	{ "disconnect",	  do_disconnect,   POS_DEAD,     L2, LOG_ALWAYS, 1 },
	{ "flag",	      do_flag,	       POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "norestore",	  do_norestore,    POS_DEAD,     L8, LOG_ALWAYS, 1 },
	{ "permban",	  do_permban,      POS_DEAD,     L1, LOG_ALWAYS, 1 },
	{ "rdesc",	      do_rdesc,	       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "rload",	      do_rload,	       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "rsave",	      do_rsave,	       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "reboo",	      do_reboo,	       POS_DEAD,     L5, LOG_NORMAL, 0 },
	{ "reboot",	      do_reboot,       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "copyover",	  do_copyover,     POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "set",	      do_set,	       POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "shutdow",	  do_shutdow,      POS_DEAD,     ML, LOG_NORMAL, 0 },
	{ "shutdown",	  do_shutdown,     POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "sockets",      do_sockets,      POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "target",	      do_target,       POS_DEAD,     L2, LOG_ALWAYS, 1 },
	{ "for",	      do_for,	       POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "force",	      do_force,	       POS_DEAD,     L3, LOG_ALWAYS, 1 },
	{ "fry",	      do_fry,	       POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "ffry",	      do_ffry,	       POS_DEAD,     L1, LOG_ALWAYS, 1 },
	{ "load",	      do_load,	       POS_DEAD,     L4, LOG_ALWAYS, 1 },
	{ "newlock",	  do_newlock,      POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "chown",	      do_chown,	       POS_DEAD,     L6, LOG_ALWAYS, 1 },
	{ "mode",	      do_mode,	       POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "pecho",	      do_pecho,	       POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "pardon",	      do_pardon,       POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "ploa",	      do_ploa,	       POS_DEAD,     ML, LOG_NORMAL, 0 },
	{ "pload",	      do_pload,	       POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "punloa",	      do_punloa,       POS_DEAD,     ML, LOG_NORMAL, 0 },
	{ "punload",	  do_punload,      POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "purge",	      do_purge,	       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "repop",	      do_repop,	       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "sla",	      do_sla,	       POS_DEAD,     L5, LOG_NORMAL, 0 },
	{ "slay",	      do_slay,	       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "teleport",	  do_teleport,     POS_STANDING, 0,	 LOG_NORMAL, 1 },
	{ "transfer",	  do_transfer,     POS_DEAD,     IM, LOG_ALWAYS, 1 },

	{ "wizlock",	      do_wizlock,      POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "poofin",	      do_bamfin,       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "poofout",	  do_bamfout,      POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "srestore",	  do_setrestore,   POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "gecho",	      do_echo,	       POS_DEAD,     L8, LOG_ALWAYS, 1 },
	{ "holylight",	  do_holylight,    POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "incognito",	  do_incognito,    POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "invisible",	  do_invisible,    POS_DEAD,     0,	 LOG_NORMAL, 0 },
	{ "log",	      do_log,	       POS_DEAD,     ML, LOG_ALWAYS, 1 },
	{ "memory",	      do_memory,       POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "mwhere",	      do_mwhere,       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "owhere",	      do_owhere,       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "outfit",	      do_outfit,       POS_DEAD,     IM, LOG_ALWAYS, 1 },
	{ "peace",	      do_peace,	       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "wpeace",	      do_wpeace,       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "penalty",	  do_penalty,      POS_DEAD,     0,	 LOG_NORMAL, 1 },
	{ "pig",	      do_pig,	       POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "pnlist",	      do_pnlist,       POS_DEAD,     L7, LOG_NORMAL, 1 },
	{ "echo",	      do_recho,	       POS_DEAD,     HE, LOG_ALWAYS, 1 },
	{ "rename",	      do_rename,       POS_DEAD,     L2, LOG_NORMAL, 1 },
	{ "return",	      do_return,       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "snoop",	      do_snoop,	       POS_DEAD,     L7, LOG_ALWAYS, 1 },
	{ "snlist",	      do_snlist,       POS_DEAD,     L7, LOG_NORMAL, 1 },
	{ "slot",	      do_slot,	       POS_DEAD,     L2, LOG_NORMAL, 1 },
	{ "stat",	      do_stat,	       POS_DEAD,     L8, LOG_NORMAL, 1 },
	{ "omnistat",	  do_omnistat,     POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "string",	      do_string,       POS_DEAD,     IM, LOG_ALWAYS, 1 },
	{ "switch",	      do_switch,       POS_DEAD,     L5, LOG_ALWAYS, 1 },
	{ "wizinvis",	  do_winvis,       POS_DEAD,     IM, LOG_NORMAL, 1 },
/*  { "vnum",         do_vnum,         POS_DEAD,     L4, LOG_NORMAL, 1 }, */
	{ "ovnum",	      do_ovnum,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "mvnum",	      do_mvnum,	       POS_DEAD,     L5, LOG_NORMAL, 1 },

	{ "zecho",	      do_zecho,	       POS_DEAD,     IM, LOG_ALWAYS, 1 },
	{ "affstrip",	  do_affstrip,     POS_DEAD,     L8, LOG_ALWAYS, 1 },
	{ "clone",	      do_clone,	       POS_DEAD,     L6, LOG_ALWAYS, 1 },

	{ "wiznet",	      do_wiznet,       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "impnet",	      do_impnet,       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "imotd",	      do_imotd,	       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "prefi",	      do_prefi,	       POS_DEAD,     IM, LOG_NORMAL, 0 },
	{ "prefix",	      do_prefix,       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "busy",	      do_busy,	       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "coding",	      do_coding,       POS_DEAD,     L1, LOG_NORMAL, 1 },
	{ "building",	  do_building,     POS_DEAD,     L5, LOG_NORMAL, 1 },
/*	{ "host",		do_host,		POS_DEAD,	ML,		LOG_ALWAYS,		1	}, */

/*  OLC  */

	{ "edit",	      do_olc,	       POS_DEAD,     L5, LOG_NORMAL, 0 },
	{ "asave",	      do_asave,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "alist",	      do_alist,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "resets",	      do_resets,       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "redit",	      do_redit,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "medit",	      do_medit,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "aedit",	      do_aedit,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "oedit",	      do_oedit,	       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "mpedit",	      do_mpedit,       POS_DEAD,     L5, LOG_NORMAL, 1 },
	{ "hedit",	      do_hedit,	       POS_DEAD,     IM, LOG_NORMAL, 1 },
	{ "skedit",	      do_skedit,       POS_DEAD,     L5, LOG_NORMAL, 0 },
	{ "gredit",	      do_gredit,       POS_DEAD,     L5, LOG_NORMAL, 0 },

/* End of list. */
	{ "",		      NULL,	       POS_DEAD,     0,	 LOG_NORMAL, 0 }
};



/***************************************************************************
*	interpret
*
*	command interpreter
***************************************************************************/
void interpret(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char command[MIL];
	char logline[MIL];
    static char wiznet_message[MIL];
	int cmd;
	int trust;
	bool found;

	while (is_space(*argument))
		argument++;

	if (argument[0] == '\0')
		return;

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
	strcpy(logline, argument);
	strcpy(buf, argument);
	sprintf(last_command, "The last command was by %s in room[%ld] : %s.", ch->name, ch->in_room->vnum, buf);

	if (!is_alpha(argument[0]) && !is_digit(argument[0])) {
		command[0] = argument[0];
		command[1] = '\0';
		argument++;
		while (is_space(*argument))
			argument++;
	} else {
		argument = one_argument(argument, command);
	}

/*  Look for command in command table  */

	found = false;
	trust = get_trust(ch);
	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++) {
		if (command[0] == cmd_table[cmd].name[0]
		    && !str_prefix(command, cmd_table[cmd].name)
		    && cmd_table[cmd].level <= trust) {
			found = true;
			break;
		}
	}

	if (IS_SET(ch->affected_by, AFF_HIDE)) {
		if (ch->class == class_lookup("thief")) {
			if (cmd_table[cmd].position <= POS_RESTING || cmd < 6) {
				if (number_percent() > (get_learned_percent(ch, gsp_hide) - 10))
					REMOVE_BIT(ch->affected_by, AFF_HIDE);
			} else {
				REMOVE_BIT(ch->affected_by, AFF_HIDE);
			}
		} else {
			REMOVE_BIT(ch->affected_by, AFF_HIDE);
		}
	}

	/* Log and snoop. */
	log_to(LOG_SINK_LASTCMD, ch->name, "[%s] -- '%s'", ch->name, logline);

	if (cmd_table[cmd].log == LOG_NEVER)
		strcpy(logline, "");

	if (cmd_table[cmd].log == LOG_ALWAYS) {
		if (!IS_NPC(ch)) {
			log_to(LOG_SINK_ALWAYS, ch->name, "[%s] -- '%s'", ch->name, logline);
			sprintf(wiznet_message, "`4Log `O%s`4: `O%s``", ch->name, logline);
			wiznet(wiznet_message, ch, NULL, WIZ_SECURE, 0, get_trust(ch));
		} else {
			log_to(LOG_SINK_ALWAYS, ch->name, "[%s] -- '%s'", ch->name, logline);
			log_string("Log %s(mob): %s", ch->name, logline);
			sprintf(wiznet_message, "`4Log `O%s(mob)`4: `O%s``", ch->name, logline);
			wiznet(wiznet_message, ch, NULL, WIZ_SECURE, 0, get_trust(ch));
		}
	}

	if (!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG)) {
		log_to(LOG_SINK_PLAYER, ch->name, "[%s] -- '%s'", ch->name, logline);
		sprintf(wiznet_message, "`4Log `O%s(plr)`4: `O%s``", ch->name, logline);
		wiznet(wiznet_message, ch, NULL, WIZ_PLOG, 0, get_trust(ch));
	}

	if (log_all) {
		log_to(LOG_SINK_ALL, "NULL", "[%s] -- '%s'", ch->name, logline);
		sprintf(wiznet_message, "`4Log `O%s(all)`4: `O%s``", ch->name, logline);
		wiznet(wiznet_message, ch, NULL, WIZ_ALOG, 0, get_trust(ch));
	}

	if (ch->desc != NULL && ch->desc->snoop_by != NULL) {
		write_to_buffer(ch->desc->snoop_by, "% ", 2);
		write_to_buffer(ch->desc->snoop_by, logline, 0);
		write_to_buffer(ch->desc->snoop_by, "\n\r", 2);
	}

	if (!found) {
        send_to_char("```COMMAND NOT FOUND``\n\r", ch);
		return;
	}

	if (check_disabled(ch, DISABLED_CMD, cmd_table[cmd].name)) {
		send_to_char("This command has been temporarily disabled.\n\r", ch);
		return;
	}

	/* Character not in position for command? */

	if (ch->position < cmd_table[cmd].position) {
		switch (ch->position) {
		case POS_DEAD:
			send_to_char("Lie still; you are DEAD.\n\r", ch);
			break;

		case POS_MORTAL:

		case POS_INCAP:
			send_to_char("You are hurt far too bad for that.\n\r", ch);
			break;

		case POS_STUNNED:
			send_to_char("You are too stunned to do that.\n\r", ch);
			break;

		case POS_SLEEPING:
			send_to_char("In your dreams, or what?\n\r", ch);
			break;

		case POS_RESTING:
			send_to_char("Nah... You feel too relaxed...\n\r", ch);
			break;

		case POS_SITTING:
			send_to_char("Better stand up first.\n\r", ch);
			break;

		case POS_FIGHTING:
			send_to_char("No way!  You are still fighting!\n\r", ch);
			break;
		}
		return;
	}

	/* Dispatch the command.*/

	(*cmd_table[cmd].do_fun)(ch, argument);

	tail_chain();
}

/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument(char *argument, char *arg)
{
	char *pdot;
	int number;

	for (pdot = argument; *pdot != '\0'; pdot++) {
		if (*pdot == '.') {
			*pdot = '\0';
			number = parse_int(argument);
			*pdot = '.';
			strcpy(arg, pdot + 1);
			return number;
		}
	}

	strcpy(arg, argument);

	return 1;
}

/*
 * Given a string like 14*foo, return 14 and 'foo'
 */
int mult_argument(char *argument, char *arg)
{
	char *pdot;
	int number;

	for (pdot = argument; *pdot != '\0'; pdot++) {
		if (*pdot == '*') {
			*pdot = '\0';
			number = parse_int(argument);
			*pdot = '*';
			strcpy(arg, pdot + 1);
			return number;
		}
	}

	strcpy(arg, argument);
	return 1;
}


/***************************************************************************
*	one_line
*
*	get one line from a string - fill the buffer with the line
*	and return the new position
***************************************************************************/
char *one_line(char *base, char *buf)
{
	char *tmp;
	int idx;

	tmp = base;
	while (*tmp == ' ')
		tmp++;

	while (*tmp != '\0') {
		if (*tmp == '\n'
		    || *tmp == '\r')
			break;

		*buf++ = *tmp++;
	}

	*buf = '\0';

	for (idx = 0; idx < 2; idx++)
		if (*tmp == '\n' || *tmp == '\r')
			tmp++;

	return tmp;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char *one_argument(char *argument, char *arg_first)
{
	char cEnd;

	while (is_space(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"') {
		cEnd = *argument++;
    }

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}
		*arg_first = LOWER(*argument);
		arg_first++;
		argument++;
	}
	*arg_first = '\0';

	while (is_space(*argument))
		argument++;

	return argument;
}

/*
 * Contributed by Alander.
 */
void do_commands(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	int cmd;
	int col;

	col = 0;
	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++) {
		if (cmd_table[cmd].level < LEVEL_HERO
		    && cmd_table[cmd].level <= get_trust(ch)
		    && cmd_table[cmd].show) {
			sprintf(buf, "%-12s", cmd_table[cmd].name);
			send_to_char(buf, ch);
			if (++col % 6 == 0)
				send_to_char("\n\r", ch);
		}
	}

	if (col % 6 != 0)
		send_to_char("\n\r", ch);

	return;
}

/*
 * Contributed by Monrick.
 */
void do_wizcommands(CHAR_DATA *ch, char *argument)
{
	BUFFER *buf;
	int cmd;
	int level;
	int col;

	buf = new_buf();

	for (level = LEVEL_HERO; level <= (int)UMIN(MAX_LEVEL, get_trust(ch)); level++) {
		printf_buf(buf, repeater("-", 80));
		printf_buf(buf, "\n\r");
		printf_buf(buf, "`O%41d``:\n\r", level);
		printf_buf(buf, repeater("-", 80));
		printf_buf(buf, "\n\r");
		col = 0;
		for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++) {
			if (cmd_table[cmd].level == level
			    && cmd_table[cmd].show) {
				printf_buf(buf, "%-20s", capitalize(cmd_table[cmd].name));
				if (++col % 4 == 0)
					printf_buf(buf, "\n\r");
			}
		}
		if (col % 4 != 0 || col == 0)
			printf_buf(buf, "\n\r");
		printf_buf(buf, "\n\r");
	}

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
}



void do_wizhelp(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	int cmd;
	int col;

	col = 0;
	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++) {
		if (cmd_table[cmd].level >= LEVEL_HERO
		    && cmd_table[cmd].level <= get_trust(ch)
		    && cmd_table[cmd].show) {
			sprintf(buf, "`&%-3d`7: `O%-15s`7",
				cmd_table[cmd].level,
				capitalize(cmd_table[cmd].name));
			send_to_char(buf, ch);
			if (++col % 4 == 0)
				send_to_char("\n\r", ch);
		}
	}

	if (col % 6 != 0)
		send_to_char("\n\r", ch);

	return;
}


const CMD *cmd_lookup(CHAR_DATA *ch, char *argument)
{
	char cmd_buf[MSL];
	int trust;
	int iter;

	strcpy(cmd_buf, argument);
	if (!is_alpha(argument[0]) && !is_digit(argument[0])) {
		cmd_buf[0] = argument[0];
		cmd_buf[1] = '\0';
	} else {
		one_argument(argument, cmd_buf);
	}

	/*
	 * Look for command in command table.
	 */
	trust = get_trust(ch);
	for (iter = 0; cmd_table[iter].name[0] != '\0'; iter++) {
		if (cmd_buf[0] == cmd_table[iter].name[0]
		    && !str_prefix(cmd_buf, cmd_table[iter].name)
		    && cmd_table[iter].level <= trust)
			return &cmd_table[iter];
	}

	return NULL;
}
