#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
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

/*
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

typedef struct monster_race {
    std::string name; /*!< 名前データのオフセット(日本語) /  Name offset(Japanese) */
#ifdef JP
    std::string E_name; /*!< 名前データのオフセット(英語) /  Name offset(English) */
#endif
    std::string text; /*!< 思い出テキストのオフセット / Lore text offset */
    DICE_NUMBER hdice{}; /*!< HPのダイス数 / Creatures hit dice count */
    DICE_SID hside{}; /*!< HPのダイス面数 / Creatures hit dice sides */
    ARMOUR_CLASS ac{}; /*!< アーマークラス / Armour Class */
    SLEEP_DEGREE sleep{}; /*!< 睡眠値 / Inactive counter (base) */
    POSITION aaf{}; /*!< 感知範囲(1-100スクエア) / Area affect radius (1-100) */
    SPEED speed{}; /*!< 加速(110で+0) / Speed (normally 110) */
    EXP mexp{}; /*!< 殺害時基本経験値 / Exp value for kill */
    BIT_FLAGS16 extra{}; /*!< 未使用 /  Unused (for now) */
    RARITY freq_spell{}; /*!< 魔法＆特殊能力仕様頻度(1/n) /  Spell frequency */
    BIT_FLAGS flags1{}; /* Flags 1 (general) */
    BIT_FLAGS flags2{}; /* Flags 2 (abilities) */
    BIT_FLAGS flags3{}; /* Flags 3 (race/resist) */
    BIT_FLAGS flags4{}; /* Flags 4 (inate/breath) */
    BIT_FLAGS flags7{}; /* Flags 7 (movement related abilities) */
    BIT_FLAGS flags8{}; /* Flags 8 (wilderness info) */
    BIT_FLAGS flags9{}; /* Flags 9 (drops info) */
    BIT_FLAGS flagsr{}; /* Flags R (resistances info) */
    BIT_FLAGS a_ability_flags1{}; /* Activate Ability Flags 5 (normal spells) */
    BIT_FLAGS a_ability_flags2{}; /* Activate Ability Flags 6 (special spells) */
    BIT_FLAGS a_ability_flags3{}; /* Activate Ability Flags 7 (implementing) */
    BIT_FLAGS a_ability_flags4{}; /* Activate Ability Flags 8 (implementing) */
    monster_blow blow[MAX_NUM_BLOWS]{}; /* Up to four blows per round */
    MONRACE_IDX reinforce_id[6]{};
    DICE_NUMBER reinforce_dd[6]{};
    DICE_SID reinforce_ds[6]{};
    ARTIFACT_IDX artifact_id[4]{}; /* 特定アーティファクトドロップID */
    RARITY artifact_rarity[4]{}; /* 特定アーティファクトレア度 */
    PERCENTAGE artifact_percent[4]{}; /* 特定アーティファクトドロップ率 */
    PERCENTAGE arena_ratio{}; /* モンスター闘技場の掛け金倍率修正値(%基準 / 0=100%) / The adjustment ratio for gambling monster */
    MONRACE_IDX next_r_idx{};
    EXP next_exp{};
    DEPTH level{}; /* Level of creature */
    RARITY rarity{}; /* Rarity of creature */
    TERM_COLOR d_attr{}; /* Default monster attribute */
    SYMBOL_CODE d_char{}; /* Default monster character */
    TERM_COLOR x_attr{}; /* Desired monster attribute */
    SYMBOL_CODE x_char{}; /* Desired monster character */
    MONSTER_NUMBER max_num{}; /* Maximum population allowed per level */
    MONSTER_NUMBER cur_num{}; /* Monster population on current level */
    FLOOR_IDX floor_id{}; /* Location of unique monster */
    MONSTER_NUMBER r_sights{}; /* Count sightings of this monster */
    MONSTER_NUMBER r_deaths{}; /* Count deaths from this monster */
    MONSTER_NUMBER r_pkills{}; /* Count visible monsters killed in this life */
    MONSTER_NUMBER r_akills{}; /* Count all monsters killed in this life */
    MONSTER_NUMBER r_tkills{}; /* Count monsters killed in all lives */
    byte r_wake{}; /* Number of times woken up (?) */
    byte r_ignore{}; /* Number of times ignored (?) */
#define MR1_EVOLUTION 0x01
    byte r_xtra1{}; /* Something */
    byte r_xtra2{}; /* Something (unused) */
    ITEM_NUMBER r_drop_gold{}; /*!< これまでに撃破時に落とした財宝の数 / Max number of gold dropped at once */
    ITEM_NUMBER r_drop_item{}; /*!< これまでに撃破時に落としたアイテムの数 / Max number of item dropped at once */
    byte r_cast_spell{}; /* Max number of other spells seen */
    byte r_blows[MAX_NUM_BLOWS]{}; /* Number of times each blow type was seen */
    u32b r_flags1{}; /* Observed racial flags */
    u32b r_flags2{}; /* Observed racial flags */
    u32b r_flags3{}; /* Observed racial flags */
    u32b r_flags4{}; /* Observed racial flags */
    u32b r_flags5{}; /* Observed racial flags */
    u32b r_flags6{}; /* Observed racial flags */
    u32b r_flagsr{}; /* Observed racial resistance flags */
} monster_race;
