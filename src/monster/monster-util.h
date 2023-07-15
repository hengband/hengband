#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class PlayerType;
typedef bool (*monsterrace_hook_type)(PlayerType *, MonsterRaceId);

extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;
extern int chameleon_change_m_idx;
enum summon_type : int;
extern summon_type summon_specific_type;

monsterrace_hook_type get_monster_hook(PlayerType *player_ptr);
monsterrace_hook_type get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x);
errr get_mon_num_prep(PlayerType *player_ptr, monsterrace_hook_type hook1, monsterrace_hook_type hook2);
errr get_mon_num_prep_bounty(PlayerType *player_ptr);
