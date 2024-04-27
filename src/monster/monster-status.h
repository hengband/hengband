#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class FloorType;
class PlayerType;
class MonsterEntity;
bool monster_is_powerful(FloorType *floor_ptr, MONSTER_IDX m_idx);
DEPTH monster_level_idx(FloorType *floor_ptr, MONSTER_IDX m_idx);

int mon_damage_mod(PlayerType *player_ptr, MonsterEntity *m_ptr, int dam, bool is_psy_spear);

void dispel_monster_status(PlayerType *player_ptr, MONSTER_IDX m_idx);
void monster_gain_exp(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterRaceId s_idx);

void process_monsters_mtimed(PlayerType *player_ptr, int mtimed_idx);

int get_mproc_idx(FloorType *floor_ptr, MONSTER_IDX m_idx, int mproc_type);
void mproc_init(FloorType *floor_ptr);
void mproc_add(FloorType *floor_ptr, MONSTER_IDX m_idx, int mproc_type);

std::pair<int, int> get_damage_cap(int hp, int maxhp);
std::tuple<int, int, int> get_damage_cap_contains_max(int hp, int maxhp);
