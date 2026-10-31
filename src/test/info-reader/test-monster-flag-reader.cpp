/*!
 * @brief モンスターの定義とダンジョンの出現条件に共通するフラグ名の読み込み (monster-flag-reader) のテスト
 *
 * 各読み込み処理からの呼び出しは、test-race-reader.cpp・test-dungeon-reader.cpp で扱う。
 */

#include "info-reader/monster-flag-reader.h"
#include "info-reader/race-info-tokens-table.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-sex.h"
#include "util/flag-group.h"
#include "view/display-messages.h"
#include <cstddef>
#include <doctest/doctest.h>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
/*!
 * @brief 基本フラグの格納先を実体で持ち、MonsterBasicFlagGroups として渡す
 */
struct BasicFlagStore {
    EnumClassFlagGroup<MonsterResistanceType> resistance_flags;
    EnumClassFlagGroup<MonsterAuraType> aura_flags;
    EnumClassFlagGroup<MonsterBehaviorType> behavior_flags;
    EnumClassFlagGroup<MonsterVisualType> visual_flags;
    EnumClassFlagGroup<MonsterKindType> kind_flags;
    EnumClassFlagGroup<MonsterDropType> drop_flags;
    EnumClassFlagGroup<MonsterWildernessType> wilderness_flags;
    EnumClassFlagGroup<MonsterFeatureType> feature_flags;
    EnumClassFlagGroup<MonsterPopulationType> population_flags;
    EnumClassFlagGroup<MonsterSpeakType> speak_flags;
    EnumClassFlagGroup<MonsterBrightnessType> brightness_flags;
    EnumClassFlagGroup<MonsterSpecialType> special_flags;
    EnumClassFlagGroup<MonsterMiscType> misc_flags;

    MonsterBasicFlagGroups groups()
    {
        return {
            .resistance_flags = &this->resistance_flags,
            .aura_flags = &this->aura_flags,
            .behavior_flags = &this->behavior_flags,
            .visual_flags = &this->visual_flags,
            .kind_flags = &this->kind_flags,
            .drop_flags = &this->drop_flags,
            .wilderness_flags = &this->wilderness_flags,
            .feature_flags = &this->feature_flags,
            .population_flags = &this->population_flags,
            .speak_flags = &this->speak_flags,
            .brightness_flags = &this->brightness_flags,
            .special_flags = &this->special_flags,
            .misc_flags = &this->misc_flags,
        };
    }

    size_t total_count() const
    {
        return this->resistance_flags.count() + this->aura_flags.count() + this->behavior_flags.count() + this->visual_flags.count() +
               this->kind_flags.count() + this->drop_flags.count() + this->wilderness_flags.count() + this->feature_flags.count() +
               this->population_flags.count() + this->speak_flags.count() + this->brightness_flags.count() + this->special_flags.count() +
               this->misc_flags.count();
    }
};

/*!
 * @brief 表のすべての名前について、その表のフラグの格納先にだけフラグが1つ立つことを確かめる
 */
template <typename Enum>
void check_every_name_sets_only_its_group(const std::unordered_map<std::string_view, Enum> &table, EnumClassFlagGroup<Enum> BasicFlagStore::*member)
{
    for (const auto &[name, flag] : table) {
        CAPTURE(std::string(name));
        BasicFlagStore store;
        ScopedMessageCapture diagnostics;
        CHECK(grab_one_monster_basic_flag(store.groups(), name));
        CHECK((store.*member).has(flag));
        CHECK((store.*member).count() == 1);
        CHECK(store.total_count() == 1);
        CHECK(diagnostics.messages().empty());
    }
}

template <typename Enum>
void insert_names(std::multiset<std::string_view> &names, const std::unordered_map<std::string_view, Enum> &table)
{
    for (const auto &entry : table) {
        names.insert(entry.first);
    }
}
}

TEST_CASE("grab_one_monster_basic_flag sets a flag found in a basic flag table")
{
    BasicFlagStore store;
    ScopedMessageCapture diagnostics;
    CHECK(grab_one_monster_basic_flag(store.groups(), "IM_FIRE"));
    CHECK(grab_one_monster_basic_flag(store.groups(), "AURA_FIRE"));
    CHECK(grab_one_monster_basic_flag(store.groups(), "NEVER_MOVE"));
    CHECK(store.resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE));
    CHECK(store.aura_flags.has(MonsterAuraType::FIRE));
    CHECK(store.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE));
    CHECK(store.total_count() == 3);
    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_monster_basic_flag sets every name only in the group of its table")
{
    check_every_name_sets_only_its_group(r_info_flagsr, &BasicFlagStore::resistance_flags);
    check_every_name_sets_only_its_group(r_info_aura_flags, &BasicFlagStore::aura_flags);
    check_every_name_sets_only_its_group(r_info_behavior_flags, &BasicFlagStore::behavior_flags);
    check_every_name_sets_only_its_group(r_info_visual_flags, &BasicFlagStore::visual_flags);
    check_every_name_sets_only_its_group(r_info_kind_flags, &BasicFlagStore::kind_flags);
    check_every_name_sets_only_its_group(r_info_drop_flags, &BasicFlagStore::drop_flags);
    check_every_name_sets_only_its_group(r_info_wilderness_flags, &BasicFlagStore::wilderness_flags);
    check_every_name_sets_only_its_group(r_info_feature_flags, &BasicFlagStore::feature_flags);
    check_every_name_sets_only_its_group(r_info_population_flags, &BasicFlagStore::population_flags);
    check_every_name_sets_only_its_group(r_info_speak_flags, &BasicFlagStore::speak_flags);
    check_every_name_sets_only_its_group(r_info_brightness_flags, &BasicFlagStore::brightness_flags);
    check_every_name_sets_only_its_group(r_info_special_flags, &BasicFlagStore::special_flags);
    check_every_name_sets_only_its_group(r_info_misc_flags, &BasicFlagStore::misc_flags);
}

TEST_CASE("monster basic flag tables and the sex table share no name")
{
    // 名前が重ならないので、表を探す順序 (ダンジョンの出現条件では性別を先に探す) は結果に影響しない
    std::multiset<std::string_view> names;
    insert_names(names, r_info_flagsr);
    insert_names(names, r_info_aura_flags);
    insert_names(names, r_info_behavior_flags);
    insert_names(names, r_info_visual_flags);
    insert_names(names, r_info_kind_flags);
    insert_names(names, r_info_drop_flags);
    insert_names(names, r_info_wilderness_flags);
    insert_names(names, r_info_feature_flags);
    insert_names(names, r_info_population_flags);
    insert_names(names, r_info_speak_flags);
    insert_names(names, r_info_brightness_flags);
    insert_names(names, r_info_special_flags);
    insert_names(names, r_info_misc_flags);
    insert_names(names, r_info_sex);
    for (const auto name : names) {
        CAPTURE(std::string(name));
        CHECK(names.count(name) == 1);
    }
}

TEST_CASE("grab_one_monster_basic_flag keeps the flags already set")
{
    BasicFlagStore store;
    store.resistance_flags.set(MonsterResistanceType::IMMUNE_COLD);
    CHECK(grab_one_monster_basic_flag(store.groups(), "IM_FIRE"));
    CHECK(store.resistance_flags.has(MonsterResistanceType::IMMUNE_COLD));
    CHECK(store.resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE));
    CHECK(store.total_count() == 2);
}

TEST_CASE("grab_one_monster_basic_flag rejects an unknown name and prints it once")
{
    // 性別・能力フラグ・メッセージの名前は基本フラグの表には無い
    for (const auto *name : { "UNKNOWN_FLAG", "", "im_fire", "IM_FIRE ", "MALE", "NONE", "BR_FIRE", "WALK_CLOSERANGE" }) {
        CAPTURE(name);
        BasicFlagStore store;
        store.aura_flags.set(MonsterAuraType::COLD);
        ScopedMessageCapture diagnostics;
        CHECK_FALSE(grab_one_monster_basic_flag(store.groups(), name));
        CHECK(store.aura_flags.has(MonsterAuraType::COLD));
        CHECK(store.total_count() == 1);
        CHECK(diagnostics.messages().size() == 1);
    }
}

#ifndef JP
TEST_CASE("grab_one_monster_basic_flag reports the unknown name in the monster flag message")
{
    BasicFlagStore store;
    ScopedMessageCapture diagnostics;
    CHECK_FALSE(grab_one_monster_basic_flag(store.groups(), "UNKNOWN_FLAG"));
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0] == "Unknown monster flag 'UNKNOWN_FLAG'.");
}
#endif

TEST_CASE("grab_one_monster_spell_flag sets a flag found in the ability flag table")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    ScopedMessageCapture diagnostics;
    CHECK(grab_one_monster_spell_flag(ability_flags, "BR_FIRE"));
    CHECK(ability_flags.has(MonsterAbilityType::BR_FIRE));
    CHECK(ability_flags.count() == 1);
    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_monster_spell_flag keeps the flags already set")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags{ MonsterAbilityType::SHRIEK };
    CHECK(grab_one_monster_spell_flag(ability_flags, "BR_FIRE"));
    CHECK(ability_flags.has(MonsterAbilityType::SHRIEK));
    CHECK(ability_flags.has(MonsterAbilityType::BR_FIRE));
    CHECK(ability_flags.count() == 2);
}

TEST_CASE("grab_one_monster_spell_flag rejects an unknown name and prints it once")
{
    for (const auto *name : { "UNKNOWN_FLAG", "", "br_fire", "BR_FIRE ", "NEVER_MOVE" }) {
        CAPTURE(name);
        EnumClassFlagGroup<MonsterAbilityType> ability_flags{ MonsterAbilityType::SHRIEK };
        ScopedMessageCapture diagnostics;
        CHECK_FALSE(grab_one_monster_spell_flag(ability_flags, name));
        CHECK(ability_flags.has(MonsterAbilityType::SHRIEK));
        CHECK(ability_flags.count() == 1);
        CHECK(diagnostics.messages().size() == 1);
    }
}

#ifndef JP
TEST_CASE("grab_one_monster_spell_flag reports the unknown name in the monster flag message")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    ScopedMessageCapture diagnostics;
    CHECK_FALSE(grab_one_monster_spell_flag(ability_flags, "UNKNOWN_FLAG"));
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0] == "Unknown monster flag 'UNKNOWN_FLAG'.");
}
#endif
