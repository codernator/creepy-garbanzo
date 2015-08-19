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
#include <sys/time.h>
#include <sys/select.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "merc.h"
#include "recycle.h"
#include "interp.h"

/**
 * Socket and TCP/IP stuff.
 */
#include <signal.h>
#if !defined(STDOUT_FILENO)
#define STDOUT_FILENO 1
#endif

/** OS-dependent declarations. */
extern int close(int fd);
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);

extern int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
extern int socket(int domain, int type, int protocol);

extern pid_t waitpid(pid_t pid, int *status, int options);
extern pid_t fork(void);
extern int kill(pid_t pid, int sig);
extern int pipe(int filedes[2]);
extern int dup2(int oldfd, int newfd);
extern int execl(const char *path, const char *arg, ...);

/** socketio.c */
extern bool read_from_descriptor(DESCRIPTOR_DATA *d);
extern bool write_to_descriptor(int desc, char *txt, int length);
extern void init_descriptor(int control);
/** ~socketio.c */


/** Game declarations. */
extern char *color_table[];
extern bool is_space(const char test);
extern bool run_olc_editor(DESCRIPTOR_DATA * d);
extern char *olc_ed_name(CHAR_DATA * ch);
extern char *olc_ed_vnum(CHAR_DATA * ch);
extern void mp_act_trigger(char *argument, CHAR_DATA * mob, CHAR_DATA * ch, const void *arg1, const void *arg2, int type);
extern void string_add(CHAR_DATA * ch, char *argument);
extern char *string_replace(char *orig, char *old, char *new);

bool copyover();
void sig_handler(int sig);
void game_loop(int port, int control);
void copyover_recover(void);

static bool process_output(DESCRIPTOR_DATA * d, bool fPrompt);
static void read_from_buffer(DESCRIPTOR_DATA * d);
static void check_afk(CHAR_DATA * ch);
static void bust_a_prompt(CHAR_DATA * ch);
static void auto_shutdown(void);
volatile sig_atomic_t fatal_error_in_progress = 0;



/** TODO - these are only outside of main scope because the copyover routine needs them. */
static int listen_port;
static int listen_control;
void game_loop(int port, int control)
{
	static struct timeval null_time;
	struct timeval last_time;
    static const struct descriptor_iterator_filter allfilter = { .all = true };

    listen_port = port;
    listen_control = control;

	gettimeofday(&last_time, NULL);
	globalSystemState.current_time = (time_t)last_time.tv_sec;

    /** Main loop */
	while (!globalSystemState.merc_down) {
		fd_set in_set;
		fd_set out_set;
		fd_set exc_set;
		DESCRIPTOR_DATA *d;
        DESCRIPTOR_DATA *dpending;
		int maxdesc;

		FD_ZERO(&in_set);
		FD_ZERO(&out_set);
		FD_ZERO(&exc_set);
		FD_SET(control, &in_set);
		maxdesc = control;

        dpending = descriptor_iterator_start(&allfilter);
        while ((d = dpending) != NULL) {
            dpending = descriptor_iterator(d, &allfilter);

            if (d->pending_delete) {
                descriptor_list_remove(d);
                free_descriptor(d);
            } else {
                maxdesc = UMAX(maxdesc, (int)d->descriptor);
                FD_SET(d->descriptor, &in_set);
                FD_SET(d->descriptor, &out_set);
                FD_SET(d->descriptor, &exc_set);
            }
		}

		if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0) {
			perror("Game_loop: select: poll");
            raise(SIGABRT);
		}

		/** New connection? */
		if (FD_ISSET(control, &in_set)) {
			init_descriptor(control);
        }

		/** Kick out the freaky folks. Kyndig: Get rid of idlers as well. */
        dpending = descriptor_iterator_start(&descriptor_empty_filter);
        while ((d = dpending) != NULL) {
            dpending = descriptor_iterator(d, &descriptor_empty_filter);

			d->idle++;
			if (FD_ISSET(d->descriptor, &exc_set)) {
				FD_CLR(d->descriptor, &in_set);
				FD_CLR(d->descriptor, &out_set);
				if (d->character && d->character->level > 1) {
					save_char_obj(d->character);
                }
				d->outtop = 0;
				close_socket(d);
			} else if ((!d->character && d->idle > 50 /* 10 seconds */) || d->idle > 28800 /* 2 hrs  */) { 
				write_to_descriptor(d->descriptor, "Idle.\n\r", 0);
				d->outtop = 0;
				close_socket(d);
				continue;
			}
		}

		/** Process input. */
        dpending = descriptor_iterator_start(&descriptor_empty_filter);
        while ((d = dpending) != NULL) {
            dpending = descriptor_iterator(d, &descriptor_empty_filter);
			d->fcommand = false;

			if (FD_ISSET(d->descriptor, &in_set)) {
				if (d->character != NULL) {
					d->character->timer = 0;
					d->idle = 0; /* Kyndig: reset their idle timer */
				}

				if (!read_from_descriptor(d)) {
					FD_CLR(d->descriptor, &out_set);
					if (d->character != NULL && d->character->level > 1)
						save_char_obj(d->character);
					d->outtop = 0;
					close_socket(d);
					continue;
				}
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

		/** Autonomous game motion. */
		update_handler();

		/** Output. */
        dpending = descriptor_iterator_start(&descriptor_empty_filter);
        while ((d = dpending) != NULL) {
            dpending = descriptor_iterator(d, &descriptor_empty_filter);

			if ((d->fcommand || d->outtop > 0) && FD_ISSET(d->descriptor, &out_set)) {
				if (!process_output(d, true)) {
					if (d->character != NULL && d->character->level > 1) {
						save_char_obj(d->character);
                    }
					d->outtop = 0;
					close_socket(d);
				}
			}
		}

		/*
		 * Synchronize to a clock.
		 * Sleep(last_time + 1/PULSE_PER_SECOND - now).
		 * Careful here of signed versus unsigned arithmetic.
		 */
		{
			struct timeval now_time;
			long secDelta;
			long usecDelta;

			gettimeofday(&now_time, NULL);
			usecDelta = ((int)last_time.tv_usec) - ((int)now_time.tv_usec) + 1000000 / PULSE_PER_SECOND;
			secDelta = ((int)last_time.tv_sec) - ((int)now_time.tv_sec);
			while (usecDelta < 0) {
				usecDelta += 1000000;
				secDelta -= 1;
			}

			while (usecDelta >= 1000000) {
				usecDelta -= 1000000;
				secDelta += 1;
			}

			if (secDelta > 0 || (secDelta == 0 && usecDelta > 0)) {
				struct timeval stall_time;

				stall_time.tv_usec = usecDelta;
				stall_time.tv_sec = secDelta;
				if (select(0, NULL, NULL, NULL, &stall_time) < 0) {
					perror("Game_loop: select: stall");
                    raise(SIGABRT);
				}
			}
		}

		gettimeofday(&last_time, NULL);
		globalSystemState.current_time = (time_t)last_time.tv_sec;
	}
}

void close_socket(DESCRIPTOR_DATA *dclose)
{
	CHAR_DATA *ch;

	if (dclose->outtop > 0)
		process_output(dclose, false);

	if (dclose->snoop_by != NULL)
		write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r", 0);

	{
        struct descriptor_iterator_filter playing_filter = { .all = true };
        DESCRIPTOR_DATA *dpending;
        DESCRIPTOR_DATA *d;

        dpending = descriptor_iterator_start(&playing_filter);
        while ((d = dpending) != NULL) {
            dpending = descriptor_iterator(d, &playing_filter);
			if (d->snoop_by == dclose) {
				d->snoop_by = NULL;
            }
        }
	}

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
	close(dclose->descriptor);
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
			write_to_descriptor(d->descriptor, "Line too long.\n\r", 0);

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
	if (!write_to_descriptor(d->descriptor, d->outbuf, d->outtop)) {
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
		case 'a':
			if (ch->level > 9) {
				sprintf(buf2, "%d", ch->alignment);
			} else {
				sprintf(buf2, "%s",
					IS_GOOD(ch) ? "good" :
					IS_EVIL(ch) ? "evil" : "neutral");
			}

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

		sprintf(buf3, "`#(`6Incog: `^%d`#)`` ", ch->incog_level);
		send_to_char(buf3, ch);
	}

	if (ch->invis_level) {
		char buf4[MSL];

		sprintf(buf4, "`O(`@W`Pi`@Z`Pi`@: %d`O)``", ch->invis_level);
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
	while (d->outtop + (int)strlen(txt) >= d->outsize) {
		char *outbuf;

		if (d->outsize >= 32000) {
			log_bug("Buffer overflow. Closing.\n\r");
			close_socket(d);
			return;
		}
		outbuf = alloc_mem((unsigned int)(2 * d->outsize));
		strncpy(outbuf, d->outbuf, (size_t)d->outtop);
		free_mem(d->outbuf, (unsigned int)d->outsize);
		d->outbuf = outbuf;
		d->outsize *= 2;
	}

    /** Copy. */
/*  strcpy(d->outbuf + d->outtop, txt);  growl ..   */
	strncpy(d->outbuf + d->outtop, txt, (size_t)length);
	d->outtop += length;
	return;
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
		write_to_buffer(ch->desc, color_table[c], (int)strlen(color_table[c]));
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
void page_to_char(char *txt, CHAR_DATA *ch)
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

	one_argument(input, buf);
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

/* quick sex fixer */
void fix_sex(CHAR_DATA *ch)
{
	if (ch->sex < 0 || ch->sex > 2)
		ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type)
{
	/* to be compatible with older code */
	act_new(format, ch, arg1, arg2, type, POS_RESTING, true);
}

void act_new(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type, int min_pos, bool mob_trigger)
{
	static char *const he_she[] = { "it", "he", "she" };
	static char *const him_her[] = { "it", "him", "her" };
	static char *const his_her[] = { "its", "his", "her" };

	CHAR_DATA *to;
	CHAR_DATA *vch = (CHAR_DATA *)arg2;
	OBJ_DATA *obj1 = (OBJ_DATA *)arg1;
	OBJ_DATA *obj2 = (OBJ_DATA *)arg2;
	char buf[MSL];
	char fname[MIL];
	const char *str;
	char *i = NULL;
	char *point;

    /*
     * Discard null and zero-length messages.
     */
	if (format == NULL || format[0] == '\0')
		return;


    /* discard null rooms and chars */
	if (ch == NULL || ch->in_room == NULL)
		return;

	to = ch->in_room->people;
	if (type == TO_VICT) {
		if (vch == NULL) {
			log_bug("Act: null vch with TO_VICT.");
			return;
		}

		if (vch->in_room == NULL)
			return;

		to = vch->in_room->people;
	}

	for (; to != NULL; to = to->next_in_room) {
		if (to->position < min_pos)
			continue;

		if (to->desc == NULL
		    && (!IS_NPC(to) || !HAS_TRIGGER(to, TRIG_ACT)))
			continue;

		if ((type == TO_CHAR) && to != ch)
			continue;

		if (type == TO_VICT && (to != vch || to == ch))
			continue;

		if (type == TO_ROOM && to == ch)
			continue;

		if (type == TO_NOTVICT && (to == ch || to == vch))
			continue;

		point = buf;
		str = format;
		while (*str != '\0') {
			if (*str != '$') {
				*point++ = *str++;
				continue;
			}
			++str;

			if (arg2 == NULL && *str >= 'A' && *str <= 'Z') {
				log_bug("Act: missing arg2 for code %d.", (int)*str);
				i = " <@@@> ";
			} else {
				switch (*str) {
				default:
					log_bug("Act: bad code %d.", (int)*str);
					i = " <@@@> ";
					break;
				/* Thx alex for 't' idea */
				case 't':
					if (arg1) i = (char *)arg1;
					else log_bug("Act: bad code $t for 'arg1'");
					break;
				case 'T':
					if (arg2) i = (char *)arg2;
					else log_bug("Act: bad code $T for 'arg2'");
					break;
				case 'n':
					if (ch && to) i = PERS(ch, to);
					else log_bug("Act: bad code $n for 'ch' or 'to'");
					break;
				case 'N':
					if (vch && to) i = PERS(vch, to);
					else log_bug("Act: bad code $N for 'vch' or 'to'");
					break;
				case 'e':
					if (ch) i = he_she[URANGE(0, ch->sex, 2)];
					else log_bug("Act: bad code $e for 'ch'");
					break;
				case 'E':
					if (vch) i = he_she[URANGE(0, vch->sex, 2)];
					else log_bug("Act: bad code $E for 'vch'");
					break;
				case 'm':
					if (ch) i = him_her[URANGE(0, ch->sex, 2)];
					else log_bug("Act: bad code $m for 'ch'");
					break;
				case 'M':
					if (vch) i = him_her[URANGE(0, vch->sex, 2)];
					else log_bug("Act: bad code $M for 'vch'");
					break;
				case 's':
					if (ch) i = his_her[URANGE(0, ch->sex, 2)];
					else log_bug("Act: bad code $s for 'ch'");
					break;
				case 'S':
					if (vch) i = his_her[URANGE(0, vch->sex, 2)];
					else log_bug("Act: bad code $S for 'vch'");
					break;
				case 'p':
					i = can_see_obj(to, obj1) ? obj1->short_descr : "something";
					break;
				case 'P':
					i = can_see_obj(to, obj2) ? obj2->short_descr : "something";
					break;

				case 'd':
					if (arg2 == NULL || ((char *)arg2)[0] == '\0') {
						i = "door";
					} else {
						one_argument((char *)arg2, fname);
						i = fname;
					}
					break;
				}
			}

			++str;
			while ((*point = *i) != '\0')
				++point, ++i;
		}

		*point++ = '\n';
		*point++ = '\r';
		*point = '\0';
		buf[0] = UPPER(buf[0]);

		if (to->desc) {
			send_to_char(buf, to);
		} else {
			if (mob_trigger)
				mp_act_trigger(buf, to, ch, arg1, arg2, TRIG_ACT);
		}
	}
	return;
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

/**
 * 2015-08-10
 * see: http://www.gnu.org/software/libc/manual/html_node/Termination-in-Handler.html#Termination-in-Handler 
 */
void sig_handler(int sig)
{ 
    /* Since this handler is established for more than one kind of signal, it might still 
     * get invoked recursively by delivery of some other kind of signal.  Use a static 
     * variable to keep track of that. 
     */
    if (fatal_error_in_progress) {
        raise(sig);
    }

    fatal_error_in_progress = 1;
    psignal(sig, "Auto shutdown invoked.");
    log_bug("Critical signal received %d", sig);
    auto_shutdown();

    /* Now reraise the signal. We reactivate the signal’s default handling, which is to 
     * terminate the process. We could just call exit or abort,  but reraising the signal 
     * sets the return status from the process correctly.
     */
    signal(sig, SIG_DFL);
    raise(sig);
}

void auto_shutdown()
{
	FILE *cmdLog;

	if ((cmdLog = fopen(LAST_COMMANDS, "r")) == NULL) {
		log_string("Crash function: can't open last commands log..");
        return;
	} 
    
    time_t rawtime;
    struct tm *timeinfo;
    char buf[128];
    char cmd[256];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buf, 128, "./log/command/lastCMDs-%m%d-%H%M.txt", timeinfo);
    sprintf(cmd, "mv ./log/command/lastCMDs.txt %s", buf);
    if (system(cmd) == -1) {
        log_string("System command failed: ");
        log_string(cmd);
    }
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

bool copyover()
{
	FILE *fp, *cmdLog;
	DESCRIPTOR_DATA *d, *dpending;
	char buf [100], buf2[100];

	fp = fopen(COPYOVER_FILE, "w");

	if (!fp) {
		perror("do_copyover:fopen");
		return false;
	}

	/* Consider changing all saved areas here, if you use OLC */

	/* do_asave (NULL, ""); - autosave changed areas */


	sprintf(buf, "\n\r Preparing for a copyover....\n\r");

	/* For each playing descriptor, save its state */
    dpending = descriptor_iterator_start(&descriptor_empty_filter);
    while ((d = dpending) != NULL) {
		CHAR_DATA *och = CH(d);

        dpending = descriptor_iterator(d, &descriptor_empty_filter);

		if (!d->character || d->connected > CON_PLAYING) { /* drop those logging on */
			write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
			close_socket(d);  /* throw'em out */
		} else {
			fprintf(fp, "%d %s %s\n", d->descriptor, och->name, d->host);

			if (och->level == 1) {
				write_to_descriptor(d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\n\r", 0);
				advance_level(och, 1);
			}
			do_stand(och, "");
			save_char_obj(och);
		}
	}

	fprintf(fp, "-1\n");
	fclose(fp);

	/* Dalamar - Save our last commands file.. */
	if ((cmdLog = fopen(LAST_COMMANDS, "r")) == NULL) {
		log_string("Crash function: can't open last commands log..");
	} else {
		time_t rawtime;
		struct tm *timeinfo;
		char buf[128];
		char cmd[256];

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buf, 128, "./log/command/lastCMDs-%m%d-%H%M.txt", timeinfo);
		sprintf(cmd, "mv ./log/command/lastCMDs.txt %s", buf);
		if (system(cmd) == -1) {
            log_string("System command failed: ");
            log_string(cmd);
        }
	}

	/** exec - descriptors are inherited */
	sprintf(buf, "%d", listen_port);
	sprintf(buf2, "%d", listen_control);
	execl(EXE_FILE, "Badtrip", buf, "copyover", buf2, (char *)NULL);

	/** Failed - sucessful exec will not return */
	perror("do_copyover: execl");
	return false;
}

/* Recover from a copyover - load players */
void copyover_recover()
{
	DESCRIPTOR_DATA *d;
	FILE *fp;
	char name [100];
	char host[MSL];
	int desc;
	bool fOld;

/*	logf ("Copyover recovery initiated");*/

	fp = fopen(COPYOVER_FILE, "r");

	if (!fp) { /* there are some descriptors open which will hang forever then ? */
		perror("copyover_recover:fopen");
		raise(SIGABRT);
		return;
	}

	unlink(COPYOVER_FILE);  /* In case something crashes - doesn't prevent reading	*/

	for (;; ) {
        int scancount;
		scancount = fscanf(fp, "%d %s %s\n", &desc, name, host);
		if (scancount == EOF || desc == -1)
			break;

		/* Write something, and check if it goes error-free */
		if (!write_to_descriptor(desc, "", 0)) {
			close(desc);  /* nope */
			continue;
		}

		d = new_descriptor();
		d->descriptor = desc;

		d->host = str_dup(host);
        descriptor_list_add(d);
		d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */


		/* Now, find the pfile */

		fOld = load_char_obj(d, name);

		if (!fOld) { /* Player file not found?! */
			write_to_descriptor(desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
			close_socket(d);
		} else { /* ok! */
/*			write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r",0);*/

			/* Just In Case */
			if (!d->character->in_room)
				d->character->in_room = get_room_index(ROOM_VNUM_TEMPLE);

			/* Insert in the char_list */
			d->character->next = char_list;
			char_list = d->character;

			send_to_char("\n\r`6o`&-`O-====`8---------------------------------------------------------`O===`&--`6o``\n\r", d->character);
			send_to_char("`2       ___    ___        __________________        ___    ___``\n\r", d->character);
			send_to_char("`2  ____/ _ \\__/ _ \\_____ (------------------) _____/ _ \\__/ _ \\____``\n\r", d->character);
			send_to_char("`2 (  _| / \\/  \\/ \\ |_   ) \\    `&Copyover`2    / (   _| / \\/  \\/ \\ |_  )``\n\r", d->character);
			send_to_char("`2  \\(  \\|  )  (  |/  ) (___)  __________  (___) (  \\|  )  (  |/  )/``\n\r", d->character);
			send_to_char("`2   '   '  \\`!''`2/  '  (_________)        (_________)  '  \\`!''`2/  '   '``\n\r", d->character);
			send_to_char("`2           ||                                          ||``\n\r", d->character);
			send_to_char("`6o`&-`O-====`8---------------------------------------------------------`O===`&--`6o``\n\r", d->character);
			char_to_room(d->character, d->character->in_room);
			do_look(d->character, "auto");
			act("$n materializes!", d->character, NULL, NULL, TO_ROOM);
			d->connected = CON_PLAYING;

			if (d->character->pet != NULL) {
				char_to_room(d->character->pet, d->character->in_room);
				act("$n materializes!.", d->character->pet, NULL, NULL, TO_ROOM);
			}
		}
	}
	fclose(fp);
}
