/*!
 * @brief 耐性によるダメージ倍率の計算のテスト
 *
 * player/player-status-resist.h のダメージ倍率が、耐性・一時耐性・免疫に応じて減り、弱点・突然変異に応じて増えることを確かめる。
 */

#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player/player-status-resist.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-vector-wrapper.h"
#include <array>
#include <doctest/doctest.h>

namespace {
/*!
 * @brief ベースアイテム (ID 1) の兜に耐性・免疫・弱点などの印を足して身に着ける (呼ぶたびに印が重なる)
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
        // 6 / (d4 + 7) 倍。最小は 6/11、最大は 6/8
        CHECK(calc_nexus_damage_rate(&player, CALC_MIN) == 54);
        CHECK(calc_nexus_damage_rate(&player, CALC_MAX) == 75);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MAX) == 100);
    }

    SUBCASE("resistance to disenchantment reduces only disenchantment damage")
    {
        wear_helm_with(player, TR_RES_DISEN);
        CHECK(calc_nexus_damage_rate(&player, CALC_MAX) == 100);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MIN) == 54);
        CHECK(calc_disenchant_damage_rate(&player, CALC_MAX) == 75);
    }
}

TEST_CASE("the damage rates of the basic elements are zero with immunity even if vulnerable")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    BaseitemList::get_instance().resize(2);
    PlayerType player;

    SUBCASE("acid")
    {
        wear_helm_with(player, TR_VUL_ACID);
        CHECK(calc_acid_damage_rate(&player) == 133);
        wear_helm_with(player, TR_IM_ACID);
        CHECK(calc_acid_damage_rate(&player) == 0);
    }

    SUBCASE("elec")
    {
        wear_helm_with(player, TR_VUL_ELEC);
        CHECK(calc_elec_damage_rate(&player) == 133);
        wear_helm_with(player, TR_IM_ELEC);
        CHECK(calc_elec_damage_rate(&player) == 0);
    }

    SUBCASE("fire")
    {
        wear_helm_with(player, TR_VUL_FIRE);
        CHECK(calc_fire_damage_rate(&player) == 133);
        wear_helm_with(player, TR_IM_FIRE);
        CHECK(calc_fire_damage_rate(&player) == 0);
    }

    SUBCASE("cold")
    {
        wear_helm_with(player, TR_VUL_COLD);
        CHECK(calc_cold_damage_rate(&player) == 133);
        wear_helm_with(player, TR_IM_COLD);
        CHECK(calc_cold_damage_rate(&player) == 0);
    }
}

namespace {
using DamageRateCalculator = PERCENTAGE (*)(PlayerType *);

/*!
 * @brief 基本 4 属性ごとの耐性・弱点の印・一時耐性とダメージ倍率の計算
 */
struct BasicElement {
    const char *name;
    tr_type resist;
    tr_type vuln;
    TIME_EFFECT PlayerType::*oppose;
    DamageRateCalculator calc_damage_rate;
};

const std::array<BasicElement, 4> BASIC_ELEMENTS{ {
    { "acid", TR_RES_ACID, TR_VUL_ACID, &PlayerType::oppose_acid, calc_acid_damage_rate },
    { "elec", TR_RES_ELEC, TR_VUL_ELEC, &PlayerType::oppose_elec, calc_elec_damage_rate },
    { "fire", TR_RES_FIRE, TR_VUL_FIRE, &PlayerType::oppose_fire, calc_fire_damage_rate },
    { "cold", TR_RES_COLD, TR_VUL_COLD, &PlayerType::oppose_cold, calc_cold_damage_rate },
} };
}

TEST_CASE("the damage rates of the basic elements combine resistance and vulnerability")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    BaseitemList::get_instance().resize(2);
    for (const auto &element : BASIC_ELEMENTS) {
        CAPTURE(element.name);
        PlayerType player;
        CHECK(element.calc_damage_rate(&player) == 100);

        // 耐性で 1/3 (切り上げ)、弱点で 4/3 倍してから耐性で 1/3
        wear_helm_with(player, element.resist);
        CHECK(element.calc_damage_rate(&player) == 34);
        wear_helm_with(player, element.vuln);
        CHECK(element.calc_damage_rate(&player) == 45);
    }
}

TEST_CASE("the damage rates of the basic elements double with the mutation")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    BaseitemList::get_instance().resize(2);
    for (const auto &element : BASIC_ELEMENTS) {
        CAPTURE(element.name);
        PlayerType player;
        player.muta.set(PlayerMutationType::VULN_ELEM);
        CHECK(element.calc_damage_rate(&player) == 200);

        // 兜の弱点で 4/3 倍、突然変異で 2 倍と、要因ごとに重なる
        wear_helm_with(player, element.vuln);
        CHECK(element.calc_damage_rate(&player) == 266);
    }
}

TEST_CASE("temporary resistance reduces only the damage rate of its own basic element")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    BaseitemList::get_instance().resize(2);
    for (const auto &element : BASIC_ELEMENTS) {
        CAPTURE(element.name);
        PlayerType player;
        player.*element.oppose = 10;
        for (const auto &other : BASIC_ELEMENTS) {
            CAPTURE(other.name);
            CHECK(other.calc_damage_rate(&player) == (&other == &element ? 34 : 100));
        }

        // 一時耐性と耐性は重なる
        wear_helm_with(player, element.resist);
        CHECK(element.calc_damage_rate(&player) == 12);
    }
}
