#include "world/world-object.h"
#include "dungeon/dungeon-flag-types.h"
#include "object-enchant/item-apply-magic.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <iterator>

/*!
 * @brief グローバルオブジェクト配列から空きを取得する /
 * Acquires and returns the index of a "free" object.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @return 開いているオブジェクト要素のID
 * @details
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
OBJECT_IDX o_pop(FloorType *floor_ptr)
{
    if (floor_ptr->o_max < MAX_FLOOR_ITEMS) {
        const auto i = floor_ptr->o_max;
        floor_ptr->o_max++;
        floor_ptr->o_cnt++;
        return i;
    }

    for (short i = 1; i < floor_ptr->o_max; i++) {
        if (floor_ptr->o_list[i].is_valid()) {
            continue;
        }

        floor_ptr->o_cnt++;
        return i;
    }

    if (AngbandWorld::get_instance().character_dungeon) {
        msg_print(_("アイテムが多すぎる！", "Too many objects!"));
    }

    return 0;
}
