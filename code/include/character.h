/* character.c */
bool character_is_blind(/*@partial@*/CHAR_DATA *ch);
bool character_toggle_comm(/*@partial@*/CHAR_DATA *ch, long commflag, int location);

/* character_service.c */
void toggle_comm(/*@partial@*/CHAR_DATA *ch, long commflag);
void toggle_afk(/*@partial@*/CHAR_DATA *ch, /*@null@*/char *message);
void sit(/*@partial@*/CHAR_DATA *ch, /*@nulL@*//*@partial@*/OBJ_DATA *on);
void stand(/*@partial@*/CHAR_DATA *ch, /*@nulL@*//*@partial@*/OBJ_DATA *on);
void look_room(/*@partial@*/CHAR_DATA *ch, /*@partial@*/ROOM_INDEX_DATA *room);
void look_object(/*@partial@*/CHAR_DATA *ch, /*@partial@*/OBJ_DATA *obj, char *argument);
void look_character(/*@partial@*/CHAR_DATA *ch, /*@partial@*/CHAR_DATA *victim);
void look_extras(/*@partial@*/CHAR_DATA *ch, char *name, int number);
void look_direction(/*@partial@*/CHAR_DATA *ch, int door);
void look_equipment(/*@partial@*/CHAR_DATA *ch);
void get_obj(/*@partial@*/CHAR_DATA * ch, /*@partial@*/OBJ_DATA * obj, /*@partial@*/OBJ_DATA * container);
