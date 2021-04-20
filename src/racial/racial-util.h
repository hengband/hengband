#pragma once

#include "system/angband.h"
#include <string>
#include <array>

#define MAX_RACIAL_POWERS 36

/*!
 * レイシャル/クラスパワー定義構造体
 */
struct rpi_type {
    std::string racial_name{};
    PLAYER_LEVEL min_level{}; //!<体得レベル
    int cost{};
    int stat{};
    PERCENTAGE fail{};
    int number{};
    int racial_cost{};
};

/*!
 * レイシャル/クラスパワー管理構造体
 */
struct rc_type {
    std::array<rpi_type, MAX_RACIAL_POWERS> power_desc{};
    int num{};
    COMMAND_CODE command_code{};
    int ask{};
    PLAYER_LEVEL lvl{};
    bool flag{};
    bool redraw{};
    bool cast{};
    bool is_warrior{};
    char choice{};
    char out_val[160]{};
    int menu_line{};
};

rc_type *initialize_rc_type(player_type *creature_ptr, rc_type *rc_ptr);
