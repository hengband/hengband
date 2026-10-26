#include "artifact/random-art-effects.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-digging-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-rng.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/dungeon-list-test-access.h"
#include "util/finalizer.h"
#include <array>
#include <cstdint>
#include <doctest/doctest.h>
#include <functional>
#include <set>
#include <utility>

namespace {

struct BaseSpec {
    short id;
    ItemKindType kind;
    int sval;
};

// 先頭の各部位と追加の特殊な品を、キャッシュを参照する前にすべて登録する。
const std::array BASE_SPECS{
    BaseSpec{ 42, ItemKindType::SWORD, SV_LONG_SWORD },
    BaseSpec{ 43, ItemKindType::HAFTED, SV_WAR_HAMMER },
    BaseSpec{ 44, ItemKindType::POLEARM, SV_SPEAR },
    BaseSpec{ 84, ItemKindType::DIGGING, SV_SHOVEL },
    BaseSpec{ 45, ItemKindType::BOW, SV_LONG_BOW },
    BaseSpec{ 46, ItemKindType::ARROW, 1 },
    BaseSpec{ 47, ItemKindType::SHIELD, SV_SMALL_METAL_SHIELD },
    BaseSpec{ 94, ItemKindType::HELM, SV_HARD_LEATHER_CAP },
    BaseSpec{ 48, ItemKindType::CROWN, SV_IRON_CROWN },
    BaseSpec{ 49, ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL },
    BaseSpec{ 101, ItemKindType::SOFT_ARMOR, SV_ROBE },
    BaseSpec{ 50, ItemKindType::BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
    BaseSpec{ 51, ItemKindType::GLOVES, SV_SET_OF_LEATHER_GLOVES },
    BaseSpec{ 52, ItemKindType::CLOAK, SV_ELVEN_CLOAK },
    BaseSpec{ 53, ItemKindType::LITE, SV_LITE_LANTERN },
    BaseSpec{ 122, ItemKindType::SHIELD, SV_MIRROR_SHIELD },
    BaseSpec{ 582, ItemKindType::LITE, SV_LITE_FEANOR },
    BaseSpec{ 645, ItemKindType::SOFT_ARMOR, SV_TWILIGHT_ROBE },
    BaseSpec{ 413, ItemKindType::HELM, SV_DRAGON_HELM },
    BaseSpec{ 414, ItemKindType::SHIELD, SV_DRAGON_SHIELD },
    BaseSpec{ 54, ItemKindType::POTION, 1 },
    BaseSpec{ 55, ItemKindType::RING, 1 },
};

// BASE_SPECS の先頭から、エゴを付けられる種類（フェアノールのランプまで）の数。残りはエゴの置き換え先とエゴを付けない品。
constexpr size_t EGO_APPLICABLE_SPEC_COUNT = 17;

// 可視のエゴに足される弱いテレパシーの候補（add_low_telepathy() の付けるフラグ）。
const TrFlags LOW_TELEPATHY_FLAGS{ TR_TELEPATHY, TR_ESP_EVIL, TR_ESP_NONLIVING, TR_ESP_ANIMAL, TR_ESP_UNDEAD, TR_ESP_DEMON, TR_ESP_ORC, TR_ESP_TROLL, TR_ESP_GIANT, TR_ESP_DRAGON, TR_ESP_HUMAN, TR_ESP_GOOD, TR_ESP_UNIQUE };

class MagicFixture {
public:
    MagicFixture()
    {
        egos_info.clear();
        const auto add = [](EgoType id, INVENTORY_IDX slot, bool good = true, bool natural = true) {
            auto &ego = egos_info.try_emplace(id).first->second;
            ego.idx = id;
            ego.slot = slot;
            ego.rating = good ? 10 : 0;
            ego.rarity = natural ? 1 : 0;
            ego.cost = 100;
            ego.max_to_h = 3;
            ego.max_to_d = 3;
            ego.max_to_a = 3;
        };
        for (const auto id : { EgoType::DF, EgoType::BRAND_FIRE, EgoType::SHARPNESS, EgoType::EARTHQUAKES }) {
            add(id, INVEN_MAIN_HAND);
        }
        add(EgoType::DIGGING, INVEN_MAIN_HAND, true, false);
        add(EgoType::MORGUL, INVEN_MAIN_HAND, false);
        for (const auto id : { EgoType::RESISTANCE, EgoType::ELVENKIND, EgoType::DWARVEN, EgoType::DRUID }) {
            add(id, INVEN_BODY);
        }
        add(EgoType::PERMANENCE, INVEN_BODY, true, false);
        add(EgoType::TWILIGHT, INVEN_BODY, true, false);
        add(EgoType::A_MORGUL, INVEN_BODY, false);
        for (const auto id : { EgoType::ENDURE_FIRE, EgoType::ENDURE_COLD, EgoType::REFLECTION, EgoType::S_DWARVEN }) {
            add(id, INVEN_SUB_HAND);
        }
        add(EgoType::NIGHT_DAY, INVEN_SUB_HAND, false);
        for (const auto id : { EgoType::SEEING, EgoType::INFRAVISION, EgoType::TELEPATHY }) {
            add(id, INVEN_HEAD);
        }
        add(EgoType::FOOL, INVEN_HEAD, false);
        add(EgoType::ACCURACY, INVEN_BOW);
        add(EgoType::VELOCITY, INVEN_BOW);
        add(EgoType::HURT_ANIMAL, INVEN_AMMO);
        add(EgoType::HURT_EVIL, INVEN_AMMO);
        add(EgoType::BACKBITING, INVEN_AMMO, false);
        add(EgoType::FREE_ACTION, INVEN_ARMS);
        add(EgoType::AGILITY, INVEN_ARMS);
        add(EgoType::WEAKNESS, INVEN_ARMS, false);
        add(EgoType::SLOW_DESCENT, INVEN_FEET);
        add(EgoType::QUIET, INVEN_FEET);
        add(EgoType::NOISE, INVEN_FEET, false);
        add(EgoType::PROTECTION, INVEN_OUTER);
        add(EgoType::STEALTH, INVEN_OUTER);
        add(EgoType::IRRITATION, INVEN_OUTER, false);
        add(EgoType::LITE_SHINE, INVEN_LITE);
        add(EgoType::LITE_INFRA, INVEN_LITE);
        add(EgoType::LITE_LONG, INVEN_LITE);
        add(EgoType::LITE_DARKNESS, INVEN_LITE, false);

        auto &items = BaseitemList::get_instance();
        items.resize(646);
        for (const auto &spec : BASE_SPECS) {
            auto &base = items.get_baseitem(spec.id);
            base.name = "Test";
            base.bi_key = BaseitemKey(spec.kind, spec.sval);
            base.cost = 100;
            base.damage_dice = Dice(2, 5);
            base.ac = 3;
        }
        DungeonDefinition dungeon;
        dungeon.obj_good = 100;
        dungeon.obj_great = 100;
        DungeonList::get_instance().emplace(DungeonId::WILDERNESS, std::move(dungeon));
        floor.set_dungeon_index(DungeonId::WILDERNESS);
        floor.dun_level = 0;
        player.current_floor_ptr = &floor;
    }

    MagicFixture(const MagicFixture &) = delete;
    MagicFixture &operator=(const MagicFixture &) = delete;
    MagicFixture(MagicFixture &&) = delete;
    MagicFixture &operator=(MagicFixture &&) = delete;

private:
    decltype(egos_info) saved_egos = std::move(egos_info);
    util::Finalizer<std::function<void()>> restore_egos{ [this] { egos_info.swap(this->saved_egos); } };

public:
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    test::DungeonListTestAccess saved_dungeons;
    FloorType floor;
    PlayerType player;
};

struct NaturalExpected {
    short base_id;
    uint32_t seed;
    int ego;
    short result_id; //!< 生成後のベースアイテム ID（宵闇のローブは黄昏のローブに替わる）
    int ac;
    std::array<int, 3> to_h;
    std::array<int, 3> to_d;
    std::array<int, 3> to_a;
    std::array<int, 3> pval;
    std::array<TrFlags, 3> art_flags;
    std::array<uint32_t, 3> next;
};

// 分割前のコードで採取した値。配列は階 1・40・80 の順で、ダイスは全行 2d5。
// 先頭の 17 行は各部位をシード 12345 で生成し、残りの行はエゴの選択ループの引き直しと、
// エゴごとの補正（切れ味・地震・可視・ローブの高級エゴ）を通るシードを選んだ。
// 期待値は enchanter だけでなく、apply_ego() と乱数の補助関数（m_bonus()・randnor()・ProbabilityTable）の
// 乱数の消費回数にも依存する。
const std::array NATURAL_EXPECTED{
    NaturalExpected{ 42, 12345U, 65, 42, 3, { { 11, 16, 20 } }, { { 9, 13, 18 } }, { { 1, 1, 1 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3712427335U, 3712427335U, 3712427335U } } },
    NaturalExpected{ 43, 12345U, 65, 43, 3, { { 11, 16, 20 } }, { { 9, 13, 18 } }, { { 1, 1, 1 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3712427335U, 3712427335U, 3712427335U } } },
    NaturalExpected{ 44, 12345U, 65, 44, 3, { { 11, 16, 20 } }, { { 9, 13, 18 } }, { { 1, 1, 1 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3712427335U, 3712427335U, 3712427335U } } },
    NaturalExpected{ 84, 12345U, 100, 84, 3, { { 9, 14, 18 } }, { { 7, 11, 16 } }, { { 3, 3, 3 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 4287762405U, 4287762405U, 4287762405U } } },
    NaturalExpected{ 45, 12345U, 104, 45, 3, { { 9, 14, 18 } }, { { 9, 13, 18 } }, { { 3, 3, 3 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 1380172825U, 1380172825U, 1380172825U } } },
    NaturalExpected{ 46, 12345U, 112, 46, 3, { { 8, 12, 14 } }, { { 8, 11, 14 } }, { { 3, 3, 3 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 1380172825U, 1380172825U, 1380172825U } } },
    NaturalExpected{ 47, 12345U, 21, 47, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 9, 14, 18 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3983896229U, 3983896229U, 3983896229U } } },
    NaturalExpected{ 94, 12345U, 31, 94, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 9, 14, 18 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3983896229U, 3983896229U, 3983896229U } } },
    NaturalExpected{ 48, 12345U, 33, 48, 3, { { 1, 1, 1 } }, { { 2, 2, 2 } }, { { 10, 15, 19 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 2377245718U, 2377245718U, 2377245718U } } },
    NaturalExpected{ 49, 12345U, 9, 49, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 9, 14, 18 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3983896229U, 3983896229U, 3983896229U } } },
    NaturalExpected{ 101, 12345U, 8, 101, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 10, 15, 19 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 1268158873U, 1268158873U, 1268158873U } } },
    NaturalExpected{ 50, 12345U, 57, 50, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 9, 14, 18 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3983896229U, 3983896229U, 3983896229U } } },
    NaturalExpected{ 51, 12345U, 50, 51, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 9, 14, 18 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3983896229U, 3983896229U, 3983896229U } } },
    NaturalExpected{ 52, 12345U, 40, 52, 3, { { 2, 2, 2 } }, { { 2, 2, 2 } }, { { 15, 15, 19 } }, { { 2, 2, 2 } }, { { {}, {}, {} } }, { { 1268158873U, 1268158873U, 1268158873U } } },
    NaturalExpected{ 53, 12345U, 140, 53, 3, { { 3, 3, 3 } }, { { 3, 3, 3 } }, { { 2, 2, 2 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 4222726864U, 4222726864U, 4222726864U } } },
    // 鏡の盾: 反射を引いてエゴなしに落とす
    NaturalExpected{ 122, 12345U, 0, 122, 3, { { 0, 0, 0 } }, { { 0, 0, 0 } }, { { 7, 12, 16 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 1845991263U, 1845991263U, 1845991263U } } },
    // フェアノールのランプ: 長時間を引き直す
    NaturalExpected{ 582, 12345U, 140, 582, 3, { { 3, 3, 3 } }, { { 3, 3, 3 } }, { { 2, 2, 2 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 4222726864U, 4222726864U, 4222726864U } } },
    // 剣: 地震を引き直して切れ味。pval を足す
    NaturalExpected{ 42, 12374U, 78, 42, 3, { { 6, 9, 14 } }, { { 7, 11, 16 } }, { { 1, 1, 1 } }, { { 1, 2, 4 } }, { { {}, {}, {} } }, { { 405891879U, 405891879U, 405891879U } } },
    // 鈍器: 切れ味を引き直して地震。階 80 で攻撃回数、階 1・40 で pval
    NaturalExpected{ 43, 12513U, 79, 43, 3, { { 6, 7, 11 } }, { { 3, 5, 11 } }, { { 1, 1, 2 } }, { { 0, 1, 0 } }, { { {}, {}, { TR_BLOWS } } }, { { 2543139531U, 2543139531U, 1241127586U } } },
    // 鈍器: 地震。階 80 でも pval
    NaturalExpected{ 43, 12475U, 79, 43, 3, { { 7, 7, 11 } }, { { 9, 13, 18 } }, { { 2, 2, 2 } }, { { 1, 1, 2 } }, { { {}, {}, {} } }, { { 1942775778U, 1942775778U, 1942775778U } } },
    // 兜: 可視で弱いテレパシーを足す
    NaturalExpected{ 94, 12362U, 30, 94, 3, { { 2, 2, 2 } }, { { 3, 3, 3 } }, { { 7, 11, 16 } }, { { 0, 0, 0 } }, { { { TR_ESP_DEMON }, { TR_ESP_DEMON }, { TR_ESP_DEMON } } }, { { 2560577606U, 2560577606U, 2560577606U } } },
    // 兜: 可視で弱いテレパシーを足さない
    NaturalExpected{ 94, 12346U, 30, 94, 3, { { 3, 3, 3 } }, { { 2, 2, 2 } }, { { 4, 6, 8 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3927726292U, 3927726292U, 3927726292U } } },
    // 冠: 可視で弱いテレパシーを足す
    NaturalExpected{ 48, 12360U, 30, 48, 3, { { 3, 3, 3 } }, { { 3, 3, 3 } }, { { 8, 11, 15 } }, { { 0, 0, 0 } }, { { { TR_ESP_ANIMAL }, { TR_ESP_ANIMAL }, { TR_ESP_ANIMAL } } }, { { 1712875735U, 1712875735U, 1712875735U } } },
    // 冠: 可視で弱いテレパシーを足さない
    NaturalExpected{ 48, 12346U, 30, 48, 3, { { 3, 3, 3 } }, { { 2, 2, 2 } }, { { 4, 6, 8 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3927726292U, 3927726292U, 3927726292U } } },
    // ローブ: 高級エゴの永続
    NaturalExpected{ 101, 12353U, 11, 101, 3, { { 3, 3, 3 } }, { { 3, 3, 3 } }, { { 8, 12, 17 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 2616022695U, 2616022695U, 2616022695U } } },
    // ローブ: 高級エゴの宵闇。黄昏のローブに置き換えて AC 0
    NaturalExpected{ 101, 12348U, 12, 645, 0, { { 3, 3, 3 } }, { { 1, 1, 1 } }, { { 2, 2, 2 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 2871976539U, 2871976539U, 2871976539U } } },
    // ローブ: ドワーフを引き直す
    NaturalExpected{ 101, 12352U, 9, 101, 3, { { 3, 3, 3 } }, { { 1, 1, 1 } }, { { 8, 12, 16 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3483976439U, 3483976439U, 3483976439U } } },
    // 重鎧: ドルイドを引き直す
    NaturalExpected{ 49, 12346U, 8, 49, 3, { { 3, 3, 3 } }, { { 2, 2, 2 } }, { { 4, 6, 8 } }, { { 0, 0, 0 } }, { { {}, {}, {} } }, { { 3927726292U, 3927726292U, 3927726292U } } },
};

}

TEST_CASE("natural enchanters keep the generated values and RNG consumption")
{
    MagicFixture fixture;
    const std::array<DEPTH, 3> depths{ { 1, 40, 80 } };
    for (size_t depth_index = 0; depth_index < depths.size(); ++depth_index) {
        const auto depth = depths[depth_index];
        for (const auto &expected : NATURAL_EXPECTED) {
            CAPTURE(expected.base_id);
            CAPTURE(expected.seed);
            CAPTURE(depth);
            const auto rng = test::scoped_rng(expected.seed);
            ItemEntity item(expected.base_id);
            ItemMagicApplier(&fixture.player, &item, depth, AM_GOOD | AM_GREAT).execute();
            CHECK(!item.is_random_artifact());
            CHECK(item.bi_id == expected.result_id);
            CHECK(static_cast<int>(item.ego_idx) == expected.ego);
            CHECK(item.ac == expected.ac);
            CHECK(item.to_h == expected.to_h[depth_index]);
            CHECK(item.to_d == expected.to_d[depth_index]);
            CHECK(item.to_a == expected.to_a[depth_index]);
            CHECK(item.pval == expected.pval[depth_index]);
            CHECK(item.damage_dice.num == 2);
            CHECK(item.damage_dice.sides == 5);
            CHECK(item.art_flags.str() == expected.art_flags[depth_index].str());
            CHECK(get_game_rng()() == expected.next[depth_index]);
        }
    }
}

TEST_CASE("execute_with_ego applies the specified ego with its special adjustments")
{
    MagicFixture fixture;
    egos_info.at(EgoType::DF).act_idx = RandomArtActType::SUNLIGHT;
    struct Specified {
        short base_id;
        EgoType ego;
    };
    const std::array cases{
        Specified{ 42, EgoType::DF },
        Specified{ 42, EgoType::SHARPNESS },
        Specified{ 43, EgoType::EARTHQUAKES },
        Specified{ 44, EgoType::DF },
        Specified{ 84, EgoType::DIGGING },
        Specified{ 45, EgoType::ACCURACY },
        Specified{ 46, EgoType::HURT_EVIL },
        Specified{ 47, EgoType::S_DWARVEN },
        Specified{ 122, EgoType::ENDURE_FIRE },
        Specified{ 94, EgoType::INFRAVISION },
        Specified{ 94, EgoType::SEEING },
        Specified{ 48, EgoType::SEEING },
        Specified{ 48, EgoType::TELEPATHY },
        Specified{ 49, EgoType::DWARVEN },
        Specified{ 101, EgoType::DRUID },
        Specified{ 101, EgoType::PERMANENCE },
        Specified{ 101, EgoType::TWILIGHT },
        Specified{ 50, EgoType::QUIET },
        Specified{ 51, EgoType::FREE_ACTION },
        Specified{ 52, EgoType::STEALTH },
        Specified{ 53, EgoType::LITE_LONG },
        Specified{ 582, EgoType::LITE_SHINE },
    };
    auto saw_seeing_telepathy = false;
    auto saw_earthquakes_blows = false;
    for (const auto depth : std::array<DEPTH, 3>{ { 1, 40, 80 } }) {
        for (const auto seed : { 1U, 42U, 12345U, 98765U }) {
            for (const auto &entry : cases) {
                CAPTURE(entry.base_id);
                CAPTURE(static_cast<int>(entry.ego));
                CAPTURE(depth);
                CAPTURE(seed);
                const auto rng = test::scoped_rng(seed);
                ItemEntity item(entry.base_id);
                REQUIRE(can_apply_ego(item.bi_key, entry.ego));
                ItemMagicApplier(&fixture.player, &item, depth, AM_NO_FIXED_ART | AM_GOOD).execute_with_ego(entry.ego);
                CHECK(item.ego_idx == entry.ego);
                CHECK(!item.is_random_artifact());
                CHECK(!item.is_fixed_artifact());
                CHECK(item.activation_id == egos_info.at(entry.ego).act_idx);
                CHECK(item.curse_flags.none());
                if (entry.ego == EgoType::SHARPNESS) {
                    CHECK(item.pval > 0);
                }
                if (entry.ego == EgoType::EARTHQUAKES) {
                    // 浅い階では pval の m_bonus() が 0 になり得るので、効果は深い階でだけ確かめる。
                    const auto has_blows = item.art_flags.has(TR_BLOWS);
                    if (depth > 60) {
                        CHECK((has_blows || (item.pval > 0)));
                    }
                    if (has_blows) {
                        CHECK(depth > 60);
                        saw_earthquakes_blows = true;
                    }
                }
                if (entry.ego == EgoType::SEEING) {
                    saw_seeing_telepathy |= item.art_flags.has_any_of(LOW_TELEPATHY_FLAGS);
                }
                if (entry.ego == EgoType::TWILIGHT) {
                    CHECK(item.bi_id == 645);
                    CHECK(item.bi_key == BaseitemKey(ItemKindType::SOFT_ARMOR, SV_TWILIGHT_ROBE));
                    CHECK(item.ac == 0);
                } else {
                    CHECK(item.bi_id == entry.base_id);
                }
            }
        }
    }
    CHECK(saw_seeing_telepathy);
    CHECK(saw_earthquakes_blows);
}

TEST_CASE("execute_with_random_ego picks exactly the egos can_apply_ego accepts without artifacts")
{
    MagicFixture fixture;
    bool saw_permanence = false;
    bool saw_twilight = false;
    std::array<std::set<EgoType>, EGO_APPLICABLE_SPEC_COUNT> picked_egos;
    for (uint32_t seed = 1; seed <= 1024; ++seed) {
        for (size_t index = 0; index < EGO_APPLICABLE_SPEC_COUNT; ++index) {
            const auto &spec = BASE_SPECS[index];
            CAPTURE(spec.id);
            CAPTURE(seed);
            const auto rng = test::scoped_rng(seed);
            ItemEntity item(spec.id);
            const auto original_key = item.bi_key;
            ItemMagicApplier(&fixture.player, &item, 80, AM_NO_FIXED_ART | AM_GOOD).execute_with_random_ego();
            REQUIRE(item.is_ego());
            CHECK(!item.is_random_artifact());
            CHECK(!item.is_fixed_artifact());
            CHECK(!is_worthless_ego(item.get_ego()));
            CHECK(can_apply_ego(original_key, item.ego_idx));
            if (spec.kind == ItemKindType::DIGGING) {
                CHECK(item.ego_idx == EgoType::DIGGING);
            }
            if (item.ego_idx == EgoType::TWILIGHT) {
                saw_twilight = true;
                CHECK(item.bi_id == 645);
                CHECK(item.ac == 0);
            }
            saw_permanence |= item.ego_idx == EgoType::PERMANENCE;
            picked_egos[index].insert(item.ego_idx);
        }
    }
    CHECK(saw_permanence);
    CHECK(saw_twilight);

    // 逆向きに、can_apply_ego() が通すエゴはすべて enchanter の選択でも選ばれる。
    for (size_t index = 0; index < EGO_APPLICABLE_SPEC_COUNT; ++index) {
        const auto &spec = BASE_SPECS[index];
        CAPTURE(spec.id);
        const BaseitemKey key(spec.kind, spec.sval);
        for (const auto &entry : egos_info) {
            const auto ego = entry.first;
            CAPTURE(static_cast<int>(ego));
            CHECK(can_apply_ego(key, ego) == picked_egos[index].contains(ego));
        }
    }
}
