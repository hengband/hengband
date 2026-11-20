#pragma once

#include "system/h-type.h"
#include <tl/optional.hpp>

/*!
 * @brief 魔法1つの習得と詠唱に関するパラメータ
 */
struct magic_type {
    tl::optional<PLAYER_LEVEL> slevel; /* 習得レベル (習得できない魔法は値なし) */
    MANA_POINT smana{}; /* Required mana (to cast) */
    PERCENTAGE sfail{}; /* Minimum chance of failure */
    EXP sexp{}; /* Encoded experience bonus */

    bool is_learnable() const;
    bool can_learn_at(PLAYER_LEVEL level) const;
};
