#pragma once

/*
 * @file player-status.h
 * @brief プレーヤーのステータスに関する状態取得処理ヘッダ
 */

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
concptr your_alignment(player_type *creature_ptr, bool with_value = false);
int weapon_exp_level(int weapon_exp);
int riding_exp_level(int riding_exp);
int spell_exp_level(int spell_exp);

int calc_weapon_weight_limit(player_type *creature_ptr);
WEIGHT calc_inventory_weight(player_type *creature_ptr);

s16b calc_num_fire(player_type *creature_ptr, object_type *o_ptr);
WEIGHT calc_weight_limit(player_type *creature_ptr);
bool has_melee_weapon(player_type *creature_ptr, int i);

bool heavy_armor(player_type *creature_ptr);
void update_creature(player_type *creature_ptr);
BIT_FLAGS16 empty_hands(player_type *creature_ptr, bool riding_control);
bool player_has_no_spellbooks(player_type *creature_ptr);

void take_turn(player_type *creature_ptr, PERCENTAGE need_cost);
void free_turn(player_type *creature_ptr);

bool player_place(player_type *creature_ptr, POSITION y, POSITION x);

void check_experience(player_type *creature_ptr);
void wreck_the_pattern(player_type *creature_ptr);
void cnv_stat(int val, char *out_val);
s16b modify_stat_value(int value, int amount);
long calc_score(player_type *creature_ptr);

bool is_blessed(player_type *creature_ptr);
bool is_time_limit_esp(player_type *creature_ptr);
bool is_time_limit_stealth(player_type *creature_ptr);
bool can_two_hands_wielding(player_type *creature_ptr);
bool is_fast(player_type *creature_ptr);
bool is_invuln(player_type *creature_ptr);
bool is_hero(player_type *creature_ptr);
bool is_shero(player_type *creature_ptr);
bool is_echizen(player_type *creature_ptr);
bool is_in_dungeon(player_type *creature_ptr);

void stop_singing(player_type *creature_ptr);
void stop_mouth(player_type *caster_ptr);
PERCENTAGE calculate_upkeep(player_type *creature_ptr);
bool music_singing(player_type *caster_ptr, int music_songs);
bool music_singing_any(player_type *creature_ptr);

MAGIC_NUM1 get_singing_song_effect(const player_type *creature_ptr);
void set_singing_song_effect(player_type *creature_ptr, const MAGIC_NUM1 magic_num);
MAGIC_NUM1 get_interrupting_song_effect(const player_type *creature_ptr);
void set_interrupting_song_effect(player_type *creature_ptr, const MAGIC_NUM1 magic_num);
MAGIC_NUM1 get_singing_count(const player_type *creature_ptr);
void set_singing_count(player_type *creature_ptr, const MAGIC_NUM1 magic_num);
MAGIC_NUM2 get_singing_song_id(const player_type *creature_ptr);
void set_singing_song_id(player_type *creature_ptr, const MAGIC_NUM2 magic_num);
