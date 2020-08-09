#pragma once

#include "system/angband.h"

#define MAX_RACIAL_POWERS 36

/* Racial Power Info / レイシャル・パワー情報の構造体定義 */
typedef struct rpi_type {
    GAME_TEXT racial_name[MAX_NLEN];
    PLAYER_LEVEL min_level; //!<体得レベル
    int cost;
    int stat;
    PERCENTAGE fail;
    int number;
    int racial_cost;
} rpi_type;

// Racial Command.
typedef struct rc_type {
    rpi_type power_desc[MAX_RACIAL_POWERS];
    int num;
    COMMAND_CODE command_code;
    int ask;
    PLAYER_LEVEL lvl;
    bool flag;
    bool redraw;
    bool cast;
    bool is_warrior;
    char choice;
    char out_val[160];
    int menu_line;
} rc_type;

rc_type *initialize_rc_type(player_type *creature_ptr, rc_type *rc_ptr);
