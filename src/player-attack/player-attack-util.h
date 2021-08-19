﻿#pragma once

#include "combat/combat-options-type.h"
#include "combat/martial-arts-table.h"
#include "system/angband.h"
#include "system/system-variables.h"

/*!
 * @brief カオス効果種別
 */
enum chaotic_effect {
    CE_NONE = 0,
    CE_VAMPIRIC = 1,
    CE_QUAKE = 2,
    CE_CONFUSION = 3,
    CE_TELE_AWAY = 4,
    CE_POLYMORPH = 5,
};

/*!
 * @brief 魔術効果種別
 */
enum class MagicalBrandEffect { NONE = 0, EXTRA = 1, STUN = 2, SCARE = 3, DISPELL = 4, PROBE = 5, MAX };

/*!
 * @brief プレイヤーの打撃に関する情報
 */
typedef struct grid_type grid_type;
typedef struct monster_race monster_race;
typedef struct monster_type monster_type;
typedef struct player_attack_type {
    short hand{}; //!< 武器の持ち手
    grid_type *g_ptr; //!< ターゲットのいる地形情報
    MONSTER_IDX m_idx{}; //!< モンスターID
    monster_type *m_ptr{}; //!< モンスター情報(参照ポインタ)
    GAME_TEXT m_name[MAX_NLEN]{}; //!< モンスター名
    MONRACE_IDX r_idx{}; //!< モンスター種族ID
    monster_race *r_ptr{}; //!< モンスター種族情報(参照ポインタ)
    bool backstab{}; //!< 眠っている敵を不意打ちしたか
    bool surprise_attack{}; //!< 気づいていない敵に闇討ちしたか(忍者)
    bool stab_fleeing{}; //!< 逃げている敵の背後を襲ったか
    combat_options mode{}; //!< 剣術種別
    bool monk_attack{}; //!< 素手/マーシャルアーツかどうか
    const martial_arts *ma_ptr{}; //!< マーシャルアーツ種別
    HIT_POINT attack_damage{}; //!< 累積ダメージ
    int num_blow{}; //!< 打撃回数
    BIT_FLAGS flags[TR_FLAG_SIZE]{}; //!< 武器フラグ
    chaotic_effect chaos_effect{}; //!< カオス効果
    MagicalBrandEffect magical_effect{}; //!< 魔術効果
    bool *fear{}; //!< 恐怖したかどうか
    bool *mdeath{}; //!< 死んだかどうか
    bool can_drain{}; //!< 吸血できるかどうか
    int drain_result{}; //!< 吸血した累積量
    int drain_left{}; //!< 吸血できる残量(最大MAX_VAMPIRIC_DRAIN)
    bool weak{}; //!< 打撃効果でモンスターが弱くなったかどうか
} player_attack_type;
