/*!
 * @brief プレイヤーの魔法領域 (PlayerRealm) のテスト
 *
 * player/player-realm.cpp の、第一領域・第二領域の位置の判定と、位置ごとの魔法の表の引き分けを確かめる。
 * 技能領域の表に基準値を写す player_magic::set_technic_base_values() もここで確かめる。
 */

#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/player-type-definition.h"
#include "system/spell-info-list.h"
#include "system/system-variables.h"
#include "test/scoped-restore.h"
#include "test/system/spell-info-list-test-access.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <utility>

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
    magic.info[realm][3].set_learn_level(10);
    magic.info_second[realm][3].set_learn_level(20);
    mp_ptr = &magic;

    PlayerType player;
    player.pclass = PlayerClassType::WARRIOR;
    PlayerRealm pr(&player);

    // 第一領域として選んだときは第一領域用の表
    pr.set(RealmType::DEATH, RealmType::LIFE);
    CHECK(pr.realm1().get_spell_info(3).get_learn_level() == 10);
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3, pr.position_of(RealmType::DEATH)).get_learn_level() == 10);

    // 第二領域として選んだときは第二領域用の表
    pr.set(RealmType::LIFE, RealmType::DEATH);
    CHECK(pr.realm2().get_spell_info(3).get_learn_level() == 20);
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3, pr.position_of(RealmType::DEATH)).get_learn_level() == 20);

    // 位置を渡さなければ第一領域用の表 (職業を指定しても同じ)
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3).get_learn_level() == 10);
    CHECK(PlayerRealm::get_spell_info(RealmType::DEATH, 3, RealmPosition::SECOND, PlayerClassType::WARRIOR).get_learn_level() == 20);
}

TEST_CASE("PlayerRealm reads technic spells from the base values copied to the class table")
{
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    SpellInfo song;
    song.idx = 2;
    song.tag = "song";
    song.base.set_learn_level(7);
    song.base.smana = 9;
    song.base.sfail = 35;
    song.base.sexp = 50;
    spells.set_spell_info(RealmType::MUSIC, 2, std::move(song));

    const auto restore = test::scoped_restore(class_magics_info, mp_ptr);
    class_magics_info.assign(1, {});
    auto &magic = class_magics_info[0];
    magic.set_technic_base_values();
    mp_ptr = &magic;

    const auto check_song = [](const magic_type &spell) {
        CHECK(spell.get_learn_level() == 7);
        CHECK(spell.smana == 9);
        CHECK(spell.sfail == 35);
        CHECK(spell.sexp == 50);
    };
    check_song(magic.info[enum2i(RealmType::MUSIC)][2]);
    check_song(magic.info_second[enum2i(RealmType::MUSIC)][2]);
    CHECK_FALSE(magic.has_distinct_second(RealmType::MUSIC));

    // 技能領域は職業によらない表なので、位置を渡しても同じ値
    check_song(PlayerRealm::get_spell_info(RealmType::MUSIC, 2));
    check_song(PlayerRealm::get_spell_info(RealmType::MUSIC, 2, RealmPosition::SECOND));
    check_song(PlayerRealm::get_spell_info(RealmType::MUSIC, 2, RealmPosition::FIRST, PlayerClassType::WARRIOR));

    // 魔法の定義に無い枠は、習得も使用もできない値になる
    for (const auto *table : { magic.info, magic.info_second }) {
        const auto &missing = table[enum2i(RealmType::MUSIC)][3];
        CHECK_FALSE(missing.is_learnable());
        CHECK_FALSE(missing.can_learn_at(PY_MAX_LEVEL));
        CHECK(missing.smana == 0);
    }
}
