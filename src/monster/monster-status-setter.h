#pragma once

#include "system/angband.h"

struct monster_type;
class PlayerType;
void set_friendly(monster_type *m_ptr);
void set_pet(PlayerType *player_ptr, monster_type *m_ptr);
void set_hostile(PlayerType *player_ptr, monster_type *m_ptr);
void anger_monster(PlayerType *player_ptr, monster_type *m_ptr);
bool set_monster_csleep(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_fast(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_slow(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_stunned(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_confused(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_monfear(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_invulner(PlayerType *player_ptr, MONSTER_IDX m_idx, int v, bool energy_need);
bool set_monster_timewalk(PlayerType *player_ptr, int num, MONRACE_IDX who, bool vs_player);
