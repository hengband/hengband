#include "info-reader/dungeon-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
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
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <exception>
#include <memory>
#include <nlohmann/json.hpp>
#include <span>

DungeonReader::DungeonReader(const nlohmann::json &dungeon_data)
    : dungeon_data(dungeon_data)
{
}

int DungeonReader::read() const
{
    const auto &element = this->dungeon_data;
    if (element.is_null() || !element.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int id = 0;
    if (auto err = info_set_integer(get_json_value(element, "id"), id, true, Range(0, 9999))) {
        return err;
    }
    if (id <= error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    error_idx = id;

    DungeonDefinition dungeon;
    if (auto err = info_set_string(get_json_value(element, "name"), dungeon.name, true)) {
        return err;
    }
    if (auto err = this->set_dungeon_description(dungeon)) {
        return err;
    }

    const auto &position_obj = get_json_value(element, "position");
    if (position_obj.is_null() || !position_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    int wild_y = 0;
    int wild_x = 0;
    if (auto err = info_set_integer(get_json_value(position_obj, "wild_y"), wild_y, true)) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(position_obj, "wild_x"), wild_x, true)) {
        return err;
    }
    dungeon.initialize_position({ wild_y, wild_x });

    if (auto err = this->set_dungeon_generation(dungeon)) {
        return err;
    }
    if (auto err = this->set_dungeon_floor(dungeon)) {
        return err;
    }
    if (auto err = this->set_dungeon_wall(dungeon)) {
        return err;
    }
    if (auto err = this->set_dungeon_final_floor(dungeon)) {
        return err;
    }
    if (auto err = this->set_dungeon_flags(dungeon)) {
        return err;
    }
    if (auto err = this->set_dungeon_monsters(dungeon)) {
        return err;
    }

    auto &dungeons = DungeonList::get_instance();
    dungeons.emplace(i2enum<DungeonId>(id), std::move(dungeon));
    return PARSE_ERROR_NONE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what) const
{
    if (EnumClassFlagGroup<DungeonFeatureType>::grab_one_flag(dungeon.flags, dungeon_flags, what)) {
        return true;
    }

    msg_print(_("未知のダンジョン・フラグ '{}'。", "Unknown dungeon type flag '{}'."), what);
    return false;
}

/*!
 * @brief テキストトークンを走査してモンスター生成条件フラグの結合モードを得る
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_dungeon_mode(DungeonDefinition &dungeon, std::string_view what) const
{
    const auto it = dungeon_modes.find(what);
    if (it != dungeon_modes.end()) {
        dungeon.mode = it->second;
        return true;
    }

    msg_print(_("未知のダンジョン・モンスター生成モード '{}'。", "Unknown dungeon monster generation mode '{}'."), what);
    return false;
}

/*!
 * @brief JSON配列からEnumClassFlagGroupを読み込む
 * @param obj JSONオブジェクト
 * @param key 読み込むキー
 * @param flags 読み込み先フラグ群
 * @param tokens 文字列トークンとenum値の対応表
 * @param label エラー表示用ラベル
 * @return パースエラー
 */
template <typename Enum>
static errr info_set_enum_flag_group(const nlohmann::json &obj, std::string_view key, EnumClassFlagGroup<Enum> &flags, const std::unordered_map<std::string_view, Enum> &tokens, std::string_view label)
{
    const auto it = obj.find(key);
    if (it == obj.end() || it->is_null() || !it->is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &array_obj = *it;

    for (const auto &flag_obj : array_obj) {
        if (!flag_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto token = flag_obj.get<std::string_view>();
        if (token.empty()) {
            continue;
        }

        if (EnumClassFlagGroup<Enum>::grab_one_flag(flags, tokens, token)) {
            continue;
        }

        msg_print(_("未知のダンジョン{}種別 '{}'。", "Unknown dungeon {} kind '{}'."), label, token);
        return PARSE_ERROR_INVALID_FLAG;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what) const
{
    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(dungeon.mon_resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(dungeon.mon_behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(dungeon.mon_visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(dungeon.mon_kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(dungeon.mon_drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(dungeon.mon_wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(dungeon.mon_feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(dungeon.mon_population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(dungeon.mon_speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(dungeon.mon_brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(dungeon.mon_special_flags, r_info_special_flags, what)) {
        return true;
    }
    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(dungeon.mon_misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool DungeonReader::grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what) const
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(dungeon.mon_ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}

static tl::optional<ProbabilityTable<short>> parse_terrain_probability(const nlohmann::json &tiles_obj)
{
    if (tiles_obj.is_null() || !tiles_obj.is_array()) {
        return tl::nullopt;
    }

    const auto &terrains = TerrainList::get_instance();
    ProbabilityTable<short> prob_table;
    for (const auto &tile_obj : tiles_obj) {
        if (!tile_obj.is_object() || !get_json_value(tile_obj, "type").is_string() || !get_json_value(tile_obj, "rate").is_number_integer()) {
            return tl::nullopt;
        }

        try {
            const auto terrain_id = terrains.get_terrain_id(get_json_value(tile_obj, "type").get<std::string>());
            const auto prob = get_json_value(tile_obj, "rate").get<short>();
            prob_table.entry_item(terrain_id, prob);
        } catch (const std::exception &) {
            return tl::nullopt;
        }
    }

    return prob_table;
}

int DungeonReader::set_dungeon_description(DungeonDefinition &dungeon) const
{
    return info_set_string(get_json_value(this->dungeon_data, "description"), dungeon.text, false);
}

int DungeonReader::set_dungeon_generation(DungeonDefinition &dungeon) const
{
    const auto &generation_obj = get_json_value(this->dungeon_data, "generation");
    if (generation_obj.is_null() || !generation_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    if (auto err = info_set_integer(get_json_value(generation_obj, "minDepth"), dungeon.mindepth, true)) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(generation_obj, "maxDepth"), dungeon.maxdepth, true)) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(generation_obj, "minPlayerLevel"), dungeon.min_plev, true)) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(generation_obj, "objGood"), dungeon.obj_good, true)) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(generation_obj, "objGreat"), dungeon.obj_great, true)) {
        return err;
    }

    if (auto err = info_set_enum_flag_group(generation_obj, "pit", dungeon.pit, dungeon_pit_kinds, "pit")) {
        return err;
    }

    if (auto err = info_set_enum_flag_group(generation_obj, "nest", dungeon.nest, dungeon_nest_kinds, "nest")) {
        return err;
    }

    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_floor(DungeonDefinition &dungeon) const
{
    const auto &floor_obj = get_json_value(this->dungeon_data, "floor");
    if (floor_obj.is_null() || !floor_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto prob_table = parse_terrain_probability(get_json_value(floor_obj, "tiles"));
    if (!prob_table) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    dungeon.prob_table_floor = std::move(*prob_table);

    return info_set_integer(get_json_value(floor_obj, "tunnelRate"), dungeon.tunnel_percent, true);
}

int DungeonReader::set_dungeon_streams(const nlohmann::json &streams_obj, DungeonDefinition &dungeon) const
{
    if (streams_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!streams_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &terrains = TerrainList::get_instance();
    for (const auto &stream_obj : streams_obj) {
        if (!stream_obj.is_object() || !get_json_value(stream_obj, "type").is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        DungeonStreamDefinition stream;
        try {
            stream.terrain_id = terrains.get_terrain_id(get_json_value(stream_obj, "type").get<std::string>());
        } catch (const std::exception &) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }

        if (auto err = info_set_integer(get_json_value(stream_obj, "count"), stream.count, true, Range(1, 255))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(stream_obj, "chance"), stream.chance, true, Range(1, 65535))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(stream_obj, "priority"), stream.priority, true, Range(0, 255))) {
            return err;
        }

        if (stream.count <= 0 || stream.chance <= 0) {
            return PARSE_ERROR_INVALID_VALUE;
        }

        dungeon.streams.emplace_back(stream);
    }

    dungeon.sort_streams_by_priority();

    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_wall(DungeonDefinition &dungeon) const
{
    const auto &wall_obj = get_json_value(this->dungeon_data, "wall");
    if (wall_obj.is_null() || !wall_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto prob_table = parse_terrain_probability(get_json_value(wall_obj, "tiles"));
    if (!prob_table) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    dungeon.prob_table_wall = std::move(*prob_table);

    const auto &terrains = TerrainList::get_instance();
    try {
        dungeon.outer_wall = terrains.get_terrain_id(get_json_value(wall_obj, "outer").get<std::string>());
        dungeon.inner_wall = terrains.get_terrain_id(get_json_value(wall_obj, "inner").get<std::string>());
    } catch (const std::exception &) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }

    return this->set_dungeon_streams(get_json_value(wall_obj, "streams"), dungeon);
}

int DungeonReader::set_dungeon_flags(DungeonDefinition &dungeon) const
{
    const auto &flags_obj = get_json_value(this->dungeon_data, "flags");
    if (flags_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flags_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto f = f_obj.get<std::string>();
        if (f.empty()) {
            continue;
        }

        if (!this->grab_one_dungeon_flag(dungeon, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }

    return PARSE_ERROR_NONE;
}

template <typename Enum, typename Validator>
static errr info_set_enum_from_integer_checked(const nlohmann::json &json, Enum &data, std::string_view label, Validator validator)
{
    int value{};
    if (auto err = info_set_integer(json, value, true)) {
        return err;
    }

    const auto enum_value = i2enum<Enum>(value);
    if (!validator(enum_value)) {
        msg_print(_("不正な{} ID '{}'。", "Invalid {} ID '{}'."), label, value);
        return PARSE_ERROR_INVALID_VALUE;
    }

    data = enum_value;
    return PARSE_ERROR_NONE;
}

static errr info_set_baseitem_id_checked(const nlohmann::json &json, DungeonDefinition &dungeon)
{
    int value{};
    if (auto err = info_set_integer(json, value, true, Range(1, 9999))) {
        return err;
    }

    const auto value_short = static_cast<short>(value);
    try {
        static_cast<void>(BaseitemList::get_instance().get_baseitem(value_short));
    } catch (const std::exception &) {
        msg_print(_("不正なfinal_floor.object ID '{}'。", "Invalid final_floor.object ID '{}'."), value);
        return PARSE_ERROR_INVALID_VALUE;
    }

    dungeon.final_object = value_short;
    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_final_floor(DungeonDefinition &dungeon) const
{
    const auto &final_floor_obj = get_json_value(this->dungeon_data, "final_floor");
    if (final_floor_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (!final_floor_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    if (final_floor_obj.contains("artifact") && !final_floor_obj.contains("guardian")) {
        msg_print(_("final_floor.artifactを指定する場合はguardianも必要です。", "final_floor.guardian is required when final_floor.artifact is specified."));
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    if (final_floor_obj.contains("guardian")) {
        if (auto err = info_set_enum_from_integer_checked(get_json_value(final_floor_obj, "guardian"), dungeon.final_guardian, "final_floor.guardian", [](MonraceId monrace_id) {
                const auto &monraces = MonraceList::get_instance();
                return MonraceList::is_valid(monrace_id) && monraces.contains(monrace_id);
            })) {
            return err;
        }
    }
    if (final_floor_obj.contains("object")) {
        if (auto err = info_set_baseitem_id_checked(get_json_value(final_floor_obj, "object"), dungeon)) {
            return err;
        }
    }
    if (final_floor_obj.contains("artifact")) {
        if (auto err = info_set_enum_from_integer_checked(get_json_value(final_floor_obj, "artifact"), dungeon.final_artifact, "final_floor.artifact", [](FixedArtifactId artifact_id) {
                const auto &artifacts = ArtifactList::get_instance();
                return (artifact_id == FixedArtifactId::NONE) || artifacts.contains(artifact_id);
            })) {
            return err;
        }
    }

    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_monster_flags(const nlohmann::json &flags_obj, DungeonDefinition &dungeon) const
{
    if (flags_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flags_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto f = f_obj.get<std::string>();
        if (f.empty()) {
            continue;
        }

        if (!this->grab_one_basic_monster_flag(dungeon, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }

    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_monster_symbols(const nlohmann::json &symbols_obj, DungeonDefinition &dungeon) const
{
    if (symbols_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!symbols_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &symbol_obj : symbols_obj) {
        if (!symbol_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto symbol = symbol_obj.get<std::string>();
        if (symbol.empty()) {
            continue;
        }
        if (symbol.size() != 1) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        dungeon.r_chars.push_back(symbol[0]);
    }

    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_monster_spells(const nlohmann::json &spells_obj, DungeonDefinition &dungeon) const
{
    if (spells_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!spells_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &s_obj : spells_obj) {
        if (!s_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto s = s_obj.get<std::string>();
        if (s.empty()) {
            continue;
        }

        if (!this->grab_one_spell_monster_flag(dungeon, s)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }

    return PARSE_ERROR_NONE;
}

int DungeonReader::set_dungeon_monsters(DungeonDefinition &dungeon) const
{
    const auto &monsters_obj = get_json_value(this->dungeon_data, "monsters");
    if (monsters_obj.is_null() || !monsters_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    if (auto err = info_set_integer(get_json_value(monsters_obj, "minCount"), dungeon.min_monster_count_on_floor, true)) {
        return err;
    }
    auto additionalSpawnProbability = 1;
    constexpr auto conversion_rate = 1000000;
    if (auto err = info_set_integer(get_json_value(monsters_obj, "additionalSpawnProbability"), additionalSpawnProbability, true, Range(1, conversion_rate))) {
        return err;
    }
    dungeon.additional_monster_spawn_chance = conversion_rate / additionalSpawnProbability;
    if (auto err = info_set_integer(get_json_value(monsters_obj, "normalMonsterRate"), dungeon.normal_monster_rate, true, Range(0, 100))) {
        return err;
    }

    const auto &flags_mode_obj = get_json_value(monsters_obj, "flagsMode");
    if (!flags_mode_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto mode = flags_mode_obj.get<std::string>();
    if (!this->grab_one_dungeon_mode(dungeon, mode)) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    if (auto it = monsters_obj.find("flags"); it != monsters_obj.end()) {
        if (auto err = this->set_dungeon_monster_flags(*it, dungeon)) {
            return err;
        }
    }

    if (auto it = monsters_obj.find("symbols"); it != monsters_obj.end()) {
        if (auto err = this->set_dungeon_monster_symbols(*it, dungeon)) {
            return err;
        }
    }

    if (auto it = monsters_obj.find("spells"); it != monsters_obj.end()) {
        if (auto err = this->set_dungeon_monster_spells(*it, dungeon)) {
            return err;
        }
    }

    return PARSE_ERROR_NONE;
}
