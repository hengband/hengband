#include "info-reader/general-parser.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/definition-hash-data.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/tokenizer.h"
#include "object-enchant/trg-types.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "realm/realm-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/building-type-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include <string>

dungeon_grid letter[255];

void dungeon_grid::set_terrain_id(TerrainTag tag)
{
    this->feature = TerrainList::get_instance().get_terrain_id(tag);
}

void dungeon_grid::set_trap_id(TerrainTag tag)
{
    this->trap = TerrainList::get_instance().get_terrain_id(tag);
}

/*!
 * @brief パース関数に基づいてデータファイルからデータを読み取る
 * @param ifs 読み取りに使うファイルストリーム
 * @param dhdt ヘッダ種別
 * @param parse_info_txt_line パース関数
 * @return エラーコード, エラー行番号, エラーが起きた行の内容
 */
std::tuple<int, int, std::string> init_info_txt(std::ifstream &ifs, DefinitionHashDataType dhdt, Parser parse_info_txt_line)
{
    error_idx = -1;
    auto error_line = 0;

    util::SHA256 sha256;
    std::string line;
    while (std::getline(ifs, line)) {
        line = utf8_to_local(line);
        const std::string_view sv = line;
        error_line++;
        if (sv.empty() || sv.starts_with('#')) {
            continue;
        }

        if (!sv.substr(1).starts_with(':')) {
            return { PARSE_ERROR_GENERIC, error_line, line };
        }

        if (sv.starts_with('V')) {
            continue;
        }

        // N/D/J行はハッシュから除外（日本語対応）
        if (!sv.starts_with('N') && !sv.starts_with('D') && !sv.starts_with('J')) {
            sha256.update(sv);
        }

        if (auto err = parse_info_txt_line(sv); err != 0) {
            return { err, error_line, line };
        }
    }

    DefinitionHashData::get_instance().set_digest(dhdt, sha256.digest());
    return { PARSE_ERROR_NONE, error_line, "" };
}

/*!
 * @brief 地形情報の「F:」情報をパースする
 * Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid
 * @param floor フロアへの参照
 * @param buf 解析文字列
 * @return エラーコード
 */
parse_error_type parse_line_feature(const FloorType &floor, std::string_view buf)
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    const auto tokens = tokenize(buf.substr(2), 9);
    const auto tokens_size = tokens.size();
    if (tokens_size <= 1) {
        return PARSE_ERROR_GENERIC;
    }

    const auto &terrains = TerrainList::get_instance();
    const int index = tokens.at(0).at(0);
    auto &one_letter = letter[index];
    one_letter.set_terrain_id(TerrainTag::NONE);
    one_letter.monster = 0;
    one_letter.object = 0;
    one_letter.ego = EgoType::NONE;
    one_letter.artifact = FixedArtifactId::NONE;
    one_letter.set_trap_id(TerrainTag::NONE);
    one_letter.cave_info = 0;
    one_letter.special = 0;
    one_letter.random = RANDOM_NONE;

    switch (tokens_size) {
    case 9:
        one_letter.special = static_cast<short>(std::stoi(tokens.at(8)));
        [[fallthrough]];
    case 8: {
        const auto &token = tokens.at(7);
        if (token == "*") {
            one_letter.random |= RANDOM_TRAP;
        } else {
            try {
                one_letter.trap = terrains.get_terrain_id(token);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }
        }
    }
        [[fallthrough]];
    case 7: {
        const auto &token = tokens.at(6);
        if (token.starts_with('*')) {
            one_letter.random |= RANDOM_ARTIFACT;
            if (token.length() > 1) {
                one_letter.artifact = i2enum<FixedArtifactId>(std::stoi(token.substr(1)));
            }
        } else if (token.starts_with('!')) {
            if (floor.is_in_quest()) {
                const auto &quest = QuestList::get_instance().get_quest(floor.quest_number);
                one_letter.artifact = quest.get_reward().value_or(FixedArtifactId::NONE);
            }
        } else {
            one_letter.artifact = i2enum<FixedArtifactId>(std::stoi(token));
        }
    }
        [[fallthrough]];
    case 6: {
        const auto &token = tokens.at(5);
        if (token.starts_with('*')) {
            one_letter.random |= RANDOM_EGO;
            if (token.length() > 1) {
                one_letter.ego = i2enum<EgoType>(std::stoi(token.substr(1)));
            }
        } else {
            one_letter.ego = i2enum<EgoType>(std::stoi(token));
        }
    }
        [[fallthrough]];
    case 5: {
        const auto &token = tokens.at(4);
        if (token.starts_with('*')) {
            one_letter.random |= RANDOM_OBJECT;
            if (token.length() > 1) {
                one_letter.object = static_cast<short>(std::stoi(token.substr(1)));
            }
        } else if (token.starts_with('!')) {
            if (floor.is_in_quest()) {
                const auto &quests = QuestList::get_instance();
                const auto &quest = quests.get_quest(floor.quest_number);
                if (quest.has_reward()) {
                    if (!quest.is_reward_instant_artifact()) {
                        one_letter.object = quest.get_reward_bi_id();
                    }
                }
            }
        } else {
            one_letter.object = static_cast<short>(std::stoi(token));
        }
    }
        [[fallthrough]];
    case 4: {
        const auto &token = tokens.at(3);
        if (token.starts_with('*')) {
            one_letter.random |= RANDOM_MONSTER;
            if (token.length() > 1) {
                one_letter.monster = static_cast<short>(std::stoi(token.substr(1)));
            }
        } else if (token.starts_with('c')) {
            if (token.length() < 2) {
                return PARSE_ERROR_GENERIC;
            }
            one_letter.monster = -std::stoi(token.substr(1));
        } else {
            one_letter.monster = static_cast<short>(std::stoi(token));
        }
    }
        [[fallthrough]];
    case 3: {
        const auto &token = tokens.at(2);
        one_letter.cave_info = static_cast<uint32_t>(std::stoi(token));
    }
        [[fallthrough]];
    case 2: {
        const auto &token = tokens.at(1);
        if (token == "*") {
            one_letter.random |= RANDOM_FEATURE;
            break;
        }

        try {
            one_letter.feature = terrains.get_terrain_id(token);
            break;
        } catch (const std::exception &) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
    }
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief 地形情報の「B:」情報をパースする
 * Process "B:<Index>:<Command>:..." -- Building definition
 * @param buf 解析文字列
 * @return エラーコード
 */
parse_error_type parse_line_building(std::string_view buf)
{
#ifdef JP
    if (buf[2] == '$') {
        return PARSE_ERROR_NONE;
    }
    auto s = buf.substr(2);
#else
    if (buf[2] != '$') {
        return PARSE_ERROR_NONE;
    }
    auto s = buf.substr(3);
#endif
    const auto index = std::stoi(std::string(s));
    const auto colon_pos = s.find(':');
    if (colon_pos == std::string_view::npos) {
        return PARSE_ERROR_GENERIC;
    }
    s.remove_prefix(colon_pos + 1);
    if (s.empty()) {
        return PARSE_ERROR_GENERIC;
    }
    switch (s[0]) {
    case 'N': {
        const auto tokens = tokenize(s.substr(2), 3);
        if (tokens.size() == 3) {
            auto &building = buildings[index];
            angband_strcpy(building.name, tokens[0], sizeof(building.name));
            angband_strcpy(building.owner_name, tokens[1], sizeof(building.owner_name));
            angband_strcpy(building.owner_race, tokens[2], sizeof(building.owner_race));
            break;
        }

        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    case 'A': {
        const auto tokens = tokenize(s.substr(2), 7);
        if (tokens.size() >= 7) {
            const auto action_index = std::stoi(tokens[0]);
            auto &building = buildings[index];
            angband_strcpy(building.act_names[action_index], tokens[1], sizeof(building.act_names[action_index]));
            building.member_costs[action_index] = std::stoi(tokens[2]);
            building.other_costs[action_index] = std::stoi(tokens[3]);
            building.letters[action_index] = tokens[4][0];
            building.actions[action_index] = static_cast<int16_t>(std::stoi(tokens[5]));
            building.action_restr[action_index] = static_cast<int16_t>(std::stoi(tokens[6]));
            break;
        }

        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    case 'C': {
        const auto tokens = tokenize(s.substr(2), PLAYER_CLASS_TYPE_MAX);
        for (size_t i = 0; i < PLAYER_CLASS_TYPE_MAX; i++) {
            buildings[index].member_class[i] = (i < tokens.size()) ? std::stoi(tokens[i]) : 1;
        }

        break;
    }
    case 'R': {
        const auto tokens = tokenize(s.substr(2), MAX_RACES);
        for (size_t i = 0; i < MAX_RACES; i++) {
            buildings[index].member_race[i] = (i < tokens.size()) ? std::stoi(tokens[i]) : 1;
        }

        break;
    }
    case 'M': {
        const auto tokens = tokenize(s.substr(2), MAX_MAGIC);
        for (size_t i = 0; i < MAX_MAGIC; i++) {
            buildings[index].member_realm[i + 1] = ((i < tokens.size()) ? static_cast<int16_t>(std::stoi(tokens[i])) : 1);
        }

        break;
    }
    case 'Z': {
        break;
    }
    default: {
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }
    }

    return PARSE_ERROR_NONE;
}
