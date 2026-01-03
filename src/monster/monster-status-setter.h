#pragma once

#include "system/angband.h"

enum class MonraceId : short;
class FloorType;
class MonsterEntity;
class PlayerType;
void set_pet(PlayerType *player_ptr, MonsterEntity &monster);
void anger_monster(PlayerType *player_ptr, MonsterEntity &monster);
bool set_monster_csleep(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_fast(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_slow(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_stunned(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_confused(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_monfear(FloorType &floor, MONSTER_IDX m_idx, int v);
bool set_monster_invulner(FloorType &floor, MONSTER_IDX m_idx, int v, bool energy_need);
bool set_monster_timewalk(PlayerType *player_ptr, MONSTER_IDX m_idx, int num, bool vs_player);
