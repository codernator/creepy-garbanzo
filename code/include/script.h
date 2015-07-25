/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor(rtaylor@hypercube.org)                                 *
*       Gabrielle Taylor(gtaylor@hypercube.org)                            *
*       Brian Moore(zump@rom.org)                                          *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

#if !defined(___SCRIPT_H)
#define ___SCRIPT_H

#if !defined(__MERC_H)
#include "merc.h"
#endif

/***************************************************************************
*	type definitions
***************************************************************************/
typedef struct script SCRIPT;
typedef struct script_event SCRIPT_EVENT;
typedef struct script_line SCRIPT_LINE;
typedef struct script_stack SCRIPT_STACK;
typedef struct variant_args VARIANT_ARGS;


/***************************************************************************
*	constant definitions
***************************************************************************/
#define SCRIPT_LINE_CMD         0
#define SCRIPT_LINE_IF          1
#define SCRIPT_LINE_AND         2
#define SCRIPT_LINE_OR          3
#define SCRIPT_LINE_ELSE        4
#define SCRIPT_LINE_ENDIF       5
#define SCRIPT_LINE_BREAK       6


#define MAX_STACK_DEPTH         5


#define SCRIPT_LINE_CMD         0
#define SCRIPT_LINE_IF          1
#define SCRIPT_LINE_AND         2
#define SCRIPT_LINE_OR          3
#define SCRIPT_LINE_ELSE        4
#define SCRIPT_LINE_ENDIF       5
#define SCRIPT_LINE_BREAK       6

/***************************************************************************
*	script
***************************************************************************/
struct script {
	int		vnum;
	SCRIPT *	next;
	SCRIPT_EVENT *	event;
	char *		comment;
	char *		raw_code;
	SCRIPT_LINE *	code;
	bool		valid;
	char *		error_desc;
	int		error_line;
};


/***************************************************************************
*	script_event
***************************************************************************/
struct script_event {
	SCRIPT_EVENT *	next;
	int		type;
	VARIANT *	event_trigger;
	SCRIPT *	script;
};

/***************************************************************************
*	script_line
***************************************************************************/
struct script_line {
	char *		code;           // the code to evaluate
	int		cmd_type;       // if/and/or/etc.
	int		line_number;    // the number of the line
	int		nest_level;     // the level of nesting
	bool		valid;
	SCRIPT_LINE *	next;           // the next line of code
	SCRIPT_LINE *	prev;           // the previous script line
	SCRIPT_LINE *	jump;           // the line to jump to when an evaluation fails or for "for" statements
	SCRIPT_LINE *	start_block;    // the start line for a block
	SCRIPT_LINE *	end_block;      // the end line of a block
};


/***************************************************************************
*	script_stack
***************************************************************************/
struct script_stack {
	VARIANT_ARGS *	arg_list;
	int		depth;
};


/***************************************************************************
*	function declarations
***************************************************************************/
void reset_script(SCRIPT * script);
int script_line_type(char *key);
void compile_script(SCRIPT * script);

SCRIPT *script_lookup(char *arg);
SCRIPT_LINE *get_next_ifjump(SCRIPT_LINE * line);



/***************************************************************************
*	memory recycling
***************************************************************************/
SCRIPT_LINE *script_line_free;
SCRIPT_EVENT *script_event_free;
SCRIPT *script_free;


SCRIPT_LINE *new_script_line(char *code);
void free_script_line(SCRIPT_LINE * line);

SCRIPT_EVENT *new_script_event(void);
void free_script_event(SCRIPT_EVENT * event);

SCRIPT *new_script(void);
void free_script(SCRIPT * script);


#endif
