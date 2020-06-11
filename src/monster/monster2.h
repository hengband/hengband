#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

void set_target(monster_type *m_ptr, POSITION y, POSITION x);
void reset_target(monster_type *m_ptr);
MONSTER_IDX m_pop(player_type *player_ptr);

#define GMN_ARENA 0x00000001 //!< 賭け闘技場向け生成
MONRACE_IDX get_mon_num(player_type *player_ptr, DEPTH level, BIT_FLAGS option);
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what);
void choose_new_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx);
SPEED get_mspeed(player_type *player_ptr, monster_race *r_ptr);
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr);

int get_monster_crowd_number(player_type *player_ptr, MONSTER_IDX m_idx);
void monster_name(player_type *player_ptr, MONSTER_IDX m_idx, char *m_name);
