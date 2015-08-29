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
*	command level constants
***************************************************************************/
#define ML      (int)MAX_LEVEL          /* implementor */
#define L1      (int)(MAX_LEVEL - 1)    /* creator */
#define L2      (int)(MAX_LEVEL - 2)    /* supreme being */
#define L3      (int)(MAX_LEVEL - 3)    /* deity */
#define L4      (int)(MAX_LEVEL - 4)    /* god */
#define L5      (int)(MAX_LEVEL - 5)    /* immortal */
#define L6      (int)(MAX_LEVEL - 6)    /* demigod */
#define L7      (int)(MAX_LEVEL - 7)    /* angel */
#define L8      (int)(MAX_LEVEL - 8)    /* avatar */
#define IM      (int)LEVEL_IMMORTAL     /* angel */
#define HE      (int)LEVEL_HERO         /* hero */

/***************************************************************************
*	OLC Levels
***************************************************************************/
#define HB      (int)MAX_LEVEL          /* head builder */
#define BL      (int)(MAX_LEVEL - 2)    /* standard builder */
#define MP      (int)(MAX_LEVEL - 2)    /* mob-prog coder */
#define HP      (int)LEVEL_IMMORTAL     /* help editor */
#define SK      (int)MAX_LEVEL          /* skill editor */
#define GR      (int)MAX_LEVEL          /* groups editor */


/* Random definition */
#define COM_INGORE      1


/***************************************************************************
*	declarations
***************************************************************************/
typedef struct cmd_type {
	char *const	name;
	DO_FUN *	do_fun;
	int		position;
	int		level;
	int		log;
	int		show;
}CMD;

const CMD *cmd_lookup(CHAR_DATA * ch, char *argument);

extern const struct cmd_type cmd_table[];



/***************************************************************************
*	command functions
***************************************************************************/
DECLARE_DO_FUN(command_channel);

DECLARE_DO_FUN(do_addalias);
DECLARE_DO_FUN(do_advance);
DECLARE_DO_FUN(do_affects);
DECLARE_DO_FUN(do_affstrip);
DECLARE_DO_FUN(do_afk);
DECLARE_DO_FUN(do_alia);
DECLARE_DO_FUN(do_alias);
DECLARE_DO_FUN(do_allow);
DECLARE_DO_FUN(do_areas);
DECLARE_DO_FUN(do_at);
DECLARE_DO_FUN(do_atm_balance);
DECLARE_DO_FUN(do_atm_deposit);
DECLARE_DO_FUN(do_atm_withdraw);
DECLARE_DO_FUN(do_auction);
DECLARE_DO_FUN(do_autoassist);
DECLARE_DO_FUN(do_autoeq);
DECLARE_DO_FUN(do_autoexit);
DECLARE_DO_FUN(do_autogold);
DECLARE_DO_FUN(do_autolist);
DECLARE_DO_FUN(do_autoloot);
DECLARE_DO_FUN(do_autosac);
DECLARE_DO_FUN(do_autosplit);
DECLARE_DO_FUN(do_autoticks);
DECLARE_DO_FUN(do_balance);
DECLARE_DO_FUN(do_bamfin);
DECLARE_DO_FUN(do_bamfout);
DECLARE_DO_FUN(do_ban);
DECLARE_DO_FUN(do_bash);
DECLARE_DO_FUN(do_battlefield);
DECLARE_DO_FUN(do_brandish);
DECLARE_DO_FUN(do_brew);
DECLARE_DO_FUN(do_brief);
DECLARE_DO_FUN(do_bug);
DECLARE_DO_FUN(do_building);
DECLARE_DO_FUN(do_busy);
DECLARE_DO_FUN(do_buy);
DECLARE_DO_FUN(do_cast);
DECLARE_DO_FUN(do_catchup);
DECLARE_DO_FUN(do_channels);
DECLARE_DO_FUN(do_chown);
DECLARE_DO_FUN(do_clearscreen);
DECLARE_DO_FUN(do_clone);
DECLARE_DO_FUN(do_close);
DECLARE_DO_FUN(do_coding);
DECLARE_DO_FUN(do_color);
DECLARE_DO_FUN(do_combine);
DECLARE_DO_FUN(do_commands);
DECLARE_DO_FUN(do_compact);
DECLARE_DO_FUN(do_compare);
DECLARE_DO_FUN(do_consider);
DECLARE_DO_FUN(do_copyover);
DECLARE_DO_FUN(do_credits);
DECLARE_DO_FUN(do_cuo);
DECLARE_DO_FUN(do_dash);
DECLARE_DO_FUN(do_deathcry);
DECLARE_DO_FUN(do_deft);
DECLARE_DO_FUN(do_delet);
DECLARE_DO_FUN(do_delete);
DECLARE_DO_FUN(do_deny);
DECLARE_DO_FUN(do_deposit);
DECLARE_DO_FUN(do_description);
DECLARE_DO_FUN(do_dice);
DECLARE_DO_FUN(do_disarm);
DECLARE_DO_FUN(do_disconnect);
DECLARE_DO_FUN(do_disenchant);
DECLARE_DO_FUN(do_donate);
DECLARE_DO_FUN(do_down);
DECLARE_DO_FUN(do_drag);
DECLARE_DO_FUN(do_drink);
DECLARE_DO_FUN(do_drop);
DECLARE_DO_FUN(do_dump);
DECLARE_DO_FUN(do_east);
DECLARE_DO_FUN(do_eat);
DECLARE_DO_FUN(do_echo);
DECLARE_DO_FUN(do_enchant);
DECLARE_DO_FUN(do_enter);
DECLARE_DO_FUN(do_envenom);
DECLARE_DO_FUN(do_equipment);
DECLARE_DO_FUN(do_ewho);
DECLARE_DO_FUN(do_examine);
DECLARE_DO_FUN(do_exits);
DECLARE_DO_FUN(do_extend);
DECLARE_DO_FUN(do_familiar);
DECLARE_DO_FUN(do_ffry);
DECLARE_DO_FUN(do_fill);
DECLARE_DO_FUN(do_filter);
DECLARE_DO_FUN(do_findflags);
DECLARE_DO_FUN(do_finger);
DECLARE_DO_FUN(do_fixscreen);
DECLARE_DO_FUN(do_flag);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_follow);
DECLARE_DO_FUN(do_for);
DECLARE_DO_FUN(do_force);
DECLARE_DO_FUN(do_fry);
DECLARE_DO_FUN(do_gain);
DECLARE_DO_FUN(do_get);
DECLARE_DO_FUN(do_give);
DECLARE_DO_FUN(do_glance);
DECLARE_DO_FUN(do_gobstopper);
DECLARE_DO_FUN(do_goto);
DECLARE_DO_FUN(do_grant);
DECLARE_DO_FUN(do_group);
DECLARE_DO_FUN(do_groups);
DECLARE_DO_FUN(do_heal);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_here);
DECLARE_DO_FUN(do_hide);
DECLARE_DO_FUN(do_history);
DECLARE_DO_FUN(do_holylight);
DECLARE_DO_FUN(do_ignore);
DECLARE_DO_FUN(do_imotd);
DECLARE_DO_FUN(do_impnet);
DECLARE_DO_FUN(do_incognito);
DECLARE_DO_FUN(do_inlay);
DECLARE_DO_FUN(do_intimidate);
DECLARE_DO_FUN(do_inventory);
DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_laston);
DECLARE_DO_FUN(do_list);
DECLARE_DO_FUN(do_load);
DECLARE_DO_FUN(do_lock);
DECLARE_DO_FUN(do_log);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_lore);
DECLARE_DO_FUN(do_makecard);
DECLARE_DO_FUN(do_memory);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_mlevel);
DECLARE_DO_FUN(do_mload);
DECLARE_DO_FUN(do_mode);
DECLARE_DO_FUN(do_motd);
DECLARE_DO_FUN(do_mrelic);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_mwhere);
DECLARE_DO_FUN(do_newlock);
DECLARE_DO_FUN(do_nickname);
DECLARE_DO_FUN(do_nofollow);
DECLARE_DO_FUN(do_noloot);
DECLARE_DO_FUN(do_noquit);
DECLARE_DO_FUN(do_north);
DECLARE_DO_FUN(do_nosummon);
DECLARE_DO_FUN(do_objident);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_olevel);
DECLARE_DO_FUN(do_oload);
DECLARE_DO_FUN(do_omnistat);
DECLARE_DO_FUN(do_open);
DECLARE_DO_FUN(do_order);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_owhere);
DECLARE_DO_FUN(do_pardon);
DECLARE_DO_FUN(do_password);
DECLARE_DO_FUN(do_peace);
DECLARE_DO_FUN(do_pecho);
DECLARE_DO_FUN(do_permban);
DECLARE_DO_FUN(do_pick);
DECLARE_DO_FUN(do_pnlist);
DECLARE_DO_FUN(do_pour);
DECLARE_DO_FUN(do_practice);
DECLARE_DO_FUN(do_prefi);
DECLARE_DO_FUN(do_prefix);
DECLARE_DO_FUN(do_prompt);
DECLARE_DO_FUN(do_purge);
DECLARE_DO_FUN(do_push);
DECLARE_DO_FUN(do_put);
DECLARE_DO_FUN(do_quaff);
DECLARE_DO_FUN(do_qui);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_quote);
DECLARE_DO_FUN(do_ravage);
DECLARE_DO_FUN(do_rdesc);
DECLARE_DO_FUN(do_reboo);
DECLARE_DO_FUN(do_reboot);
DECLARE_DO_FUN(do_recho);
DECLARE_DO_FUN(do_recite);
DECLARE_DO_FUN(do_remove);
DECLARE_DO_FUN(do_rename);
DECLARE_DO_FUN(do_repair);
DECLARE_DO_FUN(do_replay);
DECLARE_DO_FUN(do_repop);
DECLARE_DO_FUN(do_report);
DECLARE_DO_FUN(do_rescue);
DECLARE_DO_FUN(do_rest);
DECLARE_DO_FUN(do_return);
DECLARE_DO_FUN(do_rload);
DECLARE_DO_FUN(do_rpoint);
DECLARE_DO_FUN(do_rsave);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_rules);
DECLARE_DO_FUN(do_sacrifice);
DECLARE_DO_FUN(do_save);
DECLARE_DO_FUN(do_scan);
DECLARE_DO_FUN(do_score);
DECLARE_DO_FUN(do_scribe);
DECLARE_DO_FUN(do_scroll);
DECLARE_DO_FUN(do_second);
DECLARE_DO_FUN(do_sell);
DECLARE_DO_FUN(do_set);
DECLARE_DO_FUN(do_setrestore);
DECLARE_DO_FUN(do_show);
DECLARE_DO_FUN(do_shutdow);
DECLARE_DO_FUN(do_shutdown);
DECLARE_DO_FUN(do_sit);
DECLARE_DO_FUN(do_skills);
DECLARE_DO_FUN(do_sla);
DECLARE_DO_FUN(do_slay);
DECLARE_DO_FUN(do_sleep);
DECLARE_DO_FUN(do_slot);
DECLARE_DO_FUN(do_sneak);
DECLARE_DO_FUN(do_snlist);
DECLARE_DO_FUN(do_snoop);
DECLARE_DO_FUN(do_sockets);
DECLARE_DO_FUN(do_south);
DECLARE_DO_FUN(do_spellflags);
DECLARE_DO_FUN(do_spells);
DECLARE_DO_FUN(do_split);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_stat);
DECLARE_DO_FUN(do_steal);
DECLARE_DO_FUN(do_story);
DECLARE_DO_FUN(do_suicide);
DECLARE_DO_FUN(do_switch);
DECLARE_DO_FUN(do_tally);
DECLARE_DO_FUN(do_team);
DECLARE_DO_FUN(do_teamtalk);
DECLARE_DO_FUN(do_teleport);
DECLARE_DO_FUN(do_tick);
DECLARE_DO_FUN(do_time);
DECLARE_DO_FUN(do_title);
DECLARE_DO_FUN(do_trade);
DECLARE_DO_FUN(do_train);
DECLARE_DO_FUN(do_transfer);
DECLARE_DO_FUN(do_trust);
DECLARE_DO_FUN(do_typo);
DECLARE_DO_FUN(do_unalias);
DECLARE_DO_FUN(do_unignore);
DECLARE_DO_FUN(do_unlock);
DECLARE_DO_FUN(do_unread);
DECLARE_DO_FUN(do_untally);
DECLARE_DO_FUN(do_up);
DECLARE_DO_FUN(do_value);
DECLARE_DO_FUN(do_viewhist);
DECLARE_DO_FUN(do_violate);
DECLARE_DO_FUN(do_visible);
DECLARE_DO_FUN(do_vnum);
DECLARE_DO_FUN(do_wake);
DECLARE_DO_FUN(do_wear);
DECLARE_DO_FUN(do_weather);
DECLARE_DO_FUN(do_west);
DECLARE_DO_FUN(do_where);
DECLARE_DO_FUN(do_whirlwind);
DECLARE_DO_FUN(do_who);
DECLARE_DO_FUN(do_whois);
DECLARE_DO_FUN(do_wimpy);
DECLARE_DO_FUN(do_winvis);
DECLARE_DO_FUN(do_withdraw);
DECLARE_DO_FUN(do_wizcommands);
DECLARE_DO_FUN(do_wizhelp);
DECLARE_DO_FUN(do_wizlist);
DECLARE_DO_FUN(do_wizlock);
DECLARE_DO_FUN(do_wiznet);
DECLARE_DO_FUN(do_worldflag);
DECLARE_DO_FUN(do_worth);
DECLARE_DO_FUN(do_zap);
DECLARE_DO_FUN(do_zecho);
DECLARE_DO_FUN(do_die);
DECLARE_DO_FUN(do_imprint);
DECLARE_DO_FUN(do_ovnum);
DECLARE_DO_FUN(do_mvnum);
DECLARE_DO_FUN(do_raceinfo);
DECLARE_DO_FUN(do_radio);
DECLARE_DO_FUN(do_ploa);
DECLARE_DO_FUN(do_pload);
DECLARE_DO_FUN(do_punloa);
DECLARE_DO_FUN(do_punload);
DECLARE_DO_FUN(do_wpeace);


/***************************************************************************
*	blacksmith
***************************************************************************/
DECLARE_DO_FUN(do_engulf);
DECLARE_DO_FUN(do_poison);
DECLARE_DO_FUN(do_electrify);
DECLARE_DO_FUN(do_energize);
DECLARE_DO_FUN(do_chill);
DECLARE_DO_FUN(do_sharpen);
DECLARE_DO_FUN(do_estimate);

/***************************************************************************
*	message commands
***************************************************************************/
DECLARE_DO_FUN(do_note);
DECLARE_DO_FUN(do_contest);
DECLARE_DO_FUN(do_qnote);
DECLARE_DO_FUN(do_rpnote);
DECLARE_DO_FUN(do_aucnote);
DECLARE_DO_FUN(do_news);
DECLARE_DO_FUN(do_penalty);
DECLARE_DO_FUN(do_changes);
DECLARE_DO_FUN(do_idea);
DECLARE_DO_FUN(do_build);

/***************************************************************************
*	OLC and Mob Programs
***************************************************************************/
DECLARE_DO_FUN(do_mob);
DECLARE_DO_FUN(do_mpstat);
DECLARE_DO_FUN(do_mpdump);
DECLARE_DO_FUN(do_olc);
DECLARE_DO_FUN(do_asave);
DECLARE_DO_FUN(do_alist);
DECLARE_DO_FUN(do_resets);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_mpedit);
DECLARE_DO_FUN(do_hedit);
DECLARE_DO_FUN(do_sedit);
DECLARE_DO_FUN(do_skedit);
DECLARE_DO_FUN(do_gredit);
DECLARE_DO_FUN(do_scedit);

