﻿#include "info-reader/dungeon-reader.h"
#include "dungeon/dungeon.h"
#include "grid/feature.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/feature-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用) /
 * Grab one flag for a dungeon type from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_dungeon_flag(dungeon_type *d_ptr, std::string_view what)
{
    if (EnumClassFlagGroup<DF>::grab_one_flag(d_ptr->flags, d_info_flags, what))
        return true;

    msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_basic_monster_flag(dungeon_type *d_ptr, std::string_view what)
{
    if (info_grab_one_flag(d_ptr->mflags1, r_info_flags1, what))
        return true;

    if (info_grab_one_flag(d_ptr->mflags2, r_info_flags2, what))
        return true;

    if (info_grab_one_flag(d_ptr->mflags3, r_info_flags3, what))
        return true;

    if (info_grab_one_flag(d_ptr->mflags7, r_info_flags7, what))
        return true;

    if (info_grab_one_flag(d_ptr->mflags8, r_info_flags8, what))
        return true;

    if (info_grab_one_flag(d_ptr->mflags9, r_info_flags9, what))
        return true;

    if (info_grab_one_flag(d_ptr->mflagsr, r_info_flagsr, what))
        return true;

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_spell_monster_flag(dungeon_type *d_ptr, std::string_view what)
{
    if (EnumClassFlagGroup<RF_ABILITY>::grab_one_flag(d_ptr->m_ability_flags, r_info_ability_flags, what))
        return true;

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief ダンジョン情報(d_info)のパース関数 /
 * Initialize the "d_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_d_info(std::string_view buf, angband_header *head)
{
    static dungeon_type *d_ptr = NULL;
    const auto &tokens = str_split(buf, ':', false);

    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        auto i = std::stoi(tokens[1]);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        if (i >= head->info_num)
            return PARSE_ERROR_OUT_OF_BOUNDS;

        error_idx = i;
        d_ptr = &d_info[static_cast<int>(i)];
#ifdef JP
        d_ptr->name = tokens[2];
#endif
    } else if (!d_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if (tokens[0] == "E") {
        // E:name_en
#ifndef JP
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        d_ptr->name = tokens[1];
#endif
    } else if (tokens[0] == "D") {
        // D:text_ja
        // D:$text_en
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
#ifdef JP
        if (tokens[1][0] == '$')
            return PARSE_ERROR_NONE;
        d_ptr->text.append(buf.substr(2));
#else
        if (tokens[1][0] != '$')
            return PARSE_ERROR_NONE;
        d_ptr->text.append(buf.substr(3));
#endif
    } else if (tokens[0] == "W") {
        // W:min_level:max_level:(1):mode:(2):(3):(4):(5):prob_pit:prob_nest
        // (1)minimum player level (unused)
        // (2)minimum level of allocating monster
        // (3)maximum probability of level boost of allocation monster
        // (4)maximum probability of dropping good objects
        // (5)maximum probability of dropping great objects
        if (tokens.size() < 11)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(d_ptr->mindepth, tokens[1]);
        info_set_value(d_ptr->maxdepth, tokens[2]);
        info_set_value(d_ptr->min_plev, tokens[3]);
        info_set_value(d_ptr->mode, tokens[4]);
        info_set_value(d_ptr->min_m_alloc_level, tokens[5]);
        info_set_value(d_ptr->max_m_alloc_chance, tokens[6]);
        info_set_value(d_ptr->obj_good, tokens[7]);
        info_set_value(d_ptr->obj_great, tokens[8]);
        info_set_value(d_ptr->pit, tokens[9], 16);
        info_set_value(d_ptr->nest, tokens[10], 16);
    } else if (tokens[0] == "P") {
        // P:wild_y:wild_x
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(d_ptr->dy, tokens[1]);
        info_set_value(d_ptr->dx, tokens[2]);
    } else if (tokens[0] == "L") {
        // L:floor_1:prob_1:floor_2:prob_2:floor_3:prob_3:tunnel_prob
        if (tokens.size() < DUNGEON_FEAT_PROB_NUM * 2 + 2)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        for (size_t i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            auto feat_idx = i * 2 + 1;
            auto per_idx = feat_idx + 1;
            d_ptr->floor[i].feat = f_tag_to_index(tokens[feat_idx]);
            if (d_ptr->floor[i].feat < 0)
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

            info_set_value(d_ptr->floor[i].percent, tokens[per_idx]);
        }

        auto tunnel_idx = DUNGEON_FEAT_PROB_NUM * 2 + 1;
        info_set_value(d_ptr->tunnel_percent, tokens[tunnel_idx]);
    } else if (tokens[0] == "A") {
        // A:wall_1:prob_1:wall_2:prob_2:wall_3:prob_3:outer_wall:inner_wall:stream_1:stream_2
        if (tokens.size() < DUNGEON_FEAT_PROB_NUM * 2 + 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            auto feat_idx = i * 2 + 1;
            auto prob_idx = feat_idx + 1;
            d_ptr->fill[i].feat = f_tag_to_index(tokens[feat_idx]);
            if (d_ptr->fill[i].feat < 0)
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

            info_set_value(d_ptr->fill[i].percent, tokens[prob_idx]);
        }

        auto idx = DUNGEON_FEAT_PROB_NUM * 2 + 1;
        d_ptr->outer_wall = f_tag_to_index(tokens[idx++]);
        if (d_ptr->outer_wall < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        d_ptr->inner_wall = f_tag_to_index(tokens[idx++]);
        if (d_ptr->inner_wall < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        d_ptr->stream1 = f_tag_to_index(tokens[idx++]);
        if (d_ptr->stream1 < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        d_ptr->stream2 = f_tag_to_index(tokens[idx]);
        if (d_ptr->stream2 < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    } else if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0)
                continue;

            const auto &f_tokens = str_split(f, '_');
            if (f_tokens.size() == 3) {
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "ARTIFACT") {
                    info_set_value(d_ptr->final_artifact, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "OBJECT") {
                    info_set_value(d_ptr->final_object, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "GUARDIAN") {
                    info_set_value(d_ptr->final_guardian, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "MONSTER" && f_tokens[1] == "DIV") {
                    info_set_value(d_ptr->special_div, f_tokens[2]);
                    continue;
                }
            }

            if (!grab_one_dungeon_flag(d_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    } else if (tokens[0] == "M") {
        // M:monsterflags
        if (tokens.size() < 2)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0)
                continue;

            const auto &m_tokens = str_split(f, '_');
            if (m_tokens[0] == "R" && m_tokens[1] == "CHAR") {
                if (m_tokens[2].size() > 4)
                    return PARSE_ERROR_GENERIC;

                strcpy(d_ptr->r_char, m_tokens[2].c_str());
                continue;
            }

            if (!grab_one_basic_monster_flag(d_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    } else if (tokens[0] == "S") {
        // S: flags
        if (tokens.size() < 2)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0)
                continue;

            const auto &s_tokens = str_split(f, '_');
            if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
                if (s_tokens[0] != "1")
                    return PARSE_ERROR_GENERIC;
                continue; //!< r_info.txtからのコピペ対策
            }

            if (!grab_one_spell_monster_flag(d_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    }
    else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return 0;
}
