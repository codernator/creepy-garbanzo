#include "merc.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void lastCommands(const char *str);
static void cmdLogAlways(const char *str);
static void cmdLogALL(const char *str);
static void cmdLogPlayer(const char *str, char username[]);
static char *logStamp(void);
static void write_file(const char *fileName, char *mode, char *text);

static int numCmds = 1;

void log_bug(const char *fmt, ...)
{
	char *strtime;
	char buf[MSL];

	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);


	strtime = ctime(&globalSystemState.current_time);
	strtime[strlen(strtime) - 1] = '\0';
	fprintf(stderr, "%s :: %s\n", strtime, buf);
	return;
}

void log_new(const char *log, const char *str, char username[])
{
	char buf[MSL];

	if (!strncmp(log, "LASTCMD", 8)) {
		lastCommands(str);
	} else if (!strncmp(log, "CMDALWAYS", 10)) {
		cmdLogAlways(str);
	} else if (!strncmp(log, "LOGALLCMD", 10)) {
		cmdLogALL(str);
	} else if (!strncmp(log, "LOGGEDPLR", 10)) {
		if (username[0] == '\0')
			log_string("ERROR- log_new: Log Type 'Player' requires a username!");
		else
			cmdLogPlayer(str, username);
	} else {
		sprintf(buf, "ERROR - log_new: Log type '%s' does not exist.", log);
		log_string(buf);
	}
	return;
}

static char *logStamp(void)
{
	char *strtime;

	strtime = ctime(&globalSystemState.current_time);
	strtime[strlen(strtime) - 1] = '\0';

	return strtime;
}

static void cmdLogAlways(const char *str)
{
	static char buf[LOG_BUF_LENGTH];
	(void)snprintf(buf, LOG_BUF_LENGTH, "[%s] %s\n", logStamp(), str);
	write_file(LOG_ALWAYS_FILE, "a+", buf);
}

static void cmdLogALL(const char *str)
{
	static char buf[LOG_BUF_LENGTH];
	(void)snprintf(buf, LOG_BUF_LENGTH, "[%s] %s\n", logStamp(), str);
	write_file(LOG_ALL_CMDS_FILE, "a+", buf);
}

static void cmdLogPlayer(const char *str, char username[])
{
	static char log_file_name[2*MIL];
	static char bff[LOG_BUF_LENGTH];

	(void)snprintf(log_file_name, 2*MIL, LOG_PLAYER_FILE, username);
	(void)snprintf(bff, LOG_BUF_LENGTH, "[%s] %s\n", logStamp(), str);
	write_file(log_file_name, "a+", bff);
}

static void lastCommands(const char *str)
{
	static char buf[LOG_BUF_LENGTH];

	if (numCmds < 51) {
		(void)snprintf(buf, LOG_BUF_LENGTH, "[%d][%s] %s\n", numCmds, logStamp(), str);
		write_file(LAST_COMMANDS, "a+", buf);
		numCmds++;
	} else {
		numCmds = 1;
		(void)snprintf(buf, LOG_BUF_LENGTH, "[%d][%s] %s\n", numCmds, logStamp(), str);
		write_file(LAST_COMMANDS, "w+", buf);
		numCmds++;
	}
	return;
}

static void write_file(const char *fileName, char *mode, char *text)
{
	static char buf[LOG_BUF_LENGTH];
	FILE *thisFile;

	if ((thisFile = fopen(fileName, mode)) == NULL) {
		(void)snprintf(buf, LOG_BUF_LENGTH, "Error - log_new - cannot open file '%s' in mode %s", fileName, mode);
		log_string(buf);
	} else {
		fprintf(thisFile, "%s", text);
		fclose(thisFile);
	}
}
