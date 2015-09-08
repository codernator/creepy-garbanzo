#include "merc.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

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

    if (fclose(fp) != 0) {
        perror(file);
        ABORT;
    }
}

void log_bug(const char *fmt, ...)
{
    static char buf[MSL];
    va_list args;

    va_start(args, fmt);
    (void)vsnprintf(buf, MSL, fmt, args);
    va_end(args);

    fprintf(stderr, "%s :: %s\n", ctime(&globalSystemState.current_time), buf);
}

/*
 * Writes a string to the log.
 */
void log_string(const char *fmt, ...)
{
    static char buf[MSL];
    va_list args;

    va_start(args, fmt);
    (void)vsnprintf(buf, MSL, fmt, args);
    va_end(args);

    fprintf(stderr, "%s :: %s\n", ctime(&globalSystemState.current_time), buf);
}


void log_to(int log, char username[], const char *fmt, ...)
{
    static char buf[MSL];
    va_list args;

    va_start(args, fmt);
    (void)vsnprintf(buf, MSL, fmt, args);
    va_end(args);

    switch (log)
    {
        case LOG_SINK_LASTCMD:
            write_file(LAST_COMMANDS, "a+", "[%s] %s\n", ctime(&globalSystemState.current_time), buf);
            break;
        case LOG_SINK_ALWAYS:
        case LOG_SINK_ALL:
            write_file(LOG_ALWAYS_FILE, "a+", "[%s] %s\n", ctime(&globalSystemState.current_time), buf);
            break;
        case LOG_SINK_PLAYER:
            if (username[0] == '\0') {
                log_bug("ERROR- log_to: Log Type 'Player' requires a username!");
            } else {
                static char log_file_name[2*MIL];
                (void)snprintf(log_file_name, 2*MIL, LOG_PLAYER_FILE, username);
                write_file(log_file_name, "a+", "[%s] %s\n", ctime(&globalSystemState.current_time), buf);
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
    (void)vsnprintf(buf, MSL, fmt, args);
    va_end(args);

    if ((thisFile = fopen(fileName, mode)) == NULL) {
        perror(fileName);
        ABORT;
    }

    fprintf(thisFile, "%s", buf);

    if (fclose(thisFile) != 0) {
        perror(fileName);
        ABORT;
    }
}

