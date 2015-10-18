#include "merc.h"
#include "recycle.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"
#include "skills.h"

#include <stdio.h>
#include "libfile.h"
#include <assert.h>
#include <string.h>

extern int _filbuf(FILE *);
extern unsigned int fread_uint(FILE *fp);
extern long fread_long(FILE *fp);

/***************************************************************************
 *	array of containers read for proper re-nesting of objects.
 ***************************************************************************/
#define MAX_NEST        100

static GAMEOBJECT *rgObjNest[MAX_NEST];


void fread_char(CHAR_DATA * ch, FILE * fp);
extern int message_type_lookup(char *name);

extern char *fread_norm_string(FILE * fp);
int rename(const char *oldfname, const char *newfname);
static bool load_rdesc(ROOM_INDEX_DATA * location, const char *name);


/***************************************************************************
 *	local functions
 ***************************************************************************/
static void fwrite_char(CHAR_DATA * ch, FILE * fp);
static void fwrite_obj(CHAR_DATA * ch, GAMEOBJECT * obj, FILE * fp, int iNest);
static void fwrite_pet(CHAR_DATA * pet, FILE * fp);
static void fwrite_rdesc(ROOM_INDEX_DATA * location, FILE * fp);
static void fread_pet(CHAR_DATA * ch, FILE * fp);
static void fread_obj(CHAR_DATA * ch, FILE * fp);
static void fread_rdesc(ROOM_INDEX_DATA * location, FILE * fp);



/***************************************************************************
 *	print_flags
 ***************************************************************************/
char *print_flags(long flag)
{
    unsigned int count;
    int pos = 0;
    static char buf[52];

    for (count = 0; count < 32; count++) {
        if (IS_SET(flag, 1 << count)) {
            if (count < 26)
                buf[pos] = 'A' + (char)count;
            else
                buf[pos] = 'a' + (char)(count - 26);
            pos++;
        }
    }

    if (pos == 0) {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}



/***************************************************************************
 *	save_char_obj
 ***************************************************************************/
void save_char_obj(CHAR_DATA *ch)
{
    extern SKILL *gsp_deft;
    extern SKILL *gsp_dash;
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    if (IS_NPC(ch))
        return;

    if (ch->level == 1)
        return;

    if (ch->desc != NULL && ch->desc->original != NULL)
        ch = ch->desc->original;

    /** these don't want to be saved. */
    REMOVE_BIT(ch->act, PLR_LINKDEAD);
    affect_strip(ch, gsp_deft);
    affect_strip(ch, gsp_dash);


    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(ch->name));
    if ((fp = fopen(TEMP_FILE, "w")) == NULL) {
        log_bug("Save_char_obj: fopen");
        perror(strsave);
    } else {
        fwrite_char(ch, fp);
        if (ch->carrying != NULL)
            fwrite_obj(ch, ch->carrying, fp, 0);

        /* save the pets */
        if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
            fwrite_pet(ch->pet, fp);

        fprintf(fp, "#END\n");
    }
    fclose(fp);

    rename(TEMP_FILE, strsave);
    impnet("$N is saved.", ch, NULL, IMN_SAVES, 0, 0);
    return;
}



/***************************************************************************
 *	fwrite_char
 ***************************************************************************/
static void fwrite_char(CHAR_DATA *ch, FILE *fp)
{
    AFFECT_DATA *paf;
    LEARNED *learned;
    int pos;
    int idx;

    fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", ch->name);
    fprintf(fp, "Id   %ld\n", ch->id);
    fprintf(fp, "LogO %ld\n", (long)globalSystemState.current_time);

    if (ch->short_descr[0] != '\0')
        fprintf(fp, "ShD  %s~\n", ch->short_descr);

    if (ch->long_descr[0] != '\0')
        fprintf(fp, "LnD  %s~\n", ch->long_descr);

    if (ch->description[0] != '\0')
        fprintf(fp, "Desc %s~\n", ch->description);
    if (ch->prompt != NULL || !str_cmp(ch->prompt, "<%hhp %mm %vmv> "))
        fprintf(fp, "Prom %s~\n", ch->prompt);
    fprintf(fp, "Race %s~\n", pc_race_table[ch->race].name);

    fprintf(fp, "Sex  %d\n", ch->sex);
    fprintf(fp, "Cla  %d\n", ch->class);
    fprintf(fp, "Levl %d\n", ch->level);

    if (ch->trust != 0)
        fprintf(fp, "Tru  %d\n", ch->trust);
    if (ch->pcdata->security != 0)
        fprintf(fp, "Sec  %d\n", ch->pcdata->security);

    fprintf(fp, "Plyd %d\n", ch->played + (int)(globalSystemState.current_time - ch->logon));

    //for (idx = 0; message_type_table[idx].name[0] != '\0'; idx++) {
    //    fprintf(fp, "Msg '%s' %ld\n",
    //            message_type_table[idx].name,
    //            (long)ch->pcdata->last_read[message_type_table[idx].type]);
    //}

    fprintf(fp, "Scro %d\n", ch->lines);

    fprintf(fp, "Room %ld\n",
            (ch->in_room == get_room_index(ROOM_VNUM_LIMBO) && ch->was_in_room != NULL)
                ? ch->was_in_room->vnum
                : ch->in_room == NULL ? ROOM_VNUM_LIMBO : ch->in_room->vnum);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
            ch->hit, ch->max_hit, ch->mana,
            ch->max_mana, ch->move, ch->max_move);

    fprintf(fp, "Gold %u\n", ch->gold);
    fprintf(fp, "Silv %u\n", ch->silver);

    if (ch->pcdata->history != NULL && ch->pcdata->history[0] != '\0')
        fprintf(fp, "Hist %s~\n", ch->pcdata->history);

    if (ch->pcdata->killer_time != 0)
        fprintf(fp, "Killer_time %ld\n", (long)ch->pcdata->killer_time);

    if (ch->pcdata->thief_time != 0)
        fprintf(fp, "Thief_time %ld\n", (long)ch->pcdata->thief_time);

    if (ch->pcdata->last_bank != 0)
        fprintf(fp, "Last_bank %ld\n", (long)ch->pcdata->last_bank);

    fprintf(fp, "Silver_in_bank %u\n", ch->pcdata->silver_in_bank);
    fprintf(fp, "Gold_in_bank %u\n", ch->pcdata->gold_in_bank);

    fprintf(fp, "Exp  %d\n", ch->exp);
    if (ch->act != 0)
        fprintf(fp, "Act  %s\n", print_flags(ch->act));

    if (ch->affected_by != 0)
        fprintf(fp, "AfBy %s\n", print_flags(ch->affected_by));

    if (ch->mLag != 0)
        fprintf(fp, "MLag %d\n", ch->mLag);

    if (ch->tLag != 0)
        fprintf(fp, "TLag %d\n", ch->tLag);

    fprintf(fp, "Pkil %ld\n", ch->pcdata->pkills);
    fprintf(fp, "Pdea %ld\n", ch->pcdata->pdeaths);
    fprintf(fp, "Mkil %ld\n", ch->pcdata->mobkills);
    fprintf(fp, "Mdea %ld\n", ch->pcdata->mobdeaths);
    fprintf(fp, "ChannelsE %s\n", print_flags(ch->channels_enabled));
    fprintf(fp, "ChannelsD %s\n", print_flags(ch->channels_denied));
    fprintf(fp, "Comm %s\n", print_flags(ch->comm));
    fprintf(fp, "Dcry %s~\n", ch->pcdata->deathcry);

    if (ch->pcdata->wiznet)
        fprintf(fp, "Wizn %s\n", print_flags(ch->pcdata->wiznet));

    if (ch->pcdata->impnet)
        fprintf(fp, "Wizn2 %s\n", print_flags(ch->pcdata->impnet));

    if (ch->invis_level)
        fprintf(fp, "Invi %d\n", ch->invis_level);

    if (ch->incog_level)
        fprintf(fp, "Inco %d\n", ch->incog_level);

    fprintf(fp, "Pos  %d\n", ch->position == POS_FIGHTING ? POS_STANDING : ch->position);
    if (ch->pcdata->practice != 0)
        fprintf(fp, "Prac %d\n", ch->pcdata->practice);

    if (ch->pcdata->train != 0)
        fprintf(fp, "Trai %d\n", ch->pcdata->train);

    if (ch->saving_throw != 0)
        fprintf(fp, "Save  %d\n", ch->saving_throw);

    if (ch->hitroll != 0)
        fprintf(fp, "Hit   %d\n", ch->hitroll);

    if (ch->damroll != 0)
        fprintf(fp, "Dam   %d\n", ch->damroll);

    fprintf(fp, "ACs %ld %ld %ld %ld\n",
            ch->armor[0], ch->armor[1],
            ch->armor[2], ch->armor[3]);

    if (ch->wimpy != 0)
        fprintf(fp, "Wimp  %d\n", ch->wimpy);

    fprintf(fp, "Attr %d %d %d %d %d %d\n",
            ch->perm_stat[STAT_STR],
            ch->perm_stat[STAT_INT],
            ch->perm_stat[STAT_WIS],
            ch->perm_stat[STAT_DEX],
            ch->perm_stat[STAT_CON],
            ch->perm_stat[STAT_LUCK]);

    fprintf(fp, "AMod %d %d %d %d %d %d\n",
            ch->mod_stat[STAT_STR],
            ch->mod_stat[STAT_INT],
            ch->mod_stat[STAT_WIS],
            ch->mod_stat[STAT_DEX],
            ch->mod_stat[STAT_CON],
            ch->perm_stat[STAT_LUCK]);

    if (IS_NPC(ch)) {
        fprintf(fp, "Vnum %ld\n", ch->mob_idx->vnum);
    } else {
        fprintf(fp, "Pass %s~\n", ch->pcdata->pwd);

        if (ch->pcdata->bamfin[0] != '\0')
            fprintf(fp, "Bin  %s~\n", ch->pcdata->bamfin);

        if (ch->pcdata->bamfout[0] != '\0')
            fprintf(fp, "Bout %s~\n", ch->pcdata->bamfout);

        if (ch->pcdata->grestore_string[0] != '\0')
            fprintf(fp, "Gres  %s~\n", ch->pcdata->grestore_string);

        if (ch->pcdata->rrestore_string[0] != '\0')
            fprintf(fp, "Rres  %s~\n", ch->pcdata->rrestore_string);

        if (ch->pcdata->who_thing[0] != '\0')
            fprintf(fp, "Who  %s~\n", ch->pcdata->who_thing);

        fprintf(fp, "Titl %s~\n", ch->pcdata->title);
        fprintf(fp, "Pnts %d\n", ch->pcdata->points);
        fprintf(fp, "TSex %d\n", ch->pcdata->true_sex);
        fprintf(fp, "LLev %d\n", ch->pcdata->last_level);
        fprintf(fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit,
                ch->pcdata->perm_mana,
                ch->pcdata->perm_move);

        fprintf(fp, "Cnd  %ld %ld %ld %ld %ld\n",
                ch->pcdata->condition[0],
                ch->pcdata->condition[1],
                ch->pcdata->condition[2],
                ch->pcdata->condition[3],
                ch->pcdata->condition[4]);

        fprintf(fp, "Color %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                ch->use_ansi_color,
                (int)ch->pcdata->color_hp,
                (int)ch->pcdata->color_mana,
                (int)ch->pcdata->color_move,
                (int)ch->pcdata->color_combat_s,
                (int)ch->pcdata->color_combat_o,
                (int)ch->pcdata->color_combat_condition_s,
                (int)ch->pcdata->color_combat_condition_o,
                (int)ch->pcdata->color_invis,
                (int)ch->pcdata->color_wizi,
                (int)ch->pcdata->color_hidden,
                (int)ch->pcdata->color_charmed,
                (int)ch->pcdata->color_say,
                (int)ch->pcdata->color_tell,
                (int)ch->pcdata->color_reply);
        fprintf(fp, "Extended %u %ld\n",
                ch->pcdata->extendedlevel,
                ch->pcdata->extendedexp);

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++) {
            if (ch->pcdata->alias[pos] == NULL
                || ch->pcdata->alias_sub[pos] == NULL)
                break;

            fprintf(fp, "Alias %s %s~\n", ch->pcdata->alias[pos], ch->pcdata->alias_sub[pos]);
        }


        for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
            if (learned->skill != NULL && learned->type == LEARNED_TYPE_SKILL) {
                if (learned->percent > 0)
                    fprintf(fp, "Sk %d '%s'\n", learned->percent, learned->skill->name);

            } else if (learned->group != NULL && learned->type == LEARNED_TYPE_GROUP) {
                fprintf(fp, "Gr '%s'\n", learned->group->name);
            }
        }
    }




    for (paf = ch->affected; paf != NULL; paf = paf->next) {
        SKILL *skill;

        if (paf->type == -1) {
            fprintf(fp, "Affc 'reserved' %3d %3d %3d %3ld %3d %10ld\n",
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector);
        } else if ((skill = resolve_skill_sn(paf->type)) != NULL) {
            fprintf(fp, "Affc '%s' %3d %3d %3d %3ld %3d %10ld\n",
                    skill->name,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector);
        }
    }

    fprintf(fp, "End\n\n");
    return;
}


/***************************************************************************
 *	fwrite_pet
 ***************************************************************************/
static void fwrite_pet(CHAR_DATA *pet, FILE *fp)
{
    AFFECT_DATA *paf;

    if (!IS_NPC(pet))
        return;

    fprintf(fp, "#PET\n");
    fprintf(fp, "Vnum %ld\n", pet->mob_idx->vnum);
    fprintf(fp, "Name %s~\n", pet->name);
    fprintf(fp, "LogO %ld\n", (long)globalSystemState.current_time);

    if (pet->short_descr != pet->mob_idx->short_descr)
        fprintf(fp, "ShD  %s~\n", pet->short_descr);

    if (pet->long_descr != pet->mob_idx->long_descr)
        fprintf(fp, "LnD  %s~\n", pet->long_descr);

    if (pet->description != pet->mob_idx->description)
        fprintf(fp, "Desc %s~\n", pet->description);

    if (pet->race != pet->mob_idx->race)
        fprintf(fp, "Race %s~\n", race_table[pet->race].name);

    fprintf(fp, "Sex  %d\n", pet->sex);

    if (pet->level != pet->mob_idx->level)
        fprintf(fp, "Levl %d\n", pet->level);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
            pet->hit, pet->max_hit, pet->mana,
            pet->max_mana, pet->move, pet->max_move);

    if (pet->gold > 0)
        fprintf(fp, "Gold %u\n", pet->gold);

    if (pet->silver > 0)
        fprintf(fp, "Silv %u\n", pet->silver);

    if (pet->exp > 0)
        fprintf(fp, "Exp  %d\n", pet->exp);

    if (pet->act != pet->mob_idx->act)
        fprintf(fp, "Act  %s\n", print_flags(pet->act));

    if (pet->affected_by != pet->mob_idx->affected_by)
        fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));

    if (pet->comm != 0)
        fprintf(fp, "Comm %s\n", print_flags(pet->comm));

    fprintf(fp, "Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
        fprintf(fp, "Save %d\n", pet->saving_throw);

    if (pet->hitroll != pet->mob_idx->hitroll)
        fprintf(fp, "Hit  %d\n", pet->hitroll);

    if (pet->damroll != pet->mob_idx->damage[DICE_BONUS])
        fprintf(fp, "Dam  %d\n", pet->damroll);

    fprintf(fp, "ACs  %ld %ld %ld %ld\n",
            pet->armor[0], pet->armor[1],
            pet->armor[2], pet->armor[3]);

    fprintf(fp, "Attr %d %d %d %d %d %d\n",
            pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
            pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
            pet->perm_stat[STAT_CON], pet->perm_stat[STAT_LUCK]);
    fprintf(fp, "AMod %d %d %d %d %d %d\n",
            pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
            pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
            pet->mod_stat[STAT_CON], pet->mod_stat[STAT_LUCK]);

    for (paf = pet->affected; paf != NULL; paf = paf->next) {
        SKILL *skill;

        if (paf->type == -1) {
            fprintf(fp, "Affc 'reserved' %3d %3d %3d %3ld %3d %10ld\n",
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector);
        } else if ((skill = resolve_skill_sn(paf->type)) != NULL) {
            fprintf(fp, "Affc '%s' %3d %3d %3d %3ld %3d %10ld\n",
                    skill->name,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector);
        }
    }

    fprintf(fp, "End\n");
    return;
}

/***************************************************************************
 *	fwrite_obj
 ***************************************************************************/
static void fwrite_obj(CHAR_DATA *ch, GAMEOBJECT *obj, FILE *fp, int iNest)
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    SKILL *skill;


    if (obj->next_content != NULL)
        fwrite_obj(ch, obj->next_content, fp, iNest);

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %ld\n", obj->objprototype->vnum);
    if (obj->enchanted)
        fprintf(fp, "Enchanted\n");

    fprintf(fp, "Nest %d\n", iNest);


    if (obj->override_name != NULL)
        fprintf(fp, "Name %s~\n", obj->override_name);

    if (obj->short_descr != obj->objprototype->short_descr)
        fprintf(fp, "ShD  %s~\n", obj->short_descr);

    if (obj->description != obj->objprototype->description)
        fprintf(fp, "Desc %s~\n", obj->description);

    if (obj->extra_flags != obj->objprototype->extra_flags)
        fprintf(fp, "ExtF %ld\n", obj->extra_flags);

    if (obj->extra2_flags != obj->objprototype->extra2_flags)
        fprintf(fp, "Ex2F %ld\n", obj->extra2_flags);

    if (obj->wear_flags != obj->objprototype->wear_flags)
        fprintf(fp, "WeaF %ld\n", obj->wear_flags);

    if (obj->item_type != obj->objprototype->item_type)
        fprintf(fp, "Ityp %d\n", obj->item_type);

    if (obj->weight != obj->objprototype->weight)
        fprintf(fp, "Wt   %d\n", obj->weight);

    if (obj->condition != obj->objprototype->condition)
        fprintf(fp, "Cond %d\n", obj->condition);

    /* variable data */
    fprintf(fp, "Wear %d\n", obj->wear_loc);

    if (obj->plevel > 0)
        fprintf(fp, "Plev %d\n", obj->plevel);
    if (obj->exp > 0)
        fprintf(fp, "Exp %d\n", obj->exp);
    if (obj->xp_tolevel > 0)
        fprintf(fp, "Xptolevel %d\n", obj->xp_tolevel);

    if (obj->level != obj->objprototype->level)
        fprintf(fp, "Lev  %d\n", obj->level);

    if (obj->timer != 0)
        fprintf(fp, "Time %d\n", obj->timer);

    fprintf(fp, "Cost %u\n", obj->cost);
    if (obj->value[0] != obj->objprototype->value[0]
        || obj->value[1] != obj->objprototype->value[1]
        || obj->value[2] != obj->objprototype->value[2]
        || obj->value[3] != obj->objprototype->value[3]
        || obj->value[4] != obj->objprototype->value[4]) {
        fprintf(fp, "Val  %ld %ld %ld %ld %ld\n",
                obj->value[0], obj->value[1], obj->value[2],
                obj->value[3], obj->value[4]);
    }


    switch (obj->item_type) {
      case ITEM_POTION:
      case ITEM_SCROLL:
          {
              int idx;

              for (idx = 1; idx <= 3; idx++) {
                  if (obj->value[idx] > 0
                      && (skill = resolve_skill_sn((int)obj->value[idx])) != NULL)
                      fprintf(fp, "Spell %d '%s'\n", idx, skill->name);
              }
          }
          break;

      case ITEM_PILL:
      case ITEM_STAFF:
      case ITEM_WAND:
          if (obj->value[3] > 0) {
              if ((skill = resolve_skill_sn((int)obj->value[3])) != NULL)
                  fprintf(fp, "Spell 3 '%s'\n", skill->name);
          }

          break;
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next) {
        if (paf->type <= 0) {
            fprintf(fp, "Affc 'reserved' %3d %3d %3d %3ld %3d %10ld\n",
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector);
        } else if ((skill = resolve_skill_affect(paf)) != NULL) {
            fprintf(fp, "Affc '%s' %3d %3d %3d %3ld %3d %10ld\n",
                    skill->name,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector);
        }
    }

    for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
        fprintf(fp, "ExDe %s~ %s~\n", ed->keyword, ed->description);

    fprintf(fp, "End\n\n");

    if (obj->contains != NULL)
        fwrite_obj(ch, obj->contains, fp, iNest + 1);

    return;
}



/***************************************************************************
 *	load_char_obj
 ***************************************************************************/
bool load_char_obj(struct descriptor_data *d, char *name)
{
    CHAR_DATA *ch;
    FILE *fp;
    LEARNED *learned;
    char strsave[MAX_INPUT_LENGTH];
    bool found;
    int stat;

    char buf[100];

    ch = new_char();
    ch->pcdata = new_pcdata();

    d->character = ch;
    ch->desc = d;
    ch->name = str_dup(name);
    ch->id = get_pc_id();
    ch->race = race_lookup("human");
    ch->act = PLR_NOSUMMON;
    ch->channels_enabled = 0;
    ch->channels_denied = 0;
    ch->comm = COMM_COMBINE | COMM_PROMPT;
    ch->comm = 0;
    ch->prompt = str_dup("<%hhp %mm %vmv> ");
    ch->mLag = 0;
    ch->tLag = 0;

    ch->pcdata->confirm_delete = false;
    ch->pcdata->confirm_suicide = false;

    ch->pcdata->pwd = str_dup("");
    ch->pcdata->title = str_dup("");
    ch->pcdata->bamfin = str_dup("");
    ch->pcdata->bamfout = str_dup("");
    ch->pcdata->history = str_dup("");

    ch->pcdata->grestore_string = str_dup("");
    ch->pcdata->rrestore_string = str_dup("");
    ch->pcdata->who_thing = str_dup("");

    ch->pcdata->restring_name = str_dup("");
    ch->pcdata->restring_short = str_dup("");
    ch->pcdata->restring_long = str_dup("");

    ch->linked = NULL;

    for (stat = 0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat] = 13;

    ch->perm_stat[STAT_LUCK] = 13;
    ch->pcdata->condition[COND_THIRST] = 98;
    ch->pcdata->condition[COND_FULL] = 98;
    ch->pcdata->condition[COND_HUNGER] = 98;
    ch->pcdata->condition[COND_FEED] = 98;
    ch->pcdata->security = 0;

    ch->pcdata->silver_in_bank = 0;
    ch->pcdata->gold_in_bank = 0;
    ch->pcdata->prefix = str_dup("");
    ch->pcdata->last_bank = 0;
    ch->pcdata->killer_time = 0;
    ch->pcdata->thief_time = 0;

    ch->deathroom = 0;
    found = false;

    ch->pcdata->color_combat_condition_s = (byte)0x1;
    ch->pcdata->color_combat_s = (byte)0x2;
    ch->pcdata->color_invis = (byte)0x3;
    ch->pcdata->color_wizi = (byte)0x4;
    ch->pcdata->color_hp = (byte)0x5;
    ch->pcdata->color_combat_condition_o = (byte)0x6;
    ch->pcdata->color_combat_o = (byte)0x7;
    ch->pcdata->color_hidden = (byte)0x8;
    ch->pcdata->color_charmed = (byte)0x9;
    ch->pcdata->color_mana = (byte)0xa;
    ch->pcdata->color_move = (byte)0xb;
    ch->pcdata->color_say = (byte)0xc;
    ch->pcdata->color_tell = (byte)0x0;
    ch->pcdata->color_reply = (byte)0x1;

    sprintf(strsave, "%s%s%s", PLAYER_DIR, capitalize(name), ".gz");
    if ((fp = fopen(strsave, "r")) != NULL) {
        fclose(fp);
        sprintf(buf, "gzip -dfq %s", strsave);
        system(buf);
    }

    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(name));
    if ((fp = fopen(strsave, "r")) != NULL) {
        int iNest;

        for (iNest = 0; iNest < MAX_NEST; iNest++)
            rgObjNest[iNest] = NULL;

        found = true;
        for (;; ) {
            char *word;
            char letter;

            letter = fread_letter(fp);
            if (letter == '*') {
                fread_to_eol(fp);
                continue;
            }

            if (letter != '#') {
                log_bug("Load_char_obj: # not found.");
                break;
            }

            word = fread_word(fp);
            if (!str_cmp(word, "PLAYER")) {
                fread_char(ch, fp);
            } else if (!str_cmp(word, "OBJECT")) {
                fread_obj(ch, fp);
            } else if (!str_cmp(word, "O")) {
                fread_obj(ch, fp);
            } else if (!str_cmp(word, "PET")) {
                fread_pet(ch, fp);
            } else if (!str_cmp(word, "END")) {
                break;
            } else {
                log_bug("Load_char_obj: bad section.");
                break;
            }
        }

        fclose(fp);
    }

    /* initialize race */
    if (found) {
        int idx;

        if (ch->race == 0)
            ch->race = race_lookup("human");

        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = 17;      /*punch */

        for (idx = 0; idx < 5; idx++) {
            if (pc_race_table[ch->race].skills[idx] == NULL)
                break;


            if ((learned = get_learned(ch, pc_race_table[ch->race].skills[idx])) == NULL) {
                if ((learned = create_learned_skill(pc_race_table[ch->race].skills[idx], 1)) != NULL) {
                    add_learned_skill(ch, learned);
                } else {
                    if ((learned = create_learned_group(pc_race_table[ch->race].skills[idx])) != NULL)
                        add_learned_group(ch, learned);
                }
            }
        }

        ch->affected_by = ch->affected_by | race_table[ch->race].aff;
        ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
        ch->form = race_table[ch->race].form;
        ch->parts = race_table[ch->race].parts;
    }

    return found;
}



/***************************************************************************
 *	KEY
 ***************************************************************************/
#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)           \
    if (!str_cmp(word, literal))             \
{                                        \
    field = value;                       \
    fMatch = true;                       \
    break;                               \
}


/**************************************************************************
 *	KEYS
 ***************************************************************************/
/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS(literal, field, value)         \
    if (!str_cmp(word, literal))            \
{                                       \
    free_string(field);                 \
    field = value;                      \
    fMatch = true;                      \
    log_bug(word);                      \
    break;                              \
}

/***************************************************************************
 *	fread_char
 ***************************************************************************/
void fread_char(CHAR_DATA *ch, FILE *fp)
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    int count = 0;
    time_t lastlogoff = globalSystemState.current_time;

    log_string("Loading %s.", ch->name);

    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
          case '*':
              fMatch = true;
              fread_to_eol(fp);
              break;

          case 'A':
              KEY("Act", ch->act, fread_flag(fp));
              if (IS_SET(ch->act, ACT_IS_NPC));
              {
                  REMOVE_BIT(ch->act, ACT_IS_NPC);
              }

              KEY("AffectedBy", ch->affected_by, fread_flag(fp));
              KEY("AfBy", ch->affected_by, fread_flag(fp));
              if (!str_cmp(word, "AucNot")) {
                  KEY("AucNot", ch->pcdata->last_aucnote, (time_t)fread_long(fp));
                  ch->pcdata->last_read[NOTE_AUCNOTE] = ch->pcdata->last_aucnote;
              }

              if (!str_cmp(word, "Alia")) {
                  if (count >= MAX_ALIAS) {
                      fread_to_eol(fp);
                      fMatch = true;
                      break;
                  }

                  ch->pcdata->alias[count] = str_dup(fread_word(fp));
                  ch->pcdata->alias_sub[count] = str_dup(fread_word(fp));
                  count++;
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "Alias")) {
                  if (count >= MAX_ALIAS) {
                      fread_to_eol(fp);
                      fMatch = true;
                      break;
                  }
                  ch->pcdata->alias[count] = str_dup(fread_word(fp));
                  ch->pcdata->alias_sub[count] = fread_string(fp);
                  count++;
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "AC") || !str_cmp(word, "Armor")) {
                  fread_to_eol(fp);
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "ACs")) {
                  int i;

                  for (i = 0; i < 4; i++)
                      ch->armor[i] = fread_number(fp);

                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "AffD")) {
                  AFFECT_DATA *paf;
                  SKILL *skill;
                  char *affd;

                  paf = new_affect();
                  affd = fread_word(fp);

                  if (!str_cmp("reserved", affd)) {
                      paf->type = -1;
                  } else {
                      if ((skill = skill_lookup(affd)) == NULL)
                          log_bug("Fread_char (Affd): unknown skill: %s.", affd);
                      else
                          paf->type = skill->sn;
                  }

                  paf->level = fread_number(fp);
                  paf->duration = fread_number(fp);
                  paf->modifier = fread_number(fp);
                  paf->location = fread_number(fp);
                  paf->bitvector = fread_number(fp);

                  paf->next = ch->affected;
                  ch->affected = paf;
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "Affc")) {
                  AFFECT_DATA *paf;
                  SKILL *skill;
                  char *affc;

                  paf = new_affect();
                  affc = fread_word(fp);
                  if (!str_cmp("reserved", affc)) {
                      paf->type = -1;
                  } else {
                      if ((skill = skill_lookup(affc)) == NULL)
                          log_bug("Fread_char (Affc): unknown skill: %s.", affc);
                      else
                          paf->type = skill->sn;
                  }

                  paf->where = fread_number(fp);
                  paf->level = fread_number(fp);
                  paf->duration = fread_number(fp);
                  paf->modifier = fread_number(fp);
                  paf->location = fread_number(fp);
                  paf->bitvector = fread_number(fp);
                  paf->next = ch->affected;
                  ch->affected = paf;
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "AttrMod") || !str_cmp(word, "AMod")) {
                  int stat;

                  for (stat = 0; stat < MAX_STATS; stat++)
                      ch->mod_stat[stat] = fread_number(fp);

                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "AttrPerm") || !str_cmp(word, "Attr")) {
                  int stat;

                  for (stat = 0; stat < MAX_STATS; stat++)
                      ch->perm_stat[stat] = fread_number(fp);
                  fMatch = true;
                  break;
              }
              break;

          case 'B':
              KEY("Bin", ch->pcdata->bamfin, fread_string(fp));
              KEY("Bout", ch->pcdata->bamfout, fread_string(fp));
              break;

          case 'C':
              KEY("Class", ch->class, fread_number(fp));
              KEY("Cla", ch->class, fread_number(fp));

              if (!str_cmp(word, "Condition") || !str_cmp(word, "Cond")) {
                  ch->pcdata->condition[0] = fread_number(fp);
                  ch->pcdata->condition[1] = fread_number(fp);
                  ch->pcdata->condition[2] = fread_number(fp);
                  ch->pcdata->condition[3] = fread_number(fp);
                  fMatch = true;
                  break;
              }

              KEY("ChannelsE", ch->channels_enabled, fread_flag(fp));
              KEY("ChannelsD", ch->channels_denied, fread_flag(fp));

              if (!str_cmp(word, "Cnd")) {
                  ch->pcdata->condition[0] = fread_number(fp);
                  ch->pcdata->condition[1] = fread_number(fp);
                  ch->pcdata->condition[2] = fread_number(fp);
                  ch->pcdata->condition[3] = fread_number(fp);
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "Color")) {
                  log_string("Reading colors");
                  ch->use_ansi_color = fread_number(fp);
                  ch->pcdata->color_hp = (byte)fread_number(fp);
                  ch->pcdata->color_mana = (byte)fread_number(fp);
                  ch->pcdata->color_move = (byte)fread_number(fp);
                  ch->pcdata->color_combat_s = (byte)fread_number(fp);
                  ch->pcdata->color_combat_o = (byte)fread_number(fp);
                  ch->pcdata->color_combat_condition_s = (byte)fread_number(fp);
                  ch->pcdata->color_combat_condition_o = (byte)fread_number(fp);
                  ch->pcdata->color_invis = (byte)fread_number(fp);
                  ch->pcdata->color_wizi = (byte)fread_number(fp);
                  ch->pcdata->color_hidden = (byte)fread_number(fp);
                  ch->pcdata->color_charmed = (byte)fread_number(fp);
                  ch->pcdata->color_say = (byte)fread_number(fp);
                  ch->pcdata->color_tell = (byte)fread_number(fp);
                  ch->pcdata->color_reply = (byte)fread_number(fp);
                  fMatch = true;
                  break;
              }

              KEY("Comm", ch->comm, fread_flag(fp));
              break;

          case 'D':
              KEY("Damroll", ch->damroll, fread_number(fp));
              KEY("Dam", ch->damroll, fread_number(fp));
              KEY("Dcry", ch->pcdata->deathcry, fread_string(fp));
              KEY("Description", ch->description, fread_string(fp));
              KEY("Desc", ch->description, fread_string(fp));
              break;

          case 'E':
              if (!str_cmp(word, "End")) {
                  return;
              }

              if (!str_cmp(word, "Extended")) {
                  ch->pcdata->extendedlevel = fread_uint(fp);
                  ch->pcdata->extendedexp = fread_number(fp);
                  fMatch = true;
                  break;
              }

              KEY("Exp", ch->exp, fread_number(fp));
              break;


          case 'F':
              break;

          case 'G':
              KEY("Gold_in_bank", ch->pcdata->gold_in_bank, fread_uint(fp));
              KEY("Gold", ch->gold, fread_uint(fp));
              KEY("Grestore_string", ch->pcdata->grestore_string, fread_string(fp));
              KEY("Gres", ch->pcdata->grestore_string, fread_string(fp));
              if (!str_cmp(word, "Group") || !str_cmp(word, "Gr")) {
                  GROUP *group;
                  LEARNED *learned;
                  char *temp;

                  temp = fread_word(fp);

                  group = group_lookup(temp);
                  if (group != NULL) {
                      learned = get_learned_group(ch, group);
                      if (learned == NULL) {
                          learned = new_learned();
                          learned->group = group;
                          learned->type = LEARNED_TYPE_GROUP;
                          add_learned(ch, learned);
                      }
                  }

                  fMatch = true;
              }
              break;

          case 'H':
              KEY("Hitroll", ch->hitroll, fread_number(fp));
              KEY("Hit", ch->hitroll, fread_number(fp));
              KEYS("Hist", ch->pcdata->history, fread_string(fp));

              if (!str_cmp(word, "HpManaMove") || !str_cmp(word, "HMV")) {
                  ch->hit = fread_number(fp);
                  ch->max_hit = fread_number(fp);
                  ch->mana = fread_number(fp);
                  ch->max_mana = fread_number(fp);
                  ch->move = fread_number(fp);
                  ch->max_move = fread_number(fp);
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "HpManaMovePerm") || !str_cmp(word, "HMVP")) {
                  ch->pcdata->perm_hit = fread_number(fp);
                  ch->pcdata->perm_mana = fread_number(fp);
                  ch->pcdata->perm_move = fread_number(fp);
                  fMatch = true;
                  break;
              }
              break;

          case 'I':
              KEY("Id", ch->id, fread_number(fp));
              KEY("InvisLevel", ch->invis_level, fread_number(fp));
              KEY("Inco", ch->incog_level, fread_number(fp));
              KEY("Invi", ch->invis_level, fread_number(fp));
              break;

          case 'J':
              break;

          case 'K':
              KEY("Killer_time", ch->pcdata->killer_time, (time_t)fread_long(fp));
              break;

          case 'L':
              KEY("LastLevel", ch->pcdata->last_level, fread_number(fp));
              KEY("LLev", ch->pcdata->last_level, fread_number(fp));
              KEY("Level", ch->level, fread_number(fp));
              KEY("Lev", ch->level, fread_number(fp));
              KEY("Levl", ch->level, fread_number(fp));
              if (!str_cmp(word, "LogO")) {
                  lastlogoff = (time_t)fread_long(fp);
                  ch->llogoff = lastlogoff;
                  fMatch = true;
                  break;
              }
              KEY("LongDescr", ch->long_descr, fread_string(fp));
              KEY("LnD", ch->long_descr, fread_string(fp));
              KEY("Last_bank", ch->pcdata->last_bank, (time_t)fread_long(fp));
              break;

          case 'M':
              KEY("MLag", ch->mLag, fread_number(fp));
              KEY("Mkil", ch->pcdata->mobkills, fread_number(fp));
              KEY("Mdea", ch->pcdata->mobdeaths, fread_number(fp));
              if (!str_cmp(word, "Msg")) {
                  char *thread;
                  int type;
                  time_t msg_tm;

                  thread = fread_word(fp);
                  msg_tm = (time_t)fread_long(fp);
                  type = message_type_lookup(thread);
                  if (type > -1 && type < NOTE_MAX) {
                      ch->pcdata->last_read[type] = msg_tm;
                      fMatch = true;
                  }
                  break;
              }

          case 'N':
              KEY("Name", ch->name, fread_string(fp));
              KEY("Note", ch->pcdata->last_note, (time_t)fread_long(fp));
              if (!str_cmp(word, "Not")) {
                  ch->pcdata->last_note = (time_t)fread_long(fp);
                  ch->pcdata->last_penalty = (time_t)fread_long(fp);
                  ch->pcdata->last_news = (time_t)fread_long(fp);
                  ch->pcdata->last_changes = (time_t)fread_long(fp);
                  ch->pcdata->last_idea = (time_t)fread_long(fp);
                  ch->pcdata->last_build = (time_t)fread_long(fp);
                  fMatch = true;

                  /* conversion hack */
                  ch->pcdata->last_read[NOTE_NOTE] = ch->pcdata->last_note;
                  ch->pcdata->last_read[NOTE_PENALTY] = ch->pcdata->last_penalty;
                  ch->pcdata->last_read[NOTE_NEWS] = ch->pcdata->last_news;
                  ch->pcdata->last_read[NOTE_CHANGES] = ch->pcdata->last_changes;
                  ch->pcdata->last_read[NOTE_IDEA] = ch->pcdata->last_idea;
                  ch->pcdata->last_read[NOTE_BUILD] = ch->pcdata->last_build;
                  break;
              }
              break;

          case 'P':
              KEY("Password", ch->pcdata->pwd, fread_string(fp));
              KEY("Pass", ch->pcdata->pwd, fread_string(fp));
              KEY("Pkil", ch->pcdata->pkills, fread_number(fp));
              KEY("Pdea", ch->pcdata->pdeaths, fread_number(fp));
              KEY("Played", ch->played, fread_number(fp));
              KEY("Plyd", ch->played, fread_number(fp));
              KEY("Points", ch->pcdata->points, fread_number(fp));
              KEY("Pnts", ch->pcdata->points, fread_number(fp));
              KEY("Position", ch->position, fread_number(fp));
              KEY("Pos", ch->position, fread_number(fp));
              KEY("Practice", ch->pcdata->practice, fread_number(fp));
              KEY("Prac", ch->pcdata->practice, fread_number(fp));
              KEY("Prompt", ch->prompt, fread_string(fp));
              KEY("Prom", ch->prompt, fread_string(fp));
              break;

          case 'R':
              KEY("Rrestore_string", ch->pcdata->rrestore_string, fread_string(fp));
              KEY("Rres", ch->pcdata->rrestore_string, fread_string(fp));
              KEY("Race", ch->race, race_lookup(fread_string(fp)));
              KEY("Rank0", ch->pcdata->rank, fread_number(fp));
              if (!str_cmp(word, "Room")) {
                  ch->in_room = get_room_index(fread_number(fp));
                  if (ch->in_room == NULL)
                      ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
                  fMatch = true;
                  break;
              }
              if (!str_cmp(word, "RPNot")) {
                  KEY("RPNot", ch->pcdata->last_rpnote, (time_t)fread_long(fp));
                  ch->pcdata->last_read[NOTE_RPNOTE] = ch->pcdata->last_rpnote;
              }
              break;

          case 'S':
              KEY("SavingThrow", ch->saving_throw, fread_number(fp));
              KEY("Save", ch->saving_throw, fread_number(fp));
              KEY("Scro", ch->lines, fread_number(fp));
              KEY("Sec", ch->pcdata->security, fread_number(fp));
              KEY("Sex", ch->sex, fread_number(fp));
              KEY("ShortDescr", ch->short_descr, fread_string(fp));
              KEY("ShD", ch->short_descr, fread_string(fp));
              KEY("Silver_in_bank", ch->pcdata->silver_in_bank, fread_uint(fp));
              KEY("Silv", ch->silver, fread_uint(fp));


              if (!str_cmp(word, "Skill") || !str_cmp(word, "Sk")) {
                  SKILL *skill;
                  LEARNED *learned;
                  char *temp;
                  int value;

                  value = fread_number(fp);
                  temp = fread_word(fp);

                  skill = skill_lookup(temp);
                  if (skill != NULL) {
                      learned = get_learned_skill(ch, skill);
                      if (learned == NULL) {
                          learned = new_learned();
                          learned->skill = skill;
                          learned->percent = value;
                          learned->type = LEARNED_TYPE_SKILL;
                          add_learned(ch, learned);
                      }
                  }

                  fMatch = true;
              }

              break;

          case 'T':
              KEY("TLag", ch->tLag, fread_number(fp));
              KEY("Thief_time", ch->pcdata->thief_time, (time_t)fread_long(fp));
              KEY("TrueSex", ch->pcdata->true_sex, fread_number(fp));
              KEY("TSex", ch->pcdata->true_sex, fread_number(fp));
              KEY("Trai", ch->pcdata->train, fread_number(fp));
              KEY("Trust", ch->trust, fread_number(fp));
              KEY("Tru", ch->trust, fread_number(fp));

              if (!str_cmp(word, "Title") || !str_cmp(word, "Titl")) {
                  ch->pcdata->title = fread_string(fp);
                  if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ','
                      && ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?'
                      && ch->pcdata->title[0] != ';') {
                      sprintf(buf, " %s", ch->pcdata->title);
                      free_string(ch->pcdata->title);
                      ch->pcdata->title = str_dup(buf);
                  }

                  fMatch = true;
                  break;
              }

              break;
          case 'V':
              if (!str_cmp(word, "Vnum")) {
                  ch->mob_idx = get_mob_index(fread_number(fp));
                  fMatch = true;
                  break;
              }
              break;

          case 'W':
              KEY("Wimpy", ch->wimpy, fread_number(fp));
              KEY("Wimp", ch->wimpy, fread_number(fp));
              KEY("Wizn", ch->pcdata->wiznet, fread_flag(fp));
              KEY("Wizn2", ch->pcdata->impnet, fread_flag(fp));
              KEY("Who", ch->pcdata->who_thing, fread_string(fp));
              break;
        }

        if (!fMatch) {
            log_bug("Fread_char: no match.");
            fread_to_eol(fp);
        }
    }
}


/***************************************************************************
 *	fread_pet
 ***************************************************************************/
static void fread_pet(CHAR_DATA *ch, FILE *fp)
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;

    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word, "Vnum")) {
        long vnum;

        vnum = fread_number(fp);
        if (get_mob_index(vnum) == NULL) {
            log_bug("Fread_pet: bad vnum %d.", vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        } else {
            pet = create_mobile(get_mob_index(vnum));
        }
    } else {
        log_bug("Fread_pet: no vnum in file.");
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }

    for (;; ) {
        word = feof(fp) ? "END" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
          case '*':
              fMatch = true;
              fread_to_eol(fp);
              break;

          case 'A':
              KEY("Act", pet->act, fread_flag(fp));
              KEY("AfBy", pet->affected_by, fread_flag(fp));

              if (!str_cmp(word, "ACs")) {
                  int i;

                  for (i = 0; i < 4; i++)
                      pet->armor[i] = fread_number(fp);
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "AffD")) {
                  AFFECT_DATA *paf;
                  SKILL *skill;
                  char *affd;

                  paf = new_affect();
                  affd = fread_word(fp);
                  if (!str_cmp("reserved", affd)) {
                      paf->type = -1;
                  } else {
                      if ((skill = skill_lookup(affd)) == NULL)
                          log_bug("Fread_char: unknown skill.");
                      else
                          paf->type = skill->sn;
                  }

                  paf->level = fread_number(fp);
                  paf->duration = fread_number(fp);
                  paf->modifier = fread_number(fp);
                  paf->location = fread_number(fp);
                  paf->bitvector = fread_number(fp);
                  paf->next = pet->affected;
                  pet->affected = paf;

                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "Affc")) {
                  AFFECT_DATA *paf;
                  SKILL *skill;
                  char *affc;

                  affc = fread_word(fp);
                  paf = new_affect();
                  if (!str_cmp("reserved", affc)) {
                      paf->type = -1;
                  } else {
                      if ((skill = skill_lookup(affc)) == NULL)
                          log_bug("Fread_char: unknown skill.");
                      else

                          paf->type = skill->sn;
                  }

                  paf->where = fread_number(fp);
                  paf->level = fread_number(fp);
                  paf->duration = fread_number(fp);
                  paf->modifier = fread_number(fp);
                  paf->location = fread_number(fp);
                  paf->bitvector = fread_number(fp);
                  paf->next = pet->affected;
                  pet->affected = paf;

                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "AMod")) {
                  int stat;

                  for (stat = 0; stat < MAX_STATS; stat++)
                      pet->mod_stat[stat] = fread_number(fp);
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "Attr")) {
                  int stat;

                  for (stat = 0; stat < MAX_STATS; stat++)
                      pet->perm_stat[stat] = fread_number(fp);
                  fMatch = true;
                  break;
              }
              break;

          case 'C':
              KEY("Comm", pet->comm, fread_flag(fp));
              break;

          case 'D':
              KEY("Dam", pet->damroll, fread_number(fp));
              KEY("Desc", pet->description, fread_string(fp));
              break;

          case 'E':
              if (!str_cmp(word, "End")) {
                  pet->leader = ch;
                  pet->master = ch;
                  ch->pet = pet;
                  return;
              }
              KEY("Exp", pet->exp, fread_number(fp));
              break;

          case 'G':
              KEY("Gold", pet->gold, fread_uint(fp));
              break;

          case 'H':
              KEY("Hit", pet->hitroll, fread_number(fp));

              if (!str_cmp(word, "HMV")) {
                  pet->hit = fread_number(fp);
                  pet->max_hit = fread_number(fp);
                  pet->mana = fread_number(fp);
                  pet->max_mana = fread_number(fp);
                  pet->move = fread_number(fp);
                  pet->max_move = fread_number(fp);
                  fMatch = true;
                  break;
              }
              break;

          case 'L':
              KEY("Levl", pet->level, fread_number(fp));
              KEY("LnD", pet->long_descr, fread_string(fp));
              //KEY("LogO", lastlogoff, (time_t)fread_long(fp));
              break;

          case 'N':
              KEY("Name", pet->name, fread_string(fp));
              break;

          case 'P':
              KEY("Pos", pet->position, fread_number(fp));
              break;

          case 'R':
              KEY("Race", pet->race, race_lookup(fread_string(fp)));
              break;

          case 'S':
              KEY("Save", pet->saving_throw, fread_number(fp));
              KEY("Sex", pet->sex, fread_number(fp));
              KEY("ShD", pet->short_descr, fread_string(fp));
              KEY("Silv", pet->silver, fread_uint(fp));
              break;
        }

        if (!fMatch) {
            log_bug("Fread_pet: no match.");
            fread_to_eol(fp);
        }
    }
}



/***************************************************************************
 *	fread_obj
 ***************************************************************************/
static void fread_obj(CHAR_DATA *ch, FILE *fp)
{
    GAMEOBJECT *obj;
    char *word;
    int iNest = 0;
    bool fMatch = false;
    long vnum; 

    word = feof(fp) ? "End" : fread_word(fp);
    if (str_cmp(word, "Vnum") != 0) {
        int lines_skipped;

        lines_skipped = 0;
        for (;;) {
            word = feof(fp) ? "End" : fread_word(fp);
            if (str_cmp(word, "End") != 0)
                break;
        }
        log_bug("Bad object format starting with word, %s. Skipping %d lines.", word, lines_skipped);
        return;
    }

    vnum = fread_number(fp);
    if (objectprototype_getbyvnum(vnum) == NULL) {
        int lines_skipped;

        lines_skipped = 0;
        for (;;) {
            word = feof(fp) ? "End" : fread_word(fp);
            if (str_cmp(word, "End") != 0)
                break;
        }
        log_bug("Bad vnum %ld. Skipping %d lines.", vnum, lines_skipped);
        return;
    }

    obj = create_object(objectprototype_getbyvnum(vnum), -1);
    assert(obj != NULL);
    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
          case '*':
              fMatch = true;
              fread_to_eol(fp);
              break;

          case 'A':
              if (!str_cmp(word, "AffD")) {
                  AFFECT_DATA *paf;
                  SKILL *skill;
                  char *affd;

                  paf = new_affect();
                  affd = fread_word(fp);
                  if (!str_cmp("reserved", affd)) {
                      paf->type = -1;
                  } else {
                      if ((skill = skill_lookup(affd)) == NULL)
                          log_bug("fread_obj: unknown skill.");
                      else
                          paf->type = skill->sn;
                  }

                  paf->level = fread_number(fp);
                  paf->duration = fread_number(fp);
                  paf->modifier = fread_number(fp);
                  paf->location = fread_number(fp);
                  paf->bitvector = fread_number(fp);
                  paf->next = obj->affected;
                  obj->affected = paf;
                  fMatch = true;
                  break;
              }
              if (!str_cmp(word, "Affc")) {
                  AFFECT_DATA *paf;
                  SKILL *skill;
                  char *affc;

                  paf = new_affect();
                  affc = fread_word(fp);
                  if (!str_cmp("reserved", affc)) {
                      paf->type = -1;
                  } else {
                      if ((skill = skill_lookup(affc)) == NULL)
                          log_bug("Fread_obj: unknown skill.");
                      else
                          paf->type = skill->sn;
                  }

                  paf->where = fread_number(fp);
                  paf->level = fread_number(fp);
                  paf->duration = fread_number(fp);
                  paf->modifier = fread_number(fp);
                  paf->location = fread_number(fp);
                  paf->bitvector = fread_number(fp);
                  paf->next = obj->affected;
                  obj->affected = paf;
                  fMatch = true;
                  break;
              }
              break;

          case 'C':
              KEY("Cond", obj->condition, fread_number(fp));
              KEY("Cost", obj->cost, fread_uint(fp));
              break;

          case 'D':
              KEY("Description", obj->description, fread_string(fp));
              KEY("Desc", obj->description, fread_string(fp));
              break;

          case 'E':
              if (!str_cmp(word, "Enchanted")) {
                  obj->enchanted = true;
                  fMatch = true;
                  break;
              }

              KEY("ExtraFlags", obj->extra_flags, fread_number(fp));
              KEY("Exp", obj->exp, fread_number(fp));
              KEY("ExtF", obj->extra_flags, fread_number(fp));

              KEY("Ex2F", obj->extra2_flags, fread_number(fp));

              if (!str_cmp(word, "ExtraDescr") || !str_cmp(word, "ExDe")) {
                  EXTRA_DESCR_DATA *ed;

                  ed = new_extra_descr();

                  ed->keyword = fread_string(fp);
                  ed->description = fread_string(fp);
                  ed->next = obj->extra_descr;
                  obj->extra_descr = ed;
                  fMatch = true;
              }

              if (!str_cmp(word, "End")) {
                  if (iNest == 0 || rgObjNest[iNest] == NULL) {
                      obj_to_char(obj, ch);
                      if (obj->wear_loc != WEAR_NONE) {
                          int wear;
                          wear = obj->wear_loc;
                          unequip_char(ch, obj);
                          equip_char(ch, obj, wear);
                      }
                  } else {
                      obj_to_obj(obj, rgObjNest[iNest - 1]);
                  }

                  return;
              }
              break;

          case 'I':
              KEY("ItemType", obj->item_type, fread_number(fp));
              KEY("Ityp", obj->item_type, fread_number(fp));
              break;

          case 'L':
              KEY("Level", obj->level, fread_number(fp));
              KEY("Lev", obj->level, fread_number(fp));
              break;

          case 'N':
              KEY("Name", obj->override_name, fread_string(fp));

              if (!str_cmp(word, "Nest")) {
                  iNest = fread_number(fp);
                  if (iNest < 0 || iNest >= MAX_NEST) {
                      log_bug("Fread_obj: bad nest %d.", iNest);
                  } else {
                      rgObjNest[iNest] = obj;
                  }
                  fMatch = true;
              }
              break;

          case 'P':
              KEY("Plevel", obj->plevel, fread_number(fp));
              break;


          case 'S':
              KEY("ShortDescr", obj->short_descr, fread_string(fp));
              KEY("ShD", obj->short_descr, fread_string(fp));

              if (!str_cmp(word, "Spell")) {
                  SKILL *skill;
                  int value;

                  value = fread_number(fp);
                  if (value < 0 || value > 3)
                      log_bug("fread_obj: bad iValue %d.", value);
                  else if ((skill = skill_lookup(fread_word(fp))) == NULL)
                      log_bug("fread_obj: unknown skill.");
                  else
                      obj->value[value] = skill->sn;
                  fMatch = true;
                  break;
              }

              break;

          case 'T':
              KEY("Timer", obj->timer, fread_number(fp));
              KEY("Time", obj->timer, fread_number(fp));
              break;

          case 'V':
              if (!str_cmp(word, "Values") || !str_cmp(word, "Vals")) {
                  obj->value[0] = fread_number(fp);
                  obj->value[1] = fread_number(fp);
                  obj->value[2] = fread_number(fp);
                  obj->value[3] = fread_number(fp);
                  if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                      obj->value[0] = obj->objprototype->value[0];
                  fMatch = true;
                  break;
              }

              if (!str_cmp(word, "Val")) {
                  obj->value[0] = fread_number(fp);
                  obj->value[1] = fread_number(fp);
                  obj->value[2] = fread_number(fp);
                  obj->value[3] = fread_number(fp);
                  obj->value[4] = fread_number(fp);
                  fMatch = true;
                  break;
              }
              break;

          case 'W':
              KEY("WearFlags", obj->wear_flags, fread_number(fp));
              KEY("WeaF", obj->wear_flags, fread_number(fp));
              KEY("WearLoc", obj->wear_loc, fread_number(fp));
              KEY("Wear", obj->wear_loc, fread_number(fp));
              KEY("Weight", obj->weight, fread_number(fp));
              KEY("Wt", obj->weight, fread_number(fp));
              break;

          case 'X':
              KEY("Xptolevel", obj->xp_tolevel, fread_number(fp));
              break;
        }


        if (!fMatch) {
            log_bug("Fread_obj: no match.");
            fread_to_eol(fp);
        }
    }
}


/***************************************************************************
 *	do_rload
 ***************************************************************************/
void do_rload(CHAR_DATA *ch, const char *argument)
{
    ROOM_INDEX_DATA *location;

    location = ch->in_room;
    if (location == NULL) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char("Mobiles can't load rooms.  Get serious.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("And what name is it saved under?\n\r", ch);
        return;
    }

    {
        static char buf[MAX_INPUT_LENGTH];
        (void)snprintf(buf, UMIN(strlen(argument), MAX_INPUT_LENGTH), "%s", argument);
        smash_tilde(buf);
        load_rdesc(ch->in_room, buf);
        send_to_char("Ok.\n\r", ch);
    }
}

void do_rsave(CHAR_DATA *ch, const char *argument)
{
    char strsave[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    FILE *fp;

    location = ch->in_room;

    if (location == NULL) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char("Mobiles can't save rooms.  Get serious.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("And what name shall it be saved under?\n\r", ch);
        return;
    }

    {
        static char buf[MAX_INPUT_LENGTH];
        (void)snprintf(buf, UMIN(strlen(argument), MAX_INPUT_LENGTH), "%s", argument);
        smash_tilde(buf);

        sprintf(strsave, "%s%s", RDESC_DIR, capitalize(buf));
        if ((fp = fopen(TEMP_FILE, "w")) == NULL) {
            log_bug("Rdesc_save: fopen");
            perror(strsave);
        } else {
            fwrite_rdesc(location, fp);
            fprintf(fp, "#END\n");
        }
    }

    fclose(fp);
    rename(TEMP_FILE, strsave);
}


/***************************************************************************
 *	fwrite_rdesc
 ***************************************************************************/
static void fwrite_rdesc(ROOM_INDEX_DATA *location, FILE *fp)
{
    fprintf(fp, "#RDESC\n");
    fprintf(fp, "%s~\n", location->name);
    fprintf(fp, "%s~\n", location->owner);
    fprintf(fp, "%d\n", location->heal_rate);
    fprintf(fp, "%d\n", location->mana_rate);
    fprintf(fp, "%ld\n", location->room_flags);
    fprintf(fp, "%d\n", location->sector_type);
    fprintf(fp, "%s~\n", location->description);
    fprintf(fp, "S\n\r");
}


/***************************************************************************
 *	load_rdesc
 ***************************************************************************/
static bool load_rdesc(ROOM_INDEX_DATA *location, const char *name)
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;
    bool found;

    found = false;
    sprintf(strsave, "%s%s", RDESC_DIR, capitalize(name));
    if ((fp = fopen(strsave, "r")) != NULL) {
        found = true;
        for (;; ) {
            char letter;
            char *word;

            letter = fread_letter(fp);
            if (letter == '*') {
                fread_to_eol(fp);
                continue;
            }

            if (letter != '#') {
                log_bug("Load_rdesc: # not found.");
                break;
            }

            word = fread_word(fp);

            if (!str_cmp(word, "RDESC")) {
                fread_rdesc(location, fp);
            } else if (!str_cmp(word, "END")) {
                break;
            } else {
                log_bug("Load_rdesc: bad section.");
                break;
            }
        }

        fclose(fp);
    }
    return found;
}

static void fread_rdesc(ROOM_INDEX_DATA *location, FILE *fp)
{
    char letter;

    log_string("Loading rdesc %ld.", location->vnum);

    location->name = fread_string(fp);
    location->owner = fread_string(fp);
    location->heal_rate = fread_number(fp);
    location->mana_rate = fread_number(fp);
    location->room_flags = fread_number(fp);
    location->sector_type = fread_number(fp);
    location->description = fread_string(fp);

    for (;; ) {
        letter = fread_letter(fp);

        if (letter == 'S')
            break;
    }
}
