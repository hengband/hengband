#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include "world/world.h"

/*!
 * 賞金首の報酬テーブル / List of prize object
 */
typedef struct bounty_prize_type {
    tval_type tval; /*!< ベースアイテムのメイン種別ID */
    OBJECT_SUBTYPE_VALUE sval; /*!< ベースアイテムのサブ種別ID */
} bounty_prize_type;

extern bounty_prize_type prize_list[MAX_BOUNTY];
