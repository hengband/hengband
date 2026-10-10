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
