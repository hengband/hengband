#pragma once

// clang-format off
enum player_update_type {
    PU_BONUS            = 0x00000001L, /*!< ステータス更新フラグ: 能力値修正 / Calculate bonuses */
    PU_TORCH            = 0x00000002L, /*!< ステータス更新フラグ: 光源半径 / Calculate torch radius */
    PU_HP               = 0x00000010L, /*!< ステータス更新フラグ: HP / Calculate chp and mhp */
    PU_MP               = 0x00000020L, /*!< ステータス更新フラグ: MP / Calculate csp and msp */
    PU_SPELLS           = 0x00000040L, /*!< ステータス更新フラグ: 魔法学習数 / Calculate spells */
    PU_COMBINATION      = 0x00000100L, /*!< アイテム処理フラグ: アイテムの結合を要する / Combine the pack */
    PU_REORDER          = 0x00000200L, /*!< アイテム処理フラグ: アイテムの並び替えを要する / Reorder the pack */
    PU_AUTO_DESTRUCTION = 0x00000400L, /*!< アイテム処理フラグ: アイテムの自動破壊を要する / Auto-destroy marked item */
    PU_UN_VIEW          = 0x00010000L, /*!< ステータス更新フラグ: 地形の視界外化 / Forget view */
    PU_UN_LITE          = 0x00020000L, /*!< ステータス更新フラグ: 明暗範囲の視界外化 / Forget lite */
    PU_VIEW             = 0x00100000L, /*!< ステータス更新フラグ: 視界 / Update view */
    PU_LITE             = 0x00200000L, /*!< ステータス更新フラグ: 明暗範囲 / Update lite */
    PU_MONSTER_LITE     = 0x00400000L, /*!< ステータス更新フラグ: モンスターの光源範囲 / Monster illumination */
    PU_DELAY_VISIBILITY = 0x00800000L, /*!< ステータス更新フラグ: 視界の追加更新 / Mega-Hack -- Delayed visual update */
    PU_MONSTER_STATUSES = 0x01000000L, /*!< ステータス更新フラグ: モンスターのステータス / Update monsters */
    PU_DISTANCE         = 0x02000000L, /*!< ステータス更新フラグ: プレイヤーとモンスターの距離 / Update distances */
    PU_FLOW             = 0x10000000L, /*!< ステータス更新フラグ: プレイヤーから各マスへの到達距離 / Update flow */
};

// clang-format on
