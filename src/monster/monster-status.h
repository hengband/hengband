#pragma once

#include "system/angband.h"

struct floor_type;
struct player_type;
struct monster_type;
bool monster_is_powerful(floor_type *floor_ptr, MONSTER_IDX m_idx);
DEPTH monster_level_idx(floor_type *floor_ptr, MONSTER_IDX m_idx);

HIT_POINT mon_damage_mod(player_type *target_ptr, monster_type *m_ptr, HIT_POINT dam, bool is_psy_spear);
bool monster_is_valid(monster_type *m_ptr);

void dispel_monster_status(player_type *target_ptr, MONSTER_IDX m_idx);
void monster_gain_exp(player_type *target_ptr, MONSTER_IDX m_idx, MONRACE_IDX s_idx);

void process_monsters_mtimed(player_type *target_ptr, int mtimed_idx);

TIME_EFFECT monster_csleep_remaining(monster_type *m_ptr);
TIME_EFFECT monster_fast_remaining(monster_type *m_ptr);
TIME_EFFECT monster_slow_remaining(monster_type *m_ptr);
TIME_EFFECT monster_stunned_remaining(monster_type *m_ptr);
TIME_EFFECT monster_confused_remaining(monster_type *m_ptr);
TIME_EFFECT monster_fear_remaining(monster_type *m_ptr);
TIME_EFFECT monster_invulner_remaining(monster_type *m_ptr);

int get_mproc_idx(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type);
void mproc_init(floor_type *floor_ptr);
void mproc_add(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type);
