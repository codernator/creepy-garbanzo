/**
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include "merc.h"
#include "recycle.h"
#include "remote.h"
#include <time.h>
#if !defined(S_SPLINT_S)
#include <ctype.h> /** isascii, isprint */
#endif
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(STDOUT_FILENO)
#define STDOUT_FILENO 1
#endif


/** exports */
/** TODO - these are only outside of main scope because the copyover routine needs them. */
int listen_port;
int listen_control;

static WEATHER_DATA weather = { 
    .mmhg = 0,
    .change = 0,
    .sky = 0,
    .sunlight = 0
};

static TIME_INFO_DATA gametime = {
    .hour = 0,
    .day = 0,
    .month = 0,
    .year = 0
};

GAME_STATE globalGameState = {
    .weather = &weather,
    .gametime = &gametime
};

void auto_shutdown();
void game_loop(int port, int control);


/** imports */
/** OS-dependent declarations. */
#ifdef S_SPLINT_S
typedef unsigned int fd_set;
#endif
extern int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);


/** Game declarations. */
extern char *color_table[];
extern bool is_space(const char test);
extern bool run_olc_editor(DESCRIPTOR_DATA * d);
extern char *olc_ed_name(CHAR_DATA * ch);
extern char *olc_ed_vnum(CHAR_DATA * ch);
extern void string_add(CHAR_DATA * ch, const char *argument);
extern char *string_replace(char *orig, char *old, char *new);

static bool process_output(DESCRIPTOR_DATA * d, bool fPrompt);
static void read_from_buffer(DESCRIPTOR_DATA * d);
static void check_afk(CHAR_DATA * ch);
static void bust_a_prompt(CHAR_DATA * ch);

static void on_new_connection(int descriptor, int ipaddress, const char *hostname);
static void process_all_input();
static void process_all_output();

static const double loop_time_slice = ((double)CLOCKS_PER_SEC/PULSE_PER_SECOND);
static const struct descriptor_iterator_filter allfilter = { .all = true };
static void synchronize(clock_t last_clock);

void game_loop(int port, int control)
{

    listen_port = port;
    listen_control = control;

    /** Main loop */
    while (!globalSystemState.merc_down) {
	clock_t last_clock;

	last_clock = clock();
	(void)time(&globalSystemState.current_time);

	remote_poll(control, on_new_connection);
	process_all_input();
	update_handler();
	process_all_output();
	synchronize(last_clock);
    }
}

void synchronize(clock_t last_clock)
{
    clock_t current_clock;
    static struct timeval stall_time;
    long usecDelta;

    current_clock = clock();
    usecDelta = (long)(1000000 * (loop_time_slice - (double)(current_clock - last_clock)) / CLOCKS_PER_SEC);

    if (usecDelta > 0) {
	stall_time.tv_sec = 0;
	stall_time.tv_usec = usecDelta;
	if (select(0, NULL, NULL, NULL, &stall_time) < 0) {
	    perror("Game_loop: select: stall");
	    raise(SIGABRT);
	}
    }
}

void close_socket(DESCRIPTOR_DATA *dclose, bool withProcessOutput, bool withSaveChar)
{
    CHAR_DATA *ch;

    if (withProcessOutput ) {
	if (dclose->outtop > 0)
	    process_output(dclose, false);
	else
	    dclose->outtop = 0;
    }

    if (withSaveChar) {
	if (dclose->character != NULL) {
	    save_char_obj(dclose->character);
	}
    }

    if (dclose->snoop_by != NULL)
	write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r", 0);

    cancel_snoops(dclose);

    if ((ch = dclose->character) != NULL) {
	log_string("Closing link to %s.", ch->name);
	if (dclose->connected == CON_PLAYING) {
	    act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
	    wiznet("Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
	    SET_BIT(ch->act, PLR_LINKDEAD);
	    ch->desc = NULL;
	} else {
	    free_char(dclose->original ? dclose->original : dclose->character);
	}
    }

    dclose->pending_delete = true;
    remote_disconnect(dclose->descriptor);
}

/**
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA *d)
{
    int i, j, k;

    /** Hold horses if pending command already. */
    if (d->incomm[0] != '\0')
	return;

    /** Look for at least one new line.  */
    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
	if (d->inbuf[i] == '\0')
	    return;

    /** Canonical input processing. */
    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
	if (k >= MIL - 2) {
	    remote_write(d->descriptor, "Line too long.\n\r", 0);

	    /* skip the rest of the line */
	    for (; d->inbuf[i] != '\0'; i++)
		if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
		    break;

	    d->inbuf[i] = '\n';
	    d->inbuf[i + 1] = '\0';
	    break;
	}

	if (d->inbuf[i] == '\b' && k > 0)
	    --k;
	else if (isascii((int)d->inbuf[i]) && isprint((int)d->inbuf[i]))
	    d->incomm[k++] = d->inbuf[i];
    }

    /** Finish off the line. */
    if (k == 0)
	d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /** Deal with bozos with #repeat 1000 ... */
    if (k > 1 || d->incomm[0] == '!') {
	if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast)) {
	    d->repeat = 0;
	} else {
	    if (++d->repeat >= 50 && !IS_IMMORTAL(d->character)) {
		send_to_char("`@Acid-Fiend-1 tells you '`tlay off the spam Bucky!`@'`7\n\r", d->character);
		log_string("%s input spamming!", d->host);
		WAIT_STATE(d->character, 25);
		wiznet("Spam spam spam $N spam spam spam spam spam!", d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
		if (d->incomm[0] == '!')
		    wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
		else
		    wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));

		d->repeat = 0;
	    }
	}
    }


    /*
     * Do '!' substitution.
     */
    if (d->incomm[0] == '!')
	strcpy(d->incomm, d->inlast);
    else
	strcpy(d->inlast, d->incomm);

    /*
     * Shift the input buffer.
     */
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
	i++;

    for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++)
	;

    return;
}

/**
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt)
{
    /** Bust a prompt. */
    if (!globalSystemState.merc_down) {
	if (d->showstr_point) {
	    write_to_buffer(d, "\n\r[Hit Return to continue]\n\r\n\r", 0);
	} else if (fPrompt && d->ed_string && d->connected == CON_PLAYING) {
	    write_to_buffer(d, "> ", 2);
	} else if (fPrompt && !globalSystemState.merc_down && d->connected == CON_PLAYING) {
	    CHAR_DATA *ch;
	    CHAR_DATA *victim;

	    ch = d->character;

	    /** battle prompt */
	    if ((victim = ch->fighting) != NULL && can_see(ch, victim))
		show_damage_display(ch, victim);

	    ch = CH(d);
	    if (!IS_SET(ch->comm, COMM_COMPACT))
		write_to_buffer(d, "\n\r", 2);


	    if (IS_SET(ch->comm, COMM_PROMPT))
		bust_a_prompt(d->character);
	}
    }

    /** Short-circuit if nothing to write. */
    if (d->outtop == 0)
	return true;

    /** Snoop-o-rama. */
    if (d->snoop_by != NULL) {
	if (d->character != NULL)
	    write_to_buffer(d->snoop_by, d->character->name, 0);
	write_to_buffer(d->snoop_by, "> ", 2);
	write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
    }

    /** OS-dependent output. */
    if (!remote_write(d->descriptor, d->outbuf, d->outtop)) {
	d->outtop = 0;
	return false;
    } else {
	d->outtop = 0;
	return true;
    }
}

/**
 * Bust a prompt(player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt(CHAR_DATA *ch)
{
    EXIT_DATA *pexit;
    const char *str;
    const char *i;
    char buf[MSL];
    char buf2[MSL];
    char *point;
    char doors[MIL];
    bool found;
    const char *dir_name[] = { "N", "E", "S", "W", "U", "D" };
    bool dClosed;

    int door;

    point = buf;
    str = ch->prompt;
    if (str == NULL || str[0] == '\0') {
	if (!IS_NPC(ch))
	    sprintf(buf, "<`H%d``hp `M%d``m `V%d``mv> %s",
		    ch->hit, ch->mana, ch->move, ch->pcdata->prefix);
	else
	    sprintf(buf, "<`H%d``hp `M%d``m `V%d``mv>", ch->hit, ch->mana, ch->move);

	send_to_char(buf, ch);
	return;
    }

    if (IS_SET(ch->comm, COMM_AFK)) {
	send_to_char("<AFK> ", ch);
	return;
    }

    while (*str != '\0') {
	if (*str != '%') {
	    *point++ = *str++;
	    continue;
	}
	++str;
	switch (*str) {
	    default:
		i = " ";
		break;
	    case 'e':
		found = false;
		doors[0] = '\0';
		for (door = 0; door < 6; door++) {
		    if ((pexit = ch->in_room->exit[door]) != NULL
			    && pexit->u1.to_room != NULL
			    && (can_see_room(ch, pexit->u1.to_room)
				|| (IS_AFFECTED(ch, AFF_INFRARED)
				    && !IS_AFFECTED(ch, AFF_BLIND)))) {
			found = true;
			dClosed = IS_SET(pexit->exit_info, EX_CLOSED);
			if (dClosed) strcat(doors, "("); strcat(doors, dir_name[door]);
			if (dClosed) strcat(doors, ")");
		    }
		}

		if (!found)
		    strcat(buf, "none");
		sprintf(buf2, "%s", doors);
		i = buf2;
		break;
	    case 'c':
		sprintf(buf2, "%s", "\n\r");
		i = buf2;
		break;
	    case 'h':
		sprintf(buf2, "`H%d``", ch->hit);
		i = buf2;
		break;
	    case 'H':
		sprintf(buf2, "`H%d``", ch->max_hit);
		i = buf2;
		break;
	    case 'm':
		sprintf(buf2, "`M%d``", ch->mana);
		i = buf2;
		break;
	    case 'M':
		sprintf(buf2, "`M%d``", ch->max_mana);
		i = buf2;
		break;
	    case 'v':
		sprintf(buf2, "`V%d``", ch->move);
		i = buf2;
		break;
	    case 'V':
		sprintf(buf2, "`V%d``", ch->max_move);
		i = buf2;
		break;
	    case 'x':
		sprintf(buf2, "%d", ch->exp);
		i = buf2;
		break;
	    case 'X':
		if (!IS_NPC(ch))
		    sprintf(buf2, "%d", (ch->level + 1) *
			    exp_per_level(ch, ch->pcdata->points) - ch->exp);
		i = buf2;
		break;
	    case 'g':
		sprintf(buf2, "%u", ch->gold);
		i = buf2;
		break;
	    case 's':
		sprintf(buf2, "%u", ch->silver);
		i = buf2;
		break;
	    case 'r':
		if (ch->in_room != NULL) {
		    sprintf(buf2, "%s",
			    ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) ||
			     (!IS_AFFECTED(ch, AFF_BLIND) && !room_is_dark(ch, ch->in_room)))
			    ? ch->in_room->name : "darkness");
		} else {
		    sprintf(buf2, " ");
		}
		i = buf2;
		break;
	    case 'R':
		if (IS_IMMORTAL(ch) && ch->in_room != NULL)
		    sprintf(buf2, "%ld", ch->in_room->vnum);
		else
		    sprintf(buf2, " ");
		i = buf2;
		break;
	    case 'z':
		if (IS_IMMORTAL(ch) && ch->in_room != NULL)
		    sprintf(buf2, "%s", ch->in_room->area->name);
		else
		    sprintf(buf2, " ");

		i = buf2;
		break;
	    case '%':
		sprintf(buf2, "%%");
		i = buf2;
		break;
	    case 'o':
		sprintf(buf2, "%s", olc_ed_name(ch));
		i = buf2;
		break;
	    case 'O':
		sprintf(buf2, "%s", olc_ed_vnum(ch));
		i = buf2; break;
	}
	++str;
	while ((*point = *i) != '\0')
	    ++point, ++i;
    }

    *point = '\0';
    send_to_char(buf, ch);

    if (ch->desc != NULL) {
	if (is_affected(ch, skill_lookup("sneak")))
	    send_to_char("(`Is``) ", ch);

	if ((is_affected(ch, skill_lookup("invis")))
		|| IS_AFFECTED(ch, AFF_INVISIBLE))
	    send_to_char("(`ii``) ", ch);

	if (IS_AFFECTED(ch, AFF_HIDE))
	    send_to_char("(`hh``) ", ch);

	if (is_affected(ch, skill_lookup("darkness")))
	    send_to_char("(`8d``) ", ch);
    }


    if (ch->incog_level) {
	char buf3[MSL];

	(void)snprintf(buf3, MSL - 1, "`#(`6Incog: `^%d`#)`` ", ch->incog_level);
	send_to_char(buf3, ch);
    }

    if (ch->invis_level) {
	char buf4[MSL];

	(void)snprintf(buf4, MSL - 1, "`O(`@W`Pi`@Z`Pi`@: %d`O)``", ch->invis_level);
	send_to_char(buf4, ch);
    }

    if (ch->pcdata != NULL
	    && ch->pcdata->prefix != NULL
	    && ch->pcdata->prefix[0] != '\0')
	send_to_char(ch->pcdata->prefix, ch);

    send_to_char("\n\r", ch);       /* mutter */
    return;
}

/**
 * Append onto an output buffer.
 */
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length)
{
    if (d == NULL)
	return;

    /** Find length in case caller didn't. */
    if (length <= 0)
	length = (int)strlen(txt);

    /** Initial \n\r if needed. */
    if (d->outtop == 0 && !d->fcommand) {
	d->outbuf[0] = '\n';
	d->outbuf[1] = '\r';
	d->outtop = 2;
    }

    /** Expand the buffer as needed. */
    while (d->outtop + (int)strlen(txt) >= (int)d->outsize) {
	char *outbuf;
	size_t bufsize;

	bufsize = 2 * d->outsize;

	if (d->outsize >= 32000) {
	    log_bug("Buffer overflow. Closing.\n\r");
	    close_socket(d, true, true);
	    return;
	}
	outbuf = calloc(bufsize, sizeof(char));
	if (d->outbuf != NULL) {
	    strncpy(outbuf, d->outbuf, (size_t)d->outtop);
	    free(d->outbuf);
	}
	d->outbuf = outbuf;
	d->outsize = bufsize;
    }

    /** Copy. */
    strncpy(d->outbuf + d->outtop, txt, (size_t)length);
    d->outtop += length;
}


#define CNUM(x) ch->pcdata->x
void process_color(CHAR_DATA *ch, char a)
{
    byte c = (byte)0;
    bool real = true;

    switch (a) {
	case '`':       /* off color */
	    c = (byte)0xf;
	    break;
	case 'A':       /* combat melee opponent */
	    c = (byte)CNUM(color_combat_o);
	    break;
	case 'a':       /* combat melee self */
	    c = (byte)CNUM(color_combat_s);
	    break;
	case 'w':       /* wizi mobs */
	    c = (byte)CNUM(color_wizi);
	    break;
	case 'i':       /* invis mobs */
	    c = (byte)CNUM(color_invis);
	    break;
	case 'h':       /* hidden mobs */
	    c = (byte)CNUM(color_hidden);
	    break;
	case 'H':       /* hp */
	    c = (byte)CNUM(color_hp);
	    break;
	case 'M':       /*mana */
	    c = (byte)CNUM(color_mana);
	    break;
	case 'V':       /* move */
	    c = (byte)CNUM(color_move);
	    break;
	case 's':       /* say */
	    c = (byte)CNUM(color_say);
	    break;
	case 't':       /* tell */
	    c = (byte)CNUM(color_tell);
	    break;
	case 'r':       /* reply */
	    c = (byte)CNUM(color_reply);
	    break;
	case 'c':       /*charm color */
	    c = (byte)CNUM(color_charmed);
	    break;
	case 'C':       /*condition color self */
	    c = (byte)CNUM(color_combat_condition_s);
	    break;
	case 'D':       /* condition opponent */
	    c = (byte)CNUM(color_combat_condition_o);
	    break;
	case '1':
	    c = (byte)0;
	    break;
	case '2':
	    c = (byte)1;
	    break;
	case '3':
	    c = (byte)2;
	    break;
	case '4':
	    c = (byte)3;
	    break;
	case '5':
	    c = (byte)4;
	    break;
	case '6':
	    c = (byte)5;
	    break;
	case '7':
	    c = (byte)6;
	    break;
	case '8':
	    c = (byte)7;
	    break;
	case '!':
	    c = (byte)8;
	    break;
	case '@':
	    c = (byte)9;
	    break;
	case '#':
	    c = (byte)10;
	    break;
	case '^':
	    c = (byte)13;
	    break;
	case '&':
	    c = (byte)14;
	    break;
	case '*':
	    c = (byte)15;
	    break;
	case 'E':       /* Dark red */
	    c = (byte)0;
	    break;
	case 'F':       /* dark green */
	    c = (byte)1;
	    break;
	case 'G':       /* brown */
	    c = (byte)2;
	    break;
	case 'T':       /* Dark Blue  */
	    c = (byte)3;
	    break;
	case 'I':       /* Dark Purple */
	    c = (byte)4;
	    break;
	case 'J':       /* Cyan */
	    c = (byte)5;
	    break;
	case 'K':       /* Bright Gray */
	    c = (byte)6;
	    break;
	case 'L':       /* Bright Black */
	    c = (byte)7;
	    break;
	case 'S':       /* Bright Green */
	    c = (byte)9;
	    break;
	case 'N':       /* Yellow */
	    c = (byte)10;
	    break;
	case 'O':       /* Bright Blue */
	    c = (byte)11;
	    break;
	case 'P':       /* Bright Purple */
	    c = (byte)12;
	    break;
	case 'Q':       /* Bright Cyan */
	    c = (byte)13;
	    break;
	case '%':
	case '$':
	    write_to_buffer(ch->desc, "", 0);
	    real = false;
	    break;
	case 'z':
	    write_to_buffer(ch->desc, "`", 0);
	    real = false;
	    break;
	case '-':
	    write_to_buffer(ch->desc, "~", 0);
	    real = false;
	    break;
	case ';':
	    write_to_buffer(ch->desc, "\033", 0);
	    real = false;
	    break;
	    /*
	     * case 'R':
	     *  c = 14;
	     *  break;
	     * case 'B':
	     *  c = 8;
	     *  break;
	     */
	case 'R':
	    write_to_buffer(ch->desc, "\n\r", 0);
	    real = false;
	    break;
	case 'B':
	    write_to_buffer(ch->desc, "\a", 0);
	    real = false;
	    break;
	case '|':
	    c = (byte)number_range(0, 14);
	    break;
	default:        /* unknown ignore */
	    return;
    }
    if (real)
	write_to_buffer(ch->desc, color_table[(int)c], (int)strlen(color_table[c]));
}

/*
 * Write to one char.
 */
void send_to_char(char *txt, CHAR_DATA *ch)
{
    char *a, *b;
    int length, l, c = 0, curlen = 0;

    /*    a=txt; */
    length = (int)strlen(txt);

    /*    stupid_telix_users(txt); */
    a = txt;

    if (txt != NULL && ch->desc != NULL) {
	while (curlen < length) {
	    b = a;
	    l = 0;
	    while (curlen < length && *a != '`') {
		l++;
		curlen++;
		a++;
		c++;
	    }

	    if (l)
		write_to_buffer(ch->desc, b, l);

	    if (*a != '\0') {
		a++;
		curlen++;
		if (curlen < length && ch->use_ansi_color) {
		    process_color(ch, *a++);
		    curlen++;
		} else {
		    a++;
		    curlen++;
		}
	    }
	}
    }
}

/* routine used to send color codes without displaying colors */
/* JDS */
void send_to_char_ascii(char *txt, CHAR_DATA *ch)
{
    if (txt != NULL && ch->desc != NULL)
	write_to_buffer(ch->desc, string_replace(txt, "|", "?"), (int)strlen(txt));
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char(const char *txt, const CHAR_DATA *ch)
{
    if (txt == NULL || ch->desc == NULL)
	return;

    ch->desc->showstr_head = alloc_mem((unsigned int)(strlen(txt) + 1));
    strcpy(ch->desc->showstr_head, txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc, "");
}

/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[4 * MSL];
    char buf[MIL];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    (void)one_argument(input, buf);
    if (buf[0] != '\0') {
	if (d->showstr_head) {
	    free_string(d->showstr_head);
	    d->showstr_head = 0;
	}
	d->showstr_point = 0;
	return;
    }

    if (d->character)
	show_lines = d->character->lines;
    else
	show_lines = 0;

    for (scan = buffer;; scan++, d->showstr_point++) {
	if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
		&& (toggle = -toggle) < 0) {
	    lines++;
	} else if (!*scan || (show_lines > 0 && lines >= show_lines)) {
	    *scan = '\0';
	    if (d->character)
		send_to_char(buffer, d->character);
	    else
		write_to_buffer(d, buffer, (int)strlen(buffer));
	    for (chk = d->showstr_point; is_space(*chk); chk++) ;
	    {
		if (!*chk) {
		    if (d->showstr_head) {
			free_string(d->showstr_head);
			d->showstr_head = 0;
		    }
		    d->showstr_point = 0;
		}
	    }
	    return;
	}
    }
}


/* source: EOD, by John Booth <???> */
/***************************************************************************
 *	printf_to_char
 ***************************************************************************/
void printf_to_char(CHAR_DATA *ch, char *fmt, ...)
{
    char buf[MSL];

    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    send_to_char(buf, ch);
}

void set_wait(CHAR_DATA *ch, int len)
{
    CHAR_DATA *vch;
    int mod;
    float newmod;

    if (ch == NULL || IS_IMMORTAL(ch))
	return;

    /* length is decreased by DEX maxxed out at 1/2 len + 1*/
    mod = get_curr_stat(ch, STAT_DEX) / 100;
    mod = UMIN((len / 2) + 1, mod);

    /* subtract the modifier */
    len -= mod;

    /* additional modifier added by Monrick, May 2008 */
    if (ch->mLag != 0) {
	newmod = (float)(len * ((float)ch->mLag / 100));
	if (ch->mLag > 0)
	    newmod = UMAX(1.0f, newmod);
	else
	    newmod = UMIN(-1.0f, newmod);
	len += newmod;
    }
    if ((vch = ch->fighting) != NULL) {
	if (vch->tLag != 0) {
	    newmod = (float)(len * ((float)vch->tLag / 100));
	    if (vch->tLag > 0)
		newmod = UMAX(1.0f, newmod);
	    else
		newmod = UMIN(-1.0f, newmod);
	    len += newmod;
	}
    }

    /* set the wait time */
    ch->wait = UMAX(ch->wait, len);
}

void auto_shutdown()
{
}

/**
 * determine whether a character is active - if they are, remove the AFK bit
 */
void check_afk(CHAR_DATA *ch)
{
    if (ch == NULL || ch->desc == NULL || ch->desc->connected != CON_PLAYING)
	return;

    if (IS_SET(ch->comm, COMM_AFK))
	REMOVE_BIT(ch->comm, COMM_AFK);
}


void process_all_input()
{
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;

    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &descriptor_empty_filter);

	/** 
	 * The exc_set condition specifies some sort of exceptional condition (not erroneous),
	 * such as an urgent message. This code is not prepared to handle such situations, so
	 * a disconnect is really the only course of action. (aka Kick out the freaky folks.)
	 */
	if (d->ready_exceptional) {
	    log_string("Freaky host %s!", d->host);
	    close_socket(d, false, true);
	    continue;
	} 
	
	if (d->ready_input) {
	    /* Hold horses if pending command already. */
	    if (d->incomm[0] == '\0') {
		int outcome = remote_read(d->descriptor, 4*MIL, d->inbuf);
		if (outcome != DESC_READ_RESULT_OK) {
		    switch (outcome) {
			case DESC_READ_RESULT_OVERFLOW:
			    log_string("%s input overflow!", d->host);
			    break;
			case DESC_READ_RESULT_EOF:
			    log_string("%s EOF encountered on read.", d->host);
			    break;
			case DESC_READ_RESULT_IOERROR:
			    log_string("%s read error.", d->host);
			    break;
		    }

		    close_socket(d, false, true);
		    continue;
		}
	    }

	    d->idle = 0; /* Kyndig: reset their idle timer */
	    if (d->character != NULL) {
		d->character->timer = 0;
	    }
	}


	/** Kyndig: Get rid of idlers as well. */
	if ((!d->character && d->idle > 50 /* 10 seconds */) || d->idle > 28800 /* 2 hrs  */) { 
	    remote_write(d->descriptor, "Idle.\n\r", 0);
	    close_socket(d, false, true);
	    continue;
	}

	if (d->character != NULL && d->character->wait > 0) {
	    --d->character->wait;
	    continue;
	}

	read_from_buffer(d);
	if (d->incomm[0] != '\0') {
	    d->fcommand = true;
	    stop_idling(d->character);
	    check_afk(d->character);

	    if (d->showstr_point) {
		show_string(d, d->incomm);
	    } else {
		if (d->ed_string) {
		    string_add(d->character, d->incomm);
		} else {
		    switch (d->connected) {
			case CON_PLAYING:
			    if (!run_olc_editor(d))
				substitute_alias(d, d->incomm);
			    break;
			default:
			    nanny(d, d->incomm);
			    break;
		    }
		}
	    }

	    d->incomm[0] = '\0';
	}
    }
}

void process_all_output() 
{
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;

    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
	dpending = descriptor_iterator(d, &descriptor_empty_filter);

	if (d->ready_output && (d->fcommand || d->outtop > 0)) {
	    if (!process_output(d, true)) {
		close_socket(d, false, true);
	    }
	}
    }
}

void on_new_connection(int descriptor, int ipaddress, const char *hostname)
{
    DESCRIPTOR_DATA *dnew;

    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if (check_ban(hostname, BAN_ALL)) {
	remote_write(descriptor, "Your site has been banned from this mud.\n\r", 0);
	remote_disconnect(descriptor);
	return;
    }

    dnew = descriptor_new(descriptor);
    dnew->host = str_dup(hostname);
    log_string("Sock.sinaddr: %d.%d.%d.%d", 
	    (ipaddress >> 24) & 0xFF, 
	    (ipaddress >> 16) & 0xFF, 
	    (ipaddress >> 8) & 0xFF, 
	    (ipaddress) & 0xFF);

    /** Init descriptor data. */
    remote_write(dnew->descriptor, "Ansi intro screen?(y/n) \n\r", 0);
}

