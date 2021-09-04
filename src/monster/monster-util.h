#pragma once

#include "system/angband.h"

struct player_type;
typedef bool (*monsterrace_hook_type)(player_type *, MONRACE_IDX);

extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;
extern int chameleon_change_m_idx;
enum summon_type : int;
extern summon_type summon_specific_type;

monsterrace_hook_type get_monster_hook(player_type *player_ptr);
monsterrace_hook_type get_monster_hook2(player_type *player_ptr, POSITION y, POSITION x);
errr get_mon_num_prep(player_type *player_ptr, monsterrace_hook_type hook1, monsterrace_hook_type hook2);
errr get_mon_num_prep_bounty(player_type *player_ptr);
