#pragma once

#include "system/angband.h"
#include <functional>
#include <optional>

enum class MonraceId : short;
class PlayerType;
using monsterrace_hook_type = std::function<bool(PlayerType *, MonraceId)>;

enum summon_type : int;

monsterrace_hook_type get_monster_hook(PlayerType *player_ptr);
monsterrace_hook_type get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x);
errr get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type &hook1, const monsterrace_hook_type &hook2, std::optional<summon_type> summon_specific_type = std::nullopt);
errr get_mon_num_prep_chameleon(PlayerType *player_ptr, const monsterrace_hook_type &hook1);
errr get_mon_num_prep_bounty(PlayerType *player_ptr);
bool is_player(MONSTER_IDX m_idx);
bool is_monster(MONSTER_IDX m_idx);
