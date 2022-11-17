#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class MonsterEntity;
class PlayerType;
void set_friendly(MonsterEntity *m_ptr);
void set_pet(PlayerType *player_ptr, MonsterEntity *m_ptr);
void set_hostile(PlayerType *player_ptr, MonsterEntity *m_ptr);
void anger_monster(PlayerType *player_ptr, MonsterEntity *m_ptr);
bool set_monster_csleep(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_fast(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_slow(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_stunned(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_confused(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_monfear(PlayerType *player_ptr, MONSTER_IDX m_idx, int v);
bool set_monster_invulner(PlayerType *player_ptr, MONSTER_IDX m_idx, int v, bool energy_need);
bool set_monster_timewalk(PlayerType *player_ptr, int num, MonsterRaceId who, bool vs_player);
