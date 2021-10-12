#pragma once

#include "system/angband.h"
#include "world/world.h"

/*!
 * 賞金首の報酬テーブル / List of prize object
 */
enum class ItemKindType : short;
struct bounty_prize_type {
    ItemKindType tval; /*!< ベースアイテムのメイン種別ID */
    OBJECT_SUBTYPE_VALUE sval; /*!< ベースアイテムのサブ種別ID */
};

extern bounty_prize_type prize_list[MAX_BOUNTY];
