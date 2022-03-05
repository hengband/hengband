#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
struct floor_type;
class PlayerType;
struct monster_type;
bool monster_is_powerful(floor_type *floor_ptr, MONSTER_IDX m_idx);
DEPTH monster_level_idx(floor_type *floor_ptr, MONSTER_IDX m_idx);

int mon_damage_mod(PlayerType *player_ptr, monster_type *m_ptr, int dam, bool is_psy_spear);
bool monster_is_valid(monster_type *m_ptr);

void dispel_monster_status(PlayerType *player_ptr, MONSTER_IDX m_idx);
void monster_gain_exp(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterRaceId s_idx);

void process_monsters_mtimed(PlayerType *player_ptr, int mtimed_idx);

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
