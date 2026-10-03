#pragma once

#include <string_view>

constexpr auto MAX_UNIQUE_NUM = 1;
constexpr auto MAX_NAZGUL_NUM = 5;
constexpr auto MAX_BUNBUN_NUM = 2;
#define SCREEN_BUF_MAX_SIZE (1024 * 1024) /*!< Max size of screen dump buffer */
#define PY_MAX_LEVEL 50 /*!< プレイヤーレベルの最大値 / Maximum level */
#define PY_MAX_EXP 99999999L /*!< プレイヤー経験値の最大値 / Maximum exp */

/*
 * @details v3.0.0 Alpha20現在、使われていない。こんなに大量の所持金を得ることが想定されていないためか
 * 必要に応じて復活させること
 */
// #define PY_MAX_GOLD 999999999L /*!< プレイヤー所持金の最大値 / Maximum gold */

enum init_flags_type {
    INIT_CREATE_DUNGEON = 0x08,
    INIT_ONLY_FEATURES = 0x10,
    INIT_ONLY_BUILDINGS = 0x20,
};

extern init_flags_type init_flags;
extern std::string_view ANGBAND_SYS;
extern std::string_view ANGBAND_KEYBOARD;
extern std::string_view ANGBAND_GRAF;
