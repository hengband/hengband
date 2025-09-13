#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include "util/point-2d.h"

/*!
 * @brief scan_floor_items() の動作を指定するフラグたち。
 */
enum class ScanFloorMode {
    ITEM_TESTER = 0, /*!< item_tester_hook によるフィルタリングを適用する */
    ONLY_MARKED = 1, /*!< マークされたアイテムのみを対象とする */
    AT_MOST_ONE = 2, /*!< 高々1つのアイテムしか取得しない */
    MAX,
};

class FloorType;
class PlayerType;
class ItemTester;
int scan_floor_items(const FloorType &floor, OBJECT_IDX *items, const Pos2D &pos, EnumClassFlagGroup<ScanFloorMode> mode, const ItemTester &item_tester);
COMMAND_CODE show_floor_items(PlayerType *player_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, const ItemTester &item_tester);
