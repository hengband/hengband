#pragma once

/*! @brief 消費する行動エネルギー値を正規乱数で返す(中央100/分散25) / Random energy */
#define ENERGY_NEED() (randnor(100, 25))

/*! @brief 加速値に応じた基本行動エネルギー消費量を返す / Extract energy from speed (Assumes that SPEED is unsigned) */
#define SPEED_TO_ENERGY(SPEED) (((SPEED) > 199) ? 49 : extract_energy[(SPEED)])

extern const byte extract_energy[200];
