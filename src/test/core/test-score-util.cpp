/*!
 * @brief 前の版のスコアファイル (scores.raw) の 1 件を読むテスト
 */

#include "core/score-util.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <string>
#include <string_view>

namespace {
template <size_t N>
void set_field(char (&field)[N], std::string_view value)
{
    std::copy_n(value.begin(), std::min(value.length(), N), field);
}
}

TEST_CASE("high_score::to_score reads right-justified numeric fields")
{
    high_score raw{};
    set_field(raw.what, "3.1.0");
    set_field(raw.pts, "  2512132");
    set_field(raw.gold, " 11823427");
    set_field(raw.turns, "  6496289");
    set_field(raw.day, "@20261117");
    set_field(raw.who, "Henkma");
    set_field(raw.uid, "    508");
    set_field(raw.sex, "f");
    set_field(raw.p_r, " 8");
    set_field(raw.p_c, "12");
    set_field(raw.p_a, " 3");
    set_field(raw.cur_lev, " 50");
    set_field(raw.cur_dun, "  0");
    set_field(raw.max_lev, " 50");
    set_field(raw.max_dun, "100");
    set_field(raw.how, "ripe");

    const auto score = raw.to_score();
    CHECK(score.version == "3.1.0");
    CHECK(score.points == 2512132);
    CHECK(score.gold == 11823427);
    CHECK(score.turns == 6496289);
    CHECK(score.date == "@20261117");
    CHECK(score.name == "Henkma");
    CHECK(score.uid == 508);
    CHECK(score.sex == "f");
    CHECK(score.race == 8);
    CHECK(score.pclass == 12);
    CHECK(score.personality == 3);
    CHECK(score.level == 50);
    CHECK(score.dungeon_level == 0);
    CHECK(score.max_level == 50);
    CHECK(score.max_dungeon_level == 100);
    CHECK(score.died_from == "ripe");
}

TEST_CASE("high_score::to_score reads fields that fill the whole array without NUL")
{
    high_score raw{};
    const std::string name(sizeof(raw.who), 'n');
    set_field(raw.who, name);
    CHECK(raw.to_score().name == name);
}
