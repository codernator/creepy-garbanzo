#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "help.h"


#define DIF(a, b)(~((~a) | (b)))


static char *fix_string(const char *str);
static char *fwrite_flag(long flags, char buf[]);
static void save_area_list();
static void save_area(AREA_DATA * area);
static void save_mobprogs(FILE *fp, AREA_DATA *area);
static void save_mobile(FILE *fp, MOB_INDEX_DATA *mob_idx);
static void save_mobiles(FILE *fp, AREA_DATA *area);
static void save_object(FILE *fp, OBJECTPROTOTYPE *pObjIndex);
static void save_objects(FILE *fp, AREA_DATA *area);
static void save_rooms(FILE *fp, AREA_DATA *area);
static void save_door_resets(FILE *fp, AREA_DATA *area);
static void save_resets(FILE *fp, AREA_DATA *area);
static void save_shops(FILE *fp, AREA_DATA *area);
static void save_helps(const char const *filename);
static void save_area(AREA_DATA *area);
static void show_save_help(CHAR_DATA *ch);


void do_asave(CHAR_DATA *ch, const char *argument)
{
    AREA_DATA *area;
    char arg[MIL];
    int value;

    DENY_NPC(ch);

    (void)one_argument(argument, arg);
    if (is_help(arg)) {
        show_save_help(ch);
        return;
    }

    /*
     * see if we have a numeric arugment - if we do, then
     * save the area it corresponds to
     */
    if (is_number(arg)) {
        value = parse_int(arg);

        if ((area = get_area_data(value)) == NULL) {
            send_to_char("That area does not exist.\n\r", ch);
            return;
        }

        /* save the area */
        if (!IS_BUILDER(ch, area))
        {
            send_to_char("You are not a builder for this area.\n\r", ch);
        }

        save_area_list();
        save_area(area);
        return;
    }

    /* save everything */
    if (!str_cmp(arg, "world")) {
        save_area_list();
        save_helps(HELP_FILE);

        for (area = area_first; area; area = area->next) {
            if (!IS_BUILDER(ch, area))
                continue;

            save_area(area);
            REMOVE_BIT(area->area_flags, AREA_CHANGED);
        }
        send_to_char("You saved the world.\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "helps")) {
        save_helps(HELP_FILE);
        send_to_char("All helps have been saved.\n\r", ch);
        return;
    }

    /* Save changed areas, only authorized areas. */
    /* ------------------------------------------ */
    if (!str_cmp(arg, "changed")) {
        bool saved;

        save_area_list();
        save_helps(HELP_FILE);

        send_to_char("Saved areas:\n\r", ch);
        log_string("Saved areas:");

        saved = false;
        for (area = area_first; area; area = area->next) {
            /* Builder must be assigned this area. */
            if (!IS_BUILDER(ch, area))
            {
                continue;
            }

            /* Save changed areas. */
            if (IS_SET(area->area_flags, AREA_CHANGED)) {
                save_area(area);
                saved = true;
                printf_to_char(ch, "%24s - '%s'\n\r", area->name, area->file_name);
                log_string("%24s - '%s'", area->name, area->file_name);

                REMOVE_BIT(area->area_flags, AREA_CHANGED);
            }
        }

        if (!saved) {
            send_to_char("None.", ch);
            log_string("None.");
        }
        return;
    }

    /* save the area list */
    if (!str_prefix(arg, "list")) {
        save_area_list();
        return;
    }

    /* save the area that is currently being edited */
    if (!str_prefix(arg, "area")) {
        if (ch->desc->editor == ED_NONE) {
            send_to_char("You are not editing an area, therefore an area vnum is required.\n\r", ch);
            return;
        }

        switch (ch->desc->editor) {
          case ED_AREA:
              area = (AREA_DATA *)ch->desc->ed_data;
              break;
          case ED_ROOM:
              area = ch->in_room->area;
              break;
          case ED_OBJECT:
              area = ((OBJECTPROTOTYPE *)ch->desc->ed_data)->area;
              break;
          case ED_MOBILE:
              area = ((MOB_INDEX_DATA *)ch->desc->ed_data)->area;
              break;
          case ED_HELP:
              send_to_char("Saving helps.", ch);
              save_helps(HELP_FILE);
              return;
          default:
              area = ch->in_room->area;
              break;
        }

        if (area == NULL || !IS_BUILDER(ch, area)) {
            send_to_char("You are not a builder for this area.\n\r", ch);
            return;
        }

        save_area_list();
        save_area(area);
        REMOVE_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Area saved.\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "skills")) {
        save_skills();
        save_groups();
        send_to_char("Skills saved.\n\r", ch);
        return;
    }

    /* display help */
    show_save_help(ch);
    return;
}



char *fix_string(const char *str)
{
    static char strfix[MSL * 2];
    int idx;
    int pos;

    if (str == NULL) {
        strfix[0] = '\0';
        return strfix;
    }

    for (pos = idx = 0; str[idx + pos] != '\0'; idx++) {
        if (str[idx + pos] == '\r' || str[idx + pos] == '~')
            pos++;
        strfix[idx] = str[idx + pos];
    }

    strfix[idx] = '\0';
    return strfix;
}

char *fwrite_flag(long flags, char buf[])
{
    unsigned int offset;
    char *cp;

    buf[0] = '\0';
    if (flags == 0) {
        strcpy(buf, "0");
        return buf;
    }

    /* 32 -- number of bits in a long */
    for (offset = 0, cp = buf; offset < 32; offset++) {
        if (flags & (1u << offset)) {
            if (offset <= (unsigned int)('Z' - 'A'))
                *(cp++) = 'A' + (char)offset;
            else
                *(cp++) = 'a' + (char)(offset - ((unsigned int)('Z' - 'A') + 1));
        }
    }

    *cp = '\0';

    return buf;
}

void save_area_list()
{
    FILE *fp;
    AREA_DATA *area;

    if ((fp = fopen(AREA_LIST, "w")) == NULL) {
        log_bug("Save_area_list: fopen");
        perror("area.lst");
    } else {
        for (area = area_first; area; area = area->next)
            fprintf(fp, "%s\n", area->file_name);

        fprintf(fp, "$\n");
        fclose(fp);
    }
}

void save_mobprogs(FILE *fp, AREA_DATA *area)
{
    MPROG_CODE *mprog;
    long iter;

    fprintf(fp, "#PROGRAMS\n");

    for (iter = area->min_vnum; iter <= area->max_vnum; iter++) {
        if ((mprog = get_mprog_index(iter)) != NULL) {
            fprintf(fp, "#%ld\n", iter);
            if (mprog->comment[0] != '\0')
                fprintf(fp, "Comment %s~\n", mprog->comment);
            fprintf(fp, "Code\n%s~\n\n", fix_string(mprog->code));
        }
    }

    fprintf(fp, "#0\n\n");
    return;
}

void save_mobile(FILE *fp, MOB_INDEX_DATA *mob_idx)
{
    MPROG_LIST *mprog;
    int race = mob_idx->race;
    char buf[MSL];
    long temp;

    fprintf(fp, "#%ld\n", mob_idx->vnum);
    fprintf(fp, "%s~\n", mob_idx->player_name);
    fprintf(fp, "%s~\n", mob_idx->short_descr);
    fprintf(fp, "%s~\n", fix_string(mob_idx->long_descr));
    fprintf(fp, "%s~\n", fix_string(mob_idx->description));
    fprintf(fp, "%s~\n", race_table[race].name);
    fprintf(fp, "%s ", fwrite_flag(mob_idx->act, buf));
    fprintf(fp, "%s ", fwrite_flag(mob_idx->affected_by, buf));
    fprintf(fp, "%d ", mob_idx->level);
    fprintf(fp, "%d ", mob_idx->hitroll);
    fprintf(fp, "%dd%d+%d ", mob_idx->hit[DICE_NUMBER],
            mob_idx->hit[DICE_TYPE],
            mob_idx->hit[DICE_BONUS]);
    fprintf(fp, "%dd%d+%d ", mob_idx->mana[DICE_NUMBER],
            mob_idx->mana[DICE_TYPE],
            mob_idx->mana[DICE_BONUS]);
    fprintf(fp, "%dd%d+%d ", mob_idx->damage[DICE_NUMBER],
            mob_idx->damage[DICE_TYPE],
            mob_idx->damage[DICE_BONUS]);
    fprintf(fp, "'%s'\n", attack_table[mob_idx->dam_type].name);
    fprintf(fp, "%ld %ld %ld %ld\n",
            mob_idx->ac[AC_PIERCE] / 10,
            mob_idx->ac[AC_BASH] / 10,
            mob_idx->ac[AC_SLASH] / 10,
            mob_idx->ac[AC_EXOTIC] / 10);
    fprintf(fp, "%s ", fwrite_flag(mob_idx->off_flags, buf));
    fprintf(fp, "%s ", fwrite_flag(mob_idx->imm_flags, buf));
    fprintf(fp, "%s ", fwrite_flag(mob_idx->res_flags, buf));
    fprintf(fp, "%s\n", fwrite_flag(mob_idx->vuln_flags, buf));
    fprintf(fp, "%s %s %s %u\n",
            position_table[mob_idx->start_pos].short_name,
            position_table[mob_idx->default_pos].short_name,
            (mob_idx->sex > 0 && sex_table[mob_idx->sex].name != NULL) ? sex_table[mob_idx->sex].name : "either",
            mob_idx->wealth);
    fprintf(fp, "%s ", fwrite_flag(mob_idx->form, buf));
    fprintf(fp, "%s ", fwrite_flag(mob_idx->parts, buf));

    fprintf(fp, "%s ", size_table[mob_idx->size].name);
    fprintf(fp, "%s\n", IS_NULLSTR(mob_idx->material) ? mob_idx->material : "unknown");

    if ((temp = DIF(race_table[race].act, mob_idx->act)))
        fprintf(fp, "F act %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].aff, mob_idx->affected_by)))
        fprintf(fp, "F aff %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].off, mob_idx->off_flags)))
        fprintf(fp, "F off %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].imm, mob_idx->imm_flags)))
        fprintf(fp, "F imm %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].res, mob_idx->res_flags)))
        fprintf(fp, "F res %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].vuln, mob_idx->vuln_flags)))
        fprintf(fp, "F vul %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].form, mob_idx->form)))
        fprintf(fp, "F for %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].parts, mob_idx->parts)))
        fprintf(fp, "F par %s\n", fwrite_flag(temp, buf));

    for (mprog = mob_idx->mprogs; mprog; mprog = mprog->next) {
        fprintf(fp, "M %s %ld %s~\n",
                mprog_type_to_name(mprog->trig_type),
                mprog->vnum,
                mprog->trig_phrase);
    }

    return;
}

void save_mobiles(FILE *fp, AREA_DATA *area)
{
    MOB_INDEX_DATA *pMob;
    long iter;

    fprintf(fp, "#MOBILES\n");

    for (iter = area->min_vnum; iter <= area->max_vnum; iter++)
        if ((pMob = get_mob_index(iter)))
            save_mobile(fp, pMob);

    fprintf(fp, "#0\n\n\n\n");
    return;
}



//void save_object(FILE *fp, OBJECTPROTOTYPE *pObjIndex)
//{
//    struct keyvaluepair_array *serialized;
//
//    serialized = objectprototype_serialize(pObjIndex);
//    database_write(fp, serialized);
//    free(serialized);
//}

void save_object(FILE *fp, OBJECTPROTOTYPE *pObjIndex)
{
    AFFECT_DATA *pAf;
    EXTRA_DESCR_DATA *extra;
    char letter;
    char buf[MSL];

    fprintf(fp, "#%ld\n", pObjIndex->vnum);
    fprintf(fp, "%s~\n", pObjIndex->name);
    fprintf(fp, "%s~\n", pObjIndex->short_descr);
    fprintf(fp, "%s~\n", fix_string(pObjIndex->description));
    fprintf(fp, "%s~\n", pObjIndex->material);
    fprintf(fp, "%s ", fwrite_flag(pObjIndex->extra2_flags, buf));
    fprintf(fp, "%s ", item_name(pObjIndex->item_type));
    fprintf(fp, "%s ", fwrite_flag(pObjIndex->extra_flags, buf));
    fprintf(fp, "%s\n", fwrite_flag(pObjIndex->wear_flags, buf));

    /*
     *  Using fwrite_flag to write most values gives a strange
     *  looking area file, consider making a case for each
     *  item type later.
     */

    switch (pObjIndex->item_type) {
      default:
          fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[0], buf));
          fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[1], buf));
          fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[2], buf));
          fprintf(fp, "%s ", fwrite_flag(pObjIndex->value[3], buf));
          fprintf(fp, "%s\n", fwrite_flag(pObjIndex->value[4], buf));
          break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
          fprintf(fp, "%ld %ld '%s' %ld %ld\n",
                  pObjIndex->value[0],
                  pObjIndex->value[1],
                  liq_table[pObjIndex->value[2]].liq_name,
                  pObjIndex->value[3],
                  pObjIndex->value[4]);
          break;

      case ITEM_CONTAINER:
          fprintf(fp, "%ld %s %ld %ld %ld\n",
                  pObjIndex->value[0],
                  fwrite_flag(pObjIndex->value[1], buf),
                  pObjIndex->value[2],
                  pObjIndex->value[3],
                  pObjIndex->value[4]);
          break;

      case ITEM_WEAPON:
          fprintf(fp, "%s %ld %ld '%s' %s\n",
                  weapon_name((int)pObjIndex->value[0]),
                  pObjIndex->value[1],
                  pObjIndex->value[2],
                  attack_table[pObjIndex->value[3]].name,
                  fwrite_flag(pObjIndex->value[4], buf));
          break;

      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
          {
              SKILL *skills[4];
              int idx;

              for (idx = 1; idx <= 4; idx++)
                  skills[idx - 1] = resolve_skill_sn((int)pObjIndex->value[idx]);

              fprintf(fp, "%ld '%s' '%s' '%s' '%s'\n",
                      pObjIndex->value[0],
                      (skills[0] != NULL) ? skills[0]->name : "",
                      (skills[1] != NULL) ? skills[1]->name : "",
                      (skills[2] != NULL) ? skills[2]->name : "",
                      (skills[3] != NULL) ? skills[3]->name : "");
              break;
          }

      case ITEM_STAFF:
      case ITEM_WAND:
          {
              SKILL *skill;

              skill = resolve_skill_sn((int)pObjIndex->value[3]);

              fprintf(fp, "%ld %ld %ld '%s' %ld\n",
                      pObjIndex->value[0],
                      pObjIndex->value[1],
                      pObjIndex->value[2],
                      (skill != NULL) ? skill->name : "",
                      pObjIndex->value[4]);
              break;
          }
    }

    fprintf(fp, "%d ", pObjIndex->level);
    fprintf(fp, "%d ", pObjIndex->weight);
    fprintf(fp, "%u ", pObjIndex->cost);
    fprintf(fp, "%d ", pObjIndex->init_timer);
    fprintf(fp, "%d ", pObjIndex->condition);

    for (pAf = pObjIndex->affected; pAf; pAf = pAf->next) {
        if (pAf->where == TO_OBJECT || pAf->bitvector == 0) {
            fprintf(fp, "A\n%d %ld\n", pAf->location, pAf->modifier);
        } else {
            fprintf(fp, "F\n");

            switch (pAf->where) {
              case TO_AFFECTS:
                  fprintf(fp, "A ");
                  break;
              case TO_IMMUNE:
                  fprintf(fp, "I ");
                  break;
              case TO_RESIST:
                  fprintf(fp, "R ");
                  break;
              case TO_VULN:
                  fprintf(fp, "V ");
                  break;
              default:
                  log_bug("olc_save: Invalid Affect->where");
                  break;
            }

            fprintf(fp, "%d %ld %s\n", pAf->location, pAf->modifier,
                    fwrite_flag(pAf->bitvector, buf));
        }
    }

    for (extra = pObjIndex->extra_descr; extra; extra = extra->next)
        fprintf(fp, "E\n%s~\n%s~\n", extra->keyword, fix_string(extra->description));
    return;
}

void save_objects(FILE *fp, AREA_DATA *area)
{
    OBJECTPROTOTYPE *pObj;
    long iter;

    fprintf(fp, "#OBJECTS\n");

    for (iter = area->min_vnum; iter <= area->max_vnum; iter++)
        if ((pObj = objectprototype_getbyvnum(iter)))
            save_object(fp, pObj);

    fprintf(fp, "#0\n\n\n\n");
    return;
}

void save_rooms(FILE *fp, AREA_DATA *area)
{
    ROOM_INDEX_DATA *room;
    EXTRA_DESCR_DATA *extra;
    EXIT_DATA *exit;
    AFFECT_DATA *paf;
    SKILL *skill;
    int hash_idx;
    int door;

    fprintf(fp, "#ROOMS\n");
    for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
        for (room = room_index_hash[hash_idx];
             room != NULL;
             room = room->next) {
            if (room->area == area) {
                fprintf(fp, "#%ld\n", room->vnum);
                fprintf(fp, "%s~\n", room->name);
                fprintf(fp, "%s~\n", fix_string(room->description));
                fprintf(fp, "0 ");
                fprintf(fp, "%ld ", room->room_flags);
                fprintf(fp, "%d\n", room->sector_type);

                for (extra = room->extra_descr; extra; extra = extra->next) {
                    fprintf(fp, "E\n%s~\n%s~\n",
                            extra->keyword,
                            fix_string(extra->description));
                }

                for (door = 0; door < MAX_DIR; door++) { /* I hate this! */
                    if ((exit = room->exit[door])
                        && exit->u1.to_room) {
                        int locks = 0;

                        /* HACK : TO PREVENT EX_LOCKED etc without EX_ISDOOR
                         * to stop booting the mud */
                        if (IS_SET(exit->rs_flags, EX_CLOSED)
                            || IS_SET(exit->rs_flags, EX_LOCKED)
                            || IS_SET(exit->rs_flags, EX_PICKPROOF)
                            || IS_SET(exit->rs_flags, EX_NOPASS)
                            || IS_SET(exit->rs_flags, EX_EASY)
                            || IS_SET(exit->rs_flags, EX_HARD)
                            || IS_SET(exit->rs_flags, EX_INFURIATING)
                            || IS_SET(exit->rs_flags, EX_NOCLOSE)
                            || IS_SET(exit->rs_flags, EX_NOLOCK))
                            SET_BIT(exit->rs_flags, EX_ISDOOR);
                        else
                            REMOVE_BIT(exit->rs_flags, EX_ISDOOR);

                        /* THIS SUCKS but it's backwards compatible */
                        /* NOTE THAT EX_NOCLOSE NOLOCK etc aren't being saved */
                        if (IS_SET(exit->rs_flags, EX_ISDOOR)
                            && (!IS_SET(exit->rs_flags, EX_PICKPROOF))
                            && (!IS_SET(exit->rs_flags, EX_NOPASS)))
                            locks = 1;

                        if (IS_SET(exit->rs_flags, EX_ISDOOR)
                            && (IS_SET(exit->rs_flags, EX_PICKPROOF))
                            && (!IS_SET(exit->rs_flags, EX_NOPASS)))
                            locks = 2;

                        if (IS_SET(exit->rs_flags, EX_ISDOOR)
                            && (!IS_SET(exit->rs_flags, EX_PICKPROOF))
                            && (IS_SET(exit->rs_flags, EX_NOPASS)))
                            locks = 3;

                        if (IS_SET(exit->rs_flags, EX_ISDOOR)
                            && (IS_SET(exit->rs_flags, EX_PICKPROOF))
                            && (IS_SET(exit->rs_flags, EX_NOPASS)))
                            locks = 4;

                        fprintf(fp, "D%d\n", exit->orig_door);
                        fprintf(fp, "%s~\n", fix_string(exit->description));
                        fprintf(fp, "%s~\n", exit->keyword);
                        fprintf(fp, "%d %ld %ld\n",
                                locks,
                                exit->key,
                                exit->u1.to_room->vnum);
                    }
                }

                if (room->mana_rate != 100 || room->heal_rate != 100) {
                    fprintf(fp, "M %d H %d\n",
                            room->mana_rate,
                            room->heal_rate);
                }

                if (!IS_NULLSTR(room->owner))
                    fprintf(fp, "O %s~\n", room->owner);

                for (paf = room->affected; paf != NULL; paf = paf->next)
                    if (paf->duration < 0 && (skill = resolve_skill_sn(paf->type)) != NULL)
                        fprintf(fp, "A '%s' %d\n", skill->name, paf->level);
                fprintf(fp, "S\n");
            }
        }
    }

    fprintf(fp, "#0\n\n\n\n");
    return;
}

void save_door_resets(FILE *fp, AREA_DATA *area)
{
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit;
    int hash_idx;
    int door;

    for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
        for (room = room_index_hash[hash_idx]; room; room = room->next) {
            if (room->area == area) {
                for (door = 0; door < MAX_DIR; door++) {
                    if ((exit = room->exit[door])
                        && exit->u1.to_room
                        && (IS_SET(exit->rs_flags, EX_CLOSED) || IS_SET(exit->rs_flags, EX_LOCKED))) {
                        fprintf(fp, "D 0 %ld %d %d\n",
                                room->vnum,
                                exit->orig_door,
                                IS_SET(exit->rs_flags, EX_LOCKED) ? 2 : 1);
                    }
                }
            }
        }
    }

    return;
}

void save_resets(FILE *fp, AREA_DATA *area)
{
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pLastMob = NULL;
    ROOM_INDEX_DATA *pRoom;
    int hash_idx;

    fprintf(fp, "#RESETS\n");

    save_door_resets(fp, area);

    for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
        for (pRoom = room_index_hash[hash_idx]; pRoom; pRoom = pRoom->next) {
            if (pRoom->area == area) {
                for (pReset = pRoom->reset_first; pReset; pReset = pReset->next) {
                    switch (pReset->command) {
                      default:
                          log_bug("Save_resets: bad command %c.", (int)pReset->command);
                          break;

                      case 'M':
                          pLastMob = get_mob_index(pReset->arg1);
                          fprintf(fp, "M 0 %ld %d %ld %d\n",
                                  pReset->arg1,
                                  pReset->arg2,
                                  pReset->arg3,
                                  pReset->arg4);
                          break;

                      case 'O':
                          pRoom = get_room_index(pReset->arg3);
                          fprintf(fp, "O 0 %ld 0 %ld\n",
                                  pReset->arg1,
                                  pReset->arg3);
                          break;

                      case 'P':
                          fprintf(fp, "P 0 %ld %d %ld %d\n",
                                  pReset->arg1,
                                  pReset->arg2,
                                  pReset->arg3,
                                  pReset->arg4);
                          break;

                      case 'G':
                          fprintf(fp, "G 0 %ld 0\n", pReset->arg1);
                          if (!pLastMob)
                              log_string("Save_resets: !NO_MOB! in [%s]", area->file_name);
                          break;
                      case 'E':
                          fprintf(fp, "E 0 %ld 0 %ld\n",
                                  pReset->arg1,
                                  pReset->arg3);
                          if (!pLastMob)
                              log_string("Save_resets: !NO_MOB! in [%s]", area->file_name);
                          break;

                      case 'D':
                          break;

                      case 'R':
                          pRoom = get_room_index(pReset->arg1);
                          fprintf(fp, "R 0 %ld %d\n",
                                  pReset->arg1,
                                  pReset->arg2);
                          break;
                    }
                }
            }       /* End if correct area */
        }               /* End for pRoom */
    } /* End for hash_idx */

    fprintf(fp, "S\n\n\n\n");
    return;
}

void save_shops(FILE *fp, AREA_DATA *area)
{
    SHOP_DATA *shopIndex;
    MOB_INDEX_DATA *mob_idx;
    int iTrade;
    int hash_idx;

    fprintf(fp, "#SHOPS\n");

    for (hash_idx = 0; hash_idx < MAX_KEY_HASH; hash_idx++) {
        for (mob_idx = mob_index_hash[hash_idx]; mob_idx; mob_idx = mob_idx->next) {
            if (mob_idx && mob_idx->area == area && mob_idx->shop) {
                shopIndex = mob_idx->shop;

                fprintf(fp, "%ld ", shopIndex->keeper);
                for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
                    if (shopIndex->buy_type[iTrade] != 0)
                        fprintf(fp, "%d ", shopIndex->buy_type[iTrade]);
                    else
                        fprintf(fp, "0 ");
                }
                fprintf(fp, "%d %d ", shopIndex->profit_buy, shopIndex->profit_sell);
                fprintf(fp, "%d %d\n", shopIndex->open_hour, shopIndex->close_hour);
            }
        }
    }

    fprintf(fp, "0\n\n\n\n");
    return;
}

void save_helps(const char const *filename)
{
    struct database_controller *db;
    struct helpdata_iterator *iterator;

    db = database_open(filename);
    if (db == NULL) {
        log_bug("Unable to open help file %s", filename);
        perror(filename);
        return;
    }
    
    iterator = helpdata_iteratorstart();
    while (iterator != NULL) {
        HELP_DATA *current = iterator->current;
        KEYVALUEPAIR_ARRAY *data = helpdata_serialize(current);
        database_write(db, data);
        free(data);
        iterator = helpdata_iteratornext(iterator);
    }

    database_close(db);
}

void save_area(AREA_DATA *area)
{
    FILE *fp;
    KEYVALUEPAIR_ARRAY *serialized;
    char haf[MIL];
    struct database_controller *db;

    snprintf(haf, MIL, "%s%s", AREA_FOLDER, area->file_name);
    db = database_open(haf);

    if (db == NULL) {
        log_bug("Open_area: fopen");
        perror(haf);
        return;
    }

    fprintf(db->_cfptr, "#AREADATA\n");
    serialized = area_serialize(area);
    database_write(db, serialized);
    keyvaluepairarray_free(serialized);

    save_mobiles(db->_cfptr, area);
    save_objects(db->_cfptr, area);
    save_rooms(db->_cfptr, area);
    save_mobprogs(db->_cfptr, area);
    save_resets(db->_cfptr, area);
    save_shops(db->_cfptr, area);

    fprintf(fp, "#$\n");

    database_close(db);
}

void show_save_help(CHAR_DATA *ch)
{
    send_to_char("`#Syntax`3:``\n\r", ch);
    send_to_char("  `!asave `1<`!vnum`1>``   - saves a particular area\n\r", ch);
    send_to_char("  `!asave list``     - saves the area.lst file\n\r", ch);
    send_to_char("  `!asave area``     - saves the area being edited\n\r", ch);
    send_to_char("  `!asave changed``  - saves all changed zones\n\r", ch);
    send_to_char("  `!asave helps``    - saves all helps\n\r", ch);
    send_to_char("  `!asave skills``   - saves the skill list\n\r", ch);
    send_to_char("  `!asave world``    - saves EVERYTHING\n\r", ch);
    send_to_char("\n\r", ch);
}

