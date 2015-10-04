#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h> //strncpy
#include <sys/time.h>
#include "recycle.h"


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


extern char *password_encrypt(const char *plain);


static CHAR_DATA *create();
static PC_DATA *create_pcdata();

static char *name = "Codernator";
static char *empty = "";
static char *password = "creepy_garbanzo";
static char *prompt = "<%hhp %mm %vmv> ";

static time_t now_time;



int main(int argc, char **argv)
{
    CHAR_DATA *ch;
    (void)time(&now_time);
    globalSystemState.current_time = now_time;
    boot_db();
    
    ch = create();
    save_char_obj(ch);
    free_pcdata(ch->pcdata);
    free_char(ch);
}


CHAR_DATA *create()
{
    CHAR_DATA *ch = new_char();

    ch->pcdata = create_pcdata();

    ch->name = name;
    ch->id = 1;
    ch->short_descr = empty;
    ch->long_descr = empty;
    ch->description = empty;
    ch->prompt = prompt;
    ch->race = 1;
    ch->sex = 0;
    ch->class = 0;
    ch->level = MAX_LEVEL;
    ch->trust = MAX_LEVEL;
    ch->logon = now_time;
    ch->lines = 20;
    ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
    ch->hit = 100;
    ch->max_hit = 100;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->position = POS_STANDING;

    return ch;
}

PC_DATA *create_pcdata()
{
    PC_DATA *ch = new_pcdata();

    ch->pwd = password_encrypt(password);
    ch->security = 9;
    ch->deathcry = empty;
    ch->points = 100;
    ch->true_sex = 0;
    ch->last_level = MAX_LEVEL;
    ch->perm_hit = 1000;
    ch->perm_mana = 1000;
    ch->perm_move = 1000;
    ch->bamfin = empty;
    ch->bamfout = empty;
    ch->grestore_string = empty;
    ch->rrestore_string = empty;
    ch->who_thing = empty;
    ch->title = empty;

    return ch;
}
