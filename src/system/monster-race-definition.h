#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-aura-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-population-flags.h"
#include "monster-race/race-visual-flags.h"
#include "monster-race/race-wilderness-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <string>
#include <tuple>
#include <vector>

/*! モンスターが1ターンに攻撃する最大回数 (射撃を含む) / The maximum number of times a monster can attack in a turn (including SHOOT) */
constexpr int MAX_NUM_BLOWS = 4;

enum class MonsterRaceId : int16_t;

class MonsterBlow {
public:
    RaceBlowMethodType method{};
    RaceBlowEffectType effect{};
    DICE_NUMBER d_dice{};
    DICE_SID d_side{};
};

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
    MonsterRaceId idx{};
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
    byte speed{}; //!< 加速(110で+0) / Speed (normally 110)
    EXP mexp{}; //!< 殺害時基本経験値 / Exp value for kill
    BIT_FLAGS16 extra{}; //!< 未使用 /  Unused (for now)
    RARITY freq_spell{}; //!< 魔法＆特殊能力仕様頻度(1/n) /  Spell frequency
    BIT_FLAGS flags1{}; //!< Flags 1 (general)
    BIT_FLAGS flags2{}; //!< Flags 2 (abilities)
    BIT_FLAGS flags3{}; //!< Flags 3 (race/resist)
    BIT_FLAGS flags7{}; //!< Flags 7 (movement related abilities)
    BIT_FLAGS flags8{}; //!< Flags 8 (wilderness info)
    EnumClassFlagGroup<MonsterAbilityType> ability_flags; //!< 能力フラグ(魔法/ブレス) / Ability Flags
    EnumClassFlagGroup<MonsterAuraType> aura_flags; //!< オーラフラグ / Aura Flags
    EnumClassFlagGroup<MonsterBehaviorType> behavior_flags; //!< 能力フラグ（習性）
    EnumClassFlagGroup<MonsterVisualType> visual_flags; //!< 能力フラグ（シンボル） / Symbol Flags
    EnumClassFlagGroup<MonsterKindType> kind_flags; //!< 能力フラグ（種族・徳） / Attr Flags
    EnumClassFlagGroup<MonsterResistanceType> resistance_flags; //!< 耐性フラグ / Flags R (resistances info)
    EnumClassFlagGroup<MonsterDropType> drop_flags; //!< 能力フラグ（ドロップ） / Drop Flags
    EnumClassFlagGroup<MonsterWildernessType> wilderness_flags; //!< 荒野フラグ / Wilderness Flags
    EnumClassFlagGroup<MonsterFeatureType> feature_flags; //!< 能力フラグ（地形関連） / Feature Flags
    EnumClassFlagGroup<MonsterPopulationType> population_flags; //!< 能力フラグ（出現数関連） / Population Flags
    MonsterBlow blow[MAX_NUM_BLOWS]{}; //!< 打撃能力定義 / Up to four blows per round

    //! 指定護衛リスト <モンスター種族ID,護衛数ダイス数,護衛数ダイス面>
    std::vector<std::tuple<MonsterRaceId, DICE_NUMBER, DICE_SID>> reinforces;

    //! 特定アーティファクトドロップリスト <アーティファクトID,ドロップ率>
    std::vector<std::tuple<ARTIFACT_IDX, PERCENTAGE>> drop_artifacts;

    PERCENTAGE arena_ratio{}; //!< モンスター闘技場の掛け金倍率修正値(%基準 / 0=100%) / The adjustment ratio for gambling monster
    MonsterRaceId next_r_idx{}; //!< 進化先モンスター種族ID
    EXP next_exp{}; //!< 進化に必要な経験値
    DEPTH level{}; //!< レベル / Level of creature
    RARITY rarity{}; //!< レアリティ / Rarity of creature
    TERM_COLOR d_attr{}; //!< デフォルトの表示色 / Default monster attribute
    char d_char{}; //!< デフォルトの表示文字 / Default monster character
    TERM_COLOR x_attr{}; //!< 設定した表示色(またはタイル位置Y) / Desired monster attribute
    char x_char{}; //!< 設定した表示文字(またはタイル位置X) / Desired monster character
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
    EnumClassFlagGroup<MonsterAbilityType> r_ability_flags; //!< 見た能力フラグ(魔法/ブレス) / Observed racial ability flags
    EnumClassFlagGroup<MonsterAuraType> r_aura_flags; //!< 見た能力フラグ(オーラ) / Observed aura flags
    EnumClassFlagGroup<MonsterBehaviorType> r_behavior_flags; //!< 見た能力フラグ（習性） / Observed racial attr flags
    EnumClassFlagGroup<MonsterKindType> r_kind_flags; //!< 見た能力フラグ（種族・徳） / Observed racial attr flags
    EnumClassFlagGroup<MonsterResistanceType> r_resistance_flags; //!< 見た耐性フラグ / Observed racial resistances flags
    EnumClassFlagGroup<MonsterDropType> r_drop_flags; //!< 見た能力フラグ（ドロップ） / Observed drop flags
    EnumClassFlagGroup<MonsterFeatureType> r_feature_flags; //!< 見た能力フラグ(地形関連) / Observed feature flags
    PLAYER_LEVEL defeat_level{}; //!< 倒したレベル(ユニーク用) / player level at which defeated this race
    REAL_TIME defeat_time{}; //!< 倒した時間(ユニーク用) / time at which defeated this race
    PERCENTAGE cur_hp_per{}; //!< 生成時現在HP率(%)
};
