/*!
 * @file main-window-row-column.h
 * @brief メイン画面に表示する各項目の位置 (行と桁)
 * @details 負の値は画面の下端 (行) や右端 (桁) から数える
 */

#pragma once

constexpr auto ROW_RACE = 1;
constexpr auto COL_RACE = 0; /* <race name> */

constexpr auto ROW_TITLE = 2;
constexpr auto COL_TITLE = 0; /* <title> or <mode> */

constexpr auto ROW_LEVEL = 3;
constexpr auto COL_LEVEL = 0; /* "LEVEL xxxxxx" */

constexpr auto ROW_EXP = 4;
constexpr auto COL_EXP = 0; /* "EXP xxxxxxxx" */

constexpr auto ROW_GOLD = 5;
constexpr auto COL_GOLD = 0; /* "AU xxxxxxxxx" */

constexpr auto ROW_EQUIPPY = 6;
constexpr auto COL_EQUIPPY = 0; /* equippy chars */

constexpr auto ROW_STAT = 7;
constexpr auto COL_STAT = 0; /* "xxx   xxxxxx" */

constexpr auto ROW_AC = 13;
constexpr auto COL_AC = 0; /* "Cur AC xxxxx" */

constexpr auto ROW_CURHP = 14;
constexpr auto COL_CURHP = 0; /* "Cur HP xxxxx" */

constexpr auto ROW_CURSP = 15;
constexpr auto COL_CURSP = 0; /* "Cur SP xxxxx" */

constexpr auto ROW_RIDING_INFO = 19;
constexpr auto COL_RIDING_INFO = 0; /* "xxxxxxxxxxxx" */

constexpr auto ROW_INFO = 20;
constexpr auto COL_INFO = 0; /* "xxxxxxxxxxxx" */

constexpr auto ROW_CUT = 16;
constexpr auto COL_CUT = 0; /* <cut> */

constexpr auto ROW_STUN = 17;
constexpr auto COL_STUN = 0; /* <stun> */

constexpr auto ROW_HUNGRY = 18;
constexpr auto COL_HUNGRY = 0; /* "Weak" / "Hungry" / "Full" / "Gorged" */

constexpr auto ROW_STATE = 18;
constexpr auto COL_STATE = 7; /* <state> */

constexpr auto ROW_DAY = -3;
constexpr auto COL_DAY = 0; /* day */

constexpr auto ROW_DUNGEON = -2;
constexpr auto COL_DUNGEON = 0; /* dungeon */

constexpr auto ROW_SPEED = -1;
constexpr auto COL_SPEED = -24; /* "Slow (-NN)" or "Fast (+NN)" */

constexpr auto ROW_STUDY = -1;
constexpr auto COL_STUDY = -13; /* "Study" */

constexpr auto ROW_DEPTH = -1;
constexpr auto COL_DEPTH = -8; /* "Lev NNN" / "NNNN ft" */

constexpr auto ROW_STATBAR = -1;
constexpr auto COL_STATBAR = 0;
constexpr auto MAX_COL_STATBAR = -26;
