#include "merc.h"
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include "telnet.h"


extern int gettimeofday(struct timeval *tp, struct timezone *tzp);

/** recycle.h */
extern DESCRIPTOR_DATA * new_descriptor(void);
extern void free_descriptor(DESCRIPTOR_DATA * d);
/** ~recycle.h */


typedef void handle_new_connection(int control);


void init_time(SYSTEM_STATE *system_state);
bool read_from_descriptor(DESCRIPTOR_DATA *d);
bool write_to_descriptor(int desc, char *txt, int length);
int listen_port(int port);
void deafen_port(int listen_control);
void init_descriptor(int control);


void init_time(SYSTEM_STATE *system_state) 
{
	struct timeval now_time;
    gettimeofday(&now_time, NULL);
    system_state->current_time = (time_t)now_time.tv_sec;
    strcpy(system_state->boot_time, (char *)ctime(&system_state->current_time));
}

void deafen_port(int listen_control)
{
    close(listen_control);
}

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

	if (fcntl(desc, F_SETFL, FNDELAY) == -1) {
		perror("new_descriptor: fcntl: FNDELAY");
		return;
	}

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
		log_string("Sock.sinaddr: %d.%d.%d.%d", (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, (addr) & 0xFF);
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
    descriptor_list_add(dnew);
	write_to_descriptor(dnew->descriptor, "Ansi intro screen?(y/n) \n\r", 0);
}

int listen_port(int port)
{
	static struct sockaddr_in sa_zero;
	struct sockaddr_in sa;
	int x = 1;
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Init_socket: socket");
        raise(SIGABRT);
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) < 0) {
		perror("Init_socket: SO_REUSEADDR");
		close(fd);
        raise(SIGABRT);
	}

#if defined(SO_DONTLINGER) && !defined(SYSV)
	{
		struct linger ld;

		ld.l_onoff = 1;
		ld.l_linger = 1000;

		if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, (char *)&ld, sizeof(ld)) < 0) {
			perror("Init_socket: SO_DONTLINGER");
			close(fd);
            raise(SIGABRT);
		}
	}
#endif

	sa = sa_zero;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("Init socket: bind");
		close(fd);
        raise(SIGABRT);
	}


	if (listen(fd, 3) < 0) {
		perror("Init socket: listen");
		close(fd);
        raise(SIGABRT);
	}

	return fd;
}

bool read_from_descriptor(DESCRIPTOR_DATA *d)
{
	int iStart;

/* Hold horses if pending command already. */
	if (d->incomm[0] != '\0')
		return true;

/* Check for overflow. */
	iStart = (int)strlen(d->inbuf);

	if (iStart >= (int)(sizeof(d->inbuf) - 10)) {
		log_string("%s input overflow!", d->host);
		write_to_descriptor(d->descriptor, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
		return false;
	}


	for (;; ) {
		int nRead;

		nRead = read(d->descriptor, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart);

		if (nRead > 0) {
			iStart += nRead;
			if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
				break;
		} else if (nRead == 0) {
			log_string("EOF encountered on read.");
			return false;
		}
		else if (errno == EWOULDBLOCK) {
			break;
		}
		else {
			perror("Read_from_descriptor");
			return false;
		}
	}

	d->inbuf[iStart] = '\0';
	return true;
}

/**
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
		if ((nWrite = (int)write(desc, txt + iStart, (size_t)nBlock)) < 0) {
			perror("Write_to_descriptor");
			return false;
		}
	}

	return true;
}
