#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/item/item-entity.h"
#include "test/scoped-rng.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>
#include <limits>
#include <utility>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("apply_ego skips probability rolls at zero and certain percent")
{
    for (const auto chance : { 0, 100 }) {
        auto saved_egos = std::move(egos_info);
        const auto restore = util::make_finalizer([&saved_egos] { egos_info.swap(saved_egos); });
        egos_info.clear();
        auto &ego = egos_info.try_emplace(EgoType::A_MORGUL).first->second;
        ego.cost = 12345;
        const auto rng_guard = test::scoped_rng();
        ego_generate_type extra;
        extra.chance = chance;
        extra.tr_flags = { TR_RES_FIRE };
        ego.xtra_flags.push_back(std::move(extra));
        ItemEntity item;
        item.ego_idx = i2enum<EgoType>(4);
        item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
        auto expected_rng = get_game_rng();
        apply_ego(&item, 10);
        CHECK(item.art_flags.has(TR_RES_FIRE) == (chance == 100));
        const auto expected = expected_rng();
        const auto actual = get_game_rng()();
        CHECK(actual == expected);
    }
}

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("apply_ego saturates composed bonuses to save storage")
{
    auto saved_egos = std::move(egos_info);
    const auto restore = util::make_finalizer([&saved_egos] { egos_info.swap(saved_egos); });
    egos_info.clear();
    auto &ego = egos_info.try_emplace(EgoType::A_MORGUL).first->second;
    ego.cost = 12345;
    const auto rng_guard = test::scoped_rng();
    ego.base_to_h = 32767;
    ego.base_to_d = 32767;
    ego.base_to_a = -32768;
    ego.max_pval = -32768;
    ItemEntity item;
    item.ego_idx = i2enum<EgoType>(4);
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    item.to_h = 1;
    item.to_d = std::numeric_limits<int>::max();
    item.to_a = -1;
    item.pval = -32768;
    apply_ego(&item, 10);
    CHECK(item.to_h == 32767);
    CHECK(item.to_d == 32767);
    CHECK(item.to_a == -32768);
    CHECK(item.pval == -32768);
}

/*!
 * @brief ウィザード操作で荒野の大きな基本レベルを渡しても追加攻撃の乱数引数を保つ
 */
TEST_CASE("apply_ego bounds extra attacks at wizard wilderness generation levels")
{
    auto saved_egos = std::move(egos_info);
    const auto restore = util::make_finalizer([&saved_egos] { egos_info.swap(saved_egos); });
    egos_info.clear();
    auto &ego = egos_info.try_emplace(EgoType::ATTACKS).first->second;
    ego.cost = 1;
    ego.max_pval = 32767;
    ego.flags.set(TR_BLOWS);
    const auto rng_guard = test::scoped_rng();
    ItemEntity item;
    item.ego_idx = EgoType::ATTACKS;
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    apply_ego(&item, std::numeric_limits<DEPTH>::max());
    CHECK(item.pval == 3);
}
