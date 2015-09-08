#include "merc.h"
#include "recycle.h"
#include "interp.h"
#include "telnet.h"

#include <stdio.h>
#include <string.h>



/** exports */
const unsigned char echo_off_str [] = { (unsigned char)IAC, (unsigned char)WILL, (unsigned char)TELOPT_ECHO, (unsigned char)'\0' };
const unsigned char echo_on_str  [] = { (unsigned char)IAC, (unsigned char)WONT, (unsigned char)TELOPT_ECHO, (unsigned char)'\0' };
const unsigned char go_ahead_str [] = { (unsigned char)IAC, (unsigned char)GA, (unsigned char)'\0' };


/** imports */
extern char *password_encrypt(const char *plain);
extern bool password_matches(char *existing, const char *plain);
extern int password_acceptance(const char *plain);
extern const char *password_accept_message(int code);


/** locals */
static char log_buf[MIL];


static void ansi_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    char *the_greeting;
    extern char *help_greeting;

    if (argument[0] == '\0') {
	close_socket(d);
	return;
    }

    if ((argument[0] == 'y') || (argument[0] == 'Y'))
	the_greeting = help_greeting;
    else
	the_greeting = help_greeting;

    if (the_greeting[0] == '.')
	write_to_buffer(d, the_greeting + 1, 0);
    else
	write_to_buffer(d, the_greeting, 0);

    d->connected = CON_GET_NAME;
}

static void name_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch;
    static char name_buf[MIL];
    static char buf[MIL];
    bool found;

    if (argument[0] == '\0') {
	close_socket(d);
	return;
    }

    (void)snprintf(name_buf, UMIN(strlen(argument), MIL), "%s", argument);
    name_buf[0] = UPPER(name_buf[0]);
    if (!check_parse_name(name_buf)) {
	write_to_buffer(d, "Illegal name.  Disconnecting.\n\r", 0);
	close_socket(d);
	return;
    }

    log_string("char name entered %s", name_buf);

    found = load_char_obj(d, name_buf);
    ch = d->character;

    if (IS_SET(ch->act, PLR_DENY)) {
	log_string("Denying access to %s@%s.", name_buf, d->host);
	write_to_buffer(d, "Boom Biddy Bye Bye.\n\r", 0);
	close_socket(d);
	return;
    }

    if (check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT)) {
	write_to_buffer(d, "Your site has been banned from this mud.   Boom Biddy Bye Bye.\n\r", 0);
	close_socket(d);
	return;
    }

    if (check_reconnect(d, name_buf, false)) {
	found = true;
    } else {
	if (globalSystemState.wizlock && !IS_IMMORTAL(ch)) {
	    write_to_buffer(d, "The game is wizlocked.\n\r", 0);
	    close_socket(d);
	    return;
	}
    }

    if (found) {
	snprintf(buf, 32, "Password: %s", (char *)echo_off_str);
	write_to_buffer(d, buf, 0);
	d->connected = CON_GET_OLD_PASSWORD;
    } else {
	/* New player */
	if (globalSystemState.newlock) {
	    write_to_buffer(d, "The game is newlocked.\n\r", 0);
	    close_socket(d);
	    return;
	}

	if (check_ban(d->host, BAN_NEWBIES)) {
	    write_to_buffer(d, "New players are not allowed from your site.\n\r", 0);
	    close_socket(d);
	    return;
	}

	sprintf(buf, "Did I get that right, %s(Y/N)? ", name_buf);
	write_to_buffer(d, buf, 0);
	d->connected = CON_CONFIRM_NEW_NAME;
    }
}


static void oldpassword_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;

    write_to_buffer(d, "\n\r", 2);

    if (!password_matches(ch->pcdata->pwd, argument)) {
	write_to_buffer(d, "Wrong password.  Boom Biddy Bye Bye.\n\r", 0);
	close_socket(d);
	return;
    }

    write_to_buffer(d, (char *)echo_on_str, 0);

    if (check_playing(d, ch->name) || check_reconnect(d, ch->name, true)) {
	return;
    }

    (void)snprintf(log_buf, MIL, "%s@%s has connected.", ch->name, d->host);
    log_string(log_buf);

    wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
    (void)snprintf(log_buf, MIL, "%s has entered the world.", ch->name);

    if (IS_SET(ch->act, PLR_LINKDEAD))
	REMOVE_BIT(ch->act, PLR_LINKDEAD);

    if (IS_SET(ch->comm, COMM_AFK))
	REMOVE_BIT(ch->comm, COMM_AFK);

    if (IS_IMMORTAL(ch)) {
	ch->invis_level = get_trust(ch);
	do_help(ch, "imotd");
	d->connected = CON_READ_IMOTD;
    } else {
	do_help(ch, "motd");
	d->connected = CON_READ_MOTD;
    }
}

/** RT code for breaking link */
static void breakconnect_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    switch (*argument) {
	case 'y':
	case 'Y': 
	    {
		DESCRIPTOR_DATA *d_old;
		DESCRIPTOR_DATA *dpending;
		CHAR_DATA *ch = d->character;

		dpending = descriptor_iterator_start(&descriptor_empty_filter);
		while ((d_old = dpending) != NULL) {
		    dpending = descriptor_iterator(d_old, &descriptor_empty_filter);

		    if (d_old == d || d_old->character == NULL)
			continue;

		    if (str_cmp(ch->name, d_old->original ? d_old->original->name : d_old->character->name))
			continue;

		    close_socket(d_old);
		}

		if (check_reconnect(d, ch->name, true)) {
		    return;
		}

		write_to_buffer(d, "Reconnect attempt failed.\n\rName: ", 0);
		if (d->character != NULL) {
		    free_char(d->character);
		    d->character = NULL;
		}

		d->connected = CON_GET_NAME;
		break;
	    }
	case 'n':
	case 'N':
	    write_to_buffer(d, "Name: ", 0);
	    if (d->character != NULL) {
		free_char(d->character);
		d->character = NULL;
	    }
	    d->connected = CON_GET_NAME;
	    break;

	default:
	    write_to_buffer(d, "Please type Y or N? Or I if you are an idiot.", 0);
	    break;
    }
}


static void confirmnew_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;
    static char buf[MIL];

    switch (*argument) {
	case 'y':
	case 'Y':
	    sprintf(buf, "New character.\n\rGive me a password for %s: %s", ch->name, (char *)echo_off_str);
	    write_to_buffer(d, buf, 0);
	    d->connected = CON_GET_NEW_PASSWORD;
	    break;

	case 'n':
	case 'N':
	    write_to_buffer(d, "Ok, what IS it, then? ", 0);
	    free_char(d->character);
	    d->character = NULL;
	    d->connected = CON_GET_NAME;
	    break;

	default:
	    write_to_buffer(d, "Please type Yes or No? ", 0);
	    break;
    }
}

static void newpassword_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;
    int passaccept;

    write_to_buffer(d, "\n\r", 2);
    passaccept = password_acceptance(argument);
    if (passaccept != 0) {
	write_to_buffer(d, password_accept_message(passaccept), 0);
	write_to_buffer(d, "\n\rPassword: ", 0);
	return;
    }
    free_string(ch->pcdata->pwd);
    ch->pcdata->pwd = password_encrypt(argument);
    write_to_buffer(d, "Please retype password: ", 0);
    d->connected = CON_CONFIRM_NEW_PASSWORD;
}

static void confirmnewpass_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;
    int race_idx;

    write_to_buffer(d, "\n\r", 2);

    if (!password_matches(ch->pcdata->pwd, argument)) {
	write_to_buffer(d, "Passwords don't match.\n\rRetype password: ", 0);
	d->connected = CON_GET_NEW_PASSWORD;
	return;
    }

    write_to_buffer(d, (char *)echo_on_str, 0);
    write_to_buffer(d, "The following races are available:\n\r  ", 0);
    for (race_idx = 1; race_table[race_idx].name != NULL; race_idx++) {
	if (!race_table[race_idx].pc_race)
	    break;
	write_to_buffer(d, race_table[race_idx].name, 0);
	write_to_buffer(d, " ", 1);
    }

    write_to_buffer(d, "\n\r", 0);
    write_to_buffer(d, "What is your race(help for more information)? ", 0);
    d->connected = CON_GET_NEW_RACE;
}

static void newrace_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;
    int idx;
    int race_idx;
    LEARNED *learned;
    static char arg[MIL];

    one_argument(argument, arg);

    if (!strcmp(arg, "help")) {
	argument = one_argument(argument, arg);
	if (argument[0] == '\0')
	    do_help(ch, "race");
	else
	    do_help(ch, argument);
	write_to_buffer(d, "What is your race(help for more information)? ", 0);
	return;
    }

    race_idx = race_lookup(argument);

    if (race_idx == 0 || !race_table[race_idx].pc_race) {
	write_to_buffer(d, "That is not a valid race.\n\r", 0);
	write_to_buffer(d, "The following races are available:\n\r  ", 0);
	for (race_idx = 1; race_table[race_idx].name != NULL; race_idx++) {
	    if (!race_table[race_idx].pc_race)
		break;
	    write_to_buffer(d, race_table[race_idx].name, 0);
	    write_to_buffer(d, " ", 1);
	}
	write_to_buffer(d, "\n\r", 0);
	write_to_buffer(d, "What is your race?(help for more information) ", 0);
	return;
    }

    ch->race = race_idx;
    /* initialize stats */
    for (idx = 0; idx < MAX_STATS; idx++)
	ch->perm_stat[idx] = pc_race_table[race_idx].stats[idx];

    ch->affected_by = ch->affected_by | race_table[race_idx].aff;
    ch->imm_flags = ch->imm_flags | race_table[race_idx].imm;
    ch->res_flags = ch->res_flags | race_table[race_idx].res;
    ch->vuln_flags = ch->vuln_flags | race_table[race_idx].vuln;
    ch->form = race_table[race_idx].form;
    ch->parts = race_table[race_idx].parts;
    ch->pcdata->points = pc_race_table[race_idx].points;
    ch->size = pc_race_table[race_idx].size;

    /* add skills */
    for (idx = 0; idx < 5; idx++) {
	if (pc_race_table[race_idx].skills[idx] == NULL)
	    break;

	learned = create_learned_skill(pc_race_table[race_idx].skills[idx], 75);
	if (learned != NULL) {
	    add_learned_skill(ch, learned);
	} else {
	    learned = create_learned_group(pc_race_table[race_idx].skills[idx]);
	    if (learned != NULL)
		add_learned_group(ch, learned);
	}
    }

    write_to_buffer(d, "\n\rWhat is your sex(M/F)? ", 0);
    d->connected = CON_GET_NEW_SEX;
}

static void newsex_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;

    switch (argument[0]) {
	case 'm':
	case 'M':
	    ch->sex = SEX_MALE;
	    ch->pcdata->true_sex = SEX_MALE;
	    break;
	case 'f':
	case 'F':
	    ch->sex = SEX_FEMALE;
	    ch->pcdata->true_sex = SEX_FEMALE;
	    break;
	default:
	    write_to_buffer(d, "\n\rThat's not a sex.\n\rWhat IS your sex? ", 0);
	    return;
    }

    write_to_buffer(d, "\n\r", 0);
    do_help(ch, "class");
    write_to_buffer(d, "Select a class: ", 0);
    d->connected = CON_GET_NEW_CLASS;
}

static void newclass_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    static char arg[MIL];
    CHAR_DATA *ch = d->character;
    LEARNED *learned;
    int class_idx;

    one_argument(argument, arg);

    if (!strcmp(arg, "help")) {
	argument = one_argument(argument, arg);
	if (argument[0] == '\0')
	    do_help(ch, "class");
	else
	    do_help(ch, argument);
	write_to_buffer(d, "What is your class? ", 0);
	return;
    }

    class_idx = class_lookup(argument);

    if (class_idx == -1) {
	write_to_buffer(d, "\n\rThat's not a class.\n\rWhat IS your class? ", 0);
	return;
    }

    if (!(class_table[class_idx].canCreate)) {
	write_to_buffer(d, "\n\rThat class is not available.\n\rWhat IS your class? ", 0);
	return;
    }

    ch->class = class_idx;

    (void)snprintf(log_buf, MIL, "%s@%s new player.", ch->name, d->host);
    log_string(log_buf);
    wiznet("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
    wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

    if ((learned = create_learned_group("rom basics")) != NULL)
	add_learned_group(ch, learned);
    if ((learned = create_learned_group(class_table[ch->class].base_group)) != NULL)
	add_learned_group(ch, learned);

    write_to_buffer(d, "\n\r", 2);

    list_group_costs(ch);
    write_to_buffer(d, "You already have the following skills:\n\r", 0);

    do_help(ch, "menu choice");
    d->connected = CON_GEN_GROUPS;
}

static void pickweapon_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;
    LEARNED *learned;
    int weapon_idx;

    write_to_buffer(d, "\n\r", 2);

    weapon_idx = weapon_lookup(argument);
    if (weapon_idx == -1 || (learned = get_learned(ch, weapon_table[weapon_idx].name)) == NULL) {
	static char buf[MSL];
	int idx;

	write_to_buffer(d, "That's not a valid selection. Choices are:\n\r", 0);
	buf[0] = '\0';
	for (idx = 0; weapon_table[idx].name != NULL; idx++) {
	    if ((learned = get_learned(ch, weapon_table[idx].name)) != NULL) {
		strcat(buf, weapon_table[idx].name);
		strcat(buf, " ");
	    }
	}

	strcat(buf, "\n\rYour choice? ");
	write_to_buffer(d, buf, 0);
	return;
    }

    if ((learned = create_learned_skill(weapon_table[weapon_idx].name, 40)) != NULL)
	add_learned_skill(ch, learned);
    write_to_buffer(d, "\n\r", 2);
    do_help(ch, "motd");
    d->connected = CON_READ_MOTD;
}

static void gengroups_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;

    send_to_char("\n\r", ch);

    if (!str_cmp(argument, "done")) {
	static char buf[MSL];
	bool found;
	int idx;

	printf_to_char(ch, "Creation points: %d\n\r", ch->pcdata->points);
	printf_to_char(ch, "Experience per level: %d\n\r", exp_per_level(ch, ch->pcdata->points));

	buf[0] = '\0';
	found = false;
	for (idx = 0; weapon_table[idx].name != NULL; idx++) {
	    if (get_learned(ch, weapon_table[idx].name) != NULL) {
		if (!found) {
		    write_to_buffer(d, "\n\r", 2);
		    write_to_buffer(d, "Please pick a weapon from the following choices:\n\r", 0);
		    found = true;
		}

		strcat(buf, weapon_table[idx].name);
		strcat(buf, " ");
	    }
	}

	if (found) {
	    strcat(buf, "\n\rYour choice? ");
	    write_to_buffer(d, buf, 0);
	    d->connected = CON_PICK_WEAPON;
	    return;
	} else {
	    write_to_buffer(d, "You must choose at least one weapon skill.\n\r", 0);
	}
    }

    if (!parse_gen_groups(ch, argument))
	send_to_char("Choices are: list,learned,premise,add,drop,inform,help, and done.\n\r", ch);

    do_help(ch, "menu choice");
}

static void readimotd_answered(DESCRIPTOR_DATA *d, /*@unused@*/const char *argument)
{
    CHAR_DATA *ch = d->character;

    write_to_buffer(d, "\n\r", 2);
    do_help(ch, "motd");
    d->connected = CON_READ_MOTD;
}

static void readmotd_answered(DESCRIPTOR_DATA *d, const char *argument)
{
    CHAR_DATA *ch = d->character;

    if (ch->played < 0)
	ch->played = 0;

    write_to_buffer(d, "\n\rMake sure you know the rules!  Refresh your memory regularly!\n\r\n\r", 0);
    ch->next = char_list;
    char_list = ch;

    d->connected = CON_PLAYING;
    reset_char(ch);

    if (ch->level == 0) {
	int rnd;

	ch->perm_stat[class_table[ch->class].attr_prime] += 3;

	ch->level = 1;
	ch->exp = exp_per_level(ch, ch->pcdata->points);
	ch->hit = ch->max_hit;
	ch->mana = ch->max_mana;
	ch->move = ch->max_move;

	ch->pcdata->train = 3;
	ch->pcdata->practice = 5;
	ch->gold = 10000;

	set_title(ch, "is `&new`` to this realm.");

	obj_to_char(create_object(objectprototype_getbyvnum(OBJ_VNUM_MAP), 0), ch);

	char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
	rnd = number_range(0, 9);

	switch (rnd) {
	    case (0):
		do_deathcry(ch, "Oh dear. Oswald will be rather perturbed. I am dead.");
		break;
	    case (1):
		do_deathcry(ch, "`2*`@gleep`2*`! I'm DEAD.");
		break;
	    case (2):
		do_deathcry(ch, "Oh NO! Not again!!");
		break;
	    case (3):
		do_deathcry(ch, "The weasels! THE WEASELS!!! AAAGH!!");
		break;
	    case (4):
		do_deathcry(ch, "I see a light.");
		break;
	    default:
		do_deathcry(ch, "AAAAAAAAAAAAAAAAAARGH! I'm DEAD!!");
	}

	send_to_char("\n\r", ch);
	do_help(ch, "NEWBIE INFO");
	send_to_char("\n\r", ch);
    } else if (ch->in_room != NULL) {
	char_to_room(ch, ch->in_room);
    } else if (IS_IMMORTAL(ch)) {
	char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
    } else {
	char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
    }

    if (ch->pcdata->killer_time != 0)
	ch->pcdata->killer_time = time(NULL);

    if (ch->pcdata->thief_time != 0)
	ch->pcdata->thief_time = time(NULL);

    act("$n has entered the game.", ch, NULL, NULL, TO_ROOM);

    do_look(ch, "auto");

    wiznet("$N has left real life behind.", ch, NULL, WIZ_LOGINS, WIZ_SITES, get_trust(ch));

    if (ch->pet != NULL) {
	char_to_room(ch->pet, ch->in_room);
	act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM);
    }

    if (ch->level > MAX_LEVEL)
	ch->trust = 1;

    do_unread(ch, "");
}

void nanny(DESCRIPTOR_DATA *d, const char *argument)
{
    while (is_space(*argument))
	argument++;

    switch (d->connected) {
	default:
	    log_bug("Nanny: bad d->connected %d.", d->connected);
	    close_socket(d);
	    return;

	case CON_GET_ANSI:
	    ansi_answered(d, argument);
	    return;

	case CON_GET_NAME:
	    name_answered(d, argument);
	    return;

	case CON_GET_OLD_PASSWORD:
	    oldpassword_answered(d, argument);
	    return;

	case CON_BREAK_CONNECT:
	    breakconnect_answered(d, argument);
	    return;

	case CON_CONFIRM_NEW_NAME:
	    confirmnew_answered(d, argument);
	    return;

	case CON_GET_NEW_PASSWORD:
	    newpassword_answered(d, argument);
	    return;

	case CON_CONFIRM_NEW_PASSWORD:
	    confirmnewpass_answered(d, argument);
	    return;

	case CON_GET_NEW_RACE:
	    newrace_answered(d, argument);
	    return;

	case CON_GET_NEW_SEX:
	    newsex_answered(d, argument);
	    return;

	case CON_GET_NEW_CLASS:
	    newclass_answered(d, argument);
	    return;

	case CON_PICK_WEAPON:
	    pickweapon_answered(d, argument);
	    return;

	case CON_GEN_GROUPS:
	    gengroups_answered(d, argument);
	    return;

	case CON_READ_IMOTD:
	    readimotd_answered(d, argument);
	    return;

	case CON_READ_MOTD:
	    readmotd_answered(d, argument);
	    return;
    }
}

/***************************************************************************
 *    check a name to see if it is acceptable
 ***************************************************************************/
bool check_parse_name(const char *name)
{
    extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
    MOB_INDEX_DATA *pMobIndex;
    int hash;
    const char *pc;

    /*
     * Reserved words.
     */
    if (is_name(name, "all auto immortal imp self someone something the you your none socket who anonymous fuck")) {
	return false;
    }

    if (!str_infix(name, "fuck")
	    || !str_infix(name, "shit")
	    || !str_infix(name, "frog")
	    || !str_infix(name, "cum")
	    || !str_infix(name, "lick")
	    || !str_infix(name, "george")
	    || !str_infix(name, "kerry")
	    || !str_infix(name, "jim")
	    || !str_infix(name, "twat")
	    || !str_infix(name, "bert")
	    || !str_infix(name, "death")
	    || !str_infix(name, "kill")
	    || !str_infix(name, "satan")
	    || !str_infix(name, "pink")
	    || !str_infix(name, "blade")
	    || !str_infix(name, "suck")
	    || !str_infix(name, "ihate")
	    || !str_infix(name, "pussy")
	    || !str_infix(name, "uoykcuf")
	    || !str_infix(name, "kcuf")
	    || !str_infix(name, "monger")
	    || !str_infix(name, "tard")
	    || !str_infix(name, "idiot")
	    || !str_infix(name, "cunt")
	    || !str_infix(name, "crap")
	    || !str_infix(name, "penis")
	    || !str_infix(name, "asshole")
	    || !str_infix(name, "vagina")
	    || !str_cmp(name, "ass")) {
		return false;
	    }


    /*
     * Length restrictions.
     */
    if (strlen(name) < 2 || strlen(name) > MAX_NAME_LENGTH) {
	return false;
    }

    /*
     * Alphanumerics only.
     */
    for (pc = name; *pc != '\0'; pc++) {
	if (!is_alpha(*pc))
	    return false;
    }

    /*
     * Edwin's been here too. JR -- 10/15/00
     *
     * Check names of people playing. Yes, this is necessary for multiple
     * newbies with the same name (thanks Saro)
     */
    {
	int count = 0;
	DESCRIPTOR_DATA *d, *dpending;

	dpending = descriptor_iterator_start(&descriptor_empty_filter);
	while ((d = dpending) != NULL) {
	    dpending = descriptor_iterator(d, &descriptor_empty_filter);

	    if (d->connected != CON_PLAYING 
		    && d->character != NULL
		    && d->character->name
		    && d->character->name[0] 
		    && !str_cmp(d->character->name, name)) {
		count++;
		close_socket(d);
	    }
	}
	if (count) {
	    (void)snprintf(log_buf, MIL, "Double newbie alert (%s)", name);
	    wiznet(log_buf, NULL, NULL, WIZ_LOGINS, 0, 0);
	    return false;
	}
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    for (hash = 0; hash < MAX_KEY_HASH; hash++) {
	for (pMobIndex = mob_index_hash[hash]; pMobIndex != NULL; pMobIndex = pMobIndex->next) {
	    if (is_name(name, pMobIndex->player_name)) {
		return false;
	    }
	}
    }

    return true;
}



/**
 * see if there is an existing player to reconnect
 */
bool check_reconnect(DESCRIPTOR_DATA *d, const char *name, bool reconnect)
{
    CHAR_DATA *ch;

    for (ch = char_list; ch != NULL; ch = ch->next) {
	if (!IS_NPC(ch) && (!reconnect || ch->desc == NULL) && !str_cmp(d->character->name, ch->name)) {
	    if (reconnect == false) {
		free_string(d->character->pcdata->pwd);
		d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
	    } else {
		GAMEOBJECT *obj;

		free_char(d->character);
		d->character = ch;
		ch->desc = d;
		ch->timer = 0;

		send_to_char("Reconnecting. Type replay to see missed tells.\n\r", ch);
		do_unread(ch, NULL);

		act("$n has reconnected.", ch, NULL, NULL, TO_ROOM);

		if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
			&& obj->item_type == ITEM_LIGHT
			&& obj->value[2] != 0
			&& ch->in_room->light > 0)
		    --ch->in_room->light;

		(void)snprintf(log_buf, MIL, "%s@%s reconnected.", ch->name, d->host);
		log_string(log_buf);
		wiznet("$N groks the fullness of $S link.", ch, NULL, WIZ_LINKS, 0, 0);

		REMOVE_BIT(ch->act, PLR_LINKDEAD);
		d->connected = CON_PLAYING;
	    }
	    return true;
	}
    }

    return false;
}



/**
 * determine whether a character is already playing
 */
bool check_playing(DESCRIPTOR_DATA *d, const char *name)
{
    DESCRIPTOR_DATA *dpending;
    DESCRIPTOR_DATA *dold;

    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((dold = dpending) != NULL) {
	dpending = descriptor_iterator(dold, &descriptor_empty_filter);

	if (dold != d
		&& dold->character != NULL
		&& dold->connected != CON_GET_NAME
		&& dold->connected != CON_GET_OLD_PASSWORD
		&& !str_cmp(name, (dold->original) ? dold->original->name : dold->character->name)) {
	    write_to_buffer(d, "That character is already playing.\n\r", 0);
	    write_to_buffer(d, "Do you wish to connect anyway(Y/N)?", 0);
	    d->connected = CON_BREAK_CONNECT;
	    return true;
	}
    }

    return false;
}


/***************************************************************************
 *    stop_idling
 *
 *    bring a character back from the void if they have
 *    poofed off to limbo
 ***************************************************************************/
void stop_idling(CHAR_DATA *ch)
{
    if (ch == NULL
	    || ch->desc == NULL
	    || ch->desc->connected != CON_PLAYING
	    || ch->was_in_room == NULL
	    || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
	return;

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = NULL;
    act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);

    return;
}
