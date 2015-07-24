/**************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,        *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                             *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael          *
 *   Chastain, Michael Quan, and Mitchell Tse.                              *
 *	                                                                       *
 *   In order to use any part of this Merc Diku Mud, you must comply with   *
 *   both the original Diku license in 'license.doc' as well the Merc	   *
 *   license in 'license.txt'.  In particular, you may not remove either of *
 *   these copyright notices.                                               *
 *                                                                             *
 *   Much time and thought has gone into this software and you are          *
 *   benefitting.  We hope that you share your changes too.  What goes      *
 *   around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor(rtaylor@hypercube.org)                                *
*       Gabrielle Taylor(gtaylor@hypercube.org)                           *
*       Brian Moore(zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
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
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <strings.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include "merc.h"
#include "recycle.h"
#include "interp.h"

extern char *color_table[];
extern bool is_space(const char test);



/***************************************************************************
* Command tracking stuff.
***************************************************************************/
void init_signals       args((void));
void auto_shutdown      args((void));



/***************************************************************************
* Signal handling.
* Apollo has a problem with __attribute(atomic) in signal.h,
*   I dance around it.
***************************************************************************/
#if defined(unix) || defined(WIN32)
#include <signal.h>
#endif



/***************************************************************************
*  Socket and TCP/IP stuff.
***************************************************************************/

#if     defined(unix)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "telnet.h"
#include <signal.h>
#if !defined(STDOUT_FILENO)
#define STDOUT_FILENO 1
#endif
#endif


/***************************************************************************
* OS-dependent declarations.
***************************************************************************/

#if     defined(linux)
int close                   args((int fd));
char *crypt                   args((const char *key, const char *salt));
int gettimeofday    args((struct timeval *tp, struct timezone *tzp));

int select                  args((int width, fd_set * readfds, fd_set * writefds,
				  fd_set * exceptfds, struct timeval *timeout));
int socket                  args((int domain, int type, int protocol));

pid_t waitpid                 args((pid_t pid, int *status, int options));
pid_t fork                    args((void));
int kill                    args((pid_t pid, int sig));
int pipe                    args((int filedes[2]));
int dup2                    args((int oldfd, int newfd));
int execl                   args((const char *path, const char *arg, ...));
#endif




/***************************************************************************
* Global variables.
***************************************************************************/
DESCRIPTOR_DATA *descriptor_list;               /*   All open descriptors         */
DESCRIPTOR_DATA *d_next;                        /*   Next descriptor in loop      */
FILE *fpReserve;                                /*   Reserved file handle         */
bool god;                                       /*   All new chars are gods!      */
bool merc_down;                                 /*   Shutdown                     */
bool wizlock;                                   /*   Game is wizlocked            */
bool tickset;                                   /*   force a tick? whaat? --Eo    */
bool quiet_note_post;                           /*   Post notes quietly! --Eo     */
bool newlock;                                   /*   Game is newlocked            */
char boot_time[MIL];
time_t current_time;                            /*   time of this pulse           */
int port;
int control;
int max_on = 0;

/***************************************************************************
* OS-dependent local functions.
***************************************************************************/

#if defined(unix) || defined(WIN32)
void game_loop                               args((int control));
int init_socket                             args((int port));
void init_descriptor                 args((int control));
bool read_from_descriptor    args((DESCRIPTOR_DATA * d));
bool write_to_descriptor             args((int desc, char *txt, int length));
#endif



/***************************************************************************
* Other local functions(OS-independent).
***************************************************************************/
int main                            args((int argc, char **argv));
bool process_output          args((DESCRIPTOR_DATA * d, bool fPrompt));
void read_from_buffer        args((DESCRIPTOR_DATA * d));
void check_afk                       args((CHAR_DATA * ch));
void bust_a_prompt           args((CHAR_DATA * ch));
bool is_host_exception       args((char *host));
void init_signals            args((void));
extern void do_auto_shutdown        args((void));


/***************************************************************************
* sig_handler
***************************************************************************/
/*
 * void sig_handler(int sig)
 * {
 * #if defined(__USE_SIGNALS)
 *  switch(sig)
 *  {
 *      case SIGTERM:
 *          log_string("Sig handler SIGTERM.");
 *          do_auto_shutdown();
 *          break;
 *      case SIGABRT:
 *          log_string("Sig handler SIGABRT.");
 *          do_auto_shutdown();
 *          break;
 *      case SIGSEGV:
 *          log_string("Sig handler SIGSEGV.");
 *          break;
 *  }
 * #endif
 * }*/

/*
 * void init_signals()
 * {
 * #if defined(__USE_SIGNALS)
 *  signal(SIGTERM, sig_handler);
 *  signal(SIGABRT, sig_handler);
 *  signal(SIGSEGV, sig_handler);
 * #endif
 * }*/
int main(int argc, char **argv)
{
	struct timeval now_time;
	bool fCopyOver = FALSE;

	init_signals();


/*
 * Init time.
 */
	gettimeofday(&now_time, NULL);
	current_time = (time_t)now_time.tv_sec;
	strcpy(boot_time, (char *)ctime(&current_time));


/*
 * Reserve one channel for our use.
 */
	if ((fpReserve = fopen(NULL_FILE, "r")) == NULL) {
		perror(NULL_FILE);
		exit(1);
	}

/*
 * Get the port number.
 */
	port = 7778;
	if (argc > 1) {
		if (!is_number(argv[1])) {
			fprintf(stderr, "Usage: %s [port #]\n", argv[0]);
			exit(1);
		} else if ((port = atoi(argv[1])) <= 1024) {
			fprintf(stderr, "Port number must be above 1024.\n");
			exit(1);
		}

		/* Are we recovering from a copyover? */
		if (argv[2] && argv[2][0]) {
			fCopyOver = TRUE;
			control = atoi(argv[3]);
		} else {
			fCopyOver = FALSE;
		}
	}



/*
 * Run the game.
 */

#if defined(unix) || defined(WIN32)
	if (!fCopyOver)
		control = init_socket(port);

	boot_db();
	sprintf(log_buf, "BT is ready to rock on port %d.", port);
	log_string(log_buf);

	if (fCopyOver)
		copyover_recover();

	game_loop(control);
	close(control);
#endif


	log_string("Normal termination of game.");
	exit(0);
}


#if defined(unix) || defined(WIN32)
int init_socket(int port)
{
	static struct sockaddr_in sa_zero;
	struct sockaddr_in sa;
	int x = 1;
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Init_socket: socket");
		exit(1);
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) < 0) {
		perror("Init_socket: SO_REUSEADDR");
		close(fd);
		exit(1);
	}

#if defined(SO_DONTLINGER) && !defined(SYSV)
	{
		struct linger ld;

		ld.l_onoff = 1;
		ld.l_linger = 1000;

		if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, (char *)&ld, sizeof(ld)) < 0) {
			perror("Init_socket: SO_DONTLINGER");
			close(fd);
			exit(1);
		}
	}
#endif

	sa = sa_zero;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("Init socket: bind");
		close(fd);
		exit(1);
	}


	if (listen(fd, 3) < 0) {
		perror("Init socket: listen");
		close(fd);
		exit(1);
	}

	return fd;
}
#endif

#if defined(unix) || defined(WIN32)
void game_loop(int control)
{
	static struct timeval null_time;
	struct timeval last_time;

	gettimeofday(&last_time, NULL);
	current_time = (time_t)last_time.tv_sec;

	init_signals();

/* Main loop */
	while (!merc_down) {
		fd_set in_set;
		fd_set out_set;
		fd_set exc_set;
		DESCRIPTOR_DATA *d;
		int maxdesc;



		FD_ZERO(&in_set);
		FD_ZERO(&out_set);
		FD_ZERO(&exc_set);
		FD_SET(control, &in_set);
		maxdesc = control;

		for (d = descriptor_list; d; d = d->next) {
			maxdesc = UMAX(maxdesc, (int)d->descriptor);
			FD_SET(d->descriptor, &in_set);
			FD_SET(d->descriptor, &out_set);
			FD_SET(d->descriptor, &exc_set);
		}

		if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0) {
			perror("Game_loop: select: poll");
			exit(1);
		}

		/*
		 * New connection?
		 */
		if (FD_ISSET(control, &in_set))
			init_descriptor(control);

		/*
		 * Kick out the freaky folks.
		 * Kyndig: Get rid of idlers as well
		 */
		for (d = descriptor_list; d != NULL; d = d_next) {
			d_next = d->next;

			d->idle++;
			if (FD_ISSET(d->descriptor, &exc_set)) {
				FD_CLR(d->descriptor, &in_set);
				FD_CLR(d->descriptor, &out_set);
				if (d->character && d->character->level > 1)
					save_char_obj(d->character);
				d->outtop = 0;
				close_socket(d);
			} else
			if ((!d->character && d->idle > 50)             /* 10 seconds */
			    || d->idle > 28800) {                       /* 2 hrs  */
				write_to_descriptor(d->descriptor, "Idle.\n\r", 0);
				d->outtop = 0;
				close_socket(d);

				continue;
			}
		}

		/*
		 * Process input.
		 */
		for (d = descriptor_list; d != NULL; d = d_next) {
			d_next = d->next;
			d->fcommand = FALSE;

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

			if (d->character != NULL && d->character->daze > 0)
				--d->character->daze;

			if (d->character != NULL && d->character->wait > 0) {
				--d->character->wait;
				continue;
			}

			read_from_buffer(d);
			if (d->incomm[0] != '\0') {
				d->fcommand = TRUE;
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


		/*
		 * Autonomous game motion.
		 */
		update_handler();


		/*
		 * Output.
		 */
		for (d = descriptor_list; d != NULL; d = d_next) {
			d_next = d->next;

			if ((d->fcommand || d->outtop > 0)
			    && FD_ISSET(d->descriptor, &out_set)) {
				if (!process_output(d, TRUE)) {
					if (d->character != NULL && d->character->level > 1)
						save_char_obj(d->character);
					d->outtop = 0;
					close_socket(d);
				}
			}
		}


#if !defined(WIN32)
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
					exit(1);
				}
			}
		}

		gettimeofday(&last_time, NULL);
		current_time = (time_t)last_time.tv_sec;
#else
		gettimeofday(&last_time, NULL);
		_ftime(&temp_time);
		last_time.tv_usec = temp_time.millitm;
		current_time = (time_t)last_time.tv_sec;

		times_up = 0;
		while (times_up == 0) {
			gettimeofday(&end_time, NULL);
			_ftime(&temp_time);
			end_time.tv_usec = temp_time.millitm;

			if ((wait_time = (int)(1000 * (double)(((int)end_time.tv_sec - (int)start_time.tv_sec)
							       + ((double)((int)end_time.tv_usec - (int)start_time.tv_usec) / 1000.0))))
			    >= (double)(1000 / PULSE_PER_SECOND)) {
				times_up = 1;
			} else {
				Sleep((int)((double)(1000 / PULSE_PER_SECOND) - (double)wait_time));
				times_up = 1;
			}
		}
#endif
	}

	return;
}
#endif



#if defined(unix) || defined(WIN32)
void init_descriptor(int control)
{
	DESCRIPTOR_DATA *dnew;
	struct sockaddr_in sock;
	struct hostent *from;
	char buf[MSL];
	int desc;
	socklen_t size;

	size = sizeof(sock);
	getsockname(control, (struct sockaddr *)&sock, &size);
	if ((desc = accept(control, (struct sockaddr *)&sock, &size)) < 0) {
		perror("New_descriptor: accept");
		return;
	}

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

#if defined(WIN32)
	if (ioctlsocket(desc, FIONBIO, &scmd) == -1) {
		perror("new_descriptor: ioctlsocket: FIONBIO");
		return;
	}
#else
	if (fcntl(desc, F_SETFL, FNDELAY) == -1) {
		perror("new_descriptor: fcntl: FNDELAY");
		return;
	}
#endif

	/*
	 * Cons a new descriptor.
	 */
/*	dnew				= new_descriptor();
 *
 *      dnew->descriptor	= desc;
 *      dnew->connected		= CON_GET_ANSI;
 *      dnew->showstr_head	= NULL;
 *      dnew->showstr_point     = NULL;
 *      dnew->idle              = 0;
 *      dnew->outsize		= 2000;
 *      dnew->outbuf		= alloc_mem(dnew->outsize);
 *      dnew->ed_data		= NULL;
 *      dnew->ed_string		= NULL;
 *      dnew->editor		= 0;	*/
/*	dnew->character		= NULL;*/

	dnew = new_descriptor(); /* new_descriptor now also allocates things */
	dnew->descriptor = desc;

	size = sizeof(sock);
	if (getpeername(desc, (struct sockaddr *)&sock, &size) < 0) {
		perror("new_descriptor: getpeername");
		dnew->host = str_dup("(unknown)");
	} else {
		/*
		 * Would be nice to use inet_ntoa here but it takes a struct arg,
		 * which ain't very compatible between gcc and system libraries.
		 */
		int addr;

		addr = ntohl(sock.sin_addr.s_addr);
		sprintf(buf, "%d.%d.%d.%d",
			(addr >> 24) & 0xFF,
			(addr >> 16) & 0xFF,
			(addr >> 8) & 0xFF,
			(addr) & 0xFF);
		sprintf(log_buf, "Sock.sinaddr:  %s", buf);

		log_string(log_buf);
		from = NULL;
		if (FALSE) /*!is_host_exception(buf))*/
			from = gethostbyaddr((char *)&sock.sin_addr, sizeof(sock.sin_addr), AF_INET);
		dnew->host = str_dup(from ? from->h_name : buf);
	}

/*
 * Swiftest: I added the following to ban sites.  I don't
 * endorse banning of sites, but Copper has few descriptors now
 * and some people from certain sites keep abusing access by
 * using automated 'autodialers' and leaving connections hanging.
 *
 * Furey: added suffix check by request of Nickel of HiddenWorlds.
 */
	if (check_ban(dnew->host, BAN_ALL)) {
		write_to_descriptor(desc, "Your site has been banned from this mud.\n\r", 0);
		close(desc);
		free_descriptor(dnew);
		return;
	}
/*
 * Init descriptor data.
 */
	dnew->next = descriptor_list;
	descriptor_list = dnew;

	if (port == 7779)
		write_to_descriptor(dnew->descriptor, "\n\r\n\r.---------------------------------.\n\r| Bad Trip MUD -- Test/Build Port |\n\r| For the main MUD, use port 7778 |\n\r.---------------------------------.\n\r\n\r", 0);

	write_to_descriptor(dnew->descriptor, "Ansi intro screen?(y/n) \n\r", 0);

	return;
}
#endif



void close_socket(DESCRIPTOR_DATA *dclose)
{
	CHAR_DATA *ch;

	if (dclose->outtop > 0)
		process_output(dclose, FALSE);

	if (dclose->snoop_by != NULL)
		write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r", 0);

	{
		DESCRIPTOR_DATA *d;

		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->snoop_by == dclose)
				d->snoop_by = NULL;
	}

	if ((ch = dclose->character) != NULL) {
		sprintf(log_buf, "Closing link to %s.", ch->name);
		log_string(log_buf);
		if (dclose->connected == CON_PLAYING) {
			act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
			wiznet("Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
			SET_BIT(ch->act, PLR_LINKDEAD);
			ch->desc = NULL;
		} else {
			free_char(dclose->original ? dclose->original : dclose->character);
		}
	}

	if (d_next == dclose)
		d_next = d_next->next;

	if (dclose == descriptor_list) {
		descriptor_list = descriptor_list->next;
	} else {
		DESCRIPTOR_DATA *d;

		for (d = descriptor_list; d && d->next != dclose; d = d->next)
			;

		if (d != NULL)
			d->next = dclose->next;
		else
			bug("Close_socket: dclose not found.", 0);
	}

	close(dclose->descriptor);
	free_descriptor(dclose);
	return;
}


bool read_from_descriptor(DESCRIPTOR_DATA *d)
{
	int iStart;

/* Hold horses if pending command already. */
	if (d->incomm[0] != '\0')
		return TRUE;

/* Check for overflow. */
	iStart = (int)strlen(d->inbuf);

	if (iStart >= (int)(sizeof(d->inbuf) - 10)) {
		sprintf(log_buf, "%s input overflow!", d->host);
		log_string(log_buf);
		write_to_descriptor(d->descriptor, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
		return FALSE;
	}


#if defined(MSDOS) || defined(unix) || defined(WIN32)
	for (;; ) {
		int nRead;

		nRead = read(d->descriptor, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart);

		if (nRead > 0) {
			iStart += nRead;
			if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
				break;
		} else if (nRead == 0) {
			log_string("EOF encountered on read.");
			return FALSE;
		}
#if defined(WIN32)
		else if (WSAGetLastError() == WSAEWOULDBLOCK) {
			break;
		}
#else
		else if (errno == EWOULDBLOCK) {
			break;
		}
#endif
		else {
			perror("Read_from_descriptor");
			return FALSE;
		}
	}
#endif

	d->inbuf[iStart] = '\0';
	return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA *d)
{
	int i, j, k;

/*
 * Hold horses if pending command already.
 */

	if (d->incomm[0] != '\0')
		return;

/*
 * Look for at least one new line.
 */
	for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
		if (d->inbuf[i] == '\0')
			return;

/*
 * Canonical input processing.
 */
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
/*
 * Finish off the line.
 */
	if (k == 0)
		d->incomm[k++] = ' ';
	d->incomm[k] = '\0';

/*
 * Deal with bozos with #repeat 1000 ...
 */

	if (k > 1 || d->incomm[0] == '!') {
		if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast)) {
			d->repeat = 0;
		} else {
			if (++d->repeat >= 50 && !IS_IMMORTAL(d->character)) {
				send_to_char("`@Acid-Fiend-1 tells you '`tlay off the spam Bucky!`@'`7\n\r", d->character);
				sprintf(log_buf, "%s input spamming!", d->host);
				WAIT_STATE(d->character, 25);
				log_string(log_buf);
				wiznet("Spam spam spam $N spam spam spam spam spam!",
				       d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
				if (d->incomm[0] == '!')
					wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
				else
					wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0,
					       get_trust(d->character));

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

/*
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt)
{
	extern bool merc_down;

/*
 * Bust a prompt.
 */
	if (!merc_down) {
		if (d->showstr_point) {
			write_to_buffer(d, "\n\r[Hit Return to continue]\n\r\n\r", 0);
		} else if (fPrompt && d->ed_string && d->connected == CON_PLAYING) {
			write_to_buffer(d, "> ", 2);
		} else if (fPrompt && !merc_down && d->connected == CON_PLAYING) {
			CHAR_DATA *ch;
			CHAR_DATA *victim;

			ch = d->character;

			/* battle prompt */
			if ((victim = ch->fighting) != NULL && can_see(ch, victim))
				show_damage_display(ch, victim);
				/*
				 * char buf[MSL];
				 * int percent;
				 *
				 * if(victim->max_hit > 0)
				 * {
				 *      percent = victim->hit * 100 / victim->max_hit;
				 * }
				 * else
				 * {
				 *      percent = -1;
				 * }
				 *
				 * strcpy(buf, PERS(victim, ch));
				 *
				 *
				 * if(percent >= 100)
				 * {
				 *      strcat(buf, " `Dis in excellent condition.``\n\r");
				 * }
				 * else if(percent >= 90)
				 * {
				 *      strcat(buf, " `Dhas a few scratches.``\n\r");
				 * }
				 * else if(percent >= 75)
				 * {
				 *      strcat(buf, " `Dhas some small wounds and bruises.``\n\r");
				 * }
				 * else if(percent >= 50)
				 * {
				 *      strcat(buf, " `Dhas quite a few wounds.``\n\r");
				 * }
				 * else if(percent >= 30)
				 * {
				 *      strcat(buf, " `Dhas some big nasty wounds and scratches.``\n\r");
				 * }
				 * else if(percent >= 15)
				 * {
				 *      strcat(buf, " `Dlooks pretty hurt.``\n\r");
				 * }
				 * else if(percent >= 0)
				 * {
				 *      strcat(buf, " `Dis in awful condition.``\n\r");
				 * }
				 * else
				 * {
				 *      strcat(buf, " `Dis bleeding to death.``\n\r");
				 * }
				 *
				 * buf[0] = UPPER(buf[0]);
				 * send_to_char(buf, ch);
				 */


			ch = CH(d);
			if (!IS_SET(ch->comm, COMM_COMPACT))
				write_to_buffer(d, "\n\r", 2);


			if (IS_SET(ch->comm, COMM_PROMPT))
				bust_a_prompt(d->character);
		}
	}
/*
 * Short-circuit if nothing to write.
 */
	if (d->outtop == 0)
		return TRUE;

/*
 * Snoop-o-rama.
 */
	if (d->snoop_by != NULL) {
		if (d->character != NULL)
			write_to_buffer(d->snoop_by, d->character->name, 0);
		write_to_buffer(d->snoop_by, "> ", 2);
		write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
	}

/*
 * OS-dependent output.
 */
	if (!write_to_descriptor(d->descriptor, d->outbuf, d->outtop)) {
		d->outtop = 0;
		return FALSE;
	} else {
		d->outtop = 0;
		return TRUE;
	}
}


/*
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

	if (IS_SET(ch->comm2, COMM2_AFK)) {
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
			found = FALSE;
			doors[0] = '\0';
			for (door = 0; door < 6; door++) {
				if ((pexit = ch->in_room->exit[door]) != NULL
				    && pexit->u1.to_room != NULL
				    && (can_see_room(ch, pexit->u1.to_room)
					|| (IS_AFFECTED(ch, AFF_INFRARED)
					    && !IS_AFFECTED(ch, AFF_BLIND)))) {
					found = TRUE;
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



/*
 * Append onto an output buffer.
 */
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length)
{
	if (d == NULL)
		return;
/*
 * Find length in case caller didn't.
 */
	if (length <= 0)
		length = (int)strlen(txt);

/*
 * Initial \n\r if needed.
 */
	if (d->outtop == 0 && !d->fcommand) {
		d->outbuf[0] = '\n';
		d->outbuf[1] = '\r';
		d->outtop = 2;
	}

/*
 * Expand the buffer as needed.
 */
	while (d->outtop + (int)strlen(txt) >= d->outsize) {
		char *outbuf;

		if (d->outsize >= 32000) {
			bug("Buffer overflow. Closing.\n\r", 0);
			close_socket(d);
			return;
		}
		outbuf = alloc_mem((unsigned int)(2 * d->outsize));
		strncpy(outbuf, d->outbuf, (size_t)d->outtop);
		free_mem(d->outbuf, (unsigned int)d->outsize);
		d->outbuf = outbuf;
		d->outsize *= 2;
	}

/*
 * Copy.
 */
/*  strcpy(d->outbuf + d->outtop, txt);  growl ..   */
	strncpy(d->outbuf + d->outtop, txt, (size_t)length);
	d->outtop += length;
	return;
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks(like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor(int desc, char *txt, int length)
{
	int iStart;
	int nWrite;
	int nBlock;


	if (length <= 0)
		length = (int)strlen(txt);

	for (iStart = 0; iStart < length; iStart += nWrite) {
		nBlock = UMIN(length - iStart, 8192);
#if defined(WIN32)
		if ((nWrite = send(desc, txt + iStart, nBlock, 0)) < 0) {
			perror("Write_to_descriptor");
			return FALSE;
		}
#else
		if ((nWrite = (int)write(desc, txt + iStart, (size_t)nBlock)) < 0) {
			perror("Write_to_descriptor");
			return FALSE;
		}
#endif
	}

	return TRUE;
}



#define CNUM(x) ch->pcdata->x
void process_color(CHAR_DATA *ch, char a)
{
	byte c = (byte)0;
	bool real = TRUE;

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
		real = FALSE;
		break;
	case 'z':
		write_to_buffer(ch->desc, "`", 0);
		real = FALSE;
		break;
	case '-':
		write_to_buffer(ch->desc, "~", 0);
		real = FALSE;
		break;
	case ';':
		write_to_buffer(ch->desc, "\033", 0);
		real = FALSE;
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
		real = FALSE;
		break;
	case 'B':
		write_to_buffer(ch->desc, "\a", 0);
		real = FALSE;
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
	int length, l, c = 0, foo, curlen = 0;

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

				if ((is_affected(ch, skill_lookup("bad trip")))
				    && (c >= 10)
				    && (ch->color == 1)) {
					foo = number_range(0, 13);

					switch (foo) {
					case 0:
						write_to_buffer(ch->desc, "[0;31m", 7);
						break;
					case 1:
						write_to_buffer(ch->desc, "[0;32m", 7);
						break;
					case 2:
						write_to_buffer(ch->desc, "[0;33m", 7);
						break;
					case 3:
						write_to_buffer(ch->desc, "[0;34m", 7);
						break;
					case 4:
						write_to_buffer(ch->desc, "[0;35m", 7);
						break;
					case 5:
						write_to_buffer(ch->desc, "[0;36m", 7);
						break;
					case 6:
						write_to_buffer(ch->desc, "[0;37m", 7);
						break;
					case 7:
						write_to_buffer(ch->desc, "[1;31m", 7);
						break;
					case 8:
						write_to_buffer(ch->desc, "[1;32m", 7);
						break;
					case 9:
						write_to_buffer(ch->desc, "[1;33m", 7);
						break;
					case 10:
						write_to_buffer(ch->desc, "[1;34m", 7);
						break;
					case 11:
						write_to_buffer(ch->desc, "[1;35m", 7);
						break;
					case 12:
						write_to_buffer(ch->desc, "[1;36m", 7);
						break;
					default:
						write_to_buffer(ch->desc, "[1;37m", 7);
						break;
					}
					c = 0;
				}
			}

			if (l)
				write_to_buffer(ch->desc, b, l);

			if (*a != '\0') {
				a++;
				curlen++;
				if (curlen < length && ch->color) {
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

int espBroadcastAndCheck(CHAR_DATA *ch, char *CanSeeAct, char *CanTSeeAct)
{
	DESCRIPTOR_DATA *d;
	int esper = 0;

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING)
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (wch->linked == ch) {
			esper = 1;
			if (CanSeeAct != NULL && CanTSeeAct != NULL) {
				if (can_see(wch, ch))
					send_to_char(CanSeeAct, wch);
				else
					send_to_char(CanTSeeAct, wch);
			}
		}
	}

	return esper;
}


void act(const char *format, CHAR_DATA *ch, const void *arg1,
	 const void *arg2, int type)
{
	/* to be compatible with older code */
	act_new(format, ch, arg1, arg2, type, POS_RESTING, TRUE);
}

void act_new(const char *format, CHAR_DATA *ch, const void *arg1,
	     const void *arg2, int type, int min_pos, bool mob_trigger)
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
			bug("Act: null vch with TO_VICT.", 0);
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
				bug("Act: missing arg2 for code %d.", (int)*str);
				i = " <@@@> ";
			} else {
				switch (*str) {
				default:
					bug("Act: bad code %d.", (int)*str);
					i = " <@@@> ";
					break;
				/* Thx alex for 't' idea */
				case 't':
					if (arg1) i = (char *)arg1;
					else bug("Act: bad code $t for 'arg1'", 0);
					break;
				case 'T':
					if (arg2) i = (char *)arg2;
					else bug("Act: bad code $T for 'arg2'", 0);
					break;
				case 'n':
					if (ch && to) i = PERS(ch, to);
					else bug("Act: bad code $n for 'ch' or 'to'", 0);
					break;
				case 'N':
					if (vch && to) i = PERS(vch, to);
					else bug("Act: bad code $N for 'vch' or 'to'", 0);
					break;
				case 'e':
					if (ch) i = he_she[URANGE(0, ch->sex, 2)];
					else bug("Act: bad code $e for 'ch'", 0);
					break;
				case 'E':
					if (vch) i = he_she[URANGE(0, vch->sex, 2)];
					else bug("Act: bad code $E for 'vch'", 0);
					break;
				case 'm':
					if (ch) i = him_her[URANGE(0, ch->sex, 2)];
					else bug("Act: bad code $m for 'ch'", 0);
					break;
				case 'M':
					if (vch) i = him_her[URANGE(0, vch->sex, 2)];
					else bug("Act: bad code $M for 'vch'", 0);
					break;
				case 's':
					if (ch) i = his_her[URANGE(0, ch->sex, 2)];
					else bug("Act: bad code $s for 'ch'", 0);
					break;
				case 'S':
					if (vch) i = his_her[URANGE(0, vch->sex, 2)];
					else bug("Act: bad code $S for 'vch'", 0);
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

/***************************************************************************
*	printf_bug
***************************************************************************/
void printf_bug(char *fmt, ...)
{
	char buf[2 * MSL];

	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	bug(buf, 0);
}


/***************************************************************************
*	printf_log
***************************************************************************/
void printf_log(char *fmt, ...)
{
	char buf[2 * MSL];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	log_string(buf);
}


/***************************************************************************
*	set_wait
*
*	set the wait time
***************************************************************************/
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


/***************************************************************************
*	set_daze
*
*	set the wait time
***************************************************************************/
void set_daze(CHAR_DATA *ch, int len)
{
	int mod;

	if (ch == NULL || IS_IMMORTAL(ch))
		return;

	/* length is decreased by DEX maxxed out at 1/2 len + 1*/
	mod = get_curr_stat(ch, STAT_CON) / 100;
	mod = UMIN((len / 2) + 1, mod);


	/* subtract the modifier */
	len -= mod;

	/* set the wait time */
	ch->daze = UMAX(ch->daze, len);
}

void sig_handler(int sig)
{
	switch (sig) {
	case SIGBUS:
		bug("Sig handler SIGBUS.", 0);
		do_auto_shutdown();
		break;
	case SIGTERM:
		bug("Sig handler SIGTERM.", 0);
		do_auto_shutdown();
		break;
	case SIGABRT:
		bug("Sig handler SIGABRT", 0);
		do_auto_shutdown();
	case SIGSEGV:
		bug("Sig handler SIGSEGV", 0);
		do_auto_shutdown();
		break;
	}
}

void init_signals()
{
	signal(SIGBUS, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGABRT, sig_handler);
	signal(SIGSEGV, sig_handler);
}
