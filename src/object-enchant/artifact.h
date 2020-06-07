#pragma once

#include "system/angband.h"
#include "cmd-item/cmd-activate.h"

/*!
 * @struct artifact_type
 * @brief 固定アーティファクト情報の構造体 / Artifact structure.
 * @details
 * @note
 * the save-file only writes "cur_num" to the savefile.
 * "max_num" is always "1" (if that artifact "exists")
 */
typedef struct artifact_type {
	STR_OFFSET name;			/*!< アーティファクト名(headerオフセット参照) / Name (offset) */
	STR_OFFSET text;			/*!< アーティファクト解説(headerオフセット参照) / Text (offset) */
	tval_type tval;		/*!< ベースアイテム大項目ID / Artifact type */
	OBJECT_SUBTYPE_VALUE sval;	/*!< ベースアイテム小項目ID / Artifact sub type */
	PARAMETER_VALUE pval;	/*!< pval修正値 / Artifact extra info */
	HIT_PROB to_h;			/*!< 命中ボーナス値 /  Bonus to hit */
	HIT_POINT to_d;		/*!< ダメージボーナス値 / Bonus to damage */
	ARMOUR_CLASS to_a;			/*!< ACボーナス値 / Bonus to armor */
	ARMOUR_CLASS ac;			/*!< 上書きベースAC値 / Base armor */
	DICE_NUMBER dd;
	DICE_SID ds;	/*!< ダイス値 / Damage when hits */
	WEIGHT weight;		/*!< 重量 / Weight */
	PRICE cost;			/*!< 基本価格 / Artifact "cost" */
	BIT_FLAGS flags[TR_FLAG_SIZE];       /*! アイテムフラグ / Artifact Flags */
	BIT_FLAGS gen_flags;		/*! アイテム生成フラグ / flags for generate */
	DEPTH level;		/*! 基本生成階 / Artifact level */
	RARITY rarity;		/*! レアリティ / Artifact rarity */
	byte cur_num;		/*! 現在の生成数 / Number created (0 or 1) */
	byte max_num;		/*! (未使用)最大生成数 / Unused (should be "1") */
	FLOOR_IDX floor_id;      /*! アイテムを落としたフロアのID / Leaved on this location last time */
	byte act_idx;		/*! 発動能力ID / Activative ability index */
} artifact_type;

extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;
extern ARTIFACT_IDX max_a_idx;

bool become_random_artifact(player_type *player_ptr, object_type *o_ptr, bool a_scroll);
int activation_index(object_type *o_ptr);
const activation_type* find_activation_info(object_type *o_ptr);
void random_artifact_resistance(player_type *player_ptr, object_type *o_ptr, artifact_type *a_ptr);
bool create_named_art(player_type *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x);
bool make_artifact(player_type *player_ptr, object_type *o_ptr);
bool make_artifact_special(player_type *player_ptr, object_type *o_ptr);

/* Bows */
#define ART_BELTHRONDING        124
#define ART_BARD                125
#define ART_BRAND               126
#define ART_CRIMSON             16
#define ART_BUCKLAND            134
#define ART_YOICHI              148
#define ART_HARAD               180
#define ART_NAMAKE_BOW          182
#define ART_ROBIN_HOOD          221
#define ART_HELLFIRE            222

/* Arrows */
#define ART_BARD_ARROW          153


/* "Biases" for random artifact gen */
#define BIAS_ELEC            1 /*!< ランダムアーティファクトバイアス:電撃 */
#define BIAS_POIS            2 /*!< ランダムアーティファクトバイアス:毒 */
#define BIAS_FIRE            3 /*!< ランダムアーティファクトバイアス:火炎 */
#define BIAS_COLD            4 /*!< ランダムアーティファクトバイアス:冷気 */
#define BIAS_ACID            5 /*!< ランダムアーティファクトバイアス:酸 */
#define BIAS_STR             6 /*!< ランダムアーティファクトバイアス:腕力 */
#define BIAS_INT             7 /*!< ランダムアーティファクトバイアス:知力 */
#define BIAS_WIS             8 /*!< ランダムアーティファクトバイアス:賢さ */
#define BIAS_DEX             9 /*!< ランダムアーティファクトバイアス:器用さ */
#define BIAS_CON            10 /*!< ランダムアーティファクトバイアス:耐久 */
#define BIAS_CHR            11 /*!< ランダムアーティファクトバイアス:魅力 */
#define BIAS_CHAOS          12 /*!< ランダムアーティファクトバイアス:混沌 */
#define BIAS_PRIESTLY       13 /*!< ランダムアーティファクトバイアス:プリースト系 */
#define BIAS_NECROMANTIC    14 /*!< ランダムアーティファクトバイアス:死霊 */
#define BIAS_LAW            15 /*!< ランダムアーティファクトバイアス:法 */
#define BIAS_ROGUE          16 /*!< ランダムアーティファクトバイアス:盗賊系 */
#define BIAS_MAGE           17 /*!< ランダムアーティファクトバイアス:メイジ系 */
#define BIAS_WARRIOR        18 /*!< ランダムアーティファクトバイアス:戦士系 */
#define BIAS_RANGER         19 /*!< ランダムアーティファクトバイアス:レンジャー系 */
#define MAX_BIAS            20 /*!< ランダムアーティファクトバイアス:最大数 */
