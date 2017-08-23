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
static void save_area(struct area_data *area);
static void save_mobprogs(FILE *fp, struct area_data *area);
static void save_mobile(FILE *fp, struct mob_index_data *mob_idx);
static void save_mobiles(FILE *fp, struct area_data *area);
static void save_objects(const struct database_controller *db, const struct area_data *area);
static void save_rooms(const struct database_controller *db, const struct area_data *area);
static void save_shops(FILE *fp, struct area_data *area);
static void save_helps(const char const *filename);
static void show_save_help(struct char_data *ch);


void do_asave(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
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
        struct area_data *area;
        value = parse_int(arg);

        if ((area = area_getbyvnum(value)) == NULL) {
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
        struct area_data *area;

        save_area_list();
        save_helps(HELP_FILE);

        area = area_iterator_start(NULL);
        while (area != NULL) {
            if (IS_BUILDER(ch, area)) {
                save_area(area);
                REMOVE_BIT(area->area_flags, AREA_CHANGED);
            }
            area = area_iterator(area, NULL);
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
        struct area_data *area;

        save_area_list();
        save_helps(HELP_FILE);

        send_to_char("Saved areas:\n\r", ch);
        log_string("Saved areas:");

        saved = false;

        area = area_iterator_start(NULL);
        while (area != NULL) {
            /* Builder must be assigned this area. */
            if (IS_BUILDER(ch, area))
            {
                /* Save changed areas. */
                if (IS_SET(area->area_flags, AREA_CHANGED)) {
                    save_area(area);
                    saved = true;
                    printf_to_char(ch, "%24s - '%s'\n\r", area->name, area->file_name);
                    log_string("%24s - '%s'", area->name, area->file_name);

                    REMOVE_BIT(area->area_flags, AREA_CHANGED);
                }
            }

            area = area_iterator(area, NULL);
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
        struct area_data *area;
        if (ch->desc->editor == ED_NONE) {
            send_to_char("You are not editing an area, therefore an area vnum is required.\n\r", ch);
            return;
        }

        switch (ch->desc->editor) {
          case ED_AREA:
              area = (struct area_data *)ch->desc->ed_data;
              break;
          case ED_ROOM:
              area = ch->in_room->area;
              break;
          case ED_OBJECT:
              area = ((struct objecttemplate *)ch->desc->ed_data)->area;
              break;
          case ED_MOBILE:
              area = ((struct mob_index_data *)ch->desc->ed_data)->area;
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
}



char *fix_string(const char *str)
{
    static char strfix[MAX_STRING_LENGTH * 2];
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
    struct area_data *iterator;

    if ((fp = fopen(AREA_LIST, "w")) == NULL) {
        log_bug("Save_area_list: fopen");
        perror("area.lst");
        return;
    }


    iterator = area_iterator_start(NULL);
    while (iterator != NULL) {
        fprintf(fp, "%s\n", iterator->file_name);
        iterator = area_iterator(iterator, NULL);
    }

    fprintf(fp, "$\n");
    fclose(fp);
}

void save_mobprogs(FILE *fp, struct area_data *area)
{
    struct mprog_code *mprog;
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

void save_mobile(FILE *fp, struct mob_index_data *mob_idx)
{
    struct mprog_list *mprog;
    int race = mob_idx->race;
    char buf[MAX_STRING_LENGTH];
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

void save_mobiles(FILE *fp, struct area_data *area)
{
    struct mob_index_data *pMob;
    long iter;

    fprintf(fp, "#MOBILES\n");

    for (iter = area->min_vnum; iter <= area->max_vnum; iter++)
        if ((pMob = get_mob_index(iter)))
            save_mobile(fp, pMob);

    fprintf(fp, "#0\n\n\n\n");
    return;
}


void save_objects(const struct database_controller *db, const struct area_data *area)
{
    struct objecttemplate *template;
    struct array_list *serialized;
    char *dbstream;

    template = objecttemplate_iterator_start(&objecttemplate_empty_filter);
    while (template != NULL) {
        fprintf(db->_cfptr, "#OBJECT\n");

        serialized = objecttemplate_serialize(template);
        dbstream = database_create_stream(serialized);
        kvp_free_array(serialized);
        database_write_stream(db, dbstream);
        free(dbstream);

        template = objecttemplate_iterator(template, &objecttemplate_empty_filter);
    }

    return;
}

void save_rooms(const struct database_controller *db, const struct area_data *area)
{
    struct roomtemplate *template;
    struct array_list *serialized;
    char *dbstream;

    template = roomtemplate_iterator_start(&roomtemplate_empty_filter);
    while (template != NULL) {
        fprintf(db->_cfptr, "#ROOM\n");

        serialized = roomtemplate_serialize(template);
        dbstream = database_create_stream(serialized);
        kvp_free_array(serialized);
        database_write_stream(db, dbstream);
        free(dbstream);

        template = roomtemplate_iterator(template, &roomtemplate_empty_filter);
    }

    return;
}

void save_shops(FILE *fp, struct area_data *area)
{
    struct shop_data *shopIndex;
    struct mob_index_data *mob_idx;
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
    struct help_data *current;

    db = database_open(filename, false);
    if (db == NULL) {
        log_bug("Unable to open help file %s", filename);
        perror(filename);
        return;
    }

    current = helpdata_iteratorstart();
    while (current != NULL) {
        struct array_list *data = helpdata_serialize(current);
        char *dbstream = database_create_stream(data);
        kvp_free_array(data);
        database_write_stream(db, dbstream);
        free(dbstream);
        current = helpdata_iteratornext(current);
    }

    database_close(db);
}

void save_area(struct area_data *area)
{
    char haf[MAX_INPUT_LENGTH];
    struct database_controller *db;

    snprintf(haf, MAX_INPUT_LENGTH, "%s%s", AREA_FOLDER, area->file_name);
    db = database_open(haf, false);

    if (db == NULL) {
        log_bug("Open_area: fopen");
        perror(haf);
        return;
    }

    fprintf(db->_cfptr, "#AREADATA\n");
    {
        struct array_list *serialized = area_serialize(area);
        char *dbstream = database_create_stream(serialized);
        kvp_free_array(serialized);
        database_write_stream(db, dbstream);
        free(dbstream);
    }

    save_rooms(db, area);
    save_objects(db, area);
    save_mobiles(db->_cfptr, area);
    save_mobprogs(db->_cfptr, area);
    save_shops(db->_cfptr, area);

    fprintf(db->_cfptr, "#$\n");

    database_close(db);
}

void show_save_help(struct char_data *ch)
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

