#include "rumor/rumor-list.h"
#include "artifact/fixed-art-types.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "rumor/rumor-definition.h"
#include "system/angband-exceptions.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/floor/town-records.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "term/z-rand.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <fmt/format.h>
#include <fstream>
#include <range/v3/view.hpp>

namespace {
constexpr auto DEPTH_THRESHOLD = 30;

constexpr auto NAME_TEMPLATE = "{Name}";
}

RumorList RumorList::instance{};

RumorList::RumorList()
{
    this->rumor_definitions[RumorRarity::LOW][RumorType::GOSSIP] = std::vector<RumorDefinition>{};

    auto &medium = this->rumor_definitions[RumorRarity::MEDIUM];
    medium[RumorType::TOWN] = std::vector<RumorDefinition>{};
    medium[RumorType::SHALLOW_DUNGEON] = std::vector<RumorDefinition>{};
    medium[RumorType::SHALLOW_ARTIFACT] = std::vector<RumorDefinition>{};
    medium[RumorType::NORMAL_MONSTER] = std::vector<RumorDefinition>{};

    auto &high = this->rumor_definitions[RumorRarity::HIGH];
    high[RumorType::DEEP_ARTIFACT] = std::vector<RumorDefinition>{};
    high[RumorType::UNIQUE_MONSTER] = std::vector<RumorDefinition>{};
    high[RumorType::DEEP_DUNGEON] = std::vector<RumorDefinition>{};

    this->type_template[RumorType::SHALLOW_DUNGEON] = _("{Name}の場所はココだ： -続く-", "The location of {Name} is here: -more-");
    this->type_template[RumorType::NORMAL_MONSTER] = _("{Name}というモンスターがいるらしい。", "There is a monster called {Name}.");
    this->type_template[RumorType::SHALLOW_ARTIFACT] = _("{Name}というお宝が地下浅くにあるそうだ。", "There is a treasure called {Name} in shallow dungeons.");
    this->type_template[RumorType::DEEP_DUNGEON] = _("{Name}の場所はココだ： -続く-", "The location of {Name} is here: -more-");
    this->type_template[RumorType::UNIQUE_MONSTER] = _("{Name}というユニーク・モンスターがいるらしい。", "There is a unique monster called {Name}.");
    this->type_template[RumorType::DEEP_ARTIFACT] = _("{Name}というお宝が地下深くにあるそうだ。", "There is a treasure called {Name} in deep dungeons.");
    this->type_template[RumorType::TOWN] = _("{Name}という街に行ったことはあるかい？", "Have you ever been to the town of {Name}?");
}

RumorList &RumorList::get_instance()
{
    return instance;
}

const RumorDefinition &RumorList::select_random_rumor() const
{
    std::vector<int> selected_rumors;
    ProbabilityTable<int>::lottery(std::back_inserter(selected_rumors), this->rumor_tables, 1);
    return this->random_rumors.at(selected_rumors.front());
}

const RumorDefinition &RumorList::select_rumor(RumorRarity rt) const
{
    const auto &rumors_map = this->rumor_definitions.at(rt);
    switch (rt) {
    case RumorRarity::LOW:
        return this->select_low_rumor(rumors_map);
    case RumorRarity::MEDIUM:
        return this->select_medium_rumor(rumors_map);
    case RumorRarity::HIGH:
        return this->select_high_rumor(rumors_map);
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid RumorRarity value: {}", enum2i(rt)));
    }
}

void RumorList::read_rumors(const std::filesystem::path &path)
{
    auto file = std::ifstream(path);
    if (!file) {
        THROW_EXCEPTION(std::runtime_error, "Failed to open rumor file: " + path.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        line = utf8_to_local(line);
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        constexpr std::monostate dummy;
        this->rumor_definitions.at(RumorRarity::LOW).at(RumorType::GOSSIP).emplace_back(RumorType::GOSSIP, dummy, line);
    }
}

void RumorList::add_towns()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::TOWN);
    const auto &towns = TownList::get_instance();
    for (auto i = 1; i < std::ssize(towns); i++) {
        if (i != SECRET_TOWN) {
            const auto town_name = str_replace(this->type_template.at(RumorType::TOWN), NAME_TEMPLATE, towns.get_town(i).get_name());
            rumors.emplace_back(RumorType::TOWN, i2enum<TownId>(i - 1), town_name);
        }
    }
}

void RumorList::add_shallow_dungeons()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::SHALLOW_DUNGEON);
    for (const auto &[id, dungeon] : DungeonList::get_instance() | ranges::views::drop(1)) {
        if (0 < dungeon->mindepth && dungeon->mindepth <= DEPTH_THRESHOLD) {
            const auto dungeon_name = str_replace(this->type_template.at(RumorType::SHALLOW_DUNGEON), NAME_TEMPLATE, dungeon->name);
            rumors.emplace_back(RumorType::SHALLOW_DUNGEON, id, dungeon_name);
        }
    }
}

void RumorList::add_normal_monsters()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::NORMAL_MONSTER);
    for (const auto &[id, name] : MonraceList::get_instance().get_normal_monster_names()) {
        const auto monster_name = str_replace(this->type_template.at(RumorType::NORMAL_MONSTER), NAME_TEMPLATE, name.string());
        rumors.emplace_back(RumorType::NORMAL_MONSTER, id, monster_name);
    }
}

/*!
 * @brief 地下浅くにあるアーティファクトの噂を追加する
 */
void RumorList::add_shallow_artifacts()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::SHALLOW_ARTIFACT);
    for (const auto &[id, artifact] : ArtifactList::get_instance()) {
        if (artifact.level <= DEPTH_THRESHOLD) {
            const auto artifact_name = str_replace(this->type_template.at(RumorType::SHALLOW_ARTIFACT), NAME_TEMPLATE, artifact.build_full_name());
            rumors.emplace_back(RumorType::SHALLOW_ARTIFACT, id, artifact_name);
        }
    }
}

void RumorList::add_deep_dungeons()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::DEEP_DUNGEON);
    for (const auto &[id, dungeon] : DungeonList::get_instance() | ranges::views::drop(1)) {
        if (dungeon->mindepth > DEPTH_THRESHOLD) {
            const auto dungeon_name = str_replace(this->type_template.at(RumorType::DEEP_DUNGEON), NAME_TEMPLATE, dungeon->name);
            rumors.emplace_back(RumorType::DEEP_DUNGEON, id, dungeon_name);
        }
    }
}

void RumorList::add_unique_monsters()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::UNIQUE_MONSTER);
    for (const auto &[id, name] : MonraceList::get_instance().get_unique_monster_names()) {
        const auto monster_name = str_replace(this->type_template.at(RumorType::UNIQUE_MONSTER), NAME_TEMPLATE, name.string());
        rumors.emplace_back(RumorType::UNIQUE_MONSTER, id, monster_name);
    }
}

/*!
 * @brief 地下深くにあるアーティファクトの噂を追加する
 */
void RumorList::add_deep_artifacts()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::DEEP_ARTIFACT);
    for (const auto &[id, artifact] : ArtifactList::get_instance()) {
        if (artifact.level > DEPTH_THRESHOLD) {
            const auto artifact_name = str_replace(this->type_template.at(RumorType::DEEP_ARTIFACT), NAME_TEMPLATE, artifact.build_full_name());
            rumors.emplace_back(RumorType::DEEP_ARTIFACT, id, artifact_name);
        }
    }
}

void RumorList::validate() const
{
    for (const auto &[rarity, rumors_map] : this->rumor_definitions) {
        for (const auto &[type, rumors] : rumors_map) {
            if (rumors.empty()) {
                THROW_EXCEPTION(std::runtime_error, fmt::format("No rumors found for type: {}", enum2i(type)));
            }
        }
    }
}

void RumorList::make_table()
{
    auto i = 0;
    for (const auto &[rarity, rumors_map] : this->rumor_definitions) {
        for (const auto &[type, rumors] : rumors_map) {
            for (const auto &rumor : rumors) {
                this->rumor_tables.entry_item(i, enum2i(rarity));
                this->random_rumors.push_back(rumor);
                i++;
            }
        }
    }
}

const RumorDefinition &RumorList::select_low_rumor(const std::map<RumorType, std::vector<RumorDefinition>> &rumors_map) const
{
    const auto &gossips = rumors_map.at(RumorType::GOSSIP);
    const auto gossip_size = gossips.size();
    return gossips.at(randnum0<size_t>(gossip_size));
}

const RumorDefinition &RumorList::select_medium_rumor(const std::map<RumorType, std::vector<RumorDefinition>> &rumors_map) const
{
    const auto town_size = rumors_map.at(RumorType::TOWN).size();
    const auto shallow_dungeon_size = rumors_map.at(RumorType::SHALLOW_DUNGEON).size();
    const auto normal_monster_size = rumors_map.at(RumorType::NORMAL_MONSTER).size();
    const auto shallow_artifact_size = rumors_map.at(RumorType::SHALLOW_ARTIFACT).size();
    const auto rumor_num = randnum0<size_t>(town_size + shallow_dungeon_size + normal_monster_size + shallow_artifact_size);
    if (rumor_num < town_size) {
        return rumors_map.at(RumorType::TOWN).at(rumor_num);
    }

    const auto max_shallow_dungeon_size = town_size + shallow_dungeon_size;
    if (town_size <= rumor_num && rumor_num < max_shallow_dungeon_size) {
        return rumors_map.at(RumorType::SHALLOW_DUNGEON).at(rumor_num - town_size);
    }

    const auto max_normal_monster_size = max_shallow_dungeon_size + normal_monster_size;
    if (max_shallow_dungeon_size <= rumor_num && rumor_num < max_normal_monster_size) {
        return rumors_map.at(RumorType::NORMAL_MONSTER).at(rumor_num - max_shallow_dungeon_size);
    }

    return rumors_map.at(RumorType::SHALLOW_ARTIFACT).at(rumor_num - max_normal_monster_size);
}

const RumorDefinition &RumorList::select_high_rumor(const std::map<RumorType, std::vector<RumorDefinition>> &rumors_map) const
{
    const auto deep_dungeon_size = rumors_map.at(RumorType::DEEP_DUNGEON).size();
    const auto unique_monster_size = rumors_map.at(RumorType::UNIQUE_MONSTER).size();
    const auto deep_artifact_size = rumors_map.at(RumorType::DEEP_ARTIFACT).size();
    const auto rumor_num = randnum0<size_t>(deep_dungeon_size + unique_monster_size + deep_artifact_size);
    if (rumor_num < deep_dungeon_size) {
        return rumors_map.at(RumorType::DEEP_DUNGEON).at(rumor_num);
    }

    const auto max_unique_monster_size = deep_dungeon_size + unique_monster_size;
    if (deep_dungeon_size <= rumor_num && rumor_num < max_unique_monster_size) {
        return rumors_map.at(RumorType::UNIQUE_MONSTER).at(rumor_num - deep_dungeon_size);
    }

    return rumors_map.at(RumorType::DEEP_ARTIFACT).at(rumor_num - max_unique_monster_size);
}
