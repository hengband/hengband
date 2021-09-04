#pragma once

/*
 * @file player-status.h
 * @brief プレーヤーのステータスに関する状態取得処理ヘッダ
 */

#include "system/angband.h"

struct object_type;;
struct player_type;
int weapon_exp_level(int weapon_exp);
int riding_exp_level(int riding_exp);
int spell_exp_level(int spell_exp);

WEIGHT calc_weapon_weight_limit(player_type *creature_ptr);
WEIGHT calc_bow_weight_limit(player_type *creature_ptr);
WEIGHT calc_inventory_weight(player_type *creature_ptr);

int16_t calc_num_fire(player_type *creature_ptr, object_type *o_ptr);
WEIGHT calc_weight_limit(player_type *creature_ptr);
void update_creature(player_type *creature_ptr);
bool player_has_no_spellbooks(player_type *creature_ptr);

bool player_place(player_type *creature_ptr, POSITION y, POSITION x);

void check_experience(player_type *creature_ptr);
void wreck_the_pattern(player_type *creature_ptr);
void cnv_stat(int val, char *out_val);
int16_t modify_stat_value(int value, int amount);
long calc_score(player_type *creature_ptr);

bool is_blessed(player_type *creature_ptr);
bool is_time_limit_esp(player_type *creature_ptr);
bool is_time_limit_stealth(player_type *creature_ptr);
bool is_fast(player_type *creature_ptr);
bool is_invuln(player_type *creature_ptr);
bool is_hero(player_type *creature_ptr);
bool is_shero(player_type *creature_ptr);
bool is_echizen(player_type *creature_ptr);
bool is_chargeman(player_type *creature_ptr);

void stop_mouth(player_type *caster_ptr);
