#pragma once

#include "system/angband.h"

// clang-format off
/*!
 * @brief サブウィンドウ描画フラグ
 */
enum window_redraw_type {
    PW_INVENTORY       = 1U <<  0, /*!<サブウィンドウ描画フラグ: 所持品-装備品 / Display inven/equip */
    PW_EQUIPMENT       = 1U <<  1, /*!<サブウィンドウ描画フラグ: 装備品-所持品 / Display equip/inven */
    PW_SPELL           = 1U <<  2, /*!<サブウィンドウ描画フラグ: 魔法一覧 / Display spell list */
    PW_PLAYER          = 1U <<  3, /*!<サブウィンドウ描画フラグ: プレイヤーのステータス / Display character */
    PW_SIGHT_MONSTERS  = 1U <<  4, /*!<サブウィンドウ描画フラグ: 視界内モンスターの一覧 / Display monster list */
    PW_MESSAGE         = 1U <<  6, /*!<サブウィンドウ描画フラグ: メッセージログ / Display messages */
    PW_OVERHEAD        = 1U <<  7, /*!<サブウィンドウ描画フラグ: 周辺の光景 / Display overhead view */
    PW_MONSTER_LORE    = 1U <<  8, /*!<サブウィンドウ描画フラグ: モンスターの思い出 / Display monster recall */
    PW_ITEM_KNOWLEDGTE = 1U <<  9, /*!<サブウィンドウ描画フラグ: アイテムの知識 / Display object recall */
    PW_DUNGEON         = 1U << 10, /*!<サブウィンドウ描画フラグ: ダンジョンの地形 / Display dungeon view */
    PW_SNAPSHOT        = 1U << 11, /*!<サブウィンドウ描画フラグ: 記念写真 / Display snap-shot */
    PW_FLOOR_ITEMS     = 1U << 12, /*!<サブウィンドウ描画フラグ: 床上のアイテム一覧 / Display items on grid */
    PW_FOUND_ITEMS     = 1U << 13, /*!<サブウィンドウ描画フラグ: 発見済みのアイテム一覧 / Display found items*/

    PW_ALL = (PW_INVENTORY | PW_EQUIPMENT | PW_SPELL | PW_PLAYER | PW_SIGHT_MONSTERS | PW_MESSAGE | PW_OVERHEAD | PW_MONSTER_LORE | PW_ITEM_KNOWLEDGTE | PW_DUNGEON | PW_SNAPSHOT | PW_FLOOR_ITEMS | PW_FOUND_ITEMS),
};

// clang-format on
class PlayerType;
void redraw_window(void);
void window_stuff(PlayerType *player_ptr);
void redraw_stuff(PlayerType *player_ptr);
