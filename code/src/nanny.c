/**************************************************************************
 *  Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "recycle.h"
#include "interp.h"
#include "telnet.h"


extern bool is_space(const char test);
extern bool is_alpha(const char test);
extern bool is_upper(const char test);


const unsigned char echo_off_str [] = { (unsigned char)IAC, (unsigned char)WILL, (unsigned char)TELOPT_ECHO, (unsigned char)'\0' };
const unsigned char echo_on_str  [] = { (unsigned char)IAC, (unsigned char)WONT, (unsigned char)TELOPT_ECHO, (unsigned char)'\0' };
const unsigned char go_ahead_str [] = { (unsigned char)IAC, (unsigned char)GA, (unsigned char)'\0' };


void nanny(DESCRIPTOR_DATA *d, char *argument)
{
	DESCRIPTOR_DATA *d_old;
	DESCRIPTOR_DATA *d_next;
	CHAR_DATA *ch;
	LEARNED *learned;
	char buf[MSL];
	char arg[MIL];
	char *pwdnew;
	char *p;
	int class_idx;
	int race_idx;
	int weapon_idx;
	int idx;
	int rnd;
	bool found;
	extern char *help_greeting;
	char *the_greeting;

	while (is_space(*argument))
		argument++;

	ch = d->character;

	switch (d->connected) {
	default:
		bug("Nanny: bad d->connected %d.", d->connected);
		close_socket(d);
		return;

	case CON_GET_ANSI:
		/* Kyndig: they can only input y or n - else close them */
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
		break;

	case CON_GET_NAME:
		if (argument[0] == '\0') {
			close_socket(d);
			return;
		}

		argument[0] = UPPER(argument[0]);
		if (!check_parse_name(argument)) {
			write_to_buffer(d, "Illegal name.  Disconnecting.\n\r", 0);
			close_socket(d);
			return;
		}

		sprintf(log_buf, "char name enterred %s", argument);
		log_string(log_buf);

		found = load_char_obj(d, argument);
		ch = d->character;

		if (IS_SET(ch->act, PLR_DENY)) {
			sprintf(log_buf, "Denying access to %s@%s.", argument, d->host);
			log_string(log_buf);
			write_to_buffer(d, "Boom Biddy Bye Bye.\n\r", 0);
			close_socket(d);
			return;
		}

		if (check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT)) {
			write_to_buffer(d, "Your site has been banned from this mud.   Boom Biddy Bye Bye.\n\r", 0);
			close_socket(d);
			return;
		}

		if (check_reconnect(d, argument, FALSE)) {
			found = TRUE;
		} else {
			if (wizlock && !IS_IMMORTAL(ch)) {
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
			if (newlock) {
				write_to_buffer(d, "The game is newlocked.\n\r", 0);
				close_socket(d);
				return;
			}

			if (check_ban(d->host, BAN_NEWBIES)) {
				write_to_buffer(d, "New players are not allowed from your site.\n\r", 0);
				close_socket(d);
				return;
			}

			sprintf(buf, "Did I get that right, %s(Y/N)? ", argument);
			write_to_buffer(d, buf, 0);
			d->connected = CON_CONFIRM_NEW_NAME;
		}
		break;

	case CON_GET_OLD_PASSWORD:
		write_to_buffer(d, "\n\r", 2);

		if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
			write_to_buffer(d, "Wrong password.  Boom Biddy Bye Bye.\n\r", 0);
			close_socket(d);
			return;
		}

		write_to_buffer(d, (char *)echo_on_str, 0);

		if (check_playing(d, ch->name) || check_reconnect(d, ch->name, TRUE)) {
			return;
        }

		sprintf(log_buf, "%s@%s has connected.", ch->name, d->host);
		log_string(log_buf);

		wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));
		sprintf(log_buf, "%s has entered the world.", ch->name);

		if (IS_SET(ch->act, PLR_LINKDEAD))
			REMOVE_BIT(ch->act, PLR_LINKDEAD);

		if (IS_SET(ch->comm2, COMM2_AFK))
			REMOVE_BIT(ch->comm2, COMM2_AFK);

		if (IS_SET(ch->act, PLR_BATTLE))
			REMOVE_BIT(ch->act, PLR_BATTLE);

		if (IS_IMMORTAL(ch)) {
			ch->invis_level = get_trust(ch);
			do_help(ch, "imotd");
			d->connected = CON_READ_IMOTD;
			REMOVE_BIT(ch->comm2, COMM2_OOC);
		} else {
			do_help(ch, "motd");
			d->connected = CON_READ_MOTD;
			REMOVE_BIT(ch->comm2, COMM2_RP);
			SET_BIT(ch->comm2, COMM2_OOC);
		}
		break;

	/* RT code for breaking link */
	case CON_BREAK_CONNECT:
		switch (*argument) {
		case 'y':
		case 'Y':
			for (d_old = descriptor_list; d_old != NULL; d_old = d_next) {
				d_next = d_old->next;

				if (d_old == d || d_old->character == NULL)
					continue;

				if (str_cmp(ch->name, d_old->original ? d_old->original->name : d_old->character->name))
					continue;

				close_socket(d_old);
			}

			if (check_reconnect(d, ch->name, TRUE))
				return;

			write_to_buffer(d, "Reconnect attempt failed.\n\rName: ", 0);
			if (d->character != NULL) {
				free_char(d->character);
				d->character = NULL;
			}

			d->connected = CON_GET_NAME;
			break;

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
		break;

	case CON_CONFIRM_NEW_NAME:
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
		break;

	case CON_GET_NEW_PASSWORD:
		write_to_buffer(d, "\n\r", 2);

		if (strlen(argument) < 5) {
			write_to_buffer(d, "Password must be at least five characters long.\n\rPassword: ", 0);
			return;
		}

		pwdnew = crypt(argument, ch->name);
		for (p = pwdnew; *p != '\0'; p++) {
			if (*p == '~') {
				write_to_buffer(d, "New password not acceptable, try again.\n\rPassword: ", 0);
				return;
			}
		}

		free_string(ch->pcdata->pwd);
		ch->pcdata->pwd = str_dup(pwdnew);
		write_to_buffer(d, "Please retype password: ", 0);
		d->connected = CON_CONFIRM_NEW_PASSWORD;
		break;

	case CON_CONFIRM_NEW_PASSWORD:
		write_to_buffer(d, "\n\r", 2);

		if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
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
		break;

	case CON_GET_NEW_RACE:
		one_argument(argument, arg);

		if (!strcmp(arg, "help")) {
			argument = one_argument(argument, arg);
			if (argument[0] == '\0')
				do_help(ch, "race");
			else
				do_help(ch, argument);
			write_to_buffer(d, "What is your race(help for more information)? ", 0);
			break;
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
			break;
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
		break;

	case CON_GET_NEW_SEX:
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
		break;

	case CON_GET_NEW_CLASS:
		one_argument(argument, arg);

		if (!strcmp(arg, "help")) {
			argument = one_argument(argument, arg);
			if (argument[0] == '\0')
				do_help(ch, "class");
			else
				do_help(ch, argument);
			write_to_buffer(d, "What is your class? ", 0);
			break;
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
		ch->alignment = class_table[class_idx].dAlign; /* New!!! Added by Monrick */

		sprintf(log_buf, "%s@%s new player.", ch->name, d->host);
		log_string(log_buf);
		wiznet("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
		wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

		write_to_buffer(d, "\n\r", 2);
		write_to_buffer(d, "Will this character be a player killer?\n\r", 0);
		write_to_buffer(d, "If you are a pkiller, you can do all sorts of neat stuff like kill other\n\r", 0);
		write_to_buffer(d, "pkillers and be killed in turn.  Of course, if you decide not to be a pkiller,\n\r", 0);
		write_to_buffer(d, "you are safe from pkillers.\n\r\n\r", 0);
		write_to_buffer(d, "If you want to join a guild or get into any real roleplaying, you'll want to\n\r", 0);
		write_to_buffer(d, "be a pkiller\n\r\n\r", 0);
		write_to_buffer(d, "You can choose to go pk later in the game. This is suggested for players new to the mud.\n\r\n\r", 0);
		write_to_buffer(d, "Will this character be a pkiller?(Y/N)", 0);
		d->connected = CON_PKILL_CHOICE;
		break;

	case CON_PKILL_CHOICE:
		write_to_buffer(d, "\n\r", 2);
		switch (argument[0]) {
		case 'Y':
		case 'y':
			write_to_buffer(d, "MOSH on!\n\r", 0);
			write_to_buffer(d, "a PKiLLER you are\n\r", 0);
			write_to_buffer(d, "WARNING - NO ANSWER IS NOT IMPLEMENTED.", 0);
			break;
		case 'N':
		case 'n':
			write_to_buffer(d, "Alright, you will be safe from the wrath of the pkiller\n\r", 0);
			write_to_buffer(d, "WARNING - NO ANSWER IS NOT IMPLEMENTED.", 0);
			break;
		default:
			write_to_buffer(d, "This is a yes or no question chumpskie, lets try it again\n\r", 0);
			write_to_buffer(d, "Will this character be a pkiller?(Y/N)", 0);
			return;
		}

		if ((learned = create_learned_group("rom basics")) != NULL)
			add_learned_group(ch, learned);
		if ((learned = create_learned_group(class_table[ch->class].base_group)) != NULL)
			add_learned_group(ch, learned);

		write_to_buffer(d, "\n\r", 2);

		list_group_costs(ch);
		write_to_buffer(d, "You already have the following skills:\n\r", 0);

		do_help(ch, "menu choice");
		d->connected = CON_GEN_GROUPS;
		break;

	case CON_PICK_WEAPON:
		write_to_buffer(d, "\n\r", 2);

		weapon_idx = weapon_lookup(argument);
		if (weapon_idx == -1 || (learned = get_learned(ch, weapon_table[weapon_idx].name)) == NULL) {
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
		break;

	case CON_GEN_GROUPS:
	{
		bool found;

		send_to_char("\n\r", ch);

		if (!str_cmp(argument, "done")) {
			sprintf(buf, "Creation points: %d\n\r", ch->pcdata->points);
			send_to_char(buf, ch);
			sprintf(buf, "Experience per level: %d\n\r", exp_per_level(ch, ch->pcdata->points));

			send_to_char(buf, ch);
			buf[0] = '\0';
			found = FALSE;
			for (idx = 0; weapon_table[idx].name != NULL; idx++) {
				if ((learned = get_learned(ch, weapon_table[idx].name)) != NULL) {
					if (!found) {
						write_to_buffer(d, "\n\r", 2);
						write_to_buffer(d, "Please pick a weapon from the following choices:\n\r", 0);
						found = TRUE;
					}

					strcat(buf, weapon_table[idx].name);
					strcat(buf, " ");
				}
			}

			if (found) {
				strcat(buf, "\n\rYour choice? ");
				write_to_buffer(d, buf, 0);
				d->connected = CON_PICK_WEAPON;
				break;
			} else {
				write_to_buffer(d, "You must choose at least one weapon skill.\n\r", 0);
			}
		}

		if (!parse_gen_groups(ch, argument))
			send_to_char("Choices are: list,learned,premise,add,drop,inform,help, and done.\n\r", ch);

		do_help(ch, "menu choice");
		break;
	}
	case CON_READ_IMOTD:
		write_to_buffer(d, "\n\r", 2);
		do_help(ch, "motd");
		d->connected = CON_READ_MOTD;
		break;

	case CON_READ_MOTD:
		if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0') {
			write_to_buffer(d, "Warning! Null password!\n\r", 0);
			write_to_buffer(d, "Please report old password with bug.\n\r", 0);
			write_to_buffer(d, "Type 'password null <new password>' to fix.\n\r", 0);
		}

		if (ch->played < 0)
			ch->played = 0;

		write_to_buffer(d, "\n\rMake sure you know the rules!  Refresh your memory regularly!\n\r\n\r", 0);
		ch->next = char_list;
		char_list = ch;

		d->connected = CON_PLAYING;
		ch->ticks_since_last_fight = 10;
		reset_char(ch);

		if (ch->level == 0) {
			ch->perm_stat[class_table[ch->class].attr_prime] += 3;

			ch->level = 1;
			ch->exp = exp_per_level(ch, ch->pcdata->points);
			ch->hit = ch->max_hit;
			ch->mana = ch->max_mana;
			ch->move = ch->max_move;

			ch->pcdata->train = 3;
			ch->pcdata->practice = 5;
			ch->gold = 10000;

			sprintf(buf, "is `&new`` to this realm.");
			set_title(ch, buf);

			do_outfit(ch, "");
			obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP), 0), ch);

			char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
			rnd = number_range(0, 9);

			do_ooc(ch, "I am new to this realm, and I could use some help (AUTOMSG)");

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
		} else if (ch->pcdata->jail_time != 0) {
			char_to_room(ch, get_room_index(20925));
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

		if (IS_SET(ch->act, PLR_BATTLE)) {
			REMOVE_BIT(ch->act, PLR_BATTLE);
        }

		if (!IS_SET(ch->comm2, COMM2_ENABLE)) {
			send_to_char("Player File Tokens Enabled!!! You will only see this message once in your entire character life.\n\r", ch);

			SET_BIT(ch->comm2, COMM2_ENABLE);

			ch->pcdata->tenten = 0;
			ch->pcdata->twohundred = 0;
			ch->pcdata->armorclass = 0;
			ch->pcdata->restring = 0;
			ch->pcdata->weaponflag = 0;
			ch->pcdata->fireproof = 0;
			ch->pcdata->damnoun = 0;
			ch->pcdata->immtrivia = 0;
			ch->pcdata->immhidden = 0;
			ch->pcdata->immwild = 0;
			ch->pcdata->imp = 0;
			ch->pcdata->skillset = 0;
			ch->pcdata->rp = 0;
		}


		if (ch->level > 300 && ch->vernew <= 3) {
			if (ch->level <= 600) {
				if (IS_IMMORTAL(ch)) {
				} else {
					ch->level = 300;
					ch->vernew = 4;
					send_to_char("\n\r`^Reverting level back to 300.``\n\r `#O`&M`#G your level `&300`# now`3!`#?`3!``\n\r", ch);
				}
			}
		}

		if (ch->vernew <= 4) {
			if (!IS_IMMORTAL(ch)) {
				LEARNED *learned;

				for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next)
					if (learned->percent > 75)
						learned->percent = 75;
			}
			ch->vernew = 5;
		}

		if (ch->vernew == 5) {
			if (!IS_IMMORTAL(ch)) {
				LEARNED *learned;

				for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
					if (learned->percent > 75)
						learned->percent = UMIN(100, 85 + (learned->percent - 75));
					if (learned->percent == 75)
						learned->percent = 85;
				}
			}
			ch->vernew = 6;
		}

		wiznet("$N has left real life behind.", ch, NULL, WIZ_LOGINS, WIZ_SITES, get_trust(ch));

		if (ch->pet != NULL) {
			char_to_room(ch->pet, ch->in_room);
			act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM);
		}

		if (ch->level > MAX_LEVEL)
			ch->trust = 1;

		do_unread(ch, "");
		break;
	}

	return;
}

/***************************************************************************
*	check_parse_name
*
*	check a name to see if it is acceptable
***************************************************************************/
bool check_parse_name(char *name)
{
	extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
	MOB_INDEX_DATA *pMobIndex;
	int hash;
	char *pc;

	/*
	 * Reserved words.
	 */
	if (is_name(name, "all auto immortal imp self someone something the you your none socket who anonymous fuck")) {
		return FALSE;
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
		return FALSE;
    }


    /*
     * Length restrictions.
     */
	if (strlen(name) < 2 || strlen(name) > MAX_NAME_LENGTH) {
		return FALSE;
    }

    /*
     * Alphanumerics only.
     */
	for (pc = name; *pc != '\0'; pc++) {
		if (!is_alpha(*pc))
			return FALSE;
	}

	/*
	 * Edwin's been here too. JR -- 10/15/00
	 *
	 * Check names of people playing. Yes, this is necessary for multiple
	 * newbies with the same name (thanks Saro)
	 */
	if (descriptor_list) {
		int count = 0;
		DESCRIPTOR_DATA *d, *dnext;

		for (d = descriptor_list; d != NULL; d = dnext) {
			dnext = d->next;
			if (d->connected != CON_PLAYING && d->character && d->character->name
			    && d->character->name[0] && !str_cmp(d->character->name, name)) {
				count++;
				close_socket(d);
			}
		}
		if (count) {
			sprintf(log_buf, "Double newbie alert (%s)", name);
			wiznet(log_buf, NULL, NULL, WIZ_LOGINS, 0, 0);

			return FALSE;
		}
	}

    /*
     * Prevent players from naming themselves after mobs.
     */
	for (hash = 0; hash < MAX_KEY_HASH; hash++) {
		for (pMobIndex = mob_index_hash[hash]; pMobIndex != NULL; pMobIndex = pMobIndex->next) {
			if (is_name(name, pMobIndex->player_name)) {
				return FALSE;
            }
        }
	}

	return TRUE;
}



/***************************************************************************
*	check_reconnect
*
*	see if there is an existing player to reconnect
***************************************************************************/
bool check_reconnect(DESCRIPTOR_DATA *d, char *name, bool reconnect)
{
	CHAR_DATA *ch;

	for (ch = char_list; ch != NULL; ch = ch->next) {
		if (!IS_NPC(ch) && (!reconnect || ch->desc == NULL) && !str_cmp(d->character->name, ch->name)) {
			if (reconnect == FALSE) {
				free_string(d->character->pcdata->pwd);
				d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
			} else {
				OBJ_DATA *obj;

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

				sprintf(log_buf, "%s@%s reconnected.", ch->name, d->host);
				log_string(log_buf);
				wiznet("$N groks the fullness of $S link.", ch, NULL, WIZ_LINKS, 0, 0);

				REMOVE_BIT(ch->act, PLR_LINKDEAD);
				d->connected = CON_PLAYING;
			}
			return TRUE;
		}
	}

	return FALSE;
}



/***************************************************************************
*	check_playing
*
*	check to see if a character is already playing
***************************************************************************/
bool check_playing(DESCRIPTOR_DATA *d, char *name)
{
	DESCRIPTOR_DATA *dold;

	for (dold = descriptor_list; dold; dold = dold->next) {
		if (dold != d
		    && dold->character != NULL
		    && dold->connected != CON_GET_NAME
		    && dold->connected != CON_GET_OLD_PASSWORD
		    && !str_cmp(name, (dold->original) ? dold->original->name : dold->character->name)) {
			write_to_buffer(d, "That character is already playing.\n\r", 0);
			write_to_buffer(d, "Do you wish to connect anyway(Y/N)?", 0);
			d->connected = CON_BREAK_CONNECT;
			return TRUE;
		}
	}

	return FALSE;
}


/***************************************************************************
*	check_afk
*
*	check to see if a character is active - if they are
*	remove the AFK bit
***************************************************************************/
void check_afk(CHAR_DATA *ch)
{
	if (ch == NULL
	    || ch->desc == NULL
	    || ch->desc->connected != CON_PLAYING)
		return;

	if (IS_SET(ch->comm2, COMM2_AFK))
		REMOVE_BIT(ch->comm2, COMM2_AFK);

	return;
}

/***************************************************************************
*	stop_idling
*
*	bring a character back from the void if they have
*	poofed off to limbo
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
