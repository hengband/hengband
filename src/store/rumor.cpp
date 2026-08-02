#include "store/rumor.h"
#include "locale/language-switcher.h"
#include "locale/localized-string.h"
#include "rumor/rumor-definition.h"
#include "rumor/rumor-service.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/town-list.h"
#include "system/floor/town-records.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-record.h"
#include "system/monrace/monrace-records.h"
#include "view/display-messages.h"
#include <fmt/format.h>

namespace {
void display_town_rumor(const RumorDefinition &rumor)
{
    msg_erase();
    msg_print(rumor.get_description());
    const auto town_id = std::get<TownId>(rumor.get_id());
    auto &town_records = TownRecords::get_instance();
    if (town_records.has_visited(town_id)) {
        return;
    }

    town_records.set_visited(town_id);
    msg_erase();
    constexpr auto fmt = _("{}への行き方が分かった。", "You know the way to {}.");
    msg_print(fmt, TownList::get_instance().get_town(town_id).get_name());
}

void display_dungeon_rumor(const RumorDefinition &rumor)
{
    const auto dungeon_id = std::get<DungeonId>(rumor.get_id());
    msg_erase();
    msg_print(rumor.get_description());
    auto &dungeon_record = DungeonRecords::get_instance().get_record(dungeon_id);
    const auto &dungeon = DungeonList::get_instance().get_dungeon(dungeon_id);
    const auto &dungeon_name = dungeon.name;
    if (dungeon_record.has_entered()) {
        msg_erase();
        constexpr auto fmt = _("あなたは{}への行き方を既に知っている。", "You can already recall to {}.");
        msg_print(fmt, dungeon_name);
        return;
    }

    dungeon_record.set_max_level(dungeon.mindepth);
    msg_erase();
    constexpr auto fmt = _("{}に帰還できるようになった。", "You can recall to {}.");
    msg_print(fmt, dungeon_name);
}

void display_monster_rumor(const RumorDefinition &rumor)
{
    const auto monrace_id = std::get<MonraceId>(rumor.get_id());
    auto record = MonraceRecords::get_instance().get_record(monrace_id);
    msg_erase();
    msg_print(rumor.get_description());
    if (record->has_been_seen()) {
        return;
    }

    record->increment_seen_count();
    msg_erase();
    constexpr auto fmt = _("{}の名前を書き留めた。", "You note the name of {}.");
    msg_print(fmt, MonraceList::get_instance().get_name(monrace_id));
}

void display_artifact_rumor(const RumorDefinition &rumor)
{
    msg_erase();
    msg_print(rumor.get_description());
    const auto fa_id = std::get<FixedArtifactId>(rumor.get_id());
    auto &records = ArtifactRecords::get_instance();
    if (records.get_known(fa_id)) {
        return;
    }

    records.set_known(fa_id);
    msg_erase();
    constexpr auto fmt = _("{}の名前を書き留めた。", "You note the name of {}.");
    msg_print(fmt, ArtifactList::get_instance().get_full_name(fa_id));
}
}

void display_selected_rumor(const RumorDefinition &rumor)
{
    const auto rumor_type = rumor.get_type();
    switch (rumor_type) {
    case RumorType::GOSSIP:
        msg_erase();
        msg_print(rumor.get_description());
        return;
    case RumorType::TOWN:
        display_town_rumor(rumor);
        return;
    case RumorType::SHALLOW_DUNGEON:
    case RumorType::DEEP_DUNGEON:
        display_dungeon_rumor(rumor);
        return;
    case RumorType::NORMAL_MONSTER:
    case RumorType::UNIQUE_MONSTER:
        display_monster_rumor(rumor);
        return;
    case RumorType::SHALLOW_ARTIFACT:
    case RumorType::DEEP_ARTIFACT:
        display_artifact_rumor(rumor);
        return;
    default:
        THROW_EXCEPTION(std::runtime_error, fmt::format("Unknown rumor type exists in rumor list: {}", enum2i(rumor_type)));
    }
}
