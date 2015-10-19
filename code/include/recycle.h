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


/* extra descr recycling */
EXTRA_DESCR_DATA * new_extra_descr(void);
void free_extra_descr(EXTRA_DESCR_DATA * ed);

/* affect recycling */
AFFECT_DATA * new_affect(void);
void free_affect(AFFECT_DATA * af);


/* character recyling */
CHAR_DATA * new_char(void);
void free_char(CHAR_DATA * ch);
struct pc_data *new_pcdata(void);
void free_pcdata(struct pc_data * pcdata);


/* mob id and memory procedures */
long get_pc_id(void);
long get_mob_id(void);
MEM_DATA *new_mem_data(void);
void free_mem_data(MEM_DATA * memory);
MEM_DATA *find_memory(MEM_DATA * memory, long id);

/* buffer procedures */



/***************************************************************************
*	mob program recycling
***************************************************************************/
MPROG_LIST *new_mprog(void);
void free_mprog(MPROG_LIST * mp);

