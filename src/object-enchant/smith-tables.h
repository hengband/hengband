#pragma once

#include "system/angband.h"

#include <vector>

enum class SmithEssence : int16_t;
enum tr_type : int32_t;

/*!
 * @brief エッセンス抽出情報構造体
 */
struct essence_drain_type {
    tr_type tr_flag; //!< 抽出する対象アイテムの持つ特性フラグ
    std::vector<SmithEssence> essences; //!< 抽出されるエッセンスのリスト
    int amount; //! エッセンス抽出量。ただしマイナスのものは抽出時のペナルティ源として扱う
};
