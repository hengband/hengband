#pragma once

#include "system/angband.h"

enum class MonraceId : short;
class FloorType;
class PlayerType;
class MonsterEntity;
bool monster_is_powerful(const FloorType &floor, MONSTER_IDX m_idx);
DEPTH monster_level_idx(const FloorType &floor, MONSTER_IDX m_idx);

int mon_damage_mod(PlayerType *player_ptr, const MonsterEntity &monster, int dam, bool is_psy_spear);

void dispel_monster_status(PlayerType *player_ptr, MONSTER_IDX m_idx);
void monster_gain_exp(PlayerType *player_ptr, MONSTER_IDX m_idx, MonraceId s_idx);

enum class MonsterTimedEffect : int;
void process_monsters_mtimed(PlayerType *player_ptr, MonsterTimedEffect mte);
