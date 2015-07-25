/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                          *
*   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                          *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/* externs */
extern char str_empty[1];
extern int mobile_count;

/* stuff for providing a crash-proof buffer */

#define MAX_BUF                 32768
#define MAX_BUF_LIST    11
#define BASE_BUF                1024

/* valid states */
#define BUFFER_SAFE                     0
#define BUFFER_OVERFLOW         1
#define BUFFER_FREED            2

/* note recycling */
NOTE_DATA * new_note(void);
void free_note(NOTE_DATA * note);

/* ban data recycling */
BAN_DATA * new_ban(void);
void free_ban(BAN_DATA * ban);

/* descriptor recycling */
DESCRIPTOR_DATA * new_descriptor(void);
void free_descriptor(DESCRIPTOR_DATA * d);


/* extra descr recycling */
EXTRA_DESCR_DATA * new_extra_descr(void);
void free_extra_descr(EXTRA_DESCR_DATA * ed);

/* affect recycling */
AFFECT_DATA * new_affect(void);
void free_affect(AFFECT_DATA * af);

/* object recycling */
OBJ_DATA * new_obj(void);
void free_obj(OBJ_DATA * obj);


/* character recyling */
CHAR_DATA * new_char(void);
void free_char(CHAR_DATA * ch);
PC_DATA *new_pcdata(void);
void free_pcdata(PC_DATA * pcdata);


/* mob id and memory procedures */
long get_pc_id(void);
long get_mob_id(void);
MEM_DATA *new_mem_data(void);
void free_mem_data(MEM_DATA * memory);
MEM_DATA *find_memory(MEM_DATA * memory, long id);

/* buffer procedures */

BUFFER * new_buf(void);
BUFFER *new_buf_size(int size);
void free_buf(BUFFER * buffer);
bool add_buf(BUFFER * buffer, char *string);
void clear_buf(BUFFER * buffer);
char *buf_string(BUFFER * buffer);
void printf_buf(BUFFER * buffer, char *fmt, ...);

HELP_AREA *new_had(void);
HELP_DATA *new_help(void);
void free_help(HELP_DATA * help);


/***************************************************************************
*	mob program recycling
***************************************************************************/
MPROG_LIST *new_mprog(void);
void free_mprog(MPROG_LIST * mp);

