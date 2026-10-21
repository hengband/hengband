#pragma once

#include <string_view>

constexpr auto DAMAGE_FORCE = 1;
constexpr auto DAMAGE_GENO = 2;
constexpr auto DAMAGE_LOSELIFE = 3;
constexpr auto DAMAGE_ATTACK = 4;
constexpr auto DAMAGE_NOESCAPE = 5;
constexpr auto DAMAGE_USELIFE = 6;

class MonsterEntity;
class PlayerType;
int take_hit(PlayerType *player_ptr, int damage_type, int damage, std::string_view kb_str);
int acid_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
int elec_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
int fire_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
int cold_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
void touch_zap_player(const MonsterEntity &monster, PlayerType *player_ptr);
