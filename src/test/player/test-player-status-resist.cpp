/*!
 * @brief 耐性によるダメージ倍率の計算のテスト
 *
 * player/player-status-resist.h の因果混乱・劣化のダメージ倍率が、それぞれの属性の耐性だけを見て減ることを確かめる。
 */

#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "player/player-status-resist.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>

namespace {
/*!
 * @brief ベースアイテム (ID 1) の兜に耐性を付けて身に着ける
 */
void wear_helm_with(PlayerType &player, tr_type flag)
{
    auto &helm = *player.inventory[INVEN_HEAD];
    helm.bi_id = 1;
    helm.art_flags.set(flag);
}
}

TEST_CASE("calc_nexus_damage_rate and calc_disenchant_damage_rate depend only on their own resistance")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    BaseitemList::get_instance().resize(2);
    PlayerType player;

    SUBCASE("without resistance")
    {
        CHECK(calc_nexus_damage_rate(&player, CALC_MAX) == 100);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MAX) == 100);
    }

    SUBCASE("resistance to nexus reduces only nexus damage")
    {
        wear_helm_with(player, TR_RES_NEXUS);
        // 6 / (d4 + 7) 倍。CALC_MIN・CALC_MAX は除数 (d4 + 7) の最小・最大なので、倍率の大小は逆になる
        CHECK(calc_nexus_damage_rate(&player, CALC_MIN) == 75);
        CHECK(calc_nexus_damage_rate(&player, CALC_MAX) == 54);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MAX) == 100);
    }

    SUBCASE("resistance to disenchantment reduces only disenchantment damage")
    {
        wear_helm_with(player, TR_RES_DISEN);
        CHECK(calc_nexus_damage_rate(&player, CALC_MAX) == 100);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MIN) == 75);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MAX) == 54);
    }
}
