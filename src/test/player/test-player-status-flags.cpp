/*!
 * @brief プレイヤーの能力のフラグの判定のテスト
 *
 * player/player-status-flags.h の発光 (has_lite()) が、忍者ではマンチキンの性格と吸血鬼 (種族・変身) の分だけを除くことを確かめる。
 */

#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player-info/class-types.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>

TEST_CASE("has_lite excludes only the munchkin and vampire light for ninjas")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    BaseitemList::get_instance().resize(2);
    PlayerType player;
    player.lev = 1;

    SUBCASE("munchkin")
    {
        player.ppersonality = PERSONALITY_MUNCHKIN;
        player.pclass = PlayerClassType::WARRIOR;
        CHECK(has_lite(&player) == FLAG_CAUSE_PERSONALITY);
        player.pclass = PlayerClassType::NINJA;
        CHECK(has_lite(&player) == 0);
        // 除くのはこの理由の発光だけで、ほかの理由の発光は残る
        player.ult_res = 1;
        CHECK(has_lite(&player) == FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    SUBCASE("vampire race")
    {
        player.prace = PlayerRaceType::VAMPIRE;
        player.pclass = PlayerClassType::WARRIOR;
        CHECK(has_lite(&player) == FLAG_CAUSE_RACE);
        player.pclass = PlayerClassType::NINJA;
        CHECK(has_lite(&player) == 0);
        // 除くのはこの理由の発光だけで、ほかの理由の発光は残る
        player.ult_res = 1;
        CHECK(has_lite(&player) == FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    SUBCASE("vampire form")
    {
        player.mimic_form = MimicKindType::VAMPIRE;
        player.pclass = PlayerClassType::WARRIOR;
        CHECK(has_lite(&player) == FLAG_CAUSE_RACE);
        player.pclass = PlayerClassType::NINJA;
        CHECK(has_lite(&player) == 0);
        // 除くのはこの理由の発光だけで、ほかの理由の発光は残る
        player.ult_res = 1;
        CHECK(has_lite(&player) == FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    SUBCASE("fire body mutation")
    {
        player.pclass = PlayerClassType::NINJA;
        player.muta.set(PlayerMutationType::FIRE_BODY);
        CHECK(has_lite(&player) == FLAG_CAUSE_MUTATION);
    }

    SUBCASE("temporary fire aura")
    {
        player.pclass = PlayerClassType::NINJA;
        player.tim_sh_fire = 1;
        CHECK(has_lite(&player) == FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    SUBCASE("fire aura of equipment")
    {
        player.pclass = PlayerClassType::NINJA;
        auto &helm = *player.inventory[INVEN_HEAD];
        helm.bi_id = 1;
        helm.art_flags.set(TR_SH_FIRE);
        CHECK(has_lite(&player) == FLAG_CAUSE_INVEN_HEAD);
    }
}
