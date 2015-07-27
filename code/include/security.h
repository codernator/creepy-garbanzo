/* Player Security */
#define PLAYER_ROLE_PLAYER     0l
#define PLAYER_ROLE_ADVOCATE   1l
#define PLAYER_ROLE_CODER      2l
#define PLAYER_ROLE_BUILDER    3l
#define PLAYER_ROLE_QUESTOR    4l

typedef struct role_trust_type ROLE_TRUST_TYPE;


struct role_trust_type {
    int player;
    int advocate;
    int coder;
    int builder;
    int questor;
};

/* determine if player is assigned a role at a minimum trust. 
 * player: not null
 * role_flag: any value from the PLAYER_ROLE_[A-Z]+ definitions.
 * trust: any integer
 */
bool is_trusted_role(PC_DATA *player, long role_flag, int min_trust);
void assign_role(PC_DATA *player, long role_flag, int trust);
void remove_role(PC_DATA *player, long role_flag);



