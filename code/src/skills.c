#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "skills.h"
#include "lookup.h"
#include "interp.h"
#include "channels.h"


/**
 * quick-lookup skill pointers
 *
 * skills that commonly require calls to skill_lookup - especially
 * those that just happen as part of game processing, that is not
 * initiated by a user - can cause a lot of processor utilization
 *
 * this system replaces the one that stock ROM created for gsn's
 * I tried just removing the gsn's but the number of calls to
 * skill_lookup, particularly in the battle system, was just
 * unmanagable.
 */
SKILL *gsp_axe;
SKILL *gsp_dagger;
SKILL *gsp_flail;
SKILL *gsp_mace;
SKILL *gsp_polearm;
SKILL *gsp_shield_block;
SKILL *gsp_spear;
SKILL *gsp_sword;
SKILL *gsp_whip;

/* skill lookup return values for frequently  used skills */
SKILL *gsp_hand_to_hand;
SKILL *gsp_aggressive_parry;
SKILL *gsp_second_attack;
SKILL *gsp_third_attack;
SKILL *gsp_fourth_attack;
SKILL *gsp_fifth_attack;
SKILL *gsp_poison;
SKILL *gsp_web;
SKILL *gsp_banzai;
SKILL *gsp_enhanced_damage;
SKILL *gsp_flanking;
SKILL *gsp_whirlwind;
SKILL *gsp_darkness;
SKILL *gsp_invisibility;
SKILL *gsp_mass_invisibility;
SKILL *gsp_sleep;
SKILL *gsp_hide;
SKILL *gsp_sneak;
SKILL *gsp_gate;
SKILL *gsp_portal;
SKILL *gsp_nexus;
SKILL *gsp_blindness;
SKILL *gsp_fear;
SKILL *gsp_evade;
SKILL *gsp_dodge;
SKILL *gsp_parry;
SKILL *gsp_plague;
SKILL *gsp_deft;
SKILL *gsp_dash;
SKILL *gsp_black_mantle;
SKILL *gsp_fast_healing;
SKILL *gsp_black_plague;
SKILL *gsp_meditation;
SKILL *gsp_burning_flames;
SKILL *gsp_peek;
SKILL *gsp_detect_magic;
SKILL *gsp_faerie_fog;
SKILL *gsp_haste;
SKILL *gsp_haggle;
SKILL *gsp_bless;
SKILL *gsp_obless;
SKILL *gsp_curse;
SKILL *gsp_frenzy;

/* race skills */
SKILL *gsp_supernatural_speed;
SKILL *gsp_tooth_and_claw;
SKILL *gsp_veil;
SKILL *gsp_anti_magic_aura;
SKILL *gsp_dream;

/* room affects */
SKILL *gsp_haven;
SKILL *gsp_mana_vortex;


int gn_max_skill_sn;
int gn_max_group_sn;
static SKILL **gsp_skill_hash;

/**
 * match a name to a skill pointer
 */
static const struct skill_resolve_type {
    char *    name;
    SKILL **skill;
} skill_resolve_table[] =
{
    { "axe", &gsp_axe },
    { "dagger", &gsp_dagger },
    { "flail", &gsp_flail },
    { "mace", &gsp_mace },
    { "polearm", &gsp_polearm },
    { "shield block", &gsp_shield_block },
    { "spear", &gsp_spear },
    { "sword", &gsp_sword },
    { "whip", &gsp_whip },
    { "hand to hand", &gsp_hand_to_hand },
    { "parry", &gsp_parry },
    { "dodge", &gsp_dodge },
    { "evade", &gsp_evade },
    { "aggressive parry", &gsp_aggressive_parry },
    { "second attack", &gsp_second_attack },
    { "third attack", &gsp_third_attack },
    { "fourth attack", &gsp_fourth_attack },
    { "fifth attack", &gsp_fifth_attack },
    { "poison", &gsp_poison },
    { "web", &gsp_web },
    { "banzai", &gsp_banzai },
    { "enhanced damage", &gsp_enhanced_damage },
    { "flanking", &gsp_flanking },
    { "darkness", &gsp_darkness },
    { "invisibility", &gsp_invisibility },
    { "mass invisibility", &gsp_mass_invisibility },
    { "sleep", &gsp_sleep },
    { "hide", &gsp_hide },
    { "sneak", &gsp_sneak },
    { "nexus", &gsp_nexus },
    { "portal", &gsp_portal },
    { "gate", &gsp_gate },
    { "blindness", &gsp_blindness },
    { "fear", &gsp_fear },
    { "plague", &gsp_plague },
    { "deft", &gsp_deft },
    { "dash", &gsp_dash },
    { "black mantle", &gsp_black_mantle },
    { "fast healing", &gsp_fast_healing },
    { "black plague", &gsp_black_plague },
    { "meditation", &gsp_meditation },
    { "black plague", &gsp_black_plague },
    { "burning flames", &gsp_burning_flames },
    { "peek", &gsp_peek },
    { "detect magic", &gsp_detect_magic },
    { "faerie fog", &gsp_faerie_fog },
    { "haste", &gsp_haste },
    { "haggle", &gsp_haggle },
    { "bless", &gsp_bless },
    { "curse", &gsp_curse },
    { "frenzy", &gsp_frenzy },

    /* race skills */
    { "supernatural speed", &gsp_supernatural_speed },
    { "tooth and claw", &gsp_tooth_and_claw },
    { "veil", &gsp_veil },
    { "anti-magic aura", &gsp_anti_magic_aura },

    /* room affects */
    { "haven", &gsp_haven },
    { "mana vortex", &gsp_mana_vortex },

    { "", NULL }
};

/**
 * resolve the global skill pointers
 */
void resolve_global_skills()
{
    int idx;

    for (idx = 0; skill_resolve_table[idx].name[0] != '\0'; idx++) {
        *skill_resolve_table[idx].skill = skill_lookup(skill_resolve_table[idx].name);
        if (*skill_resolve_table[idx].skill == NULL)
            log_string("ERROR - resolve_global_skills NULL skill: %s", skill_resolve_table[idx].name);
    }


    resolve_global_hash();
}

/**
 * resolve the global skill pointers
 */
void resolve_global_hash()
{
    SKILL *skill;
    SKILL **skill_idx;

    if (gn_max_skill_sn > 0) {
        size_t size;

        if (gsp_skill_hash != NULL)
            free(gsp_skill_hash);

        size = sizeof(SKILL) * (gn_max_skill_sn + 1);

        gsp_skill_hash = malloc(sizeof(SKILL) * (gn_max_skill_sn + 1));
        memset(gsp_skill_hash, 0, size);

        skill_idx = gsp_skill_hash;
        for (skill = skill_list; skill != NULL; skill = skill->next)
            skill_idx[skill->sn] = skill;
    }
}

/**
 * create a new level info structure
 */
LEVEL_INFO *new_level_info(void)
{
    static LEVEL_INFO li_zero;
    LEVEL_INFO *li;

    if (level_info_free == NULL) {
        li = alloc_perm((unsigned int)sizeof(*li));
    } else {
        li = level_info_free;
        level_info_free = level_info_free->next;
    }

    *li = li_zero;

    VALIDATE(li);
    return li;
}

/**
 * free the level info structure
 */
void free_level_info(LEVEL_INFO *li)
{
    if (!IS_VALID(li))
        return;

    INVALIDATE(li);
    li->next = level_info_free;
    level_info_free = li;
}

/**
 * create a new learned skill structure
 */
LEARNED *new_learned(void)
{
    static LEARNED learned_zero;
    LEARNED *learned;

    if (learned_free == NULL) {
        learned = alloc_perm((unsigned int)sizeof(*learned));
    } else {
        learned = learned_free;
        learned_free = learned_free->next;
    }

    *learned = learned_zero;

    VALIDATE(learned);
    return learned;
}

/**
 * free the learned skill structure
 */
void free_learned(LEARNED *learned)
{
    if (!IS_VALID(learned))
        return;

    INVALIDATE(learned);
    learned->skill = NULL;
    learned->next = learned_free;
    learned->prev = NULL;
    learned->percent = 0;
    learned_free = learned;
}

/**
 * create a new skill list structure
 */
SKILL_LIST *new_skill_list(void)
{
    static SKILL_LIST list_zero;
    SKILL_LIST *list;

    if (skill_list_free == NULL) {
        list = alloc_perm((unsigned int)sizeof(*list));
    } else {
        list = skill_list_free;
        skill_list_free = skill_list_free->next;
    }

    *list = list_zero;

    VALIDATE(list);
    return list;
}

/**
 * free the skill list structure
 */
void free_skill_list(SKILL_LIST *list)
{
    if (!IS_VALID(list))
        return;

    INVALIDATE(list);
    list->skill = NULL;
    list->prev = NULL;
    list->next = skill_list_free;
    skill_list_free = list;
}

/**
 * create a skill group structure
 */
GROUP *new_group(void)
{
    static GROUP group_zero;
    GROUP *group;

    if (group_free == NULL) {
        group = alloc_perm((unsigned int)sizeof(*group));
    } else {
        group = group_free;
        group_free = group_free->next;
    }

    *group = group_zero;

    VALIDATE(group);
    return group;
}

/**
 * free the skill group structure
 */
void free_group(GROUP *group)
{
    if (!IS_VALID(group))
        return;

    INVALIDATE(group);
    if (group->name != NULL)
        free_string(group->name);
    group->levels = NULL;
    group->skills = NULL;
    group->next = group_free;
    group_free = group;
}

/**
 * create a new skill structure
 */
SKILL *new_skill(void)
{
    static SKILL skill_zero;
    SKILL *skill;

    if (skill_free == NULL) {
        skill = alloc_perm((unsigned int)sizeof(*skill));
    } else {
        skill = skill_free;
        skill_free = skill_free->next;
    }

    *skill = skill_zero;

    VALIDATE(skill);
    return skill;
}

/**
 * free the skill structure
 */
void free_skill(SKILL *skill)
{
    if (!IS_VALID(skill))
        return;

    INVALIDATE(skill);
    if (skill->name != NULL)
        free_string(skill->name);

    if (skill->help_keyword != NULL)
        free_string(skill->help_keyword);

    if (skill->msg != NULL)
        free_string(skill->msg);

    if (skill->msg_obj != NULL)
        free_string(skill->msg_obj);

    if (skill->msg_others != NULL)
        free_string(skill->msg_others);

    skill->next = skill_free;
    skill_free = skill;
}

/**
 * create a new spell list
 */
SPELL_LIST *new_spell_list(void)
{
    static SPELL_LIST spells_zero;
    SPELL_LIST *spells;

    if (spell_list_free == NULL) {
        spells = alloc_perm((unsigned int)sizeof(*spells));
    } else {
        spells = spell_list_free;
        spell_list_free = spell_list_free->next;
    }

    *spells = spells_zero;

    VALIDATE(spells);
    return spells;
}

/**
 * free the spell list structure
 */
void free_spell_list(SPELL_LIST *spells)
{
    if (!IS_VALID(spells))
        return;

    INVALIDATE(spells);
    spells->next = spell_list_free;
    spell_list_free = spells;
}

/**
 * create a new affect list
 */
AFFECT_LIST *new_affect_list(void)
{
    static AFFECT_LIST affects_zero;
    AFFECT_LIST *affects;

    if (affect_list_free == NULL) {
        affects = alloc_perm((unsigned int)sizeof(*affects));
    } else {
        affects = affect_list_free;
        affect_list_free = affect_list_free->next;
    }

    *affects = affects_zero;

    VALIDATE(affects);
    return affects;
}

/**
 * free the affect list structure
 */
void free_affect_list(AFFECT_LIST *affects)
{
    if (!IS_VALID(affects))
        return;

    INVALIDATE(affects);
    affects->next = affect_list_free;
    affect_list_free = affects;
}

/**
 * create a new argument structure
 */
ARGUMENT *new_argument(void)
{
    static ARGUMENT arg_zero;
    ARGUMENT *arg;

    if (spell_list_free == NULL) {
        arg = alloc_perm((unsigned int)sizeof(*arg));
    } else {
        arg = argument_free;
        argument_free = argument_free->next;
    }

    *arg = arg_zero;

    VALIDATE(arg);
    return arg;
}

/**
 * free the argument structure
 */
void free_argument(ARGUMENT *argument)
{
    if (!IS_VALID(argument))
        return;

    INVALIDATE(argument);
    free_string(argument->key);
    if (argument->data != NULL)
        free_variant(argument->data);

    argument->next = argument_free;
    argument_free = argument;
}

/**
 * add a spell to a spell list for a skill
 */
void add_spell(SKILL *skill, SPELL_FUN *spell)
{
    SPELL_LIST *spells;

    if (spell != NULL) {
        spells = new_spell_list();
        spells->spell_fn = spell;

        if (skill->spells == NULL) {
            spells->next = skill->spells;
            skill->spells = spells;
        } else {
            SPELL_LIST *spells_idx;

            for (spells_idx = skill->spells;
                 spells_idx->next != NULL;
                 spells_idx = spells_idx->next) {
                /* just get the last item in the list */
            }
            if (spells_idx != NULL && spells_idx->next == NULL) {
                spells_idx->next = spells;
                spells->next = NULL;
            }
        }
    }
}

/**
 * add a affect to an affect list for a skill
 */
void add_affect(SKILL *skill, AFFECT_FUN *affect)
{
    AFFECT_LIST *affects;

    if (affect != NULL) {
        affects = new_affect_list();
        affects->affect_fn = affect;

        if (skill->affects == NULL) {
            affects->next = skill->affects;
            skill->affects = affects;
        } else {
            AFFECT_LIST *affects_idx;

            for (affects_idx = skill->affects;
                 affects_idx->next != NULL;
                 affects_idx = affects_idx->next) {
                /* just get the last item in the list */
            }
            if (affects_idx != NULL && affects_idx->next == NULL) {
                affects_idx->next = affects;
                affects->next = NULL;
            }
        }
    }
}

/**
 * add an argument to the list
 */
void add_argument(SKILL *skill, ARGUMENT *arg)
{
    if (arg != NULL) {
        if (skill->args == NULL) {
            arg->next = skill->args;
            skill->args = arg;
        } else {
            ARGUMENT *arg_idx;

            for (arg_idx = skill->args;
                 arg_idx->next != NULL;
                 arg_idx = arg_idx->next) {
                /* just get the last item in the list */
            }
            if (arg_idx != NULL && arg_idx->next == NULL) {
                arg_idx->next = arg;
                arg->next = NULL;
            }
        }
    }
}

/**
 * find an argument in the list
 */
VARIANT *find_argument(ARGUMENT *argument, char *key)
{
    ARGUMENT *arg;

    for (arg = argument; arg != NULL; arg = arg->next)
        if (!str_cmp(arg->key, key))
            return arg->data;
    return NULL;
}

const struct spell_lookup_type spell_lookup_table[] =
{
    { "acid blast", spell_acid_blast },
    { "acidic rain", spell_acidic_rain },
    { "armor", spell_armor },
    { "bless", spell_bless },
    { "blindness", spell_blindness },
    { "burning hands", spell_burning_hands },
    { "call lightning", spell_call_lightning },
    { "calm", spell_calm },
    { "cancellation", spell_cancellation },
    { "cause critical", spell_cause_critical },
    { "cause light", spell_cause_light },
    { "cause serious", spell_cause_serious },
    { "change sex", spell_change_sex },
    { "chain lightning", spell_chain_lightning },
    { "charm person", spell_charm_person },
    { "chill touch", spell_chill_touch },
    { "cloud of death", spell_cloud_of_death },
    { "colour spray", spell_colour_spray },
    { "continual light", spell_continual_light },
    { "control weather", spell_control_weather },
    { "create water", spell_create_water },
    { "cure blindness", spell_cure_blindness },
    { "cure critical", spell_cure_critical },
    { "cure disease", spell_cure_disease },
    { "cure light", spell_cure_light },
    { "cure poison", spell_cure_poison },
    { "cure serious", spell_cure_serious },
    { "curse", spell_curse },
    { "darkness", spell_darkness },
    { "demonfire", spell_demonfire },
    { "detect evil", spell_detect_evil },
    { "detect good", spell_detect_good },
    { "detect invis", spell_detect_invis },
    { "detect magic", spell_detect_magic },
    { "detect poison", spell_detect_poison },
    { "dispel evil", spell_dispel_evil },
    { "dispel good", spell_dispel_good },
    { "dispel magic", spell_dispel_magic },
    { "earthquake", spell_earthquake },
    { "enchant armor", spell_enchant_armor },
    { "enchant weapon", spell_enchant_weapon },
    { "energy drain", spell_energy_drain },
    { "extinguish flames", spell_extinguish_flames },
    { "faerie fire", spell_faerie_fire },
    { "faerie fog", spell_faerie_fog },
    { "farsight", spell_farsight },
    { "fireball", spell_fireball },
    { "fireblade", spell_fireblade },
    { "fireproof", spell_fireproof },
    { "flamestrike", spell_flamestrike },
    { "floating disc", spell_floating_disc },
    { "fly", spell_fly },
    { "freeze bolt", spell_freeze_bolt },
    { "freeze storm", spell_freeze_storm },
    { "frenzy", spell_frenzy },
    { "frost hands", spell_frost_hands },
    { "gate", spell_gate },
    { "giant strength", spell_giant_strength },
    { "grandeur", spell_grandeur },
    { "harm", spell_harm },
    { "haste", spell_haste },
    { "heal", spell_heal },
    { "heal mana", spell_heal_mana },
    { "holy word", spell_holy_word },
    { "identify", spell_identify },
    { "infravision", spell_infravision },
    { "invis", spell_invis },
    { "know alignment", spell_know_alignment },
    { "lightning bolt", spell_lightning_bolt },
    { "locate object", spell_locate_object },
    { "magic missile", spell_magic_missile },
    { "mass healing", spell_mass_healing },
    { "mass invis", spell_mass_invis },
    { "meteor storm", spell_meteor_storm },
    { "monsoon", spell_monsoon },
    { "nexus", spell_nexus },
    { "pass door", spell_pass_door },
    { "plague", spell_plague },
    { "pollenburst", spell_pollenburst },
    { "poison", spell_poison },
    { "portal", spell_portal },
    { "protection evil", spell_protection_evil },
    { "protection good", spell_protection_good },
    { "ray of truth", spell_ray_of_truth },
    { "recharge", spell_recharge },
    { "refresh", spell_refresh },
    { "remove curse", spell_remove_curse },
    { "revive", spell_revive },
    { "ring of fire", spell_ring_of_fire },
    { "sanctuary", spell_sanctuary },
    { "super speed", spell_super_speed },
    { "druid call", spell_druid_call },
    { "anti magic aura", spell_anti_magic_aura },
    { "shocking grasp", spell_shocking_grasp },
    { "shield", spell_shield },
    { "sleep", spell_sleep },
    { "slow", spell_slow },
    { "stone skin", spell_stone_skin },
    { "thorns", spell_thorns },
    { "ventriloquate", spell_ventriloquate },
    { "weaken", spell_weaken },
    { "timewarp", spell_timewarp },
    { "winds", spell_winds },
    { "acid breath", spell_acid_breath },
    { "fire breath", spell_fire_breath },
    { "frost breath", spell_frost_breath },
    { "gas breath", spell_gas_breath },
    { "lightning breath", spell_lightning_breath },
    { "general purpose", spell_general_purpose },
    { "high explosive", spell_high_explosive },
    { "blood boil", spell_blood_boil },
    { "black plague", spell_black_plague },
    { "black mantle", spell_black_mantle },
    { "equipment invis", spell_equipment_invis },
    { "noremove", spell_noremove },
    { "cure blood", spell_cure_blood },
    { "fear", spell_fear },
    { "shatter curse", spell_shatter_curse },
    { "web", spell_web },
    { "displacement", spell_displacement },
    { "haven", spell_haven },
    { "parasitic cloud", spell_parasitic_cloud },
    { "mana vortex", spell_mana_vortex },
    { "",                   NULL },
};

/**
 * affect_lookup_table
 */
const struct affect_lookup_type affect_lookup_table[] =
{
    { "displacement",    affect_displacement },
    { "parasitic cloud", affect_parasitic_cloud },
    { "black plague",    affect_black_plague },
    { "disease",         affect_disease },
    { "burning flames",  affect_burning_flames },
    { "poison",         affect_poison },
    { "",             NULL },
};

/**
 * lookup a spell function by name
 * used for reading persisted spells from disk
 */
SPELL_FUN *spell_fn_lookup(char *name)
{
    int idx;

    for (idx = 0; spell_lookup_table[idx].name[0] != '\0'; idx++)
        if (!str_prefix(name, spell_lookup_table[idx].name))
            return spell_lookup_table[idx].fn;

    return NULL;
}

/**
 *    gets a spell name by function pointer
 *    used for persisting the spells to disk
 */
char *spell_fn_name(SPELL_FUN *fn)
{
    int idx;

    for (idx = 0; spell_lookup_table[idx].name[0] != '\0'; idx++)
        if (spell_lookup_table[idx].fn == fn)
            return spell_lookup_table[idx].name;

    return "";
}

/**
 * lookup a affect function by name
 * used for reading persisted affect functions from disk
 */
AFFECT_FUN *affect_fn_lookup(char *name)
{
    int idx;

    for (idx = 0; affect_lookup_table[idx].name[0] != '\0'; idx++)
        if (!str_prefix(name, affect_lookup_table[idx].name))
            return affect_lookup_table[idx].fn;

    return NULL;
}

/**
 * gets an affect name by function pointer
 * used for persisting the skill affects to disk
 */
char *affect_fn_name(AFFECT_FUN *fn)
{
    int idx;

    for (idx = 0; affect_lookup_table[idx].name[0] != '\0'; idx++)
        if (affect_lookup_table[idx].fn == fn)
            return affect_lookup_table[idx].name;

    return "";
}

/**
 * lookup a dynamic skill - try to get a skill that is as
 * acurate as possible
 */
SKILL *skill_lookup(char *name)
{
    SKILL *skill;
    SKILL *skill_tmp;

    if (name[0] == '\0')
        return NULL;

    skill_tmp = NULL;
    for (skill = skill_list; skill != NULL; skill = skill->next) {
        /* short circuit the loop if we know that they dont match */
        if (name[0] != skill->name[0])
            continue;

        /* if we have an exact match on name, then return it */
        if (!str_cmp(name, skill->name))
            return skill;

        /* if we have a partial name then we need to do some funk */
        if (skill_tmp == NULL && !str_prefix(name, skill->name))
            /* set a temporary variable */
            skill_tmp = skill;

    }

    return skill_tmp;
}

/**
 * lookup a dynamic group
 */
GROUP *group_lookup(char *name)
{
    GROUP *group;
    GROUP *group_tmp;

    if (name[0] == '\0')
        return NULL;

    group_tmp = NULL;
    for (group = group_list; group != NULL; group = group->next) {
        if (group->name[0] != name[0])
            continue;

        if (!str_cmp(name, group->name))
            return group;

        if (group_tmp == NULL && !str_prefix(name, group->name))
            group_tmp = group;
    }

    return group_tmp;
}

/**
 * lookup a dynamic skill
 */
SKILL *resolve_skill_sn(int sn)
{
    SKILL *skill;

    if (sn > 0
        && sn <= gn_max_skill_sn
        && gsp_skill_hash[sn]) {
        return gsp_skill_hash[sn];
    } else {
        for (skill = skill_list; skill != NULL; skill = skill->next)
            if (skill->sn == sn)
                return skill;
    }

    return NULL;
}

/**
 * resolve a skill from an affect - skills are cached on the
 * affect structure so we dont always have to look it up
 */
SKILL *resolve_skill_affect(AFFECT_DATA *paf)
{
    if (paf->skill == NULL)
        paf->skill = resolve_skill_sn(paf->type);

    return paf->skill;
}

/**
 * get the level of a skill for a given class
 */
LEVEL_INFO *get_skill_level(CHAR_DATA *ch, SKILL *skill)
{
    LEVEL_INFO *levels;

    if (IS_NPC(ch))
        return NULL;

    for (levels = skill->levels; levels != NULL; levels = levels->next)
        if (levels->class == ch->class)
            return levels;

    return NULL;
}

/**
 * get the level of a group for a given class
 */
LEVEL_INFO *get_group_level(GROUP *group, int cls)
{
    LEVEL_INFO *levels;

    for (levels = group->levels; levels != NULL; levels = levels->next)
        if (levels->class == cls)
            return levels;

    return NULL;
}

/**
 * get a learned group
 */
LEARNED *get_learned_group(CHAR_DATA *ch, GROUP *group)
{
    LEARNED *learned;

    if (IS_NPC(ch))
        return NULL;

    for (learned = ch->pcdata->skills;
         learned != NULL;
         learned = learned->next)
        if (learned->group == group && learned->type == LEARNED_TYPE_GROUP)
            return learned;

    return NULL;
}

/**
 * get a learned skill
 */
LEARNED *get_learned_skill(CHAR_DATA *ch, SKILL *skill)
{
    LEARNED *learned;

    if (IS_NPC(ch))
        return NULL;

    if (skill != NULL) {
        for (learned = ch->pcdata->skills;
             learned != NULL;
             learned = learned->next) {
            if (learned->skill == skill
                && learned->type == LEARNED_TYPE_SKILL)
                return learned;
        }
    }

    return NULL;
}

/**
 * find a learned group on a character
 */
LEARNED *get_learned(CHAR_DATA *ch, char *name)
{
    LEARNED *learned;

    if (IS_NPC(ch))
        return NULL;

    for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
        if ((learned->skill != NULL && !str_cmp(name, learned->skill->name))
            || (learned->group != NULL && !str_cmp(name, learned->group->name)))
            return learned;
    }

    return NULL;
}

/**
 * add a learned skill to a characters pcdata
 */
void add_learned(CHAR_DATA *ch, LEARNED *learned)
{
    if (learned == NULL)
        return;

    switch (learned->type) {
    case LEARNED_TYPE_SKILL:
        add_learned_skill(ch, learned);
        break;
    case LEARNED_TYPE_GROUP:
        add_learned_group(ch, learned);
        break;
    default:
        log_bug("Invalid learned type: %s",
               (learned->skill != NULL) ? learned->skill->name :
               (learned->group != NULL) ? learned->group->name : "unknown");
    }
}

/**
 * add a learned skill
 */
void add_learned_skill(CHAR_DATA *ch, LEARNED *learned)
{
    LEARNED *learned_idx;

    if (learned->skill == NULL
        || learned->type != LEARNED_TYPE_SKILL)
        return;

    if (ch->pcdata->skills == NULL) {
        ch->pcdata->skills = learned;
        return;
    }

    learned_idx = get_learned_skill(ch, learned->skill);
    if (learned_idx == NULL) {
        LEARNED *last_skill;
        LEVEL_INFO *level;
        LEVEL_INFO *level_idx;

        /* we have a skill - insert it at the right place
         * in the list */
        level = get_skill_level(ch, learned->skill);

        /* the skill is out of level for the character - drop it */
        if (level == NULL) {
            free_learned(learned);
            return;
        }

        last_skill = NULL;
        for (learned_idx = ch->pcdata->skills;
             learned_idx != NULL;
             learned_idx = learned_idx->next) {
            if (learned_idx->next == learned_idx)
                break;

            if (learned_idx->skill != NULL
                && learned_idx->type == LEARNED_TYPE_SKILL) {
                level_idx = get_skill_level(ch, learned_idx->skill);
                if (level_idx != NULL
                    && level_idx->level >= level->level) {
                    /* reset the previous item in the list */
                    if (learned_idx->prev != NULL) {
                        LEARNED *prev;

                        prev = learned_idx->prev;
                        prev->next = learned;
                        learned->prev = prev;
                    }

                    /*
                     * if the index is the first skill in
                     * the list, then we need to insert
                     * the new skill at the beginning
                     */

                    if (ch->pcdata->skills == learned_idx)
                        ch->pcdata->skills = learned;

                    /* add the learned */
                    learned->next = learned_idx;
                    learned_idx->prev = learned;

                    /* return, exiting the loop */
                    return;
                }

                last_skill = learned_idx;
            }
        }

        /* if we make it to here, then we havent added it
         * yet...so add it after the last skill */
        if (last_skill != NULL) {
            /* the last skill is also the first skill */
            if (last_skill == ch->pcdata->skills)
                ch->pcdata->skills = learned;

            /* realistically, this case should never happen */
            if (last_skill->next != NULL) {
                LEARNED *next;

                next = last_skill->next;
                learned->next = next;
                next->prev = learned;
            }

            /* link the new skill */
            last_skill->next = learned;
            learned->prev = last_skill;
        }
    } else {
        /* we have an existing skill - update the percent */
        learned_idx->percent = learned->percent;
        free_learned(learned);
    }
}

/**
 * add a learned group
 */
void add_learned_group(CHAR_DATA *ch, LEARNED *learned)
{
    LEARNED *learned_idx;
    LEARNED *learned_new;
    SKILL_LIST *list;


    if (learned->group != NULL
        && learned->type == LEARNED_TYPE_GROUP) {
        if (ch->pcdata->skills == NULL) {
            ch->pcdata->skills = learned;
        } else {
            /* it is a learned group - add it to the end */
            for (learned_idx = ch->pcdata->skills;
                 learned_idx->next != NULL;
                 learned_idx = learned_idx->next) {
                /* just move to the end */
            }

            if (learned_idx != NULL
                && learned_idx->next == NULL) {
                learned_idx->next = learned;
                learned->prev = learned_idx;
            }
        }

        for (list = learned->group->skills;
             list != NULL;
             list = list->next) {
            if (list->next == list)
                break;


            if (list->skill != NULL
                && (learned_new = get_learned_skill(ch, list->skill)) == NULL) {
                learned_new = new_learned();
                learned_new->skill = list->skill;
                learned_new->percent = 1;
                learned_new->type = LEARNED_TYPE_SKILL;
                learned_new->group = learned->group;
                add_learned(ch, learned_new);
            }
        }
    }
}

/**
 * remove a learned skill to a characters pcdata
 */
void remove_learned(CHAR_DATA *ch, LEARNED *learned)
{
    LEARNED *learned_idx;
    LEARNED *learned_next;
    LEARNED *learned_prev;

    if (learned == NULL
        || ch->pcdata == NULL
        || ch->pcdata->skills == NULL)
        return;

    if (ch->pcdata->skills == learned) {
        ch->pcdata->skills = learned->next;
        ch->pcdata->skills->prev = learned->prev;
    } else {
        /* make sure we already have the learned data */
        for (learned_idx = ch->pcdata->skills;
             learned_idx != NULL;
             learned_idx = learned_next) {
            learned_next = learned_idx->next;
            learned_prev = learned_idx->prev;

            if (learned_idx == learned) {
                /* found the learned data on the character -
                 * unlink it */
                if (learned_prev != NULL)
                    learned_prev->next = learned_next;
                if (learned_next != NULL)
                    learned_next->prev = learned_prev;
                break;
            }
        }
    }

    /* if we have a group, we need to drop all of the
     * individual spells/skills */
    if (learned->group != NULL && learned->type == LEARNED_TYPE_GROUP) {
        SKILL_LIST *skills;

        for (skills = learned->group->skills;
             skills != NULL;
             skills = skills->next) {
            if (skills->skill != NULL) {
                learned_idx = get_learned_skill(ch, skills->skill);
                if (learned_idx != NULL
                    && learned_idx->type == LEARNED_TYPE_SKILL
                    && learned_idx->skill == skills->skill
                    && learned_idx->group == learned->group)
                    remove_learned(ch, learned_idx);
            }
        }
    }

    /* recycle the learned structure */
    free_learned(learned);
}

/**
 * create a learned group by name
 */
LEARNED *create_learned_group(char *name)
{
    GROUP *group;
    LEARNED *learned;

    group = group_lookup(name);
    if (group != NULL) {
        learned = new_learned();
        learned->group = group;
        learned->type = LEARNED_TYPE_GROUP;

        return learned;
    }

    return NULL;
}

/**
 * create a learned skill by name
 */
LEARNED *create_learned_skill(char *name, int percent)
{
    SKILL *skill;
    LEARNED *learned;

    skill = skill_lookup(name);
    if (skill != NULL) {
        learned = new_learned();
        learned->skill = skill;
        learned->percent = percent;
        learned->type = LEARNED_TYPE_SKILL;

        return learned;
    }

    return NULL;
}

/**
 * determine whether a character is affected by a skill by name
 */
bool check_affected(CHAR_DATA *ch, char *name)
{
    SKILL *skill;
    AFFECT_DATA *af;
    bool is_affected;

    is_affected = false;

    skill = skill_lookup(name);
    if (skill != NULL) {
        for (af = ch->affected; af != NULL; af = af->next)
            if (af->type == skill->sn)
                return true;
    }

    return is_affected;
}

/**
 * get the percentage which the character knows a skill
 */
int get_learned_percent(CHAR_DATA *ch, SKILL *skill)
{
    LEARNED *learned;
    LEVEL_INFO *level;
    int pcnt = 0;

    if (skill == NULL) {
        pcnt = 0;
    } else if (IS_NPC(ch)) {
        pcnt = 0;
        if (IS_SET(ch->act, skill->act_flag))
            pcnt = skill->percent;
        else if (IS_SET(ch->off_flags, skill->off_flag))
            pcnt = skill->percent;
        else
            pcnt = skill->percent;
    } else if (IS_IMMORTAL(ch)) {
        pcnt = 100;
    } else {
        learned = get_learned_skill(ch, skill);
        level = get_skill_level(ch, skill);

        if (learned != NULL && level != NULL && level->level <= ch->level)
            pcnt = learned->percent;
    }

    return pcnt;
}

/**
 * get a skill number for a skill by name
 */
int get_skill_number(char *name)
{
    SKILL *skill;

    if ((skill = skill_lookup(name)) != NULL)
        return skill->sn;

    return -1;
}

/**
 * add a skill to a group
 */
void add_group_skill(GROUP *group, SKILL *skill)
{
    SKILL_LIST *list;


    list = new_skill_list();
    list->skill = skill;

    if (group->skills != NULL) {
        SKILL_LIST *list_ex;

        list_ex = group->skills;
        list_ex->prev = list;
        list->next = group->skills;
    }

    group->skills = list;
}

/**
 * add a level information structure to a skill
 */
void add_skill_level(SKILL *skill, LEVEL_INFO *level)
{
    LEVEL_INFO *level_idx;
    LEVEL_INFO *level_prev;

    if (skill == NULL || level == NULL)
        return;

    if (skill->levels == NULL || skill->levels->class > level->class) {
        level->next = skill->levels;
        skill->levels = level;
    } else {
        level_prev = NULL;

        for (level_idx = skill->levels; level_idx != NULL; level_idx = level_idx->next) {
            if (level_idx->class > level->class)
                break;
            level_prev = level_idx;
        }

        if (level_prev != NULL) {
            level_prev->next = level;
            level->next = level_idx;
        }
    }
}

/**
 * add a level information structure to a group
 */
void add_group_level(GROUP *group, LEVEL_INFO *level)
{
    LEVEL_INFO *level_idx;
    LEVEL_INFO *level_prev;

    if (group == NULL || level == NULL)
        return;

    if (group->levels == NULL || group->levels->class > level->class) {
        level->next = group->levels;
        group->levels = level;
    } else {
        level_prev = NULL;

        for (level_idx = group->levels; level_idx != NULL; level_idx = level_idx->next) {
            if (level_idx->class > level->class)
                break;
            level_prev = level_idx;
        }

        if (level_prev != NULL) {
            level_prev->next = level;
            level->next = level_idx;
        }
    }
}


static void gain_list(CHAR_DATA * ch, CHAR_DATA * trainer);
static void gain_convert(CHAR_DATA * ch, CHAR_DATA * trainer);
static void gain_study(CHAR_DATA * ch, CHAR_DATA * trainer);
static void gain_points(CHAR_DATA * ch, CHAR_DATA * trainer);
static void gain_group(CHAR_DATA * ch, CHAR_DATA * trainer, GROUP * group);
static void gain_skill(CHAR_DATA * ch, CHAR_DATA * trainer, SKILL * skill);


/**
 * display a list of gain-able skills
 */
static void gain_list(CHAR_DATA *ch, CHAR_DATA *trainer)
{
    GROUP *group;
    SKILL *skill;
    LEVEL_INFO *level;
    LEARNED *learned;
    int col;

    col = 0;
    printf_to_char(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
               "group", "cost", "group", "cost", "group", "cost");

    for (group = group_list; group != NULL; group = group->next) {
        level = get_group_level(group, ch->class);

        if (level != NULL
            && level->difficulty > 0
            && (learned = get_learned_group(ch, group)) == NULL) {
            printf_to_char(ch, "%-18s %-5d ",
                       group->name,
                       level->difficulty);

            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }

    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    printf_to_char(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
               "skill", "cost", "skill", "cost", "skill", "cost");

    for (skill = skill_list; skill != NULL; skill = skill->next) {
        level = get_skill_level(ch, skill);

        if (level != NULL
            && level->difficulty > 0
            && ((learned = get_learned_skill(ch, skill)) == NULL)
            && skill->spells == NULL) {
            printf_to_char(ch, "%-18s %-5d ",
                       skill->name, level->difficulty);

            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);
}

/**
 *    convert practices to trains
 */
static void gain_convert(CHAR_DATA *ch, CHAR_DATA *trainer)
{
    if (ch->pcdata->practice < 10) {
        act("$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    act("$N helps you apply your practice to training.", ch, NULL, trainer, TO_CHAR);
    ch->pcdata->practice -= 10;
    ch->pcdata->train += 1;
}

/**
 * convert trains to practices
 */
static void gain_study(CHAR_DATA *ch, CHAR_DATA *trainer)
{
    if (ch->pcdata->train < 1) {
        act("$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    act("$N helps you apply your trains to practicing.", ch, NULL, trainer, TO_CHAR);
    ch->pcdata->train -= 1;
    ch->pcdata->practice += 10;
}

/**
 * lower the number of creation points - lowers exp per level
 */
static void gain_points(CHAR_DATA *ch, CHAR_DATA *trainer)
{
    if (ch->pcdata->train < 2) {
        act("$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    if (ch->pcdata->points <= 40) {
        act("$N tells you 'There would be no point in that.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    act("$N trains you, and you feel more at ease with your skills.", ch, NULL, trainer, TO_CHAR);

    ch->pcdata->train -= 2;
    ch->pcdata->points -= 1;
    ch->exp = exp_per_level(ch, ch->pcdata->points) * ch->level;
}

/**
 * gain a group
 */
static void gain_group(CHAR_DATA *ch, CHAR_DATA *trainer, GROUP *group)
{
    LEVEL_INFO *level;
    LEARNED *learned;

    if (group == NULL) {
        act("$N tells you 'You cannot learn that group.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    learned = get_learned_group(ch, group);
    level = get_group_level(group, ch->class);
    if (learned != NULL) {
        act("$N tells you 'You already know that group!'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    if (level == NULL
        || level->difficulty <= 0
        || level->level <= 0) {
        act("$N tells you 'That group is beyond your powers.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    if (ch->pcdata->train < level->difficulty) {
        act("$N tells you 'You are not yet ready for that group.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    learned = new_learned();
    learned->group = group;
    learned->percent = 1;
    learned->type = LEARNED_TYPE_GROUP;


    add_learned(ch, learned);
    act("$N trains you in the art of $t", ch, group->name, trainer, TO_CHAR);
    ch->pcdata->train -= level->difficulty;
}

/**
 *    gain a skill
 */
static void gain_skill(CHAR_DATA *ch, CHAR_DATA *trainer, SKILL *skill)
{
    LEVEL_INFO *level;
    LEARNED *learned;

    if (skill == NULL) {
        act("$N tells you 'You cannot learn that skill.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    if (skill->spells != NULL) {
        act("$N tells you 'You must learn the full group.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    learned = get_learned_skill(ch, skill);
    level = get_skill_level(ch, skill);

    if (learned != NULL) {
        act("$N tells you 'You already know that skill!'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    if (level == NULL
        || level->difficulty <= 0) {
        act("$N tells you 'That skill is beyond your powers.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    if (ch->pcdata->train < level->difficulty) {
        act("$N tells you 'You are not yet ready for that skill.'", ch, NULL, trainer, TO_CHAR);
        return;
    }

    learned = new_learned();
    learned->skill = skill;
    learned->percent = 1;
    learned->type = LEARNED_TYPE_SKILL;

    add_learned(ch, learned);

    act("$N trains you in the art of $t.", ch, skill->name, trainer, TO_CHAR);
    ch->pcdata->train -= level->difficulty;
}

/**
 * used to gain new skills
 */
void do_gain(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *trainer;
    GROUP *group;
    SKILL *skill;
    char arg[MIL];

    if (IS_NPC(ch))
        return;

    /* find a trainer */
    for (trainer = ch->in_room->people;
         trainer != NULL;
         trainer = trainer->next_in_room) {
        if (IS_NPC(trainer)
            && IS_SET(trainer->act, ACT_GAIN))
            break;
    }

    if (trainer == NULL || !can_see(ch, trainer)) {
        send_to_char("You can't do that here.\n\r", ch);
        return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        broadcast_channel(trainer, channels_find(CHANNEL_SAY), NULL, "Pardon me?");
        return;
    }

    if (!str_prefix(arg, "list")) {
        gain_list(ch, trainer);
        return;
    }

    if (!str_prefix(arg, "convert")) {
        gain_convert(ch, trainer);
        return;
    }

    if (!str_prefix(arg, "study")) {
        gain_study(ch, trainer);
        return;
    }

    if (!str_prefix(arg, "points")) {
        gain_points(ch, trainer);
        return;
    }

    group = group_lookup(argument);
    if (group != NULL) {
        gain_group(ch, trainer, group);
        return;
    }


    skill = skill_lookup(argument);
    if (skill != NULL) {
        gain_skill(ch, trainer, skill);
        return;
    }

    act("$N tells you 'I do not understand...'", ch, NULL, trainer, TO_CHAR);
}

/**
 * show information for a skill
 */
static void skill_info(CHAR_DATA *ch, SKILL *skill)
{
    LEVEL_INFO *level;
    char buf[MSL];

    printf_to_char(ch, "\n\r%-17.17s %s\n\r", (skill->spells == NULL) ? "`@Skill`2:``" : "`1Spell`!:``", capitalize(skill->name));
    if (skill->spells != NULL) {
        send_to_char("`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r", ch);
        printf_to_char(ch, "Brewable:   %s\n\r", (IS_SET(skill->flags, SPELL_NOBREW)) ? "`1No``" : "`@Yes``");
        printf_to_char(ch, "Scribable:  %s\n\r", (IS_SET(skill->flags, SPELL_NOSCRIBE)) ? "`1No``" : "`@Yes``");
        if (IS_IMMORTAL(ch))
            printf_to_char(ch, "Slot:       %d\n\r", skill->sn);
    }

    send_to_char("\n\rClass       Level\n\r", ch);
    send_to_char("`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r", ch);
    for (level = skill->levels; level != NULL; level = level->next) {
        sprintf(buf, "%s:``", capitalize(class_table[level->class].name));
        printf_to_char(ch, "%-13.13s %d\n\r", buf, level->level);
    }

    /* we need to put help info in here */
    if (skill->help != NULL) {
        send_to_char("\n\r`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r\n\r", ch);
        page_to_char(skill->help->text, ch);
    }
}

/**
 * used by do_skills and do_spells to parse the numeric
 * values out of an argument list to get a max level and a min level
 */
static bool parse_levels(char *argument, int *min_level, int *max_level)
{
    char arg[MIL];

    argument = one_argument(argument, arg);
    if (!is_number(arg))
        return false;

    *max_level = parse_int(arg);
    if (argument[0] != '\0') {
        int tmp;

        argument = one_argument(argument, arg);
        if (!is_number(arg))
            return false;

        *min_level = *max_level;
        *max_level = parse_int(arg);
        if (*min_level > *max_level) {
            tmp = *min_level;
            *min_level = *max_level;
            *max_level = tmp;
        }
    }

    return true;
}

/**
 * display a list of skills known
 */
void do_skills(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf;
    LEARNED *learned;
    SKILL *skill;
    LEVEL_INFO *level;
    int min_level;
    int max_level;
    int last_level;
    int col;

    if (IS_NPC(ch))
        return;

    min_level = 1;
    max_level = UMIN(ch->level, MAX_LEVEL);

    if (argument[0] != '\0') {
        if (!str_prefix(argument, "all")) {
            max_level = (IS_IMMORTAL(ch)) ? MAX_LEVEL : LEVEL_HERO;
        } else {
            if ((skill = skill_lookup(argument)) != NULL) {
                skill_info(ch, skill);
                return;
            } else {
                if (!parse_levels(argument, &min_level, &max_level)) {
                    send_to_char("Arguments must be numerical or all.\n\r", ch);
                    return;
                }
            }       /* skill info */
        }               /* !skill "all" */
    }                       /* argument[0] != '\0' */


    buf = new_buf();
    last_level = 0;
    col = 0;

    for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
        if (learned->skill != NULL) {
            level = get_skill_level(ch, learned->skill);

            if (level != NULL
                && level->level >= min_level
                && level->level <= max_level
                && learned->skill->spells == NULL) {
                if (last_level != level->level) {
                    printf_buf(buf, "\n\r`3Level `#%3d`*:`` ", level->level);
                    col = 0;
                } else {
                    if (++col % 2 == 0)
                        add_buf(buf, "\n\r           ");
                }

                if (ch->level < level->level)
                    printf_buf(buf, "%-18s  `1n/a``      ", learned->skill->name);
                else
                    printf_buf(buf, "%-18s `O%3d%%``      ", learned->skill->name,
                           learned->percent);

                last_level = level->level;
            }
        }
    }


    add_buf(buf, "\n\r");
    page_to_char(buf_string(buf), ch);
    free_buf(buf);
}

/**
 * display a list of spells known
 */
void do_spells(CHAR_DATA *ch, char *argument)
{
    BUFFER *buf;
    LEARNED *learned;
    SKILL *skill;
    LEVEL_INFO *level;
    int min_level;
    int max_level;
    int last_level;
    int col;


    if (IS_NPC(ch))
        return;

    min_level = 1;
    max_level = UMIN(ch->level, MAX_LEVEL);

    if (argument[0] != '\0') {
        if (!str_prefix(argument, "all")) {
            max_level = (IS_IMMORTAL(ch)) ? MAX_LEVEL : LEVEL_HERO;
        } else {
            if ((skill = skill_lookup(argument)) != NULL) {
                skill_info(ch, skill);
                return;
            } else {
                if (!parse_levels(argument, &min_level, &max_level)) {
                    send_to_char("Arguments must be numerical or all.\n\r", ch);
                    return;
                }
            }       /* skill info */
        }               /* !skill "all" */
    }                       /* argument[0] != '\0' */


    buf = new_buf();
    last_level = 0;
    col = 0;

    for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
        if (learned->skill != NULL) {
            level = get_skill_level(ch, learned->skill);

            if (level != NULL
                && level->level >= min_level
                && level->level <= max_level
                && learned->skill->spells != NULL) {
                if (last_level != level->level) {
                    printf_buf(buf, "\n\r`3Level `#%3d`*:`` ", level->level);
                    col = 0;
                } else {
                    if (++col % 2 == 0)
                        add_buf(buf, "\n\r           ");
                }

                if (ch->level < level->level) {
                    printf_buf(buf, "%-18s  `!n/a``     ", learned->skill->name);
                } else {
                    printf_buf(buf, "%-18s `2%4d `@mana `O%3d%%``  ", learned->skill->name,
                           learned->skill->min_mana, learned->percent);
                }

                last_level = level->level;
            }
        }
    }


    add_buf(buf, "\n\r");
    page_to_char(buf_string(buf), ch);
    free_buf(buf);
}

/**
 * lists groups/skills that have not been learned and their
 * associated costs
 */
void list_group_costs(CHAR_DATA *ch)
{
    GROUP *group;
    SKILL *skill;
    LEVEL_INFO *level;
    LEARNED *learned;
    int col;

    col = 0;
    printf_to_char(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cp", "group", "cp", "group", "cp");

    for (group = group_list; group != NULL; group = group->next) {
        level = get_group_level(group, ch->class);

        if (level != NULL
            && level->difficulty > 0
            && (learned = get_learned_group(ch, group)) == NULL) {
            printf_to_char(ch, "%-18s %-5d ",
                       group->name,
                       level->difficulty);

            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }

    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    printf_to_char(ch, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cp", "skill", "cp", "skill", "cp");

    for (skill = skill_list; skill != NULL; skill = skill->next) {
        level = get_skill_level(ch, skill);

        if (level != NULL
            && level->difficulty > 0
            && ((learned = get_learned_skill(ch, skill)) == NULL)
            && skill->spells == NULL) {
            printf_to_char(ch, "%-18s %-5d ",
                       skill->name, level->difficulty);

            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0)
        send_to_char("\n\r", ch);

    printf_to_char(ch, "Creation points: %d\n\r", ch->pcdata->points);
    printf_to_char(ch, "Experience per level: %d\n\r", exp_per_level(ch, ch->pcdata->points));
}

/**
 * lists groups/skills that have been chosen and their
 * associated costs
 */
static void list_group_chosen(CHAR_DATA *ch)
{
    LEARNED *learned;
    LEVEL_INFO *level;
    int col;

    if (IS_NPC(ch))
        return;

    col = 0;
    printf_to_char(ch, "  %-18s %-5s   %-18s %-5s   %-18s %-5s\n\r", "group", "cp", "group", "cp", "group", "cp");

    for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
        if (learned->group != NULL && learned->type == LEARNED_TYPE_GROUP) {
            level = get_group_level(learned->group, ch->class);
            if (level != NULL && level->difficulty > 0) {
                printf_to_char(ch, "%c %-18s %-5d ",
                           (learned->removable) ? ' ' : '*',
                           learned->group->name,
                           level->difficulty);

                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }
    }

    if (col % 3 != 0)
        send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;
    printf_to_char(ch, "  %-18s %-5s   %-18s %-5s   %-18s %-5s\n\r", "skill", "cp", "skill", "cp", "skill", "cp");
    for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
        if (learned->skill != NULL && learned->skill->spells == NULL) {
            level = get_skill_level(ch, learned->skill);
            if (level != NULL && level->difficulty > 0) {
                printf_to_char(ch, "%c %-18s %-5d ",
                           (learned->removable) ? ' ' : '*',
                           learned->skill->name,
                           level->difficulty);

                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }
    }

    if (col % 3 != 0)
        send_to_char("\n\r", ch);

    send_to_char("\n\r", ch);

    printf_to_char(ch, "Creation points: %d\n\r", ch->pcdata->points);
    printf_to_char(ch, "Experience per level: %d\n\r", exp_per_level(ch, ch->pcdata->points));

    send_to_char("\n\rNOTE: Skills/Groups marked with a '*' cannot be dropped.\n\r", ch);
}

/**
 * calculate the experience per level
 */
int exp_per_level(CHAR_DATA *ch, int points)
{
    int expl;
    int inc;

    if (IS_NPC(ch))
        return 1000;

    expl = 1000;
    inc = 500;

    if (points < MIN_POINTS)
        return 1000 * (pc_race_table[ch->race].class_mult[ch->class] ? pc_race_table[ch->race].class_mult[ch->class] / 100 : 1);

    /* processing */
    points -= MIN_POINTS;
    while (points > 9) {
        expl += inc;
        points -= 10;
        if (points > 9) {
            expl += inc;
            inc *= 2;
            points -= 10;
        }
    }
    expl += points * inc / 10;

    return expl * pc_race_table[ch->race].class_mult[ch->class] / 100;
}

/**
 *    used during character creation to parse the skill/group
 *    adding/remove input
 */
bool parse_gen_groups(CHAR_DATA *ch, char *argument)
{
    GROUP *group;
    SKILL *skill;
    LEARNED *learned;
    LEVEL_INFO *level;
    char arg[MIL];

    if (argument[0] == '\0')
        return false;

    argument = one_argument(argument, arg);
    if (is_help(arg)) {
        if (argument[0] == '\0') {
            do_help(ch, "group help");
            return true;
        }

        do_help(ch, argument);
        return true;
    }

    if (!str_prefix(arg, "add")) {
        if (argument[0] == '\0') {
            send_to_char("You must provide a skill name.\n\r", ch);
            return true;
        }

        group = group_lookup(argument);
        if (group != NULL) {
            learned = get_learned_group(ch, group);
            if (learned != NULL) {
                send_to_char("You already know that group!\n\r", ch);
                return true;
            }

            level = get_group_level(group, ch->class);
            if (level == NULL || level->difficulty < 1) {
                send_to_char("That group is not available.\n\r", ch);
                return true;
            }

            printf_to_char(ch, "%s group added\n\r", group->name);

            learned = new_learned();
            learned->group = group;
            learned->type = LEARNED_TYPE_GROUP;
            learned->removable = true;
            add_learned(ch, learned);

            ch->pcdata->points += level->difficulty;
            return true;
        }

        skill = skill_lookup(argument);
        if (skill != NULL) {
            learned = get_learned_skill(ch, skill);
            if (learned != NULL) {
                send_to_char("You already know that skill!\n\r", ch);
                return true;
            }

            level = get_skill_level(ch, skill);
            if (level == NULL || level->difficulty < 1 || skill->spells != NULL) {
                send_to_char("That skill is not available.\n\r", ch);
                return true;
            }

            printf_to_char(ch, "%s skill added\n\r", skill->name);
            learned = new_learned();
            learned->skill = skill;
            learned->percent = 1;
            learned->type = LEARNED_TYPE_SKILL;
            learned->removable = true;
            add_learned(ch, learned);

            ch->pcdata->points += level->difficulty;
            return true;
        }

        send_to_char("No skills or groups by that name...\n\r", ch);
        return true;
    }

    if (!strcmp(arg, "drop")) {
        if (argument[0] == '\0') {
            send_to_char("You must provide a skill to drop.\n\r", ch);
            return true;
        }

        group = group_lookup(argument);
        if (group != NULL) {
            learned = get_learned_group(ch, group);
            if (learned != NULL) {
                if (!learned->removable) {
                    send_to_char("That group cannot be dropped.\n\r", ch);
                    return true;
                }

                send_to_char("Group dropped.\n\r", ch);
                level = get_group_level(group, ch->class);
                if (level != NULL)
                    ch->pcdata->points -= level->difficulty;

                remove_learned(ch, learned);
                return true;
            }
        }

        skill = skill_lookup(argument);
        if (skill != NULL) {
            learned = get_learned_skill(ch, skill);
            if (learned != NULL) {
                if (!learned->removable || skill->spells != NULL) {
                    send_to_char("That skill cannot be dropped.\n\r", ch);
                    return true;
                }

                send_to_char("Skill dropped.\n\r", ch);
                level = get_skill_level(ch, skill);
                if (level != NULL)
                    ch->pcdata->points -= level->difficulty;

                remove_learned(ch, learned);
                return true;
            }
        }

        send_to_char("You haven't bought any such skill or group.\n\r", ch);
        return true;
    }

    if (!str_prefix(arg, "premise")) {
        do_help(ch, "premise");
        return true;
    }

    if (!str_prefix(arg, "list")) {
        list_group_costs(ch);
        return true;
    }

    if (!str_prefix(arg, "learned")) {
        list_group_chosen(ch);
        return true;
    }

    if (!str_prefix(arg, "info")) {
        do_groups(ch, argument);
        return true;
    }

    return false;
}

/**
 *    shows all groups or the skills/spells in a group
 */
void do_groups(CHAR_DATA *ch, char *argument)
{
    GROUP *group;
    SKILL_LIST *skill_idx;
    LEARNED *learned;
    LEVEL_INFO *level;
    int col;

    if (IS_NPC(ch))
        return;

    col = 0;
    if (argument[0] == '\0'
        || !str_cmp(argument, "all")) { /* show all groups */
        send_to_char("\n\rGroup                Group                Group\n\r", ch);
        send_to_char("`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r", ch);

        for (group = group_list; group != NULL; group = group->next) {
            learned = get_learned_group(ch, group);

            if (learned != NULL
                || !str_cmp(argument, "all")) {
                printf_to_char(ch, "%-20s ", group->name);
                if (++col % 3 == 0)
                    send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\n\r", ch);

        send_to_char("\n\r`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r", ch);
        printf_to_char(ch, "Creation points: `#%d``\n\r", ch->pcdata->points);
        return;
    }


    /* show the sub-members of a group */
    group = group_lookup(argument);
    if (group == NULL) {
        send_to_char("No group of that name exist.\n\r", ch);
        send_to_char("Type '`!groups all``' for a full listing.\n\r", ch);
        return;
    }

    /* dump information about the class costs */
    printf_to_char(ch, "Group: %s\n\r", group->name);

    send_to_char("\n\rClass         Cost\n\r", ch);
    send_to_char("`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r", ch);
    for (level = group->levels; level != NULL; level = level->next)
        printf_to_char(ch, "%-13.13s [`#%2d``]\n\r", class_table[level->class].name, level->difficulty);


    send_to_char("\n\rSkills/Spells\n\r", ch);
    send_to_char("`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r", ch);
    for (skill_idx = group->skills; skill_idx != NULL; skill_idx = skill_idx->next) {
        if (skill_idx->skill != NULL) {
            printf_to_char(ch, "%-20s ", skill_idx->skill->name);
            if (++col % 3 == 0)
                send_to_char("\n\r", ch);
        }
    }

    if (col % 3 != 0)
        send_to_char("\n\r", ch);

    /* we need to put help info in here */
    if (group->help != NULL) {
        send_to_char("\n\r`8=`7=`&=`7==================================================================`&=`7=`8=``\n\r\n\r", ch);
        page_to_char(group->help->text, ch);
    }
}

/**
 *    check to see if a character has improved at a skill
 */
void check_improve(CHAR_DATA *ch, SKILL *skill, bool success, int multiplier)
{
    LEARNED *learned;
    LEVEL_INFO *level;
    int chance;
    int rating;


    if (IS_NPC(ch))
        return;

    if ((learned = get_learned_skill(ch, skill)) == NULL)
        return;

    if ((level = get_skill_level(ch, skill)) == NULL)
        return;

    rating = UMAX(level->difficulty, 1);

    if (ch->level < level->level
        || learned->percent == 0
        || learned->percent >= 90)
        return;         /* skill is not known */

/* check to see if the character has a chance to learn */
/* Decreased chances on 8/17 by Monrick */

    /* chance = 10 * URANGE(3, get_curr_stat(ch, STAT_INT), 85);*/
    chance = URANGE(3, get_curr_stat(ch, STAT_INT), 85);
    chance /= (UMAX(multiplier, 1) * rating * 4);
    /* chance += ch->level; */
    chance += ch->level / 5;

    if (number_range(1, 1000) > chance)
        return;

/* now that the character has a CHANCE to learn, see if they really have */
    if (success) {
        chance = URANGE(5, 100 - learned->percent, 95);
        if (number_percent() < chance) {
            printf_to_char(ch, "You have become better at %s!\n\r", skill->name);
            learned->percent++;
            learned->percent = UMIN(learned->percent, 100);

            gain_exp(ch, 2 * rating);
        }
    } else {
        chance = URANGE(5, learned->percent / 2, 30);
        if (number_percent() < chance) {
            printf_to_char(ch, "You learn from your mistakes, and your %s skill improves.\n\r", skill->name);

            learned->percent += number_range(1, 3);
            learned->percent = UMIN(learned->percent, 100);

            gain_exp(ch, 2 * rating);
        }
    }
}

/**
 *    practice a skill or view learned percents
 */
void do_practice(CHAR_DATA *ch, char *argument)
{
    char arg[MSL];

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);
    if (arg[0] == '\0' || !str_prefix(arg, "list")) {
        BUFFER *buf;
        LEARNED *learned;
        LEVEL_INFO *level;
        int col;
        bool check_percent;
        bool found;
        int min_pcnt;
        int max_pcnt;


        min_pcnt = 0;
        max_pcnt = 100;
        argument = one_argument(argument, arg);
        check_percent = parse_levels(argument, &min_pcnt, &max_pcnt);

        found = false;
        buf = new_buf();
        col = 0;
        add_buf(buf, "\n\r");
        for (learned = ch->pcdata->skills; learned != NULL; learned = learned->next) {
            if (learned->percent < 1
                || learned->skill == NULL
                || learned->type != LEARNED_TYPE_SKILL)
                continue;

            level = get_skill_level(ch, learned->skill);
            if (level == NULL
                || level->level < 1
                || level->level > ch->level)
                continue;

            /*
             * if we have "skill list 40 80" it will return all
             * skills that have a percentage between 40 and 80
             * continue through the list if it doesnt meet this
             * criteria
             */
            if (check_percent
                && (learned->percent < min_pcnt || learned->percent > max_pcnt))
                continue;


            /*
             * if we have "skill list acid" we look up all
             * skills that start with "acid"
             */
            if (!check_percent && argument[0] != '\0') {
                if (argument[0] != learned->skill->name[0])
                    continue;

                if (str_prefix(argument, learned->skill->name))
                    continue;
            }

            found = true;
            printf_buf(buf, "%-18s `O%3d%%``  ",
                   learned->skill->name,
                   learned->percent);
            if (++col % 3 == 0)
                add_buf(buf, "\n\r");
        }

        if (col % 3 != 0)
            add_buf(buf, "\n\r");

        if (!found)
            send_to_char("\n\rThere were no skills that matched your search criteria.\n\r", ch);

        printf_buf(buf, "\n\rYou have ```2%d ``practice sessions left.\n\r", ch->pcdata->practice);
        page_to_char(buf_string(buf), ch);
        free_buf(buf);
    } else {
        CHAR_DATA *mob;
        LEARNED *learned;
        LEVEL_INFO *level;
        SKILL *skill;

        if (!IS_AWAKE(ch)) {
            send_to_char("You must be awake to practice...\n\r", ch);
            return;
        }

        for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room) {
            if (IS_NPC(mob)
                && IS_SET(mob->act, ACT_PRACTICE))
                break;
        }

        if (mob == NULL) {
            send_to_char("You can't do that here.\n\r", ch);
            return;
        }

        if (ch->pcdata->practice <= 0) {
            send_to_char("You have no practice sessions left.  NONE!\n\r", ch);
            return;
        }

        if ((skill = skill_lookup(argument)) != NULL) {
            if ((learned = get_learned_skill(ch, skill)) != NULL
                && (level = get_skill_level(ch, skill)) != NULL) {
                if (level->level <= ch->level) {
                    if (learned->percent >= class_table[ch->class].skill_adept) {
                        printf_to_char(ch, "You are already learned at %s.\n\r", skill->name);
                        return;
                    }

                    ch->pcdata->practice--;
                    learned->percent += UMAX(3, get_curr_stat(ch, STAT_INT) * 5) / UMAX(level->difficulty, 1);

                    if (learned->percent < class_table[ch->class].skill_adept) {
                        act("You practice $T.", ch, NULL, skill->name, TO_CHAR);
                        act("$n practices $T.", ch, NULL, skill->name, TO_ROOM);
                    } else {
                        learned->percent = class_table[ch->class].skill_adept;
                        act("You are now learned at $T.", ch, NULL, skill->name, TO_CHAR);
                        act("$n is now learned at $T.", ch, NULL, skill->name, TO_ROOM);
                    }

                    return;
                }
            }
        }
        send_to_char("You can't practice that.\n\r", ch);
    }
}

static const struct train_type {
    char *cmd;
    char *desc;
    int cost;
    int stat;
} train_cmds[] = {
    { "str", "strength", 1, STAT_STR },
    { "int", "intelligence", 1, STAT_INT },
    { "wis", "wisdom", 1, STAT_WIS },
    { "dex", "dexterity", 1, STAT_DEX },
    { "con", "constitution", 1, STAT_CON },
    { "luck", "luck", 1, STAT_LUCK },
    { "", "", 0, -1 }
};

/**
 * use trains
 */
void do_train(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    char arg[MIL], buf[MSL];
    int idx;
    int cost;
    int count;

    if (IS_NPC(ch))
        return;

    for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
        if (IS_TRAINER(mob))
            break;

    if (is_help(argument) || mob == NULL) {
        if (mob == NULL)
            send_to_char("\n\rYou can't do that here.\n\r", ch);

        printf_to_char(ch, "\n\rYou have %d training sessions.\n\r", ch->pcdata->train);
        send_to_char("You can train:", ch);
        for (idx = 0; train_cmds[idx].cmd[0] != '\0'; idx++)
            if (ch->perm_stat[train_cmds[idx].stat] < get_max_train(ch, train_cmds[idx].stat))
                printf_to_char(ch, " %s", train_cmds[idx].cmd);

        send_to_char(" hp mana.\n\r", ch);
        return;
    }


    /* in case i ever want to change it */
    count = mult_argument(argument, arg);
    cost = 1 * count;
/* added by Cyrkle */
    if (count == 0) {
        send_to_char("You have to spend at least 1 training session.\n\r", ch);
        return;
    }
    if (count < 0) {
        send_to_char("Now now, no cheating.\n\r", ch);
        sprintf(buf, "$N tried to use the negative train bug.");
        wiznet(buf, ch, NULL, WIZ_SECURE, 0, 0);
        return;
    }
/* ---------------- */

    if (!str_cmp("hp", arg)) {
        if (ch->pcdata->train < cost) {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }

        ch->pcdata->train -= cost;
        ch->pcdata->perm_hit += 10 * count;
        ch->max_hit += 10 * count;
        ch->hit += 10 * count;

        act("Your durability increases!", ch, NULL, NULL, TO_CHAR);
        act("$n's durability increases!", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (!str_cmp("mana", arg)) {
        if (ch->pcdata->train < cost) {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }

        ch->pcdata->train -= cost;
        ch->pcdata->perm_mana += 10 * count;
        ch->max_mana += 10 * count;
        ch->mana += 10 * count;

        act("Your power increases!", ch, NULL, NULL, TO_CHAR);
        act("$n's power increases!", ch, NULL, NULL, TO_ROOM);
        return;
    }



    for (idx = 0; train_cmds[idx].cmd[0] != '\0'; idx++) {
        if (!str_cmp(arg, train_cmds[idx].cmd)) {
            int max;

            max = get_max_train(ch, train_cmds[idx].stat);
            if (ch->perm_stat[train_cmds[idx].stat] >= max) {
                act("Your $T is already at maximum.", ch, NULL, train_cmds[idx].desc, TO_CHAR);
                return;
            }

            if ((ch->perm_stat[train_cmds[idx].stat] + count) > max)
                count = max - ch->perm_stat[train_cmds[idx].stat];

            if ((train_cmds[idx].cost * count) > ch->pcdata->train) {
                send_to_char("You don't have enough training sessions.\n\r", ch);
                return;
            }


            ch->pcdata->train -= (train_cmds[idx].cost * count);
            ch->perm_stat[train_cmds[idx].stat] += (1 * count);

            act("Your $T increases!", ch, NULL, train_cmds[idx].desc, TO_CHAR);
            act("$n's $T increases!", ch, NULL, train_cmds[idx].desc, TO_ROOM);
            return;
        }
    }

    /* didnt find the right thing to train */
    do_train(ch, "");
}

