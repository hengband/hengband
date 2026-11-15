#pragma once

#include "system/h-type.h"

/*!
 * @brief 魔法1つの習得と詠唱に関するパラメータ
 */
struct magic_type {
    PLAYER_LEVEL slevel; /* Required level (to learn) */
    MANA_POINT smana; /* Required mana (to cast) */
    PERCENTAGE sfail; /* Minimum chance of failure */
    EXP sexp; /* Encoded experience bonus */
};
