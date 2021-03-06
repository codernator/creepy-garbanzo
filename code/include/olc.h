#define OLCVERSION      "ILAB Online Creation [Beta 1.0, ROM 2.3 modified]\n\r" \
    "     Port a ROM 2.4 v1.8\n\r"

#define AUTHOR  "     By Jason(jdinkel@mines.colorado.edu)\n\r" \
    "     Modified for use with ROM 2.3\n\r"        \
"     By Hans Birkeland (hansbi@ifi.uio.no)\n\r" \
"     Modificado para uso en ROM 2.4b6\n\r"     \
"     Por Ivan Toledo (itoledo@ctcreuna.cl)\n\r"

#define DATE    "     (Apr. 7, 1995 - ROM mod, Apr 16, 1995)\n\r" \
    "     (Port a ROM 2.4 - Nov 2, 1996)\n\r" \
"     Version actual : 1.8 - Sep 8, 1998\n\r"

#define CREDITS "     Original by Surreality(cxw197@psu.edu) and Locke(locke@lm.com)"



/*
 * New typedefs.
 */
typedef bool OLC_FUN(struct char_data * ch, const char *argument);
#define DECLARE_OLC_FUN(fn)     OLC_FUN fn



/* Return true if area changed, false if not. */
#define EDIT(fn)                bool fn(struct char_data * ch, const char *argument)

/*
 * Connected states for editor.
 */
#define ED_NONE         0
#define ED_AREA         1
#define ED_ROOM         2
#define ED_OBJECT       3
#define ED_MOBILE       4
#define ED_MPCODE       5
#define ED_HELP         6
#define ED_SKILL        7
#define ED_GROUP        8


/* general constants */
#define MAX_MOB 1               /* default maximum number for resetting mobs */


/* eidting interpreter entry functions */
void mpedit(struct char_data * ch, const char *argument);
void hedit(struct char_data * ch, const char *argument);
void skedit(struct char_data * ch, const char *argument);
void gredit(struct char_data * ch, const char *argument);
void scedit(struct char_data * ch, const char *argument);
void muedit(struct char_data * ch, const char *argument);
void olc_start_string_editor(/*@dependent@*/struct char_data *ch, /*@dependent@*/void *owner, /*@dependent@*/DESC_GETTER *getter, /*@dependent@*/DESC_SETTER *setter);


/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type {
    const char *const name;
    OLC_FUN *olc_fn;
};



/*
 * Structure for an OLC editor startup command.
 */
struct  editor_cmd_type {
    const char *const name;
    /*@null@*/DO_FUN *do_fn;
    const char *const lexicon;
};

enum medit_auto_config_type { mact_easy, mact_normal, mact_hard, mact_insane };


/* utility functions */
void add_reset(struct roomtemplate * room, struct reset_data * pReset, long index);
char *mprog_type_to_name(int type);



/* cmd tables for the various interpreters */
extern const struct olc_cmd_type mpedit_table[];
extern const struct olc_cmd_type hedit_table[];
extern const struct olc_cmd_type skedit_table[];
extern const struct olc_cmd_type gredit_table[];
extern const struct olc_cmd_type scedit_table[];
extern const struct olc_cmd_type muedit_table[];


/* macros */
#define ALT_FLAGVALUE_SET(_blargh, _table, _arg)                \
{                                                                                                       \
    long blah = flag_value(_table, _arg);                   \
    _blargh = (blah == NO_FLAG) ? 0 : blah;                  \
}

#define ALT_FLAGVALUE_TOGGLE(_blargh, _table, _arg)             \
{                                                                                                       \
    long blah = flag_value(_table, _arg);                   \
    _blargh ^= (blah == NO_FLAG) ? 0 : blah;                 \
}

#define TOGGLE_BIT(var, bit)    ((var) ^= (bit))


/*
 * General Functions
 */
/* general functions */
bool show_commands(struct char_data * ch, const char *argument);
bool show_olc_help(struct char_data * ch, const char *argument);
bool edit_done(struct char_data * ch);


/* area editor */
DECLARE_OLC_FUN(aedit_show);
DECLARE_OLC_FUN(aedit_create);
DECLARE_OLC_FUN(aedit_name);
DECLARE_OLC_FUN(aedit_file);
DECLARE_OLC_FUN(aedit_age);
DECLARE_OLC_FUN(aedit_reset);
DECLARE_OLC_FUN(aedit_security);
DECLARE_OLC_FUN(aedit_builder);
DECLARE_OLC_FUN(aedit_vnum);
DECLARE_OLC_FUN(aedit_lvnum);
DECLARE_OLC_FUN(aedit_uvnum);
DECLARE_OLC_FUN(aedit_complete);
DECLARE_OLC_FUN(aedit_llevel);
DECLARE_OLC_FUN(aedit_ulevel);
DECLARE_OLC_FUN(aedit_desc);


/* room editor */
DECLARE_OLC_FUN(redit_show);
DECLARE_OLC_FUN(redit_name);
DECLARE_OLC_FUN(redit_desc);
DECLARE_OLC_FUN(redit_ed);
DECLARE_OLC_FUN(redit_format);
DECLARE_OLC_FUN(redit_north);
DECLARE_OLC_FUN(redit_south);
DECLARE_OLC_FUN(redit_east);
DECLARE_OLC_FUN(redit_west);
DECLARE_OLC_FUN(redit_up);
DECLARE_OLC_FUN(redit_down);
DECLARE_OLC_FUN(redit_mreset);
DECLARE_OLC_FUN(redit_oreset);
DECLARE_OLC_FUN(redit_mlist);
DECLARE_OLC_FUN(redit_rlist);
DECLARE_OLC_FUN(redit_olist);
DECLARE_OLC_FUN(redit_mshow);
DECLARE_OLC_FUN(redit_oshow);
DECLARE_OLC_FUN(redit_heal);
DECLARE_OLC_FUN(redit_mana);
DECLARE_OLC_FUN(redit_flagall);
DECLARE_OLC_FUN(redit_showrooms);
DECLARE_OLC_FUN(redit_owner);
DECLARE_OLC_FUN(redit_room);
DECLARE_OLC_FUN(redit_sector);
DECLARE_OLC_FUN(redit_addaffect);
DECLARE_OLC_FUN(redit_delaffect);


/* object editor */
DECLARE_OLC_FUN(oedit_show);
DECLARE_OLC_FUN(oedit_name);
DECLARE_OLC_FUN(oedit_short);
DECLARE_OLC_FUN(oedit_long);
DECLARE_OLC_FUN(oedit_addaffect);
DECLARE_OLC_FUN(oedit_addapply);
DECLARE_OLC_FUN(oedit_delaffect);
DECLARE_OLC_FUN(oedit_value0);
DECLARE_OLC_FUN(oedit_value1);
DECLARE_OLC_FUN(oedit_value2);
DECLARE_OLC_FUN(oedit_value3);
DECLARE_OLC_FUN(oedit_value4);
DECLARE_OLC_FUN(oedit_weight);
DECLARE_OLC_FUN(oedit_cost);
DECLARE_OLC_FUN(oedit_ed);
DECLARE_OLC_FUN(oedit_extra);
DECLARE_OLC_FUN(oedit_wear);
DECLARE_OLC_FUN(oedit_type);
DECLARE_OLC_FUN(oedit_affect);
DECLARE_OLC_FUN(oedit_timer);
DECLARE_OLC_FUN(oedit_condition);
DECLARE_OLC_FUN(oedit_extra2);

/* mobile editor */
DECLARE_OLC_FUN(medit_show);
DECLARE_OLC_FUN(medit_create);
DECLARE_OLC_FUN(medit_clone);
DECLARE_OLC_FUN(medit_name);
DECLARE_OLC_FUN(medit_short);
DECLARE_OLC_FUN(medit_long);
DECLARE_OLC_FUN(medit_shop);
DECLARE_OLC_FUN(medit_desc);
DECLARE_OLC_FUN(medit_level);
DECLARE_OLC_FUN(medit_sex);
DECLARE_OLC_FUN(medit_act);
DECLARE_OLC_FUN(medit_affect);
DECLARE_OLC_FUN(medit_ac);
DECLARE_OLC_FUN(medit_form);
DECLARE_OLC_FUN(medit_part);
DECLARE_OLC_FUN(medit_imm);
DECLARE_OLC_FUN(medit_res);
DECLARE_OLC_FUN(medit_vuln);
DECLARE_OLC_FUN(medit_material);
DECLARE_OLC_FUN(medit_off);
DECLARE_OLC_FUN(medit_size);
DECLARE_OLC_FUN(medit_hitdice);
DECLARE_OLC_FUN(medit_manadice);
DECLARE_OLC_FUN(medit_damdice);
DECLARE_OLC_FUN(medit_race);
DECLARE_OLC_FUN(medit_position);
DECLARE_OLC_FUN(medit_gold);
DECLARE_OLC_FUN(medit_hitroll);
DECLARE_OLC_FUN(medit_damtype);
DECLARE_OLC_FUN(medit_group);
DECLARE_OLC_FUN(medit_addmprog);
DECLARE_OLC_FUN(medit_delmprog);

/* mob program editor */
DECLARE_OLC_FUN(mpedit_create);
DECLARE_OLC_FUN(mpedit_clone);
DECLARE_OLC_FUN(mpedit_code);
DECLARE_OLC_FUN(mpedit_comment);
DECLARE_OLC_FUN(mpedit_show);
DECLARE_OLC_FUN(mpedit_list);

/* Help editor */
DECLARE_OLC_FUN(hedit_keyword);
DECLARE_OLC_FUN(hedit_text);
DECLARE_OLC_FUN(hedit_new);
DECLARE_OLC_FUN(hedit_level);
DECLARE_OLC_FUN(hedit_delete);
DECLARE_OLC_FUN(hedit_show);
DECLARE_OLC_FUN(hedit_list);
DECLARE_OLC_FUN(hedit_area);

/* skill editor */
DECLARE_OLC_FUN(skedit_new);
DECLARE_OLC_FUN(skedit_name);
DECLARE_OLC_FUN(skedit_delete);
DECLARE_OLC_FUN(skedit_show);
DECLARE_OLC_FUN(skedit_level);
DECLARE_OLC_FUN(skedit_spell);
DECLARE_OLC_FUN(skedit_affect);
DECLARE_OLC_FUN(skedit_damage_msg);
DECLARE_OLC_FUN(skedit_min_pos);
DECLARE_OLC_FUN(skedit_min_mana);
DECLARE_OLC_FUN(skedit_wait);
DECLARE_OLC_FUN(skedit_target);
DECLARE_OLC_FUN(skedit_message);
DECLARE_OLC_FUN(skedit_obj_message);
DECLARE_OLC_FUN(skedit_others_message);
DECLARE_OLC_FUN(skedit_help);
DECLARE_OLC_FUN(skedit_flags);
DECLARE_OLC_FUN(skedit_difficulty);
DECLARE_OLC_FUN(skedit_act_flag);
DECLARE_OLC_FUN(skedit_off_flag);
DECLARE_OLC_FUN(skedit_percent);
DECLARE_OLC_FUN(skedit_argument);

/* group editor */
DECLARE_OLC_FUN(gredit_new);
DECLARE_OLC_FUN(gredit_delete);
DECLARE_OLC_FUN(gredit_show);
DECLARE_OLC_FUN(gredit_help);
DECLARE_OLC_FUN(gredit_skills);
DECLARE_OLC_FUN(gredit_cost);

DECLARE_OLC_FUN(muedit_show);
DECLARE_OLC_FUN(muedit_new);
DECLARE_OLC_FUN(muedit_delete);
DECLARE_OLC_FUN(muedit_title);
DECLARE_OLC_FUN(muedit_artist);
DECLARE_OLC_FUN(muedit_text);


/* Return pointers to what is being edited. */
#define EDIT_MOB(ch, mob)       (mob = (struct mob_index_data *)ch->desc->ed_data)
#define EDIT_OBJ(ch, obj)       (obj = (struct objecttemplate *)ch->desc->ed_data)
#define EDIT_ROOM(ch, room)     (room = ch->in_room)
#define EDIT_AREA(ch, area)     (area = (struct area_data *)ch->desc->ed_data)
#define EDIT_MPCODE(ch, code)   (code = (struct mprog_code *)ch->desc->ed_data)
#define EDIT_HELP(ch, help)     (help = (struct help_data *)ch->desc->ed_data)
#define EDIT_SKILL(ch, skill)   (skill = (struct dynamic_skill *)ch->desc->ed_data)
#define EDIT_GROUP(ch, group)   (group = (struct dynamic_group *)ch->desc->ed_data)


/* rooms */
struct roomtemplate * new_room_index(void);
void free_room_index(struct roomtemplate * pRoom);
/* mob indexes */
struct mob_index_data *new_mob_index(void);
void free_mob_index(struct mob_index_data * pMob);
/* extra descriptions */
extern struct extra_descr_data *new_extra_descr(void);
extern void free_extra_descr(struct extra_descr_data * pExtra);
/* reset data */
struct reset_data *new_reset_data(void);
void free_reset_data(struct reset_data * pReset);
/* exit data */
struct exit_data *new_exit(void);
void free_exit(struct exit_data * pExit);
/* affect data */
extern struct affect_data *new_affect(void);
extern void free_affect(struct affect_data * pAf);
/* shop data */
struct shop_data *new_shop(void);
void free_shop(struct shop_data * pShop);

extern struct mprog_list *new_mprog(void);
extern void free_mprog(struct mprog_list * mp);

struct mprog_code *new_mpcode(void);
void free_mpcode(struct mprog_code * pMcode);

