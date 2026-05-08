#include "rumor/rumor-list.h"
#include "artifact/fixed-art-types.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "rumor/rumor-definition.h"
#include "rumor/rumor-rarity.h"
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

/*!
 * @brief 低レアリティの噂をランダムに選択する
 * @param rumors_map 噂の種類ごとの噂定義のマップ
 * @return 選択された噂定義
 */
const RumorDefinition &select_low_rumor(const std::map<RumorType, std::vector<std::shared_ptr<const RumorDefinition>>> &rumors_map)
{
    return *rand_choice(rumors_map.at(RumorType::GOSSIP));
}

/*!
 * @brief 中レアリティの噂をランダムに選択する
 * @param rumors_map 噂の種類ごとの噂定義のマップ
 * @return 選択された噂定義
 * @details 街・ダンジョン・モンスター・アーティファクト情報が得られる確率はカテゴリごとに1/4.
 * 小規模な街 or 元祖の街 オプションがONの時に街の噂をフィルタリングする処理は呼び出し元で責任を取ること.
 */
const RumorDefinition &select_medium_rumor(const std::map<RumorType, std::vector<std::shared_ptr<const RumorDefinition>>> &rumors_map)
{
    const auto rumor_category = randint0(4);
    switch (rumor_category) {
    case 0:
        return *rand_choice(rumors_map.at(RumorType::TOWN));
    case 1:
        return *rand_choice(rumors_map.at(RumorType::SHALLOW_DUNGEON));
    case 2:
        return *rand_choice(rumors_map.at(RumorType::NORMAL_MONSTER));
    case 3:
        return *rand_choice(rumors_map.at(RumorType::SHALLOW_ARTIFACT));
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid random number for medium rumor selection: {}", rumor_category));
    }
}

/*!
 * @brief 高レアリティの噂をランダムに選択する
 * @param rumors_map 噂の種類ごとの噂定義のマップ
 * @return 選択された噂定義
 * @details ダンジョン・モンスター・アーティファクト情報を全てごちゃまぜ等確率で選ぶ.
 * 天界と地獄がサーペント撃破後に解禁される仕様は呼び出し元で責任を取ること.
 */
const RumorDefinition &select_high_rumor(const std::map<RumorType, std::vector<std::shared_ptr<const RumorDefinition>>> &rumors_map)
{
    const auto &deep_dungeons = rumors_map.at(RumorType::DEEP_DUNGEON);
    const auto deep_dungeon_size = deep_dungeons.size();
    const auto unique_monster_size = rumors_map.at(RumorType::UNIQUE_MONSTER).size();
    const auto deep_artifact_size = rumors_map.at(RumorType::DEEP_ARTIFACT).size();
    const auto rumor_num = randnum0<size_t>(deep_dungeon_size + unique_monster_size + deep_artifact_size);
    if (rumor_num < deep_dungeon_size) {
        return *deep_dungeons.at(rumor_num);
    }

    const auto max_unique_monster_size = deep_dungeon_size + unique_monster_size;
    if (rumor_num < max_unique_monster_size) {
        return *rumors_map.at(RumorType::UNIQUE_MONSTER).at(rumor_num - deep_dungeon_size);
    }

    return *rumors_map.at(RumorType::DEEP_ARTIFACT).at(rumor_num - max_unique_monster_size);
}
}

RumorList RumorList::instance{};

RumorList::RumorList()
{
    this->rumor_definitions[RumorRarity::LOW][RumorType::GOSSIP] = std::vector<std::shared_ptr<const RumorDefinition>>{};

    auto &medium = this->rumor_definitions[RumorRarity::MEDIUM];
    medium[RumorType::TOWN] = std::vector<std::shared_ptr<const RumorDefinition>>{};
    medium[RumorType::SHALLOW_DUNGEON] = std::vector<std::shared_ptr<const RumorDefinition>>{};
    medium[RumorType::SHALLOW_ARTIFACT] = std::vector<std::shared_ptr<const RumorDefinition>>{};
    medium[RumorType::NORMAL_MONSTER] = std::vector<std::shared_ptr<const RumorDefinition>>{};

    auto &high = this->rumor_definitions[RumorRarity::HIGH];
    high[RumorType::DEEP_ARTIFACT] = std::vector<std::shared_ptr<const RumorDefinition>>{};
    high[RumorType::UNIQUE_MONSTER] = std::vector<std::shared_ptr<const RumorDefinition>>{};
    high[RumorType::DEEP_DUNGEON] = std::vector<std::shared_ptr<const RumorDefinition>>{};

    this->type_template[RumorType::TOWN] = _("{Name}という街に行ったことはあるかい？", "Have you ever been to the town of {Name}?");
    this->type_template[RumorType::SHALLOW_DUNGEON] = _("{Name}の場所はココだ： -続く-", "The location of {Name} is here: -more-");
    this->type_template[RumorType::NORMAL_MONSTER] = _("{Name}というモンスターがいるらしい。", "There is a monster called {Name}.");
    this->type_template[RumorType::SHALLOW_ARTIFACT] = _("{Name}というお宝が地下浅くにあるそうだ。", "There is a treasure called {Name} in shallow dungeons.");
    this->type_template[RumorType::DEEP_DUNGEON] = _("{Name}の場所はココだ： -続く-", "The location of {Name} is here: -more-");
    this->type_template[RumorType::UNIQUE_MONSTER] = _("{Name}という特別なモンスターがいるらしい。", "There is a special monster called {Name}.");
    this->type_template[RumorType::DEEP_ARTIFACT] = _("{Name}というお宝が地下深くにあるそうだ。", "There is a treasure called {Name} in deep dungeons.");
}

RumorList &RumorList::get_instance()
{
    return instance;
}

/*!
 * @brief 噂を重み付けに従って選択する
 * @return 噂情報
 */
const RumorDefinition &RumorList::select_random_rumor() const
{
    return *this->random_rumors_table.pick_one_at_random();
}

/*!
 * @brief 噂をレアリティ指定で選択する
 * @param rt レアリティ
 * @return 噂情報
 */
const RumorDefinition &RumorList::select_rumor(RumorRarity rt) const
{
    const auto &rumors_map = this->rumor_definitions.at(rt);
    switch (rt) {
    case RumorRarity::LOW:
        return select_low_rumor(rumors_map);
    case RumorRarity::MEDIUM:
        return select_medium_rumor(rumors_map);
    case RumorRarity::HIGH:
        return select_high_rumor(rumors_map);
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid RumorRarity value: {}", enum2i(rt)));
    }
}

/*!
 * @brief 噂定義ファイルから噂の文章を読み取ってテーブルに追加する
 * @param path 噂定義ファイルのフルパス
 */
void RumorList::read_rumors(const std::filesystem::path &path)
{
    auto ifs = std::ifstream(path);
    if (!ifs) {
        constexpr auto fmt = _("噂定義ファイルの読み込みに失敗しました: {}", "Failed to open the file of rumor definitions: {}");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string()));
    }

    auto &rumors = this->rumor_definitions.at(RumorRarity::LOW).at(RumorType::GOSSIP);
    std::string line;
    while (std::getline(ifs, line)) {
        line = utf8_to_local(line);
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        constexpr std::monostate dummy;
        rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::GOSSIP, dummy, line));
    }

    if (ifs.bad() || (ifs.fail() && !ifs.eof())) {
        constexpr auto fmt = _("噂定義ファイルの読み込みに失敗しました: {}", "Failed to read the file of rumor definitions: {}");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string()));
    }
}

/*!
 * @brief 街の噂をテーブルに追加する
 */
void RumorList::add_towns()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::TOWN);
    const auto &towns = TownList::get_instance();
    for (size_t i = 1; i < towns.size(); i++) {
        if ((i < VALID_TOWNS) && (i != SECRET_TOWN)) {
            const auto town_name = str_replace(this->type_template.at(RumorType::TOWN), NAME_TEMPLATE, towns.get_town(i).get_name());
            rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::TOWN, i2enum<TownId>(i - 1), town_name));
        }
    }
}

/*!
 * @brief 入口の浅いダンジョンの噂をテーブルに追加する
 */
void RumorList::add_shallow_dungeons()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::SHALLOW_DUNGEON);
    for (const auto &[id, dungeon] : DungeonList::get_instance() | ranges::views::drop(1)) {
        if (0 < dungeon->mindepth && dungeon->mindepth <= DEPTH_THRESHOLD) {
            const auto dungeon_name = str_replace(this->type_template.at(RumorType::SHALLOW_DUNGEON), NAME_TEMPLATE, dungeon->name);
            rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::SHALLOW_DUNGEON, id, dungeon_name));
        }
    }
}

/*!
 * @brief ユニークフラグもナズグルフラグもないモンスターの噂をテーブルに追加する
 */
void RumorList::add_normal_monsters()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::NORMAL_MONSTER);
    for (const auto &[id, name] : MonraceList::get_instance().get_normal_monster_names()) {
        const auto monster_name = str_replace(this->type_template.at(RumorType::NORMAL_MONSTER), NAME_TEMPLATE, name.string());
        rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::NORMAL_MONSTER, id, monster_name));
    }
}

/*!
 * @brief 地下浅くにあるアーティファクトの噂をテーブルに追加する
 */
void RumorList::add_shallow_artifacts()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::SHALLOW_ARTIFACT);
    for (const auto &[id, artifact] : ArtifactList::get_instance()) {
        if (artifact.level <= DEPTH_THRESHOLD) {
            const auto artifact_name = str_replace(this->type_template.at(RumorType::SHALLOW_ARTIFACT), NAME_TEMPLATE, artifact.build_full_name());
            rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::SHALLOW_ARTIFACT, id, artifact_name));
        }
    }
}

/*!
 * @brief 入口の深いダンジョンの噂をテーブルに追加する
 */
void RumorList::add_deep_dungeons()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::DEEP_DUNGEON);
    for (const auto &[id, dungeon] : DungeonList::get_instance() | ranges::views::drop(1)) {
        if (dungeon->mindepth > DEPTH_THRESHOLD) {
            const auto dungeon_name = str_replace(this->type_template.at(RumorType::DEEP_DUNGEON), NAME_TEMPLATE, dungeon->name);
            rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::DEEP_DUNGEON, id, dungeon_name));
        }
    }
}

/*!
 * @brief ユニークフラグかナズグルフラグのあるモンスターの噂をテーブルに追加する
 */
void RumorList::add_unique_monsters()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::UNIQUE_MONSTER);
    for (const auto &[id, name] : MonraceList::get_instance().get_unique_monster_names()) {
        const auto monster_name = str_replace(this->type_template.at(RumorType::UNIQUE_MONSTER), NAME_TEMPLATE, name.string());
        rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::UNIQUE_MONSTER, id, monster_name));
    }
}

/*!
 * @brief 地下深くにあるアーティファクトの噂をテーブルに追加する
 */
void RumorList::add_deep_artifacts()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::DEEP_ARTIFACT);
    for (const auto &[id, artifact] : ArtifactList::get_instance()) {
        if (artifact.level > DEPTH_THRESHOLD) {
            const auto artifact_name = str_replace(this->type_template.at(RumorType::DEEP_ARTIFACT), NAME_TEMPLATE, artifact.build_full_name());
            rumors.emplace_back(std::make_shared<const RumorDefinition>(RumorType::DEEP_ARTIFACT, id, artifact_name));
        }
    }
}

/*!
 * @brief 全ての噂テーブルが1つ以上の噂を持つかを確認する
 */
void RumorList::validate() const
{
    for (const auto &[_, rumors_map] : this->rumor_definitions) {
        for (const auto &[type, rumors] : rumors_map) {
            if (rumors.empty()) {
                THROW_EXCEPTION(std::runtime_error, fmt::format("No rumors found for type: {}", enum2i(type)));
            }
        }
    }
}

/*!
 * @brief 噂の巻物用に重み付けした噂テーブルを作る
 *
 * 冪等性を担保するため、呼ばれる度にテーブルをクリアする
 */
void RumorList::make_table()
{
    this->random_rumors_table.clear();
    for (const auto &[rarity, rumors_map] : this->rumor_definitions) {
        for (const auto &[type, rumors] : rumors_map) {
            for (const auto &rumor : rumors) {
                this->random_rumors_table.entry_item(rumor, enum2i(rarity));
            }
        }
    }
}
