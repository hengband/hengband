#pragma once

#include "system/angband.h"
#include <string>
#include <string_view>

#ifdef JP

enum class JVerbConjugationType {
    AND = 0,
    TO = 1,
    OR = 2,
};
std::string conjugate_jverb(std::string_view in, JVerbConjugationType type);

std::string sindarin_to_kana(std::string_view sindarin);
bool is_kinsoku(std::string_view ch);

int lb_to_kg_integer(int x);
int lb_to_kg_fraction(int x);

/*!
 * @brief インチ→cm変換
 */
constexpr int inch_to_cm(int inch)
{
    return inch * 254 / 100;
}

/*!
 * @brief ポンド→kg変換
 *
 * 体重表記用
 * アイテムの重量は0.5kg単位にするためlb_to_kg_integer/fractionを使用する
 */
constexpr int lb_to_kg(int lb)
{
    return lb * 4536 / 10000;
}

#else

constexpr bool is_kinsoku(std::string_view)
{
    return false;
}

#endif
