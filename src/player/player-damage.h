#pragma once

#include "system/angband.h"

#define DAMAGE_FORCE    1
#define DAMAGE_GENO     2
#define DAMAGE_LOSELIFE 3
#define DAMAGE_ATTACK   4
#define DAMAGE_NOESCAPE 5
#define DAMAGE_USELIFE  6

struct monster_type;
struct player_type;
int take_hit(player_type *player_ptr, int damage_type, HIT_POINT damage, concptr kb_str);
HIT_POINT acid_dam(player_type *player_ptr, HIT_POINT dam, concptr kb_str, bool aura);
HIT_POINT elec_dam(player_type *player_ptr, HIT_POINT dam, concptr kb_str, bool aura);
HIT_POINT fire_dam(player_type *player_ptr, HIT_POINT dam, concptr kb_str, bool aura);
HIT_POINT cold_dam(player_type *player_ptr, HIT_POINT dam, concptr kb_str, bool aura);
void touch_zap_player(monster_type *m_ptr, player_type *player_ptr);
