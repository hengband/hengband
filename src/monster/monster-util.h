#pragma once

#include "system/angband.h"
#include "system/enums/monrace/monrace-hook-types.h"
#include <functional>
#include <optional>

enum summon_type : int;
enum class MonraceId : short;
class PlayerType;
using monsterrace_hook_type = std::function<bool(PlayerType *, MonraceId)>;
monsterrace_hook_type get_monster_hook(PlayerType *player_ptr);
MonraceHookTerrain get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x);
void get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type &hook1, MonraceHookTerrain hook2 = MonraceHookTerrain::NONE, std::optional<summon_type> summon_specific_type = std::nullopt);
void get_mon_num_prep_chameleon(PlayerType *player_ptr, short m_idx, short terrain_id, const std::optional<short> summoner_m_idx, bool is_unique);
void get_mon_num_prep_bounty(PlayerType *player_ptr);
bool is_player(MONSTER_IDX m_idx);
bool is_monster(MONSTER_IDX m_idx);
