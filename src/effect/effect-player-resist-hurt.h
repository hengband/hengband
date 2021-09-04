#pragma once

#include "system/angband.h"

struct effect_player_type;
struct player_type;
void effect_player_elements(
    player_type *target_ptr, effect_player_type *ep_ptr, concptr attack_message, HIT_POINT (*damage_func)(player_type *, HIT_POINT, concptr, bool));
void effect_player_poison(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_nuke(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_missile(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_holy_fire(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_hell_fire(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_arrow(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_plasma(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_nether(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_water(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_chaos(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_shards(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_sound(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_confusion(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_disenchant(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_nexus(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_force(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_rocket(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_inertial(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_lite(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_dark(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_time(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_gravity(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_disintegration(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_death_ray(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_mana(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_psy_spear(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_meteor(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_icee(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_hand_doom(player_type *target_ptr, effect_player_type *ep_ptr);
