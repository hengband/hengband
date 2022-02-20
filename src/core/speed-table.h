#pragma once

#include "system/angband.h"

/*! @brief 消費する行動エネルギー値を正規乱数で返す(中央100/分散25) / Random energy */
#define ENERGY_NEED() (randnor(100, 25))

byte speed_to_energy(byte speed);
