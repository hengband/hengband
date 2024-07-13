#pragma once

#include "system/angband.h"
#include <optional>

enum class MonsterRaceId : int16_t;
class FloorType;
class MonsterRaceInfo;
class PlayerType;
class Grid;
MONSTER_IDX m_pop(FloorType *floor_ptr);

MonsterRaceId get_mon_num(PlayerType *player_ptr, DEPTH min_level, DEPTH max_level, BIT_FLAGS mode);
void choose_chameleon_polymorph(PlayerType *player_ptr, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx = std::nullopt);
int get_monster_crowd_number(FloorType *floor_ptr, MONSTER_IDX m_idx);
