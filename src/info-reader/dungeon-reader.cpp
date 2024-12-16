#include "info-reader/dungeon-reader.h"
#include "grid/feature.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <span>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<DungeonFeatureType>::grab_one_flag(dungeon.flags, dungeon_flags, what)) {
        return true;
    }

    msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what)
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

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(dungeon.mon_ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief ダンジョン情報(DungeonsDefinition)のパース関数 /
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_dungeons_info(std::string_view buf, angband_header *)
{
    const auto &tokens = str_split(buf, ':', false);
    const auto &terrains = TerrainList::get_instance();

    // N:index:name_ja
    auto &dungeons = DungeonList::get_instance();
    if (tokens[0] == "N") {
        if (tokens.size() < 3 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto i = std::stoi(tokens[1]);
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        error_idx = i;
        DungeonDefinition dungeon;
#ifdef JP
        dungeon.name = tokens[2];
#endif
        dungeons.emplace(i2enum<DungeonId>(i), std::move(dungeon));
        return PARSE_ERROR_NONE;
    }

    if (dungeons.empty()) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    }

    // E:name_en
    auto &[dungeon_id, dungeon] = *dungeons.rbegin();
    if (tokens[0] == "E") {
#ifndef JP
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        dungeon->name = tokens[1];
#endif
        return PARSE_ERROR_NONE;
    }

    // D:text_ja
    // D:$text_en
    if (tokens[0] == "D") {
        if (tokens.size() < 2 || buf.length() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
#ifdef JP
        if (buf[2] == '$') {
            return PARSE_ERROR_NONE;
        }

        dungeon->text.append(buf.substr(2));
#else
        if (buf[2] != '$') {
            return PARSE_ERROR_NONE;
        }

        if (buf.length() == 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        append_english_text(dungeon->text, buf.substr(3));
#endif
        return PARSE_ERROR_NONE;
    }

    // W:min_level:max_level:(1):mode:(2):(3):(4):(5):prob_pit:prob_nest
    // (1)minimum player level (unused)
    // (2)minimum level of allocating monster
    // (3)maximum probability of level boost of allocation monster
    // (4)maximum probability of dropping good objects
    // (5)maximum probability of dropping great objects
    if (tokens[0] == "W") {
        if (tokens.size() < 11) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(dungeon->mindepth, tokens[1]);
        info_set_value(dungeon->maxdepth, tokens[2]);
        info_set_value(dungeon->min_plev, tokens[3]);
        info_set_value(dungeon->mode, tokens[4]);
        info_set_value(dungeon->min_m_alloc_level, tokens[5]);
        info_set_value(dungeon->max_m_alloc_chance, tokens[6]);
        info_set_value(dungeon->obj_good, tokens[7]);
        info_set_value(dungeon->obj_great, tokens[8]);
        info_set_value(dungeon->pit, tokens[9], 16);
        info_set_value(dungeon->nest, tokens[10], 16);
        return PARSE_ERROR_NONE;
    }

    // P:wild_y:wild_x
    if (tokens[0] == "P") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(dungeon->dy, tokens[1]);
        info_set_value(dungeon->dx, tokens[2]);
        return PARSE_ERROR_NONE;
    }

    // L:floor_1:prob_1:floor_2:prob_2:floor_3:prob_3:tunnel_prob
    if (tokens[0] == "L") {
        if (tokens.size() < DUNGEON_FEAT_PROB_NUM * 2 + 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (size_t i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            auto feat_idx = i * 2 + 1;
            auto per_idx = feat_idx + 1;
            try {
                dungeon->floor[i].feat = terrains.get_terrain_id_by_tag(tokens[feat_idx]);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }

            info_set_value(dungeon->floor[i].percent, tokens[per_idx]);
        }

        auto tunnel_idx = DUNGEON_FEAT_PROB_NUM * 2 + 1;
        info_set_value(dungeon->tunnel_percent, tokens[tunnel_idx]);
        return PARSE_ERROR_NONE;
    }

    // A:wall_1:prob_1:wall_2:prob_2:wall_3:prob_3:outer_wall:inner_wall:stream_1:stream_2
    if (tokens[0] == "A") {
        if (tokens.size() < DUNGEON_FEAT_PROB_NUM * 2 + 5) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            auto feat_idx = i * 2 + 1;
            auto prob_idx = feat_idx + 1;
            try {
                dungeon->fill[i].feat = terrains.get_terrain_id_by_tag(tokens[feat_idx]);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }

            info_set_value(dungeon->fill[i].percent, tokens[prob_idx]);
        }

        try {
            const std::span tags(tokens.begin() + DUNGEON_FEAT_PROB_NUM * 2 + 1, 4);
            dungeon->outer_wall = terrains.get_terrain_id_by_tag(tags[0]);
            dungeon->inner_wall = terrains.get_terrain_id_by_tag(tags[1]);
            dungeon->stream1 = terrains.get_terrain_id_by_tag(tags[2]);
            dungeon->stream2 = terrains.get_terrain_id_by_tag(tags[3]);
            return PARSE_ERROR_NONE;
        } catch (const std::exception &) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
    }

    // F:flags
    if (tokens[0] == "F") {
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto &f_tokens = str_split(f, '_');
            if (f_tokens.size() == 3) {
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "ARTIFACT") {
                    info_set_value(dungeon->final_artifact, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "OBJECT") {
                    info_set_value(dungeon->final_object, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "GUARDIAN") {
                    info_set_value(dungeon->final_guardian, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "MONSTER" && f_tokens[1] == "DIV") {
                    info_set_value(dungeon->special_div, f_tokens[2]);
                    continue;
                }
            }

            if (!grab_one_dungeon_flag(*dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    // M:Monster flags
    if (tokens[0] == "M") {
        if (tokens[1] == "X") {
            if (tokens.size() < 3) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            uint32_t sex;
            if (!info_grab_one_const(sex, r_info_sex, tokens[2])) {
                return PARSE_ERROR_INVALID_FLAG;
            }

            dungeon->mon_sex = static_cast<MonsterSex>(sex);
            return 0;
        }

        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.empty()) {
                continue;
            }

            const auto &m_tokens = str_split(f, '_');
            if (m_tokens[0] == "R" && m_tokens[1] == "CHAR") {
                dungeon->r_chars.insert(dungeon->r_chars.end(), m_tokens[2].begin(), m_tokens[2].end());
                continue;
            }

            if (!grab_one_basic_monster_flag(*dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    // S: flags
    if (tokens[0] == "S") {
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.empty()) {
                continue;
            }

            const auto &s_tokens = str_split(f, '_');
            if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
                if (s_tokens[0] != "1") {
                    return PARSE_ERROR_GENERIC;
                }

                continue; //!< @details MonsterRaceDefinitions.jsonc からのコピペ対策
            }

            if (!grab_one_spell_monster_flag(*dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_UNDEFINED_DIRECTIVE;
}
