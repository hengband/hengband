#include "system/dungeon/quest-list.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/jsonc-document-loader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "io/files-util.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

QuestList QuestList::instance{};

QuestList &QuestList::get_instance()
{
    return instance;
}

/*!
 * @brief クエストの初期化
 * @details ソフトウェア起動時ではパース関数が動作しないので、各種初期化シーケンス時に遅延初期化する
 */
void QuestList::initialize()
{
    auto &fixed_maps = QuestFixedMapList::get_instance();
    auto parsed_quests = this->quests;
    auto parsed_maps = fixed_maps.maps;
    QuestType none_quest{};
    none_quest.status = QuestStatusType::UNTAKEN;
    parsed_quests.emplace(QuestId::NONE, none_quest);

    // 全クエスト共通のベース凡例を先に読み込む (各クエスト legend がこれを letter[] 上で上書きする)
    auto parsed_legend = this->load_base_legend();
    this->load_json_quests(path_build(ANGBAND_DIR_EDIT, "quests"), parsed_quests, parsed_maps);

    // 通常の入力エラーをすべて検証してから、NONE・凡例を含む全ストアを公開する。
    // 資源障害まで含む完全な巻き戻しや、プレイ中の再読込は保証しない。
    this->quests.swap(parsed_quests);
    fixed_maps.maps.swap(parsed_maps);
    fixed_maps.base_legend.swap(parsed_legend);
}

/*!
 * @brief lib/edit/QuestPreferences.jsonc の共通ベース凡例 (X/./%/D/< など) を読み込む
 * @details 旧 QuestPreferences.txt の F: 行に相当。各クエストのフロア生成時、個別 legend を
 * letter[] へ上書きする前のベースとして QuestFixedMapList に保持する。
 */
std::map<char, QuestLegendCell> QuestList::load_base_legend()
{
    const auto path = path_build(ANGBAND_DIR_EDIT, "QuestPreferences.jsonc");
    JsoncDocumentLoader loader(path);
    if (!loader.is_open()) {
        constexpr auto fmt = _("ベース凡例ファイルをオープンできません ({})", "Cannot open base legend file ({})");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string()));
    }

    nlohmann::json data;
    try {
        data = loader.parse();
    } catch (const std::exception &e) {
        constexpr auto fmt = _("ベース凡例ファイルの解析に失敗しました ({}): {}", "Failed to parse base legend file ({}): {}");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string(), e.what()));
    }

    const auto legend_it = data.find("legend");
    if ((legend_it == data.end()) || !legend_it->is_object()) {
        constexpr auto fmt = _("ベース凡例ファイルに legend がありません ({})", "Base legend file has no legend ({})");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string()));
    }

    std::map<char, QuestLegendCell> base_legend;
    for (const auto &[symbol, cell_data] : legend_it->items()) {
        // 記号は1文字キー (QuestReader::set_legend と同じ制約)。空キーは symbol.front() が未定義動作、
        // 複数文字キーは先頭文字への暗黙切り詰めになるため弾く。
        if (symbol.size() != 1) {
            constexpr auto fmt = _("ベース凡例のキーが1文字ではありません ({}): '{}'", "Base legend key is not a single character ({}): '{}'");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string(), symbol));
        }
        if (!is_fixed_map_symbol(static_cast<unsigned char>(symbol.front()))) {
            constexpr auto fmt = _("ベース凡例のキーが印字可能なASCIIではありません ({}): コード {}", "Base legend key is not printable ASCII ({}): code {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string(), static_cast<int>(static_cast<unsigned char>(symbol.front()))));
        }

        QuestLegendCell cell;
        if (const auto err = parse_quest_legend_cell(cell_data, cell); err != PARSE_ERROR_NONE) {
            constexpr auto fmt = _("ベース凡例 '{}' にエラーがあります ({}): コード {}", "Error in base legend '{}' ({}): code {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, symbol, path.string(), static_cast<int>(err)));
        }

        base_legend.insert_or_assign(symbol.front(), cell);
    }

    return base_legend;
}

/*!
 * @brief lib/edit/quests/ 以下の各 .jsonc を読み込み、メタデータを QuestType に、
 * 地形レイアウトを QuestFixedMapList に格納する
 * @details
 * 全固定クエストは JSONC で定義される (NONE を除く全エントリをここで作成する)。ディレクトリや
 * ファイルが欠落している場合は不完全なクエスト表のまま進めず、初期化エラーとして送出する
 * (でないと後続の新規ゲーム生成が get_quest() の std::map::at で分かりにくく落ちる)。
 */
void QuestList::load_json_quests(const std::filesystem::path &quests_dir)
{
    auto &fixed_maps = QuestFixedMapList::get_instance();
    auto parsed_quests = this->quests;
    auto parsed_maps = fixed_maps.maps;
    this->load_json_quests(quests_dir, parsed_quests, parsed_maps);
    this->quests.swap(parsed_quests);
    fixed_maps.maps.swap(parsed_maps);
}

void QuestList::load_json_quests(const std::filesystem::path &quests_dir, std::map<QuestId, QuestType> &parsed_quests, std::map<QuestId, QuestFixedMap> &parsed_maps)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(quests_dir, ec)) {
        constexpr auto fmt = _("クエストデータのディレクトリが見つかりません ({})", "Quest data directory not found ({})");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, quests_dir.string()));
    }

    std::vector<std::filesystem::path> files;
    for (const auto &entry : std::filesystem::directory_iterator(quests_dir)) {
        const auto &path = entry.path();
        if (entry.is_regular_file() && (path.extension() == ".jsonc")) {
            files.push_back(path);
        }
    }
    std::stable_sort(files.begin(), files.end());
    if (files.empty()) {
        constexpr auto fmt = _("クエストデータ (*.jsonc) が見つかりません ({})", "No quest data (*.jsonc) found ({})");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, quests_dir.string()));
    }

    std::vector<QuestId> loaded_ids;
    loaded_ids.reserve(files.size());
    for (const auto &file : files) {
        JsoncDocumentLoader loader(file);
        if (!loader.is_open()) {
            constexpr auto fmt = _("クエストファイルをオープンできません ({})", "Cannot open quest file ({})");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string()));
        }

        nlohmann::json quest_data;
        try {
            quest_data = loader.parse();
        } catch (const std::exception &e) {
            constexpr auto fmt = _("クエストファイルの解析に失敗しました ({}): {}", "Failed to parse quest file ({}): {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), e.what()));
        }

        const auto id_it = quest_data.find("id");
        if ((id_it == quest_data.end()) || !id_it->is_number_integer()) {
            constexpr auto fmt = _("クエストファイルに id がありません ({})", "Quest file has no valid id ({})");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string()));
        }

        QuestId quest_id{};
        if (info_set_integer(*id_it, quest_id, true, Range(1, MAX_RANDOM_QUEST)) != PARSE_ERROR_NONE) {
            constexpr auto fmt = _("クエストIDが範囲外です ({}): id {}", "Quest id is out of range ({}): id {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), id_it->dump()));
        }
        // スキーマはファイル横断の一意性を表現できないため、重複 id はここで検出する
        // (見逃すと2つ目のファイルが既存エントリへ追記され、偽のマップバリアント化等の破損を起こす)。
        if (parsed_quests.contains(quest_id)) {
            constexpr auto fmt = _("クエストIDが重複しています ({}): id {}", "Duplicated quest id ({}): id {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), id_it->get<int>()));
        }

        QuestType quest;
        QuestFixedMap fixed_map;
        if (const auto err = QuestReader(quest_data, quest, fixed_map).read(); err != PARSE_ERROR_NONE) {
            constexpr auto fmt = _("クエストファイルにエラーがあります ({}): コード {}", "Error in quest file ({}): code {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), static_cast<int>(err)));
        }

        parsed_quests.emplace(quest_id, std::move(quest));
        parsed_maps.insert_or_assign(quest_id, std::move(fixed_map));
        loaded_ids.push_back(quest_id);
    }

    // apply_quest_metadata は UNIQUE の QUESTOR フラグも変更する。
    // 後半の欠落参照で先行モンスターのフラグが残らないよう、先に全件の参照を確認する。
    auto &monraces = MonraceList::get_instance();
    for (const auto quest_id : loaded_ids) {
        monraces.get_monrace(i2enum<MonraceId>(parsed_maps.at(quest_id).metadata.r_idx));
    }
    for (const auto quest_id : loaded_ids) {
        apply_quest_metadata(parsed_maps.at(quest_id), parsed_quests.at(quest_id));
    }
}

void QuestList::reset_all()
{
    const auto &fixed_maps = QuestFixedMapList::get_instance();
    for (auto &[quest_id, quest] : this->quests) {
        quest.reset();
        if (const auto fixed_map = fixed_maps.find(quest_id); fixed_map) {
            apply_quest_metadata(*fixed_map, quest);
        }
    }
}

QuestType &QuestList::get_quest(QuestId id)
{
    return this->quests.at(id);
}

const QuestType &QuestList::get_quest(QuestId id) const
{
    return this->quests.at(id);
}

std::vector<QuestId> QuestList::get_sorted_quest_ids() const
{
    std::vector<QuestId> quest_ids;
    std::transform(++this->quests.begin(), this->quests.end(), std::back_inserter(quest_ids), [](const auto &x) { return x.first; });
    std::stable_sort(quest_ids.begin(), quest_ids.end(), [this](auto x, auto y) { return this->order_completed(x, y); });
    return quest_ids;
}

/*!
 * @brief 遂行中のランダムクエストのうち、情報を開示できる最も浅い階層のものを返す
 * @return 開示対象のクエストID。該当するものが無ければ nullopt
 * @details 遂行中のランダムクエストを浅い順に絞り込むが、アングバンドの到達階層に届いていない
 * ものと複数体討伐のものは開示しない。「遂行中のクエスト」画面の表示規則そのもの。
 */
tl::optional<QuestId> QuestList::find_shallowest_random_quest_id() const
{
    const auto &dungeon_records = DungeonRecords::get_instance();
    tl::optional<QuestId> quest_id_shallowest;
    auto level_shallowest = 100;
    for (const auto &[quest_id, quest] : this->quests) {
        if ((quest_id == QuestId::NONE) || (quest.type != QuestKindType::RANDOM)) {
            continue;
        }

        const auto is_current = (quest.status == QuestStatusType::TAKEN) || (quest.status == QuestStatusType::COMPLETED);
        if (!is_current || (quest.flags & QUEST_FLAG_SILENT) || (quest.level >= level_shallowest)) {
            continue;
        }

        level_shallowest = quest.level;
        if ((dungeon_records.get_record(DungeonId::ANGBAND).get_max_level() < quest.level) || (quest.max_num > 1)) {
            continue;
        }

        quest_id_shallowest = quest_id;
    }

    return quest_id_shallowest;
}

void QuestList::set_defeated_monster(QuestId id, short numbers)
{
    this->quests.at(id).cur_num = numbers;
}

void QuestList::set_max_monster(QuestId id, short numbers)
{
    this->quests.at(id).max_num = numbers;
}

void QuestList::set_type(QuestId id, QuestKindType type)
{
    this->quests.at(id).type = type;
}

void QuestList::set_monrace_id(QuestId id, MonraceId monrace_id)
{
    this->quests.at(id).r_idx = monrace_id;
}

void QuestList::set_flags(QuestId id, uint32_t flags)
{
    this->quests.at(id).flags = flags;
}

void QuestList::set_reward(QuestId id, FixedArtifactId fa_id)
{
    this->quests.at(id).set_reward(fa_id);
}

void QuestList::reset_reward(QuestId id)
{
    this->quests.at(id).reset_reward();
}

bool QuestList::is_quest_equals(QuestId id, QuestKindType type) const
{
    return this->quests.at(id).type == type;
}

bool QuestList::is_bounty_valid(QuestId id) const
{
    return this->quests.at(id).get_bounty().is_valid();
}

bool QuestList::order_completed(QuestId id1, QuestId id2) const
{
    const auto &quest1 = this->get_quest(id1);
    const auto &quest2 = this->get_quest(id2);
    return (quest1.comptime != quest2.comptime) ? (quest1.comptime < quest2.comptime) : (quest1.level < quest2.level);
}
