#pragma once

#include "system/h-type.h"
#include <tl/optional.hpp>

/*!
 * @brief 魔法1つの習得と詠唱に関するパラメータ
 * @details 習得レベルは値なし (習得できない魔法) を取りうる。tl::optional は値と直接比べても
 * コンパイルが通り、値なしを任意の値より小さいとみなすため、習得できない魔法を習得できるものとして
 * 扱う誤りを防ぐよう、習得レベルは private にして判定用の関数を通して扱う。
 */
struct magic_type {
public:
    MANA_POINT smana{}; /* Required mana (to cast) */
    PERCENTAGE sfail{}; /* Minimum chance of failure */
    EXP sexp{}; /* Encoded experience bonus */

    bool is_learnable() const;
    bool can_learn_at(PLAYER_LEVEL level) const;
    PLAYER_LEVEL get_learn_level() const;
    void set_learn_level(PLAYER_LEVEL level);
    bool operator==(const magic_type &other) const = default;

private:
    tl::optional<PLAYER_LEVEL> slevel; /* 習得レベル (習得できない魔法は値なし) */
};
