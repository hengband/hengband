#pragma once

#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-digging-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "test/scoped-vector-wrapper.h"
#include "util/finalizer.h"
#include <functional>
#include <utility>

namespace test {

// 凡例の検証と配置で使う定義を隔離し、名前と種類をキャッシュ参照前に登録する。
class LegendEgoFixture {
public:
    LegendEgoFixture()
    {
        egos_info.clear();
        const auto add_ego = [](EgoType id, INVENTORY_IDX slot, bool good = true) {
            auto &ego = egos_info[id];
            ego.idx = id;
            ego.slot = slot;
            ego.rating = good ? 10 : 0;
            ego.rarity = (id == EgoType::DIGGING || id == EgoType::TWILIGHT) ? RARITY{ 0 } : RARITY{ 1 };
            ego.cost = 100;
            ego.max_to_h = 3;
            ego.max_to_d = 3;
            ego.max_to_a = 3;
        };
        for (const auto id : { EgoType::DF, EgoType::BRAND_FIRE, EgoType::SHARPNESS, EgoType::EARTHQUAKES, EgoType::DIGGING }) {
            add_ego(id, INVEN_MAIN_HAND);
        }
        add_ego(EgoType::MORGUL, INVEN_MAIN_HAND, false);
        for (const auto id : { EgoType::TELEPATHY, EgoType::SEEING, EgoType::INFRAVISION }) {
            add_ego(id, INVEN_HEAD);
        }
        for (const auto id : { EgoType::REFLECTION, EgoType::ENDURE_FIRE, EgoType::ENDURE_COLD }) {
            add_ego(id, INVEN_SUB_HAND);
        }
        for (const auto id : { EgoType::TWILIGHT, EgoType::RESISTANCE, EgoType::ELVENKIND }) {
            add_ego(id, INVEN_BODY);
        }
        for (const auto id : { EgoType::LITE_LONG, EgoType::LITE_SHINE, EgoType::LITE_INFRA }) {
            add_ego(id, INVEN_LITE);
        }
        auto &items = BaseitemList::get_instance();
        items.resize(646);
        const auto add_base = [&items](short id, ItemKindType kind, int sval) {
            auto &base = items.get_baseitem(id);
            base.name = "Test";
            base.bi_key = BaseitemKey(kind, sval);
            base.cost = 100;
            base.damage_dice = Dice(2, 5);
        };
        add_base(42, ItemKindType::SWORD, SV_LONG_SWORD);
        add_base(43, ItemKindType::HAFTED, SV_WAR_HAMMER);
        add_base(84, ItemKindType::DIGGING, SV_SHOVEL);
        add_base(94, ItemKindType::HELM, SV_HARD_LEATHER_CAP);
        add_base(48, ItemKindType::CROWN, SV_IRON_CROWN);
        add_base(101, ItemKindType::SOFT_ARMOR, SV_ROBE);
        add_base(122, ItemKindType::SHIELD, SV_MIRROR_SHIELD);
        add_base(413, ItemKindType::HELM, SV_DRAGON_HELM);
        add_base(414, ItemKindType::SHIELD, SV_DRAGON_SHIELD);
        add_base(53, ItemKindType::LITE, SV_LITE_LANTERN);
        add_base(582, ItemKindType::LITE, SV_LITE_FEANOR);
        add_base(645, ItemKindType::SOFT_ARMOR, SV_TWILIGHT_ROBE);
        add_base(54, ItemKindType::POTION, 1);
        add_base(55, ItemKindType::RING, 1);
        add_base(56, ItemKindType::GOLD, 1);
    }

    LegendEgoFixture(const LegendEgoFixture &) = delete;
    LegendEgoFixture &operator=(const LegendEgoFixture &) = delete;
    LegendEgoFixture(LegendEgoFixture &&) = delete;
    LegendEgoFixture &operator=(LegendEgoFixture &&) = delete;

private:
    decltype(egos_info) saved_egos = std::move(egos_info);
    util::Finalizer<std::function<void()>> restore_egos{ [this] { egos_info.swap(this->saved_egos); } };
    test::ScopedVectorWrapper<BaseitemList> restore_baseitems{ BaseitemList::get_instance() };
};
}
