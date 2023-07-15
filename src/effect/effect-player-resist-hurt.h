#pragma once

#include <string_view>

class EffectPlayerType;
class PlayerType;
void effect_player_elements(
    PlayerType *player_ptr, EffectPlayerType *ep_ptr, std::string_view attack_message, int (*damage_func)(PlayerType *, int, std::string_view, bool));
void effect_player_poison(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_nuke(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_missile(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_holy_fire(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_hell_fire(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_arrow(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_plasma(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_nether(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_water(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_chaos(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_shards(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_sound(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_confusion(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_disenchant(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_nexus(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_force(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_rocket(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_inertial(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_lite(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_dark(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_time(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_gravity(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_disintegration(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_death_ray(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_mana(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_psy_spear(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_meteor(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_icee(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_hand_doom(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_void(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_abyss(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
