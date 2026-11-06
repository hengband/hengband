#pragma once

#include "info-reader/general-parser.h"
#include <doctest/doctest.h>

namespace test {

/*!
 * @brief 固定マップの1マスの指定 (dungeon_grid) の全項目が一致することを確かめる
 * @param actual 確かめる指定
 * @param expected 期待する指定
 */
inline void check_same_grid(const dungeon_grid &actual, const dungeon_grid &expected)
{
    CHECK(actual.feature == expected.feature);
    CHECK(actual.monster == expected.monster);
    CHECK(actual.object == expected.object);
    CHECK(actual.ego == expected.ego);
    CHECK(actual.artifact == expected.artifact);
    CHECK(actual.trap == expected.trap);
    CHECK(actual.cave_info == expected.cave_info);
    CHECK(actual.special == expected.special);
    CHECK(actual.random == expected.random);
}

}
