#include "merc.h"
#include "db.h"
#include "object.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"



static void show_room_stats(CHAR_DATA *ch, const char *argument);
static void show_object_stats(CHAR_DATA *ch, const char *argument);
static void show_mob_stats(CHAR_DATA *ch, const char *argument);


/**
 * entry function into various game entity statistics.
 */
void do_stat(CHAR_DATA *ch, const char *argument)
{
    GAMEOBJECT *obj;
    ROOM_INDEX_DATA *location;
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    const char *string;

    DENY_NPC(ch);

    string = one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  stat <name>\n\r", ch);
        send_to_char("  stat obj <name>\n\r", ch);
        send_to_char("  stat mob <name>\n\r", ch);
        send_to_char("  stat room <number>\n\r", ch);
        send_to_char("  stat iobj <name>\n\r", ch);
        send_to_char("  stat ichar <name>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "room")) {
        show_room_stats(ch, string);
        return;
    }

    if (!str_cmp(arg, "obj")) {
        show_object_stats(ch, string);
        return;
    }
    if (!str_cmp(arg, "char") || !str_cmp(arg, "mob")) {
        show_mob_stats(ch, string);
        return;
    }

    obj = get_obj_world(ch, argument);
    if (obj != NULL) {
        show_object_stats(ch, argument);
        return;
    }

    victim = get_char_world(ch, argument);
    if (victim != NULL) {
        show_mob_stats(ch, argument);
        return;
    }

    location = find_location(ch, argument);
    if (location != NULL) {
        show_room_stats(ch, argument);
        return;
    }

    send_to_char("Nothing by that name found anywhere.\n\r", ch);
}


/**
 * show statistics for a room
 */
void show_room_stats(CHAR_DATA *ch, const char *argument)
{
    ROOM_INDEX_DATA *location;
    GAMEOBJECT *obj;
    CHAR_DATA *rch;
    AFFECT_DATA *paf;
    SKILL *skill;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int door;

    one_argument(argument, arg);
    location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
    if (location == NULL) {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, location) && ch->in_room != location
        && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    printf_to_char(ch, "Name: '%s'\n\rArea: '%s'\n\r",
                   location->name,
                   location->area->name);
    printf_to_char(ch, "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n\r",
                   location->vnum,
                   location->sector_type,
                   location->light,
                   location->heal_rate,
                   location->mana_rate);

    if (location->owner != NULL)
        printf_to_char(ch, "Owner: %s ", location->owner);

    send_to_char("\n\r", ch);

    printf_to_char(ch, "Room flags: %s\n\rDescription:\n\r%s",
                   room_flag_bit_name(location),
                   location->description);


    if (location->extra_descr != NULL) {
        EXTRA_DESCR_DATA *ed;

        send_to_char("Extra description keywords: '", ch);
        for (ed = location->extra_descr; ed; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char(" ", ch);
        }
        send_to_char("'.\n\r", ch);
    }

    send_to_char("Characters:", ch);
    for (rch = location->people; rch; rch = rch->next_in_room) {
        if (can_see(ch, rch)) {
            send_to_char(" ", ch);
            one_argument(rch->name, buf);
            send_to_char(buf, ch);
        }
    }

    send_to_char(".\n\rObjects:   ", ch);
    for (obj = location->contents; obj; obj = obj->next_content) {
        send_to_char(" ", ch);
        one_argument(object_name_get(obj), buf);
        send_to_char(buf, ch);
    }
    send_to_char(".\n\r", ch);


    for (paf = location->affected; paf != NULL; paf = paf->next) {
        if ((skill = resolve_skill_affect(paf)) != NULL) {
            printf_to_char(ch, "Spell: '%s' for %d hours, level %d.\n\r",
                           skill->name,
                           paf->duration,
                           paf->level);
        }
    }

    send_to_char("\n\r", ch);
    for (door = 0; door <= 5; door++) {
        EXIT_DATA *pexit;

        if ((pexit = location->exit[door]) != NULL) {
            printf_to_char(ch, "Door: %d.  To: %d.  Key: %d.  Exit flags: %s.\n\rKeyword: '%s'.  Description: %s",
                           door,
                           (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
                           pexit->key,
                           exit_bit_name(pexit->exit_info),
                           pexit->keyword,
                           pexit->description[0] != '\0'
                           ? pexit->description : "(none).\n\r");
        }
    }

    return;
}



/**
 * show statistics for an object
 */
void show_object_stats(CHAR_DATA *ch, const char *argument)
{
    AFFECT_DATA *paf;
    GAMEOBJECT *obj;
    SKILL *skill;
    char arg[MAX_INPUT_LENGTH];
    int idx;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Stat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_world(ch, argument)) == NULL) {
        send_to_char("Nothing like that in hell, earth, or heaven.\n\r", ch);
        return;
    }

    printf_to_char(ch, "Name(s): %s\n\r", object_name_get(obj));
    printf_to_char(ch, "Vnum: %d  Format: %s\n\r", obj->objprototype->vnum, item_type_name(obj));
    {
        const char *ownername = object_ownername_get(obj);
        printf_to_char(ch, "Owner: %s\n\r", (ownername != NULL) ? ownername : "none");
    }
    printf_to_char(ch, "Short description: %s\n\rLong description: %s\n\r", obj->short_descr, obj->description); 
    printf_to_char(ch, "Wear bits: %s\n\rExtra bits: %s\n\r", wear_bit_name(obj->wear_flags), extra_bit_name(obj->extra_flags));
    printf_to_char(ch, "Extra2 bits: %s\n\r", extra2_bit_name(obj->extra2_flags));
    printf_to_char(ch, "Number: %d/%d  Weight: %d/%d/%d(10th pounds)\n\r", 1, get_obj_number(obj), obj->weight, get_obj_weight(obj), get_true_weight(obj));
    printf_to_char(ch, "Level: %d  Cost: %d  Condition: %d  Timer: %d\n\r", obj->level, obj->cost, obj->condition, obj->timer);
    if (IS_OBJ_STAT2(obj, ITEM_RELIC)) {
        printf_to_char(ch, "Exp TNL: %d\n\r", obj->xp_tolevel);
        printf_to_char(ch, "Exp: %d\n\r", obj->exp);
    }
    printf_to_char(ch, "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
                   obj->in_room == NULL ? 0 : obj->in_room->vnum,
                   obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr,
                   obj->carried_by == NULL ? "(none)" :
                   can_see(ch, obj->carried_by) ? obj->carried_by->name : "someone",
                   obj->wear_loc);

    printf_to_char(ch, "Values: %d %d %d %d %d\n\r", obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);

    /* now give out vital statistics as per identify */
    switch (obj->item_type) {
      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
          printf_to_char(ch, "Level %d spells of:", obj->value[0]);

          for (idx = 1; idx <= 4; idx++) {
              if ((skill = resolve_skill_sn((int)obj->value[idx])) != NULL)
                  printf_to_char(ch, " '%s'", skill->name);

          }

          send_to_char(".\n\r", ch);
          break;

      case ITEM_DOLL:
          if (obj->target != NULL)
              printf_to_char(ch, "Is currently linked to %s.\n\r", obj->target->name);
          break;

      case ITEM_WAND:
      case ITEM_STAFF:
          printf_to_char(ch, "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0]);
          if ((skill = resolve_skill_sn((int)obj->value[3])) != NULL)
              printf_to_char(ch, " '%s'.\n\r", skill->name);
          break;

      case ITEM_DRINK_CON:
          printf_to_char(ch, "It holds %s-colored %s.\n\r", liq_table[obj->value[2]].liq_color, liq_table[obj->value[2]].liq_name);
          break;
      case ITEM_SOCKETS:
          send_to_char("Gem type is: ", ch);
          switch (obj->value[0]) {
            case 0: send_to_char("none.  Tell an immortal.\n\r", ch);    break;
            case 1: send_to_char("sapphire.\n\r", ch);   break;
            case 2: send_to_char("ruby.\n\r", ch);   break;
            case 3: send_to_char("emerald.\n\r", ch);    break;
            case 4: send_to_char("diamond.\n\r", ch);    break;
            case 5: send_to_char("topaz.\n\r", ch);  break;
            case 6: send_to_char("skull.\n\r", ch);  break;
            default: send_to_char("Unknown.  Something is wrong, fix this object.\n\r", ch);    break;
          }
          send_to_char("Gem value is: ", ch);
          switch (obj->value[1]) {
            case 0: send_to_char("chip.\n\r", ch);    break;
            case 1: send_to_char("flawed.\n\r", ch);   break;
            case 2: send_to_char("flawless.\n\r", ch);   break;
            case 3: send_to_char("perfect.\n\r", ch);    break;
            default: send_to_char("Unknown.  Something is wrong, fix this object.\n\r", ch);   break;
          }
          break;
      case ITEM_WEAPON:
          send_to_char("Weapon type is ", ch);
          switch (obj->value[0]) {
            case (WEAPON_EXOTIC):
                send_to_char("exotic\n\r", ch);
                break;
            case (WEAPON_SWORD):
                send_to_char("sword\n\r", ch);
                break;
            case (WEAPON_DAGGER):
                send_to_char("dagger\n\r", ch);
                break;
            case (WEAPON_SPEAR):
                send_to_char("spear/staff\n\r", ch);
                break;
            case (WEAPON_MACE):
                send_to_char("mace/club\n\r", ch);
                break;
            case (WEAPON_AXE):
                send_to_char("axe\n\r", ch);
                break;
            case (WEAPON_FLAIL):
                send_to_char("flail\n\r", ch);
                break;
            case (WEAPON_WHIP):
                send_to_char("whip\n\r", ch);
                break;
            case (WEAPON_POLEARM):
                send_to_char("polearm\n\r", ch);
                break;
            default: send_to_char("Unknown.  Something is wrong, fix this object.\n\r", ch);   
                     break;
          }
          printf_to_char(ch, "Damage is %dd%d(average %d)\n\r", obj->value[1], obj->value[2], (1 + obj->value[2]) * obj->value[1] / 2);

          printf_to_char(ch, "Damage noun is %s.\n\r", attack_table[obj->value[3]].noun);
          if (obj->value[4]) /* weapon flags */
              printf_to_char(ch, "Weapons flags: %s\n\r", weapon_bit_name(obj->value[4]));
          break;

      case ITEM_ARMOR:
          printf_to_char(ch, "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r", obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
          break;

      case ITEM_CONTAINER:
          printf_to_char(ch, "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r", obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
          if (obj->value[4] != 100)
              printf_to_char(ch, "Weight multiplier: %d%%\n\r", obj->value[4]);
          break;
    }


    if (obj->extra_descr != NULL || obj->objprototype->extra_descr != NULL) {
        EXTRA_DESCR_DATA *ed;

        send_to_char("Extra description keywords: '", ch);

        for (ed = obj->extra_descr; ed != NULL; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char(" ", ch);
        }

        for (ed = obj->objprototype->extra_descr; ed != NULL; ed = ed->next) {
            send_to_char(ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char(" ", ch);
        }

        send_to_char("'\n\r", ch);
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next) {
        printf_to_char(ch, "Affects %s by %d, level %d",
                       affect_loc_name(paf->location),
                       paf->modifier,
                       paf->level);
        if (paf->duration > -1)
            printf_to_char(ch, ", %d hours.\n\r", paf->duration);
        else
            printf_to_char(ch, ".\n\r");

        if (paf->bitvector) {
            switch (paf->where) {
              case TO_AFFECTS:
                  printf_to_char(ch, "Adds %s affect.\n", affect_bit_name(paf->bitvector));
                  break;
              case TO_WEAPON:
                  printf_to_char(ch, "Adds %s weapon flags.\n", weapon_bit_name(paf->bitvector));
                  break;
              case TO_OBJECT:
                  printf_to_char(ch, "Adds %s object flag.\n", extra_bit_name(paf->bitvector));
                  break;
              case TO_IMMUNE:
                  printf_to_char(ch, "Adds immunity to %s.\n", imm_bit_name(paf->bitvector));
                  break;
              case TO_RESIST:
                  printf_to_char(ch, "Adds resistance to %s.\n\r", imm_bit_name(paf->bitvector));
                  break;
              case TO_VULN:
                  printf_to_char(ch, "Adds vulnerability to %s.\n\r", imm_bit_name(paf->bitvector));
                  break;
              default:
                  printf_to_char(ch, "Unknown bit %d: %d\n\r", paf->where, paf->bitvector);
                  break;
            }
        }
    }

    if (!obj->enchanted) {
        for (paf = obj->objprototype->affected; paf != NULL; paf = paf->next) {
            printf_to_char(ch, "Affects %s by %d, level %d.\n\r",
                           affect_loc_name(paf->location),
                           paf->modifier, paf->level);
            if (paf->bitvector) {
                switch (paf->where) {
                  case TO_AFFECTS:
                      printf_to_char(ch, "Adds %s affect.\n\r", affect_bit_name(paf->bitvector));
                      break;
                  case TO_OBJECT:
                      printf_to_char(ch, "Adds %s object flag.\n\r", extra_bit_name(paf->bitvector));
                      break;
                  case TO_IMMUNE:
                      printf_to_char(ch, "Adds immunity to %s.\n\r", imm_bit_name(paf->bitvector));
                      break;
                  case TO_RESIST:
                      printf_to_char(ch, "Adds resistance to %s.\n\r", imm_bit_name(paf->bitvector));
                      break;
                  case TO_VULN:
                      printf_to_char(ch, "Adds vulnerability to %s.\n\r", imm_bit_name(paf->bitvector));
                      break;
                  default:
                      printf_to_char(ch, "Unknown bit %d: %d\n\r", paf->where, paf->bitvector);
                      break;
                }
            }
        }
    }
}

/**
 * show statistics for a character
 */
void show_mob_stats(CHAR_DATA *ch, const char *argument)
{
    AFFECT_DATA *paf;
    CHAR_DATA *victim;
    SKILL *skill;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Stat whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    printf_to_char(ch, "Name: %s\n\r", victim->name);
    printf_to_char(ch, "Vnum: %d  Format: %s  Race: %s  Sex: %s  Room: %d\n\r",
                   IS_NPC(victim) ? victim->mob_idx->vnum : 0,
                   IS_NPC(victim) ? "npc" : "pc",
                   race_table[victim->race].name,
                   sex_table[victim->sex].name,
                   victim->in_room == NULL ? 0 : victim->in_room->vnum);

    if (IS_NPC(victim)) {
        printf_to_char(ch, "Count: %d  Killed: %d\n\r",
                       victim->mob_idx->count,
                       victim->mob_idx->killed);
    }

    printf_to_char(ch, "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)  Luck: %d(%d)\n\r",
                   victim->perm_stat[STAT_STR],
                   get_curr_stat(victim, STAT_STR),
                   victim->perm_stat[STAT_INT],
                   get_curr_stat(victim, STAT_INT),
                   victim->perm_stat[STAT_WIS],
                   get_curr_stat(victim, STAT_WIS),
                   victim->perm_stat[STAT_DEX],
                   get_curr_stat(victim, STAT_DEX),
                   victim->perm_stat[STAT_CON],
                   get_curr_stat(victim, STAT_CON),
                   victim->perm_stat[STAT_LUCK],
                   get_curr_stat(victim, STAT_LUCK));

    printf_to_char(ch, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d\n\r",
                   victim->hit, victim->max_hit,
                   victim->mana, victim->max_mana,
                   victim->move, victim->max_move);

    if (!IS_NPC(victim)) {
        printf_to_char(ch, "Practices: %d   Trains: %d",
                       victim->pcdata->practice,
                       victim->pcdata->train);
    }

    if (!IS_NPC(victim))
        printf_to_char(ch, " Gold in Bank: %d  Silver in Bank: %d\n\r",
                       victim->pcdata->gold_in_bank, victim->pcdata->silver_in_bank);

    printf_to_char(ch, "Lv: %d  Class: %s  Gold: %ld  Silver: %ld  Exp: %d\n\r",
                   victim->level,
                   IS_NPC(victim) ? "mobile" : class_table[victim->class].name,
                   victim->gold, victim->silver, victim->exp);

    if (!IS_NPC(victim)) {
        printf_to_char(ch, "Gold in bank: `#%ld pieces   ``Silver in bank:`&%ld pieces``\n\r",
                       victim->pcdata->gold_in_bank,
                       victim->pcdata->silver_in_bank);
    }

    printf_to_char(ch, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
                   GET_AC(victim, AC_PIERCE), GET_AC(victim, AC_BASH),
                   GET_AC(victim, AC_SLASH), GET_AC(victim, AC_EXOTIC));

    printf_to_char(ch, "``Hit: `O%d``  Dam: `!%d``  Saves: %d  Size: %s Position: %s Wimpy: `#%d``\n\r",
                   GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw,
                   (victim->size >= 0) ? size_table[victim->size].name : "unknown", position_table[victim->position].name,
                   victim->wimpy);

    if (IS_NPC(victim)) {
        printf_to_char(ch, "Damage: %dd%d  Message:  %s\n\r", victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE], attack_table[victim->dam_type].noun);
    }

    printf_to_char(ch, "Trusted at level: `#%d`7\n\r", get_trust(victim));
    printf_to_char(ch, "Self Lag Modifier: `P%d%``\n\r", victim->mLag);
    printf_to_char(ch, "Target Lag Modifier: `#%d%``\n\r", victim->tLag);


    printf_to_char(ch, "Fighting: %s\n\r", victim->fighting ? victim->fighting->name : "(none)");

    if (!IS_NPC(victim)) {
        printf_to_char(ch, "Thirst: %d  Hunger: %d  Feed: %d  Full: %d\n\r",
                       victim->pcdata->condition[COND_THIRST],
                       victim->pcdata->condition[COND_HUNGER],
                       victim->pcdata->condition[COND_FEED],
                       victim->pcdata->condition[COND_FULL]);
    }

    printf_to_char(ch, "Carry number: %d  Carry weight: %d\n\r",
                   victim->carry_number,
                   victim->carry_weight / 10);


    if (!IS_NPC(victim)) {
        printf_to_char(ch, "Age: %d  Played: %d  Last Level: %d  Timer: %d\n\r",
                       get_age(victim),
                       (int)(victim->played + globalSystemState.current_time - victim->logon) / 3600,
                       victim->pcdata->last_level,
                       victim->timer);

        printf_to_char(ch, "``Pkills: `!%ld``  Pdeaths: `1%ld``  Mkills: `O%ld`` Mdeaths: `4%ld``\n\r",
                       victim->pcdata->pkills,
                       victim->pcdata->pdeaths,
                       victim->pcdata->mobkills,
                       victim->pcdata->mobdeaths);


        printf_to_char(ch, "Death room: %d \n\r", victim->deathroom);
    }

    printf_to_char(ch, "Act: %s\n\r", act_bit_name(victim->act));

    if (victim->comm)
        printf_to_char(ch, "Comm: %s\n\r", comm_bit_name(victim->comm));

    if (IS_NPC(victim) && victim->off_flags)
        printf_to_char(ch, "Offense: %s\n\r", off_bit_name(victim->off_flags));

    if (victim->imm_flags)
        printf_to_char(ch, "Immune: %s\n\r", imm_bit_name(victim->imm_flags));

    if (victim->res_flags)
        printf_to_char(ch, "Resist: %s\n\r", imm_bit_name(victim->res_flags));

    if (victim->vuln_flags)
        printf_to_char(ch, "Vulnerable: %s\n\r", imm_bit_name(victim->vuln_flags));

    printf_to_char(ch, "Form: %s\n\rParts: %s\n\r",
                   form_bit_name(victim->form),
                   part_bit_name(victim->parts));

    if (victim->affected_by)
        printf_to_char(ch, "Affected by: %s\n\r", affect_bit_name(victim->affected_by));

    printf_to_char(ch, "Master: %s  Leader: %s  Pet: %s\n\r",
                   victim->master ? victim->master->name : "(none)",
                   victim->leader ? victim->leader->name : "(none)",
                   victim->pet ? victim->pet->name : "(none)");
    if (!IS_NPC(victim))
        printf_to_char(ch, "Security: %d.\n\r", victim->pcdata->security);

    printf_to_char(ch, "Short description: %s\n\rLong  description: %s",
                   victim->short_descr,
                   victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r");

    if (IS_NPC(victim) && victim->mobmem != NULL) {
        printf_to_char(ch, "%s is `!pissed`` at: %s\n\r",
                       victim->short_descr,
                       victim->mobmem->name);
    }

    if (IS_NPC(victim) && victim->mob_wuss != NULL) {
        printf_to_char(ch, "%s is `#scared`` of: %s\n\r",
                       victim->short_descr,
                       victim->mob_wuss->name);
    }

    if (IS_NPC(victim) && (victim->mobmem == NULL)
        && (victim->mob_wuss == NULL))
        printf_to_char(ch, "%s's `8yin`` and `&yang`` are balanced.\n\r",
                       victim->short_descr);

    for (paf = victim->affected; paf != NULL; paf = paf->next) {
        if ((skill = resolve_skill_affect(paf)) != NULL) {
            printf_to_char(ch, "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
                           skill->name,
                           affect_loc_name(paf->location),
                           paf->modifier,
                           paf->duration,
                           affect_bit_name(paf->bitvector),
                           paf->level);
        }
    }
}
