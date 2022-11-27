#pragma once

#include "system/angband.h"

#define MAX_NAZGUL_NUM 5
#define SCREEN_BUF_MAX_SIZE (1024 * 1024) /*!< Max size of screen dump buffer */
#define PY_MAX_LEVEL 50 /*!< プレイヤーレベルの最大値 / Maximum level */
#define PY_MAX_EXP 99999999L /*!< プレイヤー経験値の最大値 / Maximum exp */

/*
 * @details v3.0.0 Alpha20現在、使われていない。こんなに大量の所持金を得ることが想定されていないためか
 * 必要に応じて復活させること
 */
// #define PY_MAX_GOLD 999999999L /*!< プレイヤー所持金の最大値 / Maximum gold */

enum init_flags_type {
    INIT_NAME_ONLY = 0x01,
    INIT_SHOW_TEXT = 0x02,
    INIT_ASSIGN = 0x04,
    INIT_CREATE_DUNGEON = 0x08,
    INIT_ONLY_FEATURES = 0x10,
    INIT_ONLY_BUILDINGS = 0x20,
};

extern init_flags_type init_flags;
extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;

extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool (*get_obj_index_hook)(short bi_id);
