

/***************************************************************************
*	re-write of the message system must accomplish the following
*	goals:
*		1) make it easier to add new threads
*		2) add a "recall" command
*		3) add a "forward" command
*		4) add a "reply" command
*
*	steps to making this happen:
*		1) create a table of all of the threads and properties that
*		   must be present throughout the command functions
*		2) create a command interpreter table listing all of
*		   the possible commands for the note system
*		3) create command functions for each message command
*		4) update the time_stamps in merc.h to an array of
*		   time_t structures
*		5) update save.c to save and load the array properly
*
*	adding new threads:
*		1) add a new constant definition in merc.h for
*		   NOTE_<thread name> and <thread name>_FILE
*		2) add a new definition for a note thread
*		3) add an entry in the message_type_table
*		4) create an entry function (like do_<threadname>())
*		   and register it in interp.c and interp.h
*
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
//#include <sys/time.h>
//#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <time.h>

#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "libfile.h"

/***************************************************************************
*	globals
***************************************************************************/
extern int _filbuf(FILE *);
extern FILE *fp_area;
extern char area_file[MIL];
extern void string_append(CHAR_DATA * ch, char **string);

#define MAX_MESSAGE_LENGTH              4096
#define MIN_MESSAGE_LEVEL               10

/***************************************************************************
*	local procedures
***************************************************************************/
static void load_thread(char *name, NOTE_DATA * *list, int type, time_t free_time);
static void parse_message(CHAR_DATA * ch, char *argument, int type);
static bool is_message_hidden(CHAR_DATA * ch, NOTE_DATA * pnote);
static int count_thread(CHAR_DATA * ch, NOTE_DATA * spool);
static void list_thread(CHAR_DATA * ch, int type, char *argument, bool search);

/***************************************************************************
*	lookup functions
***************************************************************************/
int message_type_lookup(char *name);
static int get_message_index(int type);
static char *get_message_file(int type);
static char *get_message_name(int type);
static NOTE_DATA **get_message_thread(int type);
static NOTE_DATA *get_message(CHAR_DATA * ch, int *msg_num, int type);
static NOTE_DATA *clone_message(NOTE_DATA * src);


/***************************************************************************
*	message specific vars
***************************************************************************/
static NOTE_DATA *note_thread;


#define WEEK    (7 * 24 * 60 * 60)

/***************************************************************************
*	mesage_type_table
***************************************************************************/
const struct message_types message_type_table[] =
{
	{
		NOTE_NOTE, "note", "Note", "General Messages.  Type '`#note``'.",
		NOTE_FILE, 1, WEEK * 2, &note_thread
	},
	{
		0, "", "", "",
		"", 0, 0, NULL
	}
};


/***************************************************************************
*	message_type_lookup
*
*	get the message type by name
***************************************************************************/
int message_type_lookup(char *name)
{
	int idx;

	for (idx = 0; message_type_table[idx].name[0] != '\0'; idx++)
		if (!str_prefix(name, message_type_table[idx].name))
			return message_type_table[idx].type;

	return -1;
}

/***************************************************************************
*	get_message_index
*
*	get the index of the note type structure in the table
***************************************************************************/
static int get_message_index(int type)
{
	int idx;

	for (idx = 0; message_type_table[idx].name[0] != '\0'; idx++)
		if (message_type_table[idx].type == type)
			return idx;

	return -1;
}


/***************************************************************************
*	get_message_name
*
*	get the index of the note type structure in the table
***************************************************************************/
static char *get_message_name(int type)
{
	int msg_idx;

	if ((msg_idx = get_message_index(type)) >= 0)
		return message_type_table[msg_idx].name;

	return "";
}


/***************************************************************************
*	get_message_file
*
*	get the index of the note type structure in the table
***************************************************************************/
static char *get_message_file(int type)
{
	int msg_idx;

	if ((msg_idx = get_message_index(type)) >= 0)
		return message_type_table[msg_idx].file_name;

	return "";
}

/***************************************************************************
*	get_message_thread
*
*	get the index of the note type structure in the table
***************************************************************************/
static NOTE_DATA **get_message_thread(int type)
{
	int msg_idx;

	if ((msg_idx = get_message_index(type)) >= 0)
		return message_type_table[msg_idx].thread;

	return NULL;
}


/***************************************************************************
*	count_thread
*
*	count the number of unread messages in a thread
***************************************************************************/
static int count_thread(CHAR_DATA *ch, NOTE_DATA *thread)
{
	NOTE_DATA *note;
	int count;

	count = 0;
	for (note = thread; note != NULL; note = note->next)
		if (!is_message_hidden(ch, note))
			count++;

	return count;
}


/***************************************************************************
*	save_thread
*
*	save the notes for a given thread
***************************************************************************/
static void save_thread(int type)
{
	NOTE_DATA **list;
	NOTE_DATA *note;
	FILE *fp;
	char *file_name;


	file_name = get_message_file(type);
	note = NULL;
	if ((list = get_message_thread(type)) == NULL
	    || file_name[0] == '\0'
	    || (note = *list) == NULL)
		return;


	if ((fp = fopen(file_name, "w")) == NULL) {
		perror(file_name);
	} else {
		for (; note != NULL; note = note->next) {
			fprintf(fp, "Sender  %s~\n", note->sender);
			fprintf(fp, "Date    %s~\n", note->date);
			fprintf(fp, "Stamp   %ld\n", (long)note->date_stamp);
			fprintf(fp, "To      %s~\n", note->to_list);
			fprintf(fp, "Subject %s~\n", note->subject);
			fprintf(fp, "Text\n%s~\n", note->text);
		}
		fclose(fp);
	}
}

/***************************************************************************
*	load_threads
*
*	load the threads from file
***************************************************************************/
void load_threads(void)
{
	NOTE_DATA *msg_list;
	char *file_name;
	int msg_type;
	int idx;
	time_t retention;

	for (idx = 0; message_type_table[idx].name[0] != '\0'; idx++) {
		msg_list = NULL;
		msg_type = message_type_table[idx].type;
		file_name = message_type_table[idx].file_name;
		retention = message_type_table[idx].retention;

		load_thread(file_name, &msg_list, msg_type, retention);

		*message_type_table[idx].thread = msg_list;
	}
}


/***************************************************************************
*	load_thread
*
*	load a single thread
***************************************************************************/
static void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time)
{
	NOTE_DATA *note_last;
	FILE *fp;

	if ((fp = fopen(name, "r")) == NULL)
		return;

	note_last = NULL;
	for (;; ) {
		NOTE_DATA *note;
		char letter;

		do {
			letter = (char)getc(fp);
			if (feof(fp)) {
				fclose(fp);
				return;
			}
		} while (is_space(letter));

		ungetc(letter, fp);

		/* create a new message */
		note = new_note();
		if (str_cmp(fread_word(fp), "sender"))
			break;
		note->sender = fread_string(fp);

		if (str_cmp(fread_word(fp), "date"))
			break;
		note->date = fread_string(fp);

		if (str_cmp(fread_word(fp), "stamp"))
			break;
		note->date_stamp = (time_t)fread_number(fp);

		if (str_cmp(fread_word(fp), "to"))
			break;
		note->to_list = fread_string(fp);

		if (str_cmp(fread_word(fp), "subject"))
			break;
		note->subject = fread_string(fp);

		if (str_cmp(fread_word(fp), "text"))
			break;
		note->text = fread_string(fp);

		if (free_time > 0
		    && note->date_stamp < globalSystemState.current_time - free_time) {
			free_note(note);
			continue;
		}

		note->type = type;
		if (*list == NULL) {
			*list = note;
		} else {
			if (note_last == NULL)
				break;

			note_last->next = note;
		}
		note_last = note;
	}

	log_bug("load_threads: bad key word.");
	_Exit(1);
}

/***************************************************************************
*	append_message
*
*	append a message to the end of a thread
***************************************************************************/
static void append_message(NOTE_DATA *note)
{
	NOTE_DATA **list;
	NOTE_DATA *last;
	FILE *fp;
	char *file_name;

	file_name = get_message_file(note->type);
	if ((list = get_message_thread(note->type)) == NULL
	    || file_name[0] == '\0')
		return;

	if (*list == NULL) {
		*list = note;
	} else {
		for (last = *list; last->next != NULL; last = last->next) {
		}
		last->next = note;
	}

	if ((fp = fopen(file_name, "a")) == NULL) {
		perror(file_name);
	} else {
		fprintf(fp, "Sender  %s~\n", note->sender);
		fprintf(fp, "Date    %s~\n", note->date);
		fprintf(fp, "Stamp   %ld\n", (long)note->date_stamp);
		fprintf(fp, "To      %s~\n", note->to_list);
		fprintf(fp, "Subject %s~\n", note->subject);
		fprintf(fp, "Text\n%s~\n", note->text);
		fclose(fp);
	}
}



/***************************************************************************
*	attach_message
*
*	attach a character to a message of a given type
***************************************************************************/
static void attach_message(CHAR_DATA *ch, int type)
{
	NOTE_DATA *note;

	if (ch->pnote != NULL)
		return;

	note = new_note();
	note->next = NULL;
	note->sender = str_dup(ch->name);
	note->date = str_dup("");
	note->to_list = str_dup("");
	note->subject = str_dup("");
	note->text = str_dup("");
	note->type = type;
	ch->pnote = note;
	return;
}



/***************************************************************************
*	remove_message
*
*	remove a note from a thread
***************************************************************************/
static void remove_message(NOTE_DATA *note)
{
	NOTE_DATA *prev;
	NOTE_DATA **list;

	if ((list = get_message_thread(note->type)) == NULL)
		return;

	if (note == *list) {
		*list = note->next;
	} else {
		for (prev = *list; prev != NULL; prev = prev->next)
			if (prev->next == note)
				break;

		if (prev == NULL) {
			log_bug("note_remove: note not found.");
			return;
		}

		prev->next = note->next;
	}

	save_thread(note->type);
	free_note(note);
	return;
}



/***************************************************************************
*	is_message_to
*
*	check to see if a character is the recipient of a message
***************************************************************************/
static bool is_message_to(CHAR_DATA *ch, NOTE_DATA *note)
{
	if (IS_NPC(ch))
		return false;

	if (!str_cmp(ch->name, note->sender))
		return true;

	if (is_name("all", note->to_list))
		return true;

	if (IS_IMMORTAL(ch)
	    && (is_name("immortal", note->to_list) || is_name("imm", note->to_list)))
		return true;

	if (IS_IMP(ch)
	    && (is_name("imp", note->to_list) || is_name("implementor", note->to_list)))
		return true;

	if (is_name(ch->name, note->to_list))
		return true;

	return false;
}


/***************************************************************************
*	is_message_hidden
*
*	can a character see a note and is that note newer than the
*	characters last read
***************************************************************************/
static bool is_message_hidden(CHAR_DATA *ch, NOTE_DATA *note)
{
	if (IS_NPC(ch))
		return true;

	if (note->type < 0 || note->type >= NOTE_MAX)
		return true;

	if (note->date_stamp <= ch->pcdata->last_read[note->type])
		return true;

	if (!str_cmp(ch->name, note->sender))
		return true;

	if (!is_message_to(ch, note))
		return true;

	return false;
}

/***************************************************************************
*	update_read
*
*	update the read time stamp for the current thread
***************************************************************************/
static void update_read(CHAR_DATA *ch, NOTE_DATA *note)
{
	time_t stamp;

	if (IS_NPC(ch))
		return;

	if (note->type < 0 || note->type >= NOTE_MAX)
		return;

	stamp = UMAX(ch->pcdata->last_read[note->type], note->date_stamp);
	ch->pcdata->last_read[note->type] = stamp;
}



/***************************************************************************
*	note functions
***************************************************************************/
typedef void NOTE_FN(CHAR_DATA * ch, char *argument, int type);

static NOTE_FN message_read;
static NOTE_FN message_list;
static NOTE_FN message_search;
static NOTE_FN message_remove;
static NOTE_FN message_delete;
static NOTE_FN message_catchup;
static NOTE_FN message_edit;
static NOTE_FN message_add;
static NOTE_FN message_subtract;
static NOTE_FN message_subject;
static NOTE_FN message_to;
static NOTE_FN message_clear;
static NOTE_FN message_show;
static NOTE_FN message_post;
static NOTE_FN message_recall;
static NOTE_FN message_reply;
static NOTE_FN message_forward;

/***************************************************************************
*	note command interpreter
***************************************************************************/
static const struct message_cmd_type {
	char *		cmd;
	NOTE_FN *	fn;
}
message_cmd_table[] =
{
	{ "+",	     message_add      },
	{ "-",	     message_subtract },
	{ "read",    message_read     },
	{ "edit",    message_edit     },
	{ "subject", message_subject  },
	{ "to",	     message_to	      },
	{ "clear",   message_clear    },
	{ "show",    message_show     },
	{ "post",    message_post     },
	{ "send",    message_post     },
	{ "list",    message_list     },
	{ "find",    message_list     },
	{ "search",  message_search   },
	{ "remove",  message_remove   },
	{ "delete",  message_delete   },
	{ "catchup", message_catchup  },
	{ "recall",  message_recall   },
	{ "reply",   message_reply    },
	{ "forward", message_forward  },
	{ "",	     NULL	      }
};


/***************************************************************************
*	parse_message
*
*	command interpreter for the message system
***************************************************************************/
static void parse_message(CHAR_DATA *ch, char *argument, int type)
{
	char cmd[MSL];
	int idx;

    DENY_NPC(ch);

	if (ch->level < MIN_MESSAGE_LEVEL) {
		printf_to_char(ch, "You must be level `#%d`` to use the note system.\n\r", MIN_MESSAGE_LEVEL);
		return;
	}

	if (argument[0] != '\0' && is_help(argument)) {
		int col;

		printf_to_char(ch, "Syntax: %s [cmd]\n\r\n\r", get_message_name(type));
		send_to_char("Available comamnds:\n\r", ch);
		col = 0;
		for (idx = 0; message_cmd_table[idx].cmd[0] != '\0'; idx++) {
			printf_to_char(ch, "%-10.10s", message_cmd_table[idx].cmd);
			if (++col % 4 == 0)
				send_to_char("\n\r", ch);
		}

		if (col % 4 == 0) {
			send_to_char("\n\r", ch);
			return;
		}
		send_to_char("\n\r", ch);
		return;
	}

	smash_tilde(argument);
	argument = one_argument(argument, cmd);
	if (cmd[0] == '\0') {
		message_read(ch, argument, type);
		return;
	} else {
		for (idx = 0; message_cmd_table[idx].cmd[0] != '\0'; idx++) {
			if (!str_prefix(cmd, message_cmd_table[idx].cmd)) {
				(*message_cmd_table[idx].fn)(ch, argument, type);
				return;
			}
		}
	}

	parse_message(ch, "help", type);
}


/***************************************************************************
*	message_read
*
*	read a single message
***************************************************************************/
static void message_read(CHAR_DATA *ch, char *argument, int type)
{
	NOTE_DATA *note;
	int msg_num;

	if (argument[0] == '\0' || !str_prefix(argument, "next")) {
		/* read next unread note */
		msg_num = 0;
	} else if (!str_cmp(argument, "all") || !str_prefix(argument, "first")) {
		msg_num = 1;
	} else if (is_number(argument)) {
		msg_num = parse_int(argument);
	} else {
		send_to_char("Read which number?\n\r", ch);
		return;
	}


	note = get_message(ch, &msg_num, type);
	if (note != NULL) {
		printf_to_char(ch, "[%3d] %s: %s\n\r%s\n\rTo: %s\n\r",
			       msg_num,
			       note->sender,
			       note->subject,
			       note->date,
			       note->to_list);
		page_to_char(note->text, ch);
		update_read(ch, note);
	} else {
		if (msg_num > 0)
			printf_to_char(ch, "There aren't that many %s messages.\n\r", get_message_name(type));
		else
			printf_to_char(ch, "You have no unread %s messages.\n\r", get_message_name(type));
	}
}


/***************************************************************************
*	message_list
*
*	display the list of messages - optionally with an argument
*	used to match sender
***************************************************************************/
static void message_list(CHAR_DATA *ch, char *argument, int type)
{
	list_thread(ch, type, argument, false);
}


/***************************************************************************
*	message_search
*
*	searcht the list of messages in the string editor
***************************************************************************/
static void message_search(CHAR_DATA *ch, char *argument, int type)
{
	list_thread(ch, type, argument, true);
}



/***************************************************************************
*	message_remove
*
*	remove a message by number - used by players - can only
*	delete things posted by them
***************************************************************************/
static void message_remove(CHAR_DATA *ch, char *argument, int type)
{
	NOTE_DATA **list;
	NOTE_DATA *note;
	int msg_num;

	if (!is_number(argument)) {
		send_to_char("Note remove which number?\n\r", ch);
		return;
	}


	if ((list = get_message_thread(type)) == NULL) {
		send_to_char("That is not a valid message type.\n\r", ch);
		return;
	}

	msg_num = parse_int(argument);

	if ((note = get_message(ch, &msg_num, type)) != NULL) {
		if (!str_cmp(note->sender, ch->name)) {
			remove_message(note);
			send_to_char("Ok.\n\r", ch);
		} else {
			send_to_char("You may only remove notes that you post.\n\r", ch);
		}
		return;
	}

	printf_to_char(ch, "There aren't that many %s messages.", get_message_name(type));
}

/***************************************************************************
*	message_delete
*
*	delete a message by number
***************************************************************************/
static void message_delete(CHAR_DATA *ch, char *argument, int type)
{
	NOTE_DATA **list;
	NOTE_DATA *note;
	int msg_num;

	if (get_trust(ch) < LEVEL_IMMORTAL) {
		send_to_char("You are not high enough level to delete notes.\n\r", ch);
		return;
	}

	if (!is_number(argument)) {
		send_to_char("Note delete which number?\n\r", ch);
		return;
	}

	if ((list = get_message_thread(type)) == NULL) {
		send_to_char("That is not a valid message type.\n\r", ch);
		return;
	}

	msg_num = parse_int(argument);

	if ((note = get_message(ch, &msg_num, type)) != NULL) {
		remove_message(note);
		send_to_char("Ok.\n\r", ch);
		return;
	}

	printf_to_char(ch, "There aren't that many %s messages.", get_message_name(type));
}

/***************************************************************************
*	message_catchup
*
*	edit the message in the string editor
***************************************************************************/
static void message_catchup(CHAR_DATA *ch, char *argument, int type)
{
	if (type < 0 || type >= NOTE_MAX)
		return;

	ch->pcdata->last_read[type] = globalSystemState.current_time;
	printf_to_char(ch, "%s thread marked as read.\n\r", get_message_name(type));
}

/***************************************************************************
*	message_edit
*
*	edit the message in the string editor
***************************************************************************/
static void message_edit(CHAR_DATA *ch, char *argument, int type)
{
	attach_message(ch, type);
	if (ch->pnote == NULL) {
		send_to_char("The message type is invalid.\n\r", ch);
		return;
	}

	string_append(ch, &ch->pnote->text);
}

/***************************************************************************
*	message_add
*
*	add a line to the current note
***************************************************************************/
static void message_add(CHAR_DATA *ch, char *argument, int type)
{
	BUFFER *buf;

	attach_message(ch, type);
	if (ch->pnote == NULL) {
		send_to_char("The message type is invalid.\n\r", ch);
		return;
	}

	if (strlen(ch->pnote->text) + strlen(argument) >= MAX_MESSAGE_LENGTH) {
		send_to_char("Note too long.\n\r", ch);
		return;
	}


	/* dump the string to a buffer */
	buf = new_buf();
	add_buf(buf, ch->pnote->text);
	add_buf(buf, argument);
	add_buf(buf, "\n\r");

	free_string(ch->pnote->text);
	ch->pnote->text = str_dup(buf_string(buf));
	free_buf(buf);

	send_to_char("Ok.\n\r", ch);
}



/***************************************************************************
*	message_subtract
*
*	remove the last line from the note
***************************************************************************/
static void message_subtract(CHAR_DATA *ch, char *argument, int type)
{
	static char buf[MAX_MESSAGE_LENGTH];
	int len, buf_len;
	bool found = false;

	attach_message(ch, type);
	if (ch->pnote == NULL) {
		send_to_char("The message type is invalid.\n\r", ch);
		return;
	}

	if (ch->pnote->text == NULL || ch->pnote->text[0] == '\0') {
		send_to_char("No lines left to remove.\n\r", ch);
		return;
	}

	if (strlen(ch->pnote->text) > MAX_MESSAGE_LENGTH) {
		send_to_char("The note you are working on is too long to do that.\n\r", ch);
		return;
	}

	strcpy(buf, ch->pnote->text);
	buf_len = (int)strlen(buf);
	for (len = buf_len; len > 0; len--) {
		if (buf[len] == '\r') {
			/* ignore the first '\r' */
			if (!found) {
				if (len > 0)
					len--;
				found = true;
			} else {
				/* crop off everything after the second '\r' */
				buf[len + 1] = '\0';
				break;
			}
		}
	}


	if (len <= 0)
		buf[0] = '\0';

	free_string(ch->pnote->text);
	ch->pnote->text = str_dup(buf);
	return;
}



/***************************************************************************
*	message_subject
*
*	set the subject property of a message
***************************************************************************/
static void message_subject(CHAR_DATA *ch, char *argument, int type)
{
	attach_message(ch, type);
	if (ch->pnote == NULL) {
		send_to_char("The message type is invalid.\n\r", ch);
		return;
	}

	free_string(ch->pnote->subject);
	ch->pnote->subject = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
}


/***************************************************************************
*	message_to
*
*	set the "to" property of a message
***************************************************************************/
static void message_to(CHAR_DATA *ch, char *argument, int type)
{
	attach_message(ch, type);
	if (ch->pnote == NULL) {
		send_to_char("The message type is invalid.\n\r", ch);
		return;
	}

	free_string(ch->pnote->to_list);
	ch->pnote->to_list = str_dup(argument);

	send_to_char("Ok.\n\r", ch);
}


/***************************************************************************
*	message_clear
*
*	clear the current message
***************************************************************************/
static void message_clear(CHAR_DATA *ch, char *argument, int type)
{
	if (ch->pnote != NULL) {
		free_note(ch->pnote);
		ch->pnote = NULL;
	}
	send_to_char("Ok.\n\r", ch);
}


/***************************************************************************
*	message_show
*
*	show the current message
***************************************************************************/
static void message_show(CHAR_DATA *ch, char *argument, int type)
{
	if (ch->pnote == NULL) {
		send_to_char("You have no note in progress.\n\r", ch);
		return;
	}

	printf_to_char(ch, "%s: %s\n\rTo: %s\n\r",
		       ch->pnote->sender,
		       ch->pnote->subject,
		       ch->pnote->to_list);
	send_to_char(ch->pnote->text, ch);
}


/***************************************************************************
*	message_post
*
*	post the current message
***************************************************************************/
static void message_post(CHAR_DATA *ch, char *argument, int type)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true };
	DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *dpending;
	char *time_str;
	int msg_idx;

	if (ch->pnote == NULL) {
		send_to_char("You have no note in progress.\n\r", ch);
		return;
	}

	if (!str_cmp(ch->pnote->to_list, "")) {
		send_to_char("You need to provide a recipient(name, all, immortal, or imp).\n\r", ch);
		return;
	}

	if (!str_cmp(ch->pnote->subject, "")) {
		send_to_char("You need to provide a subject.\n\r", ch);
		return;
	}

	if ((msg_idx = get_message_index(type)) < 0) {
		send_to_char("That is an invalid note type.\n\r", ch);
		return;
	}

	if (!IS_TRUSTED(ch, message_type_table[msg_idx].post_level)) {
		printf_to_char(ch, "You aren't high enough level to write %s messages.",
			       message_type_table[msg_idx].name);
		return;
	}


	ch->pnote->next = NULL;

	/* stuff the current time into a string */
	time_str = ctime(&globalSystemState.current_time);
	time_str[strlen(time_str) - 1] = '\0';

	free_string(ch->pnote->date);
	ch->pnote->date = str_dup(time_str);
	ch->pnote->date_stamp = globalSystemState.current_time;
	ch->pnote->type = type;

	append_message(ch->pnote);

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
		CHAR_DATA *wch;
        dpending = descriptor_iterator(d, &playing_filter);

		wch = CH(d);

		if (is_message_to(wch, ch->pnote)) {
			if (can_see(wch, ch)) {
				printf_to_char(wch, "%s just left a new %s message.\n\r", PERS(ch, wch), message_type_table[msg_idx].name);
			}
		}
	}

	ch->pnote = NULL;
}





/***************************************************************************
*	message_recall
*
*	remove a message by number - and put it back into the players
*	editing pointer
***************************************************************************/
static void message_recall(CHAR_DATA *ch, char *argument, int type)
{
	NOTE_DATA *note;
	NOTE_DATA *clone_msg;
	int msg_num;

	if (ch->pnote != NULL) {
		send_to_char("You currently have a note in progress.  "
			     "Please finish it before recalling another.\n\r", ch);
		return;
	}

	if (!is_number(argument)) {
		send_to_char("You must specify a message number.\n\r", ch);
		return;
	}

	msg_num = parse_int(argument);
	if ((note = get_message(ch, &msg_num, type)) == NULL
	    || str_cmp(note->sender, ch->name)) {
		send_to_char("You can only recall messages that you have written.\n\r", ch);
		return;
	}

	if ((clone_msg = clone_message(note)) == NULL) {
		send_to_char("You cannot recall that message.\n\r", ch);
		return;
	}

	ch->pnote = clone_msg;
	remove_message(note);
}


/***************************************************************************
*	message_reply
*
*	create a reply to a message number
***************************************************************************/
static void message_reply(CHAR_DATA *ch, char *argument, int type)
{
	NOTE_DATA *note;
	NOTE_DATA *clone_msg;
	BUFFER *buf;
	int msg_num;

	if (ch->pnote != NULL) {
		send_to_char("You currently have a note in progress.  "
			     "Please finish it before recalling another.\n\r", ch);
		return;
	}

	if (!is_number(argument)) {
		send_to_char("You must specify a message number.\n\r", ch);
		return;
	}

	msg_num = parse_int(argument);
	if ((note = get_message(ch, &msg_num, type)) == NULL
	    || !str_cmp(note->sender, ch->name)) {
		send_to_char("You cannot reply to messages that you have written.  Use 'recall'.\n\r", ch);
		return;
	}

	if ((clone_msg = clone_message(note)) == NULL) {
		send_to_char("You cannot reply to that message.\n\r", ch);
		return;
	}

	/* create a buffer for doing property manipulation */
	buf = new_buf();

	/* copy the original message text */
	printf_buf(buf, "\n\r`1----`!Original Message by %-12.12s `3[`#%24.24s`3]`1------------``\n\r\n\r%s",
		   clone_msg->sender,
		   clone_msg->date,
		   clone_msg->text);
	add_buf(buf, "\n\r`1---------------------------------------------------------------------------``\n\r\n\r");

	free_string(clone_msg->text);
	clone_msg->text = str_dup(buf_string(buf));

	/* attach "RE: " to the beginning of the subject line */
	clear_buf(buf);
	printf_buf(buf, "RE: %s", clone_msg->subject);

	free_string(clone_msg->subject);
	clone_msg->subject = str_dup(buf_string(buf));

	/* free the buffer */
	free_buf(buf);

	free_string(clone_msg->to_list);
	clone_msg->to_list = str_dup(clone_msg->sender);
	free_string(clone_msg->sender);
	clone_msg->sender = str_dup(ch->name);
	ch->pnote = clone_msg;
}


/***************************************************************************
*	message_forward
*
*	forward a message by number
***************************************************************************/
static void message_forward(CHAR_DATA *ch, char *argument, int type)
{
	NOTE_DATA *note;
	NOTE_DATA *clone_msg;
	BUFFER *buf;
	int msg_num;

	if (ch->pnote != NULL) {
		send_to_char("You currently have a note in progress.  "
			     "Please finish it before recalling another.\n\r", ch);
		return;
	}

	if (!is_number(argument)) {
		send_to_char("You must specify a message number.\n\r", ch);
		return;
	}

	msg_num = parse_int(argument);
	if ((note = get_message(ch, &msg_num, type)) == NULL
	    || (clone_msg = clone_message(note)) == NULL) {
		send_to_char("You cannot forward that message.\n\r", ch);
		return;
	}

	/* create a new buffer for manipulating properties */
	buf = new_buf();

	/* copy the originall message */
	printf_buf(buf, "\n\r`1----`!Original Message by %-12.12s `3[`#%24.24s`3]`1------------``\n\r\n\r%s",
		   clone_msg->sender,
		   clone_msg->date,
		   clone_msg->text);
	add_buf(buf, "\n\r`1---------------------------------------------------------------------------``\n\r\n\r");
	free_string(clone_msg->text);
	clone_msg->text = str_dup(buf_string(buf));

	/* attach "FWD:" to the beginning of the subject line */
	clear_buf(buf);

	printf_buf(buf, "FWD: %s", clone_msg->subject);
	free_string(clone_msg->subject);
	clone_msg->subject = str_dup(buf_string(buf));

	/* free the buffer */
	free_buf(buf);


	free_string(clone_msg->to_list);
	free_string(clone_msg->sender);
	clone_msg->to_list = str_dup("");
	clone_msg->sender = str_dup(ch->name);
	ch->pnote = clone_msg;
}



/***************************************************************************
*	list_thread
*
*	show a list of messages that meet certain criteria - used by
*	message_list and message_search
***************************************************************************/
static void list_thread(CHAR_DATA *ch, int type, char *argument, bool search)
{
	NOTE_DATA **list;
	NOTE_DATA *note;
	BUFFER *buf;
	char *txt;
	int msg_idx;
	bool found;

	if ((list = get_message_thread(type)) == NULL) {
		send_to_char("That is not a valid message type.\n\r", ch);
		return;
	}

	msg_idx = 0;
	buf = new_buf();
	found = false;
	for (note = *list; note != NULL; note = note->next) {
		if (is_message_to(ch, note)) {
			msg_idx++;

			txt = uncolor_str(note->text);
			if (argument[0] == '\0'
			    || !str_prefix(argument, note->sender)
			    || (search && str_infix(argument, txt) == 0)) {
				printf_buf(buf, "[%3d%c] %s: %s``\n\r",
					   msg_idx,
					   is_message_hidden(ch, note) ? ' ' : 'N',
					   note->sender,
					   note->subject);
				found = true;
			}
			free_string(txt);
		}
	}

	if (!found) {
		if (argument[0] == '\0') {
			add_buf(buf, "\n\rYou have no messages. Nobody loves you.\n\r");
		} else {
			if (search)
				printf_buf(buf, "\n\rThere are no messages containing '`!%s``'.\n\r", argument);
			else
				printf_buf(buf, "\n\rYou have no messages from '`!%s``'.\n\r", argument);
		}
	}

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return;
}



/***************************************************************************
*	get_message
*
*	get a single message by number
***************************************************************************/
static NOTE_DATA *get_message(CHAR_DATA *ch, int *msg_num, int type)
{
	NOTE_DATA **list;
	NOTE_DATA *note;
	int idx;

	if ((list = get_message_thread(type)) != NULL) {
		idx = 0;
		for (note = *list; note != NULL; note = note->next) {
			if (is_message_to(ch, note))
				idx++;

			if ((*msg_num == 0 && !is_message_hidden(ch, note))
			    || (is_message_to(ch, note) && idx == *msg_num)) {
				*msg_num = idx;
				return note;
			}
		}
	}

	return NULL;
}


/***************************************************************************
*	clone_message
*
*	clone a message
***************************************************************************/
static NOTE_DATA *clone_message(NOTE_DATA *src)
{
	NOTE_DATA *clone;

	clone = new_note();
	clone->next = NULL;
	clone->date = str_dup(src->date);
	clone->sender = str_dup(src->sender);
	clone->to_list = str_dup(src->to_list);
	clone->subject = str_dup(src->subject);
	clone->text = str_dup(src->text);
	clone->type = src->type;

	return clone;
}




/***************************************************************************
*	command procedures
***************************************************************************/
/***************************************************************************
*	do_unread
*
*	show a list of unread messages
***************************************************************************/
void do_unread(CHAR_DATA *ch, char *argument)
{
	int count;
	int idx;
	int total;

	if (IS_NPC(ch))
		return;

	total = 0;
	for (idx = 0; message_type_table[idx].name[0] != '\0'; idx++) {
		if ((count = count_thread(ch, *message_type_table[idx].thread)) > 0) {
			if (total == 0) {
				send_to_char("\n\r`&Message Board     Unread     Description``\n\r", ch);
				send_to_char("`8-------------------------------------------------------------------------------``\n\r", ch);
			}

			printf_to_char(ch, "%-16.16s   %3d       %s\n\r",
				       message_type_table[idx].display,
				       count,
				       message_type_table[idx].desc);
			total += count;
		}
	}

	if (total == 0) {
		send_to_char("You have `!no unread`` messages.\n\r", ch);
	} else {
		send_to_char("`8-------------------------------------------------------------------------------``\n\r", ch);
		printf_to_char(ch, "`@%d`` Total Unread Messages.\n\r", total);
	}
}

/***************************************************************************
*	do_catchup
*
*	catchup on all message lists
***************************************************************************/
void do_catchup(CHAR_DATA *ch, char *argument)
{
	int idx;

	if (IS_NPC(ch))
		return;

	for (idx = 0; message_type_table[idx].name[0] != '\0'; idx++)
		message_catchup(ch, argument, message_type_table[idx].type);
}

/***************************************************************************
*	do_note
*
*	entry point for the note thread
***************************************************************************/
void do_note(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_NOTE);
}

/***************************************************************************
*	do_rpnote
*	entry point for the role-play note thread
***************************************************************************/
void do_rpnote(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_RPNOTE);
}
/***************************************************************************
*	do_aucnote
*	entry point for the auction note thread
***************************************************************************/
void do_aucnote(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_AUCNOTE);
}

/***************************************************************************
*	do_penalty
*	entry point for the penalty thread
***************************************************************************/
void do_penalty(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_PENALTY);
}
/***************************************************************************
*	do_news
*	entry point for the news thread
***************************************************************************/
void do_news(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_NEWS);
}

/***************************************************************************
*	do_changes
*	entry point for the changes thread
***************************************************************************/
void do_changes(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_CHANGES);
}


/***************************************************************************
*	do_contest
*	entry point for the contest thread
***************************************************************************/
void do_contest(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_CONTEST);
}

/***************************************************************************
*	do_ideas
*	entry point for the contest thread
***************************************************************************/
void do_idea(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_IDEA);
}
/***************************************************************************
*	do_building
*	entry point for the contest thread
***************************************************************************/
void do_build(CHAR_DATA *ch, char *argument)
{
	parse_message(ch, argument, NOTE_BUILD);
}
