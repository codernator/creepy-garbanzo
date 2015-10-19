#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h> //strncpy
#include <sys/time.h>
#include "remote.h"


/** exports */
struct system_state globalSystemState = {
    .current_time = 0,
    .tickset = 0,
    .merc_down = false,
    .boot_time = "",
    .wizlock = false,
    .newlock = false,
    .log_all = false,
    .last_command = "",
    .copyover_tick_counter = -1,
    .reboot_tick_counter = -1
};


/** imports */
extern void psignal(int signum, const char *message);

/** comm.c */
extern void auto_shutdown();
extern void game_loop(int port, int control);
/** ~comm.c */

/** copyover.c */
extern void copyover_recover(void);
/** ~copyover.c */


/** locals */
static void init_signals();
static void init_time(struct system_state *);
static void sig_handler(int sig);
static volatile sig_atomic_t fatal_error_in_progress = 0;


int main(int argc, char **argv)
{
    bool recovering = false;
    int port, control = 0;

    init_signals();

    /** Get the port number. */
    port = 7778;
    if (argc > 1) {
	if (!is_number(argv[1])) {
	    fprintf(stderr, "Usage: %s [port #]\n", argv[0]);
	    return 0;
	} else if ((port = atoi(argv[1])) <= 1024) {
	    fprintf(stderr, "Port number must be above 1024.\n");
	    return 0;
	}

	/* Are we recovering from a copyover? */
	if ((argv[2] != NULL) && (argv[2][0] != '\0')) {
	    recovering = true;
	    control = atoi(argv[3]);
	} else {
	    recovering = false;
	}
    }

    init_time(&globalSystemState);
    boot_db();

    /** Run the game. */
    if (!recovering) {
	control = remote_listen(port);
    }

    log_string("BT is ready to rock on port %d.", port);

    if (recovering) {
	copyover_recover();
    }

    game_loop(port, control);
    remote_deafen(control);

    log_string("Normal termination of game.");
    return 0;
}

void init_signals()
{
#ifdef S_SPLINT_S
#define SIGBUS 0
#endif
    //TODO - more sophisticated signal handling.
    (void)signal(SIGBUS, sig_handler);
    (void)signal(SIGTERM, sig_handler);
    (void)signal(SIGABRT, sig_handler);
    (void)signal(SIGSEGV, sig_handler);
}


void init_time(struct system_state *system_state) 
{
    time_t now_time;

    (void)time(&now_time);
    system_state->current_time = now_time;
    memset(system_state->boot_time, 0, FRIENDLYTIME_BUFSIZE);
    strncpy(system_state->boot_time, ctime(&now_time), FRIENDLYTIME_BUFSIZE-1);
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
    if (fatal_error_in_progress != 0) {
	(void)raise(sig);
    }

    fatal_error_in_progress = 1;
    psignal(sig, "Auto shutdown invoked.");
    log_bug("Critical signal received %d", sig);
    log_to(LOG_SINK_LASTCMD, "%s", globalSystemState.last_command);
    auto_shutdown();

    /* Now reraise the signal. We reactivate the signal’s default handling, which is to 
     * terminate the process. We could just call exit or abort,  but reraising the signal 
     * sets the return status from the process correctly, and, more importantly, gives us
     * a core dump.
     */
    (void)signal(sig, SIG_DFL);
    (void)raise(sig);
}

