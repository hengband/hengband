#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class FloorType;
class MonsterRaceInfo;
class PlayerType;
MONSTER_IDX m_pop(FloorType *floor_ptr);

MonsterRaceId get_mon_num(PlayerType *player_ptr, DEPTH min_level, DEPTH max_level, BIT_FLAGS mode);
void choose_new_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool born, MonsterRaceId r_idx);
byte get_mspeed(FloorType *player_ptr, MonsterRaceInfo *r_ptr);
int get_monster_crowd_number(FloorType *floor_ptr, MONSTER_IDX m_idx);
