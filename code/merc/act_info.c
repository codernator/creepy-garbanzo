#include "merc.h"
#include "character.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "ansi.h"
#include "interp.h"

#ifndef S_SPLINT_S
#include <ctype.h>
#endif

#include <stdio.h>
#include <string.h>
#include "help.h"
#include "libfile.h"


extern void string_append(struct char_data * ch, char **string);
extern char *password_encrypt(const char *plain);
extern bool password_matches(char *existing, const char *plain);
extern int password_acceptance(const char *plain);
extern const char *password_accept_message(int code);


void print_weather(struct char_data * ch);



/*
 * Local functions.
 */
extern void show_char_to_char(struct char_data * list, struct char_data * ch);
extern void show_char_to_char_2(struct char_data * victim, struct char_data * ch);

#define MAX_NEST        100
static struct gameobject *rgObjNest[MAX_NEST];
extern void fread_char(struct char_data * ch, FILE * fp);


void do_help(struct char_data *ch, const char *argument)
{
    char topic[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, topic);
    show_help(ch->desc, topic, argument);
}


/* changes your scroll */
void do_scroll(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        if (ch->lines == 0) {
            send_to_char("You do not page long messages.\n\r", ch);
        } else {
            sprintf(buf, "You currently display %d lines per page.\n\r",
                    ch->lines + 2);
            send_to_char(buf, ch);
        }
        return;
    }

    if (!is_number(arg)) {
        send_to_char("Give me a number or I'll break your arm.\n\r", ch);
        return;
    }

    lines = parse_int(arg);

    if (lines == 0) {
        send_to_char("Paging disabled.\n\r", ch);
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 50) {
        send_to_char("You must provide a reasonable number.\n\r", ch);
        return;
    }

    sprintf(buf, "Scroll set to %d lines.\n\r", lines);
    send_to_char(buf, ch);
    ch->lines = lines - 2;
}

/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "motd", NULL);
}

void do_imotd(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "imotd", NULL);
}

void do_rules(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "rules", NULL);
}

void do_story(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "story", NULL);
}

void do_wizlist(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "wizlist", NULL);
}

/* RT this following section holds all the auto commands from ROM, as well as
 * replacements for config */
void do_autolist(struct char_data *ch, const char *argument)
{
    /* lists most player flags */
    if (IS_NPC(ch))
        return;

    send_to_char("   action     status\n\r", ch);
    send_to_char("---------------------\n\r", ch);

    send_to_char("autoassist     ", ch);
    if (IS_SET(ch->act, PLR_AUTOASSIST))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autoeq         ", ch);
    if (IS_SET(ch->act, PLR_AUTOEQ))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autoexit       ", ch);
    if (IS_SET(ch->act, PLR_AUTOEXIT))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autogold       ", ch);
    if (IS_SET(ch->act, PLR_AUTOGOLD))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autoloot       ", ch);
    if (IS_SET(ch->act, PLR_AUTOLOOT))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autosac        ", ch);
    if (IS_SET(ch->act, PLR_AUTOSAC))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autosplit      ", ch);
    if (IS_SET(ch->act, PLR_AUTOSPLIT))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("compact mode   ", ch);
    if (IS_SET(ch->comm, COMM_COMPACT))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("autoticks      ", ch);
    if (IS_SET(ch->comm, COMM_TICKS))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("prompt         ", ch);
    if (IS_SET(ch->comm, COMM_PROMPT))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    send_to_char("combine items  ", ch);
    if (IS_SET(ch->comm, COMM_COMBINE))
        send_to_char("ON\n\r", ch);
    else
        send_to_char("OFF\n\r", ch);

    if (!IS_SET(ch->act, PLR_CANLOOT))
        send_to_char("Your corpse is safe from thieves.\n\r", ch);
    else
        send_to_char("Your corpse may be looted.\n\r", ch);

    if (IS_SET(ch->act, PLR_NOSUMMON))
        send_to_char("You cannot be summoned.\n\r", ch);
    else
        send_to_char("You can be summoned.\n\r", ch);

    if (IS_SET(ch->act, PLR_NOFOLLOW))
        send_to_char("You do not welcome followers.\n\r", ch);
    else
        send_to_char("You accept followers.\n\r", ch);
}

void do_autoassist(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOASSIST)) {
        send_to_char("Autoassist removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOASSIST);
    } else {
        send_to_char("You will now assist when needed.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOASSIST);
    }
}

void do_autoeq(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOEQ)) {
        send_to_char("Empty eq slots will no longer be displayed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOEQ);
    } else {
        send_to_char("Empty eq slots will now be displayed.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOEQ);
    }
}

void do_autoexit(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOEXIT)) {
        send_to_char("Exits will no longer be displayed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOEXIT);
    } else {
        send_to_char("Exits will now be displayed.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOEXIT);
    }
}

void do_autogold(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOGOLD)) {
        send_to_char("Autogold removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOGOLD);
    } else {
        send_to_char("Automatic gold looting set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOGOLD);
    }
}

void do_autoloot(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOLOOT)) {
        send_to_char("Autolooting removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOLOOT);
    } else {
        send_to_char("Automatic corpse looting set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOLOOT);
    }
}

void do_autosac(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOSAC)) {
        send_to_char("Autosacrificing removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOSAC);
    } else {
        send_to_char("Automatic corpse sacrificing set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOSAC);
    }
}

void do_autosplit(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOSPLIT)) {
        send_to_char("Autosplitting removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOSPLIT);
    } else {
        send_to_char("Automatic gold splitting set.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOSPLIT);
    }
}

void do_brief(struct char_data *ch, const char *argument)
{
    if (IS_SET(ch->comm, COMM_BRIEF)) {
        send_to_char("Full descriptions activated.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_BRIEF);
    } else {
        send_to_char("Short descriptions activated.\n\r", ch);
        SET_BIT(ch->comm, COMM_BRIEF);
    }
}

void do_compact(struct char_data *ch, const char *argument)
{
    if (IS_SET(ch->comm, COMM_COMPACT)) {
        send_to_char("Compact mode removed.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_COMPACT);
    } else {
        send_to_char("Compact mode set.\n\r", ch);
        SET_BIT(ch->comm, COMM_COMPACT);
    }
}

void send_color_status(struct char_data *ch, char *a, char *c, byte b)
{
    sprintf(c, "%s %s %s\n\r", color_table[b], a, ANSI_NORMAL);
    send_to_char(c, ch);
}

void do_autoticks(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->comm, COMM_TICKS)) {
        send_to_char("You will no longer see ticks.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_TICKS);
    } else {
        send_to_char("You will now see the ticks.\n\r", ch);
        SET_BIT(ch->comm, COMM_TICKS);
    }
}

void do_color(struct char_data *ch, const char *argument)
{
    unsigned int i = 0;
    byte a = (byte)0;
    byte b = (byte)0;
    char cbuf[MAX_INPUT_LENGTH];

    DENY_NPC(ch);

    if (argument[0] == '\0') {
        if (ch->use_ansi_color) {
            ch->use_ansi_color = false;
            send_to_char("Ansi color turned off.\n\r", ch);
        } else {
            ch->use_ansi_color = true;
            send_to_char("Ansi color turned on.\n\r", ch);
        }
    } else {
        argument = one_argument(argument, cbuf);

        if (str_cmp(cbuf, "codes") == 0) {
            send_to_char("These are the numbers/symbols to be used for color codes.\n\r\n\r", ch);
            send_to_char("`1Red uses the color code: 1``\n\r`2Green uses the color code: 2``\n\r`3Brown uses the color code: 3``\n\r`4Blue uses the color code: 4``\n\r`5Purple uses the color code: 5`` \n\r`6Cyan uses the color code: 6``\n\r", ch);
            send_to_char("`7Grey uses the color code: 7``\n\r", ch);
            send_to_char("`8Dark grey uses the code: 8``\n\r`!Bright red uses the code: !``\n\r`@Bright green uses the code: @``\n\r`#Yellow uses the color code: #``\n\r`OBright blue uses the code: O``\n\r`PMagneta uses the code: P``\n\r", ch);
            send_to_char("`^Bright cyan uses the code: ^``\n\r`&White uses $color code: &``\n\r\n\r", ch);
            send_to_char_ascii("To use these color in things like your title, the following code and\n\rformat is required. `<color-code> <text>``  Note the `` at the end.\n\r", ch);
            send_to_char("This is used to clear the color so that it does not run onto the next\n\rline.\n\r\n\r", ch);
            send_to_char_ascii("For example:  emote `!grins`` and `#laughs``.\n\r", ch);
            send_to_char("Will produce: <Your name> `!grins`` and `#laughs``.\n\r", ch);
        } else if (str_cmp(cbuf, "list") == 0) {
            send_to_char("Listing colours.\n\r", ch);
            while (color_table[i]) {
                sprintf(cbuf, "%scolor %d%s\n\r", color_table[i], (int)i, ANSI_NORMAL);
                send_to_char(cbuf, ch);
                i++;
            }
        } else if (str_cmp(cbuf, "show") == 0) {
            send_to_char("Listing color setup.\n\r# of option\n\r", ch);
            send_color_status(ch, "1 combat melee(self)", cbuf, ch->pcdata->color_combat_s);
            send_color_status(ch, "2 combat melee(opponent)", cbuf, ch->pcdata->color_combat_o);
            send_color_status(ch, "3 combat condition(self)", cbuf, ch->pcdata->color_combat_condition_s);
            send_color_status(ch, "4 combat condition(opponent)", cbuf, ch->pcdata->color_combat_condition_o);
            send_color_status(ch, "5 wizi mobs", cbuf, ch->pcdata->color_wizi);
            send_color_status(ch, "6 invis mobs", cbuf, ch->pcdata->color_invis);
            send_color_status(ch, "7 hidden mobs", cbuf, ch->pcdata->color_hidden);
            send_color_status(ch, "8 charmed mobs", cbuf, ch->pcdata->color_charmed);
            send_color_status(ch, "9 hp", cbuf, ch->pcdata->color_hp);
            send_color_status(ch, "10 mana", cbuf, ch->pcdata->color_mana);
            send_color_status(ch, "11 movement", cbuf, ch->pcdata->color_move);
            send_color_status(ch, "12 say", cbuf, ch->pcdata->color_say);
            send_color_status(ch, "13 tell", cbuf, ch->pcdata->color_tell);
            send_color_status(ch, "14 reply", cbuf, ch->pcdata->color_reply);
        } else if (str_cmp(cbuf, "set") == 0) { /* if cbuf */
            argument = one_argument(argument, cbuf);
            a = parse_byte2(cbuf, (byte)0, (byte)15);
            argument = one_argument(argument, cbuf);

            if (cbuf[0] != '\0') { /*cbuf */
                b = parse_byte(cbuf);
                while (color_table[i])
                    i++;

                if (((unsigned int)b < i)) {/* check for valid color */
                    switch (a) { /* switch a */
                      case 1:
                          ch->pcdata->color_combat_s = b;
                          send_to_char("`acombat melee(self)``\n\r", ch);
                          break;
                      case 2:
                          ch->pcdata->color_combat_o = b;
                          send_to_char("`Acombat melee(opponent)``\n\r", ch);
                          break;
                      case 3:
                          ch->pcdata->color_combat_condition_s = b;
                          send_to_char("`Ccombat condition(self)``\n\r", ch);
                          break;
                      case 4:
                          ch->pcdata->color_combat_condition_o = b;
                          send_to_char("`Dcombat condition(opponent)``\n\r", ch);
                          break;
                      case 5:
                          ch->pcdata->color_wizi = b;
                          send_to_char("`wwizi mobs/exits``\n\r", ch);
                          break;
                      case 6:
                          ch->pcdata->color_invis = b;
                          send_to_char("`iinvis``\n\r", ch);
                          break;
                      case 7:
                          ch->pcdata->color_hidden = b;
                          send_to_char("`hhidden mobs``\n\r", ch);
                          break;
                      case 8:
                          ch->pcdata->color_charmed = b;
                          send_to_char("`ccharmed mobs``\n\r", ch);
                          break;
                      case 9:
                          ch->pcdata->color_hp = b;
                          send_to_char("`Hhp``\n\r", ch);
                          break;
                      case 10:
                          ch->pcdata->color_mana = b;
                          send_to_char("`Mmana``\n\r", ch);
                          break;
                      case 11:
                          ch->pcdata->color_move = b;
                          send_to_char("`Vmovement``\n\r", ch);
                          break;
                      case 12:
                          ch->pcdata->color_say = b;
                          send_to_char("`ssay``\n\r", ch);
                          break;
                      case 13:
                          ch->pcdata->color_tell = b;
                          send_to_char("`ttell``\n\r", ch);
                          break;
                      case 14:
                          ch->pcdata->color_reply = b;
                          send_to_char("`rreply``\n\r", ch);
                          break;
                      default:
                          send_to_char("Change color for which option?\n\r", ch);
                          break;
                    }       /* switch a */
                }               /* if(b...) */
                else {
                    send_to_char("What color!!!\n", ch);
                }
            }       /* cbuf */
        }               /* if(set==show) */
        else {
            send_to_char("Color what?\n", ch);
        }
    }
}


void do_show(struct char_data *ch, /*@unused@*/const char *argument)
{
    if (IS_SET(ch->comm, COMM_SHOW_AFFECTS)) {
        send_to_char("Affects will no longer be shown in score.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_SHOW_AFFECTS);
    } else {
        send_to_char("Affects will now be shown in score.\n\r", ch);
        SET_BIT(ch->comm, COMM_SHOW_AFFECTS);
    }
}

void do_prompt(struct char_data *ch, const char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm, COMM_PROMPT)) {
            send_to_char("You will no longer see prompts.\n\r", ch);
            REMOVE_BIT(ch->comm, COMM_PROMPT);
        } else {
            send_to_char("You will now see prompts.\n\r", ch);
            SET_BIT(ch->comm, COMM_PROMPT);
        }
        return;
    }

    if (!strcmp(argument, "all")) {
        strcpy(buf, "<%hhp %mm %vmv> ");
    } else {
        (void)strncpy(buf, argument, 150);
        smash_tilde(buf);
        if (str_suffix("%c", buf))
            strcat(buf, " ");
    }

    free_string(ch->prompt);
    ch->prompt = str_dup(buf);
    printf_to_char(ch, "Prompt set to```8: ``%s\n\r", ch->prompt);
    return;
}

void do_combine(struct char_data *ch, const char *argument)
{
    if (IS_SET(ch->comm, COMM_COMBINE)) {
        send_to_char("Long inventory selected.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_COMBINE);
    } else {
        send_to_char("Combined inventory selected.\n\r", ch);
        SET_BIT(ch->comm, COMM_COMBINE);
    }
}

void do_noloot(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_CANLOOT)) {
        send_to_char("Your corpse is now safe from `@N`2ecrophileacks``.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_CANLOOT);
    } else {
        send_to_char("Your corpse may now be `!R`1aped`` and `@P`2illeged``.\n\r", ch);
        SET_BIT(ch->act, PLR_CANLOOT);
    }
}

void do_nofollow(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_NOFOLLOW)) {
        send_to_char("You now accept followers.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_NOFOLLOW);
    } else {
        if (IS_AFFECTED(ch, AFF_CHARM)) {
            send_to_char("But you're just so happy the way things are ..\n\r", ch);
            return;
        }
        send_to_char("You no longer accept followers.\n\r", ch);
        SET_BIT(ch->act, PLR_NOFOLLOW);
        die_follower(ch);
    }
}

void do_nosummon(struct char_data *ch, const char *argument)
{
    if (IS_NPC(ch)) {
        if (IS_SET(ch->imm_flags, IMM_SUMMON)) {
            send_to_char("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT(ch->imm_flags, IMM_SUMMON);
        } else {
            send_to_char("You are now immune to summoning.\n\r", ch);
            SET_BIT(ch->imm_flags, IMM_SUMMON);
        }
    } else {
        if (IS_SET(ch->act, PLR_NOSUMMON)) {
            send_to_char("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT(ch->act, PLR_NOSUMMON);
        } else {
            send_to_char("You are now immune to summoning.\n\r", ch);
            SET_BIT(ch->act, PLR_NOSUMMON);
        }
    }
}

void do_lore(struct char_data *ch, const char *argument)
{
    struct gameobject *obj;
    struct dynamic_skill *skill;
    char arg[MAX_INPUT_LENGTH];
    int percent;

    if ((skill = skill_lookup("lore")) == NULL) {
        send_to_char("Huh?", ch);
        return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("What do you want to lore?\n\r", ch);
        return;
    }

    obj = get_obj_carry(ch, arg);
    if (obj == NULL) {
        send_to_char("You aren't carrying that.\n\r", ch);
        return;
    }

    percent = get_learned_percent(ch, skill);
    if (!IS_NPC(ch) && number_percent() > percent) {
        act("You look at $p, but you can't find out any additional information.",
            ch, obj, NULL, TO_CHAR);
        act("$n looks at $p but cannot find out anything.", ch, obj, NULL, TO_ROOM);
        return;
    } else {
        act("$n studies $p, discovering all of its hidden powers.", ch, obj, NULL, TO_ROOM);

        identify_item(ch, obj);
        check_improve(ch, skill, true, 4);
    }
}

void do_glance(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    struct char_data *victim;

    if (ch->desc == NULL)
        return;

    if (ch->position < POS_SLEEPING) {
        send_to_char("You can't see anything but stars!\n\r", ch);
        return;
    }

    if (ch->position == POS_SLEEPING) {
        send_to_char("You can't see anything, you're sleeping!\n\r", ch);
        return;
    }

    if (character_is_blind(ch)) {
        send_to_char("You are blind!", ch);
        return;
    }

    if (!IS_NPC(ch)
        && !IS_SET(ch->act, PLR_HOLYLIGHT)
        && room_is_dark(ch, ch->in_room)) {
        send_to_char("It is pitch `8black...``\n\r", ch);
        show_char_to_char(ch->in_room->people, ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0') {
        send_to_char("You glance around into thin air.", ch);
        return;
    }


    if ((victim = get_char_room(ch, arg1)) != NULL) {
        show_char_to_char_2(victim, ch);
        return;
    } else {
        send_to_char("That person isn't here.\n\r", ch);
        return;
    }
}

void do_look(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    struct gameobject *obj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int door;
    int number;


    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    number = number_argument(arg1, arg3);

    if (arg1[0] == '\0' || !str_cmp(arg1, "auto")) {
        look_room(ch, ch->in_room);
        return;
    }

    if (!str_cmp(arg1, "i") || !str_cmp(arg1, "in") || !str_cmp(arg1, "on")) {
        /* 'look in' */
        if (arg2[0] == '\0') {
            send_to_char("Look in what?\n\r", ch);
            return;
        }

        if ((obj = get_obj_here(ch, arg2)) == NULL) {
            send_to_char("You do not see that here.\n\r", ch);
            return;
        }

        look_object(ch, obj, argument);
        return;
    }

    if ((victim = get_char_room(ch, arg1)) != NULL) {
        look_character(ch, victim);
        return;
    }

    if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north")) {
        door = 0;
    } else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east")) {
        door = 1;
    } else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south")) {
        door = 2;
    } else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west")) {
        door = 3;
    } else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up")) {
        door = 4;
    } else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down")) {
        door = 5;
    } else {
        door = -1;
    }

    if (door > -1) {
        look_direction(ch, door);
    } else {
        look_extras(ch, arg3, number);
    }

    return;
}


void do_examine(struct char_data *ch, const char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    struct gameobject *obj;

    (void)one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Aww no one wants to play doctor with you?\n\r", ch);
        return;
    }

    do_look(ch, arg);

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        switch (obj->item_type) {
          default:
              break;

          case ITEM_MONEY:
              if (obj->value[0] == 0) {
                  if (obj->value[1] == 0)
                      sprintf(buf, "Odd...there's no coins in the pile.\n\r");
                  else if (obj->value[1] == 1)
                      sprintf(buf, "Wow. One gold coin.\n\r");
                  else
                      sprintf(buf, "There are %ld gold coins in the pile.\n\r", obj->value[1]);
              } else if (obj->value[1] == 0) {
                  if (obj->value[0] == 1)
                      sprintf(buf, "Wow. One silver coin.\n\r");
                  else
                      sprintf(buf, "There are %ld silver coins in the pile.\n\r", obj->value[0]);
              } else {
                  sprintf(buf, "There are %ld gold and %ld silver coins in the pile.\n\r", obj->value[1], obj->value[0]);
              }
              send_to_char(buf, ch);
              break;

          case ITEM_DRINK_CON:
          case ITEM_CONTAINER:
          case ITEM_CORPSE_NPC:
          case ITEM_CORPSE_PC:
              sprintf(buf, "in %s", argument);
              do_look(ch, buf);
        }
    }

    return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(struct char_data *ch, const char *argument)
{
    extern char *const dir_name[];
    struct exit_data *pexit;
    char buf[MAX_STRING_LENGTH];
    bool found;
    bool fAuto;
    int door;
    bool dClosed;
    char exit_name[10];


    fAuto = !str_cmp(argument, "auto");

    if (character_is_blind(ch)) {
        send_to_char("You are blind!", ch);
        return;
    }

    if (fAuto)
        sprintf(buf, "`6[`^Exits`6:``");
    else if (IS_IMMORTAL(ch))
        sprintf(buf, "Obvious exits from room %ld`8:``\n\r", ch->in_room->vnum);
    else
        sprintf(buf, "Obvious exits`8:``\n\r");

    found = false;
    for (door = 0; door <= 5; door++) {
        if ((pexit = ch->in_room->exit[door]) != NULL
            && pexit->u1.to_room != NULL
            && can_see_room(ch, pexit->u1.to_room)) {
            found = true;
            dClosed = IS_SET(pexit->exit_info, EX_CLOSED);
            if (fAuto) {
                strcat(buf, " `^");
                if (dClosed) strcat(buf, "("); strcat(buf, dir_name[door]);
                if (dClosed) strcat(buf, ")");
            } else {
                sprintf(buf + strlen(buf), "%-5s - %s",
                        capitalize(dir_name[door]),
                        room_is_dark(ch, pexit->u1.to_room)
                        ? "Too dark to tell"
                        : pexit->u1.to_room->name
                       );
                if (IS_IMMORTAL(ch))
                    sprintf(buf + strlen(buf), "(room %ld)\n\r", pexit->u1.to_room->vnum);
                else
                    sprintf(buf + strlen(buf), "\n\r");
            }
        }
    }

    if (!found)
        strcat(buf, fAuto ? " `&none``" : "`&None``.\n\r");

    if (fAuto)
        strcat(buf, "`6]``\n\r");

    send_to_char(buf, ch);

    for (door = 0; door <= 5; door++) {
        if ((pexit = ch->in_room->exit[door]) != NULL
            && pexit->u1.to_room != NULL
            && can_see_room(ch, pexit->u1.to_room)) {
            dClosed = IS_SET(pexit->exit_info, EX_CLOSED);

            if (door == 0) {
                if (dClosed) sprintf(exit_name, "(north)"); else sprintf(exit_name, "north");
            } else if (door == 1) {
                if (dClosed) sprintf(exit_name, "(south)"); else sprintf(exit_name, "south");
            } else if (door == 2) {
                if (dClosed) sprintf(exit_name, "(east)"); else sprintf(exit_name, "east");
            } else if (door == 3) {
                if (dClosed) sprintf(exit_name, "(west)"); else sprintf(exit_name, "west");
            } else if (door == 4) {
                if (dClosed) sprintf(exit_name, "(up)"); else sprintf(exit_name, "up");
            } else {
                if (dClosed) sprintf(exit_name, "(down)"); else sprintf(exit_name, "down");
            }

            found = true;
            return;
        }
        return;
    }
}

void do_worth(struct char_data *ch, const char *argument)
{
    if (!IS_NPC(ch)) printf_to_char(ch, "Extended Experience Points: %ld, Extended Level: %d\n\r",
                                    ch->pcdata->extendedexp, ch->pcdata->extendedlevel);
    if (IS_NPC(ch)) {
        printf_to_char(ch, "You have %ld gold and %ld silver.\n\r", ch->gold, ch->silver);
        return;
    }

    printf_to_char(ch, "You have ```#%ld ``gold, ```&%ld ``silver, and ```@%d experience ```8(```1%d ``exp to level```8)``.\n\r",
                   ch->gold, ch->silver, ch->exp,
                   (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);

    printf_to_char(ch, "You have `#%ld`` gold and `&%ld`` silver in the `2b`@ank``.\n\r",
                   ch->pcdata->gold_in_bank, ch->pcdata->silver_in_bank);

    return;
}


void do_score(struct char_data *ch, const char *argument)
{
    char hours[MAX_STRING_LENGTH];
    char mins[MAX_STRING_LENGTH];
    char secs[MAX_STRING_LENGTH];
    int i;

    printf_to_char(ch, "You are %s%s, level %d, ", ch->name, IS_NPC(ch) ? "" : ch->pcdata->title, ch->level);

    if (!IS_NPC(ch)) {
        printf_to_char(ch, "%d years old``.\n\r", get_age(ch));
    }

    send_to_char("`O=================================================================``\n\r", ch);


    printf_to_char(ch, "`7You have played a total of:`6 %d `7hours,`6 %d `7minutes,`6 %d `7seconds``\n\r", get_hours_played(ch), get_minutes_played(ch), get_seconds_played(ch));

    sprintf(hours, "%2.2d", get_session_hours(ch));
    sprintf(mins, "%2.2d", get_session_minutes(ch));
    sprintf(secs, "%2.2d", get_session_seconds(ch));

    printf_to_char(ch, "`7This session:`6 %s `7hours,`6 %s `7minutes,`6 %s `7seconds``\n\r", hours, mins, secs);

    if (get_trust(ch) != ch->level) {
        printf_to_char(ch, "You are trusted at level ```#%d``.\n\r", get_trust(ch));
    }

    printf_to_char(ch, "Race```8: ``%s  Sex```8: ``%s  Class`8: ``%s\n\r", race_table[ch->race].name, ch->sex == 0 ? "Pat" : ch->sex == 1 ? "dude" : "chick", class_table[ch->class].name);
    printf_to_char(ch, "You have ```!%d``/```1%d ``hit, ```@%d``/```2%d ``mana, ```O%d``/```4%d ``movement.\n\r", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move);

    if (!IS_NPC(ch)) {
        printf_to_char(ch, "You have ```6%d ``practices and ```6%d ``training sessions.\n\r", ch->pcdata->practice, ch->pcdata->train);
    }

    printf_to_char(ch, "You are carrying ```P%d``/```5%d ``items with weight ```P%d``/```5%d ``pounds.\n\r", ch->carry_number, can_carry_n(ch), ch->carry_weight / 10, can_carry_w(ch) / 10);

    printf_to_char(ch,
                   "Str```8: ``%d```8(``%d```8)  ``Int```8: ``%d```8(``%d```8)  ``Wis```8: ``%d```8(``%d```8)  ``Dex```8: ``%d```8(``%d```8)  ``Con```8: ``%d```8(``%d```8)  ``Luck```8: ``%d```8(``%d```8)``\n\r",
                   ch->perm_stat[STAT_STR],
                   get_curr_stat(ch, STAT_STR),
                   ch->perm_stat[STAT_INT],
                   get_curr_stat(ch, STAT_INT),
                   ch->perm_stat[STAT_WIS],
                   get_curr_stat(ch, STAT_WIS),
                   ch->perm_stat[STAT_DEX],
                   get_curr_stat(ch, STAT_DEX),
                   ch->perm_stat[STAT_CON],
                   get_curr_stat(ch, STAT_CON),
                   ch->perm_stat[STAT_LUCK],
                   get_curr_stat(ch, STAT_LUCK));

    send_to_char("`O=================================================================``\n\r", ch);

    printf_to_char(ch, "You have scored ```@%d ``exp, and have ```#%u ``gold and ```&%u ``silver coins.\n\r", ch->exp, ch->gold, ch->silver);

    send_to_char("`O=================================================================``\n\r", ch);

    /* RT shows exp to level */
    if (!IS_NPC(ch) && ch->level < LEVEL_HERO) {
        printf_to_char(ch, "You need ```!%d ``exp to level.\n\r", ((ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp));
    }

    printf_to_char(ch, "Wimpy set to ```1%d ``hit points.\n\r", ch->wimpy);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
        send_to_char("You are thirsty.\n\r", ch);

    switch (ch->position) {
      case POS_DEAD:
          send_to_char("You are ```1D```8E```1A```8D``!!\n\r", ch);
          break;
      case POS_MORTAL:
          send_to_char("You are ```!mortally wounded``.\n\r", ch);
          break;
      case POS_INCAP:
          send_to_char("You are ```1incapacitated``.\n\r", ch);
          break;
      case POS_STUNNED:
          send_to_char("You are ```Pstunned``.\n\r", ch);
          break;
      case POS_SLEEPING:
          send_to_char("You are sleeping.\n\r", ch);
          break;
      case POS_RESTING:
          send_to_char("You are resting.\n\r", ch);
          break;
      case POS_STANDING:
          send_to_char("You are standing.\n\r", ch);
          break;
      case POS_FIGHTING:
          send_to_char("You are ```#fighting``.\n\r", ch);
          break;
    }

    send_to_char("`O=================================================================``\n\r", ch);

    /* print AC values */
    if (ch->level >= 25) {
        printf_to_char(ch, "Armor`8: ``pierce`8: `1%ld  ``bash`8: `1%ld  ``slash`8: `1%ld  ``magic`8: `1%ld``\n\r",
                       GET_AC(ch, AC_PIERCE),
                       GET_AC(ch, AC_BASH),
                       GET_AC(ch, AC_SLASH),
                       GET_AC(ch, AC_EXOTIC));
    }
    if (ch->level <= 25) {
        for (i = 0; i < 4; i++) {
            char *temp;

            switch (i) {
              case (AC_PIERCE):
                  temp = "piercing";
                  break;
              case (AC_BASH):
                  temp = "bashing";
                  break;
              case (AC_SLASH):
                  temp = "slashing";
                  break;
              case (AC_EXOTIC):
                  temp = "magic";
                  break;
              default:
                  temp = "error";
                  break;
            }

            send_to_char("You are ", ch);

            if (GET_AC(ch, i) >= 101)
                printf_to_char(ch, "hopelessly vulnerable to %s.\n\r", temp);
            else if (GET_AC(ch, i) >= 80)
                printf_to_char(ch, "defenseless against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= 50)
                printf_to_char(ch, "barely protected from %s.\n\r", temp);
            else if (GET_AC(ch, i) >= 25)
                printf_to_char(ch, "slightly armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= 0)
                printf_to_char(ch, "somewhat armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= 0)
                printf_to_char(ch, "armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -25)
                printf_to_char(ch, "well-armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -50)
                printf_to_char(ch, "very well-armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -80)
                printf_to_char(ch, "heavily armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -110)
                printf_to_char(ch, "superbly armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -150)
                printf_to_char(ch, "almost invulnerable to %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -250)
                printf_to_char(ch, "divinely armored against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -500)
                printf_to_char(ch, "near invinceable against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -1000)
                printf_to_char(ch, "armored like a tank against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -3000)
                printf_to_char(ch, "armored like an immortal against %s.\n\r", temp);
            else if (GET_AC(ch, i) >= -30000)
                printf_to_char(ch, "armored beyond belief against %s.\n\r", temp);
            else
                printf_to_char(ch, "an immortal.  Damage from %s means squat.\n\r", temp);
        }
    }

    if (ch->imm_flags)
        printf_to_char(ch, "Immune: %s\n\r", imm_bit_name(ch->imm_flags));

    if (ch->res_flags)
        printf_to_char(ch, "Resist: %s\n\r", imm_bit_name(ch->res_flags));

    if (ch->vuln_flags)
        printf_to_char(ch, "Vulnerable: %s\n\r", imm_bit_name(ch->vuln_flags));

    if (ch->affected_by)
        printf_to_char(ch, "Affected by: %s\n\r", affect_bit_name(ch->affected_by));
    if (ch->mLag != 0)
        printf_to_char(ch, "My Lag modified by: `P%d%``\n\r", ch->mLag);
    if (ch->tLag != 0)
        printf_to_char(ch, "Their Lag modified by: `#%d%``\n\r", ch->tLag);
    if (!IS_NPC(ch)) {
        send_to_char("`O=================================================================``\n\r", ch);


        printf_to_char(ch, "You have `!killed`` `P%ld`` mobs, and have been `!killed by`` `5%ld`` mobs.\n\r", ch->pcdata->mobkills, ch->pcdata->mobdeaths);
        printf_to_char(ch, "You have `!killed`` `P%ld`` players, and have been `!killed by`` `5%ld`` players.\n\r", ch->pcdata->pkills, ch->pcdata->pdeaths);
    }

    /* RT wizinvis and holy light */
    if (IS_IMMORTAL(ch)) {
        send_to_char("Holy Light```8: ``", ch);
        if (IS_SET(ch->act, PLR_HOLYLIGHT))
            send_to_char("on", ch);
        else
            send_to_char("off", ch);

        if (ch->invis_level) {
            printf_to_char(ch, "  ```@W```Pi```@Z```Pi```8: ``level %d", ch->invis_level);
        }

        if (ch->incog_level) {
            printf_to_char(ch, "  ```6Incog```^nito```8: ``level %d", ch->incog_level);
        }
        send_to_char("\n\r", ch);
    }

    if (ch->level >= 15) {
        printf_to_char(ch, "```^Hitroll```6: ``%d  ```^Damroll```6: ``%d ```^Saves```6: ``%d.\n\r", GET_HITROLL(ch), GET_DAMROLL(ch), ch->saving_throw);
    }

    if (!IS_NPC(ch)) 
        printf_to_char(ch, "Extended Experience Points: %ld, Extended Level: %d\n\r", ch->pcdata->extendedexp, ch->pcdata->extendedlevel); 

    if (IS_SET(ch->comm, COMM_SHOW_AFFECTS))
        do_affects(ch, "");
}

void do_affects(struct char_data *ch, const char *argument)
{
    struct affect_data *paf;
    struct affect_data *paf_last = NULL;
    struct dynamic_skill *skill;
    char buf[MAX_STRING_LENGTH];

    if (ch->affected != NULL) {
        send_to_char("You are affected by the following spells```8:``\n\r", ch);
        for (paf = ch->affected; paf != NULL; paf = paf->next) {
            if ((skill = resolve_skill_affect(paf)) == NULL)
                continue;

            if (paf_last != NULL
                && paf->type == paf_last->type) {
                if (ch->level >= 20)
                    sprintf(buf, "                       ");
                else
                    continue;
            } else {
                sprintf(buf, "Spell```8: ```O%-16s``", skill->name);
            }

            send_to_char(buf, ch);

            if (ch->level >= 20) {
                sprintf(buf, "```8: ``modifies ```!%s ``by ```1%ld`` ",
                        affect_loc_name((long)paf->location),
                        paf->modifier);
                send_to_char(buf, ch);

                if (paf->duration == -1)
                    sprintf(buf, "```&FOREVER!!!``");
                else
                    sprintf(buf, "for ```O%d ``hours.", paf->duration);

                send_to_char(buf, ch);
            }

            send_to_char("\n\r", ch);
            paf_last = paf;
        }
    } else {
        send_to_char("You are not affected by any spells``.\n\r", ch);
    }

    return;
}



char *const day_name[] =
{
    "the Moon",	  "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

char *const month_name[] =
{
    "Winter",	      "the Winter Wolf",      "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring",	      "Nature",		 "Futility",	   "the Dragon",
    "the Sun",	      "the Heat",	      "the Battle",	 "the Dark Shades","the Shadows",
    "the Long Shadows",   "the Ancient Darkness", "the Great Evil"
};

void do_time(struct char_data *ch, const char *argument)
{
    char *suf;
    int day;

    day = globalGameState.gametime->day + 1;
    switch (day) {
      case 1:
          suf = "st";
          break;
      case 2:
          suf = "rd";
          break;
      case 3:
          suf = "rd";
          break;
      default:
          suf = "th";
          break;
    }

    /*
    */

    printf_to_char(ch, "\n\rIt is the Day of %s, %d%s day in the Month of %s.\n\r",
                   day_name[day % 7],
                   day,
                   suf,
                   month_name[globalGameState.gametime->month]);

    printf_to_char(ch, "The current time is %d o'clock %s.\n\r",
                   (globalGameState.gametime->hour % 12 == 0) ? 12 : globalGameState.gametime->hour % 12,
                   globalGameState.gametime->hour >= 12 ? "pm" : "am");

    send_to_char("\n\r\n\r`1-`!-----------------------------------------------------------------------------`1-``\n\r", ch);

    printf_to_char(ch, "This `#B`Pa`@d `#T`Pr`@i`#p`` kicked in at `O%s``\r"
                   "The `#B`Pa`@d `#T`Pr`@i`#p`` Server clock is screaming, `&%s``\r",
                   globalSystemState.boot_time, (char *)ctime(&globalSystemState.current_time));

    return;
}

void do_inventory(struct char_data *ch, const char *argument)
{
    send_to_char("You are carrying```8:``\n\r", ch);
    show_list_to_char(ch->carrying, ch, true, true);
    return;
}

/* New do_equipment() function. Shows eq slots not worn.*/
void do_equipment(struct char_data *ch, /*@unused@*/const char *argument)
{
    look_equipment(ch);
    return;
}

void do_compare(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct gameobject *obj1;
    struct gameobject *obj2;
    long value1;
    long value2;
    char *msg;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        send_to_char("Compare what to what to what to what?\n\r", ch);
        return;
    }

    if ((obj1 = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have one, dorkus.\n\r", ch);
        return;
    }

    if (arg2[0] == '\0') {
        for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content) {
            if (obj2->wear_loc != WEAR_NONE
                && can_see_obj(ch, obj2)
                && obj1->item_type == obj2->item_type
                && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
                break;
        }

        if (obj2 == NULL) {
            send_to_char("You aren't wearing anything comparable.\n\r", ch);
            return;
        }
    } else if ((obj2 = get_obj_carry(ch, arg2)) == NULL) {
        send_to_char("You do not have one, dorkus.\n\r", ch);
        return;
    }

    msg = NULL;
    value1 = 0;
    value2 = 0;

    if (obj1 == obj2) {
        msg = "You compare $p to itself.  It looks about the same.";
    } else if (obj1->item_type != obj2->item_type) {
        msg = "You can't compare $p and $P.";
    } else {
        switch (obj1->item_type) {
          default:
              msg = "You can't compare $p and $P.";
              break;

          case ITEM_ARMOR:
              value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
              value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
              break;

          case ITEM_WEAPON:
              value1 = (1l + obj1->value[2]) * obj1->value[1];
              value2 = (1l + obj2->value[2]) * obj2->value[1];
              break;
        }
    }

    if (msg == NULL) {
        if (value1 == value2)
            msg = "$p and $P look about the same.";
        else if (value1 > value2)
            msg = "$p looks kickass compared to $P.";
        else
            msg = "$p looks like cow dung compared to $P.";
    }

    act(msg, ch, obj1, obj2, TO_CHAR);
    return;
}



void do_credits(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "diku", NULL);
    return;
}


void do_here(struct char_data *ch, const char *argument)
{
    if (ch->desc == NULL)
        return;

    if (ch->position < POS_SLEEPING) {
        send_to_char("You can't see anything but stars!\n\r", ch);
        return;
    }

    if (ch->position == POS_SLEEPING) {
        send_to_char("You can't see anything, you're sleeping!\n\r", ch);
        return;
    }

    if (character_is_blind(ch)) {
        send_to_char("You are blind!", ch);
        return;
    }

    if (!IS_NPC(ch)
        && !IS_SET(ch->act, PLR_HOLYLIGHT)
        && room_is_dark(ch, ch->in_room)) {
        send_to_char("It is pitch ```8black...`` \n\r", ch);
        show_char_to_char(ch->in_room->people, ch);
        return;
    }

    send_to_char(" People in the room:\n\r", ch);
    show_char_to_char(ch->in_room->people, ch);
}


void do_consider(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    char *msg;
    int diff;

    (void)one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Consider killing whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL) {
        send_to_char("They're not here.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) {
        send_to_char("Don't even think about it.\n\r", ch);
        return;
    }

    diff = victim->level - ch->level;

    if (diff <= -10)
        msg = "You can kill $N bound and gagged... hey, kinky.";
    else if (diff <= -5)
        msg = "$N is a wuss... kick his ass.";
    else if (diff <= -2)
        msg = "$N looks like an easy kill.";
    else if (diff <= 1)
        msg = "Even odds, unless you rock...";
    else if (diff <= 4)
        msg = "$N mutters 'Do you feel yucky, spunk?'.";
    else if (diff <= 9)
        msg = "$N laughs at you mercilessly.  You will die a funny death.";
    else
        msg = "Death will thank you for your soul, and your cigarette will most likely go out.";

    act(msg, ch, NULL, victim, TO_CHAR);
    return;
}



void set_title(struct char_data *ch, char *title)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
        log_bug("Set_title: NPC.");
        return;
    }

    if (title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' && title[0] != ';') {
        buf[0] = ' ';
        strcpy(buf + 1, title);
    } else {
        strcpy(buf, title);
    }

    free_string(ch->pcdata->title);
    ch->pcdata->title = str_dup(buf);
    return;
}



/***************************************************************************
 *	do_title
 ***************************************************************************/
void do_title(struct char_data *ch, const char *argument)
{
    static char buf[MAX_STRING_LENGTH];

    DENY_NPC(ch);

    if (argument[0] == '\0') {
        send_to_char("Change your title to what?\n\r", ch);
        return;
    }


    strncpy(buf, argument, MAX_TITLE_LENGTH);
    smash_tilde(buf);
    set_title(ch, buf);
    send_to_char("Ok.\n\r", ch);
}


/***************************************************************************
 *	do_deathcry
 ***************************************************************************/
void do_deathcry(struct char_data *ch, const char *argument)
{
    static char buf[MAX_STRING_LENGTH];
    int rand;

    DENY_NPC(ch);

    if ((argument[0] == '\0') && (ch->pcdata->deathcry == NULL)) {
        send_to_char("Change your death cry to what?\n\r", ch);
        return;
    } else if (argument[0] == '\0') {
        printf_to_char(ch, "Your `1death `!cry`` is`8:`` %s\n\r", ch->pcdata->deathcry);
        return;
    }

    if (!str_prefix(argument, "reset")) {
        rand = number_range(0, 9);
        switch (rand) {
          case 1:
              do_deathcry(ch, "!@#$!@#$%!@#$!!@@!");
              break;
          case 2:
              do_deathcry(ch, "Oh my. I seem to have been killed. Buttworth will be MOST displeased.");
              break;
          case 3:
              do_deathcry(ch, "`2*`@gleep`2*`! I'm DEAD.");
              break;
          case 4:
              do_deathcry(ch, "GoddamnitIreallyamgettingsickandtiredofDYING!");
              break;
          case 5:
              do_deathcry(ch, "Dammit, I HATE it when that happens!");
              break;
          case 6:
              do_deathcry(ch, "Oh NO! Not again!!");
              break;
          case 7:
              do_deathcry(ch, "The weasels! THE WEASELS!!! AAAGH!!");
              break;
          case 8:
              do_deathcry(ch, "Oh `#SH`&i`#T`!!!");
              break;
          default:
              do_deathcry(ch, "AAAAAAAAAAAAAAAAAARGH! I'm DEAD!!");
              break;
        }

        return;
    }

    (void)snprintf(buf, MAX_INPUT_LENGTH, "%s", argument);
    smash_tilde(buf);
    ch->pcdata->deathcry = str_dup(buf);
    printf_to_char(ch, "Your `1death `!cry`` is`8:`` %s\n\r", ch->pcdata->deathcry);
}


/***************************************************************************
 *	do_description
 ***************************************************************************/
void do_description(struct char_data *ch, const char *argument)
{

    static char buf[MAX_STRING_LENGTH];

    DENY_NPC(ch);

    if (!str_prefix(argument, "edit")) {
        string_append(ch, &ch->description);
        return;
    }

    if (argument[0] == '-') {
        int len, buf_len;
        bool found = false;

        if (ch->description == NULL || ch->description[0] == '\0') {
            send_to_char("No lines left to remove.\n\r", ch);
            return;
        }

        strcpy(buf, ch->description);
        buf_len = (int)strlen(buf);
        for (len = buf_len; len > 0; len--) {
            if (buf[len] == '\r') {
                if (!found) {            /* back it up */
                    if (len > 0)
                        len--;
                    found = true;
                } else {
                    /* found the second one */
                    buf[len + 1] = '\0';
                    free_string(ch->description);
                    ch->description = str_dup(buf);
                    send_to_char("Your description is```8:``\n\r", ch);
                    send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
                    return;
                }
            }
        }
        buf[0] = '\0';
        free_string(ch->description);
        ch->description = str_dup(buf);
        send_to_char("Description cleared.\n\r", ch);
        return;
    }

    if (argument[0] == '+') {
        static char sanitized[MAX_STRING_LENGTH];
        char *s;

        (void)strncpy(sanitized, argument, MAX_STRING_LENGTH);
        smash_tilde(sanitized);

        if (ch->description != NULL) {
            strncpy(buf, ch->description, strlen(ch->description));
        }

        s = sanitized + 1;
        while (isspace((int)*s)) {
            s++;
        }

        if (strlen(buf) + strlen(s) >= 1024) {
            send_to_char("Description too long.\n\r", ch);
            return;
        }

        strcat(buf, s);
        strcat(buf, "\n\r");
        free_string(ch->description);
        ch->description = str_dup(buf);
    }

    send_to_char("Your description is```8:``\n\r", ch);
    send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
}



void do_report(struct char_data *ch, const char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    sprintf(buf,
            "You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\n\r",
            ch->hit, ch->max_hit,
            ch->mana, ch->max_mana,
            ch->move, ch->max_move,
            ch->exp);

    send_to_char(buf, ch);

    sprintf(buf, "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'",
            ch->hit, ch->max_hit,
            ch->mana, ch->max_mana,
            ch->move, ch->max_move,
            ch->exp);

    act(buf, ch, NULL, NULL, TO_ROOM);

    return;
}


/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    (void)one_argument(argument, arg);

    if (arg[0] == '\0')
        wimpy = ch->max_hit / 5;
    else
        wimpy = parse_int(arg);

    if (wimpy < 0) {
        send_to_char("Your courage exceeds your wisdom.\n\r", ch);
        return;
    }

    if (wimpy > ch->max_hit / 2) {
        send_to_char("Such cowardice ill becomes you.  Grow some nads.\n\r", ch);
        return;
    }

    ch->wimpy = wimpy;
    printf_to_char(ch, "You will run like a dog at %d hit points.\n\r", wimpy);
    return;
}



void do_password(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char cEnd;
    int passaccept;

    if (IS_NPC(ch))
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while (isspace((int)*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while (isspace((int)*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax```8: ``password <old> <new>.\n\r", ch);
        return;
    }

    if (!password_matches(ch->pcdata->pwd, arg1)) {
        WAIT_STATE(ch, 40);
        send_to_char("Wrong password.  Wait 10 seconds.\n\r", ch);
        return;
    }

    passaccept = password_acceptance(arg2);
    if (passaccept != 0) {
        printf_to_char(ch, "%s", password_accept_message(passaccept));
        return;
    }

    free_string(ch->pcdata->pwd);
    ch->pcdata->pwd = password_encrypt(arg2);
    save_char_obj(ch);
    send_to_char("Password changed... Hope you remember it.\n\r", ch);
    return;
}

void do_finger(struct char_data *ch, const char *argument)
{
    struct char_data *victim;
    FILE *fp;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char hours[MAX_STRING_LENGTH];
    char mins[MAX_STRING_LENGTH];
    char secs[MAX_STRING_LENGTH];
    bool fOld;
    bool vOnline = true;

    (void)one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Syntax: finger <name>\n\r\n\r", ch);
        send_to_char("The name you enter `!MUST`7\n\r", ch);
        send_to_char("be entered in full.\n\r", ch);
        return;
    }

    /* Use the currently loaded character in the game, as long as they're
     *      not a mob.  Added by Monrick, 1/2008 */

    if ((victim = get_char_world(ch, arg)) == NULL || IS_NPC(victim)) {
        if ((victim != NULL) && IS_NPC(victim)) {
            sprintf(buf,
                    "`@%s `@tells you '`tPull that again and I'm gonna kick your ass ..`@'``\n\r",
                    capitalize(victim->short_descr));
            send_to_char(buf, ch);
            return;
        }

        victim = new_char();
        victim->pcdata = new_pcdata();
        fOld = false;
        vOnline = false;

        sprintf(buf, "%s%s", PLAYER_DIR, capitalize(arg));
        if ((fp = fopen(buf, "r")) != NULL) {
            int iNest;

            for (iNest = 0; iNest < MAX_NEST; iNest++)
                rgObjNest[iNest] = NULL;

            fOld = true;
            for (;; ) {
                char letter;
                char *word;

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
                if (!str_cmp(word, "PLAYER"))
                    fread_char(victim, fp);
                else
                    break;
            }

            fclose(fp);
        }

        if (!fOld) {
            send_to_char("Invalid character.\n\r", ch);
            free_pcdata(victim->pcdata);
            free_char(victim);
            return;
        }
    }

    send_to_char("`8 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-``\n\r", ch);
    sprintf(buf, "`& Total       `7Hours`8:`6 %-10d      `7Minutes`8:`6 %-3d           `7Seconds`8:`6 %-3d``\n\r",
            get_hours_played(victim),
            get_minutes_played(victim),
            get_seconds_played(victim));
    send_to_char(buf, ch);
    sprintf(hours, "%2.2d", get_session_hours(victim));
    sprintf(mins, "%2.2d", get_session_minutes(victim));
    sprintf(secs, "%2.2d", get_session_seconds(victim));
    sprintf(buf, "`& Today      `7 Hours`8:`6 %-10s     `7 Minutes`8:`6 %-3s           `7Seconds`8:`6 %-3s``\n\r", hours, mins, secs);
    send_to_char(buf, ch);
    send_to_char("`8 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-``\n\r", ch);
    sprintf(buf, "             `7Level`8:`6 %-3d                `7Race`8:`6 %-9s       `7Class`8:`6 %-7s``\n\r",
            victim->level, race_table[victim->race].name, class_table[victim->class].name);
    send_to_char(buf, ch);
    sprintf(buf, "      `7Player Kills`8:`6 %-4ld     `7 Player Deaths`8:`6 %-4ld        `7Mob Kills`8:`6 %ld``\n\r",
            victim->pcdata->pkills, victim->pcdata->pdeaths, victim->pcdata->mobkills);
    send_to_char(buf, ch);
    send_to_char("`8 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-``\n\r", ch);

    send_to_char(victim->description, ch);
    if (!vOnline) {
        free_pcdata(victim->pcdata);
        free_char(victim);
    }
    return;
}

void do_laston(struct char_data *ch, const char *argument)
{
    struct descriptor_iterator_filter filter = { .must_playing = true, .skip_character = ch };
    struct descriptor_data *d;
    struct descriptor_data *dpending;
    struct char_data *victim = ch;
    FILE *fp;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    bool fOld;

    (void)one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Syntax: laston <name>\n\r\n\r", ch);
        send_to_char("The name you enter `!MUST`7\n\r", ch);
        send_to_char("be entered in full.\n\r", ch);
        return;
    }

    dpending = descriptor_iterator_start(&filter);
    while ((d = dpending) != NULL) {
        struct char_data *wch;
        dpending = descriptor_iterator(d, &filter);

        if (can_see(ch, d->character)) {
            wch = CH(d);

            if (can_see(ch, wch)) {
                if (!str_cmp(arg, wch->name)) {
                    if (victim->name == ch->name) {
                        send_to_char("`@You are on right now.`7\n\r", ch);
                    } else if (!can_see(ch, wch)) {
                        sprintf(buf, "`@`t%s was last here on %s`@`7\n\r", victim->name, (char *)ctime(&victim->llogoff));
                        send_to_char(buf, ch);
                    } else {
                        act("`@`t$N is currently connected ..`@`7", ch, NULL, victim, TO_CHAR);
                    }

                    /* PERHAPS THIS IS A NASTY SHORT CIRCUIT, NO? */
                    return;
                }
            }
        }
    }
    /* character is not connected, so go snooping save games. */

    victim = new_char();
    victim->pcdata = new_pcdata();
    fOld = false;

    sprintf(buf, "%s%s", PLAYER_DIR, capitalize(arg));
    if ((fp = fopen(buf, "r")) != NULL) {
        int iNest;

        for (iNest = 0; iNest < MAX_NEST; iNest++)
            rgObjNest[iNest] = NULL;

        fOld = true;
        for (;; ) {
            char letter;
            char *word;

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
            if (!str_cmp(word, "PLAYER"))
                fread_char(victim, fp);
            else
                break;
        }

        fclose(fp);
    }

    if (!fOld) {
        send_to_char("`@Acid-Fiend-1 tells you '`tNo player by that name exists.`@'`7\n\r", ch);
        free_pcdata(victim->pcdata);
        free_char(victim);
        return;
    }

    if (victim->name == ch->name) {
        send_to_char("`@Acid-Fiend-1 tells you '`tAre you stupid?`@'`7\n\r", ch);
        free_pcdata(victim->pcdata);
        free_char(victim);
        return;
    }

    printf_to_char(ch, "`@Acid-Fiend-1 tells you '`t%s was last here on %s`@'`7", victim->name, (char *)ctime(&victim->llogoff));
    free_pcdata(victim->pcdata);
    free_char(victim);
}

void print_weather(struct char_data *ch)
{
    static char *const sky_look[4] =
    {
        "`Ocloudless``",
        "`Opa`&rtl`Oy c`&lo`Oudy``",
        "`8r`4a`8i`4n`8y`` as `1hell``",
        "lit by flashes of `8l`&i`8ghtning``"
    };

    printf_to_char(ch, "The sky is %s and %s\n\r", sky_look[globalGameState.weather->sky], globalGameState.weather->change >= 0 ? "rainy." : "snowing.");
}

void do_weather(struct char_data *ch, const char *argument)
{
    if (!IS_OUTSIDE(ch)) {
        send_to_char("You can't see the weather indoors.\n\r", ch);
        return;
    }

    print_weather(ch);
}

void do_die(struct char_data *ch, const char *argument)
{
    show_help(ch->desc, "BLOW", NULL);
    return;
}



/* add this function - add definition in merc.h*/
void show_damage_display(struct char_data *ch, struct char_data *victim)
{
    char buf[MAX_STRING_LENGTH];
    int percent;

    /* if the victims race is human and it is a PC, it shows nothing*/
    if (IS_NPC(victim)) {
        strcpy(buf, PERS(victim, ch));

        if (victim->max_hit > 0)
            percent = (100 * victim->hit) / victim->max_hit;
        else
            percent = -1;

        if (percent >= 100)
            strcat(buf, " `Cis in excellent condition.``\n\r");
        else if (percent >= 90)
            strcat(buf, " `Chas a few scratches.``\n\r");
        else if (percent >= 75)
            strcat(buf, " `Chas some small wounds and bruises.``\n\r");
        else if (percent >= 50)
            strcat(buf, " `Chas quite a few wounds.``\n\r");
        else if (percent >= 30)
            strcat(buf, " `Chas some big nasty wounds and scratches.``\n\r");
        else if (percent >= 15)
            strcat(buf, "`C looks pretty hurt.``\n\r");
        else if (percent >= 0)
            strcat(buf, "`C is in awful condition.``\n\r");
        else
            strcat(buf, " `Cis bleeding to death.``\n\r");

        buf[0] = UPPER(buf[0]);
        send_to_char(buf, ch);
    }
}


static void print_history_help(struct char_data *ch)
{
    send_to_char("Syntax: history [cmd]\n\r\n\r", ch);
    send_to_char("Available commands:\n\r", ch);
    printf_to_char(ch, "%-10.10s", "+");
    printf_to_char(ch, "%-10.10s", "-");
    /*printf_to_char(ch, "%-10.10s", "edit");*/
    printf_to_char(ch, "%-10.10s", "clear");
    /*send_to_char("\n\r", ch);*/
    printf_to_char(ch, "%-10.10s", "show");
    send_to_char("\n\r\n\r", ch);
}

/*
 * Allows PC to make and store a history.
 * by TAKA
 * (c) 1999 TAKA of the Ghost Dancer MUD Project
 *   Modified (rewritten) by Monrick: January 6, 2008
 *    for Bad Trip
 */
void do_history(struct char_data *ch, const char *argument)
{
    char cmd[MAX_STRING_LENGTH];
    int len;
    bool found = false;
    size_t MAX_HIST_LENGTH = 4096;

    if (IS_NPC(ch))
        return;

    if (argument[0] == '\0') {
        print_history_help(ch);
        return;
    }

    argument = one_argument(argument, cmd);

    if (cmd[0] == '\0') {
        send_to_char("Your history is:\n\r", ch);
        page_to_char(ch->pcdata->history ? ch->pcdata->history : "(None).\n\r", ch);
        return;
    } else {
        static char sanitized[MAX_STRING_LENGTH];

        snprintf(sanitized, MAX_STRING_LENGTH, "%s", argument);
        smash_tilde(sanitized);

        if (cmd[0] == '+') {
            char buf[MAX_STRING_LENGTH];

            if (ch->pcdata->history == NULL || ch->pcdata->history[0] == '\0') {
                if (strlen(sanitized) >= MAX_HIST_LENGTH) {
                    send_to_char("History too long.\n\r", ch);
                    return;
                }
            } else if (strlen(ch->pcdata->history) + strlen(sanitized) >= MAX_HIST_LENGTH) {
                send_to_char("History too long.\n\r", ch);
                return;
            }

            strcpy(buf, ch->pcdata->history);
            strcat(buf, sanitized);
            strcat(buf, "\n\r");

            free_string(ch->pcdata->history);
            ch->pcdata->history = str_dup(buf);

            send_to_char("Ok.\n\r", ch);
            return;
        } else if (cmd[0] == '-') {
            char buf[MAX_STRING_LENGTH];
            int buf_len;

            if (ch->pcdata->history == NULL || ch->pcdata->history[0] == '\0') {
                send_to_char("No lines left to remove.\n\r", ch);
                return;
            }


            strcpy(buf, ch->pcdata->history);
            buf_len = (int)strlen(buf);
            for (len = buf_len; len > 0; len--) {
                if (buf[len] == '\r') {
                    /* ignore the first '\r' since we are starting at the end */
                    if (!found) {
                        if (len > 0)
                            len--;
                        found = true;
                    } else {
                        /* crop off everything after the second '\r' from the end */
                        buf[len + 1] = '\0';
                        break;
                    }
                }
            }

            if (len <= 0)
                buf[0] = '\0';

            free_string(ch->pcdata->history);
            ch->pcdata->history = str_dup(buf);

            send_to_char("Ok.\n\r", ch);
            return;
        } else if (!strcmp(cmd, "edit")) {
            print_history_help(ch);
            return;
        } else if (!strcmp(cmd, "clear")) {
            if (ch->pcdata->history != NULL && ch->pcdata->history[0] != '\0') {
                free_string(ch->pcdata->history);
                ch->pcdata->history = NULL;
            }
            send_to_char("History cleared.\n\r", ch);
            return;
        } else if (strcmp(cmd, "show")) {
            print_history_help(ch);
            return;
        }
    }

    if ((ch->pcdata->history != NULL) && (ch->pcdata->history[0] != '\0')) {
        send_to_char("Your history is:\n\r", ch);
        page_to_char(ch->pcdata->history, ch);
    } else {
        send_to_char("You have no history.  Perhaps you should write one...\n\r", ch);
    }
}

void do_viewhist(struct char_data *ch, const char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    struct char_data *victim;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0') {
        send_to_char("`!Whose history do you want to read?``\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL) {
        send_to_char("`!They aren't here.``\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
        return;

    if (victim->pcdata->history != NULL && victim->pcdata->history[0] != '\0') {
        printf_to_char(ch, "%s's history is:\n\r", victim->name);
        page_to_char(victim->pcdata->history, ch);
    } else {
        printf_to_char(ch, "%s doesn't have a history.\n\r", victim->name);
    }
    return;
}
