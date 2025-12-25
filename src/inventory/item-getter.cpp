#include "inventory/item-getter.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "game-option/option-flags.h"
#include "game-option/text-display-options.h"
#include "inventory/floor-item-getter.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "inventory/item-selection-util.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "player/player-status-flags.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include <fmt/format.h>
#include <sstream>
#include <tl/optional.hpp>

/*!
 * @brief オブジェクト選択の汎用関数 / General function for the selection of item
 * Let the user select an item, save its "index"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cp 選択したオブジェクトのID
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す
 * Return TRUE only if an acceptable item was chosen by the user
 */
bool get_item(PlayerType *player_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester &item_tester)
{
    const auto floor_item_indice = get_item_floor(player_ptr, pmt, str, mode, item_tester);
    if (floor_item_indice) {
        *cp = *floor_item_indice;
        return true;
    }

    return false;
}
