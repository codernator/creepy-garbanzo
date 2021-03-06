#include "merc.h"
#include "recycle.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>


/** exports */

/** imports */
extern char str_empty[1];
extern struct note_data *note_free;

/** locals */




struct note_data *new_note()
{
    struct note_data *note;

    if (note_free == NULL) {
        note = alloc_perm((unsigned int)sizeof(*note));
    } else {
        note = note_free;
        note_free = note_free->next;
    }

    VALIDATE(note);
    return note;
}

void free_note(struct note_data *note)
{
    if (!IS_VALID(note))
        return;

    free_string(note->text);
    free_string(note->subject);
    free_string(note->to_list);
    free_string(note->date);
    free_string(note->sender);
    INVALIDATE(note);

    note->next = note_free;
    note_free = note;
}


/***************************************************************************
 *	bans
 ***************************************************************************/
static struct ban_data *ban_free;

/***************************************************************************
 *	new_ban
 *
 *	create a new ban structure
 ***************************************************************************/
struct ban_data *new_ban(void)
{
    static struct ban_data ban_zero;
    struct ban_data *ban;

    if (ban_free == NULL) {
        ban = alloc_perm((unsigned int)sizeof(*ban));
    } else {
        ban = ban_free;
        ban_free = ban_free->next;
    }
    *ban = ban_zero;
    VALIDATE(ban);
    ban->name = &str_empty[0];
    return ban;
}

/***************************************************************************
 *	free_ban
 *
 *	free a ban structure
 ***************************************************************************/
void free_ban(struct ban_data *ban)
{
    if (!IS_VALID(ban))
        return;

    free_string(ban->name);
    INVALIDATE(ban);

    ban->next = ban_free;
    ban_free = ban;
}

/***************************************************************************
 *	extended descriptions
 ***************************************************************************/
extern struct extra_descr_data *extra_descr_free;

/***************************************************************************
 *	new_extra_descr
 *
 *	create a new extra description
 ***************************************************************************/
struct extra_descr_data *new_extra_descr(void)
{
    struct extra_descr_data *ed;

    if (extra_descr_free == NULL) {
        ed = alloc_perm((unsigned int)sizeof(*ed));
    } else {
        ed = extra_descr_free;
        extra_descr_free = extra_descr_free->next;
    }

    ed->keyword = &str_empty[0];
    ed->description = &str_empty[0];

    VALIDATE(ed);
    return ed;
}


/***************************************************************************
 *	free_extra_descr
 *
 *	free an extra description
 ***************************************************************************/
void free_extra_descr(struct extra_descr_data *ed)
{
    if (!IS_VALID(ed))
        return;

    free_string(ed->keyword);
    free_string(ed->description);
    INVALIDATE(ed);

    ed->next = extra_descr_free;
    extra_descr_free = ed;
}


/***************************************************************************
 *	affects
 ***************************************************************************/
struct affect_data *affect_free;

/***************************************************************************
 *	new_affect
 *
 *	create a new affect
 ***************************************************************************/
struct affect_data *new_affect(void)
{
    static struct affect_data af_zero;
    struct affect_data *af;

    if (affect_free == NULL) {
        af = alloc_perm((unsigned int)sizeof(*af));
    } else {
        af = affect_free;
        affect_free = affect_free->next;
    }

    *af = af_zero;
    af->skill = NULL;

    VALIDATE(af);
    return af;
}

/***************************************************************************
 *	free_affect
 *
 *	free the affect memory
 ***************************************************************************/
void free_affect(struct affect_data *af)
{
    if (!IS_VALID(af))
        return;

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}


/***************************************************************************
 *	characters
 ***************************************************************************/
struct char_data *char_free;

/***************************************************************************
 *	new_char
 *
 *	create a new character
 ***************************************************************************/
struct char_data *new_char(void)
{
    static struct char_data ch_zero;
    struct char_data *ch;
    int iter;

    if (char_free == NULL) {
        ch = alloc_perm((unsigned int)sizeof(*ch));
    } else {
        ch = char_free;
        char_free = char_free->next;
    }

    *ch = ch_zero;
    VALIDATE(ch);
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->prompt = &str_empty[0];
    ch->logon = globalSystemState.current_time;
    ch->lines = PAGELEN;
    for (iter = 0; iter < 4; iter++)
        ch->armor[iter] = 100;

    ch->position = POS_STANDING;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->use_ansi_color = false;

    for (iter = 0; iter < MAX_STATS; iter++) {
        ch->perm_stat[iter] = 13;
        ch->mod_stat[iter] = 0;
    }

    return ch;
}


/***************************************************************************
 *	free_char
 *
 *	free an unused character
 ***************************************************************************/
void free_char(struct char_data *ch)
{
    struct gameobject *obj;
    struct gameobject *obj_next;
    struct affect_data *paf;
    struct affect_data *paf_next;

    if (!IS_VALID(ch))
        return;

    if (IS_NPC(ch))
        mobile_count--;

    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
        obj_next = obj->next_content;
        extract_obj(obj);
    }
    ch->carrying = NULL;

    for (paf = ch->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;
        affect_remove(ch, paf);
    }
    ch->affected = NULL;

    free_string(ch->name);
    free_string(ch->short_descr);
    free_string(ch->long_descr);
    free_string(ch->description);
    free_string(ch->prompt);
    if (ch->pcdata != NULL)
        free_pcdata(ch->pcdata);
    ch->next = char_free;
    char_free = ch;

    INVALIDATE(ch);
    return;
}



/***************************************************************************
 *	player data
 ***************************************************************************/
struct pc_data *pcdata_free;

/***************************************************************************
 *	new_pcdata
 *
 *	allocate new pcdata
 ***************************************************************************/
struct pc_data *new_pcdata(void)
{
    static struct pc_data pcdata_zero;
    struct pc_data *pcdata;
    int alias;


    if (pcdata_free == NULL) {
        pcdata = alloc_perm((unsigned int)sizeof(*pcdata));
    } else {
        pcdata = pcdata_free;
        pcdata_free = pcdata_free->next;
    }

    *pcdata = pcdata_zero;
    for (alias = 0; alias < MAX_ALIAS; alias++) {
        pcdata->alias[alias] = NULL;
        pcdata->alias_sub[alias] = NULL;
    }

    pcdata->skills = NULL;
    pcdata->buffer = new_buf();

    VALIDATE(pcdata);
    return pcdata;
}


/***************************************************************************
 *	free_pcdata
 *
 *	recylce unused pcdata
 ***************************************************************************/
void free_pcdata(struct pc_data *pcdata)
{
    struct learned_info *learned;
    struct learned_info *learned_next;
    int alias;

    if (!IS_VALID(pcdata))
        return;

    free_string(pcdata->pwd);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->grestore_string);
    free_string(pcdata->rrestore_string);
    free_string(pcdata->title);
    free_string(pcdata->prefix);
    free_string(pcdata->who_thing);
    free_string(pcdata->deathcry);
    free_string(pcdata->afk_message);
    free_buf(pcdata->buffer);

    /* mortal restring code */
    free_string(pcdata->restring_name);
    free_string(pcdata->restring_short);
    free_string(pcdata->restring_long);

    for (alias = 0; alias < MAX_ALIAS; alias++) {
        free_string(pcdata->alias[alias]);
        free_string(pcdata->alias_sub[alias]);
    }

    for (learned = pcdata->skills; learned != NULL; learned = learned_next) {
        learned_next = learned->next;
        free_learned(learned);
    }
    pcdata->skills = NULL;

    INVALIDATE(pcdata);

    pcdata->next = pcdata_free;
    pcdata_free = pcdata;

    return;
}




/***************************************************************************
 *	setting unique ids
 ***************************************************************************/
static long last_pc_id;
static long last_mob_id;

/***************************************************************************
 *	get_pc_id
 *
 *	get a unique id for a player
 ***************************************************************************/
long get_pc_id(void)
{
    long val;

    val = ((long)globalSystemState.current_time <= last_pc_id) ? last_pc_id + 1 : (long)globalSystemState.current_time;
    last_pc_id = val;

    return val;
}


/***************************************************************************
 *	get_mob_id
 *
 *	get a unique number for a mob
 ***************************************************************************/
long get_mob_id(void)
{
    last_mob_id++;
    return last_mob_id;
}


/***************************************************************************
 *	memory data
 ***************************************************************************/
static struct mem_data *mem_data_free;
static struct buf_type *buf_free;

/***************************************************************************
 *	new_mem_data
 *
 *	create a new memory data structure
 ***************************************************************************/
struct mem_data *new_mem_data(void)
{
    struct mem_data *memory;

    if (mem_data_free == NULL) {
        memory = alloc_mem((unsigned int)sizeof(*memory));
    } else {
        memory = mem_data_free;
        mem_data_free = mem_data_free->next;
    }

    memory->next = NULL;
    memory->id = 0;
    memory->reaction = 0;
    memory->when = 0;
    VALIDATE(memory);

    return memory;
}

/***************************************************************************
 *	free_mem_data
 *
 *	delete the memory data
 ***************************************************************************/
void free_mem_data(struct mem_data *memory)
{
    if (!IS_VALID(memory))
        return;

    memory->next = mem_data_free;
    mem_data_free = memory;
    INVALIDATE(memory);
}


/***************************************************************************
 *	buffer functions
 ***************************************************************************/
/***************************************************************************
 *	buffer sizes
 ***************************************************************************/
static const long buf_size[MAX_BUF_LIST] =
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768 - 64
};

/***************************************************************************
 *	get_size
 *
 *	get a buffer size
 *	returns -1 if it is going to be too large
 ***************************************************************************/
static long get_size(long val)
{
    int i;

    for (i = 0; i < MAX_BUF_LIST; i++)
        if (buf_size[i] >= val)
            return buf_size[i];

    return -1;
}

/***************************************************************************
 *	new_buf
 *
 *	create a new buffer
 ***************************************************************************/
struct buf_type *new_buf()
{
    struct buf_type *buffer;

    if (buf_free == NULL) {
        buffer = alloc_perm((unsigned int)sizeof(*buffer));
    } else {
        buffer = buf_free;
        buf_free = buf_free->next;
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size(BASE_BUF);

    buffer->string = alloc_mem((unsigned int)buffer->size);
    buffer->string[0] = '\0';
    VALIDATE(buffer);

    return buffer;
}


/***************************************************************************
 *	new_buf_size
 *
 *	allocate a new buffer and set the size
 ***************************************************************************/
struct buf_type *new_buf_size(int size)
{
    struct buf_type *buffer;

    if (buf_free == NULL) {
        buffer = alloc_perm((unsigned int)sizeof(*buffer));
    } else {
        buffer = buf_free;
        buf_free = buf_free->next;
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size(size);

    if (buffer->size == -1) {
        log_bug("new_buf: buffer size %d too large.", size);
        raise(SIGABRT);
    }

    buffer->string = alloc_mem((unsigned int)buffer->size);
    buffer->string[0] = '\0';
    VALIDATE(buffer);

    return buffer;
}


/***************************************************************************
 *	free_buf
 *
 *	free the buffer
 ***************************************************************************/
void free_buf(struct buf_type *buffer)
{
    if (!IS_VALID(buffer))
        return;

    free_mem(buffer->string, (unsigned int)buffer->size);
    buffer->string = NULL;
    buffer->size = 0;
    buffer->state = BUFFER_FREED;
    INVALIDATE(buffer);

    buffer->next = buf_free;
    buf_free = buffer;
}

bool add_buf(struct buf_type *buffer, const char *string)
{
    char *oldstr;
    long len;
    long oldsize;

    oldstr = buffer->string;
    oldsize = buffer->size;
    if (buffer->state == BUFFER_OVERFLOW)
        return false;

    len = (long)strlen(buffer->string) + (long)strlen(string) + 1;
    while (len >= buffer->size) {
        buffer->size = get_size(buffer->size + 1);
        if (buffer->size == -1) { /* overflow */
            buffer->size = oldsize;
            buffer->state = BUFFER_OVERFLOW;
            log_bug("buffer overflow past size %d", buffer->size);
            return false;
        }
    }

    if (buffer->size != oldsize) {
        buffer->string = alloc_mem((unsigned int)buffer->size);

        strcpy(buffer->string, oldstr);
        free_mem(oldstr, (unsigned int)oldsize);
    }

    strcat(buffer->string, string);
    return true;
}



/***************************************************************************
 *	printf_buf
 *
 *	printf to the buffer
 ***************************************************************************/
void printf_buf(struct buf_type *buffer, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);


    add_buf(buffer, buf);
}

/***************************************************************************
 *	clear_buf
 *
 *	clear the buffer
 ***************************************************************************/
void clear_buf(struct buf_type *buffer)
{
    buffer->string[0] = '\0';
    buffer->state = BUFFER_SAFE;
}


/***************************************************************************
 *	buf_string
 *
 *	return the string
 ***************************************************************************/
char *buf_string(struct buf_type *buffer)
{
    return buffer->string;
}


/***************************************************************************
 *	mob program recycling
 ***************************************************************************/
static struct mprog_list *mprog_free;

/***************************************************************************
 *	new_mprog
 *
 *	create a new mob program
 ***************************************************************************/
struct mprog_list *new_mprog(void)
{
    static struct mprog_list mp_zero;
    struct mprog_list *mp;

    if (mprog_free == NULL) {
        mp = alloc_perm((unsigned int)sizeof(*mp));
    } else {
        mp = mprog_free;
        mprog_free = mprog_free->next;
    }

    *mp = mp_zero;

    mp->vnum = 0;
    mp->trig_type = 0;
    mp->code = str_dup("");

    VALIDATE(mp);

    return mp;
}


/***************************************************************************
 *	free_mpgrog
 *
 *	free the mob program
 ***************************************************************************/
void free_mprog(struct mprog_list *mp)
{
    if (!IS_VALID(mp))
        return;

    INVALIDATE(mp);
    mp->next = mprog_free;
    mprog_free = mp;
}

