#pragma once

#include "system/angband.h"

class MonsterEntity;
class ItemEntity;
class PlayerType;
bool test_hit_fire(PlayerType *player_ptr, int chance, MonsterEntity *m_ptr, int vis, char *o_name);
int critical_shot(PlayerType *player_ptr, WEIGHT weight, int plus_ammo, int plus_bow, int dam);
int calc_crit_ratio_shot(PlayerType *player_ptr, int plus_ammo, int plus_bow);
int calc_expect_crit_shot(PlayerType *player_ptr, WEIGHT weight, int plus_ammo, int plus_bow, int dam);
int calc_expect_crit(PlayerType *player_ptr, WEIGHT weight, int plus, int dam, int16_t meichuu, bool dokubari, bool impact);
void exe_fire(PlayerType *player_ptr, INVENTORY_IDX item, ItemEntity *j_ptr, SPELL_IDX snipe_type);
