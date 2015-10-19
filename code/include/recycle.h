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
struct note_data * new_note(void);
void free_note(struct note_data * note);

/* ban data recycling */
struct ban_data * new_ban(void);
void free_ban(struct ban_data * ban);


/* extra descr recycling */
struct extra_descr_data * new_extra_descr(void);
void free_extra_descr(struct extra_descr_data * ed);

/* affect recycling */
struct affect_data * new_affect(void);
void free_affect(struct affect_data * af);


/* character recyling */
struct char_data * new_char(void);
void free_char(struct char_data * ch);
struct pc_data *new_pcdata(void);
void free_pcdata(struct pc_data * pcdata);


/* mob id and memory procedures */
long get_pc_id(void);
long get_mob_id(void);
struct mem_data *new_mem_data(void);
void free_mem_data(struct mem_data * memory);
struct mem_data *find_memory(struct mem_data * memory, long id);

/* buffer procedures */



/***************************************************************************
*	mob program recycling
***************************************************************************/
MPROG_LIST *new_mprog(void);
void free_mprog(MPROG_LIST * mp);

