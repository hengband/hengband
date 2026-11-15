/*!
 * @brief プレイヤーの魔法領域 (PlayerRealm) のテスト
 *
 * player/player-realm.cpp の、第一領域・第二領域の位置の判定と、位置ごとの魔法の表の引き分けを確かめる。
 */

#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/player-type-definition.h"
#include "test/scoped-restore.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>

TEST_CASE("PlayerRealm tells the position of each chosen realm")
{
    PlayerType player;
    PlayerRealm pr(&player);
    pr.set(RealmType::LIFE, RealmType::DEATH);
    CHECK(pr.realm1().position() == RealmPosition::FIRST);
    CHECK(pr.realm2().position() == RealmPosition::SECOND);
    CHECK(pr.position_of(RealmType::LIFE) == RealmPosition::FIRST);
    CHECK(pr.position_of(RealmType::DEATH) == RealmPosition::SECOND);
    // 選んでいない領域 (全領域職の詠唱) と技能領域は第一領域の表を使う
    CHECK(pr.position_of(RealmType::SORCERY) == RealmPosition::FIRST);
    CHECK(pr.position_of(RealmType::HISSATSU) == RealmPosition::FIRST);

    // 第二領域が無ければ、どの領域も第一領域の表
    pr.set(RealmType::LIFE);
    CHECK_FALSE(pr.realm2().is_available());
    CHECK(pr.position_of(RealmType::DEATH) == RealmPosition::FIRST);
    CHECK(pr.position_of(RealmType::NONE) == RealmPosition::FIRST);
}

TEST_CASE("PlayerRealm reads the spell table of the realm position")
{
    const auto restore = test::scoped_restore(class_magics_info, mp_ptr);
    class_magics_info.assign(1, {});
    auto &magic = class_magics_info[0];
    const auto realm = enum2i(RealmType::DEATH);
    magic.info[realm][3].slevel = 10;
    magic.info_second[realm][3].slevel = 20;
    mp_ptr = &magic;

    PlayerType player;
    player.pclass = PlayerClassType::WARRIOR;
    PlayerRealm pr(&player);

    // 第一領域として選んだときは第一領域用の表
    pr.set(RealmType::DEATH, RealmType::LIFE);
    CHECK(pr.realm1().get_spell_info(3).slevel == 10);
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3, pr.position_of(RealmType::DEATH)).slevel == 10);

    // 第二領域として選んだときは第二領域用の表
    pr.set(RealmType::LIFE, RealmType::DEATH);
    CHECK(pr.realm2().get_spell_info(3).slevel == 20);
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3, pr.position_of(RealmType::DEATH)).slevel == 20);

    // 位置を渡さなければ第一領域用の表 (職業を指定しても同じ)
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3).slevel == 10);
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3, RealmPosition::SECOND, PlayerClassType::WARRIOR).slevel == 20);
}
