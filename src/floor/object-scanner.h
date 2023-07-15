#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

/*!
 * @brief scan_floor_items() の動作を指定するフラグたち。
 */
enum scan_floor_mode {
    SCAN_FLOOR_ITEM_TESTER = 1U << 0, /*!< item_tester_hook によるフィルタリングを適用する */
    SCAN_FLOOR_ONLY_MARKED = 1U << 1, /*!< マークされたアイテムのみを対象とする */
    SCAN_FLOOR_AT_MOST_ONE = 1U << 2, /*!< 高々1つのアイテムしか取得しない */
};

class PlayerType;
class ItemTester;
ITEM_NUMBER scan_floor_items(PlayerType *player_ptr, OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode, const ItemTester &item_tester);
COMMAND_CODE show_floor_items(PlayerType *player_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, const ItemTester &item_tester);
