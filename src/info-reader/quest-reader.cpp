#include "info-reader/quest-reader.h"
#include "artifact/fixed-art-types.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/character-encoding.h"
#include "object-enchant/object-ego.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

const Range LEGEND_SHORT_RANGE(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
// 負のモンスターIDは生成時にshortのまま符号反転されるので、最小値は許可しない。
const Range LEGEND_MONSTER_RANGE(-std::numeric_limits<MONSTER_IDX>::max(), std::numeric_limits<MONSTER_IDX>::max());
const Range LEGEND_EGO_RANGE(1, MAX_EGO_ID);

namespace {
constexpr Range QUEST_INTEGER_RANGE(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
// メタデータはintで保持するが、対象モンスターと報酬は実行時にshort基底のIDへ変換される。
// どちらも 0 は指定なしで、ほかの定義と同じくIDの型から決めた上限までとする。
constexpr Range QUEST_MONSTER_RANGE(0, MAX_MONRACE_ID);
constexpr Range QUEST_ARTIFACT_RANGE(0, MAX_FIXED_ARTIFACT_ID);
// 階層と撃破数はセーブで16ビットになる。階層はダンジョンの最深階 (MAX_DEPTH - 1) までしか移動できない。
constexpr Range QUEST_LEVEL_RANGE(0, MAX_DEPTH - 1);
constexpr Range QUEST_MAX_NUM_RANGE(0, std::numeric_limits<int16_t>::max());
// ダンジョンIDは DungeonId の範囲 (0 は指定なし)、離脱するクエストのIDは QuestId の型から決めた上限までとする。
constexpr Range QUEST_DUNGEON_RANGE(0, enum2i(DungeonId::MAX) - 1);
constexpr Range QUEST_LEAVING_QUEST_RANGE(0, MAX_QUEST_ID);

const std::unordered_map<std::string_view, QuestKindType> QUEST_KIND_TOKENS = {
    { "NONE", QuestKindType::NONE },
    { "KILL_LEVEL", QuestKindType::KILL_LEVEL },
    { "KILL_ANY_LEVEL", QuestKindType::KILL_ANY_LEVEL },
    { "FIND_ARTIFACT", QuestKindType::FIND_ARTIFACT },
    { "FIND_EXIT", QuestKindType::FIND_EXIT },
    { "KILL_NUMBER", QuestKindType::KILL_NUMBER },
    { "KILL_ALL", QuestKindType::KILL_ALL },
    { "RANDOM", QuestKindType::RANDOM },
    { "TOWER", QuestKindType::TOWER },
};

const std::unordered_map<std::string_view, QuestStatusType> QUEST_STATUS_TOKENS = {
    { "UNTAKEN", QuestStatusType::UNTAKEN },
    { "TAKEN", QuestStatusType::TAKEN },
    { "COMPLETED", QuestStatusType::COMPLETED },
    { "REWARDED", QuestStatusType::REWARDED },
    { "FINISHED", QuestStatusType::FINISHED },
    { "FAILED", QuestStatusType::FAILED },
    { "FAILED_DONE", QuestStatusType::FAILED_DONE },
    { "STAGE_COMPLETED", QuestStatusType::STAGE_COMPLETED },
};

const std::unordered_map<std::string_view, uint32_t> QUEST_FLAG_TOKENS = {
    { "SILENT", QUEST_FLAG_SILENT },
    { "PRESET", QUEST_FLAG_PRESET },
    { "ONCE", QUEST_FLAG_ONCE },
    { "TOWER", QUEST_FLAG_TOWER },
};

const std::unordered_map<std::string_view, BIT_FLAGS> CAVE_FLAG_TOKENS = {
    { "MARK", CAVE_MARK },
    { "GLOW", CAVE_GLOW },
    { "NO_TELEPORT_DEST", CAVE_NO_TELEPORT_DEST },
    { "ROOM", CAVE_ROOM },
    { "LITE", CAVE_LITE },
};

/*!
 * @brief JSONの文字列配列をフロアに収まるマップ行として取り込む
 * @details マップ行のようなASCIIの記号列は文字コードの変換が不要なので、そのまま取り込む
 * @return 成功時は空の値、失敗時は解析エラーコード（行数または行のバイト数がフロアの上限を超える場合は
 * ParseErrorType::OUT_OF_BOUNDS、非文字列行は ParseErrorType::INVALID_TYPE、行幅が揃っていない場合や
 * 印字可能なASCII以外のセルがある場合は ParseErrorType::INVALID_VALUE）
 */
tl::expected<void, ParseErrorType> read_map_rows(const nlohmann::json &array_data, std::vector<std::string> &out)
{
    for (const auto &line : array_data) {
        if (!line.is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }

        const auto &row = line.get_ref<const std::string &>();
        FixedMapRowLimits limits{ MAX_HGT, MAX_WID };
        if (!out.empty()) {
            limits.expected_width = out.front().size();
        }
        if (const auto result = validate_fixed_map_row(row, out.size(), limits); !result) {
            return result;
        }
        out.push_back(row);
    }

    return {};
}

/*!
 * @brief 条件に関係なく、開始位置がすべてのマップ候補の範囲内か確認する
 * @return 成功時は空の値、失敗時は解析エラーコード（マップ候補が無い場合や、開始位置がいずれかのマップ候補の
 * 範囲外の場合は ParseErrorType::OUT_OF_BOUNDS）
 */
tl::expected<void, ParseErrorType> validate_quest_start(const QuestFixedMap &parsed, const QuestStartPosition &position)
{
    if (parsed.maps.empty() || position.y < 0 || position.x < 0) {
        return tl::make_unexpected(ParseErrorType::OUT_OF_BOUNDS);
    }
    for (const auto &rows : parsed.maps) {
        if (rows.empty() || static_cast<std::size_t>(position.y) >= rows.size() || static_cast<std::size_t>(position.x) >= rows.front().size()) {
            return tl::make_unexpected(ParseErrorType::OUT_OF_BOUNDS);
        }
    }
    return {};
}

/*!
 * @brief JSONの文字列配列を、各行を UTF-8 からシステムの文字コードへ変換して取り込む (説明文用、null/欠落は空)
 * @return 成功時は空の値、型不一致はINVALID_TYPE、文字コード変換の失敗はINVALID_VALUE
 */
tl::expected<void, ParseErrorType> read_localized_lines(const nlohmann::json &array_data, std::vector<std::string> &out)
{
    if (array_data.is_null()) {
        return {};
    }
    if (!array_data.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (const auto &line : array_data) {
        if (!line.is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }

        auto line_sys = utf8_to_sys(line.get_ref<const std::string &>());
        if (!line_sys) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        out.push_back(std::move(*line_sys));
    }

    return {};
}

/*!
 * @brief 地形の解決方法を分け、凡例セルの共通項目を同じ順序で解析する
 * @param cell_data 凡例セルの定義
 * @param out 変換した凡例セルの出力先
 * @param deferred 遅延解決時のタグ出力先。nullptrなら既存どおり即時解決する
 * @param reason 配置衝突で失敗した場合の理由の出力先
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> parse_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out, DeferredQuestLegendCell *deferred, std::string *reason = nullptr)
{
    if (!cell_data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    BIT_FLAGS cave_flags = 0;
    const auto &cave_info = get_json_value(cell_data, "caveInfo");
    if (!cave_info.is_null()) {
        if (!cave_info.is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        for (const auto &flag : cave_info) {
            if (!flag.is_string()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
            // null の要素も INVALID_TYPE とするため、文字列かどうかは info_read_token() の前に確かめる
            const auto cave_flag = info_read_token(flag, CAVE_FLAG_TOKENS);
            if (!cave_flag) {
                return tl::make_unexpected(cave_flag.error());
            }
            cave_flags |= *cave_flag;
        }
    }

    auto &grid = out.grid;
    if (deferred != nullptr) {
        grid.feature = 0;
    } else {
        grid.set_terrain_id(TerrainTag::NONE);
    }
    grid.monster = 0;
    grid.object = 0;
    grid.ego = EgoType::NONE;
    grid.artifact = FixedArtifactId::NONE;
    if (deferred != nullptr) {
        grid.trap = 0;
    } else {
        grid.set_trap_id(TerrainTag::NONE);
    }
    grid.cave_info = cave_flags;
    grid.special = 0;
    grid.random = RANDOM_NONE;
    out.object_is_quest_reward = false;
    out.artifact_is_quest_reward = false;

    const auto &terrain = get_json_value(cell_data, "terrain");
    if (terrain.is_string()) {
        const auto tag = terrain.get<std::string>();
        if (deferred != nullptr) {
            deferred->terrain_tag = tag;
        } else if (const auto result = resolve_legend_terrain(tag, grid.feature, grid.random, RANDOM_FEATURE); !result) {
            return result;
        }
    } else if (!terrain.is_null()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    // 整数・オブジェクト以外の値（5.0 などの整数でない数を含む）は黙って無視せず解析エラーにする。
    const auto &monster = get_json_value(cell_data, "monster");
    if (monster.is_number()) {
        if (const auto result = info_set_integer(monster, grid.monster, true, LEGEND_MONSTER_RANGE); !result) {
            return result;
        }
    } else if (monster.is_object()) {
        auto random = false;
        if (const auto result = info_set_bool(get_json_value(monster, "random"), random, false); !result) {
            return result;
        }
        if (random) {
            grid.random |= RANDOM_MONSTER;
            if (const auto result = info_set_integer(get_json_value(monster, "oodLevel"), grid.monster, false, LEGEND_SHORT_RANGE); !result) {
                return result;
            }
        } else {
            const auto &clone = get_json_value(monster, "cloneOf");
            if (!clone.is_null()) {
                int clone_id = 0;
                if (const auto result = info_set_integer(clone, clone_id, true, LEGEND_MONSTER_RANGE); !result) {
                    return result;
                }
                grid.monster = static_cast<MONSTER_IDX>(-clone_id);
            }
        }
    } else if (!monster.is_null()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto &object = get_json_value(cell_data, "object");
    if (object.is_number()) {
        if (const auto result = info_set_integer(object, grid.object, true, LEGEND_SHORT_RANGE); !result) {
            return result;
        }
    } else if (object.is_object()) {
        auto random = false;
        if (const auto result = info_set_bool(get_json_value(object, "random"), random, false); !result) {
            return result;
        }
        auto quest_reward = false;
        if (const auto result = info_set_bool(get_json_value(object, "questReward"), quest_reward, false); !result) {
            return result;
        }
        if (random) {
            grid.random |= RANDOM_OBJECT;
            if (const auto result = info_set_integer(get_json_value(object, "oodLevel"), grid.object, false, LEGEND_SHORT_RANGE); !result) {
                return result;
            }
        } else if (quest_reward) {
            out.object_is_quest_reward = true;
        }
    } else if (!object.is_null()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto &ego = get_json_value(cell_data, "ego");
    if (ego.is_number()) {
        if (const auto result = info_set_integer(ego, grid.ego, true, LEGEND_EGO_RANGE); !result) {
            return result;
        }
    } else if (ego.is_object()) {
        if (ego.contains("id")) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        auto random = false;
        if (const auto result = info_set_bool(get_json_value(ego, "random"), random, false); !result) {
            return result;
        }
        if (random) {
            grid.random |= RANDOM_EGO;
        }
    } else if (!ego.is_null()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto &artifact = get_json_value(cell_data, "artifact");
    if (artifact.is_number()) {
        if (const auto result = info_set_integer(artifact, grid.artifact, true, LEGEND_SHORT_RANGE); !result) {
            return result;
        }
    } else if (artifact.is_object()) {
        auto random = false;
        if (const auto result = info_set_bool(get_json_value(artifact, "random"), random, false); !result) {
            return result;
        }
        auto quest_reward = false;
        if (const auto result = info_set_bool(get_json_value(artifact, "questReward"), quest_reward, false); !result) {
            return result;
        }
        if (random) {
            grid.random |= RANDOM_ARTIFACT;
            if (const auto result = info_set_integer(get_json_value(artifact, "id"), grid.artifact, false, LEGEND_SHORT_RANGE); !result) {
                return result;
            }
        } else if (quest_reward) {
            out.artifact_is_quest_reward = true;
        }
    } else if (!artifact.is_null()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto &trap = get_json_value(cell_data, "trap");
    if (trap.is_string()) {
        if (deferred != nullptr) {
            deferred->trap_tag = trap.get<std::string>();
        } else if (const auto result = resolve_legend_terrain(trap.get_ref<const std::string &>(), grid.trap, grid.random, RANDOM_NONE); !result) {
            return result;
        }
    } else if (trap.is_object()) {
        auto random = false;
        if (const auto result = info_set_bool(get_json_value(trap, "random"), random, false); !result) {
            return result;
        }
        if (random) {
            if (deferred != nullptr) {
                deferred->trap_tag = "*";
            } else {
                grid.random |= RANDOM_TRAP;
            }
        }
    } else if (!trap.is_null()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    if (const auto result = info_set_integer(get_json_value(cell_data, "special"), grid.special, false, LEGEND_SHORT_RANGE); !result) {
        return result;
    }

    const auto random_trap = deferred != nullptr ? deferred->trap_tag == "*" : (grid.random & RANDOM_TRAP) != 0;
    const auto fixed_trap = deferred != nullptr ? deferred->trap_tag != "NONE" && !random_trap : grid.trap != 0 && !random_trap;
    if (has_legend_object_trap_conflict(grid, out.object_is_quest_reward, fixed_trap, random_trap)) {
        if (reason != nullptr) {
            *reason = describe_legend_object_trap_conflict();
        }
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    return {};
}

}

/*!
 * @brief 共通凡例パーサが受理するcaveInfoの名前かを調べる
 * @param name 検査するフラグ名
 * @return 登録済みのフラグ名ならtrue
 */
bool is_legend_cave_flag(std::string_view name)
{
    return CAVE_FLAG_TOKENS.contains(name);
}

/*!
 * @brief JSONC の gridDefinition 1件を地形・罠の即時解決付きで凡例セルへ変換する
 * @param cell_data 凡例セルの定義
 * @param out 変換した凡例セルの出力先
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details set_legend、QuestPreferences、TownPreferencesの読み込みから使う。
 */
tl::expected<void, ParseErrorType> parse_quest_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out)
{
    return parse_legend_cell(cell_data, out, nullptr);
}

/*! @brief 凡例セルを解析し、配置衝突で失敗した場合だけ理由を返す。 */
tl::expected<void, ParseErrorType> parse_quest_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out, std::string &reason)
{
    reason.clear();
    return parse_legend_cell(cell_data, out, nullptr, &reason);
}

/*!
 * @brief 地形・罠を解決せず、タグ文字列とそれ以外の凡例セルの値を解析する
 * @param cell_data 凡例セルの定義
 * @param out 成功時だけ置換する出力先。失敗時は変更しない
 * @return 成功時は空の値、失敗時は即時解決版と共通の解析エラーコード
 * @details TerrainListにはNONEも含めてアクセスしない。省略・nullのタグはNONE、構造化ランダム罠は*にする。
 * feature/trapの数値欄は0、RANDOM_FEATURE/RANDOM_TRAPは未設定とし、他の4ランダムビットは保持する。
 * タグの存在確認や町固有の制約の検証は行わない。
 */
tl::expected<void, ParseErrorType> parse_quest_legend_cell_deferred(const nlohmann::json &cell_data, DeferredQuestLegendCell &out)
{
    std::string reason;
    return parse_quest_legend_cell_deferred(cell_data, out, reason);
}

/*! @brief 遅延解析で配置衝突の理由を返し、成功時だけ出力を置き換える。 */
tl::expected<void, ParseErrorType> parse_quest_legend_cell_deferred(const nlohmann::json &cell_data, DeferredQuestLegendCell &out, std::string &reason)
{
    reason.clear();
    DeferredQuestLegendCell parsed;
    if (const auto result = parse_legend_cell(cell_data, parsed.cell, &parsed, &reason); !result) {
        return result;
    }
    out = std::move(parsed);
    return {};
}

QuestReader::QuestReader(const nlohmann::json &quest_data, QuestType &quest, QuestFixedMap &fixed_map)
    : quest_data(quest_data)
    , quest(quest)
    , fixed_map(fixed_map)
{
}

/*!
 * @brief クエスト定義(JSON Object)のパース関数
 * @param reason 凡例の配置衝突がある場合に理由を返す省略可能な出力先
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details 全件成功時のみ名前と固定マップを置き換える。失敗・例外時は既存の出力を保持する。
 */
tl::expected<void, ParseErrorType> QuestReader::read(std::string *reason) const
{
    if (reason != nullptr) {
        reason->clear();
    }
    if (!this->quest_data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    std::string name;
    QuestFixedMap parsed;
    if (const auto result = this->set_name(name); !result) {
        return result;
    }
    if (const auto result = this->set_definition(parsed); !result) {
        return result;
    }
    if (const auto result = this->set_descriptions(parsed); !result) {
        return result;
    }
    if (const auto result = this->set_legend(parsed, reason); !result) {
        return result;
    }
    if (const auto result = this->set_maps(parsed); !result) {
        return result;
    }
    if (const auto result = this->set_starts(parsed); !result) {
        return result;
    }

    this->quest.name = std::move(name);
    this->fixed_map = std::move(parsed);
    return {};
}

tl::expected<void, ParseErrorType> QuestReader::set_name(std::string &name) const
{
    // info_set_string は {ja, en} オブジェクトを受け取り、ビルド言語に応じた文字列を格納する
    if (const auto result = info_set_string(get_json_value(this->quest_data, "name"), name, true); !result) {
        return result;
    }
    return {};
}

tl::expected<void, ParseErrorType> QuestReader::set_definition(QuestFixedMap &parsed) const
{
    const auto &definition = get_json_value(this->quest_data, "definition");
    if (!definition.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    auto &meta = parsed.metadata;
    meta.present = true;

    const auto &type = get_json_value(definition, "type");
    const auto kind = info_read_token(type, QUEST_KIND_TOKENS);
    if (!kind) {
        return tl::make_unexpected(kind.error());
    }
    meta.type = enum2i(*kind);

    if (const auto result = info_set_integer(get_json_value(definition, "level"), meta.level, true, QUEST_LEVEL_RANGE); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(definition, "numMon"), meta.num_mon, false, QUEST_INTEGER_RANGE); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(definition, "maxNum"), meta.max_num, false, QUEST_MAX_NUM_RANGE); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(definition, "dungeon"), meta.dungeon, false, QUEST_DUNGEON_RANGE); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(definition, "monster"), meta.r_idx, false, QUEST_MONSTER_RANGE); !result) {
        return result;
    }

    const auto &flags = get_json_value(definition, "flags");
    if (!flags.is_null()) {
        const auto grab_flag = [&meta](const auto &flag) {
            const auto it = QUEST_FLAG_TOKENS.find(flag.template get_ref<const std::string &>());
            if (it == QUEST_FLAG_TOKENS.end()) {
                return false;
            }
            meta.flags |= it->second;
            return true;
        };
        if (const auto result = info_read_flag_array(flags, grab_flag); !result) {
            return result;
        }
    }

    const auto &reward = get_json_value(definition, "reward");
    if (!reward.is_null()) {
        // オブジェクトでない報酬や整数以外の値は、報酬無しとして黙って無視せず解析エラーにする。
        if (!reward.is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        if (const auto result = info_set_integer(get_json_value(reward, "artifact"), meta.reward_artifact, false, QUEST_ARTIFACT_RANGE); !result) {
            return result;
        }

        const auto &artifacts = get_json_value(reward, "artifacts");
        if (!artifacts.is_null()) {
            if (!artifacts.is_array()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
            for (const auto &candidate : artifacts) {
                int candidate_id = 0;
                if (const auto result = info_set_integer(candidate, candidate_id, true, QUEST_ARTIFACT_RANGE); !result) {
                    return result;
                }
                parsed.reward_artifact_candidates.push_back(candidate_id);
            }
        }
    }

    return {};
}

tl::expected<void, ParseErrorType> QuestReader::set_descriptions(QuestFixedMap &parsed) const
{
    const auto &descriptions = get_json_value(this->quest_data, "descriptions");
    if (descriptions.is_null()) {
        return {};
    }
    if (!descriptions.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (const auto &description : descriptions) {
        // get_json_value() はオブジェクト以外から引くと null を返すので、入れ物の型を先に確かめる。
        // 確かめないと、条件が無いものとして扱われ、説明文が黙ってどの状態でも表示される。
        if (!description.is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }

        QuestDescriptionBlock block;

        const auto &when = get_json_value(description, "when");
        if (!when.is_null() && !when.is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto &status_at_most = get_json_value(when, "statusAtMost");
        if (!status_at_most.is_null()) {
            const auto value = info_read_token(status_at_most, QUEST_STATUS_TOKENS);
            if (!value) {
                return tl::make_unexpected(value.error());
            }
            block.status_at_most = *value;
        }

        const auto &status = get_json_value(when, "status");
        if (!status.is_null()) {
            const auto value = info_read_token(status, QUEST_STATUS_TOKENS);
            if (!value) {
                return tl::make_unexpected(value.error());
            }
            block.status_equals = *value;
        }

        const auto &quest_type = get_json_value(when, "questType");
        if (!quest_type.is_null()) {
            const auto value = info_read_token(quest_type, QUEST_KIND_TOKENS);
            if (!value) {
                return tl::make_unexpected(value.error());
            }
            block.quest_type = *value;
        }

        const auto &text = get_json_value(description, "text");
        if (!text.is_null() && !text.is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        if (const auto result = read_localized_lines(get_json_value(text, "ja"), block.lines_ja); !result) {
            return result;
        }
        if (const auto result = read_localized_lines(get_json_value(text, "en"), block.lines_en); !result) {
            return result;
        }

        parsed.descriptions.push_back(std::move(block));
    }

    return {};
}

tl::expected<void, ParseErrorType> QuestReader::set_legend(QuestFixedMap &parsed, std::string *reason) const
{
    const auto &legend = get_json_value(this->quest_data, "legend");
    if (legend.is_null()) {
        return {};
    }
    if (!legend.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (const auto &[symbol, cell_data] : legend.items()) {
        const auto parsed_symbol = parse_fixed_map_symbol(symbol);
        if (!parsed_symbol) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        QuestLegendCell cell;
        std::string cell_reason;
        if (const auto result = parse_quest_legend_cell(cell_data, cell, cell_reason); !result) {
            if (reason != nullptr && !cell_reason.empty()) {
                *reason = "legend['" + symbol + "'].trap: " + cell_reason;
            }
            return result;
        }
        parsed.legend.insert_or_assign(*parsed_symbol, cell);
    }

    return {};
}

tl::expected<void, ParseErrorType> QuestReader::set_maps(QuestFixedMap &parsed) const
{
    const auto &map = get_json_value(this->quest_data, "map");
    if (!map.is_null()) {
        if (!map.is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        std::vector<std::string> rows;
        if (const auto result = read_map_rows(map, rows); !result) {
            return result;
        }
        parsed.maps.push_back(std::move(rows));
        return {};
    }

    const auto &variants = get_json_value(this->quest_data, "mapVariants");
    if (!variants.is_null()) {
        if (!variants.is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        for (const auto &variant : variants) {
            if (!variant.is_array()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
            std::vector<std::string> rows;
            if (const auto result = read_map_rows(variant, rows); !result) {
                return result;
            }
            parsed.maps.push_back(std::move(rows));
        }
    }

    return {};
}

tl::expected<void, ParseErrorType> QuestReader::set_starts(QuestFixedMap &parsed) const
{
    const auto &start = get_json_value(this->quest_data, "start");
    if (!start.is_null()) {
        QuestStartPosition position;
        if (const auto result = info_set_integer(get_json_value(start, "y"), position.y, true, QUEST_INTEGER_RANGE); !result) {
            return result;
        }
        if (const auto result = info_set_integer(get_json_value(start, "x"), position.x, true, QUEST_INTEGER_RANGE); !result) {
            return result;
        }
        if (const auto result = validate_quest_start(parsed, position); !result) {
            return result;
        }
        parsed.starts.push_back(position);
        return {};
    }

    const auto &variants = get_json_value(this->quest_data, "startVariants");
    if (!variants.is_null()) {
        if (!variants.is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        for (const auto &variant : variants) {
            QuestStartPosition position;
            // 整数以外の値は黙って無視せず解析エラーにする（無視すると条件付きの開始位置が無条件になる）。
            const auto &leaving_quest = get_json_value(variant, "leavingQuest");
            if (!leaving_quest.is_null()) {
                int leaving_id = 0;
                if (const auto result = info_set_integer(leaving_quest, leaving_id, true, QUEST_LEAVING_QUEST_RANGE); !result) {
                    return result;
                }
                position.leaving_quest = leaving_id;
            }
            if (const auto result = info_set_integer(get_json_value(variant, "y"), position.y, true, QUEST_INTEGER_RANGE); !result) {
                return result;
            }
            if (const auto result = info_set_integer(get_json_value(variant, "x"), position.x, true, QUEST_INTEGER_RANGE); !result) {
                return result;
            }
            if (const auto result = validate_quest_start(parsed, position); !result) {
                return result;
            }
            parsed.starts.push_back(position);
        }
    }

    return {};
}

/*!
 * @brief 地形・罠タグを解決し、許可された場合だけ「*」をランダム指定にする。
 * @param random_flag 対応するランダムビット。RANDOM_NONEなら「*」も通常のタグとして解決する。
 * @return 未定義のタグはUNDEFINED_TERRAIN_TAG。失敗時は出力を変更しない。
 */
tl::expected<void, ParseErrorType> resolve_legend_terrain(std::string_view tag, short &terrain, int &random, int random_flag)
{
    if (tag == "*" && random_flag != RANDOM_NONE) {
        random |= random_flag;
        return {};
    }
    try {
        terrain = TerrainList::get_instance().get_terrain_id(tag);
    } catch (const std::exception &) {
        return tl::make_unexpected(ParseErrorType::UNDEFINED_TERRAIN_TAG);
    }
    return {};
}

/*!
 * @brief 凡例の報酬参照を、確定済みのクエスト報酬から解決する。
 * @param grid 解決した報酬を書き込む先
 * @param cell 凡例のセル
 * @param quest 報酬を確定済みのクエスト
 * @details 参照先の検証と例外処理は呼出側が担当する。objectは通常の報酬だけを対象とする。
 */
void resolve_legend_reward(dungeon_grid &grid, const QuestLegendCell &cell, const QuestType &quest)
{
    if (cell.object_is_quest_reward && quest.has_reward() && !quest.is_reward_instant_artifact()) {
        grid.object = quest.get_reward_bi_id();
    }
    if (cell.artifact_is_quest_reward) {
        grid.artifact = quest.get_reward().value_or(FixedArtifactId::NONE);
    }
}
