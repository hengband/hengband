#pragma once

#include "system/angband.h"
#include <optional>

enum summon_type : int;
enum class MonsterRaceId : int16_t;
class PlayerType;
using summon_specific_pf = std::optional<MONSTER_IDX>(PlayerType *, POSITION, POSITION, DEPTH, summon_type, BIT_FLAGS, std::optional<MONSTER_IDX>);

bool mon_scatter(PlayerType *player_ptr, MonsterRaceId r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist);
std::optional<MONSTER_IDX> multiply_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
std::optional<MONSTER_IDX> place_specific_monster(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION y, POSITION x, MonsterRaceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx = std::nullopt);
std::optional<MONSTER_IDX> place_random_monster(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(PlayerType *player_ptr, POSITION y, POSITION x, summon_specific_pf summon_specific);
bool alloc_guardian(PlayerType *player_ptr, bool def_val);
bool alloc_monster(PlayerType *player_ptr, int min_dis, BIT_FLAGS mode, summon_specific_pf summon_specific, int max_dis = 65535);
