#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"
#include "system/angband.h"
#include <string>

/*! モンスターが1ターンに攻撃する最大回数 (射撃を含む) / The maximum number of times a monster can attack in a turn (including SHOOT) */
#define MAX_NUM_BLOWS 4

/*
 * Monster blow structure
 *
 *	- Method (RBM_*)
 *	- Effect (RBE_*)
 *	- Damage Dice
 *	- Damage Sides
 */
typedef struct monster_blow {
    rbm_type method{};
    rbe_type effect{};
    DICE_NUMBER d_dice{};
    DICE_SID d_side{};
} monster_blow;

/*!
 * @brief モンスター種族の定義構造体
 * @details
 * Monster "race" information, including racial memories
 *
 * Note that "d_attr" and "d_char" are used for MORE than "visual" stuff.
 *
 * Note that "x_attr" and "x_char" are used ONLY for "visual" stuff.
 *
 * Note that "cur_num" (and "max_num") represent the number of monsters
 * of the given race currently on (and allowed on) the current level.
 * This information yields the "dead" flag for Unique monsters.
 *
 * Note that "max_num" is reset when a new player is created.
 * Note that "cur_num" is reset when a new level is created.
 *
 * Note that several of these fields, related to "recall", can be
 * scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc).  All of the "recall"
 * fields have a special prefix to aid in searching for them.
 */
struct monster_race {
    MONRACE_IDX idx{};
    std::string name; //!< 名前データのオフセット(日本語) /  Name offset(Japanese)
#ifdef JP
    std::string E_name; //!< 名前データのオフセット(英語) /  Name offset(English)
#endif
    std::string text; //!< 思い出テキストのオフセット / Lore text offset
    DICE_NUMBER hdice{}; //!< HPのダイス数 / Creatures hit dice count
    DICE_SID hside{}; //!< HPのダイス面数 / Creatures hit dice sides
    ARMOUR_CLASS ac{}; //!< アーマークラス / Armour Class
    SLEEP_DEGREE sleep{}; //!< 睡眠値 / Inactive counter (base)
    POSITION aaf{}; //!< 感知範囲(1-100スクエア) / Area affect radius (1-100)
    SPEED speed{}; //!< 加速(110で+0) / Speed (normally 110)
    EXP mexp{}; //!< 殺害時基本経験値 / Exp value for kill
    BIT_FLAGS16 extra{}; //!< 未使用 /  Unused (for now)
    RARITY freq_spell{}; //!< 魔法＆特殊能力仕様頻度(1/n) /  Spell frequency
    BIT_FLAGS flags1{}; //!< Flags 1 (general)
    BIT_FLAGS flags2{}; //!< Flags 2 (abilities)
    BIT_FLAGS flags3{}; //!< Flags 3 (race/resist)
    BIT_FLAGS flags7{}; //!< Flags 7 (movement related abilities)
    BIT_FLAGS flags8{}; //!< Flags 8 (wilderness info)
    BIT_FLAGS flags9{}; //!< Flags 9 (drops info)
    BIT_FLAGS flagsr{}; //!< 耐性フラグ / Flags R (resistances info)
    EnumClassFlagGroup<RF_ABILITY> ability_flags; //!< 能力フラグ(魔法/ブレス) / Ability Flags
    monster_blow blow[MAX_NUM_BLOWS]{}; //!< 打撃能力定義 / Up to four blows per round
    MONRACE_IDX reinforce_id[6]{}; //!< 指定護衛モンスター種族ID(6種まで)
    DICE_NUMBER reinforce_dd[6]{}; //!< 指定護衛数ダイス数
    DICE_SID reinforce_ds[6]{}; //!< 指定護衛数ダイス面
    ARTIFACT_IDX artifact_id[4]{}; //!< 特定アーティファクトドロップID
    RARITY artifact_rarity[4]{}; //!< 特定アーティファクトレア度
    PERCENTAGE artifact_percent[4]{}; //!< 特定アーティファクトドロップ率
    PERCENTAGE arena_ratio{}; //!< モンスター闘技場の掛け金倍率修正値(%基準 / 0=100%) / The adjustment ratio for gambling monster
    MONRACE_IDX next_r_idx{}; //!< 進化先モンスター種族ID
    EXP next_exp{}; //!< 進化に必要な経験値
    DEPTH level{}; //!< レベル / Level of creature
    RARITY rarity{}; //!< レアリティ / Rarity of creature
    TERM_COLOR d_attr{}; //!< デフォルトの表示色 / Default monster attribute
    SYMBOL_CODE d_char{}; //!< デフォルトの表示文字 / Default monster character
    TERM_COLOR x_attr{}; //!< 設定した表示色(またはタイル位置Y) / Desired monster attribute
    SYMBOL_CODE x_char{}; //!< 設定した表示文字(またはタイル位置X) / Desired monster character
    MONSTER_NUMBER max_num{}; //!< 階に最大存在できる数 / Maximum population allowed per level
    MONSTER_NUMBER cur_num{}; //!< 階に現在いる数 / Monster population on current level
    FLOOR_IDX floor_id{}; //!< 存在している保存階ID /  Location of unique monster
    MONSTER_NUMBER r_sights{}; //!< 見えている数 / Count sightings of this monster
    MONSTER_NUMBER r_deaths{}; //!< このモンスターに殺された人数 / Count deaths from this monster
    MONSTER_NUMBER r_pkills{}; //!< このゲームで倒すのを見た数 / Count visible monsters killed in this life
    MONSTER_NUMBER r_akills{}; //!< このゲームで倒した数 / Count all monsters killed in this life
    MONSTER_NUMBER r_tkills{}; //!< 全ゲームで倒した数 / Count monsters killed in all lives
    byte r_wake{}; //!< @に気づいて起きた数 / Number of times woken up (?)
    byte r_ignore{}; //!< @に気づいていない数 / Number of times ignored (?)
    bool r_can_evolve{}; //!< 進化するか否か / Flag being able to evolve
    byte r_xtra2{}; //!< 未使用 / Something (unused)
    ITEM_NUMBER r_drop_gold{}; //!< これまでに撃破時に落とした財宝の数 / Max number of gold dropped at once
    ITEM_NUMBER r_drop_item{}; //!< これまでに撃破時に落としたアイテムの数 / Max number of item dropped at once
    byte r_cast_spell{}; //!< 使った魔法/ブレスの種類数 /  Max unique number of spells seen
    byte r_blows[MAX_NUM_BLOWS]{}; //!< 受けた打撃 /  Number of times each blow type was seen
    uint32_t r_flags1{}; //!< Observed racial flags
    uint32_t r_flags2{}; //!< Observed racial flags
    uint32_t r_flags3{}; //!< Observed racial flags
    uint32_t r_flagsr{}; //!< 見た耐性フラグ / Observed racial resistance flags
    EnumClassFlagGroup<RF_ABILITY> r_ability_flags; //!< 見た能力フラグ(魔法/ブレス) / Observed racial ability flags
    PLAYER_LEVEL defeat_level{}; //!< 倒したレベル(ユニーク用) / player level at which defeated this race
    REAL_TIME defeat_time{}; //!< 倒した時間(ユニーク用) / time at which defeated this race
};
