#pragma once

#include "system/angband.h"

/* 人畜無害なenumヘッダを先に読み込む */
#include "mutation/mutation-flag-types.h"
#include "player-info/base-status-types.h"
#include "spell/spells-util.h"
#include "util/flag-group.h"

// @todo 地雷、FlagGroupと強い関係がある
#include "player/player-classes-types.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"

/*
 * Most of the "player" information goes here.
 *
 * This stucture gives us a large collection of player variables.
 *
 * This structure contains several "blocks" of information.
 *   (1) the "permanent" info
 *   (2) the "variable" info
 *   (3) the "transient" info
 *
 * All of the "permanent" info, and most of the "variable" info,
 * is saved in the savefile.  The "transient" info is recomputed
 * whenever anything important changes.
 */

/*
 * Player constants
 */
#define PY_MAX_EXP 99999999L /*!< プレイヤー経験値の最大値 / Maximum exp */
#define PY_MAX_GOLD 999999999L /*!< プレイヤー所持金の最大値 / Maximum gold */

#define MAGIC_GLOVE_REDUCE_MANA 0x0001
#define MAGIC_FAIL_5PERCENT 0x0002
#define MAGIC_GAIN_EXP 0x0004

/* no_flowed 判定対象となるスペル */
#define SPELL_DD_S 27
#define SPELL_DD_T 13
#define SPELL_SW 22
#define SPELL_WALL 20

/*
 * Player "food" crucial values
 */
#define PY_FOOD_MAX 15000 /*!< 食べ過ぎ～満腹の閾値 / Food value (Bloated) */
#define PY_FOOD_FULL 10000 /*!< 満腹～平常の閾値 / Food value (Normal) */
#define PY_FOOD_ALERT 2000 /*!< 平常～空腹の閾値 / Food value (Hungry) */
#define PY_FOOD_WEAK 1000 /*!< 空腹～衰弱の閾値 / Food value (Weak) */
#define PY_FOOD_FAINT 500 /*!< 衰弱～衰弱(赤表示/麻痺)の閾値 / Food value (Fainting) */
#define PY_FOOD_STARVE 100 /*!< 衰弱(赤表示/麻痺)～飢餓ダメージの閾値 / Food value (Starving) */

/*
 * Player regeneration constants
 */
#define PY_REGEN_NORMAL 197 /* Regen factor*2^16 when full */
#define PY_REGEN_WEAK 98 /* Regen factor*2^16 when weak */
#define PY_REGEN_FAINT 33 /* Regen factor*2^16 when fainting */
#define PY_REGEN_HPBASE 1442 /* Min amount hp regen*2^16 */
#define PY_REGEN_MNBASE 524 /* Min amount mana regen*2^16 */

/*!< Empty hand status */
enum empty_hand_status {
    EMPTY_HAND_NONE = 0x0000, /*!<Both hands are used */
    EMPTY_HAND_SUB = 0x0001, /*!<Sub hand is empty */
    EMPTY_HAND_MAIN = 0x0002 /*!<Main hand is empty */
};

/*!< Weapon hand status */
enum player_hand { PLAYER_HAND_MAIN = 0x0000, PLAYER_HAND_SUB = 0x0001, PLAYER_HAND_OTHER = 0x0002 };

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

// @todo 後で普通の関数に直す
#define SINGING_SONG_EFFECT(P_PTR) ((P_PTR)->magic_num1[0])
#define INTERUPTING_SONG_EFFECT(P_PTR) ((P_PTR)->magic_num1[1])
#define SINGING_COUNT(P_PTR) ((P_PTR)->magic_num1[2])
#define SINGING_SONG_ID(P_PTR) ((P_PTR)->magic_num2[0])
#define music_singing_any(CREATURE_PTR) (((CREATURE_PTR)->pclass == CLASS_BARD) && (CREATURE_PTR)->magic_num1[0])
