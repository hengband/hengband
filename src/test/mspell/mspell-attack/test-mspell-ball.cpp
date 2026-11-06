/*!
 * @brief モンスターが放つボールの半径 (get_monster_ball_radius()) のテスト
 */

#include "monster-race/race-ability-flags.h"
#include "monster-race/race-ability-mask.h"
#include "mspell/mspell-attack/mspell-ball.h"
#include "util/enum-range.h"
#include <doctest/doctest.h>
#include <tuple>
#include <vector>

TEST_CASE("get_monster_ball_radius returns the radius of each ball")
{
    // ボールごとの半径 (ふつうのモンスター, 強力なモンスター)
    const std::vector<std::tuple<MonsterAbilityType, POSITION, POSITION>> radii{
        { MonsterAbilityType::BA_ACID, 2, 4 },
        { MonsterAbilityType::BA_ELEC, 2, 4 },
        { MonsterAbilityType::BA_FIRE, 2, 4 },
        { MonsterAbilityType::BA_COLD, 2, 4 },
        { MonsterAbilityType::BA_POIS, 2, 2 },
        { MonsterAbilityType::BA_NETH, 2, 2 },
        { MonsterAbilityType::BA_NUKE, 2, 2 },
        { MonsterAbilityType::BA_CHAO, 4, 4 },
        { MonsterAbilityType::BA_WATE, 4, 4 },
        { MonsterAbilityType::BA_MANA, 4, 4 },
        { MonsterAbilityType::BA_LITE, 4, 4 },
        { MonsterAbilityType::BA_DARK, 4, 4 },
        { MonsterAbilityType::BA_VOID, 4, 4 },
        { MonsterAbilityType::BA_ABYSS, 4, 4 },
        { MonsterAbilityType::BA_METEOR, 4, 4 },
    };
    for (const auto &[ability, radius, powerful_radius] : radii) {
        CAPTURE(ability);
        // 失敗したときに値が表示されるよう、無効値 (-1) で埋めてから比べる
        CHECK(get_monster_ball_radius(ability, false).value_or(-1) == radius);
        CHECK(get_monster_ball_radius(ability, true).value_or(-1) == powerful_radius);
    }
}

TEST_CASE("get_monster_ball_radius covers every ball in RF_ABILITY_BALL_MASK")
{
    // ボールを足したのに半径を決め忘れると、魔法が放たれず、ペットが巻き込みを控える判定からも漏れる
    for (const auto ability : EnumRange(MonsterAbilityType::SHRIEK, MonsterAbilityType::MAX)) {
        CAPTURE(ability);
        // ボールのマスクはブレスとロケットも含む
        const auto is_ball = RF_ABILITY_BALL_MASK.has(ability) && RF_ABILITY_BREATH_MASK.has_not(ability) && (ability != MonsterAbilityType::ROCKET);
        CHECK(get_monster_ball_radius(ability, false).has_value() == is_ball);
        CHECK(get_monster_ball_radius(ability, true).has_value() == is_ball);
    }
}
