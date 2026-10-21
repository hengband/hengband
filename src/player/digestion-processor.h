#pragma once

#include "system/angband.h"

constexpr auto PY_FOOD_MAX = 15000; /*!< 食べ過ぎ～満腹の閾値 / Food value (Bloated) */
constexpr auto PY_FOOD_FULL = 10000; /*!< 満腹～平常の閾値 / Food value (Normal) */
constexpr auto PY_FOOD_ALERT = 2000; /*!< 平常～空腹の閾値 / Food value (Hungry) */
constexpr auto PY_FOOD_WEAK = 1000; /*!< 空腹～衰弱の閾値 / Food value (Weak) */
constexpr auto PY_FOOD_FAINT = 500; /*!< 衰弱～衰弱(赤表示/麻痺)の閾値 / Food value (Fainting) */
constexpr auto PY_FOOD_STARVE = 100; /*!< 衰弱(赤表示/麻痺)～飢餓ダメージの閾値 / Food value (Starving) */

class PlayerType;
void starve_player(PlayerType *player_ptr);
bool set_food(PlayerType *player_ptr, TIME_EFFECT v);
