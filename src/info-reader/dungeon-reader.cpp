#include "info-reader/dungeon-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/monster-flag-reader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/language-switcher.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <cstdint>
#include <exception>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
//! 入口の座標 (wild_y・wild_x) の上限。読み込んだ荒野の大きさに収まるかは validate_wilderness_positions() で確かめる。
constexpr int DUNGEON_ENTRANCE_COORD_MAX = 32766;
}

DungeonReader::Session DungeonReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, enum2i(DungeonId::MAX) - 1 }, _("ダンジョン", "dungeon"), last_id);
}

DungeonReader::DungeonReader(const nlohmann::json &dungeon_data, Session &session)
    : dungeon_data(dungeon_data)
    , session(session)
{
}

/*!
 * @brief ダンジョン定義(JSON Object)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::read() const
{
    const auto &element = this->dungeon_data;
    if (const auto result = info_require_object(element); !result) {
        return result;
    }

    const auto id_result = this->session.read_and_report(get_json_value(element, "id"));
    if (!id_result) {
        return tl::make_unexpected(id_result.error());
    }
    const auto id = *id_result;

    DungeonDefinition dungeon;
    if (const auto result = info_set_string(get_json_value(element, "name"), dungeon.name, true); !result) {
        return result;
    }
    if (const auto result = this->set_dungeon_description(dungeon); !result) {
        return result;
    }

    const auto &position_obj = get_json_value(element, "position");
    if (const auto result = info_require_object(position_obj); !result) {
        return result;
    }
    int wild_y = 0;
    int wild_x = 0;
    if (const auto result = info_set_integer(get_json_value(position_obj, "wild_y"), wild_y, true, Range(0, DUNGEON_ENTRANCE_COORD_MAX)); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(position_obj, "wild_x"), wild_x, true, Range(0, DUNGEON_ENTRANCE_COORD_MAX)); !result) {
        return result;
    }
    dungeon.initialize_position({ wild_y, wild_x });

    if (const auto result = this->set_dungeon_generation(dungeon); !result) {
        return result;
    }
    if (const auto result = this->set_dungeon_floor(dungeon); !result) {
        return result;
    }
    if (const auto result = this->set_dungeon_wall(dungeon); !result) {
        return result;
    }
    if (const auto result = this->set_dungeon_final_floor(dungeon); !result) {
        return result;
    }
    if (const auto result = this->set_dungeon_flags(dungeon); !result) {
        return result;
    }
    if (const auto result = this->set_dungeon_monsters(dungeon); !result) {
        return result;
    }

    auto &dungeons = DungeonList::get_instance();
    dungeons.emplace(i2enum<DungeonId>(id), std::move(dungeon));
    return {};
}

/*!
 * @brief 読み込み済みダンジョンの入口が、荒野の実サイズの端を除いた内側（1〜高さ−2、1〜幅−2）にあるか確認する。
 * @param height 荒野の高さ
 * @param width 荒野の幅
 * @details ダンジョン定義の後に荒野定義を読むため、荒野の初期化前に呼ぶ。荒野自体（ID 0）は対象外。
 * @return 成功時は空の値、失敗時は解析エラーコード（端または範囲外の入口があれば範囲外エラー）
 */
tl::expected<void, ParseErrorType> DungeonReader::validate_wilderness_positions(int height, int width)
{
    for (const auto &[id, dungeon] : DungeonList::get_instance()) {
        if (id == DungeonId::WILDERNESS) {
            continue;
        }
        // ダンジョンを出ると荒野の現在地が入口になり、地上の生成では現在地の周囲8マスを生成するため、
        // 入口は荒野の端を除いた内側に限る。
        const auto &pos = dungeon->get_position();
        if (pos.y < 1 || pos.y > height - 2 || pos.x < 1 || pos.x > width - 2) {
            msg_print(_("DungeonDefinitions.jsonc: ダンジョンID '{}' の入口 ({}, {}) は荒野サイズ ({}, {}) の端を除いた内側にありません。",
                          "DungeonDefinitions.jsonc: dungeon ID '{}' entrance ({}, {}) must be inside wilderness size ({}, {}) excluding the edges."),
                enum2i(id), pos.y, pos.x, height, width);
            return tl::make_unexpected(ParseErrorType::OUT_OF_BOUNDS);
        }
    }
    return {};
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what) const
{
    if (info_grab_one_flag(dungeon.flags, dungeon_flags, what)) {
        return true;
    }

    msg_print(_("未知のダンジョン・フラグ '{}'。", "Unknown dungeon type flag '{}'."), what);
    return false;
}

namespace {
/*!
 * @brief JSON配列からEnumClassFlagGroupを読み込む
 * @param obj JSONオブジェクト
 * @param key 読み込むキー
 * @param flags 読み込み先フラグ群
 * @param tokens 文字列トークンとenum値の対応表
 * @param label エラー表示用ラベル
 * @return 成功、またはパースエラー
 */
template <typename Enum>
tl::expected<void, ParseErrorType> info_set_enum_flag_group(const nlohmann::json &obj, std::string_view key, EnumClassFlagGroup<Enum> &flags, const std::unordered_map<std::string_view, Enum> &tokens, std::string_view label)
{
    const auto &array_obj = get_json_value(obj, key);
    if (array_obj.is_null()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    return info_read_flag_array(array_obj, [&](const nlohmann::json &flag_obj) {
        const auto token = flag_obj.get<std::string_view>();
        if (token.empty()) {
            return true;
        }
        if (info_grab_one_flag(flags, tokens, token)) {
            return true;
        }
        msg_print(_("未知のダンジョン{}種別 '{}'。", "Unknown dungeon {} kind '{}'."), label, token);
        return false; });
}
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1)
 * @details 性別を先に探し、見つからなければ grab_one_monster_basic_flag() で基本フラグの表を探す。
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what) const
{
    // NONEは種族の性別未設定を表す値であり、出現条件のフラグにはしない。
    if (what != "NONE" && info_grab_one_flag(dungeon.mon_sex, monrace_sex, what)) {
        return true;
    }

    return grab_one_monster_basic_flag(
        {
            dungeon.mon_resistance_flags,
            dungeon.mon_aura_flags,
            dungeon.mon_behavior_flags,
            dungeon.mon_visual_flags,
            dungeon.mon_kind_flags,
            dungeon.mon_drop_flags,
            dungeon.mon_wilderness_flags,
            dungeon.mon_feature_flags,
            dungeon.mon_population_flags,
            dungeon.mon_speak_flags,
            dungeon.mon_brightness_flags,
            dungeon.mon_special_flags,
            dungeon.mon_misc_flags,
        },
        what);
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what) const
{
    return grab_one_monster_spell_flag(dungeon.mon_ability_flags, what);
}

namespace {
/*!
 * @brief 床・壁の地形の抽選表を読み込む。
 * @param tiles_obj 地形と重みの配列
 * @param section 診断に表示する節の名前 (floor / wall)
 * @return 抽選表。配列や地形・重みが無ければ TOO_FEW_ARGUMENTS、型が誤っていれば INVALID_TYPE、未知の地形なら UNDEFINED_TERRAIN_TAG、
 * 重みが範囲外か重みの合計が100でなければ INVALID_VALUE
 * @details 重みは百分率で、各0～100・合計はちょうど100とする。重みが0の地形は抽選表に登録しない。
 */
tl::expected<ProbabilityTable<short>, ParseErrorType> parse_terrain_probability(const nlohmann::json &tiles_obj, std::string_view section)
{
    constexpr auto total_percent = 100;
    const auto &terrains = TerrainList::get_instance();
    ProbabilityTable<short> prob_table;
    int total_probability = 0;
    const auto read_result = info_read_object_array(tiles_obj, [&](const nlohmann::json &tile_obj) -> tl::expected<void, ParseErrorType> {
        const auto &type_obj = get_json_value(tile_obj, "type");
        if (!type_obj.is_string()) {
            return tl::make_unexpected(type_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
        }
        const auto &rate_obj = get_json_value(tile_obj, "rate");
        if (!rate_obj.is_number_integer()) {
            return tl::make_unexpected(rate_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
        }

        int prob = 0;
        if (!info_set_integer(rate_obj, prob, true, Range(0, total_percent))) {
            msg_print(_("DungeonDefinitions.jsonc: {}.tiles の rate '{}' が0～100の範囲外です (百分率で指定してください)。",
                          "DungeonDefinitions.jsonc: {}.tiles rate '{}' is outside 0 to 100 (they are percentages)."),
                section, rate_obj.dump());
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        try {
            const auto terrain_id = terrains.get_terrain_id(type_obj.get<std::string>());
            prob_table.entry_item(terrain_id, prob);
        } catch (const std::exception &) {
            return tl::make_unexpected(ParseErrorType::UNDEFINED_TERRAIN_TAG);
        }

        // 合計は全項目の地形と重みを確かめてから判定し、未知の地形は位置によらず UNDEFINED_TERRAIN_TAG とする。
        // 各重みは100以下なので、合計が100を超えた後は加算をやめれば int の範囲を超えない。
        if (total_probability <= total_percent) {
            total_probability += prob;
        }
        return {};
    });
    if (!read_result) {
        return tl::make_unexpected(read_result.error());
    }

    if (total_probability > total_percent) {
        msg_print(_("DungeonDefinitions.jsonc: {}.tiles の rate の合計が '{}' となり100を超えています (百分率で指定してください)。",
                      "DungeonDefinitions.jsonc: {}.tiles rates reach a total of '{}', exceeding 100 (they are percentages)."),
            section, total_probability);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    if (total_probability != total_percent) {
        msg_print(_("DungeonDefinitions.jsonc: {}.tiles の rate の合計 '{}' が100ではありません (百分率で指定してください)。",
                      "DungeonDefinitions.jsonc: {}.tiles rates total '{}', not 100 (they are percentages)."),
            section, total_probability);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    return prob_table;
}
}

/*!
 * @brief ダンジョンの説明文を読み込む
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_description(DungeonDefinition &dungeon) const
{
    if (const auto result = info_set_string(get_json_value(this->dungeon_data, "description"), dungeon.text, false); !result) {
        return result;
    }
    return {};
}

/*!
 * @brief ダンジョンの生成条件を読み込む。
 * @details objGood・objGreatは幸運補正前の生成率の上限で、0～100に限定する。
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_generation(DungeonDefinition &dungeon) const
{
    const auto &generation_obj = get_json_value(this->dungeon_data, "generation");
    if (const auto result = info_require_object(generation_obj); !result) {
        return result;
    }

    if (const auto result = info_set_integer(get_json_value(generation_obj, "minDepth"), dungeon.mindepth, true, Range(0, std::numeric_limits<int16_t>::max())); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(generation_obj, "maxDepth"), dungeon.maxdepth, true, Range(0, std::numeric_limits<int16_t>::max())); !result) {
        return result;
    }
    // 保存形式は符号付き16bit。最浅階が最深階を超える定義も拒否する。
    if (dungeon.mindepth > dungeon.maxdepth) {
        msg_print(_("DungeonDefinitions.jsonc: minDepth '{}' がmaxDepth '{}' を超えています。",
                      "DungeonDefinitions.jsonc: minDepth '{}' exceeds maxDepth '{}'."),
            dungeon.mindepth, dungeon.maxdepth);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    if (const auto result = info_set_integer(get_json_value(generation_obj, "minPlayerLevel"), dungeon.min_plev, true, Range(0, PY_MAX_LEVEL)); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(generation_obj, "objGood"), dungeon.obj_good, true, Range(0, 100)); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(generation_obj, "objGreat"), dungeon.obj_great, true, Range(0, 100)); !result) {
        return result;
    }

    if (const auto result = info_set_enum_flag_group(generation_obj, "pit", dungeon.pit, dungeon_pit_kinds, "pit"); !result) {
        return result;
    }

    if (const auto result = info_set_enum_flag_group(generation_obj, "nest", dungeon.nest, dungeon_nest_kinds, "nest"); !result) {
        return result;
    }

    return {};
}

/*!
 * @brief 床の地形の抽選表と通路の生成率を読み込む
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_floor(DungeonDefinition &dungeon) const
{
    const auto &floor_obj = get_json_value(this->dungeon_data, "floor");
    if (const auto result = info_require_object(floor_obj); !result) {
        return result;
    }

    auto prob_table = parse_terrain_probability(get_json_value(floor_obj, "tiles"), "floor");
    if (!prob_table) {
        return tl::make_unexpected(prob_table.error());
    }
    dungeon.prob_table_floor = std::move(*prob_table);

    if (const auto result = info_set_integer(get_json_value(floor_obj, "tunnelRate"), dungeon.tunnel_percent, true, Range(0, std::numeric_limits<int16_t>::max())); !result) {
        return result;
    }
    return {};
}

/*!
 * @brief 壁の鉱脈の地形・本数・生成率・優先度を読み込む
 * @param streams_obj 鉱脈の配列（無ければ null）
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_streams(const nlohmann::json &streams_obj, DungeonDefinition &dungeon) const
{
    if (streams_obj.is_null()) {
        return {};
    }

    const auto &terrains = TerrainList::get_instance();
    const auto read_result = info_read_object_array(streams_obj, [&terrains, &dungeon](const nlohmann::json &stream_obj) -> tl::expected<void, ParseErrorType> {
        if (!get_json_value(stream_obj, "type").is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        DungeonStreamDefinition stream;
        try {
            stream.terrain_id = terrains.get_terrain_id(get_json_value(stream_obj, "type").get<std::string>());
        } catch (const std::exception &) {
            return tl::make_unexpected(ParseErrorType::UNDEFINED_TERRAIN_TAG);
        }

        if (const auto result = info_set_integer(get_json_value(stream_obj, "count"), stream.count, true, Range(1, 255)); !result) {
            return result;
        }
        if (const auto result = info_set_integer(get_json_value(stream_obj, "chance"), stream.chance, true, Range(1, 65535)); !result) {
            return result;
        }
        if (const auto result = info_set_integer(get_json_value(stream_obj, "priority"), stream.priority, true, Range(0, 255)); !result) {
            return result;
        }

        dungeon.streams.emplace_back(stream);
        return {};
    });
    if (!read_result) {
        return read_result;
    }

    dungeon.sort_streams_by_priority();

    return {};
}

/*!
 * @brief 壁の地形の抽選表と、外壁・内壁などの地形、鉱脈を読み込む
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_wall(DungeonDefinition &dungeon) const
{
    const auto &wall_obj = get_json_value(this->dungeon_data, "wall");
    if (const auto result = info_require_object(wall_obj); !result) {
        return result;
    }

    auto prob_table = parse_terrain_probability(get_json_value(wall_obj, "tiles"), "wall");
    if (!prob_table) {
        return tl::make_unexpected(prob_table.error());
    }
    dungeon.prob_table_wall = std::move(*prob_table);

    const auto &terrains = TerrainList::get_instance();
    for (const auto &[key, wall_id] : { std::pair{ "outer", &dungeon.outer_wall }, std::pair{ "inner", &dungeon.inner_wall } }) {
        const auto &terrain_obj = get_json_value(wall_obj, key);
        if (terrain_obj.is_null()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        if (!terrain_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        try {
            *wall_id = terrains.get_terrain_id(terrain_obj.get<std::string>());
        } catch (const std::exception &) {
            return tl::make_unexpected(ParseErrorType::UNDEFINED_TERRAIN_TAG);
        }
    }

    return this->set_dungeon_streams(get_json_value(wall_obj, "streams"), dungeon);
}

/*!
 * @brief ダンジョンのフラグを読み込む
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_flags(DungeonDefinition &dungeon) const
{
    const auto &flags_obj = get_json_value(this->dungeon_data, "flags");
    if (flags_obj.is_null()) {
        return {};
    }
    return info_read_flag_array(flags_obj, [this, &dungeon](const nlohmann::json &flag) {
        const auto token = flag.get<std::string>();
        return token.empty() || this->grab_one_dungeon_flag(dungeon, token); });
}

namespace {
/*!
 * @brief 範囲内の整数を読み込み、ID として有効な場合だけ列挙型の値で格納する
 * @param json 読み込む値
 * @param data 格納先
 * @param label 診断に表示する項目名
 * @param range 受け付ける整数の範囲
 * @param validator ID が有効かを判定する関数
 * @return 成功、またはエラーコード。範囲外や無効な ID は INVALID_VALUE
 */
template <typename Enum, typename Validator>
tl::expected<void, ParseErrorType> info_set_enum_from_integer_checked(const nlohmann::json &json, Enum &data, std::string_view label, Range range, Validator validator)
{
    Enum enum_value{};
    if (const auto result = info_set_integer(json, enum_value, true, range); !result) {
        if (result.error() == ParseErrorType::INVALID_VALUE) {
            msg_print(_("{} ID '{}' は範囲外です。", "{} ID '{}' is out of range."), label, json.dump());
        }
        return result;
    }

    const auto value = enum2i(enum_value);

    if (!validator(enum_value)) {
        msg_print(_("不正な{} ID '{}'。", "Invalid {} ID '{}'."), label, value);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    data = enum_value;
    return {};
}

/*!
 * @brief 最下層で手に入るベースアイテムの ID を読み込み、定義済みの場合だけ格納する
 * @param json 読み込む値
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード。未定義の ID は INVALID_VALUE
 */
tl::expected<void, ParseErrorType> info_set_baseitem_id_checked(const nlohmann::json &json, DungeonDefinition &dungeon)
{
    int value{};
    if (const auto result = info_set_integer(json, value, true, Range(1, std::numeric_limits<short>::max())); !result) {
        return result;
    }

    const auto value_short = static_cast<short>(value);
    try {
        static_cast<void>(BaseitemList::get_instance().get_baseitem(value_short));
    } catch (const std::exception &) {
        msg_print(_("不正なfinal_floor.object ID '{}'。", "Invalid final_floor.object ID '{}'."), value);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    dungeon.final_object = value_short;
    return {};
}
}

/*!
 * @brief 最下層の守護者と、最下層で手に入るベースアイテム・アーティファクトを読み込む
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_final_floor(DungeonDefinition &dungeon) const
{
    const auto &final_floor_obj = get_json_value(this->dungeon_data, "final_floor");
    if (final_floor_obj.is_null()) {
        return {};
    }

    if (!final_floor_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    if (final_floor_obj.contains("artifact") && !final_floor_obj.contains("guardian")) {
        msg_print(_("final_floor.artifactを指定する場合はguardianも必要です。", "final_floor.guardian is required when final_floor.artifact is specified."));
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    if (final_floor_obj.contains("guardian")) {
        if (const auto result = info_set_enum_from_integer_checked(get_json_value(final_floor_obj, "guardian"), dungeon.final_guardian, "final_floor.guardian", Range(1, MAX_MONRACE_ID), [](MonraceId monrace_id) {
                const auto &monraces = MonraceList::get_instance();
                return MonraceList::is_valid(monrace_id) && monraces.contains(monrace_id);
            });
            !result) {
            return result;
        }
    }
    if (final_floor_obj.contains("object")) {
        if (const auto result = info_set_baseitem_id_checked(get_json_value(final_floor_obj, "object"), dungeon); !result) {
            return result;
        }
    }
    if (final_floor_obj.contains("artifact")) {
        if (const auto result = info_set_enum_from_integer_checked(get_json_value(final_floor_obj, "artifact"), dungeon.final_artifact, "final_floor.artifact", Range(0, MAX_FIXED_ARTIFACT_ID), [](FixedArtifactId artifact_id) {
                const auto &artifacts = ArtifactList::get_instance();
                return (artifact_id == FixedArtifactId::NONE) || artifacts.contains(artifact_id);
            });
            !result) {
            return result;
        }
    }

    return {};
}

/*!
 * @brief モンスターの出現条件のうち、基本のフラグを読み込む
 * @param flags_obj フラグの配列（無ければ null）
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_monster_flags(const nlohmann::json &flags_obj, DungeonDefinition &dungeon) const
{
    if (flags_obj.is_null()) {
        return {};
    }
    return info_read_flag_array(flags_obj, [this, &dungeon](const nlohmann::json &flag) {
        const auto token = flag.get<std::string>();
        return token.empty() || this->grab_one_basic_monster_flag(dungeon, token); });
}

/*!
 * @brief モンスターの出現条件のうち、表示文字を読み込む
 * @param symbols_obj 1 文字の文字列の配列（無ければ null）
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_monster_symbols(const nlohmann::json &symbols_obj, DungeonDefinition &dungeon) const
{
    if (symbols_obj.is_null()) {
        return {};
    }
    if (!symbols_obj.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (const auto &symbol_obj : symbols_obj) {
        if (!symbol_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        const auto symbol = symbol_obj.get<std::string>();
        if (symbol.empty()) {
            continue;
        }
        if (symbol.size() != 1) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        dungeon.r_chars.push_back(symbol[0]);
    }

    return {};
}

/*!
 * @brief モンスターの出現条件のうち、魔法のフラグを読み込む
 * @param spells_obj 魔法のフラグの配列（無ければ null）
 * @param dungeon 読み込み先のダンジョン
 * @return 成功、またはエラーコード
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_monster_spells(const nlohmann::json &spells_obj, DungeonDefinition &dungeon) const
{
    if (spells_obj.is_null()) {
        return {};
    }
    return info_read_flag_array(spells_obj, [this, &dungeon](const nlohmann::json &flag) {
        const auto token = flag.get<std::string>();
        return token.empty() || this->grab_one_spell_monster_flag(dungeon, token); });
}

/*!
 * @brief ダンジョンのモンスター生成条件を読み込む。
 * @details minCountは基本生成試行数で、追加試行とは別に0～100へ限定する。同梱最大80を含み、集団配置用の空きを残す。
 */
tl::expected<void, ParseErrorType> DungeonReader::set_dungeon_monsters(DungeonDefinition &dungeon) const
{
    const auto &monsters_obj = get_json_value(this->dungeon_data, "monsters");
    if (const auto result = info_require_object(monsters_obj); !result) {
        return result;
    }

    if (const auto result = info_set_integer(get_json_value(monsters_obj, "minCount"), dungeon.min_monster_count_on_floor, true, Range(0, 100)); !result) {
        return result;
    }
    auto additionalSpawnProbability = 1;
    constexpr auto conversion_rate = 1000000;
    if (const auto result = info_set_integer(get_json_value(monsters_obj, "additionalSpawnProbability"), additionalSpawnProbability, true, Range(1, conversion_rate)); !result) {
        return result;
    }
    dungeon.additional_monster_spawn_chance = conversion_rate / additionalSpawnProbability;
    if (const auto result = info_set_integer(get_json_value(monsters_obj, "normalMonsterRate"), dungeon.normal_monster_rate, true, Range(0, 100)); !result) {
        return result;
    }

    const auto &flags_mode_obj = get_json_value(monsters_obj, "flagsMode");
    const auto mode = info_read_token(flags_mode_obj, dungeon_modes);
    if (!mode) {
        if (mode.error() == ParseErrorType::INVALID_FLAG) {
            msg_print(_("未知のダンジョン・モンスター生成モード '{}'。", "Unknown dungeon monster generation mode '{}'."), flags_mode_obj.get<std::string>());
        }
        return tl::make_unexpected(mode.error());
    }
    dungeon.mode = *mode;

    if (const auto result = this->set_dungeon_monster_flags(get_json_value(monsters_obj, "flags"), dungeon); !result) {
        return result;
    }

    if (const auto result = this->set_dungeon_monster_symbols(get_json_value(monsters_obj, "symbols"), dungeon); !result) {
        return result;
    }

    if (const auto result = this->set_dungeon_monster_spells(get_json_value(monsters_obj, "spells"), dungeon); !result) {
        return result;
    }

    return {};
}
