#pragma once

#include "system/angband.h"

#define NUM_SPEED 200

/*! @brief 消費する行動エネルギー値を正規乱数で返す(中央100/分散25) / Random energy */
#define ENERGY_NEED() (randnor(100, 25))

extern const byte extract_energy[NUM_SPEED];

byte speed_to_energy(byte speed);
