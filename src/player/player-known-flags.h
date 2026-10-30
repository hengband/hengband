/*!
 * @file player-known-flags.h
 * @brief プレイヤーの特性を供給源ごとに集める
 * @details キャラクター画面の特性のページと制御サーバの JSON が、本人・乗馬の特性を同じ集め方で出すために使う。
 */

#pragma once

#include "object-enchant/tr-flags.h"

class PlayerType;

/*!
 * @brief プレイヤーの特性を供給源ごとに集めたもの
 * @details キャラクター画面の特性のページの列と同じく、装備・本人 (常時と一時)・乗馬に分けて持ち、本人の免疫・弱点も持つ。
 * キャラクター画面は装備の列を装備ごとに描くので、equipment (全装備を合わせたもの) は使わず、装備ごとに get_flags_known() を読む。
 * immunity・temporary_immunity・vulnerability は、免疫・弱点を持つ属性を、その属性の耐性のビット (TR_RES_*) で持つ。
 * 免疫・弱点のビット (TR_IM_*・TR_VUL_*) では持たないので、たとえば火炎の免疫は immunity.has(TR_RES_FIRE) で調べる。
 */
struct PlayerKnownFlags {
    TrFlags equipment; //!< 鑑定済みの装備の特性 (get_flags_known())
    TrFlags permanent; //!< 種族・職業・突然変異・性格の特性 (player_flags())
    TrFlags temporary; //!< 時限効果と構えの特性 (tim_player_flags())
    TrFlags immunity; //!< 種族・職業による免疫を TR_RES_* で (player_immunity())
    TrFlags temporary_immunity; //!< 時限効果による免疫を TR_RES_* で (tim_player_immunity())
    TrFlags vulnerability; //!< 種族・突然変異・構えによる弱点を TR_RES_* で (player_vulnerability_flags())
    TrFlags riding; //!< 乗っている馬で得ている特性 (riding_flags())
    TrFlags riding_negative; //!< 乗っている馬のために失っている特性 (riding_flags())
};

PlayerKnownFlags collect_player_known_flags(PlayerType *player_ptr);
