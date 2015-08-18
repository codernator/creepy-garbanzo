#include "merc.h"
#include "recycle.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>


extern NOTE_DATA *note_free;

NOTE_DATA *new_note()
{
	NOTE_DATA *note;

	if (note_free == NULL) {
		note = alloc_perm((unsigned int)sizeof(*note));
	} else {
		note = note_free;
		note_free = note_free->next;
	}

	VALIDATE(note);
	return note;
}

void free_note(NOTE_DATA *note)
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
static BAN_DATA *ban_free;

/***************************************************************************
*	new_ban
*
*	create a new ban structure
***************************************************************************/
BAN_DATA *new_ban(void)
{
	static BAN_DATA ban_zero;
	BAN_DATA *ban;

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
void free_ban(BAN_DATA *ban)
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
extern EXTRA_DESCR_DATA *extra_descr_free;

/***************************************************************************
*	new_extra_descr
*
*	create a new extra description
***************************************************************************/
EXTRA_DESCR_DATA *new_extra_descr(void)
{
	EXTRA_DESCR_DATA *ed;

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
void free_extra_descr(EXTRA_DESCR_DATA *ed)
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
AFFECT_DATA *affect_free;

/***************************************************************************
*	new_affect
*
*	create a new affect
***************************************************************************/
AFFECT_DATA *new_affect(void)
{
	static AFFECT_DATA af_zero;
	AFFECT_DATA *af;

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
void free_affect(AFFECT_DATA *af)
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
CHAR_DATA *char_free;

/***************************************************************************
*	new_char
*
*	create a new character
***************************************************************************/
CHAR_DATA *new_char(void)
{
	static CHAR_DATA ch_zero;
	CHAR_DATA *ch;
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
	ch->color = 0;

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
void free_char(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

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
	free_nicknames(ch);
	ch->next = char_free;
	char_free = ch;

	INVALIDATE(ch);
	return;
}



/***************************************************************************
*	player data
***************************************************************************/
PC_DATA *pcdata_free;

/***************************************************************************
*	new_pcdata
*
*	allocate new pcdata
***************************************************************************/
PC_DATA *new_pcdata(void)
{
	static PC_DATA pcdata_zero;
	PC_DATA *pcdata;
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
void free_pcdata(PC_DATA *pcdata)
{
	LEARNED *learned;
	LEARNED *learned_next;
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
static MEM_DATA *mem_data_free;
static BUFFER *buf_free;

/***************************************************************************
*	new_mem_data
*
*	create a new memory data structure
***************************************************************************/
MEM_DATA *new_mem_data(void)
{
	MEM_DATA *memory;

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
void free_mem_data(MEM_DATA *memory)
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
BUFFER *new_buf()
{
	BUFFER *buffer;

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
BUFFER *new_buf_size(int size)
{
	BUFFER *buffer;

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
void free_buf(BUFFER *buffer)
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


/***************************************************************************
*	add_buf
*
*	add a string to the buffer
***************************************************************************/
bool add_buf(BUFFER *buffer, char *string)
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
void printf_buf(BUFFER *buffer, char *fmt, ...)
{
	char buf[MSL];
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
void clear_buf(BUFFER *buffer)
{
	buffer->string[0] = '\0';
	buffer->state = BUFFER_SAFE;
}


/***************************************************************************
*	buf_string
*
*	return the string
***************************************************************************/
char *buf_string(BUFFER *buffer)
{
	return buffer->string;
}


/***************************************************************************
*	mob program recycling
***************************************************************************/
static MPROG_LIST *mprog_free;

/***************************************************************************
*	new_mprog
*
*	create a new mob program
***************************************************************************/
MPROG_LIST *new_mprog(void)
{
	static MPROG_LIST mp_zero;
	MPROG_LIST *mp;

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
void free_mprog(MPROG_LIST *mp)
{
	if (!IS_VALID(mp))
		return;

	INVALIDATE(mp);
	mp->next = mprog_free;
	mprog_free = mp;
}


/***************************************************************************
*	help area data
***************************************************************************/
static HELP_AREA *had_free;
extern HELP_DATA *help_free;

/***************************************************************************
*	new_had
*
*	create a new help area structure
***************************************************************************/
HELP_AREA *new_had(void)
{
	HELP_AREA *had;
	static HELP_AREA zHad;

	if (had_free) {
		had = had_free;
		had_free = had_free->next;
	} else {
		had = alloc_perm((unsigned int)sizeof(*had));
	}

	*had = zHad;

	return had;
}

/***************************************************************************
*	new_help
*
*	create a new help
***************************************************************************/
HELP_DATA *new_help(void)
{
	HELP_DATA *help;

	if (help_free) {
		help = help_free;
		help_free = help_free->next;
	} else {
		help = alloc_perm((unsigned int)sizeof(*help));
	}

	return help;
}


/***************************************************************************
*	free_help
*
*	free a help structure
***************************************************************************/
void free_help(HELP_DATA *help)
{
	free_string(help->keyword);
	free_string(help->text);
	help->next = help_free;
	help_free = help;
}
