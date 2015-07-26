/*****************************
*   Multiple Log Handler     *
*    Written by Dalamar      *
*  for Bad Trip MUD 4/29/09  *
*****************************/

#include <stdio.h>
#include <string.h>
#include "merc.h"

static void lastCommands(const char *str);
static void cmdLogAlways(const char *str);
static void cmdLogALL(const char *str);
static void cmdLogPlayer(const char *str, char username[]);
static char *logStamp(void);
static void write_file(char *fileName, char *mode, char *text);

static int numCmds = 1;

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

	strtime = ctime(&current_time);
	strtime[strlen(strtime) - 1] = '\0';

	return strtime;
}

static void cmdLogAlways(const char *str)
{
	char *fileName = "../log/command/logAlways.txt";
	char buf[MSL];

	sprintf(buf, "[%s] %s\n", logStamp(), str);
	write_file(fileName, "a+", buf);
}

static void cmdLogALL(const char *str)
{
	char *fileName = "../log/command/logALLCommands.txt";
	char buf[MSL];

	sprintf(buf, "[%s] %s\n", logStamp(), str);
	write_file(fileName, "a+", buf);
}

static void cmdLogPlayer(const char *str, char username[])
{
	char buf[MSL];
	char bff[MSL];

	sprintf(buf, "../log/player/%s.txt", username);

	sprintf(bff, "[%s] %s\n", logStamp(), str);
	write_file(buf, "a+", bff);
}

static void lastCommands(const char *str)
{
	char *fileName = "../log/command/lastCMDs.txt";
	char buf[MSL];

	if (numCmds < 51) {
		sprintf(buf, "[%d][%s] %s\n", numCmds, logStamp(), str);
		write_file(fileName, "a+", buf);
		numCmds++;
	} else {
		numCmds = 1;
		sprintf(buf, "[%d][%s] %s\n", numCmds, logStamp(), str);
		write_file(fileName, "w+", buf);
		numCmds++;
	}
	return;
}

static void write_file(char *fileName, char *mode, char *text)
{
	char buf[MSL];
	FILE *thisFile;

	if ((thisFile = fopen(fileName, mode)) == NULL) {
		sprintf(buf, "Error - log_new - cannot open file '%s' in mode %s", fileName, mode);
		log_string(buf);
	} else {
		fprintf(thisFile, "%s", text);
		fclose(thisFile);
	}
}
