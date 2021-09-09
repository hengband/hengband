#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

#include <vector>

enum class SmithEffect;
enum class SmithCategory;
enum class SmithEssence;

/*!
 * @brief エッセンス抽出情報構造体
 */
struct essence_drain_type {
    tr_type tr_flag; //!< 抽出する対象アイテムの持つ特性フラグ
    std::vector<SmithEssence> essences; //!< 抽出されるエッセンスのリスト
    int amount; //! エッセンス抽出量。ただしマイナスのものは抽出時のペナルティ源として扱う
};

/*!
 * @brief 鍛冶情報の構造体
 */
struct smith_info_type {
    SmithEffect effect; //!< 鍛冶で与える効果の種類
    concptr name; //!< 鍛冶で与える能力の名称
    SmithCategory category; //!< 鍛冶で与える能力が所属するグループ
    std::vector<SmithEssence> need_essences; //!< 能力を与えるのに必要なエッセンスのリスト
    int consumption; //!< 能力を与えるのに必要な消費量(need_essencesに含まれるエッセンスそれぞれについてこの量を消費)
    TrFlags add_flags; //!< 鍛冶で能力を与えることにより付与されるアイテム特性フラグ
};
