#include "sysinternals.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>



#define MAX_FILENAME_LENGTH 1024

static /*@shared@*/const char *get_log_time();


void log_bug(const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "[BUG] %s ::", get_log_time());
    va_start(args, fmt);
    (void)vfprintf(stderr, fmt, args);
    va_end(args);
    //:(
    fprintf(stderr, "%s", "\n");
}

void log_string(const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "[INFO] %s ::", get_log_time());
    va_start(args, fmt);
    (void)vfprintf(stderr, fmt, args);
    va_end(args);
    //:(
    fprintf(stderr, "%s", "\n");
}

void log_to_player(const char username[], const char *fmt, ...)
{
    static char fileName[MAX_FILENAME_LENGTH];
    va_list args;
    FILE *outfile;

    if (username == NULL || username[0] == '\0') {
	log_bug("ERROR- log_to: Log Type 'Player' requires a username!");
	return;
    } else {
	(void)snprintf(fileName, MAX_FILENAME_LENGTH - 1, LOG_PLAYER_FILE, username);
    }

    if ((outfile = fopen(fileName, "a+")) == NULL) {
        perror(fileName);
	return;
    }

    fprintf(outfile, "%s ::", get_log_time());
    va_start(args, fmt);
    (void)vfprintf(outfile, fmt, args);
    va_end(args);
    //:(
    fprintf(outfile, "%s", "\n");

    if (fclose(outfile) != 0) {
        perror(fileName);
    }
}

void log_to(int log, const char *fmt, ...)
{
    static /*@observer@*/const char *fileName;
    va_list args;
    FILE *outfile;

    switch (log)
    {
        case LOG_SINK_LASTCMD:
            fileName = LOG_LAST_COMMANDS_FILE;
            break;
        case LOG_SINK_ALWAYS:
        case LOG_SINK_ALL:
	    fileName = LOG_ALWAYS_FILE;
            break;
	case LOG_SINK_BUG:
	    fileName = LOG_BUG_FILE;
	    break;
	case LOG_SINK_TYPO:
	    fileName = LOG_TYPO_FILE;
	    break;
	case LOG_SINK_SHUTDOWN:
	    fileName = LOG_SHUTDOWN_FILE;
	    break;
        default:
            log_bug("ERROR - log_to: Log type '%d' does not exist.", log);
	    return;
    }

    if ((outfile = fopen(fileName, "a+")) == NULL) {
        perror(fileName);
	return;
    }

    fprintf(outfile, "%s ::", get_log_time());
    va_start(args, fmt);
    (void)vfprintf(outfile, fmt, args);
    va_end(args);
    //:(
    fprintf(outfile, "%s", "\n");

    if (fclose(outfile) != 0) {
        perror(fileName);
    }
}

const char *get_log_time()
{
    static char buf[30];
    struct tm *broken;
    time_t now;

    now = time(NULL);
    broken = localtime(&now);
    (void)strftime(buf, 30, "%F %T", broken);

    return buf;
}

