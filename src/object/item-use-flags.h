#pragma once

/*
 * get_item()関数でアイテムの選択を行うフラグ / Bit flags for the "get_item" function
 */
enum item_use_flag {
    USE_EQUIP = 0x01, /*!< アイテム表示/選択範囲: 装備品からの選択を許可する / Allow equip items */
    USE_INVEN = 0x02, /*!< アイテム表示/選択範囲: 所持品からの選択を許可する /  Allow inven items */
    USE_FLOOR = 0x04, /*!< アイテム表示/選択範囲: 床下のアイテムからの選択を許可する /  Allow floor items */
    USE_FORCE = 0x08, /*!< 特殊: wキーで錬気術への切り替えを許可する */
    IGNORE_BOTHHAND_SLOT = 0x10, /*!< アイテム表示/選択範囲: 両手持ちスロットを選択に含めない */
    USE_FULL = 0x20, /*!< アイテム表示/選択範囲: 空欄まですべて表示する*/
};
