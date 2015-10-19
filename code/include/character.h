/* character.c */
bool character_is_blind(/*@partial@*/CHAR_DATA *ch);
bool character_toggle_comm(/*@partial@*/CHAR_DATA *ch, long commflag);
bool character_has_comm(/*@partial@*/CHAR_DATA *ch, long commflag);

/* character_service.c */
void toggle_afk(/*@partial@*/CHAR_DATA *ch, /*@null@*/const char *message);
void replay(/*@partial@*/CHAR_DATA *ch);
void sit(/*@partial@*/CHAR_DATA *ch, /*@nulL@*//*@partial@*/struct gameobject *on);
void stand(/*@partial@*/CHAR_DATA *ch, /*@nulL@*//*@partial@*/struct gameobject *on);
void look_room(/*@partial@*/CHAR_DATA *ch, /*@partial@*/struct room_index_data *room);
void look_object(/*@partial@*/CHAR_DATA *ch, /*@partial@*/struct gameobject *obj, const char *argument);
void look_character(/*@partial@*/CHAR_DATA *ch, /*@partial@*/CHAR_DATA *victim);
void look_extras(/*@partial@*/CHAR_DATA *ch, const char *name, const int number);
void look_direction(/*@partial@*/CHAR_DATA *ch, const int door);
void look_equipment(/*@partial@*/CHAR_DATA *ch);
void get_obj(/*@partial@*/CHAR_DATA * ch, /*@partial@*/struct gameobject * obj, /*@partial@*/struct gameobject * container);
