#include "info-reader/quest-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/character-encoding.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/enum-converter.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
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
 * @brief JSONの文字列配列を std::vector<std::string> に取り込む (null/欠落は空)
 * @param localize true の場合、各行を UTF-8 から内部エンコーディングへ変換する (説明文用)。
 * マップ行のようなASCIIの記号列は変換不要なので false を指定する。
 */
void read_string_lines(const nlohmann::json &array_data, std::vector<std::string> &out, bool localize)
{
    if (!array_data.is_array()) {
        return;
    }

    for (const auto &line : array_data) {
        if (line.is_string()) {
            auto value = line.get<std::string>();
            out.push_back(localize ? utf8_to_local(value) : std::move(value));
        }
    }
}

/*!
 * @brief JSON値が真偽値 true のときのみ true を返す (存在しても false や非booleanなら無効)
 */
bool json_is_true(const nlohmann::json &value)
{
    return value.is_boolean() && value.get<bool>();
}
}

parse_error_type parse_quest_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out)
{
    if (!cell_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    auto &grid = out.grid;
    grid.set_terrain_id(TerrainTag::NONE);
    grid.monster = 0;
    grid.object = 0;
    grid.ego = EgoType::NONE;
    grid.artifact = FixedArtifactId::NONE;
    grid.set_trap_id(TerrainTag::NONE);
    grid.cave_info = 0;
    grid.special = 0;
    grid.random = RANDOM_NONE;
    out.object_is_quest_reward = false;
    out.artifact_is_quest_reward = false;

    const auto &terrains = TerrainList::get_instance();

    const auto &terrain = get_json_value(cell_data, "terrain");
    if (terrain.is_string()) {
        const auto tag = terrain.get<std::string>();
        if (tag == "*") {
            grid.random |= RANDOM_FEATURE;
        } else {
            try {
                grid.feature = terrains.get_terrain_id(tag);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }
        }
    }

    const auto &cave_info = get_json_value(cell_data, "caveInfo");
    if (!cave_info.is_null()) {
        if (!cave_info.is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        for (const auto &flag : cave_info) {
            const auto it = CAVE_FLAG_TOKENS.find(flag.get<std::string>());
            if (it == CAVE_FLAG_TOKENS.end()) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            grid.cave_info |= it->second;
        }
    }

    const auto &monster = get_json_value(cell_data, "monster");
    if (monster.is_number_integer()) {
        grid.monster = static_cast<MONSTER_IDX>(monster.get<int>());
    } else if (monster.is_object()) {
        if (json_is_true(get_json_value(monster, "random"))) {
            grid.random |= RANDOM_MONSTER;
            grid.monster = static_cast<MONSTER_IDX>(get_json_value(monster, "oodLevel").is_number_integer() ? get_json_value(monster, "oodLevel").get<int>() : 0);
        } else {
            const auto &clone = get_json_value(monster, "cloneOf");
            if (clone.is_number_integer()) {
                grid.monster = static_cast<MONSTER_IDX>(-clone.get<int>());
            }
        }
    }

    const auto &object = get_json_value(cell_data, "object");
    if (object.is_number_integer()) {
        grid.object = static_cast<OBJECT_IDX>(object.get<int>());
    } else if (object.is_object()) {
        if (json_is_true(get_json_value(object, "random"))) {
            grid.random |= RANDOM_OBJECT;
            grid.object = static_cast<OBJECT_IDX>(get_json_value(object, "oodLevel").is_number_integer() ? get_json_value(object, "oodLevel").get<int>() : 0);
        } else if (json_is_true(get_json_value(object, "questReward"))) {
            out.object_is_quest_reward = true;
        }
    }

    const auto &ego = get_json_value(cell_data, "ego");
    if (ego.is_number_integer()) {
        grid.ego = i2enum<EgoType>(ego.get<int>());
    } else if (ego.is_object() && json_is_true(get_json_value(ego, "random"))) {
        grid.random |= RANDOM_EGO;
        const auto &id = get_json_value(ego, "id");
        if (id.is_number_integer()) {
            grid.ego = i2enum<EgoType>(id.get<int>());
        }
    }

    const auto &artifact = get_json_value(cell_data, "artifact");
    if (artifact.is_number_integer()) {
        grid.artifact = i2enum<FixedArtifactId>(artifact.get<int>());
    } else if (artifact.is_object()) {
        if (json_is_true(get_json_value(artifact, "random"))) {
            grid.random |= RANDOM_ARTIFACT;
            const auto &id = get_json_value(artifact, "id");
            if (id.is_number_integer()) {
                grid.artifact = i2enum<FixedArtifactId>(id.get<int>());
            }
        } else if (json_is_true(get_json_value(artifact, "questReward"))) {
            out.artifact_is_quest_reward = true;
        }
    }

    const auto &trap = get_json_value(cell_data, "trap");
    if (trap.is_string()) {
        try {
            grid.trap = terrains.get_terrain_id(trap.get<std::string>());
        } catch (const std::exception &) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
    } else if (trap.is_object() && json_is_true(get_json_value(trap, "random"))) {
        grid.random |= RANDOM_TRAP;
    }

    if (const auto err = info_set_integer(get_json_value(cell_data, "special"), grid.special, false); err != PARSE_ERROR_NONE) {
        return i2enum<parse_error_type>(err);
    }

    return PARSE_ERROR_NONE;
}

QuestReader::QuestReader(const nlohmann::json &quest_data, QuestType &quest, QuestFixedMap &fixed_map)
    : quest_data(quest_data)
    , quest(quest)
    , fixed_map(fixed_map)
{
}

int QuestReader::read() const
{
    if (!this->quest_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    if (const auto err = this->set_name(); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = this->set_definition(); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = this->set_descriptions(); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = this->set_legend(); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = this->set_maps(); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = this->set_starts(); err != PARSE_ERROR_NONE) {
        return err;
    }

    return PARSE_ERROR_NONE;
}

int QuestReader::set_name() const
{
    // info_set_string は {ja, en} オブジェクトを受け取り、ビルド言語に応じた文字列を格納する
    return info_set_string(get_json_value(this->quest_data, "name"), this->quest.name, true);
}

int QuestReader::set_definition() const
{
    const auto &definition = get_json_value(this->quest_data, "definition");
    if (!definition.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto &meta = this->fixed_map.metadata;
    meta.present = true;

    const auto &type = get_json_value(definition, "type");
    if (!type.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto kind_it = QUEST_KIND_TOKENS.find(type.get<std::string>());
    if (kind_it == QUEST_KIND_TOKENS.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    meta.type = enum2i(kind_it->second);

    if (const auto err = info_set_integer(get_json_value(definition, "level"), meta.level, true); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(definition, "numMon"), meta.num_mon, false); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(definition, "maxNum"), meta.max_num, false); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(definition, "dungeon"), meta.dungeon, false); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(definition, "monster"), meta.r_idx, false); err != PARSE_ERROR_NONE) {
        return err;
    }

    const auto &flags = get_json_value(definition, "flags");
    if (!flags.is_null()) {
        if (!flags.is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        for (const auto &flag : flags) {
            const auto it = QUEST_FLAG_TOKENS.find(flag.get<std::string>());
            if (it == QUEST_FLAG_TOKENS.end()) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            meta.flags |= it->second;
        }
    }

    const auto &reward = get_json_value(definition, "reward");
    if (!reward.is_null()) {
        const auto &artifact = get_json_value(reward, "artifact");
        if (artifact.is_number_integer()) {
            meta.reward_artifact = artifact.get<int>();
        }

        const auto &artifacts = get_json_value(reward, "artifacts");
        if (!artifacts.is_null()) {
            if (!artifacts.is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            for (const auto &candidate : artifacts) {
                if (candidate.is_number_integer()) {
                    this->fixed_map.reward_artifact_candidates.push_back(candidate.get<int>());
                }
            }
        }
    }

    return PARSE_ERROR_NONE;
}

int QuestReader::set_descriptions() const
{
    const auto &descriptions = get_json_value(this->quest_data, "descriptions");
    if (descriptions.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!descriptions.is_array()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    for (const auto &description : descriptions) {
        QuestDescriptionBlock block;

        const auto &when = get_json_value(description, "when");
        const auto &status_at_most = get_json_value(when, "statusAtMost");
        if (status_at_most.is_string()) {
            const auto it = QUEST_STATUS_TOKENS.find(status_at_most.get<std::string>());
            if (it == QUEST_STATUS_TOKENS.end()) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            block.status_at_most = it->second;
        }

        const auto &status = get_json_value(when, "status");
        if (status.is_string()) {
            const auto it = QUEST_STATUS_TOKENS.find(status.get<std::string>());
            if (it == QUEST_STATUS_TOKENS.end()) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            block.status_equals = it->second;
        }

        const auto &quest_type = get_json_value(when, "questType");
        if (quest_type.is_string()) {
            const auto it = QUEST_KIND_TOKENS.find(quest_type.get<std::string>());
            if (it == QUEST_KIND_TOKENS.end()) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            block.quest_type = it->second;
        }

        const auto &text = get_json_value(description, "text");
        read_string_lines(get_json_value(text, "ja"), block.lines_ja, true);
        read_string_lines(get_json_value(text, "en"), block.lines_en, true);

        this->fixed_map.descriptions.push_back(std::move(block));
    }

    return PARSE_ERROR_NONE;
}

int QuestReader::set_legend() const
{
    const auto &legend = get_json_value(this->quest_data, "legend");
    if (legend.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!legend.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    for (const auto &[symbol, cell_data] : legend.items()) {
        if (symbol.size() != 1) {
            return PARSE_ERROR_GENERIC;
        }

        QuestLegendCell cell;
        if (const auto err = parse_quest_legend_cell(cell_data, cell); err != PARSE_ERROR_NONE) {
            return err;
        }
        this->fixed_map.legend.insert_or_assign(symbol.front(), cell);
    }

    return PARSE_ERROR_NONE;
}

int QuestReader::set_maps() const
{
    const auto &map = get_json_value(this->quest_data, "map");
    if (!map.is_null()) {
        if (!map.is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        std::vector<std::string> rows;
        read_string_lines(map, rows, false);
        this->fixed_map.maps.push_back(std::move(rows));
        return PARSE_ERROR_NONE;
    }

    const auto &variants = get_json_value(this->quest_data, "mapVariants");
    if (!variants.is_null()) {
        if (!variants.is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        for (const auto &variant : variants) {
            if (!variant.is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            std::vector<std::string> rows;
            read_string_lines(variant, rows, false);
            this->fixed_map.maps.push_back(std::move(rows));
        }
    }

    return PARSE_ERROR_NONE;
}

int QuestReader::set_starts() const
{
    const auto &start = get_json_value(this->quest_data, "start");
    if (!start.is_null()) {
        QuestStartPosition position;
        if (const auto err = info_set_integer(get_json_value(start, "y"), position.y, true); err != PARSE_ERROR_NONE) {
            return err;
        }
        if (const auto err = info_set_integer(get_json_value(start, "x"), position.x, true); err != PARSE_ERROR_NONE) {
            return err;
        }
        this->fixed_map.starts.push_back(position);
        return PARSE_ERROR_NONE;
    }

    const auto &variants = get_json_value(this->quest_data, "startVariants");
    if (!variants.is_null()) {
        if (!variants.is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        for (const auto &variant : variants) {
            QuestStartPosition position;
            const auto &leaving_quest = get_json_value(variant, "leavingQuest");
            if (leaving_quest.is_number_integer()) {
                position.leaving_quest = leaving_quest.get<int>();
            }
            if (const auto err = info_set_integer(get_json_value(variant, "y"), position.y, true); err != PARSE_ERROR_NONE) {
                return err;
            }
            if (const auto err = info_set_integer(get_json_value(variant, "x"), position.x, true); err != PARSE_ERROR_NONE) {
                return err;
            }
            this->fixed_map.starts.push_back(position);
        }
    }

    return PARSE_ERROR_NONE;
}
