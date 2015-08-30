#include "merc.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>


static void write_file(const char *fileName, char *mode, const char *fmt, ...);



void append_file(CHAR_DATA *ch, const char *file, const char *str)
{
    FILE *fp;

    DENY_NPC(ch);
    if (str == NULL || str[0] == '\0')
	return;

    if ((fp = fopen(file, "a")) == NULL) {
	perror(file);
	send_to_char("Sorry, the needed file is inaccessible.", ch);
	return;
    }

    fprintf(fp, "[%5ld] %s: %s\n", ch->in_room == NULL ? 0 : ch->in_room->vnum, ch->name, str);
    fclose(fp);
}

void log_bug(const char *fmt, ...)
{
    static char buf[MSL];
    char *strtime;
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    strtime = ctime(&globalSystemState.current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, buf);
}

/*
 * Writes a string to the log.
 */
void log_string(const char *fmt, ...)
{
    static char buf[MSL];
    char *strtime;
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    strtime = ctime(&globalSystemState.current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, buf);
}


void log_to(int log, char username[], const char *fmt, ...)
{
    char *strtime;
    static char buf[MSL];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    strtime = ctime(&globalSystemState.current_time);
    strtime[strlen(strtime) - 1] = '\0';

    switch (log)
    {
	case LOG_SINK_LASTCMD:
	    write_file(LAST_COMMANDS, "a+", "[%s] %s\n", strtime, buf);
	    break;
	case LOG_SINK_ALWAYS:
	case LOG_SINK_ALL:
	    write_file(LOG_ALWAYS_FILE, "a+", "[%s] %s\n", strtime, buf);
	    break;
	case LOG_SINK_PLAYER:
	    if (username[0] == '\0') {
		log_bug("ERROR- log_to: Log Type 'Player' requires a username!");
	    } else {
		static char log_file_name[2*MIL];
		(void)snprintf(log_file_name, 2*MIL, LOG_PLAYER_FILE, username);
		write_file(log_file_name, "a+", "[%s] %s\n", strtime, buf);
	    }
	    break;
	default:
	    log_bug("ERROR - log_to: Log type '%d' does not exist.", log);
	    break;
    }
}

static void write_file(const char *fileName, char *mode, const char *fmt, ...)
{
    static char buf[MSL];
    FILE *thisFile;
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    if ((thisFile = fopen(fileName, mode)) == NULL) {
	log_bug("Error - log_to - cannot open file '%s' in mode %s", fileName, mode);
	return;
    }

    fprintf(thisFile, "%s", buf);
    fclose(thisFile);
}

