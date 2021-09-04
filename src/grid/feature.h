#pragma once

#include "system/angband.h"

#include "grid/feature-flag-types.h"
#include "util/flag-group.h"

#include <string>
#include <vector>

/* Number of feats we change to (Excluding default). Used in f_info.txt. */
#define MAX_FEAT_STATES	 8

/* Lighting levels of features' attr and char */
#define F_LIT_STANDARD 0 /* Standard */
#define F_LIT_LITE     1 /* Brightly lit */
#define F_LIT_DARK     2 /* Darkened */

#define F_LIT_NS_BEGIN 1 /* Nonstandard */
#define F_LIT_MAX      3

/*!
 * @struct feature_state
 * @brief 地形状態変化指定構造体 / Feature state structure
 */
typedef struct feature_state {
    FF action{}; /*!< 変化条件をFF_*のIDで指定 / Action (FF_*) */
    std::string result_tag{}; /*!< 変化先ID / Result (f_info ID) */
    FEAT_IDX result{}; /*!< 変化先ID / Result (f_info ID) */
} feature_state;

/*!
 * @struct feature_type
 * @brief 地形情報の構造体 / Information about terrain "features"
 */
typedef struct feature_type {
    std::string name; /*!< 地形名参照のためのネームバッファオフセット値 / Name (offset) */
    std::string text; /*!< 地形説明参照のためのネームバッファオフセット値 /  Text (offset) */
    std::string tag; /*!< 地形特性タグ参照のためのネームバッファオフセット値 /  Tag (offset) */
    std::string mimic_tag;
    std::string destroyed_tag;
    FEAT_IDX mimic{}; /*!< 未確定時の外形地形ID / Feature to mimic */
    FEAT_IDX destroyed{}; /*!< *破壊*に巻き込まれた時の地形移行先(未実装？) / Default destroyed state */
    EnumClassFlagGroup<FF> flags{}; /*!< 地形の基本特性ビット配列 / Flags */
    FEAT_PRIORITY priority{}; /*!< 縮小表示で省略する際の表示優先度 / Map priority */
    feature_state state[MAX_FEAT_STATES]{}; /*!< feature_state テーブル */
    FEAT_SUBTYPE subtype{}; /*!< 副特性値 */
    FEAT_POWER power{}; /*!< 地形強度 */
    TERM_COLOR d_attr[F_LIT_MAX]{}; /*!< デフォルトの地形シンボルカラー / Default feature attribute */
    SYMBOL_CODE d_char[F_LIT_MAX]{}; /*!< デフォルトの地形シンボルアルファベット / Default feature character */
    TERM_COLOR x_attr[F_LIT_MAX]{}; /*!< 設定変更後の地形シンボルカラー / Desired feature attribute */
    SYMBOL_CODE x_char[F_LIT_MAX]{}; /*!< 設定変更後の地形シンボルアルファベット / Desired feature character */
} feature_type;

extern FEAT_IDX max_f_idx;
extern std::vector<feature_type> f_info;

/*** Terrain feature variables ***/
extern FEAT_IDX feat_none;
extern FEAT_IDX feat_floor;
extern FEAT_IDX feat_rune_protection;
extern FEAT_IDX feat_rune_explosion;
extern FEAT_IDX feat_mirror;
extern FEAT_IDX feat_up_stair;
extern FEAT_IDX feat_down_stair;
extern FEAT_IDX feat_entrance;
extern FEAT_IDX feat_trap_open;
extern FEAT_IDX feat_trap_armageddon;
extern FEAT_IDX feat_trap_piranha;
extern FEAT_IDX feat_rubble;
extern FEAT_IDX feat_magma_vein;
extern FEAT_IDX feat_quartz_vein;
extern FEAT_IDX feat_granite;
extern FEAT_IDX feat_permanent;
extern FEAT_IDX feat_glass_floor;
extern FEAT_IDX feat_glass_wall;
extern FEAT_IDX feat_permanent_glass_wall;
extern FEAT_IDX feat_pattern_start;
extern FEAT_IDX feat_pattern_1;
extern FEAT_IDX feat_pattern_2;
extern FEAT_IDX feat_pattern_3;
extern FEAT_IDX feat_pattern_4;
extern FEAT_IDX feat_pattern_end;
extern FEAT_IDX feat_pattern_old;
extern FEAT_IDX feat_pattern_exit;
extern FEAT_IDX feat_pattern_corrupted;
extern FEAT_IDX feat_black_market;
extern FEAT_IDX feat_town;
extern FEAT_IDX feat_deep_water;
extern FEAT_IDX feat_shallow_water;
extern FEAT_IDX feat_deep_lava;
extern FEAT_IDX feat_shallow_lava;
extern FEAT_IDX feat_heavy_cold_zone;
extern FEAT_IDX feat_cold_zone;
extern FEAT_IDX feat_heavy_electrical_zone;
extern FEAT_IDX feat_electrical_zone;
extern FEAT_IDX feat_deep_acid_puddle;
extern FEAT_IDX feat_shallow_acid_puddle;
extern FEAT_IDX feat_deep_poisonous_puddle;
extern FEAT_IDX feat_shallow_poisonous_puddle;
extern FEAT_IDX feat_dirt;
extern FEAT_IDX feat_grass;
extern FEAT_IDX feat_flower;
extern FEAT_IDX feat_brake;
extern FEAT_IDX feat_tree;
extern FEAT_IDX feat_mountain;
extern FEAT_IDX feat_swamp;
extern FEAT_IDX feat_undetected;

extern FEAT_IDX feat_wall_outer;
extern FEAT_IDX feat_wall_inner;
extern FEAT_IDX feat_wall_solid;
extern FEAT_IDX feat_ground_type[100];
extern FEAT_IDX feat_wall_type[100];

struct floor_type;
struct player_type;
bool is_closed_door(player_type *player_ptr, FEAT_IDX feat);
bool is_trap(player_type *player_ptr, FEAT_IDX feat);
void apply_default_feat_lighting(TERM_COLOR *f_attr, SYMBOL_CODE *f_char);
bool is_ascii_graphics(char x);
bool permanent_wall(feature_type *f_ptr);
FEAT_IDX feat_locked_door_random(int door_type);
FEAT_IDX feat_jammed_door_random(int door_type);
void cave_set_feat(player_type *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat);
