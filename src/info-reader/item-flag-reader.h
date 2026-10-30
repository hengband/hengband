#pragma once

/*!
 * @file item-flag-reader.h
 * @brief アイテムの定義（ベースアイテム・固定アーティファクト・エゴ）に共通するフラグ名の読み込み
 */

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "util/flag-group.h"
#include <string_view>

bool grab_one_item_flag(TrFlags &flags, EnumClassFlagGroup<ItemGenerationTraitType> &gen_flags, std::string_view what);
