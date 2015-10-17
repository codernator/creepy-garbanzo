#include "merc.h"
#include "remote.h"
#include <stdio.h>


/** imports */
extern int listen_port; /* comm.c */
extern int listen_control; /* comm.c */
//extern pid_t waitpid(pid_t pid, int *status, int options);
//extern pid_t fork(void);
//extern int kill(pid_t pid, int sig);
//extern int pipe(int filedes[2]);
//extern int dup2(int oldfd, int newfd);
extern int execl(const char *path, const char *arg, ...);
extern DECLARE_DO_FUN(do_look);
extern DECLARE_DO_FUN(do_stand);


/** exports */
bool copyover();
void copyover_recover(void);

bool copyover()
{
    FILE *fp;
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
	    remote_write(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
	    close_socket(d, false, false);  /* throw'em out */
	} else {
	    fprintf(fp, "%d %s %s\n", d->descriptor, och->name, d->host);

	    if (och->level == 1) {
		remote_write(d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\n\r", 0);
		advance_level(och, 1);
	    }
	    do_stand(och, "");
	    save_char_obj(och);
	}
    }

    fprintf(fp, "-1\n");
    fclose(fp);

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
    char host[MAX_STRING_LENGTH];
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
	if (!remote_write(desc, "", 0)) {
	    remote_disconnect(desc);  /* nope */
	    continue;
	}

	d = descriptor_new(desc);
	d->host = str_dup(host);
	d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */


	/* Now, find the pfile */

	fOld = load_char_obj(d, name);

	if (!fOld) { /* Player file not found?! */
	    remote_write(desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
	    close_socket(d, false, false);
	} else { /* ok! */
	    /*			remote_write (desc, "\n\rCopyover recovery complete.\n\r",0);*/

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

