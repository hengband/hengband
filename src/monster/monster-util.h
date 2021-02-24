#pragma once

#include "system/angband.h"

typedef bool (*monsterrace_hook_type)(const player_type *, MONRACE_IDX);

extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;
extern int chameleon_change_m_idx;
typedef enum summon_type summon_type;
extern summon_type summon_specific_type;

monsterrace_hook_type get_monster_hook(const player_type *player_ptr);
monsterrace_hook_type get_monster_hook2(const player_type *player_ptr, POSITION y, POSITION x);
errr get_mon_num_prep(const player_type *player_ptr, monsterrace_hook_type hook1, monsterrace_hook_type hook2);
errr get_mon_num_prep_bounty(const player_type *player_ptr);
