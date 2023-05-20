#pragma once

#include <string_view>

#define DAMAGE_FORCE 1
#define DAMAGE_GENO 2
#define DAMAGE_LOSELIFE 3
#define DAMAGE_ATTACK 4
#define DAMAGE_NOESCAPE 5
#define DAMAGE_USELIFE 6

class MonsterEntity;
class PlayerType;
int take_hit(PlayerType *player_ptr, int damage_type, int damage, std::string_view kb_str);
int acid_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
int elec_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
int fire_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
int cold_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);
void touch_zap_player(MonsterEntity *m_ptr, PlayerType *player_ptr);
