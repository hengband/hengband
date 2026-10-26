#include "system/dungeon/quest-list.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/jsonc-document-loader.h"
#include "info-reader/legend-ego-validator.h"
#include "info-reader/quest-reader.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/files-util.h"
#include "system/angband-exceptions.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-conversion-type.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <iterator>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
void require_quest_reference(bool valid, const std::filesystem::path &file, std::string_view field, int id)
{
    if (!valid) {
        constexpr auto fmt = _("クエストの参照が未定義です ({}): {} {}", "Undefined quest reference ({}): {} {}");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), field, id));
    }
}

/*!
 * @brief 地図のすべての記号が、クエストの凡例か共通凡例で定義されているか確認する
 * @details 生成時はどちらにも無い記号の凡例を上書きしないため、前に生成した町やクエストの凡例がそのまま使われる。
 * 使われない地図の候補も、どれが選ばれるかはキャラクタごとに決まるため確認する。
 */
void require_defined_map_symbols(const QuestFixedMap &fixed_map, const std::map<char, QuestLegendCell> &base_legend, const std::filesystem::path &file)
{
    for (const auto &rows : fixed_map.maps) {
        for (const auto &row : rows) {
            for (const auto symbol : row) {
                if (fixed_map.legend.contains(symbol) || base_legend.contains(symbol)) {
                    continue;
                }
                constexpr auto fmt = _("クエストの地図に未定義の記号があります ({}): '{}'", "Undefined symbol in quest map ({}): '{}'");
                THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), symbol));
            }
        }
    }
}

/*!
 * @brief 配置時の地形変換の全候補を、乱数とダンジョン状態を変更せずに列挙する
 * @details Quest進入時は現在のダンジョンIDを引き継ぐため、Questのdungeonだけでは絞らない。
 * convert_terrain_id()と同じく一段の変換とし、正の重みの床・壁候補をすべて照合する。
 */
std::set<short> quest_converted_terrain_candidates(short terrain_id, const DungeonDefinition *dungeon)
{
    const auto &terrain = TerrainList::get_instance().get_terrain(terrain_id);
    if (terrain.flags.has_not(TerrainCharacteristics::CONVERT)) {
        return { terrain_id };
    }
    if (!dungeon) {
        return {};
    }

    std::set<short> candidates;
    switch (terrain.conversion_type) {
    case TerrainConversionType::FLOOR:
        for (const auto &[feature, weight] : dungeon->prob_table_floor.entries()) {
            static_cast<void>(weight);
            candidates.insert(feature);
        }
        break;
    case TerrainConversionType::WALL:
        for (const auto &[feature, weight] : dungeon->prob_table_wall.entries()) {
            static_cast<void>(weight);
            candidates.insert(feature);
        }
        break;
    case TerrainConversionType::INNER:
        candidates.insert(dungeon->inner_wall);
        break;
    case TerrainConversionType::OUTER:
    case TerrainConversionType::SOLID:
        candidates.insert(dungeon->outer_wall);
        break;
    case TerrainConversionType::STREAM:
        candidates.insert(dungeon->select_stream_terrain_id(terrain_id, terrain.stream_index));
        break;
    default:
        candidates.insert(terrain_id);
        break;
    }
    return candidates;
}

/*!
 * @brief 配置時の一段のCONVERTと、その後の一段の生成時変化の全候補を列挙する
 * @details generate_floor()の固定Quest生成後にも生成時変化が適用される。
 * 確率合計が100未満の場合だけ元地形が残り、変化先の再変換は行わない。
 */
std::set<short> quest_special_terrain_candidates(short terrain_id, const DungeonDefinition *dungeon)
{
    std::set<short> candidates;
    for (const auto feature : quest_converted_terrain_candidates(terrain_id, dungeon)) {
        auto remaining_probability = 100;
        const auto &terrain = TerrainList::get_instance().get_terrain(feature);
        for (const auto &change : terrain.generation_changes) {
            if (remaining_probability == 0) {
                break;
            }
            if (change.probability <= 0) {
                continue;
            }
            candidates.insert(change.result);
            remaining_probability -= std::min(remaining_probability, change.probability);
        }
        if (remaining_probability > 0) {
            candidates.insert(feature);
        }
    }
    return candidates;
}

/*!
 * @brief 宣言された凡例の固定参照を、共通凡例も含めて全ストア公開前に照合する
 * @details ランダム生成の深さ補正と受託時の報酬指定は固定IDではない。
 * 負のmonsterはクローンとして絶対値のIDを照合する。負のobject/artifactには
 * 特殊な配置処理がなく、未定義の固定IDとして拒否する。
 * specialは宣言地形ではなく配置後の地形で解釈されるため、CONVERT・生成時変化・
 * 罠の下地・発見後の表示まで照合する。配置側の処理を変更するときは対応テストも更新する。
 */
void validate_quest_legend_references(const std::map<char, QuestLegendCell> &legend, const std::filesystem::path &file,
    const std::map<QuestId, QuestType> &quests, bool fixed_quest, bool surface_quest)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &baseitems = BaseitemList::get_instance();
    const auto &artifacts = ArtifactList::get_instance();
    const auto &dungeons = DungeonList::get_instance();
    const auto &terrains = TerrainList::get_instance();
    std::vector<const DungeonDefinition *> contexts;
    for (const auto &[id, dungeon] : dungeons) {
        static_cast<void>(id);
        contexts.push_back(dungeon.get());
    }
    if (contexts.empty()) {
        contexts.push_back(nullptr);
    }
    for (const auto &[symbol, cell] : legend) {
        const auto &grid = cell.grid;
        const auto prefix = fmt::format("legend['{}']", symbol);
        const auto require_reference = [&file, &prefix](bool valid, std::string_view field, int id) {
            require_quest_reference(valid, file, fmt::format("{}.{}", prefix, field), id);
        };
        if (!(grid.random & RANDOM_MONSTER) && grid.monster != 0) {
            const auto monster_id = grid.monster < 0 ? -static_cast<int>(grid.monster) : grid.monster;
            require_reference(monraces.contains(i2enum<MonraceId>(monster_id)), "monster", grid.monster);
        }
        if (!(grid.random & RANDOM_OBJECT) && !cell.object_is_quest_reward && grid.object != 0) {
            require_reference(baseitems.is_valid(grid.object), "object", grid.object);
        }
        const auto ego_result = validate_legend_ego(grid, cell.object_is_quest_reward, grid.trap != 0 || (grid.random & RANDOM_TRAP));
        if (!ego_result) {
            constexpr auto fmt = _("クエストのエゴ指定が不正です ({}): {}.ego {} (object {}): {}",
                "Invalid quest ego ({}): {}.ego {} (object {}): {}");
            const auto ego = (grid.random & RANDOM_EGO) ? std::string("random") : std::to_string(enum2i(grid.ego));
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), prefix, ego, grid.object, describe_legend_ego_error(ego_result.error())));
        }
        // 配置側はRANDOM_ARTIFACTでもidを固定Artifactとして参照する。
        if (!cell.artifact_is_quest_reward && grid.artifact != FixedArtifactId::NONE) {
            require_reference(artifacts.contains(grid.artifact), "artifact", enum2i(grid.artifact));
        }
        if (grid.special == 0) {
            continue;
        }

        const auto require_quest = [&] {
            const auto valid = grid.special > 0 && grid.special <= MAX_RANDOM_QUEST && quests.contains(i2enum<QuestId>(grid.special));
            require_reference(valid, "special (QuestId)", grid.special);
        };
        const auto require_dungeon = [&] {
            require_reference(dungeons.contains(i2enum<DungeonId>(grid.special)), "special (DungeonId)", grid.special);
        };
        // 表示は一段のTerrainType::mimicを使い、QUEST_ENTER・BLDG・ENTRANCEの順に分岐する。
        const auto validate_description = [&](short feature) {
            const auto &flags = terrains.get_terrain(terrains.get_terrain(feature).mimic).flags;
            if (flags.has(TerrainCharacteristics::QUEST_ENTER)) {
                require_quest();
            } else if (flags.has(TerrainCharacteristics::ENTRANCE) && flags.has_not(TerrainCharacteristics::BLDG)) {
                require_dungeon();
            }
        };
        const auto fixed_trap = grid.trap != 0 && !(grid.random & (RANDOM_OBJECT | RANDOM_TRAP));
        for (const auto *dungeon : contexts) {
            const auto physical = quest_special_terrain_candidates(fixed_trap ? grid.trap : grid.feature, dungeon);
            for (const auto feature : physical) {
                const auto &terrain = terrains.get_terrain(feature);
                const auto &flags = terrain.flags;
                // 移動時の出口処理は、店舗・施設・Quest入口の後に実行される。
                const auto movement_exit = flags.has(TerrainCharacteristics::QUEST_EXIT) && flags.has_none_of({ TerrainCharacteristics::STORE, TerrainCharacteristics::BLDG, TerrainCharacteristics::QUEST_ENTER });
                const auto quest_stairs = flags.has(TerrainCharacteristics::QUEST) && flags.has_any_of({ TerrainCharacteristics::UP_STAIRS, TerrainCharacteristics::DOWN_STAIRS });
                if (flags.has(TerrainCharacteristics::QUEST_ENTER) || movement_exit || quest_stairs || (fixed_quest && flags.has(TerrainCharacteristics::UP_STAIRS))) {
                    require_quest();
                }
                // 地上で下りを使う場合のみDungeonへ入る。QUEST_ENTERとQUESTは先に処理される。
                if (surface_quest && flags.has_all_of({ TerrainCharacteristics::ENTRANCE, TerrainCharacteristics::DOWN_STAIRS }) && flags.has_none_of({ TerrainCharacteristics::QUEST_ENTER, TerrainCharacteristics::QUEST })) {
                    require_dungeon();
                }
                // 発見でfeatを変えずmimicだけ解除できる固定罠・隠し扉は、自身の表示も照合する。
                // SECRETは地形変化の別経路で、ここでは元featのmimic解除とは扱わない。
                const auto detected = flags.has_any_of({ TerrainCharacteristics::TRAP, TerrainCharacteristics::DOOR, TerrainCharacteristics::STAIRS, TerrainCharacteristics::HAS_GOLD });
                const auto hit_trap = flags.has(TerrainCharacteristics::HIT_TRAP) && flags.has_none_of({ TerrainCharacteristics::STORE, TerrainCharacteristics::BLDG, TerrainCharacteristics::QUEST_ENTER, TerrainCharacteristics::QUEST_EXIT });
                const auto can_disclose = flags.has_not(TerrainCharacteristics::SECRET) && (detected || hit_trap || terrain.is_closed_door());
                if (!fixed_trap || can_disclose) {
                    validate_description(feature);
                }
            }
            if (!fixed_trap && !(grid.random & RANDOM_TRAP)) {
                continue;
            }
            // 罠の下にはCONVERT後の元地形がmimicとして残り、生成時変化は掛からない。
            // 同じDungeon内で候補を組み合わせ、0はGrid::mimicの未指定として実地形へ戻す。
            for (const auto feature : quest_converted_terrain_candidates(grid.feature, dungeon)) {
                const auto &flags = terrains.get_terrain(feature).flags;
                if (!fixed_trap && (flags.has_not(TerrainCharacteristics::FLOOR) || (grid.cave_info & CAVE_OBJECT) != 0)) {
                    continue;
                }
                if (feature != 0) {
                    validate_description(feature);
                } else {
                    for (const auto fallback : physical) {
                        validate_description(fallback);
                    }
                }
            }
        }
    }
}
}

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
    this->load_json_quests(path_build(ANGBAND_DIR_EDIT, "quests"), parsed_quests, parsed_maps, parsed_legend);

    // 通常の入力エラーをすべて検証してから、NONE・凡例を含む全ストアを公開する。
    // 資源障害まで含む完全な巻き戻しや、プレイ中の再読込は保証しない。
    this->publish(parsed_quests, parsed_maps);
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
        std::string cell_reason;
        if (const auto result = parse_quest_legend_cell(cell_data, cell, cell_reason); !result) {
            constexpr auto fmt = _("ベース凡例 '{}' にエラーがあります ({}): コード {}", "Error in base legend '{}' ({}): code {}");
            const auto reason = cell_reason.empty() ? std::string() : fmt::format(": {}", cell_reason);
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, symbol, path.string(), static_cast<int>(result.error())) + reason);
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
    this->load_json_quests(quests_dir, parsed_quests, parsed_maps, fixed_maps.get_base_legend());
    this->publish(parsed_quests, parsed_maps);
}

/*!
 * @brief 検証済みのクエストと固定マップを、例外を送出せずに一括公開する
 */
void QuestList::publish(std::map<QuestId, QuestType> &parsed_quests, std::map<QuestId, QuestFixedMap> &parsed_maps) noexcept
{
    this->quests.swap(parsed_quests);
    QuestFixedMapList::get_instance().maps.swap(parsed_maps);
}

void QuestList::load_json_quests(const std::filesystem::path &quests_dir, std::map<QuestId, QuestType> &parsed_quests, std::map<QuestId, QuestFixedMap> &parsed_maps, const std::map<char, QuestLegendCell> &base_legend)
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

    std::vector<std::pair<QuestId, std::filesystem::path>> loaded_quests;
    loaded_quests.reserve(files.size());
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
        if (!info_set_integer(*id_it, quest_id, true, Range(1, MAX_RANDOM_QUEST))) {
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
        std::string reason;
        if (const auto result = QuestReader(quest_data, quest, fixed_map).read(&reason); !result) {
            constexpr auto fmt = _("クエストファイルにエラーがあります ({}): コード {}", "Error in quest file ({}): code {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), static_cast<int>(result.error())) + (reason.empty() ? std::string() : ": " + reason));
        }

        parsed_quests.emplace(quest_id, std::move(quest));
        parsed_maps.insert_or_assign(quest_id, std::move(fixed_map));
        loaded_quests.emplace_back(quest_id, file);
    }

    // apply_quest_metadata は UNIQUE の QUESTOR フラグも変更する。
    // 後半の欠落参照で先行モンスターのフラグが残らないよう、先に全件の参照を確認する。
    auto &monraces = MonraceList::get_instance();
    for (const auto &[quest_id, file] : loaded_quests) {
        const auto monster_id = parsed_maps.at(quest_id).metadata.r_idx;
        if (!monraces.contains(i2enum<MonraceId>(monster_id))) {
            constexpr auto fmt = _("クエストの対象モンスターが未定義です ({}): monster {}", "Undefined quest target monster ({}): monster {}");
            THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file.string(), monster_id));
        }
        require_defined_map_symbols(parsed_maps.at(quest_id), base_legend, file);
    }
    // 全ファイルの解析後に照合し、ファイル順に依存する前方参照の拒否を避ける。
    // 0 は「指定なし」または通常フロアからの退出を表すため、定義表を要求しない。
    const auto &dungeons = DungeonList::get_instance();
    const auto &artifacts = ArtifactList::get_instance();
    for (const auto &[quest_id, file] : loaded_quests) {
        const auto &fixed_map = parsed_maps.at(quest_id);
        const auto require_reference = [&file](bool valid, std::string_view field, int id) {
            require_quest_reference(valid, file, field, id);
        };
        const auto &meta = fixed_map.metadata;
        require_reference(meta.dungeon == 0 || dungeons.contains(i2enum<DungeonId>(meta.dungeon)), "definition.dungeon", meta.dungeon);
        require_reference(meta.reward_artifact == 0 || artifacts.contains(i2enum<FixedArtifactId>(meta.reward_artifact)),
            "definition.reward.artifact", meta.reward_artifact);
        for (size_t i = 0; i < fixed_map.reward_artifact_candidates.size(); ++i) {
            const auto candidate = fixed_map.reward_artifact_candidates[i];
            require_reference(candidate == 0 || artifacts.contains(i2enum<FixedArtifactId>(candidate)),
                // readerは整数でない候補を解析エラーにするため、候補の添字は元のJSON配列の添字と一致する。
                fmt::format("definition.reward.artifacts[{}]", i), candidate);
        }
        for (size_t i = 0; i < fixed_map.starts.size(); ++i) {
            const auto leaving_id = fixed_map.starts[i].leaving_quest;
            if (!leaving_id || *leaving_id == 0) {
                continue;
            }
            // leavingQuest は int で読み込む。QuestId(short) への変換前に範囲を確認する。
            const auto valid = *leaving_id > 0 && *leaving_id <= MAX_RANDOM_QUEST && parsed_quests.contains(i2enum<QuestId>(*leaving_id));
            require_reference(valid, fmt::format("startVariants[{}].leavingQuest", i), *leaving_id);
        }
    }
    // 共通凡例はQuestごとに上書きされるが、使われない宣言も固定参照の検証対象とする。
    // 共通凡例のspecialも全Questが揃ってから調べ、失敗時にQUESTORを残さない。
    const auto base_legend_file = path_build(ANGBAND_DIR_EDIT, "QuestPreferences.jsonc");
    std::set<std::pair<bool, bool>> validated_base_contexts;
    for (const auto &[quest_id, file] : loaded_quests) {
        const auto &fixed_map = parsed_maps.at(quest_id);
        // 候補報酬が尽きるとRANDOMもKILL_ALLになり、通常の上り階段がQuest参照になる。
        const auto fixed_quest = fixed_map.metadata.type != enum2i(QuestKindType::RANDOM) || !fixed_map.reward_artifact_candidates.empty();
        const auto surface_quest = fixed_map.metadata.level <= 0;
        if (validated_base_contexts.emplace(fixed_quest, surface_quest).second) {
            validate_quest_legend_references(base_legend, base_legend_file, parsed_quests, fixed_quest, surface_quest);
        }
        validate_quest_legend_references(fixed_map.legend, file, parsed_quests, fixed_quest, surface_quest);
    }
    for (const auto &loaded_quest : loaded_quests) {
        const auto quest_id = loaded_quest.first;
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
