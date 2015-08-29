#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/** exports */
void sick_harvey_proctor(CHAR_DATA *ch, enum e_harvey_proctor_is, const char *message);


/** imports */
extern SKILL *gsp_deft;
extern SKILL *gsp_dash;

extern long top_mob_index;
extern bool copyover();
extern AFFECT_DATA *affect_free;
extern bool is_auction_participant(CHAR_DATA *ch);
extern GAMEOBJECT *get_auction_item();
extern ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);
extern void reset_area(AREA_DATA * pArea);
extern bool add_alias(CHAR_DATA * ch, char *alias, char *cmd);
extern bool set_char_hunger(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);
extern bool set_char_thirst(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);
extern bool set_char_feed(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);

/** locals */
static DECLARE_DO_FUN(do_grestore);
static DECLARE_DO_FUN(do_rrestore);
static void fry_char(CHAR_DATA * ch, char *argument);


void do_order(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    char buf[MSL];
    char arg[MIL];
    char arg2[MIL];
    bool found;
    bool fAll;

    argument = one_argument(argument, arg);
    (void)one_argument(argument, arg2);

    if (!str_cmp(arg2, "suicide")) {
	send_to_char("That will not be done..\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "bash")) {
	send_to_char("Sorry, no way.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "mob")) {
	send_to_char("That will NOT be done.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "quit")) {
	send_to_char("That will NOT be done.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "brew")) {
	send_to_char("Sorry pal, ain't gonna happen.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "scribe")) {
	send_to_char("Mobiles are not literate enough.\n\r", ch);
	return;
    }

    if (!str_prefix(arg2, "cast")) {
	send_to_char("Survey says:  No!\n\r", ch);
	return;
    }

    if (!str_prefix(arg2, "push")) {
	send_to_char("Survey says:  No!\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "pk")) {
	send_to_char("Huh? .. Kill what?\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "eat")) {
	send_to_char("Your friend is not hungry..\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "drink")) {
	send_to_char("Your friend is not thirsty..\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "enter")) {
	send_to_char("I'm sorry Dave, but I'm afraid I can't do that.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "drop")) {
	send_to_char("No.\n\r", ch);
	return;
    }


    if (arg[0] == '\0' || argument[0] == '\0') {
	send_to_char("Order whom to do what?\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
	send_to_char("You feel like taking, not giving, orders.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "all")) {
	fAll = true;
	victim = NULL;
    } else {
	fAll = false;
	if ((victim = get_char_room(ch, arg)) == NULL) {
	    send_to_char("They aren't here.\n\r", ch);
	    return;
	}

	if (victim == ch) {
	    send_to_char("Aye aye, right away!\n\r", ch);
	    return;
	}

	if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
		|| (IS_IMMORTAL(victim) && victim->trust >= ch->trust)) {
	    send_to_char("Do it yourself!\n\r", ch);
	    return;
	}
    }

    found = false;
    for (och = ch->in_room->people; och != NULL; och = och_next) {
	och_next = och->next_in_room;

	if (IS_AFFECTED(och, AFF_CHARM)
		&& och->master == ch
		&& (fAll || och == victim)) {
	    found = true;

	    if (!str_infix(arg2, "rem")) {
		send_to_char("One at a time...\n\r", ch);
		return;
	    }

	    (void)snprintf(buf, 2 * MIL, "$n orders you to '%s'.", argument);
	    act(buf, ch, NULL, och, TO_VICT);
	    interpret(och, argument);
	}
    }

    if (found) {
	WAIT_STATE(ch, PULSE_VIOLENCE);
	send_to_char("Ok.\n\r", ch);
    } else {
	send_to_char("You have no followers here.\n\r", ch);
    }
}

void do_ignor(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    send_to_char("You must enter the full command to ignore someone.\n\r", ch);
}

void do_ignore(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    CHAR_DATA *rch;
    char arg[MIL], buf[MSL];
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;
    int pos;
    bool found = false;

    if (ch->desc == NULL)
	rch = ch;
    else
	rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
	return;

    smash_tilde(argument);

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
	if (rch->pcdata->ignore[0] == NULL) {
	    send_to_char("You are not ignoring anyone.\n\r", ch);
	    return;
	}
	send_to_char("You are currently ignoring:\n\r", ch);

	for (pos = 0; pos < MAX_IGNORE; pos++) {
	    if (rch->pcdata->ignore[pos] == NULL)
		break;

	    (void)snprintf(buf, 2 * MIL, "    %s\n\r", rch->pcdata->ignore[pos]);
	    send_to_char(buf, ch);
	}
	return;
    }

    for (pos = 0; pos < MAX_IGNORE; pos++) {
	if (rch->pcdata->ignore[pos] == NULL)
	    break;

	if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
	    send_to_char("You are already ignoring that person.\n\r", ch);
	    return;
	}
    }

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	CHAR_DATA *wch;
	dpending = descriptor_iterator(d, &playing_filter);

	if (!can_see(ch, d->character))
	    continue;

	wch = (d->original != NULL) ? d->original : d->character;

	if (!can_see(ch, wch))
	    continue;

	if (!str_cmp(arg, wch->name)) {
	    found = true;
	    if (wch == ch) {
		send_to_char("You try to ignore yourself .. and fail.\n\r", ch);
		return;
	    }
	    if (wch->level >= LEVEL_IMMORTAL) {
		send_to_char("That person is very hard to ignore.\n\r", ch);
		return;
	    }
	}
    }

    if (!found) {
	send_to_char("No one by that name is playing.\n\r", ch);
	return;
    }

    for (pos = 0; pos < MAX_IGNORE; pos++)
	if (rch->pcdata->ignore[pos] == NULL)
	    break;

    if (pos >= MAX_IGNORE) {
	send_to_char("Stop being so antisocial!\n\r", ch);
	return;
    }

    rch->pcdata->ignore[pos] = str_dup(arg);
    (void)snprintf(buf, 2 * MIL, "You are now deaf to %s.\n\r", arg);
    send_to_char(buf, ch);
}

void do_unignore(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MIL], buf[MSL];
    int pos;
    bool found = false;

    if (ch->desc == NULL)
	rch = ch;
    else
	rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
	return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
	if (rch->pcdata->ignore[0] == NULL) {
	    send_to_char("You are not ignoring anyone.\n\r", ch);
	    return;
	}
	send_to_char("You are currently ignoring:\n\r", ch);

	for (pos = 0; pos < MAX_IGNORE; pos++) {
	    if (rch->pcdata->ignore[pos] == NULL)
		break;

	    (void)snprintf(buf, 2 * MIL, "    %s\n\r", rch->pcdata->ignore[pos]);
	    send_to_char(buf, ch);
	}
	return;
    }

    for (pos = 0; pos < MAX_IGNORE; pos++) {
	if (rch->pcdata->ignore[pos] == NULL)
	    break;

	if (found) {
	    rch->pcdata->ignore[pos - 1] = rch->pcdata->ignore[pos];
	    rch->pcdata->ignore[pos] = NULL;
	    continue;
	}

	if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
	    send_to_char("Ignore removed.\n\r", ch);
	    free_string(rch->pcdata->ignore[pos]);
	    rch->pcdata->ignore[pos] = NULL;
	    found = true;
	}
    }

    if (!found)
	send_to_char("You aren't ignoring anyone by that name!\n\r", ch);
}

void do_bug(CHAR_DATA *ch, char *argument)
{
    append_file(ch, BUG_FILE, argument);
    send_to_char("Bug logged.\n\r", ch);
}

void do_typo(CHAR_DATA *ch, char *argument)
{
    append_file(ch, TYPO_FILE, argument);
    send_to_char("Typo logged.\n\r", ch);
}

void do_delet(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    send_to_char("You must type the full command to delete yourself.\n\r", ch);
}

void do_delete(CHAR_DATA *ch, char *argument)
{
    char strsave[MIL];

    DENY_NPC(ch);

    if (ch->pcdata->confirm_delete) {
	if (argument[0] != '\0') {
	    send_to_char("Delete status removed.\n\r", ch);
	    ch->pcdata->confirm_delete = false;
	    log_string("DELETE: %s just couldn't do it...", ch->name);

	    return;
	} else {
	    char filename[MAX_NAME_LENGTH];

	    filename[0] = '\0';

	    capitalize_into(ch->name, (char *)filename, MAX_NAME_LENGTH);
	    (void)snprintf(strsave, MIL, "%s%s", PLAYER_DIR, filename);

	    wiznet("$N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
	    log_string("DELETE: %s is TOAST! wOOp!", ch->name);

	    do_quit(ch, "");

	    (void)unlink(strsave);
	    do_echo(NULL, "You see a flash of `&lightning``.\n\r");
	    return;
	}
    }

    if (argument[0] != '\0') {
	send_to_char("Just type delete. No argument.\n\r", ch);
	return;
    }

    send_to_char("Type delete again to confirm this command.\n\r", ch);
    send_to_char("```!WARNING``: this command is irreversible.\n\r", ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r", ch);

    ch->pcdata->confirm_delete = true;

    wiznet("$N is contemplating deletion.", ch, NULL, 0, 0, get_trust(ch));
    log_string("DELETE: %s .. %s's thinking about it ..", ch->name, ch->sex == 0 ? "It" : ch->sex == 1 ? "He" : "She");
}

void do_fixscreen(CHAR_DATA *ch, char *argument)
{
    // fix the screen....send clear screen and set the number of lines
    int lines;

    if (argument[0] == '\0' || !is_number(argument))
	lines = 25;
    else
	lines = parse_int(argument);

    if ((lines < 1) || (lines > 100)) {
	send_to_char("Get real.\n\r", ch);
	return;
    }

    printf_to_char(ch, "\033[0;37;40m\033[%d;1f\033[2J\n\rScreen fixed.\n\r", lines);
}

void do_clearscreen(CHAR_DATA *ch, /*@unused@*/char *argument)
{
    send_to_char("\033[0H\033[2J\n", ch);
}

void do_qui(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    send_to_char("If you want to QUIT, you have to spell it out.\n\r", ch);
}

void do_quit(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    DESCRIPTOR_DATA *d;
    char strsave[2*MIL];
    long id;

    if (IS_NPC(ch))
	return;

    if (!IS_NPC(ch) && ch->timer < 30) {
	if (ch->pnote != NULL) {
	    send_to_char("You have some kind of note in progress.  Please clear it or post it before quitting.\n\r", ch);
	    return;
	}
    }

    if (ch->position == POS_FIGHTING) {
	send_to_char("No way! You are fighting.\n\r", ch);
	return;
    }

    if (ch->position < POS_STUNNED) {
	send_to_char("You're not DEAD yet.\n\r", ch);
	return;
    }

    if (is_auction_participant(ch)) {
	GAMEOBJECT *item = get_auction_item();
	if (item == NULL) {
	    log_bug("%s reported as auction participant with no auction item.", ch->name);
	} else {
	    printf_to_char(ch, "Wait until the auctioning of %s is over.\n\r", item->short_descr);
	}
	return;
    }

    furniture_check(ch);

    send_to_char("`@Disconnected.``\n\r", ch);
    act("```@$n ```Ohas ```^left ```#the ```!game.``", ch, NULL, NULL, TO_ROOM);
    log_string("%s has quit.", ch->name);
    wiznet("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0, get_trust(ch));

    /*
     * After extract_char the ch is no longer valid!
     */
    if (IS_SET(ch->act, PLR_LINKDEAD))
	REMOVE_BIT(ch->act, PLR_LINKDEAD);
    affect_strip(ch, gsp_deft);
    affect_strip(ch, gsp_dash);

    save_char_obj(ch);
    id = ch->id;
    d = ch->desc;

    if ((ch->level == 1) && (!IS_SET(ch->act, PLR_DENY))) {
	(void)snprintf(strsave, 2 * MIL, "%s%s", PLAYER_DIR, capitalize(ch->name));
	wiznet("`!Killing`7: $N `1(`7Level 1 cleanup`1)`7", ch, NULL, 0, 0, 0);
	unlink(strsave);
    }
    extract_char(ch, true);
    if (d != NULL)
	close_socket(d);

    /** if a character somehow gets duped and linked to multiple descriptors, close them, too. */
    {
	DESCRIPTOR_DATA *dpending;

	dpending = descriptor_iterator_start(&descriptor_empty_filter);
	while ((d = dpending) != NULL) {
	    CHAR_DATA *tch;
	    dpending = descriptor_iterator(d, &descriptor_empty_filter);

	    tch = CH(d);
	    if (tch && tch->id == id) {
		extract_char(tch, true);
		close_socket(d);
	    }
	}
    }
}

void do_save(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    if (IS_NPC(ch))
	return;

    REMOVE_BIT(ch->act, PLR_LINKDEAD);
    save_char_obj(ch);
    send_to_char("```^You are saved... for now!!!``\n\r", ch);
}

void do_wiznet(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf;
    long flag;

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    if (IS_SET(ch->pcdata->wiznet, WIZ_ON))
		do_wiznet(ch, "off");
	    else
		do_wiznet(ch, "on");
	    return;
	}

    if (!str_prefix(argument, "on")) {
	send_to_char("Welcome to `^W`@i`^znet``\n\r", ch);
	SET_BIT(ch->pcdata->wiznet, WIZ_ON);
	return;
    }

    if (!str_prefix(argument, "off")) {
	send_to_char("Signing off of ```^W```@i```^znet``.\n\r", ch);
	REMOVE_BIT(ch->pcdata->wiznet, WIZ_ON);
	return;
    }

    /* show wiznet status */
    if (!str_prefix(argument, "status")) {
	buf = new_buf();
	if (!IS_SET(ch->pcdata->wiznet, WIZ_ON))
	    add_buf(buf, "off ");

	for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
	    if (IS_SET(ch->pcdata->wiznet, wiznet_table[flag].flag)) {
		add_buf(buf, wiznet_table[flag].name);
		add_buf(buf, " ");
	    }
	}

	add_buf(buf, "\n\r");

	send_to_char("```^W```@i```^znet`` status```8:``\n\r", ch);
	send_to_char(buf_string(buf), ch);

	free_buf(buf);
	return;
    }

    /* list of all wiznet options */
    if (!str_prefix(argument, "show")) {
	buf = new_buf();
	for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
	    if (wiznet_table[flag].level <= get_trust(ch)) {
		add_buf(buf, wiznet_table[flag].name);
		add_buf(buf, " ");
	    }
	}

	add_buf(buf, "\n\r");

	send_to_char("```^W```@i```^znet`` options available to you are```8:``\n\r", ch);
	send_to_char(buf_string(buf), ch);

	free_buf(buf);
	return;
    }

    flag = wiznet_lookup(argument);
    if (flag == -1 || get_trust(ch) < wiznet_table[flag].level) {
	send_to_char("No such option.\n\r", ch);
	return;
    }

    if (IS_SET(ch->pcdata->wiznet, wiznet_table[flag].flag)) {
	printf_to_char(ch,
		"You will no longer see %s on ```^W```@i```^znet``.\n\r",
		wiznet_table[flag].name);
	REMOVE_BIT(ch->pcdata->wiznet, wiznet_table[flag].flag);
	return;
    } else {
	printf_to_char(ch,
		"You will now see %s on ```^W```@i```^znet``.\n\r",
		wiznet_table[flag].name);
	SET_BIT(ch->pcdata->wiznet, wiznet_table[flag].flag);
	return;
    }
}

void wiznet(char *string, /*@null@*/ CHAR_DATA *ch, /*@null@*/ GAMEOBJECT *obj, long flag, long flag_skip, int min_level)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = ch };
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);

	if (!IS_NPC(d->character)
		&& IS_IMMORTAL(d->character)
		&& IS_SET(d->character->pcdata->wiznet, WIZ_ON)
		&& (!flag || IS_SET(d->character->pcdata->wiznet, flag))
		&& (!flag_skip || !IS_SET(d->character->pcdata->wiznet, flag_skip))
		&& get_trust(d->character) >= min_level) {
	    if (IS_SET(d->character->pcdata->wiznet, WIZ_PREFIX))
		send_to_char("--> ", d->character);

	    act_new(string, d->character, obj, ch, TO_CHAR, POS_DEAD, false);
	}
    }
}

void do_impnet(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf;
    long flag;

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    if (IS_SET(ch->pcdata->impnet, IMN_ON))
		do_impnet(ch, "off");
	    else
		do_impnet(ch, "on");
	    return;
	}

    if (!str_prefix(argument, "on")) {
	send_to_char("Loading `PB`5ad `@T`2rip's `2I`8M`2P`7Net ..\n\r", ch);
	SET_BIT(ch->pcdata->impnet, IMN_ON);
	return;
    }

    if (!str_prefix(argument, "off")) {
	send_to_char("Signing off of `2I`8M`2P`7Net``.\n\r", ch);
	REMOVE_BIT(ch->pcdata->impnet, IMN_ON);
	return;
    }

    if (!str_prefix(argument, "status")) {
	buf = new_buf();

	if (!IS_SET(ch->pcdata->impnet, IMN_ON))
	    add_buf(buf, "off ");

	for (flag = 0; impnet_table[flag].name != NULL; flag++) {
	    if (IS_SET(ch->pcdata->impnet, impnet_table[flag].flag)) {
		add_buf(buf, impnet_table[flag].name);
		add_buf(buf, " ");
	    }

	    add_buf(buf, "\n\r");

	    send_to_char("`2I`8M`2P`7Net status`8:``\n\r", ch);
	    send_to_char(buf_string(buf), ch);

	    free_buf(buf);
	    return;
	}
    }

    if (!str_prefix(argument, "show")) {
	buf = new_buf();
	for (flag = 0; impnet_table[flag].name != NULL; flag++) {
	    if (impnet_table[flag].level <= get_trust(ch)) {
		add_buf(buf, impnet_table[flag].name);
		add_buf(buf, " ");
	    }
	}

	add_buf(buf, "\n\r");

	send_to_char("`2I`8M`2P`7Net options available are`8:``\n\r", ch);
	send_to_char(buf_string(buf), ch);

	free_buf(buf);
	return;
    }

    flag = impnet_lookup(argument);
    if (flag == -1 || get_trust(ch) < impnet_table[flag].level) {
	send_to_char("No such option.\n\r", ch);
	return;
    }

    if (IS_SET(ch->pcdata->impnet, impnet_table[flag].flag)) {
	printf_to_char(ch, "You will no longer see `2%s`7 on `2I`8M`2P`7Net``.\n\r", impnet_table[flag].name);
	REMOVE_BIT(ch->pcdata->impnet, impnet_table[flag].flag);
	return;
    } else {
	printf_to_char(ch, "You will now see `2%s`7 on `2I`8M`2P`7Net``.\n\r", impnet_table[flag].name);
	SET_BIT(ch->pcdata->impnet, impnet_table[flag].flag);
	return;
    }
}

void impnet(char *string, CHAR_DATA *ch, GAMEOBJECT *obj, long flag, long flag_skip, int min_level)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = ch };
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	if (!IS_NPC(d->character)
		&& IS_IMMORTAL(d->character)
		&& IS_SET(d->character->pcdata->impnet, IMN_ON)
		&& (!flag || IS_SET(d->character->pcdata->impnet, flag))
		&& (!flag_skip || !IS_SET(d->character->pcdata->impnet, flag_skip))
		&& get_trust(d->character) >= min_level) {
	    if (IS_SET(d->character->pcdata->impnet, IMN_PREFIX))
		send_to_char("--> ", d->character);

	    act_new(string, d->character, obj, ch, TO_CHAR, POS_DEAD, false);
	}
    }

    return;
}

void do_tick(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch)

	globalSystemState.tickset = !globalSystemState.tickset;
    if (globalSystemState.tickset)
	wiznet("$N forces a TICK!.", ch, NULL, 0, 0, 0);
}

void do_grant(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    SKILL *skill;
    LEARNED *learned;
    LEARNED *learned_found;
    char arg[MIL];

    DENY_NPC(ch)

	argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
	send_to_char("Syntax:\n\r", ch);
	send_to_char("       Grant [person] [skill/spell]\n\r", ch);
	return;
    }

    if (((victim = get_char_world(ch, arg)) == NULL)
	    || IS_NPC(ch)) {
	send_to_char("That person can't be found on heaven, hell, or earth\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Mobs were made with the skills they need,no more,no less.\n\r", ch);
	return;
    }

    if ((skill = skill_lookup(argument)) == NULL) {
	send_to_char("No such skill/spell\n\r", ch);
	return;
    }


    if ((learned_found = get_learned_skill(victim, skill)) != NULL
	    && learned_found->percent >= 75) {
	printf_to_char(ch, "%s already knows %s at %d%%.\n\r", victim->name, skill->name, learned_found->percent);
	return;
    }

    log_string("%s granted %s by %s", victim->name, skill->name, ch->name);

    if (victim != ch)
	printf_to_char(victim, "You have been found worthy of %s!\n\r", skill->name);

    printf_to_char(ch, "You have granted %s %s!\n\r", victim->name, skill->name);

    learned = new_learned();
    if (learned != NULL) {
	learned->skill = skill;
	learned->percent = 75;
	learned->type = LEARNED_TYPE_SKILL;

	add_learned_skill(victim, learned);
    }
}

void do_bamfin(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    if (!IS_NPC(ch)) {
	smash_tilde(argument);

	if (argument[0] == '\0') {
	    sprintf(buf, "Your poofin is %s\n\r", ch->pcdata->bamfin);
	    send_to_char(buf, ch);
	    return;
	}

	free_string(ch->pcdata->bamfin);
	ch->pcdata->bamfin = str_dup(argument);

	sprintf(buf, "Your poofin is now `8:``%s\n\r", ch->pcdata->bamfin);
	send_to_char(buf, ch);
    }

    return;
}

void do_bamfout(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    if (!IS_NPC(ch)) {
	smash_tilde(argument);

	if (argument[0] == '\0') {
	    sprintf(buf, "Your poofout is `8:``%s\n\r", ch->pcdata->bamfout);
	    send_to_char(buf, ch);
	    return;
	}

	free_string(ch->pcdata->bamfout);
	ch->pcdata->bamfout = str_dup(argument);

	sprintf(buf, "Your poofout is now `8:``%s\n\r", ch->pcdata->bamfout);
	send_to_char(buf, ch);
    }

    return;
}

void do_deny(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];

    one_argument(argument, arg);

    DENY_NPC(ch)

	if (arg[0] == '\0') {
	    send_to_char("Deny whom?\n\r", ch);
	    return;
	}

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Not on NPC's.\n\r", ch);
	return;
    }

    if (get_trust(victim) >= get_trust(ch)) {
	send_to_char("You failed.\n\r", ch);
	return;
    }

    SET_BIT(victim->act, PLR_DENY);
    send_to_char("`1O`!hhhh`8, `4d`$e`4n`$i`4e`$d`8! `2B`@am`2F\n\r", victim);


    sprintf(buf, "$N denies access to %s", victim->name);
    wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);

    send_to_char("OK.\n\r", ch);
    save_char_obj(victim);
    stop_fighting(victim, true);
    do_quit(victim, "");
    return;
}

void do_disconnect(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char arg[MIL];

    DENY_NPC(ch)

	one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Disconnect whom?\n\r", ch);
	return;
    }

    if (is_number(arg)) {
	DESCRIPTOR_ITERATOR_FILTER filter;

	filter.descriptor = (SOCKET)parse_int(arg);
	d = descriptor_iterator_start(&filter);
	if (d == NULL) {
	    printf_to_char(ch, "Descriptor %d not found.", filter.descriptor);
	    return;
	}

	victim = CH(d);
    } else {
	victim = get_char_world(ch, arg);
	if (victim == NULL) {
	    send_to_char("They aren't here.\n\r", ch);
	    return;
	}

	if (victim->desc == NULL) {
	    act("$N seems to be link-dead.", ch, NULL, victim, TO_CHAR);
	    return;
	}

	d = victim->desc;
    }

    if (ch == victim || ch->level <= victim->level) {
	send_to_char("You failed.\n\r", ch);
    } else if (ch->level > victim->level) {
	close_socket(d);
	send_to_char("Ok.\n\r", ch);
    }
}

void do_chown(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    GAMEOBJECT *obj;
    char arg1[MIL];
    char arg2[MIL];

    DENY_NPC(ch)

	argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
	send_to_char("Get what from whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, arg2)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }
    if (!IS_NPC(victim) && (victim->level >= ch->level)) {
	send_to_char("I don't think they'd like that too much.\n\r", ch);
	return;
    }
    if ((obj = get_obj_carry(victim, arg1)) == NULL
	    && (obj = get_obj_wear(victim, arg1)) == NULL) {
	send_to_char("They do not have that item.\n\r", ch);
	return;
    }

    obj_from_char(obj);
    obj_to_char(obj, ch);

    act("$n makes a magical gesture and $p flys from $N to $n.", ch, obj, victim, TO_NOTVICT);
    act("$n makes a magical gesture and $p flys from your body to $s.", ch, obj, victim, TO_VICT);
    act("$p flys from $N to you.", ch, obj, victim, TO_CHAR);
    return;
}

void do_echo(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;
    char buf[MSL];

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    send_to_char("Global echo what?\n\r", ch);
	    return;
	}

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);

	if (ch != NULL) {
	    if (ch && get_trust(d->character) >= get_trust(ch)) {
		sprintf(buf, "%s global> ", ch->name);
		send_to_char(buf, d->character);
	    }
	}
	send_to_char(argument, d->character);
	send_to_char("\n\r", d->character);
    }
}

void do_recho(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;
    char buf[MSL];

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    send_to_char("Local echo what?\n\r", ch);
	    return;
	}

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);

	if (d->character->in_room == ch->in_room) {
	    if (ch != NULL && get_trust(d->character) >= get_trust(ch)) {
		sprintf(buf, "%s local> ", ch->name);
		send_to_char(buf, d->character);
	    }
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

void do_zecho(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;
    char buf[MSL];

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    send_to_char("Zone echo what?\n\r", ch);
	    return;
	}

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);

	if (d->character->in_room != NULL && ch->in_room != NULL && d->character->in_room->area == ch->in_room->area) {
	    if (ch != NULL && get_trust(d->character) >= get_trust(ch)) {
		sprintf(buf, "%s zone> ", ch->name);
		send_to_char(buf, d->character);
	    }
	    send_to_char(argument, d->character);
	    send_to_char("\n\r", d->character);
	}
    }
}

void do_pecho(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];

    DENY_NPC(ch)

	argument = one_argument(argument, arg);
    if (argument[0] == '\0' || arg[0] == '\0') {
	send_to_char("Personal echo what?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("Target not found.\n\r", ch);
	return;
    }

    if (ch != NULL
	    && get_trust(victim) >= get_trust(ch)
	    && get_trust(ch) != MAX_LEVEL) {
	sprintf(buf, "%s personal> ", ch->name);
	send_to_char(buf, victim);
    }

    send_to_char(argument, victim);
    send_to_char("\n\r", victim);
    send_to_char("personal> ", ch);
    send_to_char(argument, ch);
    send_to_char("\n\r", ch);
}

void do_fry(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char file[MIL];
    char arg[MIL];
    char vName[MIL];
    bool isImm;

    DENY_NPC(ch)

	if (CHECK_NON_OWNER(ch)) {
	    sick_harvey_proctor(ch, hp_pissed_off, "Naughty, naughty, little boy!");
	    send_to_char("You have been logged.\n\r", ch);
	    return;
	} else {
	    sick_harvey_proctor(ch, hp_agreeable, NULL);
	}

    one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Fry whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't playing.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Not on NPC's.\n\r", ch);
	return;
    }

    if (get_trust(victim) >= get_trust(ch)) {
	send_to_char("That's not a good idea.\n\r", ch);
	return;
    }

    /* don't forget to delete the god-file, too */
    /* added by Monrick, 1/2008                 */
    snprintf(vName, MIL, "%s", capitalize(victim->name));
    isImm = IS_IMMORTAL(victim);

    act("You summon a huge bolt of `&lightning`` which utterly destroys $N ", ch, NULL, victim, TO_CHAR);
    act("A huge bolt of `&lightning`` strikes $N, utterly `!destroying`` $M", ch, NULL, victim, TO_NOTVICT);
    act("You look up, just in time to see the `!flaming`` `&lightning`` bolt strike your head.\n\r C-ya!\n\r", ch, NULL, victim, TO_VICT);

    snprintf(file, MIL, "%s%s", PLAYER_DIR, vName);
    fry_char(victim, "");
    unlink(file);

    if (isImm) {
	sprintf(file, "%s%s", GOD_DIR, vName);
	unlink(file);
    }

    do_echo(ch, "You hear a rumble of ```5thunder``.\n\r");
    return;
}

void do_ffry(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MIL];

    DENY_NPC(ch)

	one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Fake fry whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't playing.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Not on NPC's.\n\r", ch);
	return;
    }

    if (get_trust(victim) >= get_trust(ch)) {
	send_to_char("That's not a good idea.\n\r", ch);
	return;
    }

    furniture_check(victim);
    act("You summon a huge bolt of `&lightning`` which utterly destroys $N ",
	    ch, NULL, victim, TO_CHAR);
    act("A huge bolt of `&lightning`` strikes $N, utterly `!destroying`` $s",
	    ch, NULL, victim, TO_NOTVICT);
    act("You look up, just in time to see the `!flaming`` `&lightning`` bolt strike your head.\n\r C-ya!\n\r",
	    ch, NULL, victim, TO_VICT);

    fry_char(victim, "");

    do_echo(ch, "You hear a rumble of ```5thunder``.\n\r");
    return;
}

void do_pardon(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg1[MIL];
    char arg2[MIL];

    DENY_NPC(ch)

	argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
	send_to_char("Syntax: pardon <character> <killer|thief|target>.\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Not on NPC's.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "killer")) {
	if (IS_SET(victim->act, PLR_KILLER)) {
	    REMOVE_BIT(victim->act, PLR_KILLER);
	    send_to_char("Killer flag removed.\n\r", ch);
	    send_to_char("You are no longer a KILLER.\n\r", victim);
	}
	return;
    }

    if (!str_cmp(arg2, "thief")) {
	if (IS_SET(victim->act, PLR_THIEF)) {
	    REMOVE_BIT(victim->act, PLR_THIEF);
	    send_to_char("Thief flag removed.\n\r", ch);
	    send_to_char("You are no longer a THIEF.\n\r", victim);
	}
	return;
    }


    send_to_char("Syntax: pardon <character> <killer|thief|target>.\n\r", ch);
    return;
}

void do_transfer(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = ch };
    char arg1[MIL];
    char arg2[MIL];
    ROOM_INDEX_DATA *location;
    CHAR_DATA *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    DENY_NPC(ch)

	if (arg1[0] == '\0') {
	    send_to_char("Transfer whom(and where)?\n\r", ch);
	    return;
	}

    if (arg2[0] == '\0')
	location = ch->in_room;

    if (!str_cmp(arg1, "all")) {
	DESCRIPTOR_DATA *d;
	DESCRIPTOR_DATA *dpending;

	dpending = descriptor_iterator_start(&playing_filter);
	while ((d = dpending) != NULL) {
	    dpending = descriptor_iterator(d, &playing_filter);

	    if (d->character->in_room != NULL && get_trust(d->character) < get_trust(ch) && can_see(ch, d->character)) {
		char buf[MSL];
		sprintf(buf, "%s %s", d->character->name, arg2);
		do_transfer(ch, buf);
	    }
	}
	return;
    }

    /** Thanks to Grodyn for the optional location parameter. */
    if (arg2[0] == '\0') {
	location = ch->in_room;
    } else {
	if ((location = find_location(ch, arg2)) == NULL) {
	    send_to_char("No such location.\n\r", ch);
	    return;
	}

	if (!is_room_owner(ch, location) && room_is_private(location) && get_trust(ch) < LEVEL_IMMORTAL) {
	    send_to_char("That room is private right now.\n\r", ch);
	    return;
	}
    }

    if ((victim = get_char_world(ch, arg1)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (victim->in_room == NULL) {
	send_to_char("They are in limbo.\n\r", ch);
	return;
    }

    if (get_trust(victim) >= get_trust(ch)) {
	send_to_char("I don't think so ..\n\r", ch);
	return;
    }

    if (victim->fighting != NULL)
	stop_fighting(victim, true);
    act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, location);
    act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM);
    if (ch != victim)
	act("$n has transferred you.", ch, NULL, victim, TO_VICT);
    do_look(victim, "auto");
    send_to_char("Ok.\n\r", ch);
}

void do_goto(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    send_to_char("Goto where?\n\r", ch);
	    return;
	}

    if ((location = find_location(ch, argument)) == NULL) {
	send_to_char("No such location.\n\r", ch);
	return;
    }

    count = 0;
    for (rch = location->people; rch != NULL; rch = rch->next_in_room)
	count++;

    if (!is_room_owner(ch, location) && room_is_private(location)
	    && (count > 1 || get_trust(ch) < LEVEL_IMMORTAL)) {
	send_to_char("That room is private right now.\n\r", ch);
	return;
    }

    if (ch->fighting != NULL)
	stop_fighting(ch, true);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
	if (get_trust(rch) >= ch->invis_level) {
	    if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
		act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
	    else
		act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
	}
    }

    char_from_room(ch);
    char_to_room(ch, location);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
	if (get_trust(rch) >= ch->invis_level) {
	    if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
		act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
	    else
		act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT);
	}
    }

    do_look(ch, "auto");
    return;
}

void do_violate(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    send_to_char("Goto where?\n\r", ch);
	    return;
	}

    if ((location = find_location(ch, argument)) == NULL) {
	send_to_char("No such location.\n\r", ch);
	return;
    }

    if (!room_is_private(location)) {
	send_to_char("That room isn't private, use goto.\n\r", ch);
	return;
    }

    if (ch->fighting != NULL)
	stop_fighting(ch, true);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
	if (get_trust(rch) >= ch->invis_level) {
	    if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
		act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
	    else
		act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
	}
    }

    char_from_room(ch);
    char_to_room(ch, location);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
	if (get_trust(rch) >= ch->invis_level) {
	    if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
		act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
	    else
		act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT);
	}
    }

    do_look(ch, "auto");
    return;
}

void do_reboo(CHAR_DATA *ch, char *argument)
{
    send_to_char("If you want to REBOOT, spell it out.\n\r", ch);
    return;
}

void do_reboot(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *dpending;
    char buf[MSL];

    DENY_NPC(ch)

	if (ch) {
	    sprintf(buf, "%s reaches for the little red reset button.", ch->name);
	    do_echo(ch, buf);
	} else {
	    do_echo(NULL, "Reboot by Acid-Fiend-1");
	}

    do_force(ch, "all save");
    if (ch)
	do_save(ch, "");
    globalSystemState.merc_down = true;

    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &descriptor_empty_filter);
	close_socket(d);
    }
}

void do_shutdow(CHAR_DATA *ch, char *argument)
{
    send_to_char("If you want to SHUTDOWN, spell it out.\n\r", ch);
}

void do_shutdown(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    DESCRIPTOR_DATA *d, *dpending;
    char buf[MSL];

    DENY_NPC(ch)

	if (ch->invis_level < LEVEL_HERO) {
	    sprintf(buf, "Shutdown by %s.\n\r", ch->name);
	    do_echo(ch, buf);
	}

    sprintf(buf, "Shutdown by %s.", ch->name);
    append_file(ch, SHUTDOWN_FILE, buf);

    do_force(ch, "all save");
    do_save(ch, "");
    globalSystemState.merc_down = true;

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &playing_filter);
	close_socket(d);
    }
}

void do_snoop(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *dpending;
    CHAR_DATA *victim;
    char buf[MSL];
    char arg[MIL];

    DENY_NPC(ch)

	one_argument(argument, arg);

    if (arg[0] == '\0') {
	send_to_char("Snoop whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (victim->desc == NULL) {
	send_to_char("No descriptor to snoop.\n\r", ch);
	return;
    }

    if (victim == ch) {
	send_to_char("Cancelling all snoops.\n\r", ch);
	wiznet("$N stops being such a snoop.", ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust(ch));

	dpending = descriptor_iterator_start(&descriptor_empty_filter);
	while ((d = dpending) != NULL) {
	    dpending = descriptor_iterator(d, &descriptor_empty_filter);
	    if (d->snoop_by == ch->desc)
		d->snoop_by = NULL;
	}
	return;
    }

    if (victim->desc->snoop_by != NULL) {
	send_to_char("Busy already.\n\r", ch);
	return;
    }

    if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room
	    && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
	send_to_char("That character is in a private room.\n\r", ch);
	return;
    }

    if (ch->desc != NULL) {
	for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by) {
	    if (d->character == victim || d->original == victim) {
		send_to_char("No snoop loops.\n\r", ch);
		return;
	    }
	}
    }

    victim->desc->snoop_by = ch->desc;
    sprintf(buf, "$N starts snooping on %s", (IS_NPC(ch) ? victim->short_descr : victim->name));
    wiznet(buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust(ch));

    send_to_char("Ok.\n\r", ch);

    return;
}

/* SnoopList ..  November 1996  */
void do_snlist(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *dpending;
    char buf[MSL];

    DENY_NPC(ch)

	send_to_char("Currently snooped characters\n\r", ch);
    send_to_char("----------------------------\n\r", ch);

    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
	CHAR_DATA *wch;

	dpending = descriptor_iterator(d, &descriptor_empty_filter);
	wch = CH(d);

	if (d->snoop_by != NULL) {
	    sprintf(buf, "%-15s\n\r", wch->name);
	    page_to_char(buf, ch);
	}
    }

    return;
}

void do_switch(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];

    DENY_NPC(ch)

	one_argument(argument, arg);

    if (arg[0] == '\0') {
	send_to_char("Switch into whom?\n\r", ch);
	return;
    }

    if (ch->desc == NULL)
	return;


    if (ch->desc->original != NULL) {
	send_to_char("You are already switched.\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (victim == ch) {
	send_to_char("Ok.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim)) {
	send_to_char("You can only switch into mobiles.\n\r", ch);
	return;
    }

    if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room
	    && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
	send_to_char("That character is in a private room.\n\r", ch);
	return;
    }

    if (victim->desc != NULL) {
	send_to_char("Character in use.\n\r", ch);
	return;
    }

    sprintf(buf, "$N switches into %s", victim->short_descr);
    wiznet(buf, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, get_trust(ch));

    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;

    /* change communications to match */
    if (ch->prompt != NULL)
	victim->prompt = str_dup(ch->prompt);

    victim->comm = ch->comm;
    victim->lines = ch->lines;

    send_to_char("Ok.\n\r", victim);
    return;
}

void do_return(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    if (ch->desc == NULL)
	return;

    if (ch->desc->original == NULL) {
	send_to_char("You aren't switched.\n\r", ch);
	return;
    }

    send_to_char("You return to your original body. Type replay to see any missed tells.\n\r", ch);
    if (ch->prompt != NULL) {
	free_string(ch->prompt);
	ch->prompt = NULL;
    }

    sprintf(buf, "$N returns from %s.", ch->short_descr);
    wiznet(buf, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, get_trust(ch));

    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check(CHAR_DATA *ch, GAMEOBJECT *obj)
{
    if (IS_TRUSTED(ch, GOD)
	    || (IS_TRUSTED(ch, IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
	    || (IS_TRUSTED(ch, DEMI) && obj->level <= 10 && obj->cost <= 500)
	    || (IS_TRUSTED(ch, ANGEL) && obj->level <= 5 && obj->cost <= 250)
	    || (IS_TRUSTED(ch, AVATAR) && obj->level == 0 && obj->cost <= 100))
	return true;
    else
	return false;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, GAMEOBJECT *obj, GAMEOBJECT *clone)
{
    GAMEOBJECT *c_obj;
    GAMEOBJECT *t_obj;

    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content) {
	if (obj_check(ch, c_obj)) {
	    t_obj = create_object(c_obj->objprototype, 0);
	    clone_object(c_obj, t_obj);
	    obj_to_obj(t_obj, clone);
	    recursive_clone(ch, c_obj, t_obj);
	}
    }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    GAMEOBJECT *obj;
    char arg[MIL];
    char buf[MIL];
    char *rest;
    int count;
    int iter;

    count = 0;
    rest = one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Clone what?\n\r", ch);
	return;
    }

    if (arg[0] == '*') {
	send_to_char("You must put the number of items you want BEFORE the *.\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "clone")) {
	send_to_char("Check your typing.  You only need to type clone ONCE.\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "object")) {
	mob = NULL;

	count = mult_argument(rest, arg);
	obj = get_obj_here(ch, arg);
	if (obj == NULL) {
	    send_to_char("You don't see that here.\n\r", ch);
	    return;
	}
    } else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
	obj = NULL;

	count = mult_argument(rest, arg);
	mob = get_char_room(ch, arg);
	if (mob == NULL) {
	    send_to_char("You don't see that here.\n\r", ch);
	    return;
	}
    } else { /* find both */
	count = mult_argument(argument, arg);
	mob = get_char_room(ch, arg);
	obj = get_obj_here(ch, arg);

	if (mob == NULL && obj == NULL) {
	    send_to_char("You don't see that here.\n\r", ch);
	    return;
	}
    }

    if (count > 100) {
	send_to_char("You can clone a maximum of 100 objects or mobiles at once.\n\r", ch);
	return;
    }

    /* clone an object */
    if (obj != NULL) {
	if (!obj_check(ch, obj)) {
	    send_to_char("Your powers are not great enough for such a task.\n\r", ch);
	    return;
	} else {
	    GAMEOBJECT *clone = NULL;

	    for (iter = 0; iter < count; iter++) {
		clone = create_object(obj->objprototype, 0);
		clone_object(obj, clone);

		if (obj->carried_by != NULL)
		    obj_to_char(clone, ch);
		else
		    obj_to_room(clone, ch->in_room);
		recursive_clone(ch, obj, clone);
	    }

	    if (count > 1) {
		sprintf(buf, "$n has created [%d] $p.", count);
		act(buf, ch, clone, NULL, TO_ROOM);
		sprintf(buf, "You clone [%d] $p.", count);
		act(buf, ch, clone, NULL, TO_CHAR);
		sprintf(buf, "$N clones [%d] $p.", count);
		wiznet(buf, ch, clone, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
	    } else {
		act("$n has created $p.", ch, clone, NULL, TO_ROOM);
		act("You clone $p.", ch, clone, NULL, TO_CHAR);
		wiznet("$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
	    }
	    return;
	}
    } else if (mob != NULL) {
	if (!IS_NPC(mob)) {
	    send_to_char("You can only clone mobiles.\n\r", ch);
	    return;
	} else
	    if ((mob->level > 20 && !IS_TRUSTED(ch, GOD))
		    || (mob->level > 10 && !IS_TRUSTED(ch, IMMORTAL))
		    || (mob->level > 5 && !IS_TRUSTED(ch, DEMI))
		    || (mob->level > 0 && !IS_TRUSTED(ch, ANGEL))
		    || !IS_TRUSTED(ch, AVATAR)) {
		send_to_char("Your powers are not great enough for such a task.\n\r", ch);
		return;
	    } else {
		CHAR_DATA *clone = NULL;
		char buf[MSL];

		for (iter = 0; iter < count; iter++) {
		    clone = create_mobile(mob->mob_idx);
		    clone_mobile(mob, clone);

		    for (obj = mob->carrying; obj != NULL; obj = obj->next_content) {
			if (obj_check(ch, obj)) {
			    GAMEOBJECT *new_obj = create_object(obj->objprototype, 0);
			    clone_object(obj, new_obj);
			    recursive_clone(ch, obj, new_obj);
			    obj_to_char(new_obj, clone);
			    new_obj->wear_loc = obj->wear_loc;
			}
		    }
		    char_to_room(clone, ch->in_room);
		}

		if (clone != NULL) {
		    if (count > 1) {
			sprintf(buf, "$n has created [%d] $N.", count);
			act(buf, ch, NULL, clone, TO_ROOM);
			sprintf(buf, "You clone [%d] $N.", count);
			act(buf, ch, NULL, clone, TO_CHAR);
			sprintf(buf, "$N clones [%d] %s.", count, clone->short_descr);
			wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
		    } else {
			act("$n has created $N.", ch, NULL, clone, TO_ROOM);
			act("You clone $N.", ch, NULL, clone, TO_CHAR);
			sprintf(buf, "$N clones %s.", clone->short_descr);
			wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
		    }
		}

		return;
	    }
    }
}

void do_load(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];

    DENY_NPC(ch)

	argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
	send_to_char("Syntax:\n\r", ch);
	send_to_char("  load mob <vnum>\n\r", ch);
	send_to_char("  load obj <vnum> <level>\n\r", ch);
	send_to_char("  load char <character>\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "mob")) {
	do_mload(ch, argument);
	return;
    }

    if (!str_cmp(arg, "obj")) {
	do_oload(ch, argument);
	return;
    }

    if (!str_cmp(arg, "char")) {
	char newarg[MIL];
	sprintf(newarg, "%s %s", "pload", argument);
	interpret(ch, newarg);
	return;
    }

    /* echo syntax */
    do_load(ch, "");
}

void do_mload(CHAR_DATA *ch, char *argument)
{
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg)) {
	send_to_char("Syntax: load mob <vnum>.\n\r", ch);
	return;
    }

    if ((pMobIndex = get_mob_index(parse_int(arg))) == NULL) {
	send_to_char("No mob has that vnum.\n\r", ch);
	return;
    }

    victim = create_mobile(pMobIndex);
    char_to_room(victim, ch->in_room);

    act("$n has created $N!", ch, NULL, victim, TO_ROOM);
    snprintf(buf, MSL, "$N loads %s.", victim->short_descr);
    wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));

    snprintf(buf, MSL, "mload: Loaded mob: %s\n\r", victim->short_descr);
    send_to_char(buf, ch);
    return;
}

void do_oload(CHAR_DATA *ch, char *argument)
{
    OBJECTPROTOTYPE *pObjIndex;
    GAMEOBJECT *obj;
    char arg1[MIL];
    char arg2[MIL];
    char buf[MSL];
    int level;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1)) {
	send_to_char("Syntax: load obj <vnum> <level>.\n\r", ch);
	return;
    }

    level = -1;
    if (arg2[0] != '\0') { /* load with a level */
	if (!is_number(arg2)) {
	    send_to_char("Syntax: oload <vnum> <level>.\n\r", ch);
	    return;
	}

	level = parse_int(arg2);

	if (level < 0
		|| (get_trust(ch) != MAX_LEVEL
		    && (level > get_trust(ch)))) {
	    send_to_char("Level must be be between 0 and your level.\n\r", ch);
	    return;
	}
    }

    if ((pObjIndex = objectprototype_getbyvnum(parse_int(arg1))) == NULL) {
	send_to_char("No object has that vnum.\n\r", ch);
	return;
    }

    obj = create_object(pObjIndex, level);

    if (CAN_WEAR(obj, ITEM_TAKE))
	obj_to_char(obj, ch);
    else
	obj_to_room(obj, ch->in_room);

    act("$n has created $p!", ch, obj, NULL, TO_ROOM);
    wiznet("$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, get_trust(ch));

    snprintf(buf, MSL, "oload: Loaded object: %s\n\r", obj->short_descr);
    send_to_char(buf, ch);
}

void do_purge(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    GAMEOBJECT *obj;
    char arg[MIL];
    char buf[100];

    one_argument(argument, arg);

    DENY_NPC(ch)

	if (arg[0] == '\0') {
	    /* 'purge' */
	    CHAR_DATA *vnext;
	    GAMEOBJECT *obj_next;

	    for (victim = ch->in_room->people; victim != NULL; victim = vnext) {
		vnext = victim->next_in_room;
		if (IS_NPC(victim)
			&& !IS_SET(victim->act, ACT_NOPURGE)
			&& victim != ch)
		    extract_char(victim, true);
	    }

	    for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
		obj_next = obj->next_content;
		if (!IS_OBJ_STAT(obj, ITEM_NOPURGE))
		    extract_obj(obj);
	    }

	    act("$n purges the room!", ch, NULL, NULL, TO_ROOM);
	    send_to_char("Ok.\n\r", ch);
	    return;
	}

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim)) {
	if (ch == victim) {
	    send_to_char("Ho ho ho.\n\r", ch);
	    return;
	}

	if ((get_trust(ch) <= get_trust(victim)) && (victim->desc != NULL)) {
	    send_to_char("Maybe that wasn't a good idea...\n\r", ch);
	    sprintf(buf, "%s tried to purge you!\n\r", ch->name);                   send_to_char(buf, victim);
	    return;
	}

	act("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT);
	act("You turn $N to ash!", victim, NULL, NULL, TO_CHAR);
	act("$n disintegrates you!", ch, NULL, NULL, TO_VICT);

	if (victim->level > 1)
	    save_char_obj(victim);

	d = victim->desc;
	extract_char(victim, true);
	if (d != NULL)
	    close_socket(d);

	return;
    }

    act("$n purges $N.", ch, NULL, victim, TO_NOTVICT);
    extract_char(victim, true);
    return;
}

void do_advance(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    char buf[MSL];
    int level;

    DENY_NPC(ch)

	argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
	send_to_char("Syntax: advance <char> <level>.\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL) {
	send_to_char("That player is not here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Not on NPC's.\n\r", ch);
	return;
    }


    if (arg2[0] == '+') {
	if (victim->level >= LEVEL_HERO - 1)
	    return;

	if (arg3[0] != '\0' && parse_int(arg2) == 0)
	    level = UMIN(victim->level + parse_int(arg3), LEVEL_HERO - 1);
	else
	    level = UMIN(victim->level + parse_int(arg2), LEVEL_HERO - 1);
    } else if (arg2[0] == '-') {
	if (victim->level >= LEVEL_HERO || victim->level <= 1)
	    return;

	if (arg3[0] != '\0' && parse_int(arg2) == 0)
	    level = UMAX(victim->level - parse_int(arg3), 1);
	else
	    level = UMAX(victim->level + parse_int(arg2), 1);

    } else {
	level = parse_int(arg2);
    }

    if (level < 1 || level > MAX_LEVEL) {
	send_to_char("Level must be 1 to 610.\n\r", ch);
	return;
    }

    if (level == 1 && CHECK_NON_OWNER(ch)) {
	sick_harvey_proctor(ch, hp_irritated, "You may only demote to level 2.");
	level = 2;
    }

    if (level == victim->level) {
	send_to_char("Level must be different than the characters current level.\n\r", ch);
	return;
    }


    if (get_trust(victim) >= get_trust(ch)) {
	send_to_char("Not at your trust level!\n\r", ch);
	return;
    }

    if (level > get_trust(ch)) {
	send_to_char("Limited to your trust level.\n\r", ch);
	return;
    }

    if (level <= victim->level) {
	printf_to_char(ch, "Lowering %s's level!\n\r", victim->name);
	send_to_char("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim);
    } else {
	printf_to_char(ch, "Raising %s's level!\n\r", victim->name);
	send_to_char("```O**** ```4OOOOHHHHHHHHHH  YYYYEEEESSS ```O****``\n\r", victim);
    }

    /* if we're lowering them back down to mortal status,    */
    /* then delete their god-file.  Added by Monrick, 1/2008 */
    if (IS_IMMORTAL(victim) && (level < LEVEL_IMMORTAL)) {
	sprintf(buf, "%s%s", GOD_DIR, victim->name);
	unlink(buf);
    }

    advance_level(victim, level - victim->level);

    victim->exp = exp_per_level(victim, victim->pcdata->points) * UMAX(1, victim->level);
    victim->trust = 0;

    save_char_obj(victim);
    return;
}

void do_trust(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg1[MIL];
    char arg2[MIL];
    int level;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    DENY_NPC(ch)
	if (CHECK_NON_OWNER(ch)) {
	    sick_harvey_proctor(ch, hp_off_his_rocker, "NO! NO! NO!");
	    return;
	}

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
	send_to_char("Syntax: trust <char> <level>.\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL) {
	send_to_char("That player is not here.\n\r", ch);
	return;
    }

    level = parse_int(arg2);
    if (level > MAX_LEVEL) {
	send_to_char("Level must be 0(reset) or 1 to 610.\n\r", ch);
	return;
    }

    if (level > get_trust(ch)) {
	send_to_char("Limited to your trust.\n\r", ch);
	return;
    }

    if (get_trust(victim) >= get_trust(ch)) {
	send_to_char("Not at your trust level!\n\r", ch);
	return;
    }

    victim->trust = level;
    return;
}

void do_affstrip(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char target[MIL];
    char affect[MSL];

    argument = one_argument(argument, target);

    DENY_NPC(ch);

    if (target[0] == '\0') {
	send_to_char("Affect strip whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, target)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (ch != victim && get_trust(victim) >= get_trust(ch)) {
	send_to_char("You failed.\n\r", ch);
	return;
    }


    argument = one_argument(argument, affect);

    if (affect[0] == '\0') {
	remove_all_affects(victim);
	send_to_char("You feel your being stripped of all magical affects.\n\r", victim);
    } else {
	SKILL *skill;

	if ((skill = skill_lookup(affect)) != NULL) {
	    if (is_affected(victim, skill)) {
		affect_strip(victim, skill);
		printf_to_char(victim, "You have been stripped of the %s affect.\n\r",
			skill->name);
	    } else {
		send_to_char("They are not affected by that.\n\r", ch);
		return;
	    }
	} else {
	    send_to_char("That is not a valid affect.\n\r", ch);
	    return;
	}
    }

    send_to_char("Done.\n\r", ch);
    save_char_obj(victim);

    return;
}

void do_log(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    DENY_NPC(ch)

	if (arg[0] == '\0') {
	    send_to_char("Log whom?\n\r", ch);
	    return;
	}

    if (!str_cmp(arg, "all")) {
	globalSystemState.log_all = !globalSystemState.log_all;
	if (globalSystemState.log_all) {
	    send_to_char("Log ALL on.\n\r", ch);
	} else {
	    send_to_char("Log ALL off.\n\r", ch);
	}
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("Not on NPC's.\n\r", ch);
	return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (IS_SET(victim->act, PLR_LOG)) {
	REMOVE_BIT(victim->act, PLR_LOG);
	send_to_char("LOG removed.\n\r", ch);
    } else {
	SET_BIT(victim->act, PLR_LOG);
	send_to_char("LOG set.\n\r", ch);
    }

    return;
}

void do_peace(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;

    DENY_NPC(ch)

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
	    if (rch->fighting != NULL)
		stop_fighting(rch, true);

	    if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
		REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
	}

    send_to_char("Ok.\n\r", ch);
    return;
}

void do_wizlock(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch)

	globalSystemState.wizlock = !globalSystemState.wizlock;

    if (globalSystemState.wizlock) {
	wiznet("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
	send_to_char("Game wizlocked.\n\r", ch);
    } else {
	wiznet("$N removes wizlock.", ch, NULL, 0, 0, 0);
	send_to_char("Game un-wizlocked.\n\r", ch);
    }
}

void do_newlock(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch);

    globalSystemState.newlock = !globalSystemState.newlock;

    if (globalSystemState.newlock) {
	wiznet("$N locks out new characters.", ch, NULL, 0, 0, 0);
	send_to_char("New characters have been locked out.\n\r", ch);
    } else {
	wiznet("$N allows new characters back in.", ch, NULL, 0, 0, 0);
	send_to_char("Newlock removed.\n\r", ch);
    }
}

void do_mode(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];
    char arg2[MIL];

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    DENY_NPC(ch);

    if (arg[0] == '\0') {
	send_to_char("set which mode?\n\r", ch);
	send_to_char("	debug  status\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "status")) {
	send_to_char("`PB`5ad `@T`2rip`7 mode status:\n\r", ch);

	send_to_char("`5Wizlock  `7 :                ", ch);
	if (globalSystemState.wizlock)
	    send_to_char("ON\n\r", ch);
	else
	    send_to_char("`8OFF`7\n\r", ch);

	send_to_char("`#Newlock  `7 :                ", ch);
	if (globalSystemState.newlock)
	    send_to_char("ON\n\r", ch);
	else
	    send_to_char("`8OFF`7\n\r", ch);

    }



    if (!str_prefix(arg, "wizlock")) {
	do_wizlock(ch, "");
	return;
    }

    if (!str_prefix(arg, "newlock")) {
	do_newlock(ch, "");
	return;
    }

    do_mode(ch, "");
}

void do_slot(CHAR_DATA *ch, char *argument)
{
    SKILL *skill;
    char arg[MIL];

    DENY_NPC(ch);

    smash_tilde(argument);
    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
	send_to_char("Syntax`8: ``slot <spell name>\n\r", ch);
	return;
    }

    if ((skill = skill_lookup(arg)) == NULL
	    || skill->spells == NULL) {
	send_to_char("That spell does not exist!\n\r", ch);
	return;
    }

    printf_to_char(ch, "The spell '%s' is slot number %d.\n\r", skill->name, skill->sn);
    return;
}

void do_sockets(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *dpending;
    BUFFER *buf;
    char *state;
    char arg[MIL];
    char sock[MSL];
    int count;

    DENY_NPC(ch);

    count = 0;
    buf = new_buf();

    one_argument(argument, arg);
    send_to_char("`$Connection state            Socket  Name           IP Address", ch);
    send_to_char("``--------------------------+-------+--------------+------------------------------\n\r", ch);
    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &descriptor_empty_filter);

	if (d->character != NULL && can_see(ch, d->character)
		&& (arg[0] == '\0' || is_name(arg, d->character->name)
		    || (d->original && is_name(arg, d->original->name)))) {
	    count++;

	    switch (d->connected) {
		case CON_PLAYING:
		    state = "Connected and playing";
		    break;
		case CON_GET_NAME:
		    state = "Identifying remote user";
		    break;
		case CON_GET_OLD_PASSWORD:
		    state = "Entering password";
		    break;
		case CON_CONFIRM_NEW_NAME:
		    state = "Newbie: confirming name";
		    break;
		case CON_GET_NEW_PASSWORD:
		    state = "Entering new password";
		    break;
		case CON_CONFIRM_NEW_PASSWORD:
		    state = "Confirming new password";
		    break;
		case CON_GET_NEW_RACE:
		    state = "Newbie: Selecting race";
		    break;
		case CON_GET_NEW_SEX:
		    state = "Newbie: Selecting sex";
		    break;
		case CON_GET_NEW_CLASS:
		    state = "Newbie: Selecting class";
		    break;
		case CON_DEFAULT_CHOICE:
		    state = "Newbie: Getting default";
		    break;
		case CON_GEN_GROUPS:
		    state = "Newbie: Choosing groups";
		    break;
		case CON_PICK_WEAPON:
		    state = "Newbie: Choosing weapon";
		    break;
		case CON_READ_IMOTD:
		    state = "Reading IMOTD";
		    break;
		case CON_READ_MOTD:
		    state = "Reading MOTD";
		    break;
		case CON_BREAK_CONNECT:
		    state = "Breaking Connection";
		    break;
		case CON_GET_ANSI:
		    state = "Choosing ANSI [Y/n]";
		    break;
		default:
		    state = "-Unknown-";
		    break;
	    }

	    sprintf(sock, "`!%s``", d->host);
	    printf_buf(buf, "`@%-25.25s`` | `^%3d``   | %-12.12s | %s\n\r",
		    state,
		    d->descriptor,
		    d->original ? d->original->name :
		    d->character ? d->character->name : "(none)",
		    sock);
	}
    }

    if (count == 0) {
	send_to_char("No one by that name is connected.\n\r", ch);
	return;
    }

    add_buf(buf, "--------------------------+-------+--------------+------------------------------\n\r");
    printf_buf(buf, "%d user%s\n\r", count, count == 1 ? "" : "s");

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
    return;
}

void do_force(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];
    char arg[MIL];
    char arg2[MIL];

    argument = one_argument(argument, arg);


    if (arg[0] == '\0' || argument[0] == '\0') {
	send_to_char("Force whom to do what?\n\r", ch);
	return;
    }

    one_argument(argument, arg2);

    if (!str_cmp(arg2, "suicide")) {
	send_to_char("Find a more creative way to kill ..\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "mob")) {
	send_to_char("That will NOT be done.\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "drag")) {
	send_to_char("Sorry, but that seems to crash the mud ..\n\r", ch);
	send_to_char("So `5DON'T DO IT`7! ..\n\r", ch);
	return;
    }

    if (!str_cmp(arg2, "pk")) {
	send_to_char("I'm sure they can make up their own mind ..\n\r", ch);
	return;
    }

    sprintf(buf, "$n forces you to '%s'.", argument);
    if (!str_cmp(arg, "all")) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (get_trust(ch) < MAX_LEVEL - 3) {
	    send_to_char("Not at your level!\n\r", ch);
	    return;
	}

	for (vch = char_list; vch != NULL; vch = vch_next) {
	    vch_next = vch->next;

	    if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)) {
		if (ch) {
		    act(buf, ch, NULL, vch, TO_VICT);
		} else {
		    sprintf(buf, "Acid-Fiend-1 forces you to '%s'.\n\r", argument);
		    send_to_char(buf, vch);
		}

		if (!IS_NPC(vch) && vch->desc != NULL)
		    substitute_alias(vch->desc, argument);
		else
		    interpret(vch, argument);
	    }
	}
    } else if (!str_cmp(arg, "players")) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (get_trust(ch) < MAX_LEVEL - 2) {
	    send_to_char("Not at your level!\n\r", ch);
	    return;
	}

	for (vch = char_list; vch != NULL; vch = vch_next) {
	    vch_next = vch->next;

	    if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)
		    && vch->level < LEVEL_HERO) {
		act(buf, ch, NULL, vch, TO_VICT);
		interpret(vch, argument);
	    }
	}
    } else if (!str_cmp(arg, "gods")) {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (get_trust(ch) < MAX_LEVEL - 2) {
	    send_to_char("Not at your level!\n\r", ch);
	    return;
	}

	for (vch = char_list; vch != NULL; vch = vch_next) {
	    vch_next = vch->next;

	    if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)
		    && vch->level >= LEVEL_HERO) {
		act(buf, ch, NULL, vch, TO_VICT);
		interpret(vch, argument);
	    }
	}
    } else {
	CHAR_DATA *victim;

	if ((victim = get_char_world(ch, arg)) == NULL) {
	    send_to_char("They aren't here.\n\r", ch);
	    return;
	}

	if (victim == ch) {
	    send_to_char("Aye aye, right away!\n\r", ch);
	    return;
	}

	if (!is_room_owner(ch, victim->in_room)
		&& ch->in_room != victim->in_room
		&& room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
	    send_to_char("That character is in a private room.\n\r", ch);
	    return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
	    send_to_char("Do it yourself!\n\r", ch);
	    return;
	}

	if (!IS_NPC(victim) && get_trust(ch) < MAX_LEVEL - 3) {
	    send_to_char("Not at your level!\n\r", ch);
	    return;
	}

	act(buf, ch, NULL, victim, TO_VICT);
	if (!IS_NPC(victim) && victim->desc != NULL)
	    substitute_alias(victim->desc, argument);
	else
	    interpret(victim, argument);
    }

    if (ch)
	send_to_char("Ok.\n\r", ch);
    return;
}

void do_winvis(CHAR_DATA *ch, char *argument)
{
    int level;
    char arg[MSL];

    /* RT code for taking a level argument */
    (void)one_argument(argument, arg);

    DENY_NPC(ch)

	if (arg[0] == '\0') {
	    /* take the default path */

	    if (ch->invis_level) {
		ch->invis_level = 0;
		act("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM);
		send_to_char("You slowly fade back into existence.\n\r", ch);
	    } else {
		ch->invis_level = get_trust(ch);
		act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
		send_to_char("You slowly vanish into thin air.\n\r", ch);
	    }
	} else {
	    /* do the level thing */
	    level = parse_int(arg);
	    if (level < 2 || level > get_trust(ch)) {
		send_to_char("Invis level must be between 2 and your level.\n\r", ch);
		return;
	    } else {
		ch->reply = NULL;
		ch->invis_level = level;
		act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
		send_to_char("You slowly vanish into thin air.\n\r", ch);
	    }
	}

    return;
}

void do_incognito(CHAR_DATA *ch, char *argument)
{
    int level;
    char arg[MSL];

    DENY_NPC(ch)

	/* RT code for taking a level argument */
	one_argument(argument, arg);

    if (arg[0] == '\0') {
	/* take the default path */

	if (ch->incog_level) {
	    ch->incog_level = 0;
	    act("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You are no longer cloaked.\n\r", ch);
	} else {
	    ch->incog_level = get_trust(ch);
	    act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You cloak your presence.\n\r", ch);
	}
    } else {
	/* do the level thing */
	level = parse_int(arg);
	if (level < 2 || level > get_trust(ch)) {
	    send_to_char("Incog level must be between 2 and your level.\n\r", ch);
	    return;
	} else {
	    ch->reply = NULL;
	    ch->incog_level = level;
	    act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("You cloak your presence.\n\r", ch);
	}
    }

    return;
}

void do_holylight(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_HOLYLIGHT)) {
	REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char("Holy light mode off.\n\r", ch);
    } else {
	SET_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char("Holy light mode on.\n\r", ch);
    }

    return;
}

void do_prefi(CHAR_DATA *ch, char *argument)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n", ch);
    return;
}

void do_prefix(CHAR_DATA *ch, char *argument)
{
    char buf[MIL];

    if (ch) {
	if (IS_NPC(ch)) {
	    send_to_char("Mobs can't use this command.\n\r", ch);
	    return;
	}
    }

    if (argument[0] == '\0') {
	if (ch->pcdata->prefix[0] == '\0') {
	    send_to_char("You have no prefix to clear.\r\n", ch);
	    return;
	}

	send_to_char("Prefix removed.\r\n", ch);
	free_string(ch->pcdata->prefix);
	ch->pcdata->prefix = str_dup("");
	return;
    }

    if (ch->pcdata->prefix[0] != '\0') {
	sprintf(buf, "Prefix changed to %s.\r\n", argument);
	free_string(ch->pcdata->prefix);
    } else {
	sprintf(buf, "Prefix set to %s.\r\n", argument);
    }

    ch->pcdata->prefix = str_dup(argument);
}

/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
void do_copyover(CHAR_DATA *ch, char *argument)
{
    if (!copyover()) {
	send_to_char("Copyover FAILED! (check std err for reason.)\n\r", ch);
    }
}

bool check_parse_name(char *name);      /* comm.c */

void do_rename(CHAR_DATA *ch, char *argument)
{
    char old_name[MIL], new_name[MIL], strsave[MIL];

    CHAR_DATA *victim;
    FILE *file;

    argument = one_argument(argument, old_name);
    one_argument(argument, new_name);


    /* Trivial checks */

    if (ch) {
	if (IS_NPC(ch)) {
	    send_to_char("Mobs can't use this command.\n\r", ch);
	    return;
	}
    }

    if (!old_name[0]) {
	send_to_char("Rename who?\n\r", ch);
	return;
    }

    victim = get_char_world(ch, old_name);

    if (!victim) {
	send_to_char("There is no such a person online.\n\r", ch);
	return;
    }

    if (IS_NPC(victim)) {
	send_to_char("You cannot use Rename on NPCs.\n\r", ch);
	return;
    }

    /* allow rename self new_name,but otherwise only lower level */
    if ((victim != ch) && (get_trust(victim) >= get_trust(ch))) {
	send_to_char("You failed.\n\r", ch);
	return;
    }

    if (!new_name[0]) {
	send_to_char("Rename to what new name?\n\r", ch);
	return;
    }

    if (!check_parse_name(new_name)) {
	send_to_char("The new name is illegal.\n\r", ch);
	return;
    }

    /* First, check if there is a player named that off-line */
    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(new_name));

    file = fopen(strsave, "r");     /* attempt to to open pfile */

    if (file) {
	send_to_char("A player with that name already exists!\n\r", ch);
	fclose(file);
	return;
    }

    /* Check .gz file ! */
    sprintf(strsave, "%s%s.gz", PLAYER_DIR, capitalize(new_name));

    file = fopen(strsave, "r");     /* attempt to to open pfile */

    if (file) {
	send_to_char("A player with that name already exists in a compacted file.\n\r", ch);
	fclose(file);
	return;
    }

    if (get_char_world(ch, new_name)) {
	send_to_char("A player with the name you specified already exists.\n\r", ch);
	return;
    }

    /* Save the filename of the old name */

    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(victim->name));

    unlink(strsave);

    /* check immortal stuff, added by Monrick 1/2008 */
    if (IS_IMMORTAL(victim)) {
	sprintf(strsave, "%s%s", GOD_DIR, capitalize(victim->name));
	unlink(strsave);
    }

    free_string(victim->name);
    victim->name = str_dup(capitalize(new_name));

    save_char_obj(victim);

    /* unlink the old file
     * unlink(old_name);  */

    send_to_char("Character renamed.\n\r", ch);

    victim->position = POS_STANDING; /* I am laaazy */
    act("$n has renamed you to $N!", ch, NULL, victim, TO_VICT);
}



void do_pnlist(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d, *dpending;

    DENY_NPC(ch);

    send_to_char("Listing all connected penalized characters:\n\r", ch);
    send_to_char("+----------------------------------------------------------------------+\n\r", ch);
    send_to_char("Name     | Log| Idt| Klr| Thi| SnP| Per| Pns\n\r", ch);
    send_to_char("+----------------------------------------------------------------------+\n\r", ch);

    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
	CHAR_DATA *wch;
	dpending = descriptor_iterator(d, &descriptor_empty_filter);

	wch = CH(d);

	if (wch == NULL || IS_NPC(wch))
	    continue;

	if (!can_see(ch, wch))
	    continue;

	printf_to_char(ch,
		"%-13s%-9s%-9s%-9s%-9s\n\r",
		wch->name,
		IS_SET(wch->act, PLR_LOG) ? "`!X`7" : "`8-`7 ",
		IS_SET(wch->act, PLR_KILLER) ? "`!X`7" : "`8-`7 ",
		IS_SET(wch->act, PLR_THIEF) ? "`!X`7" : "`8-`7 ",
		IS_SET(wch->act, PLR_PERMIT) ? "`!X`7" : "`8-`7 ");
    }

    send_to_char("+---------------------------------------------------------------------------+\n\r", ch);
    return;
}

void do_repop(CHAR_DATA *ch, char *argument)
{
    AREA_DATA *pArea;

    DENY_NPC(ch)

	if (argument[0] == '\0') {
	    reset_area(ch->in_room->area);
	    send_to_char("Area repop!\n\r", ch);
	}


    if (!str_cmp(argument, "world")) {
	for (pArea = area_first; pArea != NULL; pArea = pArea->next) {
	    reset_area(pArea);
	    send_to_char("World Repop!", ch);
	}
    }

    return;
}

void do_omnistat(CHAR_DATA *ch, char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;
    BUFFER *output;
    char buf[MSL];
    int immmatch;
    int mortmatch;
    int hptemp;
    int manatemp = 0;

    DENY_NPC(ch)

	immmatch = 0;
    mortmatch = 0;
    output = new_buf();

    add_buf(output, " ----Immortals:----\n\r");
    add_buf(output, "Name          Level   Wiz   Incog   [Vnum]\n\r");

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	CHAR_DATA *wch;
	dpending = descriptor_iterator(d, &playing_filter);

	wch = CH(d);

	if (!can_see(ch, wch) || wch->level < 601)
	    continue;

	if (IS_IMMORTAL(wch)) {
	    immmatch++;

	    sprintf(buf, "%-14s %-3d    %-3d    %-3d    [%5ld]\n\r",
		    wch->name,
		    wch->level,
		    wch->invis_level,
		    wch->incog_level,
		    wch->in_room->vnum);
	    add_buf(output, buf);
	}
    }


    add_buf(output, " \n\r ----Mortals:----\n\r");
    add_buf(output, "Name           Race/Class   Position        Lev  %%hps  %%mana  [Vnum]\n\r");

    hptemp = 0;
    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
	CHAR_DATA *wch;
	dpending = descriptor_iterator(d, &playing_filter);
	char const *class;

	if (!can_see(ch, d->character))
	    continue;

	wch = CH(d);

	if (!can_see(ch, wch) || wch->level > ch->level || wch->level > 601)
	    continue;

	mortmatch++;
	if ((wch->max_hit != wch->hit) && (wch->hit > 0))
	    hptemp = (wch->hit * 100) / wch->max_hit;
	else if (wch->max_hit == wch->hit)
	    hptemp = 100;
	else if (wch->hit < 0)
	    hptemp = 0;

	if ((wch->max_mana != wch->mana) && (wch->mana > 0))
	    manatemp = (wch->mana * 100) / wch->max_mana;
	else if (wch->max_mana == wch->mana)
	    manatemp = 100;
	else if (wch->mana < 0)
	    manatemp = 0;

	class = class_table[wch->class].who_name;
	sprintf(buf, "%-14s %5s/%3s    %-15s %-3d  %3d%%   %3d%%  [%5ld]\n\r",
		wch->name,
		wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ",
		class,
		capitalize(position_table[wch->position].name),
		wch->level,
		hptemp,
		manatemp,
		wch->in_room->vnum);
	add_buf(output, buf);
    }

    sprintf(buf, "\n\rImmortals found: `O%d`7\n\r", immmatch);
    add_buf(output, buf);

    sprintf(buf, "Mortals found: `O%d`7\n\r", mortmatch);
    add_buf(output, buf);

    page_to_char(buf_string(output), ch);

    free_buf(output);
}

void do_olevel(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];
    char arg[MIL];
    BUFFER *buffer;
    long level;
    bool found;

    argument = one_argument(argument, arg);

    if (ch) {
	if (IS_NPC(ch)) {
	    send_to_char("Mobs can't use this command.\n\r", ch);
	    return;
	}
    }

    if (!is_number(arg)) {
	send_to_char("Syntax: olevel [level]\n\r", ch);
	return;
    }
    level = parse_int(arg);
    buffer = new_buf();
    found = false;

    {
	OBJECTPROTOTYPE *current;
	OBJECTPROTOTYPE *pending;

        pending = objectprototype_iterator_start(&objectprototype_empty_filter);
        while ((current = pending) != NULL) {
            pending = objectprototype_iterator(current, &objectprototype_empty_filter);

	    if (level == current->level) {
		found = true;
		sprintf(buf, "`7[`O%5ld`7] `&%s`7\n\r", current->vnum, current->short_descr);
		add_buf(buffer, buf);
	    }
	}
    }

    if (!found)
	send_to_char("No objects by that level.\n\r", ch);
    else
	page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return;
}

void do_mlevel(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];
    char arg[MIL];
    BUFFER *buffer;
    MOB_INDEX_DATA *pMobIndex;
    long vnum, level;
    long nMatch;
    bool found;

    argument = one_argument(argument, arg);

    if (ch) {
	if (IS_NPC(ch)) {
	    send_to_char("Mobs can't use this command.\n\r", ch);
	    return;
	}
    }

    if (!is_number(arg)) {
	send_to_char("Syntax: mlevel [level]\n\r", ch);
	return;
    }
    level = parse_int(arg);
    buffer = new_buf();
    found = false;
    nMatch = 0;

    for (vnum = 0; nMatch < top_mob_index; vnum++) {
	if ((pMobIndex = get_mob_index(vnum)) != NULL)
	    nMatch++;
	if (level == pMobIndex->level) {
	    found = true;
	    sprintf(buf, "`7[`O%5ld`7] `&%s`7\n\r", pMobIndex->vnum, pMobIndex->short_descr);
	    add_buf(buffer, buf);
	}
    }
    if (!found)
	send_to_char("No mobiles found at that level ..\n\r", ch);
    else
	page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return;
}

void do_setrestore(CHAR_DATA *ch, char *argument)
{
    char arg[MIL];

    argument = one_argument(argument, arg);

    smash_tilde(argument);

    if (IS_NPC(ch)) {
	send_to_char("You're not cool enough for restore strings ..\n\r", ch);
	return;
    }

    if (arg[0] == '\0') {
	send_to_char("Srestore syntax:\n\r", ch);
	send_to_char("  srestore room <text>  :  set restore string for rooms\n\r", ch);
	send_to_char("                           or individual players\n\r", ch);
	send_to_char("  srestore global <text>:  set restore string for global\n\r", ch);
	return;
    }

    if (!str_prefix(arg, "global")) {
	do_grestore(ch, argument);
	return;
    }

    if (!str_prefix(arg, "room")) {
	do_rrestore(ch, argument);
	return;
    }
    do_setrestore(ch, "");
}

void do_grestore(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    smash_tilde(argument);

    if (ch) {
	if (IS_NPC(ch)) {
	    send_to_char("Mobs can't use this command.\n\r", ch);
	    return;
	}
    }

    if (argument[0] == '\0') {
	if (ch->pcdata->grestore_string != NULL) {
	    sprintf(buf, "Your `Oglobal`7 restore string is currently set to:\n%s\n\r",
		    ch->pcdata->grestore_string);
	    send_to_char(buf, ch);
	    return;
	} else {
	    send_to_char("You do not have a global restore string defined\n\r", ch);
	}
    }

    free_string(ch->pcdata->grestore_string);
    ch->pcdata->grestore_string = str_dup(argument);
    sprintf(buf, "Your `Oglobal`7 restore string is now set to:\n%s\n",
	    ch->pcdata->grestore_string);
    send_to_char(buf, ch);
    return;
}

void do_rrestore(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    smash_tilde(argument);

    if (ch) {
	if (IS_NPC(ch)) {
	    send_to_char("Mobs can't use this command.\n\r", ch);
	    return;
	}
    }

    if (argument[0] == '\0') {
	if (ch->pcdata->rrestore_string != NULL) {
	    sprintf(buf, "Your `Oroom`7 restore string is currently set to:\n%s\n\r",
		    ch->pcdata->rrestore_string);
	    send_to_char(buf, ch);
	    return;
	} else {
	    send_to_char("You do not have a restore string for rooms defined\n\r", ch);
	}
    }

    free_string(ch->pcdata->rrestore_string);
    ch->pcdata->rrestore_string = str_dup(argument);
    sprintf(buf, "Your `Oroom`7 restore string is now set to:\n%s\n",
	    ch->pcdata->rrestore_string);
    send_to_char(buf, ch);
    return;
}

void do_review(CHAR_DATA *ch, char *argument)
{
    char buf[MSL];

    if (IS_NPC(ch)) {
	send_to_char("You have no identity ..\n\r", ch);
	return;
    }

    send_to_char("Here is a review of your settings.\n\r", ch);
    sprintf(buf, "Bamfin     : %s\n\r", ch->pcdata->bamfin);
    send_to_char(buf, ch);

    sprintf(buf, "Bamfout    : %s\n\r", ch->pcdata->bamfout);
    send_to_char(buf, ch);

    sprintf(buf, "G. Restore :%s\n\r", ch->pcdata->grestore_string);
    send_to_char(buf, ch);

    sprintf(buf, "R. Restore :%s\n\r", ch->pcdata->rrestore_string);
    send_to_char(buf, ch);

    sprintf(buf, "Title      :%s\n\r", ch->pcdata->title);
    send_to_char(buf, ch);

    sprintf(buf, "Long Desc  : %s\n\r", ch->long_descr);
    send_to_char(buf, ch);

    sprintf(buf, "Short Desc : %s\n\r", ch->short_descr);
    send_to_char(buf, ch);

    sprintf(buf, "Description: %s\n\r", ch->description);
    send_to_char(buf, ch);

    return;
}

/* Expand the name of a character into a string that identifies THAT
 * character within a room. E.g. the second 'guard' -> 2. guard
 */
const char *name_expand(CHAR_DATA *ch)
{
    int count = 1;
    CHAR_DATA *rch;
    char name[MIL];

    static char outbuf[MIL];

    if (!IS_NPC(ch))
	return ch->name;
    one_argument(ch->name, name);   /* copy the first word into name */

    if (!name[0]) {                 /* weird mob .. no keywords */
	strcpy(outbuf, "");
	return outbuf;
    }

    for (rch = ch->in_room->people; rch && (rch != ch); rch = rch->next_in_room)
	if (is_name(name, rch->name))
	    count++;

    sprintf(outbuf, "%d.%s", count, name);
    return outbuf;
}

void do_rdesc(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    char buf[MSL];

    DENY_NPC(ch);

    location = ch->in_room;
    if (location == NULL) {
	send_to_char("No such location.\n\r", ch);
	return;
    }

    if (!is_room_owner(ch, location) && ch->in_room != location
	    && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
	send_to_char("That room is private right now.\n\r", ch);
	return;
    }

    if (argument[0] != '\0') {
	buf[0] = '\0';
	smash_tilde(argument);

	if (argument[0] == '-') {
	    int len, buf_len;
	    bool found = false;

	    if (location->description == NULL || location->description[0] == '\0') {
		send_to_char("No lines left to remove.\n\r", ch);
		return;
	    }

	    strcpy(buf, location->description);
	    buf_len = (int)strlen(buf);
	    for (len = buf_len; len > 0; len--) {
		if (buf[len] == '\r') {
		    if (!found) {            /* back it up */
			if (len > 0)
			    len--;
			found = true;
		    } else {
			buf[len + 1] = '\0';
			free_string(location->description);
			location->description = str_dup(buf);
			send_to_char("Room description is```8:``\n\r", ch);
			send_to_char(location->description ? location->description :
				"(None).\n\r", ch);
			return;
		    }
		}
	    }

	    buf[0] = '\0';
	    free_string(location->description);
	    location->description = str_dup(buf);
	    send_to_char("Room description cleared.\n\r", ch);
	    return;
	}

	if (argument[0] == '+') {
	    if (location->description != NULL)
		strcat(buf, location->description);

	    argument++;
	    while (is_space(*argument))
		argument++;
	}

	if (strlen(buf) + strlen(argument) >= 1024) {
	    send_to_char("Room description too long.\n\r", ch);
	    return;
	}

	strcat(buf, argument);
	strcat(buf, "\n\r");
	free_string(location->description);
	location->description = str_dup(buf);
    }

    send_to_char("Room description is```8:``\n\r", ch);
    send_to_char(location->description ? location->description : "(None).\n\r", ch);
    return;
}

void do_addalias(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MIL];
    int pos;

    argument = one_argument(argument, arg);
    if ((rch = get_char_world(ch, arg)) == NULL) {
	send_to_char("They are not here.\n\r", ch);
	return;
    }

    if (IS_NPC(rch))
	return;

    if (IS_SET(rch->act, PLR_LINKDEAD)) {
	send_to_char("They are linkdead, you cannot do that.\n\r", ch);
	return;
    }
    rch = rch->desc->original ? rch->desc->original : rch;

    argument = one_argument(argument, arg);
    smash_tilde(argument);

    if (arg[0] == '\0') {
	if (rch->pcdata->alias[0] == NULL) {
	    send_to_char("They have no aliases defined.\n\r", ch);
	    return;
	}

	send_to_char("Their current aliases are:\n\r", ch);
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	    if (rch->pcdata->alias[pos] == NULL
		    || rch->pcdata->alias_sub[pos] == NULL)
		break;

	    printf_to_char(ch, "    %s:  %s\n\r", rch->pcdata->alias[pos],
		    rch->pcdata->alias_sub[pos]);
	}
	return;
    }

    if (!str_prefix("una", arg) || !str_cmp("alias", arg)) {
	send_to_char("Sorry, that word is reserved.\n\r", ch);
	return;
    }

    /* It seems the [ character crashes us .. *boggle* =)  */
    if (!str_prefix("[", arg)) {
	send_to_char("The [ Character is reserved..\n\r", ch);
	return;
    }

    if (argument[0] == '\0') {
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	    if (rch->pcdata->alias[pos] == NULL
		    || rch->pcdata->alias_sub[pos] == NULL)
		break;

	    if (!str_cmp(arg, rch->pcdata->alias[pos])) {
		printf_to_char(ch, "%s aliases to '%s'.\n\r", rch->pcdata->alias[pos],
			rch->pcdata->alias_sub[pos]);
		return;
	    }
	}

	send_to_char("That alias is not defined.\n\r", ch);
	return;
    }

    if (!str_prefix(argument, "delete") || !str_prefix(argument, "prefix")) {
	send_to_char("That shall not be done!\n\r", ch);
	return;
    }

    if (add_alias(rch, arg, argument)) {
	printf_to_char(ch, "%s is now aliased to '%s'.\n\r", arg, argument);
    } else {
	send_to_char("Sorry, they have too many aliases.\n\r", ch);
	return;
    }
}


void fry_char(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    GAMEOBJECT *obj;
    GAMEOBJECT *obj_next;

    DENY_NPC(ch)

	send_to_char("`!Get `Plost `#l```#o`#s```#e`#r``.\n\r", ch);
    log_string("%s has quit for the last time.", ch->name);

    save_char_obj(ch);
    d = ch->desc;

    char_from_room(ch);
    char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
    extract_char(ch, true);

    for (obj = get_room_index(ROOM_VNUM_LIMBO)->contents; obj != NULL; obj = obj_next) {
	obj_next = obj->next_content;
	/* obj_from_room(obj); <- wtf
	 * this is right, isnt it?!?
	 * didnt originally do the extract
	 * obj_from_room is probably a memory
	 * leak
	 */
	extract_obj(obj);
    }

    if (d != NULL)
	close_socket(d);

    return;
}

void do_gobstopper(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;

    DENY_NPC(ch);

    if (is_help(argument)) {
	send_to_char("Syntax: gobstopper <player name>\n\r", ch);
	return;
    }

    if ((vch = get_char_world(ch, argument)) == NULL) {
	send_to_char("There is no character by that name.\n\r", ch);
	return;
    }

    if (set_char_hunger(ch, vch, "-151") && set_char_thirst(ch, vch, "-151") && set_char_feed(ch, vch, "-151")) {
	send_to_char("You have been `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`@e`1d`#!`P!``.\n\r", vch);
	printf_to_char(ch, "You have `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`@e`1d`` %s!!.\n\r", vch->name);
    } else {
	printf_to_char(ch, "You failed to `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`` %s.\n\r",
		(IS_NPC(vch)) ? vch->short_descr : vch->name);
    }
}


void do_mrelic(CHAR_DATA *ch, char *argument)
{
    GAMEOBJECT *obj;
    int i = 1500;

    if (argument[0] == '\0') {
	send_to_char("Make a relic item of what?\n\r", ch);
	return;
    }

    if ((obj = get_obj_carry(ch, argument)) == NULL) {
	send_to_char("You do not have that item.\n\r", ch);
	return;
    }

    if (IS_OBJ_STAT2(obj, ITEM2_RELIC)) {
	REMOVE_BIT(obj->extra2_flags, ITEM2_RELIC);
	act("$p is no longer a relic item.", ch, obj, NULL, TO_CHAR);
    } else {
	SET_BIT(obj->extra2_flags, ITEM2_RELIC);
	if (obj->xp_tolevel <= 0)
	    obj->xp_tolevel = i;
	act("$p is now a relic item.", ch, obj, NULL, TO_CHAR);
    }

    return;
}

/*************************************************
 *  World Peace - stops all fighting in the game
 *  Added by Monrick, 1/2008
 *************************************************/
void do_wpeace(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *fch;

    for (fch = char_list; fch != NULL; fch = fch->next) {
	if (fch->desc == NULL || fch->desc->connected != CON_PLAYING)
	    continue;

	if (fch->fighting != NULL) {
	    stop_fighting(fch, true);
	    printf_to_char(fch, "%s has declared world peace.\n\r",
		    capitalize(ch->name));
	}
    }

    wiznet("$N has declared world peace.", ch, NULL, WIZ_SECURE, 0, get_trust(ch));
    return;
}

/*********************************************************
 *  pLoad - original snippet by Gary McNickle (dharvest)
 *          modified for BadTrip by Monrick, 1/2008
 *********************************************************/
void do_ploa(CHAR_DATA *ch, char *argument)
{
    send_to_char("If you want to load a pfile, type out the whole command.\n\r", ch);
    return;
}

void do_pload(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA d;      /* need a blank descriptor for the pfile data */
    char vName[MIL];        /* victim Name */
    char buf[MSL];

    if (argument[0] == '\0') {
	send_to_char("Whose pfile do you want to load?\n\r", ch);
	return;
    }

    argument = one_argument(argument, vName);

    /* don't make duplicates */
    if (get_char_world(ch, vName) != NULL) {
	printf_to_char(ch, "%s is already connected to BadTrip.\n\r",
		capitalize(vName));
	return;
    }

    if (load_char_obj(&d, vName)) {         /* found */
	d.character->desc = NULL;       /* so we know it's not a real player */
	d.character->next = char_list;
	d.character->was_in_room = d.character->in_room;
	char_list = d.character;
	d.connected = CON_PLAYING;
	SET_BIT(d.character->act, PLR_LINKDEAD);
	reset_char(d.character);

	if (d.character->in_room != NULL) {
	    char_from_room(d.character);
	    char_to_room(d.character, ch->in_room);
	}

	act("$n pulls $N from the nether regions of space and time.\n\r$N stares straight ahead, in a daze.",
		ch, NULL, d.character, TO_ROOM);
	act("You pull $N from the nether regions of space and time.",
		ch, NULL, d.character, TO_CHAR);
	sprintf(buf, "$N pLoad -> %s", capitalize(d.character->name));
	wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));

	if (d.character->pet != NULL) {
	    char_from_room(d.character->pet);
	    char_to_room(d.character->pet, d.character->in_room);
	    act("$n's loyal pet, $N, appears.",
		    d.character, NULL, d.character->pet, TO_ROOM);
	}
    } else { /* not found */
	printf_to_char(ch, "Cannot find a pfile for %s.  Check spelling and try again.\n\r", capitalize(vName));
    }
    return;
}

/*********************************************************
 *  pUnload - original snippet by Gary McNickle (dharvest)
 *          modified for BadTrip by Monrick, 1/2008
 *********************************************************/
void do_punloa(CHAR_DATA *ch, char *argument)
{
    send_to_char("If you want to unload a pfile, type out the whole command.\n\r", ch);
    return;
}

void do_punload(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char vName[MIL];
    char buf[MSL];

    argument = one_argument(argument, vName);

    if ((victim = get_char_world(ch, vName)) == NULL) {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    if (victim->desc != NULL) { /* they are legitimately logged on */
	send_to_char("Can't pUnload them - there's a person attached!\n\r", ch);
	return;
    }

    if (victim->was_in_room != NULL) {
	char_from_room(victim);
	char_to_room(victim, victim->was_in_room);
	victim->was_in_room = NULL;
	if (victim->pet != NULL) {
	    char_from_room(victim->pet);
	    char_to_room(victim->pet, victim->in_room);
	}
    }

    REMOVE_BIT(victim->act, PLR_LINKDEAD);
    save_char_obj(victim);
    do_quit(victim, "");

    act("$n has sent $N back into the nether regions of space and time.",
	    ch, NULL, victim, TO_ROOM);
    sprintf(buf, "$N pUnLoad -> %s", capitalize(victim->name));
    wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
    return;
}

void sick_harvey_proctor(CHAR_DATA *ch, enum e_harvey_proctor_is mood, const char *message)
{
    char buf[MSL];
    const char *censor_name = "Harvey Proctor";

    switch (mood) {
	case hp_pissed_off:
	    snprintf(buf, MSL, "%s leaps out and canes your ass into submission shouting, '%s'\n\r", censor_name, message);
	    break;

	case hp_irritated:
	    snprintf(buf, MSL, "%s reaches out and slaps your legs saying, '%s'\n\r", censor_name, message);
	    break;

	case hp_agreeable:
	    snprintf(buf, MSL, "%s will allow this, but is ever watchful.\n\r", censor_name);
	    break;

	case hp_off_his_rocker:
	default:
	    snprintf(buf, MSL, "%s dick-slaps your face cackling, '%s'\n\r", censor_name, message);
	    break;
    }

    send_to_char(buf, ch);
}

void do_busy(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    if (IS_SET(ch->comm, COMM_BUSY)) {
	send_to_char("Busy flag removed. Type 'replay' to see tells.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_BUSY);
    } else {
	send_to_char("You are now marked as busy.\n\r", ch);
	SET_BIT(ch->comm, COMM_BUSY);
    }
}

void do_coding(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    if (IS_SET(ch->comm, COMM_CODING)) {
	send_to_char("Coding flag removed. Type 'replay' to see tells.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_CODING);
    } else {
	send_to_char("You are now marked as `@Coding``.\n\r", ch);
	SET_BIT(ch->comm, COMM_CODING);
    }
}

void do_building(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    if (IS_SET(ch->comm, COMM_BUILD)) {
	send_to_char("Building flag removed. Type 'replay' to see tells.\n\r", ch);
	REMOVE_BIT(ch->comm, COMM_BUILD);
    } else {
	send_to_char("You are now marked as `3Building``.\n\r", ch);
	SET_BIT(ch->comm, COMM_BUILD);
    }
}
