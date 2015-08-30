
#define CHANNEL_AUCTION         1
#define CHANNEL_SHOUT           2
#define CHANNEL_TELL            4
#define CHANNEL_CODENET         8
#define CHANNEL_ADVOCATENET     16
#define CHANNEL_BUILDNET        32
#define CHANNEL_EMOTE           64
#define CHANNEL_INFO            128
#define CHANNEL_SAY             256

#define CHANNEL_TARGET_NONE        1
#define CHANNEL_TARGET_OPTIONAL    2
#define CHANNEL_TARGET_REQUIRED    3

typedef struct channel_definition CHANNEL_DEFINITION;
typedef void BROADCAST_FUNCTION(/*@notnull@*/const CHANNEL_DEFINITION const *channel, /*@partial@*/CHAR_DATA *sender, char *argument);
typedef void BROADCAST_TARGET_FUNCTION(/*@notnull@*/const CHANNEL_DEFINITION const *channel, /*@partial@*/CHAR_DATA *sender, /*@partial@*//*@null@*/CHAR_DATA *target, char *argument);
typedef unsigned long CHANNEL_FLAG_TYPE;

struct channel_definition {
    const CHANNEL_FLAG_TYPE flag;
    const char *name;
    const char *print_name;
    const bool mob_trigger;
    const int receiver_position;
    const int target_requirement;
    /*@null@*/BROADCAST_FUNCTION *broadcaster;
    /*@null@*/BROADCAST_TARGET_FUNCTION *targeted_broadcaster;
};

void channels_toggle(/*@partial@*/CHAR_DATA *ch, /*@notnull@*/const CHANNEL_DEFINITION const *channel);
void channels_show(/*@partial@*/const CHAR_DATA *ch);
void channels_permission(/*@partial@*/const CHAR_DATA *grantor, /*@partial@*/CHAR_DATA *grantee, bool granted, /*@notnull@*/const CHANNEL_DEFINITION const *channel);
const CHANNEL_DEFINITION const *channels_parse(const char *argument);
const CHANNEL_DEFINITION const *channels_find(CHANNEL_FLAG_TYPE channel_flag);
void broadcast_channel(/*@partial@*/CHAR_DATA *sender, const CHANNEL_DEFINITION const *channel, /*@partial@*//*@null@*/CHAR_DATA *target, const char *argument);
