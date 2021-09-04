#pragma once

#include "system/angband.h"

#define PY_FOOD_MAX 15000 /*!< 食べ過ぎ～満腹の閾値 / Food value (Bloated) */
#define PY_FOOD_FULL 10000 /*!< 満腹～平常の閾値 / Food value (Normal) */
#define PY_FOOD_ALERT 2000 /*!< 平常～空腹の閾値 / Food value (Hungry) */
#define PY_FOOD_WEAK 1000 /*!< 空腹～衰弱の閾値 / Food value (Weak) */
#define PY_FOOD_FAINT 500 /*!< 衰弱～衰弱(赤表示/麻痺)の閾値 / Food value (Fainting) */
#define PY_FOOD_STARVE 100 /*!< 衰弱(赤表示/麻痺)～飢餓ダメージの閾値 / Food value (Starving) */

struct player_type;
void starve_player(player_type *creature_ptr);
bool set_food(player_type *creature_ptr, TIME_EFFECT v);
