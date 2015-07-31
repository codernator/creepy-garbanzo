
void broadcast_auctalk(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_immtalk(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_imptalk(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_say(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_yell(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_shout(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_info(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_tell(/*@partial@*/CHAR_DATA *ch, /*@partial@*/CHAR_DATA *whom, char *argument);
void broadcast_reply(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_emote(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_pmote(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_sayto(/*@partial@*/CHAR_DATA *sender, /*@partial@*/CHAR_DATA *whom, char *argument);
void broadcast_gtell(/*@partial@*/CHAR_DATA *sender, char *argument);
