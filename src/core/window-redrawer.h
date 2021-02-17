#pragma once

#include "system/angband.h"

typedef enum window_redraw_type {
    PW_INVEN = 0x00000001L, /*!<サブウィンドウ描画フラグ: 所持品-装備品 / Display inven/equip */
    PW_EQUIP = 0x00000002L, /*!<サブウィンドウ描画フラグ: 装備品-所持品 / Display equip/inven */
    PW_SPELL = 0x00000004L, /*!<サブウィンドウ描画フラグ: 魔法一覧 / Display spell list */
    PW_PLAYER = 0x00000008L, /*!<サブウィンドウ描画フラグ: プレイヤーのステータス / Display character */
    PW_MONSTER_LIST = 0x00000010L, /*!<サブウィンドウ描画フラグ: 視界内モンスターの一覧 / Display monster list */
    PW_MESSAGE = 0x00000040L, /*!<サブウィンドウ描画フラグ: メッセージログ / Display messages */
    PW_OVERHEAD = 0x00000080L, /*!<サブウィンドウ描画フラグ: 周辺の光景 / Display overhead view */
    PW_MONSTER = 0x00000100L, /*!<サブウィンドウ描画フラグ: モンスターの思い出 / Display monster recall */
    PW_OBJECT = 0x00000200L, /*!<サブウィンドウ描画フラグ: アイテムの知識 / Display object recall */
    PW_DUNGEON = 0x00000400L, /*!<サブウィンドウ描画フラグ: ダンジョンの地形 / Display dungeon view */
    PW_SNAPSHOT = 0x00000800L, /*!<サブウィンドウ描画フラグ: 記念写真 / Display snap-shot */

    PW_ALL = (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER | PW_MONSTER_LIST | PW_MESSAGE | PW_OVERHEAD | PW_MONSTER | PW_OBJECT | PW_DUNGEON | PW_SNAPSHOT),
} window_redraw_type;

void redraw_window(void);
void window_stuff(player_type *player_ptr);
void redraw_stuff(player_type *creature_ptr);
