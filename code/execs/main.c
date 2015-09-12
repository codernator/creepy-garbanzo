#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "socketio.h"


/** exports */
SYSTEM_STATE globalSystemState = {
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

/** comm.c */
extern void game_loop(int port, int control);
extern void copyover_recover(void);
extern void sig_handler(int sig);
/** ~comm.c */



/** locals */
static void init_signals();


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
	control = listen_port(port);
    }

    log_string("BT is ready to rock on port %d.", port);

    if (recovering) {
	copyover_recover();
    }

    game_loop(port, control);
    deafen_port(control);

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

