#pragma once

#include "system/angband.h"

// clang-format off
/*!
 * @brief サブウィンドウ描画フラグ
 */
enum window_redraw_type {
    PW_INVEN           = 1U <<  0, /*!<サブウィンドウ描画フラグ: 所持品-装備品 / Display inven/equip */
    PW_EQUIP           = 1U <<  1, /*!<サブウィンドウ描画フラグ: 装備品-所持品 / Display equip/inven */
    PW_SPELL           = 1U <<  2, /*!<サブウィンドウ描画フラグ: 魔法一覧 / Display spell list */
    PW_PLAYER          = 1U <<  3, /*!<サブウィンドウ描画フラグ: プレイヤーのステータス / Display character */
    PW_MONSTER_LIST    = 1U <<  4, /*!<サブウィンドウ描画フラグ: 視界内モンスターの一覧 / Display monster list */
    PW_MESSAGE         = 1U <<  6, /*!<サブウィンドウ描画フラグ: メッセージログ / Display messages */
    PW_OVERHEAD        = 1U <<  7, /*!<サブウィンドウ描画フラグ: 周辺の光景 / Display overhead view */
    PW_MONSTER         = 1U <<  8, /*!<サブウィンドウ描画フラグ: モンスターの思い出 / Display monster recall */
    PW_OBJECT          = 1U <<  9, /*!<サブウィンドウ描画フラグ: アイテムの知識 / Display object recall */
    PW_DUNGEON         = 1U << 10, /*!<サブウィンドウ描画フラグ: ダンジョンの地形 / Display dungeon view */
    PW_SNAPSHOT        = 1U << 11, /*!<サブウィンドウ描画フラグ: 記念写真 / Display snap-shot */
    PW_FLOOR_ITEM_LIST = 1U << 12, /*!<サブウィンドウ描画フラグ: 床上のアイテム一覧 / Display items at feet */

    PW_ALL = (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_MONSTER_LIST | PW_MESSAGE | PW_OVERHEAD | PW_MONSTER | PW_OBJECT | PW_DUNGEON | PW_SNAPSHOT | PW_FLOOR_ITEM_LIST),
};

// clang-format on
struct player_type;
void redraw_window(void);
void window_stuff(player_type *player_ptr);
void redraw_stuff(player_type *player_ptr);
