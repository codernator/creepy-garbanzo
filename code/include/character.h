/* character.c */
bool character_is_blind(/*@partial@*/struct char_data *ch);
bool character_toggle_comm(/*@partial@*/struct char_data *ch, long commflag);
bool character_has_comm(/*@partial@*/struct char_data *ch, long commflag);

/* character_service.c */
void toggle_afk(/*@partial@*/struct char_data *ch, /*@null@*/const char *message);
void replay(/*@partial@*/struct char_data *ch);
void sit(/*@partial@*/struct char_data *ch, /*@nulL@*//*@partial@*/struct gameobject *on);
void stand(/*@partial@*/struct char_data *ch, /*@nulL@*//*@partial@*/struct gameobject *on);
void look_room(/*@partial@*/struct char_data *ch, /*@partial@*/struct room_index_data *room);
void look_object(/*@partial@*/struct char_data *ch, /*@partial@*/struct gameobject *obj, const char *argument);
void look_character(/*@partial@*/struct char_data *ch, /*@partial@*/struct char_data *victim);
void look_extras(/*@partial@*/struct char_data *ch, const char *name, const int number);
void look_direction(/*@partial@*/struct char_data *ch, const int door);
void look_equipment(/*@partial@*/struct char_data *ch);
void get_obj(/*@partial@*/struct char_data * ch, /*@partial@*/struct gameobject * obj, /*@partial@*/struct gameobject * container);
