/*!
 * @brief プリーストの処理 (mind/mind-priest.cpp) のテスト
 *
 * プリーストにふさわしい祝福が、第一領域の善悪で決まることを確かめる。
 */

#include "mind/mind-priest.h"
#include "object-enchant/tr-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/player-type-definition.h"
#include <doctest/doctest.h>

TEST_CASE("get_priest_blessing follows the alignment of the first realm")
{
    PlayerType player;
    PlayerRealm pr(&player);

    // 善の領域 (生命・破邪) は善の神の祝福
    pr.set(RealmType::LIFE);
    CHECK(get_priest_blessing(&player) == TR_BLESSED);
    pr.set(RealmType::CRUSADE);
    CHECK(get_priest_blessing(&player) == TR_BLESSED);

    // 悪の領域 (暗黒・悪魔) は邪の神の祝福
    pr.set(RealmType::DEATH);
    CHECK(get_priest_blessing(&player) == TR_BLESSED_EVIL);
    pr.set(RealmType::DAEMON);
    CHECK(get_priest_blessing(&player) == TR_BLESSED_EVIL);
}

TEST_CASE("get_priest_blessing ignores the second realm")
{
    PlayerType player;
    PlayerRealm pr(&player);

    pr.set(RealmType::LIFE, RealmType::SORCERY);
    CHECK(get_priest_blessing(&player) == TR_BLESSED);
    pr.set(RealmType::DEATH, RealmType::SORCERY);
    CHECK(get_priest_blessing(&player) == TR_BLESSED_EVIL);
}
