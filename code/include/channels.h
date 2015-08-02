
#define CHANNEL_AUCTION         1
#define CHANNEL_SHOUT           2
#define CHANNEL_TELL            4
#define CHANNEL_CODENET         8
#define CHANNEL_ADVOCATENET     16
#define CHANNEL_BUILDNET        32
#define CHANNEL_EMOTE           64
#define CHANNEL_INFO            128
#define CHANNEL_SAY             256

typedef struct channel_definition CHANNEL_DEFINITION;
typedef void BROADCAST_FUNCTION(/*@null@*/CHANNEL_DEFINITION *channel, /*@partial@*/CHAR_DATA *sender, char *argument);
typedef unsigned long CHANNEL_FLAG_TYPE;
struct channel_definition {
    CHANNEL_FLAG_TYPE flag;
    char *name;
    char *print_name;
    bool mob_trigger;
    int receiver_position;
    BROADCAST_FUNCTION *broadcaster;
};

void channels_toggle(/*@partial@*/CHAR_DATA *ch, /*@notnull@*/CHANNEL_DEFINITION *channel);
void channels_show(/*@partial@*/CHAR_DATA *ch);
void channels_permission(/*@partial@*/CHAR_DATA *grantor, /*@partial@*/CHAR_DATA *grantee, bool granted, /*@notnull@*/CHANNEL_DEFINITION *channel);
CHANNEL_DEFINITION *channels_parse(char *argument);
CHANNEL_DEFINITION *channels_find(CHANNEL_FLAG_TYPE channel_flag);

void broadcast_channel(/*@partial@*/CHAR_DATA *sender, CHANNEL_DEFINITION *channel, char *argument);
void broadcast_tell(/*@partial@*/CHAR_DATA *ch, /*@partial@*/CHAR_DATA *whom, char *argument);
void broadcast_reply(/*@partial@*/CHAR_DATA *ch, char *argument);
void broadcast_sayto(/*@partial@*/CHAR_DATA *sender, /*@partial@*/CHAR_DATA *whom, char *argument);
void broadcast_gtell(/*@partial@*/CHAR_DATA *sender, char *argument);
