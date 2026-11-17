/*!
 * @brief スコアの記録と scores.raw の 1 件の変換のテスト
 */

#include "core/score-util.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <string>
#include <string_view>

namespace {
HighScore make_score()
{
    HighScore score;
    score.version = "3.1.0";
    score.points = 2512132;
    score.gold = 11823427;
    score.turns = 6496289;
    score.date = "@20261117";
    score.name = "Henkma";
    score.uid = 508;
    score.sex = "f";
    score.race = 8;
    score.pclass = 12;
    score.personality = 3;
    score.level = 50;
    score.dungeon_level = 0;
    score.max_level = 50;
    score.max_dungeon_level = 100;
    score.died_from = "ripe";
    return score;
}

std::string_view field(const char *chars)
{
    return chars;
}
}

TEST_CASE("high_score::from_score right-justifies numeric fields")
{
    const auto raw = high_score::from_score(make_score());
    CHECK(field(raw.what) == "3.1.0");
    CHECK(field(raw.pts) == "  2512132");
    CHECK(field(raw.gold) == " 11823427");
    CHECK(field(raw.turns) == "  6496289");
    CHECK(field(raw.day) == "@20261117");
    CHECK(field(raw.who) == "Henkma");
    CHECK(field(raw.uid) == "    508");
    CHECK(field(raw.sex) == "f");
    CHECK(field(raw.p_r) == " 8");
    CHECK(field(raw.p_c) == "12");
    CHECK(field(raw.p_a) == " 3");
    CHECK(field(raw.cur_lev) == " 50");
    CHECK(field(raw.cur_dun) == "  0");
    CHECK(field(raw.max_lev) == " 50");
    CHECK(field(raw.max_dun) == "100");
    CHECK(field(raw.how) == "ripe");
}

TEST_CASE("high_score::to_score reads back what from_score wrote")
{
    const auto score = make_score();
    const auto read = high_score::from_score(score).to_score();
    CHECK(read.version == score.version);
    CHECK(read.points == score.points);
    CHECK(read.gold == score.gold);
    CHECK(read.turns == score.turns);
    CHECK(read.date == score.date);
    CHECK(read.name == score.name);
    CHECK(read.uid == score.uid);
    CHECK(read.sex == score.sex);
    CHECK(read.race == score.race);
    CHECK(read.pclass == score.pclass);
    CHECK(read.personality == score.personality);
    CHECK(read.level == score.level);
    CHECK(read.dungeon_level == score.dungeon_level);
    CHECK(read.max_level == score.max_level);
    CHECK(read.max_dungeon_level == score.max_dungeon_level);
    CHECK(read.died_from == score.died_from);
}

TEST_CASE("high_score::from_score truncates the name and the cause of death to the field size")
{
    auto score = make_score();
    score.name = "ABCDEFGHIJKLMNOPQRST";
    score.died_from = std::string(50, 'x');
    const auto raw = high_score::from_score(score);
    CHECK(field(raw.who) == "ABCDEFGHIJKLMNO");
#ifdef JP
    CHECK(field(raw.how) == std::string(37, 'x').append("…"));
#else
    CHECK(field(raw.how) == std::string(36, 'x').append("..."));
#endif
}

TEST_CASE("high_score::to_score reads fields that fill the whole array without NUL")
{
    high_score raw{};
    const std::string name(sizeof(raw.who), 'n');
    std::copy(name.begin(), name.end(), raw.who);
    CHECK(raw.to_score().name == name);
}
