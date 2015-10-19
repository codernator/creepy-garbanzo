#include <string.h>
#include <stdio.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "interp.h"
#include "help.h"



extern char *mprog_type_to_name(int type);

/***************************************************************************
 *	local defines
 ***************************************************************************/
struct olc_help_type {
    char *		applies_to;
    char *		command;
    const void *	structure;
    char *		desc;
};


static bool show_version(struct char_data * ch, const char *argument);
static void show_liqlist(struct char_data * ch);
static void show_damlist(struct char_data * ch);
static void aedit(struct char_data * ch, const char *argument);
static void redit(struct char_data * ch, const char *argument);
static void medit(struct char_data * ch, const char *argument);
static void oedit(struct char_data * ch, const char *argument);




/*****************************************************************************
 *                           Interpreter Tables.                             *
 *****************************************************************************/
static const struct olc_cmd_type aedit_table[] =
{
    /*  {   command			function		}, */
    { "age",	 aedit_age	},
    { "builder",	 aedit_builder	},
    { "commands",	 show_commands	},
    { "create",	 aedit_create	},
    { "filename",	 aedit_file	},
    { "name",	 aedit_name	},
    { "reset",	 aedit_reset	},
    { "security",	 aedit_security },
    { "show",	 aedit_show	},
    { "vnum",	 aedit_vnum	},
    { "lvnum",	 aedit_lvnum	},
    { "uvnum",	 aedit_uvnum	},
    { "credits",	 aedit_credits	},
    { "llevel",	 aedit_llevel	},
    { "ulevel",	 aedit_ulevel	},
    { "description", aedit_desc	},
    { "?",		 show_olc_help	},
    { "version",	 show_version	},
    { NULL,		 0,		}
};



static const struct olc_cmd_type redit_table[] =
{
    /*  {   command			function		}, */

    { "commands",  show_commands   },
    { "create",    redit_create    },
    { "clone",     redit_clone     },
    { "desc",      redit_desc      },
    { "ed",	       redit_ed	       },
    { "format",    redit_format    },
    { "name",      redit_name      },
    { "show",      redit_show      },
    { "heal",      redit_heal      },
    { "mana",      redit_mana      },
    { "flagall",   redit_flagall   },
    { "showrooms", redit_showrooms },
    { "north",     redit_north     },
    { "south",     redit_south     },
    { "east",      redit_east      },
    { "west",      redit_west      },
    { "up",	       redit_up	       },
    { "down",      redit_down      },

    /* New reset commands. */
    { "mreset",    redit_mreset    },
    { "oreset",    redit_oreset    },
    { "mlist",     redit_mlist     },
    { "rlist",     redit_rlist     },
    { "olist",     redit_olist     },
    { "mshow",     redit_mshow     },
    { "oshow",     redit_oshow     },
    { "owner",     redit_owner     },
    { "room",      redit_room      },
    { "sector",    redit_sector    },
    { "addaffect", redit_addaffect },
    { "delaffect", redit_delaffect },

    { "?",	       show_olc_help       },
    { "version",   show_version    },

    { NULL,	       0,	       }
};



static const struct olc_cmd_type oedit_table[] =
{
    /*  {   command		function			}, */

    { "addaffect", oedit_addaffect },
    { "addapply",  oedit_addapply  },
    { "commands",  show_commands   },
    { "cost",      oedit_cost      },
    { "create",    oedit_create    },
    { "clone",     oedit_clone     },
    { "delaffect", oedit_delaffect },
    { "ed",	       oedit_ed	       },
    { "long",      oedit_long      },
    { "name",      oedit_name      },
    { "short",     oedit_short     },
    { "show",      oedit_show      },
    { "xptolevel", oedit_xptolevel },
    { "v0",	       oedit_value0    },
    { "v1",	       oedit_value1    },
    { "v2",	       oedit_value2    },
    { "v3",	       oedit_value3    },
    { "v4",	       oedit_value4    },           /* ROM */
    { "weight",    oedit_weight    },

    { "extra",     oedit_extra     },       /* ROM */
    { "extra2",    oedit_extra2    },
    { "wear",      oedit_wear      },       /* ROM */
    { "type",      oedit_type      },       /* ROM */
    { "material",  oedit_material  },       /* ROM */
    { "level",     oedit_level     },       /* ROM */
    { "condition", oedit_condition },       /* ROM */
    { "timer",     oedit_timer     },

    { "?",	       show_olc_help       },
    { "version",   show_version    },

    { NULL,	       0,	       }
};



static const struct olc_cmd_type medit_table[] =
{
    /*  {   command			function		}, */

    { "commands",  show_commands  },
    { "create",    medit_create   },
    { "clone",     medit_clone    },
    { "desc",      medit_desc     },
    { "level",     medit_level    },
    { "long",      medit_long     },
    { "name",      medit_name     },
    { "shop",      medit_shop     },
    { "short",     medit_short    },
    { "show",      medit_show     },

    { "sex",       medit_sex      },        /* ROM */
    { "act",       medit_act      },        /* ROM */
    { "affect",    medit_affect   },        /* ROM */
    { "armor",     medit_ac	      },        /* ROM */
    { "form",      medit_form     },        /* ROM */
    { "part",      medit_part     },        /* ROM */
    { "imm",       medit_imm      },        /* ROM */
    { "res",       medit_res      },        /* ROM */
    { "vuln",      medit_vuln     },        /* ROM */
    { "material",  medit_material },        /* ROM */
    { "off",       medit_off      },        /* ROM */
    { "size",      medit_size     },        /* ROM */
    { "hitdice",   medit_hitdice  },        /* ROM */
    { "manadice",  medit_manadice },        /* ROM */
    { "damdice",   medit_damdice  },        /* ROM */
    { "race",      medit_race     },        /* ROM */
    { "position",  medit_position },        /* ROM */
    { "wealth",    medit_gold     },        /* ROM */
    { "hitroll",   medit_hitroll  },        /* ROM */
    { "damtype",   medit_damtype  },        /* ROM */
    { "group",     medit_group    },        /* ROM */
    { "addmprog",  medit_addmprog },        /* ROM */
    { "delmprog",  medit_delmprog },        /* ROM */

    { "?",	       show_olc_help      },
    { "version",   show_version   },

    { NULL,	       0,	      }
};

/*****************************************************************************
 *                          End Interpreter Tables.                          *
 *****************************************************************************/




/***************************************************************************
 *	run_olc_editor
 *
 *	send the descriptor to the proper editor interpreter
 *	based on their current edit state
 ***************************************************************************/
bool run_olc_editor(struct descriptor_data *d)
{
    bool success = true;

    switch (d->editor) {
      case ED_AREA:
          aedit(d->character, d->incomm);
          break;
      case ED_ROOM:
          redit(d->character, d->incomm);
          break;
      case ED_OBJECT:
          oedit(d->character, d->incomm);
          break;
      case ED_MOBILE:
          medit(d->character, d->incomm);
          break;
      case ED_MPCODE:
          mpedit(d->character, d->incomm);
          break;
      case ED_HELP:
          hedit(d->character, d->incomm);
          break;
      case ED_SKILL:
          skedit(d->character, d->incomm);
          break;
      case ED_GROUP:
          gredit(d->character, d->incomm);
          break;
      default:
          success = false;
    }

    return success;
}



/***************************************************************************
 *	olc_ed_name
 *
 *	get the name of the editor
 ***************************************************************************/
char *olc_ed_name(struct char_data *ch)
{
    static char buf[10];

    buf[0] = '\0';
    switch (ch->desc->editor) {
      case ED_AREA:
          sprintf(buf, "AEdit");
          break;
      case ED_ROOM:
          sprintf(buf, "REdit");
          break;
      case ED_OBJECT:
          sprintf(buf, "OEdit");
          break;
      case ED_MOBILE:
          sprintf(buf, "MEdit");
          break;
      case ED_MPCODE:
          sprintf(buf, "MPEdit");
          break;
      case ED_HELP:
          sprintf(buf, "HEdit");
          break;
      case ED_SKILL:
          sprintf(buf, "SKEdit");
          break;
      case ED_GROUP:
          sprintf(buf, "GREdit");
          break;
      default:
          sprintf(buf, " ");
          break;
    }

    return buf;
}


/***************************************************************************
 *	olc_ed_vnum
 ***************************************************************************/
char *olc_ed_vnum(struct char_data *ch)
{
    struct area_data *pArea;
    struct room_index_data *pRoom;
    struct objectprototype *pObj;
    struct mob_index_data *pMob;
    struct mprog_code *pMprog;
    HELP_DATA *pHelp;
    SKILL *pSkill;
    GROUP *pGroup;
    static char buf[MAX_INPUT_LENGTH];

    buf[0] = '\0';
    switch (ch->desc->editor) {
      case ED_AREA:
          pArea = (struct area_data *)ch->desc->ed_data;
          sprintf(buf, "%ld", pArea ? pArea->vnum : 0);
          break;
      case ED_ROOM:
          pRoom = ch->in_room;
          sprintf(buf, "%ld", pRoom ? pRoom->vnum : 0);
          break;
      case ED_OBJECT:
          pObj = (struct objectprototype *)ch->desc->ed_data;
          sprintf(buf, "%ld", pObj ? pObj->vnum : 0);
          break;
      case ED_MOBILE:
          pMob = (struct mob_index_data *)ch->desc->ed_data;
          sprintf(buf, "%ld", pMob ? pMob->vnum : 0);
          break;
      case ED_MPCODE:
          pMprog = (struct mprog_code *)ch->desc->ed_data;
          sprintf(buf, "%ld", pMprog ? pMprog->vnum : 0);
          break;
      case ED_HELP:
          pHelp = (HELP_DATA *)ch->desc->ed_data;
          sprintf(buf, "%s", pHelp ? pHelp->keyword : "");
          break;
      case ED_SKILL:
          pSkill = (SKILL *)ch->desc->ed_data;
          sprintf(buf, "%s", pSkill ? pSkill->name : "");
          break;
      case ED_GROUP:
          pGroup = (GROUP *)ch->desc->ed_data;
          sprintf(buf, "%s", pGroup ? pGroup->name : "");
      default:
          sprintf(buf, " ");
          break;
    }

    return buf;
}



/*****************************************************************************
 * Name:		show_olc_cmds
 * Purpose:	Format up the commands from given table.
 * Called by:	show_commands(olc_act.c).
 ****************************************************************************/
static void show_olc_cmds(struct char_data *ch, const struct olc_cmd_type *olc_table)
{
    struct buf_type *out;
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;

    col = 0;
    out = new_buf();
    for (cmd = 0; olc_table[cmd].name != NULL; cmd++) {
        sprintf(buf, "%-15.15s", olc_table[cmd].name);
        add_buf(out, buf);
        if (++col % 5 == 0)
            add_buf(out, "\n\r");
    }

    if (col % 5 != 0)
        add_buf(out, "\n\r");

    send_to_char(buf_string(out), ch);
    free_buf(out);
    return;
}



/*****************************************************************************
 * Name:		show_commands
 * Purpose:	Display all olc commands.
 * Called by:	olc interpreters.
 ****************************************************************************/
bool show_commands(struct char_data *ch, const char *argument)
{
    switch (ch->desc->editor) {
      case ED_AREA:
          show_olc_cmds(ch, aedit_table);
          break;
      case ED_ROOM:
          show_olc_cmds(ch, redit_table);
          break;
      case ED_OBJECT:
          show_olc_cmds(ch, oedit_table);
          break;
      case ED_MOBILE:
          show_olc_cmds(ch, medit_table);
          break;
      case ED_MPCODE:
          show_olc_cmds(ch, mpedit_table);
          break;
      case ED_HELP:
          show_olc_cmds(ch, hedit_table);
          break;
      case ED_SKILL:
          show_olc_cmds(ch, skedit_table);
          break;
      case ED_GROUP:
          show_olc_cmds(ch, gredit_table);
          break;
    }

    return false;
}


/***************************************************************************
 * Name:		edit_done
 * Purpose:	Resets builder information on completion.
 * Called by:	aedit, redit, oedit, medit(olc.c)
 **************************************************************************/
bool edit_done(struct char_data *ch)
{
    ch->desc->ed_data = NULL;
    ch->desc->editor = 0;

    return false;
}

/***************************************************************************
 *                              Interpreters.                               *
 ***************************************************************************/
/***************************************************************************
 *	aedit
 *
 *	area editing interpreter
 ***************************************************************************/
void aedit(struct char_data *ch, const char *argument)
{
    struct area_data *pArea;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    const char *parg;
    int cmd;
    int value;

    EDIT_AREA(ch, pArea);
    strcpy(arg, argument);
    smash_tilde(arg);
    parg = one_argument(arg, command);

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("AEdit:  Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        aedit_show(ch, parg);
        return;
    }

    if ((value = flag_value(area_flags, command)) != NO_FLAG) {
        TOGGLE_BIT(pArea->area_flags, value);
        send_to_char("Flag toggled.\n\r", ch);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; aedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, aedit_table[cmd].name)) {
            if ((*aedit_table[cmd].olc_fn)(ch, parg)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                return;
            } else {
                return;
            }
        }
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return;
}


/*****************************************************************************
 *	do_redit
 *
 *	room editing interpreter
 *****************************************************************************/
void redit(struct char_data *ch, const char *argument)
{
    struct area_data *pArea;
    struct room_index_data *pRoom;
    char arg[MAX_STRING_LENGTH];
    const char *parg;
    char command[MAX_INPUT_LENGTH];
    int cmd;

    EDIT_ROOM(ch, pRoom);
    pArea = pRoom->area;

    strcpy(arg, argument);
    smash_tilde(arg);
    parg = one_argument(arg, command);

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("REdit:  Insufficient security to modify room.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        redit_show(ch, parg);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; redit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, redit_table[cmd].name)) {
            if ((*redit_table[cmd].olc_fn)(ch, parg)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                return;
            } else {
                return;
            }
        }
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return;
}


/*****************************************************************************
 *	do_oedit
 *
 *	object editing interpreter
 *****************************************************************************/
void oedit(struct char_data *ch, const char *argument)
{
    struct area_data *pArea;
    struct objectprototype *pObj;
    char arg[MAX_STRING_LENGTH];
    const char *parg;
    char command[MAX_INPUT_LENGTH];
    int cmd;

    strcpy(arg, argument);
    smash_tilde(arg);
    parg = one_argument(arg, command);

    EDIT_OBJ(ch, pObj);
    pArea = pObj->area;

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("OEdit: Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        oedit_show(ch, parg);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; oedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, oedit_table[cmd].name)) {
            if ((*oedit_table[cmd].olc_fn)(ch, parg)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                return;
            } else {
                return;
            }
        }
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return;
}



/*****************************************************************************
 *	do_medit
 *
 *	mobile editing interpreter
 *****************************************************************************/
void medit(struct char_data *ch, const char *argument)
{
    struct area_data *pArea;
    struct mob_index_data *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_STRING_LENGTH];
    const char *parg;
    int cmd;

    strcpy(arg, argument);
    smash_tilde(arg);
    parg = one_argument(arg, command);

    EDIT_MOB(ch, pMob);
    pArea = pMob->area;

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("MEdit: Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        medit_show(ch, parg);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; medit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, medit_table[cmd].name)) {
            if ((*medit_table[cmd].olc_fn)(ch, parg)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                return;
            } else {
                return;
            }
        }
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return;
}




static const struct editor_cmd_type editor_table[] =
{
    /*  {   command		function	}, */

    { "area",   do_aedit  },
    { "room",   do_redit  },
    { "object", do_oedit  },
    { "mobile", do_medit  },
    { "mpcode", do_mpedit },
    { "hedit",  do_hedit  },

    { NULL,	    0,	      }
};



/*****************************************************************************
 *	do_olc
 *****************************************************************************/
void do_olc(struct char_data *ch, const char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmd;

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, command);

    if (command[0] == '\0') {
        show_help(ch->desc, "olc", NULL);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; editor_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, editor_table[cmd].name)) {
            (*editor_table[cmd].do_fn)(ch, argument);
            return;
        }
    }

    /* Invalid command, send help. */
    show_help(ch->desc, "olc", NULL);
    return;
}



/*****************************************************************************
 *	do_aedit
 *****************************************************************************/
void do_aedit(struct char_data *ch, const char *argument)
{
    struct area_data *pArea;
    int value;
    char arg[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    pArea = ch->in_room->area;
    argument = one_argument(argument, arg);

    if (is_number(arg)) {
        value = parse_int(arg);
        if (!(pArea = area_getbyvnum(value))) {
            send_to_char("That area vnum does not exist.\n\r", ch);
            return;
        }
    } else {
        if (!str_cmp(arg, "create")) {
            if (ch->pcdata->security < 9) {
                send_to_char("AEdit : Insufficient security to create areas.\n\r", ch);
                return;
            }

            aedit_create(ch, "");
            ch->desc->editor = ED_AREA;
            return;
        }
    }

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("Insufficient security to edit areas.\n\r", ch);
        return;
    }

    ch->desc->ed_data = (void *)pArea;
    ch->desc->editor = ED_AREA;
    return;
}



/*****************************************************************************
 *	display_resets
 *****************************************************************************/
static void display_resets(struct char_data *ch)
{
    struct room_index_data *pRoom;
    struct reset_data *pReset;
    struct mob_index_data *pMob = NULL;
    struct buf_type *final;
    char *uncolor;
    char *uncolor2;
    long num = 0;

    EDIT_ROOM(ch, pRoom);
    final = new_buf();

    send_to_char(" No.  Loads    Description       Location         Vnum   Mx Mn Description\n\r"
                 "==== ======== ============= =================== ======== ===== ===========\n\r", ch);

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next) {
        struct objectprototype *pObj;
        struct mob_index_data *pMobIndex;
        struct objectprototype *pObjIndex;
        struct objectprototype *pObjToIndex;
        struct room_index_data *pRoomIndex;
        struct room_index_data *pRoomIndexPrev;

        printf_buf(final, "[`1%2d``] ", ++num);
        switch (pReset->command) {
          default:
              printf_buf(final, "Bad reset command: %c.", pReset->command);
              break;

          case 'M':
              if (!(pMobIndex = get_mob_index(pReset->arg1))) {
                  printf_buf(final, "Load Mobile - Bad Mob %d\n\r", pReset->arg1);
                  continue;
              }

              if (!(pRoomIndex = get_room_index(pReset->arg3))) {
                  printf_buf(final, "Load Mobile - Bad Room %d\n\r", pReset->arg3);
                  continue;
              }

              pMob = pMobIndex;

              pRoomIndexPrev = get_room_index(pRoomIndex->vnum - 1);
              if (pRoomIndexPrev
                  && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP)) {
                  uncolor = uncolor_str(pMob->short_descr);
                  printf_buf(final, "`PP``[%5d] %-13.13s In room             `!R``[%5d] %2d-%2d %-15.15s\n\r",
                             pReset->arg1,
                             uncolor,
                             pReset->arg3,
                             pReset->arg2,
                             pReset->arg4,
                             pRoomIndex->name);
                  free_string(uncolor);
              } else {
                  uncolor = uncolor_str(pMob->short_descr);
                  printf_buf(final, "`@M``[%5d] %-13.13s In room             `!R``[%5d] %2d-%2d %-15.15s\n\r",
                             pReset->arg1,
                             uncolor,
                             pReset->arg3,
                             pReset->arg2,
                             pReset->arg4,
                             pRoomIndex->name);
                  free_string(uncolor);
              }
              break;

          case 'O':
              if (!(pObjIndex = objectprototype_getbyvnum(pReset->arg1))) {
                  printf_buf(final, "Load Object - Bad Object %d\n\r", pReset->arg1);
                  continue;
              }

              pObj = pObjIndex;

              if (!(pRoomIndex = get_room_index(pReset->arg3))) {
                  printf_buf(final, "Load Object - Bad Room %d\n\r", pReset->arg3);
                  continue;
              }

              uncolor = uncolor_str(pObj->short_descr);
              printf_buf(final, "`#O``[%5d] %-13.13s In room             "
                         "`!R``[%5d]       %-15.15s\n\r",
                         pReset->arg1,
                         uncolor,
                         pReset->arg3,
                         pRoomIndex->name);
              free_string(uncolor);
              break;

          case 'P':
              if (!(pObjIndex = objectprototype_getbyvnum(pReset->arg1))) {
                  printf_buf(final, "Put Object - Bad Object %d\n\r", pReset->arg1);
                  continue;
              }

              pObj = pObjIndex;
              if (!(pObjToIndex = objectprototype_getbyvnum(pReset->arg3))) {
                  printf_buf(final, "Put Object - Bad To Object %d\n\r", pReset->arg3);
                  continue;
              }

              uncolor = uncolor_str(pObj->short_descr);
              printf_buf(final, "`#O``[%5d] %-13.13s Inside              `#O``[%5d] %2d-%2d %-15.15s\n\r",
                         pReset->arg1,
                         uncolor,
                         pReset->arg3,
                         pReset->arg2,
                         pReset->arg4,
                         pObjToIndex->short_descr);
              free_string(uncolor);
              break;

          case 'G':
          case 'E':
              if (!(pObjIndex = objectprototype_getbyvnum(pReset->arg1))) {
                  printf_buf(final, "Give/Equip Object - Bad Object %d\n\r", pReset->arg1);
                  continue;
              }

              pObj = pObjIndex;
              if (!pMob) {
                  printf_buf(final, "Give/Equip Object - No Previous Mobile\n\r");
                  break;
              }

              uncolor = uncolor_str(pObj->short_descr);
              uncolor2 = uncolor_str(pMob->short_descr);
              if (pMob->shop) {
                  printf_buf(final, "`#O``[%5d] %-13.13s Sold in `Oshop`` of `@M``[%5d]           %-15.15s\n\r",
                             pReset->arg1,
                             uncolor,
                             pMob->vnum,
                             uncolor2);
              } else {
                  printf_buf(final, "`#O``[%5d] %-13.13s %-19.19s `@M``[%5d]       %-15.15s\n\r",
                             pReset->arg1,
                             uncolor,
                             (pReset->command == 'G')
                             ? flag_string(wear_loc_strings, WEAR_NONE)
                             : flag_string(wear_loc_strings, pReset->arg3),
                             pMob->vnum,
                             uncolor2);
              }
              free_string(uncolor);
              free_string(uncolor2);
              break;

              /*
               * Doors are set in rs_flags don't need to be displayed.
               * If you want to display them then uncomment the new_reset
               * line in the case 'D' in load_resets in db.c and here.
               */
          case 'D':
              pRoomIndex = get_room_index(pReset->arg1);
              printf_buf(final, "`!R``[%5d] %s door of %-19.19s reset to %s\n\r",
                         pReset->arg1,
                         capitalize(dir_name[pReset->arg2]),
                         pRoomIndex->name,
                         flag_string(door_resets, pReset->arg3));
              break;
          case 'R':
              if (!(pRoomIndex = get_room_index(pReset->arg1))) {
                  printf_buf(final, "Randomize Exits - Bad Room %d\n\r", pReset->arg1);
                  continue;
              }

              printf_buf(final, "`!R``[%5d] Exits are randomized in %s\n\r",
                         pReset->arg1, pRoomIndex->name);
              break;
        }
    }

    send_to_char(buf_string(final), ch);
    free_buf(final);
    return;
}





/*****************************************************************************
 * Name:		add_reset
 * Purpose:	Inserts a new reset in the given index slot.
 * Called by:	do_resets(olc.c).
 ****************************************************************************/
void add_reset(struct room_index_data *room, struct reset_data *pReset, long index)
{
    struct reset_data *reset;
    long num = 0;

    if (!room->reset_first) {
        room->reset_first = pReset;
        room->reset_last = pReset;
        pReset->next = NULL;
        return;
    }

    index--;
    if (index == 0) {
        pReset->next = room->reset_first;
        room->reset_first = pReset;
        return;
    }

    for (reset = room->reset_first; reset->next; reset = reset->next)
        if (++num == index)
            break;

    pReset->next = reset->next;
    reset->next = pReset;
    if (!pReset->next)
        room->reset_last = pReset;
    return;
}


/*****************************************************************************
 *	do_resets
 *****************************************************************************/
void do_resets(struct char_data *ch, const char *argument)
{
    struct reset_data *pReset = NULL;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char arg5[MAX_INPUT_LENGTH];
    char arg6[MAX_INPUT_LENGTH];
    char arg7[MAX_INPUT_LENGTH];

    if (!IS_BUILDER(ch, ch->in_room->area)) {
        send_to_char("Resets: Invalid security for editing this area.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    argument = one_argument(argument, arg4);
    argument = one_argument(argument, arg5);
    argument = one_argument(argument, arg6);
    argument = one_argument(argument, arg7);

    if (arg1[0] == '\0') {
        if (ch->in_room->reset_first) {
            send_to_char("Resets: M = mobile, R = room, O = object, "
                         "P = pet, S = shopkeeper\n\r", ch);
            display_resets(ch);
        } else {
            send_to_char("No resets in this room.\n\r", ch);
        }

        return;
    }


    if (is_number(arg1)) {
        struct room_index_data *pRoom = ch->in_room;

        if (!str_cmp(arg2, "delete")) {
            int insert_loc = parse_int(arg1);

            if (!ch->in_room->reset_first) {
                send_to_char("No resets in this area.\n\r", ch);
                return;
            }

            if (insert_loc - 1 <= 0) {
                pReset = pRoom->reset_first;
                pRoom->reset_first = pRoom->reset_first->next;
                if (!pRoom->reset_first)
                    pRoom->reset_last = NULL;
            } else {
                struct reset_data *prev = NULL;
                long num = 0;

                for (pReset = pRoom->reset_first;
                     pReset;
                     pReset = pReset->next) {
                    if (++num == insert_loc)
                        break;
                    prev = pReset;
                }

                if (!pReset) {
                    send_to_char("Reset not found.\n\r", ch);
                    return;
                }

                if (prev)
                    prev->next = prev->next->next;
                else
                    pRoom->reset_first = pRoom->reset_first->next;

                for (pRoom->reset_last = pRoom->reset_first;
                     pRoom->reset_last->next;
                     pRoom->reset_last = pRoom->reset_last->next) ;
            }
            free_reset_data(pReset);
            send_to_char("Reset deleted.\n\r", ch);
        } else {
            if (!str_cmp(arg2, "mob")) {
                if (get_mob_index(is_number(arg3) ? parse_int(arg3) : 1) == NULL) {
                    send_to_char("Mob does not exist.\n\r", ch);
                    return;
                }

                pReset = new_reset_data();
                pReset->command = 'M';
                pReset->arg1 = parse_int(arg3);
                pReset->arg2 = is_number(arg4) ? parse_int(arg4) : 1;
                pReset->arg3 = ch->in_room->vnum;
                pReset->arg4 = is_number(arg5) ? parse_int(arg5) : 1;

                add_reset(ch->in_room, pReset, parse_int(arg1));
                SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
                send_to_char("Mobile reset added.\n\r", ch);
            } else if (!str_cmp(arg2, "obj")) {
                pReset = new_reset_data();
                pReset->arg1 = parse_int(arg3);

                if (!str_prefix(arg4, "inside")) {
                    struct objectprototype *temp;

                    temp = objectprototype_getbyvnum(is_number(arg5) ? parse_int(arg5) : 1);
                    if (temp == NULL
                        || ((temp->item_type != ITEM_CONTAINER)
                            && (temp->item_type != ITEM_CORPSE_NPC))) {
                        send_to_char("Object 2 is not a container.\n\r", ch);
                        return;
                    }
                    pReset->command = 'P';
                    pReset->arg2 = is_number(arg6) ? parse_int(arg6) : 1;
                    pReset->arg3 = is_number(arg5) ? parse_int(arg5) : 1;
                    pReset->arg4 = is_number(arg7) ? parse_int(arg7) : 1;
                } else if (!str_cmp(arg4, "room")) {
                    if (objectprototype_getbyvnum(parse_int(arg3)) == NULL) {
                        send_to_char("Vnum doest not exist.\n\r", ch);
                        return;
                    }
                    pReset->command = 'O';
                    pReset->arg2 = 0;
                    pReset->arg3 = ch->in_room->vnum;
                    pReset->arg4 = 0;
                } else {
                    if (flag_value(wear_loc_flags, arg4) == NO_FLAG) {
                        send_to_char("Resets: '? wear-loc'\n\r", ch);
                        return;
                    }
                    if (objectprototype_getbyvnum(parse_int(arg3)) == NULL) {
                        send_to_char("Vnum does not exist.\n\r", ch);
                        return;
                    }
                    pReset->arg1 = parse_int(arg3);
                    pReset->arg3 = flag_value(wear_loc_flags, arg4);
                    if (pReset->arg3 == WEAR_NONE)
                        pReset->command = 'G';
                    else
                        pReset->command = 'E';
                }

                add_reset(ch->in_room, pReset, parse_int(arg1));
                SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
                send_to_char("Object reset added.\n\r", ch);
            } else if (!str_cmp(arg2, "random") && is_number(arg3)) {
                if (parse_int(arg3) < 1 || parse_int(arg3) > 6) {
                    send_to_char("Invalid argument.\n\r", ch);
                    return;
                }
                pReset = new_reset_data();
                pReset->command = 'R';
                pReset->arg1 = ch->in_room->vnum;
                pReset->arg2 = parse_int(arg3);
                add_reset(ch->in_room, pReset, parse_int(arg1));
                SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
                send_to_char("Random exits reset added.\n\r", ch);
            } else {
                send_to_char("Syntax: RESET <number> OBJ <vnum> <wear_loc>\n\r", ch);
                send_to_char("        RESET <number> OBJ <vnum> inside <vnum> [limit] [count]\n\r", ch);
                send_to_char("        RESET <number> OBJ <vnum> room\n\r", ch);
                send_to_char("        RESET <number> MOB <vnum> [max #x area] [max #x room]\n\r", ch);
                send_to_char("        RESET <number> DELETE\n\r", ch);
                send_to_char("        RESET <number> RANDOM [#x exits]\n\r", ch);
            }
        }
    }
}


/*****************************************************************************
 * Name:		do_alist
 * Purpose:	Normal command to list areas and display area information.
 * Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_alist(struct char_data *ch, const char *argument)
{
    struct area_data *pArea;
    struct area_iterator *iterator;
    struct buf_type *buf;

    DENY_NPC(ch);

    buf = new_buf();
    printf_buf(buf, "[%3s] [%-27s](%-5s-%5s) [%-10s] %3s [%-10s]\n\r",
               "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");


    iterator = area_iterator_start(NULL);
    while (iterator != NULL) {
        pArea = iterator->current;
        printf_buf(buf, "[%3d] %-29.29s(%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
                   pArea->vnum,
                   pArea->name,
                   pArea->min_vnum,
                   pArea->max_vnum,
                   pArea->file_name,
                   pArea->security,
                   pArea->builders);
        iterator = area_iterator(iterator, NULL);
    }

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
}



/***************************************************************************
 *	show_version
 ***************************************************************************/
bool show_version(struct char_data *ch, const char *argument)
{
    printf_to_char(ch, "%s\n\r%s\n\r%s\n\r%s\n\r", OLCVERSION, AUTHOR, DATE, CREDITS);
    return false;
}

/***************************************************************************
 *	help_table
 *
 *	shows a brief description of each command
 ***************************************************************************/
static const struct olc_help_type help_table[] =
{
    /* area/rooms */
    { "areas",     "area",	       area_flags,	 "Area attributes."	       },
    { "rooms",     "room",	       room_flags,	 "Room attributes."	       },
    { "rooms",     "sector",       sector_flags,	 "Sector types, terrain."      },
    { "rooms",     "exit",	       exit_flags,	 "Exit types."		       },

    /* objects */
    { "objects",   "type",	       type_flags,	 "Types of objects."	       },
    { "objects",   "extra",	       extra_flags,	 "Object attributes."	       },
    { "objects",   "extra2",       extra2_flags,	 "More Object attributes."     },
    { "objects",   "wear",	       wear_flags,	 "Where to wear object."       },
    { "objects",   "armor",	       ac_type,		 "Ac for different attacks."   },
    { "objects",   "apply",	       apply_flags,	 "Apply flags"		       },
    { "objects",   "wclass",       weapon_class,	 "Weapon class."	       },
    { "objects",   "weapon_flag",  weapon_flag_type, "Weapon flags."	       },
    { "objects",   "portal",       portal_flags,	 "Portal types."	       },
    { "objects",   "furniture",    furniture_flags,	 "Furniture types."	       },
    { "objects",   "liquid",       liq_table,	 "Liquid types."	       },
    { "objects",   "apptype",      apply_types,	 "Apply types."		       },
    { "objects",   "weapon",       attack_table,	 "Weapon types."	       },
    { "objects",   "socket_type",  socket_flags,	 "Inlay types."		       },
    { "objects",   "socket_value", socket_values,	 "Inlay quality values."       },

    /* mobs */
    { "mobs",      "sex",	       sex_flags,	 "Sexes."		       },
    { "mobs",      "act",	       act_flags,	 "Mobile attributes."	       },
    { "mobs",      "affect",       affect_flags,	 "Mobile affects."	       },
    { "mobs",      "form",	       form_flags,	 "Mobile body form."	       },
    { "mobs",      "part",	       part_flags,	 "Mobile body parts."	       },
    { "mobs",      "imm",	       imm_flags,	 "Mobile immunity."	       },
    { "mobs",      "res",	       res_flags,	 "Mobile resistance."	       },
    { "mobs",      "vuln",	       vuln_flags,	 "Mobile vulnerability."       },
    { "mobs",      "off",	       off_flags,	 "Mobile offensive behaviour." },
    { "mobs",      "size",	       size_flags,	 "Mobile size."		       },
    { "mobs",      "position",     position_flags,	 "Mobile positions."	       },

    /* reset flags */
    { "resets",    "wear_loc",     wear_loc_flags,	 "Where mobile wears object."  },
    { "resets",    "container",    container_flags,	 "Container status."	       },
    { "mob progs", "triggers",     mprog_flags,	 "MobProgram triggers."	       },

    /* skills */
    { "skills",    "target",       target_flags,	 "Skill status flags."	       },
    { "skills",    "skill_flags",  skill_flags,	 "Skill flags."		       },
    { "",	       "",	       NULL,		 ""			       }
};



/*****************************************************************************
 * Name:		show_flag_cmds
 * Purpose:	Displays settable flags and stats.
 * Called by:	show_olc_help(olc_act.c).
 ****************************************************************************/
static void show_flag_cmds(struct char_data *ch, const struct flag_type *flag_table)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int flag;
    int col;

    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++) {
        if (flag_table[flag].settable) {
            sprintf(buf, "%-19.18s", flag_table[flag].name);
            strcat(buf1, buf);
            if (++col % 4 == 0)
                strcat(buf1, "\n\r");
        }
    }

    if (col % 4 != 0)
        strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}


/*****************************************************************************
 * Name:		show_skill_cmds
 * Purpose:	Displays all skill functions.
 *              Does remove those damn immortal commands from the list.
 *              Could be improved by:
 *              (1) Adding a check for a particular class.
 *              (2) Adding a check for a level range.
 * Called by:	show_olc_help(olc_act.c).
 ****************************************************************************/
void show_skill_cmds(struct char_data *ch, int tar)
{
    struct buf_type *buf;
    SKILL *skill;
    int col;

    buf = new_buf();
    col = 0;
    for (skill = skill_list; skill != NULL; skill = skill->next) {
        if (!str_cmp(skill->name, "reserved")
            || skill->spells == NULL)
            continue;

        if (tar == -1 || skill->target == tar) {
            printf_buf(buf, "%-19.18s", skill->name);
            if (++col % 4 == 0)
                add_buf(buf, "\n\r");
        }
    }

    if (col % 4 != 0)
        add_buf(buf, "\n\r");

    send_to_char(buf_string(buf), ch);
    free_buf(buf);
    return;
}


/***************************************************************************
 *	show_olc_help
 *
 *	show helps for most of the tables used in OLC
 ***************************************************************************/
bool show_olc_help(struct char_data *ch, const char *argument)
{
    bool found;
    int cnt;

    /* if we have an argument, see if it matches a table */
    if (argument[0] != '\0') {
        /* find the command, show changeable data */
        for (cnt = 0; help_table[cnt].command[0] != '\0'; cnt++) {
            if (argument[0] == help_table[cnt].command[0]
                && !str_prefix(argument, help_table[cnt].command)) {
                if (help_table[cnt].structure == liq_table)
                    show_liqlist(ch);
                else if (help_table[cnt].structure == attack_table)
                    show_damlist(ch);
                else
                    show_flag_cmds(ch, help_table[cnt].structure);
                return false;
            }
        }
    }


    /* show the list of tables */
    found = false;
    for (cnt = 0; help_table[cnt].command[0] != '\0'; cnt++) {
        if (argument[0] == '\0' || !str_prefix(argument, help_table[cnt].applies_to)) {
            if (!found) {
                send_to_char("`#Syntax`3:``  ? [command]\n\r\n\r", ch);
                send_to_char("`!Applies To      Command            Description\n\r", ch);
                send_to_char("`1----------------------------------------------------``\n\r", ch);
                found = true;
            }
            printf_to_char(ch, "`@%-15.15s`` %-18.18s %s\n\r",
                           help_table[cnt].applies_to,
                           help_table[cnt].command,
                           help_table[cnt].desc);
        }
    }

    if (!found)
        show_olc_help(ch, "");

    return false;
}






/***************************************************************************
 *	wear_table
 *
 *	maps wear locations to wear bits
 ***************************************************************************/
static const struct wear_type {
    int	wear_loc;
    long	wear_bit;
}
wear_table[] =
{
    { WEAR_NONE,	 ITEM_TAKE	  },
    { WEAR_LIGHT,	 ITEM_LIGHT	  },
    { WEAR_FINGER_L, ITEM_WEAR_FINGER },
    { WEAR_FINGER_R, ITEM_WEAR_FINGER },
    { WEAR_NECK_1,	 ITEM_WEAR_NECK	  },
    { WEAR_NECK_2,	 ITEM_WEAR_NECK	  },
    { WEAR_BODY,	 ITEM_WEAR_BODY	  },
    { WEAR_HEAD,	 ITEM_WEAR_HEAD	  },
    { WEAR_FACE,	 ITEM_WEAR_FACE	  },
    { WEAR_EAR_L,	 ITEM_WEAR_EAR	  },
    { WEAR_EAR_R,	 ITEM_WEAR_EAR	  },
    { WEAR_LEGS,	 ITEM_WEAR_LEGS	  },
    { WEAR_FEET,	 ITEM_WEAR_FEET	  },
    { WEAR_HANDS,	 ITEM_WEAR_HANDS  },
    { WEAR_ARMS,	 ITEM_WEAR_ARMS	  },
    { WEAR_SHIELD,	 ITEM_WEAR_SHIELD },
    { WEAR_ABOUT,	 ITEM_WEAR_ABOUT  },
    { WEAR_WAIST,	 ITEM_WEAR_WAIST  },
    { WEAR_WRIST_L,	 ITEM_WEAR_WRIST  },
    { WEAR_WRIST_R,	 ITEM_WEAR_WRIST  },
    { WEAR_WIELD,	 ITEM_WIELD	  },
    { WEAR_HOLD,	 ITEM_HOLD	  },
    { WEAR_TATTOO,	 ITEM_WEAR_TATTOO },
    { NO_FLAG,	 NO_FLAG	  }
};



/*****************************************************************************
 * Name:		wear_loc
 * Purpose:	Returns the location of the bit that matches the count.
 *              1 = first match, 2 = second match etc.
 * Called by:	oedit_reset(olc_act.c).
 ****************************************************************************/
int wear_loc(int bits, int count)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++)
        if (IS_SET(bits, wear_table[flag].wear_bit) && --count < 1)
            return wear_table[flag].wear_loc;

    return NO_FLAG;
}



/*****************************************************************************
 * Name:		wear_bit
 * Purpose:	Converts a wear_loc into a bit.
 * Called by:	redit_oreset(olc_act.c).
 ****************************************************************************/
long wear_bit(int loc)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++)
        if (loc == wear_table[flag].wear_loc)
            return wear_table[flag].wear_bit;

    return 0;
}


/***************************************************************************
 *	show_liqlist
 *
 *	show a list of all the available liquids
 ***************************************************************************/
void show_liqlist(struct char_data *ch)
{
    int liq;

    for (liq = 0; liq_table[liq].liq_name != NULL; liq++) {
        if ((liq % 21) == 0)
            send_to_char("Name                 Color          "
                         "Proof Full Thirst Food Size\n\r", ch);

        printf_to_char(ch, "%-20s %-14s %5d %4d %6d %4d %5d\n\r",
                       liq_table[liq].liq_name,
                       liq_table[liq].liq_color,
                       liq_table[liq].liq_affect[0],
                       liq_table[liq].liq_affect[1],
                       liq_table[liq].liq_affect[2],
                       liq_table[liq].liq_affect[3],
                       liq_table[liq].liq_affect[4]);
    }
    return;
}


/***************************************************************************
 *	show_damlist
 *
 *	shows a list of all the damage types
 ***************************************************************************/
void show_damlist(struct char_data *ch)
{
    int att;

    for (att = 0; attack_table[att].name != NULL; att++) {
        if ((att % 21) == 0)
            send_to_char("Name                 Noun\n\r", ch);

        printf_to_char(ch, "%-20s %-20s\n\r",
                       attack_table[att].name,
                       attack_table[att].noun);
    }

    return;
}
