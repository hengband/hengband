#pragma once

#include "system/angband.h"

struct monster_type;
struct player_type;
void set_friendly(monster_type *m_ptr);
void set_pet(player_type *player_ptr, monster_type *m_ptr);
void set_hostile(player_type *player_ptr, monster_type *m_ptr);
void anger_monster(player_type *player_ptr, monster_type *m_ptr);
bool set_monster_csleep(player_type *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_fast(player_type *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_slow(player_type *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_stunned(player_type *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_confused(player_type *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_monfear(player_type *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_invulner(player_type *player_ptr, MONSTER_IDX m_idx, int v, bool energy_need);
bool set_monster_timewalk(player_type *player_ptr, int num, MONRACE_IDX who, bool vs_player);
