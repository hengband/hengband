#pragma once

#include "system/angband.h"
#include <string_view>

class MonsterEntity;
class ItemEntity;
class PlayerType;
bool test_hit_fire(PlayerType *player_ptr, int chance, MonsterEntity *m_ptr, int vis, std::string_view item_name);
void exe_fire(PlayerType *player_ptr, INVENTORY_IDX i_idx, ItemEntity *j_ptr, SPELL_IDX snipe_type);
int critical_shot(PlayerType *player_ptr, WEIGHT weight, int plus_ammo, int plus_bow, int dam, bool supercritical);
int calc_crit_ratio_shot(PlayerType *player_ptr, int plus_ammo, int plus_bow);
int calc_expect_crit_shot(PlayerType *player_ptr, WEIGHT weight, int plus_ammo, int plus_bow, int dam);
int calc_expect_crit(PlayerType *player_ptr, WEIGHT weight, int plus, int dam, int16_t meichuu, bool dokubari, bool supercritical, bool impact, int mult = 1);
uint32_t calc_expect_dice(
    PlayerType *player_ptr, uint32_t dam, int16_t to_h, ItemEntity *o_ptr);
uint32_t calc_expect_dice(
    PlayerType *player_ptr, uint32_t dam, int mult, int div, bool force, WEIGHT weight, int plus, int16_t meichuu, bool dokubari, bool supercritical, bool impact, int vorpal_mult, int vorpal_div);
