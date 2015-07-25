/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/
/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor (rtaylor@hypercube.org)                                *
*       Gabrielle Taylor (gtaylor@hypercube.org)                           *
*       Brian Moore (zump@rom.org)                                         *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/


#if !defined(__MAGIC_H)
#define __MAGIC_H


bool check_dispel(int dis_level, CHAR_DATA * victim, SKILL * skill);
bool saves_dispel(int dis_level, int spell_level, int duration);
void cast_spell(CHAR_DATA * ch, SKILL * skill, int level, void *vo, int target, char *argument);

/***************************************************************************
*	spell functions
***************************************************************************/
DECLARE_SPELL_FUN(spell_null);
DECLARE_SPELL_FUN(spell_acid_blast);
DECLARE_SPELL_FUN(spell_acidic_rain);
DECLARE_SPELL_FUN(spell_armor);
DECLARE_SPELL_FUN(spell_bless);
DECLARE_SPELL_FUN(spell_blindness);
DECLARE_SPELL_FUN(spell_blink);
DECLARE_SPELL_FUN(spell_burning_hands);
DECLARE_SPELL_FUN(spell_burning_flames);        /* Fizzy added 3/29/97  */
DECLARE_SPELL_FUN(spell_call_lightning);
DECLARE_SPELL_FUN(spell_calm);
DECLARE_SPELL_FUN(spell_cancellation);
DECLARE_SPELL_FUN(spell_cause_critical);
DECLARE_SPELL_FUN(spell_cause_light);
DECLARE_SPELL_FUN(spell_cause_serious);
DECLARE_SPELL_FUN(spell_change_sex);
DECLARE_SPELL_FUN(spell_chain_lightning);
DECLARE_SPELL_FUN(spell_charm_person);
DECLARE_SPELL_FUN(spell_chill_touch);
DECLARE_SPELL_FUN(spell_cloud_of_death);
DECLARE_SPELL_FUN(spell_colour_spray);
DECLARE_SPELL_FUN(spell_continual_light);
DECLARE_SPELL_FUN(spell_control_weather);
DECLARE_SPELL_FUN(spell_create_food);
DECLARE_SPELL_FUN(spell_create_rose);
DECLARE_SPELL_FUN(spell_create_spring);
DECLARE_SPELL_FUN(spell_create_water);
DECLARE_SPELL_FUN(spell_cure_blindness);
DECLARE_SPELL_FUN(spell_cure_critical);
DECLARE_SPELL_FUN(spell_cure_disease);
DECLARE_SPELL_FUN(spell_cure_light);
DECLARE_SPELL_FUN(spell_cure_poison);
DECLARE_SPELL_FUN(spell_cure_serious);
DECLARE_SPELL_FUN(spell_curse);
DECLARE_SPELL_FUN(spell_darkness);
DECLARE_SPELL_FUN(spell_demonfire);
DECLARE_SPELL_FUN(spell_detect_evil);
DECLARE_SPELL_FUN(spell_detect_good);
DECLARE_SPELL_FUN(spell_detect_invis);
DECLARE_SPELL_FUN(spell_detect_magic);
DECLARE_SPELL_FUN(spell_detect_poison);
DECLARE_SPELL_FUN(spell_dispel_evil);
DECLARE_SPELL_FUN(spell_dispel_good);
DECLARE_SPELL_FUN(spell_dispel_magic);
DECLARE_SPELL_FUN(spell_earthquake);
DECLARE_SPELL_FUN(spell_enchant_armor);
DECLARE_SPELL_FUN(spell_enchant_weapon);
DECLARE_SPELL_FUN(spell_energy_drain);
DECLARE_SPELL_FUN(spell_extinguish_flames);     /* Fizzy added 3/29/97 */
DECLARE_SPELL_FUN(spell_faerie_fire);
DECLARE_SPELL_FUN(spell_faerie_fog);
DECLARE_SPELL_FUN(spell_farsight);
DECLARE_SPELL_FUN(spell_fireball);
DECLARE_SPELL_FUN(spell_fireblade);     /* Added 9/24/96 Bors */
DECLARE_SPELL_FUN(spell_fireproof);
DECLARE_SPELL_FUN(spell_flamestrike);
DECLARE_SPELL_FUN(spell_floating_disc);
DECLARE_SPELL_FUN(spell_fly);
DECLARE_SPELL_FUN(spell_freeze_bolt);
DECLARE_SPELL_FUN(spell_freeze_storm);
DECLARE_SPELL_FUN(spell_frenzy);
DECLARE_SPELL_FUN(spell_frost_hands);
DECLARE_SPELL_FUN(spell_gate);
DECLARE_SPELL_FUN(spell_giant_strength);
DECLARE_SPELL_FUN(spell_grandeur);
DECLARE_SPELL_FUN(spell_harm);
DECLARE_SPELL_FUN(spell_haste);
DECLARE_SPELL_FUN(spell_heal);
DECLARE_SPELL_FUN(spell_heal_mana);
DECLARE_SPELL_FUN(spell_holy_word);
DECLARE_SPELL_FUN(spell_identify);
DECLARE_SPELL_FUN(spell_infravision);
DECLARE_SPELL_FUN(spell_invis);
DECLARE_SPELL_FUN(spell_know_alignment);
DECLARE_SPELL_FUN(spell_lightning_bolt);
DECLARE_SPELL_FUN(spell_locate_object);
DECLARE_SPELL_FUN(spell_magic_missile);
DECLARE_SPELL_FUN(spell_mass_healing);
DECLARE_SPELL_FUN(spell_mass_invis);
DECLARE_SPELL_FUN(spell_meteor_storm);
DECLARE_SPELL_FUN(spell_monsoon);       /* Added 6/12/96 Bors */
DECLARE_SPELL_FUN(spell_nexus);
DECLARE_SPELL_FUN(spell_pass_door);
DECLARE_SPELL_FUN(spell_plague);
DECLARE_SPELL_FUN(spell_pollenburst);   /* added 6/11/96 Bors */
DECLARE_SPELL_FUN(spell_poison);
DECLARE_SPELL_FUN(spell_portal);
DECLARE_SPELL_FUN(spell_protection_evil);
DECLARE_SPELL_FUN(spell_protection_good);
DECLARE_SPELL_FUN(spell_psychic_headbutt);
DECLARE_SPELL_FUN(spell_ray_of_truth);
DECLARE_SPELL_FUN(spell_recharge);
DECLARE_SPELL_FUN(spell_refresh);
DECLARE_SPELL_FUN(spell_reverse_align);
DECLARE_SPELL_FUN(spell_remove_curse);
DECLARE_SPELL_FUN(spell_revive);
DECLARE_SPELL_FUN(spell_ring_of_fire);          /* added 9/23/96 Bors */
DECLARE_SPELL_FUN(spell_sanctuary);
DECLARE_SPELL_FUN(spell_super_speed);
DECLARE_SPELL_FUN(spell_druid_call);
DECLARE_SPELL_FUN(spell_anti_magic_aura);
DECLARE_SPELL_FUN(spell_shocking_grasp);
DECLARE_SPELL_FUN(spell_shield);
DECLARE_SPELL_FUN(spell_sleep);
DECLARE_SPELL_FUN(spell_slow);
DECLARE_SPELL_FUN(spell_stone_skin);
DECLARE_SPELL_FUN(spell_thorns);        /* added 6/11/96 Bors */
DECLARE_SPELL_FUN(spell_ventriloquate);
DECLARE_SPELL_FUN(spell_weaken);
DECLARE_SPELL_FUN(spell_timewarp);      /* added 8/28/97 Eo   */
DECLARE_SPELL_FUN(spell_winds);
DECLARE_SPELL_FUN(spell_acid_breath);
DECLARE_SPELL_FUN(spell_fire_breath);
DECLARE_SPELL_FUN(spell_frost_breath);
DECLARE_SPELL_FUN(spell_gas_breath);
DECLARE_SPELL_FUN(spell_lightning_breath);
DECLARE_SPELL_FUN(spell_general_purpose);
DECLARE_SPELL_FUN(spell_high_explosive);
DECLARE_SPELL_FUN(spell_fst);
DECLARE_SPELL_FUN(spell_create_feast);
DECLARE_SPELL_FUN(spell_blood_boil);
DECLARE_SPELL_FUN(spell_make_bag);
DECLARE_SPELL_FUN(spell_bad_trip);
DECLARE_SPELL_FUN(spell_black_plague);
DECLARE_SPELL_FUN(spell_black_mantle);   /*  recreated 4/02 Shez  */
DECLARE_SPELL_FUN(spell_equipment_invis);
DECLARE_SPELL_FUN(spell_noremove);
DECLARE_SPELL_FUN(spell_wall_of_water);
DECLARE_SPELL_FUN(spell_wall_of_fire);
DECLARE_SPELL_FUN(spell_wall_of_ice);
DECLARE_SPELL_FUN(spell_wall_of_acid);
DECLARE_SPELL_FUN(spell_cure_blood);
DECLARE_SPELL_FUN(spell_fear);
DECLARE_SPELL_FUN(spell_air_armor);
DECLARE_SPELL_FUN(spell_bark_skin);
DECLARE_SPELL_FUN(spell_shatter_curse);
DECLARE_SPELL_FUN(spell_bloody_tears);
DECLARE_SPELL_FUN(spell_web);


DECLARE_SPELL_FUN(spell_firebomb);
DECLARE_SPELL_FUN(spell_sanatorium);
DECLARE_SPELL_FUN(spell_deathtrap);
DECLARE_SPELL_FUN(spell_displacement);
DECLARE_SPELL_FUN(spell_haven);
DECLARE_SPELL_FUN(spell_parasitic_cloud);
DECLARE_SPELL_FUN(spell_mana_vortex);
DECLARE_SPELL_FUN(spell_psychedelic);




/*******************************************************************************
*	room affects
*******************************************************************************/
DECLARE_AFFECT_FUN(affect_firebomb);
DECLARE_AFFECT_FUN(affect_sanatorium);
DECLARE_AFFECT_FUN(affect_displacement);
DECLARE_AFFECT_FUN(affect_deathtrap);
DECLARE_AFFECT_FUN(affect_parasitic_cloud);
DECLARE_AFFECT_FUN(affect_psychedelic);

/*******************************************************************************
*	character affects
*******************************************************************************/
DECLARE_AFFECT_FUN(affect_black_plague);
DECLARE_AFFECT_FUN(affect_disease);
DECLARE_AFFECT_FUN(affect_burning_flames);
DECLARE_AFFECT_FUN(affect_poison);


#endif   /* __MAGIC_H */
