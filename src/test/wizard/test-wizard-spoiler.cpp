/*!
 * @brief スポイラーの出力のテスト
 *
 * 魔法のスポイラーで、職業と領域ごとにどの表 (第一領域用・第二領域用) を出すかの判定
 * (spoiler_realm_positions()) を確かめる。
 */

#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "realm/realm-types.h"
#include "util/enum-converter.h"
#include "wizard/wizard-spoiler.h"
#include <doctest/doctest.h>
#include <vector>

namespace {
using Positions = std::vector<RealmPosition>;

/*!
 * @brief 領域の第二領域用の表だけ、第一領域用と違う値にした魔法の定義を作る
 */
player_magic make_distinct_second(RealmType realm)
{
    player_magic magic{};
    magic.info_second[enum2i(realm)][0].smana = 5;
    return magic;
}
}

TEST_CASE("spoiler_realm_positions adds the second realm table only when its values differ")
{
    // プリーストの生命は第一領域にも第二領域にも選べる
    CHECK(spoiler_realm_positions(PlayerClassType::PRIEST, make_distinct_second(RealmType::LIFE), RealmType::LIFE) == Positions{ RealmPosition::FIRST, RealmPosition::SECOND });
    CHECK(spoiler_realm_positions(PlayerClassType::PRIEST, player_magic{}, RealmType::LIFE) == Positions{ RealmPosition::FIRST });

    // 第二領域にしか選べない領域は、値が違っても表は 1 つ
    CHECK(spoiler_realm_positions(PlayerClassType::PRIEST, make_distinct_second(RealmType::SORCERY), RealmType::SORCERY) == Positions{ RealmPosition::FIRST });
}

TEST_CASE("spoiler_realm_positions lists only the realms the class can use")
{
    // 選べない領域は出さない
    CHECK(spoiler_realm_positions(PlayerClassType::WARRIOR, player_magic{}, RealmType::LIFE).empty());
    CHECK(spoiler_realm_positions(PlayerClassType::PRIEST, player_magic{}, RealmType::MUSIC).empty());

    // 全領域職は領域を選ばないので、魔法領域の第一領域用の表だけを出す
    CHECK(spoiler_realm_positions(PlayerClassType::SORCERER, make_distinct_second(RealmType::LIFE), RealmType::LIFE) == Positions{ RealmPosition::FIRST });
    CHECK(spoiler_realm_positions(PlayerClassType::SORCERER, player_magic{}, RealmType::MUSIC).empty());

    // 技能領域は選べる職業にだけ出す
    CHECK(spoiler_realm_positions(PlayerClassType::BARD, player_magic{}, RealmType::MUSIC) == Positions{ RealmPosition::FIRST });
}
