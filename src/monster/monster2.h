#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

void set_target(monster_type *m_ptr, POSITION y, POSITION x);
void reset_target(monster_type *m_ptr);
monster_race *real_r_ptr(monster_type *m_ptr);
MONRACE_IDX real_r_idx(monster_type *m_ptr);
void delete_monster_idx(player_type *player_ptr, MONSTER_IDX i);
void compact_monsters(player_type *player_ptr, int size);
void wipe_monsters_list(player_type *player_ptr);
MONSTER_IDX m_pop(player_type *player_ptr);

#define GMN_ARENA 0x00000001 //!< 賭け闘技場向け生成
MONRACE_IDX get_mon_num(player_type *player_ptr, DEPTH level, BIT_FLAGS option);
bool multiply_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
bool summon_specific(player_type *player_ptr, MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, int type, BIT_FLAGS mode);
bool summon_named_creature(player_type *player_ptr, MONSTER_IDX who, POSITION oy, POSITION ox, MONRACE_IDX r_idx, BIT_FLAGS mode);
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what);
void choose_new_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx);
SPEED get_mspeed(player_type *player_ptr, monster_race *r_ptr);
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr);
bool is_friendly(monster_type *m_ptr);
bool is_pet(monster_type *m_ptr);
bool is_hostile(monster_type *m_ptr);

/*  Determine monster race appearance index is same as race index */
bool is_original_ap(monster_type *m_ptr);

int get_monster_crowd_number(player_type *player_ptr, MONSTER_IDX m_idx);
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam);
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x);
bool alloc_guardian(player_type *player_ptr, bool def_val);
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode);
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode);
void monster_name(player_type *player_ptr, MONSTER_IDX m_idx, char *m_name);
