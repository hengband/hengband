/*!
 * @brief プレイヤーの職業 (PlayerClass) のテスト
 */

#include "player-base/player-class.h"
#include "player-info/class-types.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include <doctest/doctest.h>

TEST_CASE("PlayerClass tells the classes that use every magic realm")
{
    // 職業を引数にとる判定と、プレイヤーの職業を見る判定は同じ結果になる
    PlayerType player;
    for (const auto pclass : EnumRange(PlayerClassType::WARRIOR, PlayerClassType::MAX)) {
        CAPTURE(enum2i(pclass));
        const auto expected = (pclass == PlayerClassType::SORCERER) || (pclass == PlayerClassType::RED_MAGE);
        CHECK(PlayerClass::is_every_magic(pclass) == expected);
        player.pclass = pclass;
        CHECK(PlayerClass(&player).is_every_magic() == expected);
    }
}
